/*
 * Playground+ - dynamic.c
 * Dynamic files
 * ---------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* just in case youre on a dippy system */

#ifndef S_IRUSR
#define S_IRUSR 00400
#endif
#ifndef S_IWUSR
#define S_IWUSR 00200
#endif


#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"

/* keep the compiler happy */

extern char *end_string(char *);
extern char *store_int(char *, int);
extern char *get_int(int *, char *);
extern dfile *room_df;
extern saved_player **saved_hash[];

/* throw the keylist to disk */

void dynamic_key_sync(dfile * df)
{
  int fd, length;
  char *oldstack, *to;
  oldstack = stack;

  store_int((char *) df->keylist, df->first_free_block);
  store_int((char *) (df->keylist + 1), df->first_free_key);

/* doing a straight open could mean losing the key data
   dont risk it by moving the file first
   this is grossly slow, so provide a means of escaping it */

  sprintf(oldstack, "files/%s/keys", df->fname);
  if (!(sys_flags & SECURE_DYNAMIC))
  {
    stack = end_string(oldstack);
    to = stack;
    sprintf(to, "files/%s/keys.b", df->fname);
    rename(oldstack, to);
  }
  fd = open(oldstack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd < 0)
    handle_error("Failed to open key file");
  length = (df->nkeys + 1) * 8;
  if (write(fd, df->keylist, length) != length)
    handle_error("Failed to write key data");
  close(fd);

  stack = oldstack;
}


/* set up a dfile structure */

dfile *dynamic_init(char *fil, int granularity)
{
  dfile *df;
  char *oldstack;
  int fd, length;
  oldstack = stack;

  if (sys_flags & VERBOSE)
  {
    sprintf(oldstack, "Loading dynamic file '%s'", fil);
    stack = end_string(oldstack);
    log("sync", oldstack);
    stack = oldstack;
  }
  df = (dfile *) MALLOC(sizeof(dfile));
  memset(df, 0, sizeof(dfile));
  strcpy(df->fname, fil);
  df->granularity = granularity;

  sprintf(oldstack, "files/%s/keys", df->fname);
  fd = open(oldstack, O_RDONLY | O_NDELAY);

  /* just in case this is the first time round */

  if (fd < 0)
  {

    sprintf(oldstack, "Failed to load '%s' keys (creating)", df->fname);
    stack = end_string(oldstack);
    log("error", oldstack);
    stack = oldstack;
    df->first_free_block = 0;
    df->first_free_key = 0;
    df->nkeys = 0;
    df->keylist = 0;
  }
  else
  {
    length = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    if ((length % 8) != 0)
      handle_error("Corrupt key data");
    df->nkeys = (length / 8) - 1;
    df->keylist = (int *) MALLOC(length);
    if (read(fd, df->keylist, length) != length)
      handle_error("Failed to read keys");
    close(fd);
    (void) get_int(&(df->first_free_block), (char *) df->keylist);
    (void) get_int(&(df->first_free_key), (char *) (df->keylist + 1));
  }

  /* keep the data file open all the time */

  sprintf(oldstack, "files/%s/data", df->fname);
  df->data_fd = open(oldstack, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (df->data_fd < 0)
    handle_error("Failed to open dynamic data file");
  lseek(df->data_fd, 0, SEEK_SET);
  stack = oldstack;
  return df;
}

/* free up a dfile structure */

void dynamic_close(dfile * df)
{
  dynamic_key_sync(df);
  close(df->data_fd);
  FREE(df->keylist);
  FREE(df);
}



/* get the block and length info from the keylist */

int convert_key(dfile * df, int key, int *block, int *length)
{
  if ((key <= 0) || (key > (df->nkeys - 1)))
    return 0;
  key *= 2;
  (void) get_int(block, (char *) (df->keylist + key));
  if (*block <= 0)
    return 0;
  (void) get_int(length, (char *) (df->keylist + key + 1));
  if (*length <= 0)
    return 0;
  return 1;
}

/* grab a block from the data file */

int load_block(dfile * df, int *block, char *data)
{
  if (*block <= 0)
    return 0;
  dynamic_seek_block(df, *block);
  if (read(df->data_fd, data, 4) != 4)
    handle_error("Failed to read next block 1");
  get_int(block, data);
  if (read(df->data_fd, data, df->granularity - 4) != (df->granularity - 4))
    handle_error("Failed to read block");
  return 1;
}

/* load an entire entry from the data file */

int dynamic_load(dfile * df, int key, char *data)
{
  int block, length, l;
/*  printf("Dynamic load key=%d\n",key);  */
  if (!convert_key(df, key, &block, &length))
    return -1;
  l = length;
/*    printf("DL block %d\n",block);  */
  while (l > 0)
  {
    if (!load_block(df, &block, data))
      handle_error("Failed to load block of data");
/*    printf("DL block %d\n",block);  */
    l -= (df->granularity - 4);
    data += (df->granularity - 4);
  }
  return length;
}

/* returns a free key, if necessary it will go and create
   some new key space (in chunks of 100 keys at a time) */

int dynamic_find_free_key(dfile * df)
{
  int *newlist, oldnkeys, i, key;
  if (!df->first_free_key)
  {
    oldnkeys = df->nkeys;
    df->nkeys += 100;
    newlist = (int *) MALLOC((df->nkeys + 1) * 8);
    if (df->keylist)
    {
      memcpy(newlist + 2, df->keylist + 2, (oldnkeys + 1) * 8);
      FREE(df->keylist);
    }
    df->keylist = newlist;
    df->first_free_key = oldnkeys + 1;
    newlist += df->first_free_key * 2;
    for (i = -df->first_free_key - 1; i >= -df->nkeys; i--)
    {
      newlist = (int *) store_int((char *) newlist, i);
      newlist = (int *) store_int((char *) newlist, 0);
    }
    newlist = (int *) store_int((char *) newlist, 0);
    newlist = (int *) store_int((char *) newlist, 0);
  }
  key = df->first_free_key;
  (void) get_int(&(df->first_free_key), (char *) (df->keylist + key * 2));
  df->first_free_key = -df->first_free_key;
  return key;
}

/* read in the index from the start of a block */

int dynamic_get_next_block(dfile * df, int block)
{
  int next_block, ret;
  if (block <= 0)
    return 0;
  dynamic_seek_block(df, block);
  ret = read(df->data_fd, stack, 4);
  if (!ret)
    return 0;
  if (ret != 4)
    handle_error("Failed to read next block 2");
  get_int(&next_block, stack);
  return next_block;
}

/* go through all the blocks in a file and free them up */

void dynamic_free(dfile * df, int key)
{
  int block, length, next_block;

  if (!convert_key(df, key, &block, &length))
    return;
  (void) store_int((char *) (df->keylist + key * 2), -df->first_free_key);
  (void) store_int((char *) (df->keylist + key * 2 + 1), 0);
  df->first_free_key = key;

/* simply keep adding blocks onto the top of the free list */

  while (length > 0)
  {
    if (block <= 0)
      return;
    next_block = dynamic_get_next_block(df, block);
    dynamic_seek_block(df, block);
    store_int(stack, -df->first_free_block);
    if (write(df->data_fd, stack, 4) != 4)
      handle_error("Failed to write next block");
    df->first_free_block = block;
    block = next_block;
    length -= (df->granularity - 4);
  }
  if (!(sys_flags & SECURE_DYNAMIC))
    dynamic_key_sync(df);
}

/* go hunting for a free block, if necessary, create enough
   space on the end of the file */

int dynamic_find_free_block(dfile * df)
{
  int length, new_block;
/*  printf("Called find_free_block ffb=%d\n",df->first_free_block); */
  if (df->first_free_block)
  {
    new_block = df->first_free_block;
    df->first_free_block = -dynamic_get_next_block(df, new_block);
  }
  else
  {
    length = lseek(df->data_fd, 0, SEEK_END);
    memset(stack, 0, df->granularity);
    if (!length)
    {
      if (write(df->data_fd, stack, df->granularity) != df->granularity)
	handle_error("Failed to make data block");
      length = df->granularity;
    }
    if (write(df->data_fd, stack, df->granularity) != df->granularity)
      handle_error("Failed to make data block");
    new_block = (length / df->granularity);
  }
  return new_block;
}

/* save an entry to the data file */

int dynamic_save(dfile * df, char *data, int l, int key)
{
  int block, length, next_block, blength, free_block;

/* key==0 means this is a new entry */

  if (!key)
  {
    key = dynamic_find_free_key(df);
    block = dynamic_find_free_block(df);
  }
  else if (!convert_key(df, key, &block, &length))
    return 0;
  (void) store_int((char *) (df->keylist + key * 2), block);
  (void) store_int((char *) (df->keylist + key * 2 + 1), l);
  blength = df->granularity - 4;
  while (l > 0)
  {
    next_block = dynamic_get_next_block(df, block);

    if (l > blength)
    {
      if (next_block <= 0)
	next_block = dynamic_find_free_block(df);
    }
    else
    {

/* can we free up any blocks at the end ? */

      free_block = next_block;
      while (free_block > 0)
      {
	next_block = dynamic_get_next_block(df, free_block);
	dynamic_seek_block(df, free_block);
	store_int(stack, -df->first_free_block);
	if (write(df->data_fd, stack, 4) != 4)
	  handle_error("Failed to write next block");
	df->first_free_block = free_block;
	free_block = next_block;
      }
      next_block = 0;
    }
    store_int(stack, next_block);
    dynamic_seek_block(df, block);
    if (write(df->data_fd, stack, 4) != 4)
      handle_error("Failed to write next block");
    if (write(df->data_fd, data, blength) != blength)
      handle_error("Failed to write block data");
    block = next_block;
    l -= blength;
    data += blength;
  }

  /* make sure the keylist is up to date */

  if (!(sys_flags & SECURE_DYNAMIC))
    dynamic_key_sync(df);
  return key;
}


/* some functions used during testing */

void dynamic_test_func_keys(player * p, char *str)
{
  dfile *df;
  char *oldstack;
  int key, *scan;
  oldstack = stack;
  df = room_df;

  sprintf(oldstack, "nkeys = %d\n", df->nkeys);
  stack = end_string(oldstack);
  tell_player(p, oldstack);
  stack = oldstack;

  scan = df->keylist;
  for (key = 0; key <= df->nkeys; key++)
  {
    sprintf(stack, "Key %d\tBlock pointer %d\tlength %d\n",
	    key, *scan, *(scan + 1));
    scan += 2;
    while (*stack)
      stack++;
  }
  *stack++ = 0;
  pager(p, oldstack);
  stack = oldstack;
}



void dynamic_test_func_blocks(player * p, char *str)
{
  dfile *df;
  char *oldstack;
  int block, total, next_block, length;
  oldstack = stack;
  df = room_df;
  length = lseek(df->data_fd, 0, SEEK_END);
  total = length / df->granularity;
  sprintf(oldstack, "Length = %d ( %d blocks )\n", length, total);
  stack = end_string(oldstack);
  tell_player(p, oldstack);
  stack = oldstack;
  for (block = 0; total; total--, block++)
  {
    next_block = dynamic_get_next_block(df, block);
    sprintf(stack, "Block %d\tNext block %d\n", block, next_block);
    while (*stack)
      stack++;
  }
  *stack++ = 0;
  pager(p, oldstack);
  stack = oldstack;
}

/* produce some (interesting ?) stats on the file */

void dynamic_dfstats(player * p, char *str)
{
  dfile *df;
  char *oldstack;
  int key_length = 0, block_length, *keyscan, count, tmp;
  int nblocks, fragments = 0, block, next_block, lost;
  oldstack = stack;
  df = room_df;

  keyscan = df->keylist + 2;
  for (count = 1; count <= df->nkeys; count++)
  {
    (void) get_int(&tmp, (char *) (keyscan + 1));
    key_length += tmp;
    keyscan += 2;
  }
  block_length = lseek(df->data_fd, 0, SEEK_END);
  nblocks = block_length / df->granularity;
  for (block = 1; block < nblocks; block++)
  {
    next_block = dynamic_get_next_block(df, block);
    if (next_block > 0 && next_block != (block + 1))
      fragments++;
  }
  lost = (block_length - 4 * nblocks) - key_length;
  sprintf(oldstack, " Total keys = %d\tTotal blocks = %d\n"
	  " Key length = %d\tBlock length = %d\n"
	  " Overhead = %d\n"
	  " Lost space = %d (%d%%)\n"
	  " Data/junk percentage = %d%%\n"
	  " Fragmentation = %d (%d%%)\n",
	  df->nkeys, nblocks, key_length, block_length,
	  4 * nblocks, lost, (lost * 100) / block_length,
	  (key_length * 100) / block_length,
	  fragments, (fragments * 100) / nblocks);
  stack = end_string(oldstack);
  tell_player(p, oldstack);
  stack = oldstack;
}

/* used in the defrag routine */

void transfer_key(dfile * old, dfile * new, int key)
{
  int data_length, new_key, start_block, old_length, new_length;
  char *oldstack;
  oldstack = stack;
  data_length = dynamic_load(old, key, stack);
  if (data_length <= 0)
    return;
  stack += data_length;
  new_key = dynamic_save(new, oldstack, data_length, 0);
  if (new_key <= 0)
    handle_error("Failed to write new file on defrag");
  (void) get_int(&old_length, (char *) (old->keylist + key * 2 + 1));
  (void) get_int(&new_length, (char *) (new->keylist + new_key * 2 + 1));
  if (old_length != new_length)
  {
    printf("key = %d\nnew_key = %d\nold_length = %d\nnew_length = %d\ndata_length = %d\n",
	   key, new_key, old_length, new_length, data_length);
    handle_error("lengths dont match on defrag");
  }
  (void) get_int(&start_block, (char *) (new->keylist + new_key * 2));
  (void) store_int((char *) (old->keylist + key * 2), start_block);
  stack = oldstack;
}

/* defragment the dynamic file */

void dynamic_defrag_rooms(player * p, char *str)
{
  dfile *old_df, *new_df;
  int length, old_nblocks, new_nblocks, key;
  char *oldstack;
  oldstack = stack;

  old_df = room_df;
  if (p != NULL)
    tell_player(p, " Defragging room file\n");

  sys_flags |= SECURE_DYNAMIC;

  length = lseek(old_df->data_fd, 0, SEEK_END);
  old_nblocks = length / old_df->granularity;

/* make sure there is no previous data around */

  unlink("files/defrag/data");
  unlink("files/defrag/keys");
  unlink("files/defrag/keys.b");

/* this will be the new defragged file */

  new_df = dynamic_init("defrag", old_df->granularity);

/* now step through each key in the old file and simply copy to the new */

  for (key = 1; key <= old_df->nkeys; key++)
    transfer_key(old_df, new_df, key);

/* find the length of the new file */

  length = lseek(new_df->data_fd, 0, SEEK_END);
  new_nblocks = length / new_df->granularity;

/* make sure the first free block corresponds properly */

  old_df->first_free_block = new_df->first_free_block;

/* close the two files */

  sys_flags &= ~SECURE_DYNAMIC;
  dynamic_close(old_df);
  dynamic_close(new_df);

/* copy the defragged data accross */

  unlink("files/rooms/data");
  rename("files/defrag/data", "files/rooms/data");

/* and reopen the new defragged file */

  room_df = dynamic_init("rooms", 256);

/* a couple of stats */

  sprintf(oldstack, " Defragging completed\n"
	  " Old blocks = %d\n"
	  " New blocks = %d\n", old_nblocks, new_nblocks);
  stack = end_string(oldstack);
  if (p != NULL)
    tell_player(p, oldstack);
  stack = oldstack;
}


/* Test the integrity of the rooms file and clean it up */

int dynamic_test_integrity(dfile * df, int key)
{
  int block, length, l;
  int next_block;
  if (!convert_key(df, key, &block, &length))
    return 0;
  l = length;
  while (l > 0)
  {
    if (block <= 0)
      return 0;
    dynamic_seek_block(df, block);
    if (read(df->data_fd, &next_block, 4) != 4)
      return 0;
    get_int((int *) &block, (char *) &next_block);
    l -= (df->granularity - 4);
  }
  return 1;
}


/* Entry point into the room validation routines */

void dynamic_validate_rooms(player * p, char *str)
{
  dfile *old_df, *new_df;
  char *oldstack;
  saved_player *scan, **hash;
  int i, j, rooms = 0, rejects = 0;
  room *r;

  oldstack = stack;

  old_df = room_df;

  if (p)
  {
    tell_player(p, " Validating room file\n");
  }
/* make sure there is no previous data around 
   use the defrag file cos it happens to be there already */

  unlink("files/defrag/data");
  unlink("files/defrag/keys");
  unlink("files/defrag/keys.b");

/* this will be the validated file */

  new_df = dynamic_init("defrag", old_df->granularity);

  sys_flags |= SECURE_DYNAMIC;

/* now step through each player and copy their rooms */

/* each letter */

  for (j = 0; j < 26; j++)
  {
    hash = saved_hash[j];

/* each hash */

    for (i = 0; i < HASH_SIZE; i++, hash++)
/* each player */

      for (scan = *hash; scan; scan = scan->next)
/* each room */

	for (r = scan->rooms; r; r = r->next)
	{

/* skip bad rooms */

	  if (dynamic_test_integrity(old_df, r->data_key))
	  {
	    room_df = old_df;
	    decompress_room(r);
	    r->data_key = 0;
	    r->flags |= ROOM_UPDATED;
	    room_df = new_df;
	    compress_room(r);
	  }
	  else
	    rejects += 1;
	  rooms += 1;
	}
  }

  sys_flags &= ~SECURE_DYNAMIC;
  dynamic_close(old_df);
  dynamic_close(new_df);

/* copy the validated data accross */

  unlink("files/rooms/data");
  unlink("files/rooms/keys");
  rename("files/defrag/data", "files/rooms/data");
  rename("files/defrag/keys", "files/rooms/keys");

/* and reopen the new room file */

  room_df = dynamic_init("rooms", 256);

/* a couple of stats */

  if (p)
  {
    sprintf(oldstack, " Validation complete\n Total Rooms checked: %d\n"
	    " Rooms rejected: %d\n", rooms, rejects);
    stack = end_string(oldstack);
    tell_player(p, oldstack);
    stack = oldstack;
  }
}
