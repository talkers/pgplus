/*
 * Playground+ - plists.c
 * Player code
 * ---------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <signal.h>
#include <setjmp.h>

#include "include/fix.h"
#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"
#include "compaction.c"


#ifdef IDENT
#include "include/ident.h"
#endif

#ifdef LAST
extern void stampLogin(char *);
#endif

/* interns */

void error_on_load(int), hcadmin_check_password(player *), convert_pg_to_pgplus(player *),
  player_flags(player *), motd(player *, char *);
int bad_player_load = 0;
char player_loading[MAX_NAME + 2];
jmp_buf jmp_env;
saved_player **saved_hash[26];
int update[26];

void newbie_check_name(player *, char *), link_to_program(player *);
saved_player **birthday_list = 0;
void colours_decide(player *, char *);
void syscolour_decide(player *, char *);
void login_timeout(player *);
saved_player *find_matched_sp_verbose(char *);
int poto;

#ifdef ROBOTS
#include "robot_plists.c"
#endif

#ifdef INTELLIBOTS
#include "robot_int.c"
#endif

/* crypts the password */

char *update_password(char *oldpass)
{
  char key[9];
  strncpy(key, oldpass, 8);
  return crypt(key, "SP");
}

void players_update_function(player * p)
{
  char em[MAX_EMAIL];

  if (p->email[0] == ' ')
    strcpy(em, "     <VALIDATED AS SET>");
  else
    strcpy(em, p->email);

  if (p->residency & ADMIN)
    printf("%-18s -- %-40s >(ADMIN)\n", p->name, em);
  else if (p->residency & SU)
    printf("%-18s -- %-40s >(SU)\n", p->name, em);
  else if (p->residency & PSU)
    printf("%-18s -- %-40s >(PSEUDO)\n", p->name, em);
  else
    printf("%-18s -- %s\n", p->name, em);

}

void interesting_data_function(player * p)
{
  printf("%-17s -- %-3d %-3d -- %d\n", p->name, p->max_list, p->max_rooms, p->pennies);
}

void update_url_links_function(player * p)
{
  if (p->alt_email[0])
    printf("<A HREF = \"%s\"> %s </A>\n", p->alt_email, p->name);
}

void flags_update_function(player * p)
{
  printf("%-18s -- %-40s\n", p->name, bit_string(p->residency));
}

void rooms_update_function(player * p)
{
  room *r;
  saved_player *sp;
  sp = p->saved;
  r = sp->rooms;
  while (r)
  {
    if (r->flags & OPEN)
    {
      decompress_room(r);
      printf("-=> %s.%s (%s)\n", r->owner->lower_name, r->id, r->name);
      if (r->exits.where)
	printf(r->exits.where);
    }
    r = r->next;
  }
}


void do_update(int rooms)
{
  player *p;
  saved_player *scan, **hash;
  int i, j, fd;

  fd = open("/dev/null", O_WRONLY);

  p = (player *) MALLOC(sizeof(player));

  for (j = 0; j < 26; j++)
  {
    printf(" -=*> Updating pfile group '%c'\n", j + 'a');
    hash = saved_hash[j];
    for (i = 0; i < HASH_SIZE; i++, hash++)
    {
      for (scan = *hash; scan; scan = scan->next)
      {
	if (scan->residency != SYSTEM_ROOM
	    && (scan->residency != BANISHED) && (scan->residency != BANISHD))
	{
	  memset((char *) p, 0, sizeof(player));
	  p->fd = fd;
	  p->script = 0;
	  p->location = (room *) - 1;
	  restore_player(p, scan->lower_name);
	  if (rooms == 3)
	    update_url_links_function(p);
	  else if (rooms == 4)
	    interesting_data_function(p);
	  else if (rooms)
	    rooms_update_function(p);
	  else if (sys_flags & UPDATE)
	    players_update_function(p);
	  else
	    flags_update_function(p);
	  save_player(p);
	}
      }
    }
  }
  close(fd);

  if (rooms == 2)
    printf("0 0 noone\n");
}


/* return the top player in a hash list */

saved_player *find_top_player(char c, int h)
{
  if ((c < 0) || (c > 25))
    return 0;
  if ((h < 0) || (h > HASH_SIZE))
    return 0;
  return (*(saved_hash[(int) c] + h));
}

/* birthdays !!! */

void do_birthdays()
{
  player *p;
  saved_player *scan, **hash, **list;
  int i, j, fd;
  time_t t;
  struct tm *date, *bday;
  char *oldstack;

  fd = open("/dev/null", O_WRONLY);

  oldstack = stack;
  align(stack);
  list = (saved_player **) stack;

  t = time(0);
  date = localtime(&t);

  p = (player *) MALLOC(sizeof(player));

  for (j = 0; j < 26; j++)
  {
    hash = saved_hash[j];
    for (i = 0; i < HASH_SIZE; i++, hash++)
      for (scan = *hash; scan; scan = scan->next)
	if (scan->residency != SYSTEM_ROOM
	 && (scan->residency != BANISHED) && (!(scan->residency & BANISHD)))
	{
	  memset((char *) p, 0, sizeof(player *));
	  restore_player(p, scan->lower_name);
	  bday = localtime((time_t *) & (p->birthday));
	  if ((bday->tm_mon == date->tm_mon) &&
	      (bday->tm_mday == date->tm_mday))
	  {
	    *((saved_player **) stack) = scan;
	    stack += sizeof(saved_player *);
	    p->age++;
	    save_player(p);
	  }
	}
  }
  *((saved_player **) stack) = 0;
  stack += sizeof(saved_player *);
  if (birthday_list)
    FREE(birthday_list);

  i = (int) stack - (int) list;
  if (i > 4)
  {
    birthday_list = (saved_player **) MALLOC(i);
    memcpy(birthday_list, list, i);
  }
  else
    birthday_list = 0;

  close(fd);
}

/* saved player stuff */

/* see if a saved player exists (given lower case name) */

saved_player *find_saved_player(char *name)
{
  saved_player **hash, *list;
  int sum = 0;
  char *c;

  if (!isalpha(*name))
    return 0;
  hash = saved_hash[((int) (tolower(*name)) - (int) 'a')];
  for (c = name; *c; c++)
  {
    if (isalpha(*c))
      sum += (int) (tolower(*c)) - 'a';
    else
      return 0;
  }
  list = *(hash + (sum % HASH_SIZE));
  for (; list; list = list->next)
    if (!strcmp(name, list->lower_name))
      return list;
  return 0;
}


/* hard load and save stuff (ie to disk and back) */


/* extract one player */

void extract_player(char *where, int length)
{
  int len, sum;
  char *oldstack, *c;
  saved_player *old, *sp, **hash;

  oldstack = stack;
  where = get_int(&len, where);
  where = get_string(oldstack, where);
  stack = end_string(oldstack);
  old = find_saved_player(oldstack);
  sp = old;
  if (!old)
  {
    sp = (saved_player *) MALLOC(sizeof(saved_player));
    memset((char *) sp, 0, sizeof(saved_player));
    strncpy(sp->lower_name, oldstack, MAX_NAME);
    strncpy(player_loading, sp->lower_name, MAX_NAME);
    sp->rooms = 0;
    sp->mail_sent = 0;
    sp->mail_received = 0;
    sp->list_top = 0;
    hash = saved_hash[((int) sp->lower_name[0] - (int) 'a')];
    for (sum = 0, c = sp->lower_name; *c; c++)
      sum += (int) (*c) - 'a';
    hash = (hash + (sum % HASH_SIZE));
    sp->next = *hash;
    *hash = sp;
  }
  where = get_int(&sp->last_on, where);
  where = get_int(&sp->system_flags, where);
  where = get_int(&sp->tag_flags, where);
  where = get_int(&sp->custom_flags, where);
  where = get_int(&sp->misc_flags, where);
  where = get_int(&sp->pennies, where);
  where = get_int(&sp->residency, where);

  if (sp->residency == BANISHED)
    sp->residency = BANISHD;
  if (sp->residency == BANISHD)
  {
    sp->last_host[0] = 0;
    sp->email[0] = 0;
    sp->data.where = 0;
    sp->data.length = 0;
    stack = oldstack;
    return;
  }
  if (poto)			/* from where we got it a few clicks ago */
  {
    if (((time(0) - (sp->last_on)) > (poto * ONE_WEEK)) &&
	!(sp->residency & NO_TIMEOUT))
    {
      LOGF("timeouts", "%s timeouts", sp->lower_name);
      remove_player_file(sp->lower_name);
      stack = oldstack;
      return;
    }
  }

/* PUT ANYTHING TO CHANGE RESIDENCY OR OTHER FLAGS HERE */
  where = get_string(sp->last_host, where);
  where = get_string(sp->email, where);
  where = get_int(&sp->data.length, where);
  sp->data.where = (char *) MALLOC(sp->data.length);
  memcpy(sp->data.where, where, sp->data.length);
  where += sp->data.length;
  where = retrieve_room_data(sp, where);
  where = retrieve_list_data(sp, where);
  where = retrieve_alias_data(sp, where);
  where = retrieve_item_data(sp, where);
  where = retrieve_mail_data(sp, where);
  stack = oldstack;
}

/* hard load in on player file */

void hard_load_one_file(char c)
{
  char *oldstack, *where;
  static char *scan;
  int fd, length, len2, fromjmp;
  static int i;

  oldstack = stack;
  if (sys_flags & VERBOSE)
    LOGF("boot", "Loading player file '%c'.", c);

  sprintf(oldstack, "files/players/%c", c);
  fd = open(oldstack, O_RDONLY | O_NDELAY);
  if (fd < 0)
    LOGF("error", "Failed to load player file '%c'", c);
  else
  {
    length = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    if (length)
    {
      where = (char *) MALLOC(length);
      if (read(fd, where, length) < 0)
	handle_error("Can't read player file.");
      for (i = 0, scan = where; i < length;)
      {
	get_int(&len2, scan);
	fromjmp = setjmp(jmp_env);
	if (!fromjmp && !bad_player_load)
	{
	  extract_player(scan, len2);
	}
	else
	{
	  LOGF("error", "Bad Player \'%s\' deleted on load!",
	       player_loading);
	  remove_player_file(player_loading);
	  bad_player_load = 0;
	}
	i += len2;
	scan += len2;
      }
      FREE(where);
    }
    close(fd);
  }
  stack = oldstack;
}


/* load in all the player files */

void hard_load_files(void)
{
  char c;
  int i, hash_length;
  char *oldstack;
#if defined(hpux) | defined(linux)
  struct sigaction sa;

  sa.sa_handler = error_on_load;

#ifdef REDHAT5
  sigemptyset(&sa.sa_mask);
#else
  sa.sa_mask = 0;
#endif
  sa.sa_flags = 0;
  sigaction(SIGSEGV, &sa, 0);
  sigaction(SIGSEGV, &sa, 0);
#else /* hpux | linux */
  signal(SIGSEGV, error_on_load);
  signal(SIGBUS, error_on_load);
#endif /* hpux | linux */

  log("boot", "Loading all player files");

  oldstack = stack;
  hash_length = HASH_SIZE * sizeof(saved_player *);
  for (i = 0; i < 26; i++)
  {
    saved_hash[i] = (saved_player **) MALLOC(hash_length);
    memset((void *) saved_hash[i], 0, hash_length);
  }
  /* we are gonna set up the timeout int here,
     doing it once, instead of every time we load a player */
  poto = atoi(get_config_msg("player_timeout"));
  for (c = 'a'; c <= 'z'; c++)
    hard_load_one_file(c);
}


/* write one player file out */

void write_to_file(saved_player * sp)
{
  char *oldstack;
  int length;
  oldstack = stack;
  if (sys_flags & VERBOSE && sys_flags & PANIC)
    LOGF("sync", "Attempting to write player '%s'.", sp->lower_name);

  stack += 4;
  stack = store_string(stack, sp->lower_name);
  stack = store_int(stack, sp->last_on);
  stack = store_int(stack, sp->system_flags);
  stack = store_int(stack, sp->tag_flags);
  stack = store_int(stack, sp->custom_flags);
  stack = store_int(stack, sp->misc_flags);
  stack = store_int(stack, sp->pennies);
  stack = store_int(stack, sp->residency);
  if ((sp->residency != BANISHED))
  {
    stack = store_string(stack, sp->last_host);
    stack = store_string(stack, sp->email);
    stack = store_int(stack, sp->data.length);
    memcpy(stack, sp->data.where, sp->data.length);
    stack += sp->data.length;
    construct_room_save(sp);
    construct_list_save(sp);
    construct_alias_save(sp);
    construct_item_save(sp);
    construct_mail_save(sp);
  }
  length = (int) stack - (int) oldstack;
  (void) store_int(oldstack, length);

}

/* sync player files corresponding to one letter */

void sync_to_file(char c, int background)
{
  saved_player *scan, **hash;
  char *oldstack;
  int fd, i, length;

  if (background && fork())
    return;

  oldstack = stack;
  if (sys_flags & VERBOSE)
    LOGF("sync", "Syncing File '%c'.", c);

  hash = saved_hash[((int) c - (int) 'a')];
  for (i = 0; i < HASH_SIZE; i++, hash++)
    for (scan = *hash; scan; scan = scan->next)
      if (scan->residency != SYSTEM_ROOM
	  && (!(scan->residency & NO_SYNC) || scan->residency == BANISHED))
	write_to_file(scan);
  length = (int) stack - (int) oldstack;


  /* test that you can write out a file ok */

  strcpy(stack, "files/players/backup_write");

#ifdef BSDISH
  fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
  fd = open(stack, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif

  if (fd < 0)
    handle_error("Primary open failed (player back)");
  if (write(fd, oldstack, length) < 0)
    handle_error("Primary write failed "
		 "(playerback)");
  close(fd);

  sprintf(stack, "files/players/%c", c);

#ifdef BSDISH
  fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
  fd = open(stack, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif

  if (fd < 0)
    handle_error("Failed to open player file.");
  if (write(fd, oldstack, length) < 0)
    handle_error("Failed to write player file.");
  close(fd);
  update[(int) c - (int) 'a'] = 0;
  stack = oldstack;

  if (background)
    exit(0);

}


/* sync everything to disk */

void sync_all()
{
  char c, *oldstack;
  oldstack = stack;
  for (c = 'a'; c <= 'z'; c++)
    sync_to_file(c, 0);
  log("sync", "Full Sync Completed.");
}

/* fork and sync the playerfiles */

void fork_the_thing_and_sync_the_playerfiles(void)
{
  int fl;
  fl = fork();
  if (fl == -1)
  {
    log("error", "Forked up!");
    return;
  }
  if (fl > 0)
    return;
  sync_all();
  exit(0);
}

/* flicks on the update flag for a particular player hash */

void set_update(char c)
{
  update[(int) c - (int) 'a'] = 1;
}


/* removes an entry from the saved player lists */

int remove_player_file(char *name)
{
  saved_player *previous = 0, **hash, *list;
  char *c;
  int sum = 0;

  if (!isalpha(*name))
  {
    LOGF("error", "Tried to remove non-player from save files [%s].", name);
    return 0;
  }
  strcpy(stack, name);
  lower_case(stack);

  hash = saved_hash[((int) (*stack) - (int) 'a')];
  for (c = stack; *c; c++)
  {
    if (isalpha(*c))
      sum += (int) (*c) - 'a';
    else
    {
      log("error", "Remove bad name from save files");
      return 0;
    }
  }

  hash += (sum % HASH_SIZE);
  list = *hash;
  for (; list; previous = list, list = list->next)
  {
    if (!strcmp(stack, list->lower_name))
    {
      if (previous)
	previous->next = list->next;
      else
	*hash = list->next;
      if (list->data.where)
	FREE(list->data.where);
      if (list->mail_received)
	FREE(list->mail_received);
      free_room_data(list);
      FREE((void *) list);
      set_update(*stack);
      return 1;
    }
  }
  return 0;
}

/* remove an entire hash of players */

void remove_entire_list(char c)
{
  saved_player **hash, *sp, *next;
  int i;
  if (!isalpha(c))
    return;
  hash = saved_hash[((int) (c) - (int) 'a')];
  for (i = 0; i < HASH_SIZE; i++, hash++)
  {
    sp = *hash;
    while (sp)
    {
      next = sp->next;
      if (sp->data.where)
	FREE(sp->data.where);
      free_room_data(sp);
      FREE((void *) sp);
      sp = next;
    }
    *hash = 0;
  }
  set_update(c);
}

/* routines to save a player to the save files */

/* makes the save data onto the stack */

file construct_save_data(player * p)
{
  file d;
  int i;

  d.where = stack;

  stack = store_string(stack, p->name);
  stack = store_string(stack, p->prompt);
  stack = store_string(stack, p->converse_prompt);
  stack = store_string(stack, p->email);
  if (p->password[0] == -1)
    p->password[0] = 0;
  stack = store_string(stack, p->password);
  stack = store_string(stack, p->title);
  stack = store_string(stack, p->plan);
  stack = store_string(stack, p->description);
  stack = store_string(stack, p->enter_msg);
  stack = store_string(stack, p->pretitle);
  stack = store_string(stack, p->ignore_msg);
  stack = store_string(stack, p->room_connect);
  stack = store_int(stack, p->term_width);
  stack = store_int(stack, p->word_wrap);
  stack = store_int(stack, p->max_rooms);
  stack = store_int(stack, p->max_exits);
  stack = store_int(stack, p->max_autos);
  stack = store_int(stack, p->max_list);
  stack = store_int(stack, p->max_mail);
  stack = store_int(stack, p->gender);
  stack = store_int(stack, p->no_shout);
  stack = store_int(stack, p->total_login);
  stack = store_int(stack, p->term);
  stack = store_int(stack, p->birthday);
  stack = store_int(stack, p->age);

  /* Here goes with adding to the playerfiles */

  stack = store_int(stack, p->jetlag);
  stack = store_int(stack, p->sneezed);

  /* now the mother load of trap's spooning........ */
  stack = store_string(stack, p->logonmsg);
  stack = store_string(stack, p->logoffmsg);
  stack = store_string(stack, p->blockmsg);
  stack = store_string(stack, p->exitmsg);
  stack = store_int(stack, p->time_in_main);
  stack = store_int(stack, p->no_sing);
  stack = store_string(stack, p->married_to);
  stack = store_string(stack, p->irl_name);
  stack = store_string(stack, p->alt_email);
  stack = store_string(stack, p->hometown);
  stack = store_string(stack, p->spod_class);
  stack = store_string(stack, p->favorite1);
  stack = store_string(stack, p->favorite2);
  stack = store_string(stack, p->favorite3);
  stack = store_int(stack, p->total_idle_time);
  stack = store_int(stack, p->max_alias);
  stack = store_string(stack, p->colorset);
  stack = store_string(stack, p->ressied_by);
  stack = store_string(stack, p->git_string);
  stack = store_string(stack, p->git_by);
  stack = store_int(stack, p->warn_count);
  stack = store_int(stack, p->eject_count);
  stack = store_int(stack, p->idled_out_count);
  stack = store_int(stack, p->booted_count);
  stack = store_int(stack, p->num_ressied);
  stack = store_int(stack, p->num_warned);
  stack = store_int(stack, p->num_ejected);
  stack = store_int(stack, p->num_rmd);
  stack = store_int(stack, p->num_booted);
  stack = store_int(stack, p->first_login_date);
  stack = store_int(stack, p->max_items);
  stack = store_int(stack, p->prs_record);
  stack = store_string(stack, p->ingredients);

/* --------------------------------------------------------------------

   If you are upgrading from PG96 and you added any extra elements to the
   player files then you should insert them in here now. */



/* --------------------------------------------------------------------

   From here are the new elements added for Playground Plus ... */

  stack = store_int(stack, p->ttt_win);
  stack = store_int(stack, p->ttt_loose);
  stack = store_int(stack, p->ttt_draw);
  stack = store_int(stack, p->icq);
  stack = store_string(stack, p->finger_message);
  stack = store_string(stack, p->swarn_message);
  stack = store_string(stack, p->swarn_sender);
  for (i = 0; i < MAX_LAST_NEWS_INTS; i++)
    stack = store_int(stack, p->news_last[i]);

#ifdef DSC
  for (i = 0; i < MAX_CHANNELS; i++)
    stack = store_string(stack, p->channels[i]);
  stack = store_int(stack, p->dsc_flags);
#endif /* DSC */

/* end of my spooning... (well, not the end of me spooning, but the end of this mega-batch) */
  d.length = (int) stack - (int) d.where;
  stack = d.where;
  return d;
}

/* the routine that sets everything up for the save */

void save_player(player * p)
{
  saved_player *old, **hash, *sp;
  int sum;
  file data;
  char *c, *oldstack;
  int verb = 1;

  oldstack = stack;

  if (!(p->location) || !(p->name[0])
      || p->residency == NON_RESIDENT)
    return;

  if (sys_flags & PANIC)
    LOGF("boot", "Attempting to save player %s.", p->name);

  if (!(isalpha(p->lower_name[0])))
  {
    LOGF("error", "Tried to save non-player [%s]", p->lower_name);
    return;
  }
  if (p->residency & SYSTEM_ROOM)
    verb = 0;
  if (p != current_player)
    verb = 0;

  if (verb)
  {
    if (!(p->password[0] && p->password[0] != -1))
    {
      tell_current(get_plists_msg("no_save_pass"));
      p->residency |= NO_SYNC;
      tell_current(" NOT saved.\n");
      stack = oldstack;
      return;
    }
    if (p->email[0] == 2)
    {
      tell_current(get_plists_msg("no_save_email"));
      p->residency |= NO_SYNC;
      tell_player(p, "NOT saved.\n");
      stack = oldstack;
      return;
    }
  }
  p->residency &= ~NO_SYNC;
  p->saved_residency = p->residency;
  old = p->saved;
  sp = old;
  if (!old)
  {
    sp = (saved_player *) MALLOC(sizeof(saved_player));
    memset((char *) sp, 0, sizeof(saved_player));
    strncpy(sp->lower_name, p->lower_name, MAX_NAME);
    sp->rooms = 0;
    sp->mail_sent = 0;
    sp->mail_received = 0;
    sp->list_top = 0;
    hash = saved_hash[((int) p->lower_name[0] - (int) 'a')];
    for (sum = 0, c = p->lower_name; *c; c++)
    {
      if (isalpha(*c))
	sum += (int) (*c) - 'a';
      else
      {
	tell_player(p, " Eeek, trying to save bad player name !!\n");
	FREE(sp);
	return;
      }
    }
    hash = (hash + (sum % HASH_SIZE));
    sp->next = *hash;
    *hash = sp;
    p->saved = sp;
    sp->system_flags = p->system_flags;
    sp->tag_flags = p->tag_flags;
    sp->custom_flags = p->custom_flags;
    sp->misc_flags = p->misc_flags;
    sp->pennies = p->pennies;
    create_room(p);
  }
  data = construct_save_data(p);
  if (!data.length)
  {
    LOGF("error", "Bad construct save [%s]", p->lower_name);
    return;
  }
  if (old && sp->data.where)
    FREE((void *) sp->data.where);
  sp->data.where = (char *) MALLOC(data.length);
  sp->data.length = data.length;
  memcpy(sp->data.where, data.where, data.length);
  sp->residency = p->saved_residency;
  sp->system_flags = p->system_flags;
  sp->tag_flags = p->tag_flags;
  sp->custom_flags = p->custom_flags;
  sp->misc_flags = p->misc_flags;
  sp->pennies = p->pennies;
  if (find_player_absolute_quiet(p->lower_name))
  {
    strncpy(sp->last_host, p->inet_addr, MAX_INET_ADDR - 2);
    strncpy(sp->email, p->email, MAX_EMAIL - 3);
    sp->last_on = time(0);
  }
  set_update(*(sp->lower_name));
  p->saved = sp;
}

/* the routine that sets everything up for the save */

void create_banish_file(char *str)
{
  saved_player **hash, *sp, *scan;
  int sum;
  char *c, name[20];

  strncpy(name, str, MAX_NAME - 3);
  sp = (saved_player *) MALLOC(sizeof(saved_player));
  memset((char *) sp, 0, sizeof(saved_player));
  strcpy(stack, name);
  lower_case(stack);
  strncpy(sp->lower_name, stack, MAX_NAME - 3);
  sp->rooms = 0;
  sp->mail_sent = 0;
  sp->mail_received = 0;
  hash = saved_hash[((int) name[0] - (int) 'a')];
  for (sum = 0, c = name; *c; c++)
    if (isalpha(*c))
      sum += (int) (*c) - 'a';
    else
    {
      LOGF("error", "Tried to banish bad player, [%s]", sp->lower_name);
      FREE(sp);
      return;
    }
  hash = (hash + (sum % HASH_SIZE));
  scan = *hash;
  while (scan)
  {
    hash = &(scan->next);
    scan = scan->next;
  }
  *hash = sp;
  sp->residency = BANISHD;
  sp->system_flags = 0;
  sp->tag_flags = 0;
  sp->custom_flags = 0;
  sp->misc_flags = 0;
  sp->pennies = 50;
  sp->last_host[0] = 0;
  sp->email[0] = 0;
  sp->last_on = time(0);
  sp->next = 0;
  set_update(tolower(*(sp->lower_name)));
}


/* load from a saved player into a current player */

/* NOTE!!!!

   This code now uses Oolons get_int_safe and get_char_safe code for easy
   additions to your code. To add a variable do the following:

   (1) Add it in the player structure (player.h)
   (2) Add it here using get_int_safe (for ints) or get_char_safe (for strings)
   (3) Add it into the save player function

   When an "old format" pfile is loaded it will be automatically converted
   to the "new" format
 */

int load_player(player * p)
{
  saved_player *sp;
  char *r;
  int i;

  lower_case(p->lower_name);
  sp = find_saved_player(p->lower_name);

  if (!sp && current_player && current_player->location)
    sp = find_matched_sp_verbose(p->lower_name);

  p->saved = sp;
  if (!sp)
    return 0;

  p->residency = sp->residency;

  p->saved_residency = p->residency;
  p->system_flags = sp->system_flags;
  p->tag_flags = sp->tag_flags;
  p->custom_flags = sp->custom_flags;
  p->misc_flags = sp->misc_flags;
  p->pennies = sp->pennies;
  if (sp->residency == BANISHED
      || sp->residency == SYSTEM_ROOM
      || sp->residency == BANISHD)
    return 1;

  r = sp->data.where;
  r = get_string_safe(p->name, r, sp->data);
  r = get_string_safe(p->prompt, r, sp->data);
  r = get_string_safe(p->converse_prompt, r, sp->data);
  r = get_string_safe(p->email, r, sp->data);
  r = get_string_safe(p->password, r, sp->data);
  r = get_string_safe(p->title, r, sp->data);
  r = get_string_safe(p->plan, r, sp->data);
  r = get_string_safe(p->description, r, sp->data);
  r = get_string_safe(p->enter_msg, r, sp->data);
  r = get_string_safe(p->pretitle, r, sp->data);
  r = get_string_safe(p->ignore_msg, r, sp->data);
  r = get_string_safe(p->room_connect, r, sp->data);
  r = get_int_safe(&p->term_width, r, sp->data);
  r = get_int_safe(&p->word_wrap, r, sp->data);
  r = get_int_safe(&p->max_rooms, r, sp->data);
  r = get_int_safe(&p->max_exits, r, sp->data);
  r = get_int_safe(&p->max_autos, r, sp->data);
  r = get_int_safe(&p->max_list, r, sp->data);
  r = get_int_safe(&p->max_mail, r, sp->data);
  r = get_int_safe(&p->gender, r, sp->data);
  r = get_int_safe(&p->no_shout, r, sp->data);
  r = get_int_safe(&p->total_login, r, sp->data);
  r = get_int_safe(&p->term, r, sp->data);
  r = get_int_safe(&p->birthday, r, sp->data);
  r = get_int_safe(&p->age, r, sp->data);

  r = get_int_safe(&p->jetlag, r, sp->data);
  r = get_int_safe(&p->sneezed, r, sp->data);

/* ok the first time through, these won't be in... instead we just do blanks. */
/* i commented this out when I did the evil deed... */
  r = get_string_safe(p->logonmsg, r, sp->data);
  r = get_string_safe(p->logoffmsg, r, sp->data);
  r = get_string_safe(p->blockmsg, r, sp->data);
  r = get_string_safe(p->exitmsg, r, sp->data);
  r = get_int_safe(&p->time_in_main, r, sp->data);
  r = get_int_safe(&p->no_sing, r, sp->data);
  r = get_string_safe(p->married_to, r, sp->data);
  r = get_string_safe(p->irl_name, r, sp->data);
  r = get_string_safe(p->alt_email, r, sp->data);
  r = get_string_safe(p->hometown, r, sp->data);
  r = get_string_safe(p->spod_class, r, sp->data);
  r = get_string_safe(p->favorite1, r, sp->data);
  r = get_string_safe(p->favorite2, r, sp->data);
  r = get_string_safe(p->favorite3, r, sp->data);
  r = get_int_safe(&p->total_idle_time, r, sp->data);
  r = get_int_safe(&p->max_alias, r, sp->data);
  r = get_string_safe(p->colorset, r, sp->data);
  r = get_string_safe(p->ressied_by, r, sp->data);
  r = get_string_safe(p->git_string, r, sp->data);
  r = get_string_safe(p->git_by, r, sp->data);
  r = get_int_safe(&p->warn_count, r, sp->data);
  r = get_int_safe(&p->eject_count, r, sp->data);
  r = get_int_safe(&p->idled_out_count, r, sp->data);
  r = get_int_safe(&p->booted_count, r, sp->data);
  r = get_int_safe(&p->num_ressied, r, sp->data);
  r = get_int_safe(&p->num_warned, r, sp->data);
  r = get_int_safe(&p->num_ejected, r, sp->data);
  r = get_int_safe(&p->num_rmd, r, sp->data);
  r = get_int_safe(&p->num_booted, r, sp->data);
  r = get_int_safe(&p->first_login_date, r, sp->data);
  r = get_int_safe(&p->max_items, r, sp->data);
  r = get_int_safe(&p->prs_record, r, sp->data);
  r = get_string_safe(p->ingredients, r, sp->data);
/* end of trap's shit - the loaded version. */

/* --------------------------------------------------------------------

   If you are upgrading from PG96 and you added any extra elements to the
   player files then you should insert them in here now. */



/* --------------------------------------------------------------------

   From here are the new elements added for Playground Plus ... */

  r = get_int_safe(&p->ttt_win, r, sp->data);
  r = get_int_safe(&p->ttt_loose, r, sp->data);
  r = get_int_safe(&p->ttt_draw, r, sp->data);
  r = get_int_safe(&p->icq, r, sp->data);
  r = get_string_safe(p->finger_message, r, sp->data);
  r = get_string_safe(p->swarn_message, r, sp->data);
  r = get_string_safe(p->swarn_sender, r, sp->data);
  for (i = 0; i < MAX_LAST_NEWS_INTS; i++)
    r = get_int_safe(&p->news_last[i], r, sp->data);

#ifdef DSC
  for (i = 0; i < MAX_CHANNELS; i++)
    r = get_string_safe(p->channels[i], r, sp->data);
  r = get_int_safe(&p->dsc_flags, r, sp->data);
#endif /* DSC */


  if (((p->term_width) >> 1) <= (p->word_wrap))
    p->word_wrap = (p->term_width) >> 1;

  decompress_list(sp);
  decompress_alias(sp);
  decompress_item(sp);
  p->system_flags = sp->system_flags;
  p->tag_flags = sp->tag_flags;
  p->custom_flags = sp->custom_flags;
  p->misc_flags = sp->misc_flags;
  p->pennies = sp->pennies;
  return 1;
}

/* load and do linking */

int restore_player(player * p, char *name)
{
  return restore_player_title(p, name, 0);
}

int restore_player_title(player * p, char *name, char *title)
{
  int did_load, i;
  int found_lower;
  char *n;

  strncpy(p->name, name, MAX_NAME - 3);
  strncpy(p->lower_name, name, MAX_NAME - 3);
  lower_case(p->lower_name);
/* allow all lower case names on login
   if (!strcmp(p->name, p->lower_name))
   p->name[0] = toupper(p->name[0]);
 */
  if (!(config_flags & cfCAPPEDNAMES))
  {
    found_lower = 0;
    n = p->name;
    while (*n)
    {
      if (*n >= 'a' && *n <= 'z')
	found_lower = 1;
      n++;
    }
    if (!found_lower)
    {
      n = p->name;
      n++;
      while (*n)
      {
	*n = *n - ('A' - 'a');
	n++;
      }
    }
  }

/* Set up a default player structure, methinks */

  strncpy(p->title, get_pdefaults_msg("title"), MAX_TITLE - 1);
  strncpy(p->description, get_pdefaults_msg("description"), MAX_DESC - 1);
  strncpy(p->plan, get_pdefaults_msg("plan"), MAX_PLAN - 1);

  strncpy(p->logonmsg, get_pdefaults_msg("logonmsg"), MAX_ENTER_MSG - 1);
  strncpy(p->logoffmsg, get_pdefaults_msg("logoffmsg"), MAX_ENTER_MSG - 1);

  strncpy(p->prompt, get_pdefaults_msg("prompt"), MAX_PROMPT - 1);
  strncpy(p->converse_prompt, get_pdefaults_msg("cprompt"), MAX_PROMPT - 1);

  strncpy(p->enter_msg, get_pdefaults_msg("enter_msg"), MAX_ENTER_MSG - 1);
  strncpy(p->colorset, get_pdefaults_msg("colorset"), 10);

  p->max_rooms = atoi(get_pdefaults_msg("max_rooms"));
  p->max_exits = atoi(get_pdefaults_msg("max_exits"));
  p->max_autos = atoi(get_pdefaults_msg("max_autos"));
  p->max_list = atoi(get_pdefaults_msg("max_list"));
  p->max_mail = atoi(get_pdefaults_msg("max_mail"));
  p->max_alias = atoi(get_pdefaults_msg("max_alias"));

  p->term_width = atoi(get_pdefaults_msg("term_width"));
  p->word_wrap = atoi(get_pdefaults_msg("word_wrap"));

  p->no_shout = atoi(get_pdefaults_msg("no_shout"));
  p->no_sing = atoi(get_pdefaults_msg("no_sing"));
  p->pennies = atoi(get_pdefaults_msg("money"));

  p->column = 0;
  p->icq = 0;

  p->total_login = 0;
  p->first_login_date = time(0);

  p->gender = VOID_GENDER;

  p->tag_flags = TAG_ECHO | SEEECHO | TAG_PERSONAL | TAG_SHOUT;
  p->custom_flags = PRIVATE_EMAIL | MAIL_INFORM | NOEPREFIX | NOPREFIX | NEWS_INFORM;
  p->system_flags = IAC_GA_ON;
  p->misc_flags = NOCOLOR;


  p->suh_seen = -1;
  p->birthday = 0;
  p->age = 0;
  p->jail_timeout = 0;

  for (i = 0; i < 8; i++)
    strcpy(p->rev[i].review, "");

  p->script = 0;
  strcpy(p->script_file, "dummy.log");
  p->assisted_by[0] = 0;

  p->residency = 0;

  strcpy(p->ignore_msg, "");
  p->jetlag = 0;
/* gonna need these now.... */
  strcpy(p->blockmsg, "");
  strcpy(p->exitmsg, "");
  strcpy(p->married_to, "");
  strcpy(p->irl_name, "");
  strcpy(p->alt_email, "");
  strcpy(p->hometown, "");
  strcpy(p->spod_class, "");
  strcpy(p->favorite1, "");
  strcpy(p->favorite2, "");
  strcpy(p->favorite3, "");
  strcpy(p->ingredients, "");

  strcpy(p->ressied_by, "");
  strcpy(p->git_string, "");
  strcpy(p->git_by, "");
/* and set the ints up too =) */
  p->time_in_main = 0;
  p->total_idle_time = 0;
  p->warn_count = 0;
  p->eject_count = 0;
  p->idled_out_count = 0;
  p->booted_count = 0;
  p->num_ressied = 0;
  p->num_warned = 0;
  p->num_ejected = 0;
  p->num_rmd = 0;
  p->num_booted = 0;
  p->prs = 0;
  p->prs_record = 0;
  p->max_items = 10;

  p->gag_top = 0;

  /* ping info here */
  p->next_ping = PING_INTERVAL;
  p->last_ping = -1;

  did_load = load_player(p);
  if (did_load && !strcmp(p->lower_name, "guest"))
    did_load = 0;

  strcpy(p->assisted_by, "");

  if (title && *title)
  {
    strncpy(p->title, title, MAX_TITLE);
    p->title[MAX_TITLE] = 0;
  }
  if ((p->system_flags & IAC_GA_ON) && (!(p->flags & EOR_ON)))
    p->flags |= IAC_GA_DO;
  else
    p->flags &= ~IAC_GA_DO;

  if (p->residency == 0 && did_load == 1)
    p->residency = SYSTEM_ROOM;

/* blank the repeat stuff, just in case... */
  p->last_remote_command = -1;	/* will give error to user =) */
  strcpy(p->last_remote_msg, "");

  if (p->system_flags & SAVEDFROGGED)
    p->flags |= FROGGED;

  /* got to have someone here I'm afraid..  */

  if (ishcadmin(p->lower_name))
  {

    p->residency |= HCADMIN_INIT;	/* aha, lettem keep the privs they got */

    strcpy(p->ressied_by, "Hard Coded");
  }
/* this is just an example of how one can "mask" a site for a test char */

  if (!strcmp("deathboy", p->lower_name))
  {
    strcpy(p->num_addr, "138.253.85.33");
    strcpy(p->inet_addr, "petey.halls.com");
  }
  if (!strcmp("silver", p->lower_name))
  {
    strcpy(p->num_addr, "123.69.69.666");
    strcpy(p->inet_addr, "uberhq.gov.uk");	/* lemme keep this plz! */
  }
  if (p->residency & PSU)
    p->no_shout = 0;


  /* integrity .. sigh */

  p->saved_residency = p->residency;

  if ((p->word_wrap) > ((p->term_width) >> 1) || p->word_wrap < 0)
    p->word_wrap = (p->term_width) >> 1;
  if (p->term > 9)
    p->term = 0;

  return did_load;
}


/* current player stuff */

/* create an abstract player into the void hash list */

player *create_player()
{
  player *p;

  p = (player *) MALLOC(sizeof(player));
  memset((char *) p, 0, sizeof(player));

  if (flatlist_start)
    flatlist_start->flat_previous = p;
  p->flat_next = flatlist_start;
  flatlist_start = p;

  p->hash_next = hashlist[0];
  hashlist[0] = p;
  p->hash_top = 0;
  p->timer_fn = 0;
  p->timer_count = -1;
  p->edit_info = 0;
  p->logged_in = 0;
#ifdef IDENT
  p->ident_id = AWAITING_IDENT;
  strcpy(p->remote_user, IDENT_NOTYET);
#endif
  return p;
}

/* unlink p from all the lists */

void punlink(player * p)
{
  player *previous, *scan;

  /* reset the session p */

  p_sess = 0;

  /* first remove from the hash list */

  scan = hashlist[p->hash_top];
  previous = 0;
  while (scan && scan != p)
  {
    previous = scan;
    scan = scan->hash_next;
  }
  if (!scan)
    log("error", "Bad hash list");
  else if (!previous)
    hashlist[p->hash_top] = p->hash_next;
  else
    previous->hash_next = p->hash_next;

  /* then remove from the flat list */

  if (p->flat_previous)
    p->flat_previous->flat_next = p->flat_next;
  else
    flatlist_start = p->flat_next;
  if (p->flat_next)
    p->flat_next->flat_previous = p->flat_previous;

  /* finally remove from the location list */

  if (p->location)
  {
    previous = 0;
    scan = p->location->players_top;
    while (scan && scan != p)
    {
      previous = scan;
      scan = scan->room_next;
    }
    if (!scan)
      log("error", "Bad Location list");
    else if (!previous)
      p->location->players_top = p->room_next;
    else
      previous->room_next = p->room_next;
  }
}

/* remove a player from the current hash lists */

void destroy_player(player * p)
{

  if ((p->fd) > 0)
  {
    shutdown(p->fd, 2);
    close(p->fd);
  }

  if (p->name[0] && p->location)
    current_players--;
  punlink(p);
  if (p->edit_info)
    finish_edit(p);
  FREE(p);
}

/* get person to agree to disclaimer */

void agree_disclaimer(player * p, char *str)
{
  p->input_to_fn = 0;
  if (!strcasecmp(str, get_plists_msg("disclaimer_yes")))
  {
    p->system_flags |= AGREED_DISCLAIMER;
    if (p->saved)
      p->saved->system_flags |= AGREED_DISCLAIMER;
    if (!ishcadmin(p->name))
      link_to_program(p);
    else
      hcadmin_check_password(p);
    return;
  }
  else if (!strcasecmp(str, get_plists_msg("disclaimer_no")))
  {
    TELLPLAYER(p, "\n %s \n\n", get_plists_msg("login_quit"));
    quit(p, "");
  }
  else
  {
    do_prompt(p, get_plists_msg("disclaimer_prompt"));
    p->input_to_fn = agree_disclaimer;
  }
}

/* check if the motd is a new one */
/* HOPE that this works right ;) */

/* the function will check motd or sumotd, or any other ones we add. 
   If the motd was written since the last time the player was seen, the
   function returns 1, and that player will see the appropriate motd */

int motd_is_new(player * p, char *filename)
{
  struct stat statblk;
  char motdfile[20];		/* forget the magic number rules :P */

  if (!(p->saved))		/* hcadmin new login */
    return 1;
  sprintf(motdfile, "files/%s.msg", filename);
  stat(motdfile, &statblk);	/* Get the stats on the file */
  if (statblk.st_mtime >= p->saved->last_on)
  {
    /* check mod file vs player last login */
    return 1;
  }
  else
    return 0;
}

/*
 * Allow a player to finger a player without logging in.
 */
void finger_at_name_prompt(player * p, char *str)
{
  p->misc_flags |= NOCOLOR;
  newfinger(p, str);
  do_prompt(p, get_plists_msg("initial_name"));
  p->input_to_fn = got_name;
  return;
}

/* links a person into the program properly  (several fxns) */
void finish_player_login(player * p, char *str)
{
  char *oldstack, *hello;
  saved_player *sp;
  room *r, *rm;
  int i;

  oldstack = stack;

  if (p->residency && p->custom_flags & CONVERSE)
    p->mode |= CONV;

  current_players++;
  p->on_since = time(0);
  logins++;

  if (!(p->flags & RECONNECTION))
  {
#ifdef LAST
    stampLogin(p->name);
#endif
    p->shout_index = 50;
  }
  if (p->residency != NON_RESIDENT)
    player_flags(p);

  if (p->system_flags & SAVEDJAIL)
  {
    p->jail_timeout = -1;
    trans_to(p, "system.prison");
  }
  else if ((p->custom_flags & TRANS_TO_HOME || *p->room_connect) && !(p->flags & RECONNECTION))
  {
    sp = p->saved;
    if (!sp)
      tell_player(p, " Double Eeek (room_connect)!\n");
    else
    {
      if (p->custom_flags & TRANS_TO_HOME)
      {
	for (r = sp->rooms; r; r = r->next)
	  if (r->flags & HOME_ROOM)
	  {
	    sprintf(oldstack, "%s.%s", r->owner->lower_name, r->id);
	    stack = end_string(oldstack);
	    trans_to(p, oldstack);
	    break;
	  }
      }
      else
      {
	rm = convert_room(p, p->room_connect);
#ifdef ROBOTS
	if ((rm && rm->flags & CONFERENCE && possible_move(p, rm, 1)) || (p->residency & ROBOT_PRIV))
#else
	if (rm && rm->flags & CONFERENCE && possible_move(p, rm, 1))
#endif
	  trans_to(p, p->room_connect);
      }
      if (!(p->location))
	tell_player(p, " -=> Tried to connect you to a room, but failed"
		    " !!\n\n");
    }
  }
  if (!(p->location))
  {
    trans_to(p, sys_room_id(ENTRANCE_ROOM));
  }
  if (p->flags & RECONNECTION)
  {
    do_inform(p, get_plists_msg("inform_reconnect"));
    sprintf(oldstack,
	    "%s appears momentarily frozen as %s reconnects.\n",
	    p->name, gstring(p));
    stack = end_string(oldstack);
    command_type |= RECON_TAG;
    tell_room(p->location, oldstack);
    command_type &= ~RECON_TAG;
  }
  else
  {
    do_inform(p, get_plists_msg("inform_login"));

    if (strlen(p->logonmsg) < 1)
    {
      stack += sprintf(stack, "%s %s enters the program. ",
		       get_config_msg("logon_prefix"), p->name);
      stack += sprintf(stack, "%s\n", get_config_msg("logon_suffix"));
    }
    else
    {
      if (*p->logonmsg == 39 || *p->logonmsg == ',')
      {
	stack += sprintf(stack, "%s %s%s ",
		      get_config_msg("logon_prefix"), p->name, p->logonmsg);
	stack += sprintf(stack, "%s\n", get_config_msg("logon_suffix"));
      }
      else
      {
	stack += sprintf(stack, "%s %s %s ",
		      get_config_msg("logon_prefix"), p->name, p->logonmsg);
	stack += sprintf(stack, "%s\n", get_config_msg("logon_suffix"));
      }
    }
    stack = end_string(oldstack);
    command_type |= LOGIN_TAG;
    tell_room(p->location, oldstack);
    command_type &= ~LOGIN_TAG;
  }

#ifdef ALLOW_MULTIS
  if (!(p->flags & RECONNECTION))
    add_to_friends_multis(p);
#endif

  if (p->saved)
  {
    decompress_list(p->saved);
    decompress_alias(p->saved);
    decompress_item(p->saved);
    /* here is the bug */
    p->system_flags = p->saved->system_flags;
    p->tag_flags = p->saved->tag_flags;
    p->custom_flags = p->saved->custom_flags;
    p->misc_flags = p->saved->misc_flags;
    if (!(p->pennies) && !(p->flags & RECONNECTION))
      p->pennies = p->saved->pennies;

    /* wibble plink? =) */
    if (p->flags & RECONNECTION)
      look(p, 0);

    if (p->custom_flags & YES_QWHO_LOGIN)
      qwho(p, 0);

    stack = oldstack;

    if (p->flags & RECONNECTION)
    {
      hello = do_alias_match(p, "_recon");
      if (strcmp(hello, "\n"))
	match_commands(p, "_recon");
    }
    else
    {
      hello = do_alias_match(p, "_logon");
      if (strcmp(hello, "\n"))
	match_commands(p, "_logon");
#ifdef DSC
      connect_channels(p);
#endif /* DSC */

    }
  }
  /* clear the chanflags, just in case... */
  if (p->flags & RECONNECTION)
    p->flags &= ~RECONNECTION;

  stack = oldstack;
  for (i = 0; i < 8; i++)
    strcpy(p->rev[i].review, "");
  if (p->system_flags & SAVED_RM_MOVE)
    p->no_move = 100;
  p->input_to_fn = 0;

  /* Here is the login greating */
  tell_player(p, "\n");
  command_type |= HIGHLIGHT;
  TELLPLAYER(p, " %s\n", get_config_msg("welcome_msg"));

  /* No password warning */
  if (!p->password[0] && p->residency)
  {
    tell_player(p, "\n"
		" You have no password !!!\n"
	" Please set one as soon as possible with the 'password' command.\n"
		" If you don't your character will not save!\n");
  }

  /* saved warnings */
  if (p->swarn_message[0])
  {
    tell_player(p, "\n");
    TELLPLAYER(p, " -=*> Saved warning from %s: %s\007\n", p->swarn_sender, p->swarn_message);
    SW_BUT(p, " -=*> %s has received a saved warning from %s: %s\n", p->name, p->swarn_sender, p->swarn_message);
    p->num_warned++;
    p->swarn_sender[0] = 0;
    p->swarn_message[0] = 0;
  }

  command_type &= ~HIGHLIGHT;
  tell_player(p, "\n");

  return;
}

void show_sumotd_login(player * p, char *str)
{
  tell_player(p, sumotd_msg.where);
  do_prompt(p, get_plists_msg("hit_return"));
  p->input_to_fn = finish_player_login;
  return;
}

void show_reg_motd_login(player * p, char *str)
{
  motd(p, "");
  do_prompt(p, get_plists_msg("hit_return"));
  p->input_to_fn = finish_player_login;
  return;
}
void show_new_motd_login(player * p, char *str)
{
  tell_player(p, newbie_msg.where);
  do_prompt(p, get_plists_msg("hit_return"));
  p->input_to_fn = show_reg_motd_login;
}
void set_term_hitells_asty(player * p, char *str)
{
  if (!*str)
    hitells(p, "vt100");
  else
    hitells(p, str);

  if (p->term == 0)		/* So we don't crash if they set an unknown */
    hitells(p, "vt100");	/* terminal type ... */


  p->misc_flags &= ~NOCOLOR;

  TELLPLAYER(p, "\n If you can see this word in ^Cc^Ro^Yl^Bo^Pu^Hr^N then your telnet program supports\n"
	   "  colour and we recommend that you turn in on to get the most\n"
	     "  from %s.\n\n", get_config_msg("talker_name"));
  do_prompt(p, "Would you like colour on? (Y or N): ");

  p->input_to_fn = colours_decide;

  return;
}

void colours_decide(player * p, char *str)
{
  p->input_to_fn = 0;

  if (strncasecmp(str, "y", 1))
  {
    p->misc_flags |= NOCOLOR;
    show_new_motd_login(p, str);
    return;
  }
  TELLPLAYER(p, "\n %s allows you to have text hi-lighted in different colours\n"
     "  depending on whether it was a shout, say, friendtell etc. You can\n"
	     "  customise the colours or, if you don't like it, turn them off from within\n"
	     "  the talker.\n\n", get_config_msg("talker_name"));
  do_prompt(p, "Would you like this turned on? (Y or N): ");
  p->input_to_fn = syscolour_decide;
  return;
}

void syscolour_decide(player * p, char *str)
{
  p->input_to_fn = 0;
  if (!strncasecmp(str, "y", 1))
    p->misc_flags |= SYSTEM_COLOR;

  show_new_motd_login(p, str);
}

void in_sulog(player * p)
{
  int csu;

  csu = true_count_su();

  if (p->residency & ADMIN)
    LOGF("su", "%s login (admin) - %d sus now on.", p->name, csu + 1);
  else
    LOGF("su", "%s login (su) - %d sus now on.", p->name, csu + 1);
}

void link_to_program(player * p)
{
  char *oldstack;
  saved_player *sp;
  player *search, *previous, *scan;
  int hash;
  time_t t;
  struct tm *log_time;
  oldstack = stack;

  /* Presentation purposes */
  tell_player(p, "\n");

  /* Any conversions that are necessary */
  convert_pg_to_pgplus(p);

  /* Set up of TTT */
  p->ttt_opponent = 0;

  /* SuHistory setup */
  p->suh_seen = -1;

  /* blank finger_message */
  p->finger_message[0] = 0;

  search = hashlist[((int) (p->lower_name[0])) - (int) 'a' + 1];
  for (; search; search = search->hash_next)
  {
    if (!strcmp(p->lower_name, search->lower_name))
    {
      if (p->residency == NON_RESIDENT)
      {
	TELLPLAYER(p, "\n Sorry there is already someone on %s with that name.\n"
		   " Please try again, but use a different name.\n\n",
		   get_config_msg("talker_name"));
	quit(p, "");
	return;
      }
      else
      {
#ifdef NEW_RECONNECT
	/* careful if you change anything here...everything is
	   done in a very specfic order for a very specfic reason 
	 */
	int holder = search->fd;

	tell_player(p, get_plists_msg("new_reconnect"));

	current_player = search;
	shutdown(search->fd, 2);
	close(search->fd);
	search->fd = p->fd;
	p->fd = holder;
	strncpy(search->inet_addr, p->inet_addr, MAX_INET_ADDR - 1);
	strncpy(search->num_addr, p->num_addr, MAX_INET_ADDR - 1);
#ifdef IDENT
	strncpy(search->remote_user, p->remote_user, MAX_REMOTE_USER - 1);
#endif /* IDENT */
	search->idle = 0;
	destroy_player(p);

	do_inform(search, get_plists_msg("inform_reconnect"));
	command_type |= RECON_TAG;
	TELLROOM(search->location,
		 "%s appears momentarily frozen as %s reconnects.\n",
		 search->name, gstring(search));
	command_type &= ~RECON_TAG;

	if (config_flags & cfRECONNECTLOOK)
	  look(search, "");

	return;
#else
	tell_player(p, get_plists_msg("old_reconnect"));
	p->total_login = search->total_login;
	search->flags |= RECONNECTION;
	p->flags |= RECONNECTION;

	if (search->location)
	{
	  previous = 0;
	  scan = search->location->players_top;
	  while (scan && scan != search)
	  {
	    previous = scan;
	    scan = scan->room_next;
	  }
	  if (!scan)
	    log("error", "Bad Location list");
	  else if (!previous)
	    search->location->players_top = p;
	  else
	    previous->room_next = p;
	  p->room_next = search->room_next;
	}
	/* lets try this -- we'll copy some of the info over on reconnect */
	/*
	   search->location = 0;
	   quit(search, 0);
	 */
	p->location = search->location;
	p->flags = search->flags;
	strncpy(p->comment, search->comment, MAX_COMMENT - 3);
	p->no_shout = search->no_shout;
	p->shout_index = search->shout_index;
	p->jail_timeout = search->jail_timeout;
	p->lagged = search->lagged;
	p->no_move = search->no_move;
	p->pennies = search->pennies;
	strncpy(p->reply, search->reply, MAX_REPLY - 3);
	p->no_sing = search->no_sing;

#ifdef ALLOW_MULTIS
	multi_change_entries(search, p);
#endif

	/* now get rid of mr. search */
	search->location = 0;
	quit(search, 0);
#endif /* NEW_RECONNECTION */

      }
    }
  }


  /* do the disclaimer biz  */

  if (!(p->system_flags & AGREED_DISCLAIMER))
  {
    if (!(p->saved && p->password[0] == 0))
    {
      tell_player(p, disclaimer_msg.where);
      do_prompt(p, get_plists_msg("disclaimer_prompt"));
      p->input_to_fn = agree_disclaimer;
      return;
    }
  }
  /* remove player from non name hash list */

  previous = 0;
  scan = hashlist[0];
  while (scan && scan != p)
  {
    previous = scan;
    scan = scan->hash_next;
  }
  if (!scan)
    log("error", "Bad non-name hash list");
  else if (!previous)
    hashlist[0] = p->hash_next;
  else
    previous->hash_next = p->hash_next;

  /* now place into named hashed lists */

  hash = (int) (p->lower_name[0]) - (int) 'a' + 1;
  p->hash_next = hashlist[hash];
  hashlist[hash] = p;
  p->hash_top = hash;


  if (p->flags & SITE_LOG)
    LOGF("site", "%s - %s", p->name, p->inet_addr);

  t = time(0);
  log_time = localtime(&t);

  p->flags |= PROMPT;
  p->timer_fn = 0;
  p->timer_count = -1;

  p->system_flags &= ~NEW_SITE;

  p->mode = NONE;

  if (p->residency != NON_RESIDENT)
  {
    if (p->residency & SU && true_count_su() <= 1)
      in_sulog(p);
    p->logged_in = 1;
    sp = p->saved;

    if (motd_is_new(p, "motd"))
    {
      motd(p, "");
      do_prompt(p, get_plists_msg("hit_return"));
      if (p->residency & PSU && motd_is_new(p, "sumotd"))
	p->input_to_fn = show_sumotd_login;
      else
	p->input_to_fn = finish_player_login;
    }
    else if (p->residency & PSU && motd_is_new(p, "sumotd"))
      show_sumotd_login(p, "");
    else
    {
      finish_player_login(p, "");
      return;
    }
  }
  else
  {
    tell_player(p, hitells_msg.where);
    do_prompt(p, "Enter your termtype [vt100]: ");
    p->input_to_fn = set_term_hitells_asty;
  }
  return;
}

/* get new gender */

void enter_gender(player * p, char *str)
{
  switch (tolower(*str))
  {
    case 'm':
      p->gender = MALE;
      tell_player(p, " Gender set to Male.\n");
      break;
    case 'f':
      p->gender = FEMALE;
      tell_player(p, " Gender set to Female.\n");
      break;
    case 'n':
      p->gender = OTHER;
      tell_player(p, " Gender set to well, erm, something.\n");
      break;
    default:
      tell_player(p, " No gender set.\n");
      break;
  }
  p->input_to_fn = 0;
  link_to_program(p);
}


/* time out */

void login_timeout(player * p)
{
  tell_player(p, get_plists_msg("too_long_time"));
  quit(p, 0);
}


/* newbie stuff */

void newbie_get_gender(player * p, char *str)
{
  player *scan;

  /* Alert super users of impending newbie. Note that only lower_admins
     or above can see the account name */

  sys_color_atm = SUCsc;
  sys_flags &= ~ROOM_TAG;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (scan->residency & PSU && !(scan->flags & BLOCK_SU)
	&& scan != p)
    {
      if (scan->flags & NO_SU_WALL)
      {
	scan->flags &= ~NO_SU_WALL;
      }
      else
      {
	if (scan->misc_flags & SU_HILITED)
	{
	  command_type |= HIGHLIGHT;
	}
	TELLPLAYER(scan, get_plists_msg("newbie_login"),
		   p->name, get_address(p, scan));
	if (scan->misc_flags & SU_HILITED)
	{
	  command_type &= ~HIGHLIGHT;
	}
      }
    }
  }

  p->newbieinform = 1;

  do_prompt(p, get_plists_msg("gender_msg"));
  p->input_to_fn = enter_gender;
}

void got_new_name(player * p, char *str)
{
  char *oldstack, *cpy;
  int length = 0;
  player *search;
  char name[MAX_NAME + 1];

  oldstack = stack;

  if (!strcasecmp("who", str))
  {
    swho(p, 0);
    p->input_to_fn = got_new_name;
    do_prompt(p, get_plists_msg("second_name"));
    stack = oldstack;
    return;
  }

  if (!strcasecmp("version", str))
  {
    pg_version(p, "");
    p->input_to_fn = got_new_name;
    do_prompt(p, get_plists_msg("second_name"));
    stack = oldstack;
    return;
  }

  for (cpy = str; *cpy; cpy++)
    if (isalpha(*cpy))
    {
      *stack++ = *cpy;
      length++;
    }
  *stack++ = 0;
  length++;

  if (!strcasecmp("finger", oldstack))
  {
    if (config_flags & cfFINGERLOGIN)
    {
      if (!*str)
      {
	cpy++;
	strncpy(name, cpy, MAX_NAME);
	p->misc_flags |= NOCOLOR;
	newfinger(p, cpy);
	do_prompt(p, get_plists_msg("second_name"));
	p->input_to_fn = got_name;
	stack = oldstack;
	return;
      }
      /*
       * Else they just logged in as "finger", so ask who they want =-) 
       */
      do_prompt(p, "Enter a player to finger: ");
      p->input_to_fn = finger_at_name_prompt;
      return;
    }
    else
    {
      tell_player(p, " Sorry, that name is reserved, please choose another.\n");
      do_prompt(p, get_plists_msg("second_name"));
      p->input_to_fn = got_name;
      stack = oldstack;
      return;
    }
  }

  if (reserved_name(str))
  {  
    tell_player(p, "Sorry, but you cannot use that name. Please choose another.\n");
    do_prompt(p, get_plists_msg("second_name"));  
    p->input_to_fn = got_name;  
    stack = oldstack;  
    return;  
  }  
     

  if (length > (MAX_NAME - 3))
  {
    tell_player(p, get_plists_msg("too_long_name"));
    do_prompt(p, get_plists_msg("second_name"));
    p->input_to_fn = got_new_name;
    if (sys_flags & VERBOSE)
      LOGF("connection", "Name too long : %s\n", str);

    stack = oldstack;
    return;
  }

  if (length < 3)
  {
    tell_player(p, get_plists_msg("too_short_name"));
    do_prompt(p, get_plists_msg("second_name"));
    p->input_to_fn = got_new_name;
    stack = oldstack;
    return;
  }

  /*
   * Check to see if the player is trying to choose a name that they
   * can't pick at the normal login prompt (who and finger) -- astyanax
   */
  if (!strcasecmp(oldstack, "finger") || !strcasecmp(oldstack, "who") ||
      !strcasecmp(oldstack, "version"))
  {
    tell_player(p, " Sorry, that name is reserved, please choose another.\n");
    do_prompt(p, get_plists_msg("second_name"));
    p->input_to_fn = got_new_name;
    stack = oldstack;
    return;
  }
  /* Or they REALLY didn't want to log in, let them quit */
  if (!strcasecmp(oldstack, "quit"))
  {
    TELLPLAYER(p, "%s\n", get_plists_msg("login_quit"));
    quit(p, 0);
    return;
  }

  if (!strcasecmp(oldstack, "guest"))
  {
    TELLPLAYER(p, " Sorry, but the names containing \"guest\" are reserved"
	       " for new users to logon with and cannot be used"
	       " beyond this point.\n\n");
    TELLPLAYER(p, " In selecting a name, we suggest that you use a name"
	       " choose something that shows your interests or your"
	       " hobbies or your personality. Choosing your real"
	       " name here is probably not a good idea, because you"
	       " will be telling many total strangers (at this point)"
	       " what your real name is.\n\n");
    TELLPLAYER(p, " Remember that, once you"
	       " have a name, it can be difficult or impossible to"
	       " change it later, so choose carefully. If you get an"
	       " error message that the name you picked cannot be used"
	       " then simply choose another -- it means that someone"
	       " else has already reserved that name and you cannot use"
	       " it.\n\n");
    do_prompt(p, get_plists_msg("second_name"));
    p->input_to_fn = got_new_name;
    stack = oldstack;
    return;
  }
  if (restore_player_title(p, oldstack, 0))
  {
    tell_player(p, " Sorry, there is already someone who uses the "
		"program with that name.\n\n");
    do_prompt(p, get_plists_msg("second_name"));
    p->input_to_fn = got_new_name;
    stack = oldstack;
    return;
  }
  search = hashlist[((int) (p->lower_name[0])) - (int) 'a' + 1];
  for (; search; search = search->hash_next)
    if (!strcmp(p->lower_name, search->lower_name))
    {
      TELLPLAYER(p, "\n Sorry there is already someone on %s "
		 "with that name.\nPlease try again, but use a "
		 "different name.\n\n", get_config_msg("talker_name"));
      stack = oldstack;
      do_prompt(p, get_plists_msg("second_name"));
      p->input_to_fn = got_new_name;
      return;
    }
  newbie_get_gender(p, str);
  stack = oldstack;
}


void newbie_got_name_answer(player * p, char *str)
{
  switch (tolower(*str))
  {
    case 'y':
      newbie_get_gender(p, str);
      break;
    case 'n':
      tell_player(p, get_plists_msg("new_name_msg"));
      do_prompt(p, get_plists_msg("second_name"));
      p->input_to_fn = got_new_name;
      break;
    default:
      tell_player(p, " Please answer with Y or N.\n");
      newbie_check_name(p, str);
      break;
  }
}

void check_hcadmin_pw(player * p, char *str)
{

  p->input_to_fn = 0;
  password_mode_off(p);
  if (strcmp(str, get_config_msg("hc_passwd")))
  {
    TELLPLAYER(p, "\n\n ILLEGAL ACCESS!! Bye!! \n\n");
    /* alert the sus and admins on */
    SUWALL(" -=*> An attempt to login as %s was just made!!\n", p->name);
    LOGF("sufailpass", "someone failed to login as hcadmin %s from %s", p->name, get_address(p, NULL));
    quit(p, 0);
  }
  else
  {
    TELLPLAYER(p, "\nAccess accepted.\n");
    link_to_program(p);
  }
}

void hcadmin_check_password(player * p)
{

  /* this is a security measure against newbie hcadmins */

  TELLPLAYER(p, " You are attempting to login as a hard coded admin.\n"
	     " To continue, you must enter the HCADMIN access password.\n");
  do_prompt(p, "Enter the HCADMIN ACCESS PASSWORD: ");
  password_mode_on(p);

  p->input_to_fn = check_hcadmin_pw;
}
void newbie_check_name(player * p, char *str)
{
  char *oldstack;
  oldstack = stack;

  if (strcasecmp(p->name, "guest"))
  {
    TELLPLAYER(p, get_plists_msg("name_confirm"), p->name);
    do_prompt(p, "\nAnswer Y or N: ");
    p->input_to_fn = newbie_got_name_answer;
    stack = oldstack;
  }
  else
  {
    TELLPLAYER(p, "\nAt this point, I am going to ask you to choose a name"
	       " to use on this program.  Some of the names may"
	       " already be taken by someone else, if that happens,"
	       " just try a different name\n\n\n");
    do_prompt(p, get_plists_msg("initial_name"));
    p->input_to_fn = got_new_name;
  }
}

void newbie_start(player * p, char *str)
{
  tell_player(p, newpage2_msg.where);
  do_prompt(p, get_plists_msg("hit_return"));
  p->input_to_fn = newbie_check_name;
}


/* test password */

int check_password(char *password, char *entered, player * p)
{
  char key[9];
  strncpy(key, entered, 8);
  return (!strncmp(crypt(key, p->lower_name), password, 11));
}


void got_password(player * p, char *str)
{
  char *oldstack;
  oldstack = stack;
  p->input_to_fn = 0;
  password_mode_off(p);

  if (!*str)
  {
    tell_player(p, "\n\n");
    do_prompt(p, get_plists_msg("second_name"));
    p->input_to_fn = got_name;
    return;
  }

  if (!check_password(p->password, str, p))
  {
    if (p->password_fail < 3)
    {
      if (p->password_fail == 2)
	tell_player(p, "\n Incorrect password - last chance!\n\n");
      else
	tell_player(p, "\n Incorrect password - try again!\n\n");
      p->password_fail++;
      password_mode_on(p);
      do_prompt(p, get_plists_msg("passwd_prompt"));
      p->input_to_fn = got_password;
      return;
    }
    tell_player(p, get_plists_msg("passwd_fail"));
    sprintf(stack, "Password fail: %s - %s", get_address(p, NULL), p->name);
    stack = end_string(stack);
    if (p->residency & SU)
      log("sufailpass", oldstack);
    else
      log("connection", oldstack);
    stack = oldstack;
    p->flags |= NO_SAVE_LAST_ON;
    quit(p, "");
    return;
  }
  if (p->gender < 0)
  {
    p->input_to_fn = enter_gender;
    tell_player(p, "\n You have no gender set.\n");
    do_prompt(p, "Please choose M(ale), F(female) or N(ot applicable): ");
    return;
  }
  stack = oldstack;
  link_to_program(p);
}

/* check for soft splats */

int site_soft_splat(player * p)
{
  int no1 = 0, no2 = 0, no3 = 0, no4 = 0, out;

  out = time(0);
  sscanf(p->num_addr, "%d.%d.%d.%d", &no1, &no2, &no3, &no4);
  if (out <= soft_timeout && no1 == soft_splat1 && no2 == soft_splat2)
    return 1;
  else
    return 0;
}

void newbie_was_screend(player * p)
{
  tell_player(p, nonewbies_msg.where);
  quit(p, "");
}

void newbie_dummy_fn(player * p, char *str)
{
  tell_player(p, "\n One moment, please.\n\n");
}

void inform_sus_screening(player * p)
{
  SUWALL("\n");
  SUWALL(" -=*> Screening Information\n"
	 " -=*> '%s' is a newbie from %s\n"
	 " -=*> To allow them to connect, type:  allow %s\n"
	 " -=*> To make them try later, type:    deny %s\n\n",
	 p->name, p->inet_addr, p->lower_name, p->lower_name);
}


/* calls here when the player has entered their name */

void got_name(player * p, char *str)
{
  int t;
  char *oldstack, *cpy, *space;
  int length = 0, iss = 0, nologin;
  player *search;

  oldstack = stack;

  for (cpy = str; *cpy && *cpy != ' '; cpy++)
    if (isalpha(*cpy))
    {
      *stack++ = *cpy;
      length++;
    }
  if (*cpy == ' ')
    iss = 1;
  *stack++ = 0;
  space = stack;
  length++;

  if (length > (MAX_NAME - 3))
  {
    tell_player(p, get_plists_msg("too_long_name"));
    do_prompt(p, get_plists_msg("second_name"));
    p->input_to_fn = got_name;

    if (sys_flags & VERBOSE)
      LOGF("connection", "Name too long : %s\n", str);

    stack = oldstack;
    return;
  }
  if (length < 3)
  {
    tell_player(p, get_plists_msg("too_short_name"));
    do_prompt(p, get_plists_msg("second_name"));
    p->input_to_fn = got_name;
    stack = oldstack;
    return;
  }
  if (!strcasecmp("who", oldstack))
  {
    swho(p, 0);
    p->input_to_fn = got_name;
    do_prompt(p, get_plists_msg("second_name"));
    stack = oldstack;
    return;
  }
  if (!strcasecmp("version", str))
  {
    pg_version(p, "");
    p->input_to_fn = got_name;
    do_prompt(p, get_plists_msg("second_name"));
    stack = oldstack;
    return;
  }

  if (!strcasecmp("finger", oldstack))
  {
    if (config_flags & cfFINGERLOGIN)
    {
      char name[MAX_NAME + 1];

      if (iss)
      {
	cpy++;
	strncpy(name, cpy, MAX_NAME);
	p->misc_flags |= NOCOLOR;
	newfinger(p, cpy);
	do_prompt(p, get_plists_msg("initial_name"));
	p->input_to_fn = got_name;
	stack = oldstack;
	return;
      }
      /*
       * Else they just logged in as "finger", so ask who they want =-) 
       */
      do_prompt(p, "Enter a player to finger: ");
      p->input_to_fn = finger_at_name_prompt;
      stack = oldstack;
      return;
    }
    else
    {
      tell_player(p, " Sorry, that name is reserved, please choose another.\n");
      do_prompt(p, "Pleaser enter a name: ");
      p->input_to_fn = got_name;
      stack = oldstack;
      return;
    }
  }

  if (!strcasecmp("quit", oldstack))
  {
    quit(p, "");
    stack = oldstack;
    return;
  }
  if (reserved_name(oldstack))
  {
    tell_player(p, "Sorry, but you cannot use that name. Please choose another.\n");
    do_prompt(p, get_plists_msg("second_name"));
    p->input_to_fn = got_name;
    stack = oldstack;
    return;
  }

  if (iss)
    while (*++cpy == ' ');
  if (restore_player_title(p, oldstack, iss ? cpy : 0))
  {
    if (p->residency & BANISHD && strcasecmp("guest", oldstack))
    {
      tell_player(p, banned_msg.where);
      quit(p, "");
      stack = oldstack;
      return;
    }
    if (!strcasecmp("guest", oldstack))
    {
      if (p->flags & CLOSED_TO_NEWBIES)
      {
	tell_player(p, newban_msg.where);
	quit(p, "");
	stack = oldstack;
	return;
      }
      if (site_soft_splat(p))
      {
	tell_player(p, newban_msg.where);
	quit(p, "");
	stack = oldstack;
	return;
      }
    }
    if (p->residency == SYSTEM_ROOM)
    {
      tell_player(p, "\n Sorry but that name is reserved.\n"
		  " Choose a different name ...\n\n");
      do_prompt(p, get_plists_msg("second_name"));
      p->input_to_fn = got_name;
      stack = oldstack;
      return;
    }
    t = time(0);
    if (p->sneezed > t)
    {
      nologin = p->sneezed - t;
      TELLPLAYER(p, get_plists_msg("sneeze_login"), word_time(nologin));
      quit(p, "");
      stack = oldstack;
      return;
    }
    else
      p->sneezed = 0;
/* login limits checks here */
    if (!(p->residency & (SU | LOWER_ADMIN | ADMIN)))
    {
      if (current_players >= max_players)
      {
	tell_player(p, full_msg.where);
	quit(p, 0);
	stack = oldstack;
	return;
      }
      else if (shutdown_count < 180 && shutdown_count != -1)
      {
	tell_player(p, get_plists_msg("talker_rebooting"));
	quit(p, 0);
	stack = oldstack;
	return;
      }
      else if (sys_flags & CLOSED_TO_RESSIES)
      {
	tell_player(p, noressies_msg.where);
	quit(p, "");
	stack = oldstack;
	return;
      }
    }
    TELLPLAYER(p, get_plists_msg("char_exists"), p->name);

    if (p->password[0])
    {
      password_mode_on(p);
      do_prompt(p, get_plists_msg("passwd_prompt"));
      p->input_to_fn = got_password;
      p->timer_count = 60;
      p->timer_fn = login_timeout;
      stack = oldstack;
      return;
    }
    stack = oldstack;
    p->input_to_fn = 0;
    link_to_program(p);
    return;
  }
  p->input_to_fn = 0;

  if (p->flags & CLOSED_TO_NEWBIES)
  {
    tell_player(p, newban_msg.where);
    quit(p, "");
    stack = oldstack;
    return;
  }
  if (site_soft_splat(p))
  {
    tell_player(p, newban_msg.where);
    quit(p, "");
    stack = oldstack;
    return;
  }
  if (shutdown_count < 180 && shutdown_count != -1)
  {
    TELLPLAYER(p,
	  " We are sorry, but %s is currently rebooting for normal system\n"
	       " maintenance. We will return in less than 3 minutes, so please try again in\n"
	       " a couple minutes. Thank you =)\n",
	       get_config_msg("talker_name"));
    quit(p, 0);
    stack = oldstack;
    return;
  }
  if (sys_flags & CLOSED_TO_NEWBIES ||
      (true_count_su() == 0 && config_flags & cfNOSUNONEW))
  {
    tell_player(p, nonewbies_msg.where);
    quit(p, "");
    stack = oldstack;
    return;
  }
  if (sys_flags & SCREENING_NEWBIES)
  {
    if (count_su() < 1)
    {
      tell_player(p, nonewbies_msg.where);
      quit(p, "");
      stack = oldstack;
      return;
    }
    p->timer_fn = newbie_was_screend;
    p->timer_count = 120;
    p->input_to_fn = newbie_dummy_fn;
    inform_sus_screening(p);
    stack = oldstack;
    return;
  }

  search = hashlist[((int) (p->lower_name[0])) - (int) 'a' + 1];
  for (; search; search = search->hash_next)
    if (!strcmp(p->lower_name, search->lower_name))
    {
      TELLPLAYER(p, "\n Sorry there is already someone on %s "
		 "with that name.\nPlease try again, but use a "
		 "different name.\n\n", get_config_msg("talker_name"));
      stack = oldstack;
      do_prompt(p, get_plists_msg("second_name"));
      p->input_to_fn = got_name;
      return;
    }
  tell_player(p, newpage1_msg.where);
  do_prompt(p, get_plists_msg("hit_return"));
  p->input_to_fn = newbie_start;
  p->timer_count = 1800;
  stack = oldstack;
}

/* a new player has connected */

void connect_to_prog(player * p)
{
  player *cp;
  int wcm;
  char *oldstack = stack;

  wcm = (rand() % 3);
  cp = current_player;
  current_player = p;
  tell_player(p, "\377\373\031");	/* send will EOR */
  if (!wcm)
    tell_player(p, connect_msg.where);
  else if (wcm == 1)
    tell_player(p, connect2_msg.where);
  else
    tell_player(p, connect3_msg.where);

  /* For people with Tinyfugue */
  p->flags |= IAC_GA_DO;

  if (config_flags & cfFINGERLOGIN)
    sprintf(stack, "%s(or 'who' or 'finger'): ",
	    get_plists_msg("initial_name"));
  else
    sprintf(stack, "%s(or 'who'): ", get_plists_msg("initial_name"));

  stack = end_string(stack);
  do_prompt(p, oldstack);
  stack = oldstack;
  p->input_to_fn = got_name;
  p->timer_count = 3 * ONE_MINUTE;
  p->timer_fn = login_timeout;
  current_player = cp;
}

/* tell player motd.

   Please note that the top part of this MOTD (above the first LINE)
   should not be edited under the terms of the licence agreement
 */

void motd(player * p, char *str)
{
  char *oldstack = stack, mid[80];

  if (!p->residency)
    tell_player(p, newbie_msg.where);

  sprintf(mid, "%s Message of the Day", get_config_msg("talker_name"));
  pstack_mid(mid);
  sprintf(stack,
  "   This talker is based on Playground+ written by Silver, a stable and\n"
	  "  bug free version of PG96 by traP, astyanax, Nogard and vallie which is\n"
  "   based on Summink by Athanasius which is based on EW2 by Simon Marsh.\n"
	  LINE
	  "  Problems or questions should be sent to %s\n"
	  "  Our address is %s %d (or) %s %d\n"
	  LINE
	  "%s\n", get_config_msg("talker_email"), talker_alpha, active_port, talker_ip,
	  active_port, motd_msg.where);

  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

void fingerpaint(player * p, char *str)
{
  int test = 0;

  if (!p->term)
  {
    tell_player(p, " You must have hitells set to see color.\n");
    return;
  }
  if (p->misc_flags & NOCOLOR)
  {
    test = 1;
    p->misc_flags &= ~NOCOLOR;
  }
  tell_player(p, fingerpaint_msg.where);
  if (test)
    p->misc_flags |= NOCOLOR;
}

/* tell player sumotd */
void sumotd(player * p, char *str)
{
  tell_player(p, sumotd_msg.where);
}


/* init everything needed for the plist file */

void init_plist()
{
  char *oldstack;
  int i;

  oldstack = stack;

  hard_load_files();
  for (i = 0; i < 26; i++)
    update[i] = 0;

  stack = oldstack;
}


/* various associated routines */

/* find one player given a (partial) name */

/* little subroutinette */

int match_player(char *str1, char *str2)
{
  for (; *str2; str1++, str2++)
  {
    if (*str1 != *str2 && *str2 != ' ')
    {
      if (!(*str1) && *str2 == '.' && !(*(str2 + 1)))
	return 1;
      return 0;
    }
  }
  if (*str1)
    return -1;
  return 1;
}


/* command to view res files */

void view_saved_lists(player * p, char *str)
{
  saved_player *scan, **hash;
  int i, j, hit = 0, res = 0, ban = 0, rom = 0;
  char *oldstack;
  oldstack = stack;
  if (!*str || !isalpha(*str))
  {
    tell_player(p, " Format : lsr <letter>\n");
    return;
  }
  strcpy(stack, "[HASH] [NAME]               " RES_BIT_HEAD "\n");
  stack = strchr(stack, 0);
  hash = saved_hash[((int) (tolower(*str)) - (int) 'a')];
  for (i = 0; i < HASH_SIZE; i++, hash++)
    for (scan = *hash; scan; scan = scan->next)
    {
      hit++;
      sprintf(stack, "[%d]", i);
      j = strlen(stack);
      stack = strchr(stack, 0);
      for (j = 7 - j; j; j--)
	*stack++ = ' ';
      strcpy(stack, scan->lower_name);
      j = strlen(stack);
      stack = strchr(stack, 0);
      for (j = 21 - j; j; j--)
	*stack++ = ' ';
      switch (scan->residency)
      {
	case SYSTEM_ROOM:
	  rom++;
	  strcpy(stack, "Standard room file.");
	  break;
	case BANISHD:
	  ban++;
	  strcpy(stack, "BANISHED (Name Only)");
	  break;
	default:
/* lets show there privs anyways..as there Is a pfile there
   if (scan->residency & BANISHD)
   {
   strcpy(stack, "BANISHED");
   }
   else
 */
	  res++;
	  strcpy(stack, privs_bit_string(scan->residency));
	  break;
      }
      stack = strchr(stack, 0);
      *stack++ = '\n';
    }
  if (hit)
  {
    stack += sprintf(stack, "--- %d files (", hit);
    if (res)
      stack += sprintf(stack, "%d res", res);
    if (ban)
      stack += sprintf(stack, ", ");
    if (ban > 1)
      stack += sprintf(stack, "%d bans", ban);
    else if (ban == 1)
      stack += sprintf(stack, "1 ban");
    if (rom)
      stack += sprintf(stack, ", ");
    if (rom > 1)
      stack += sprintf(stack, "%d rooms", rom);
    else if (rom == 1)
      stack += sprintf(stack, "1 room");
    stack += sprintf(stack, ") in the '%c' hash\n", *str);
    *stack++ = 0;
    pager(p, oldstack);
    stack = oldstack;
    return;
  }
  stack = oldstack;
  TELLPLAYER(p, " There are no files at all in the '%c' hash\n", *str);
}

/* external routine to check updates */

void check_updates(player * p, char *str)
{
  char *oldstack;
  int i;
  oldstack = stack;
  strcpy(stack, "abcdefghijklmnopqrstuvwxyz\n");
  stack = strchr(stack, 0);
  for (i = 0; i < 26; i++)
    if (update[i])
      *stack++ = '*';
    else
      *stack++ = ' ';
  *stack++ = '\n';
  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}


void player_flags(player * p)
{
  char *oldstack = stack, *str;

  str = stack;
  if (p->residency == NON_RESIDENT)
  {
    LOGF("error", "trying to player_flags newbie [%s]", p->lower_name);
    return;
  }
  if (!p->saved && p->residency & HCADMIN)
    tell_player(p, " Set your email and password ASAFP.\n");
  else if (!p->saved)
    tell_player(p, " Eeeeeeek ! No saved bits !\n");
  else
  {

    TELLPLAYER(p, "\n --\n Last logged in %s (%s ago)\n",
	       convert_time(p->saved->last_on),
	       significant_time(time(0) - p->saved->last_on));
    if (strcasecmp(p->saved->last_host, p->inet_addr))
      TELLPLAYER(p, " From %s ...\n", p->saved->last_host);

    strcpy(stack, " You are ");
    stack = strchr(stack, 0);
    if (p->custom_flags & HIDING)
    {
      strcpy(stack, "in hiding, ");
      stack = strchr(stack, 0);
    }
    if (p->tag_flags & BLOCK_SHOUT)
    {
      strcpy(stack, "ignoring shouts, ");
      stack = strchr(stack, 0);
    }
    if (p->tag_flags & BLOCK_TELLS)
    {
      strcpy(stack, "ignoring tells, ");
      stack = strchr(stack, 0);
    }
    if (p->tag_flags & SINGBLOCK)
    {
      strcpy(stack, "ignoring singing, ");
      stack = strchr(stack, 0);
    }
    if (p->custom_flags & CONVERSE)
    {
      strcpy(stack, "in converse mode, ");
      stack = strchr(stack, 0);
    }
    if (p->custom_flags & NOPREFIX)
    {
      strcpy(stack, "ignoring prefixes, ");
      stack = strchr(stack, 0);
    }
    if ((str = strrchr(oldstack, ',')))
    {
      *str++ = '.';
      *str++ = '\n';
      *str++ = 0;
      stack = strchr(stack, 0);
      stack++;
      tell_player(p, oldstack);
    }
    if (p->system_flags & NEW_MAIL)
    {
      TELLPLAYER(p, " %s\n", get_config_msg("new_mail"));
      p->system_flags &= ~NEW_MAIL;
      p->saved->system_flags &= ~NEW_MAIL;
    }

    if (!(p->custom_flags & NO_NEW_NEWS_INFORM))
    {
      command_type |= HIGHLIGHT;
      new_news_inform(p);
      command_type &= ~HIGHLIGHT;
    }

    if (p->residency & (SU | ADC | HCADMIN))
      show_logs(p, "new");
    if (p->residency & SU && sys_flags & CLOSED_TO_NEWBIES)
      TELLPLAYER(p, " %s is closed to newbies.\n",
		 get_config_msg("talker_name"));
    if (p->residency & SU && sys_flags & SCREENING_NEWBIES &&
	!(sys_flags & CLOSED_TO_NEWBIES))
      TELLPLAYER(p, " %s is screening newbies\n",
		 get_config_msg("talker_name"));
  }
  tell_player(p, " --\n");
  stack = oldstack;
}

void player_flags_verbose(player * p, char *str)
{
  char *oldstack, *wibble, *argh;
  player *p2;
  oldstack = stack;

  if (*str && (p->residency & SU || p->residency & ADMIN))
  {
    p2 = find_player_absolute_quiet(str);
    if (!p2)
    {
      tell_player(p, " No-one on of that name.\n");
      return;
    }
  }
  else
    p2 = p;

  if (p2->residency == NON_RESIDENT)
  {
    tell_player(p, " You aren't a resident, so your character won't be "
		"saved when you log off.\n");
    return;
  }
  strcpy(stack, "\n");
  stack = strchr(stack, 0);
  argh = stack;
  strcpy(stack, " You are ");
  stack = strchr(stack, 0);

  if (p2->custom_flags & HIDING)
  {
    strcpy(stack, "in hiding, ");
    stack = strchr(stack, 0);
  }
  if (p2->tag_flags & BLOCK_SHOUT)
  {
    strcpy(stack, "ignoring shouts, ");
    stack = strchr(stack, 0);
  }
  if (p2->tag_flags & BLOCK_TELLS)
  {
    strcpy(stack, "ignoring tells, ");
    stack = strchr(stack, 0);
  }
  if (p2->tag_flags & SINGBLOCK)
  {
    strcpy(stack, "ignoring singing, ");
    stack = strchr(stack, 0);
  }
  if (p2->custom_flags & CONVERSE)
  {
    strcpy(stack, "in converse mode, ");
    stack = strchr(stack, 0);
  }
  if (p2->custom_flags & NOPREFIX)
  {
    strcpy(stack, "ignoring prefixes, ");
    stack = strchr(stack, 0);
  }
  if ((wibble = strrchr(oldstack, ',')))
  {
    *wibble++ = '.';
    *wibble++ = '\n';
    *wibble = 0;
  }
  else
    stack = argh;

  if (p2->custom_flags & PRIVATE_EMAIL)
    strcpy(stack, " Your email is private.\n");
  else
    strcpy(stack, " Your email is public for all to see.\n");
  stack = strchr(stack, 0);

  if (p2->custom_flags & TRANS_TO_HOME)
  {
    strcpy(stack, " You will be taken to your home when you log in.\n");
    stack = strchr(stack, 0);
  }
  else if (*p2->room_connect)
  {
    sprintf(stack, " You will try to connect to room '%s' when you log"
	    " in\n", p->room_connect);
    stack = strchr(stack, 0);
  }
  if (p2->tag_flags & NO_ANONYMOUS)
    strcpy(stack, " You won't receive anonymous mail.\n");
  else
    strcpy(stack, " You are currently able to receive anonymous mail.\n");
  stack = strchr(stack, 0);

  if (p2->system_flags & IAC_GA_ON)
    strcpy(stack, " Iacga prompting is turned on.\n");
  else
    strcpy(stack, " Iacga prompting is turned off.\n");
  stack = strchr(stack, 0);

  if (p2->custom_flags & NO_PAGER)
    strcpy(stack, " You are not recieving paged output.\n");
  else
    strcpy(stack, " You are recieving paged output.\n");
  stack = strchr(stack, 0);

  if (p2->flags & BLOCK_SU && p->residency & PSU)
  {
    strcpy(stack, " You are ignoring sus.\n");
    stack = strchr(stack, 0);
  }
  /* will they be notified when people enter their rooms (ie have they
     room notify set) */
  if (p2->custom_flags & ROOM_ENTER)
    strcpy(stack, " You will be informed when someone enters one of "
	   "your rooms.\n");
  else
    strcpy(stack, " You will not be informed when someone enters "
	   "one of your rooms.\n");
  stack = strchr(stack, 0);

  strcpy(stack, "\n");
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}


/* Catch SEGV's and BUS's on load of players, hopefully... */

void error_on_load(int c)
{
  bad_player_load = 1;
  longjmp(jmp_env, 0);
  longjmp(jmp_env, 0);
}



/* Count number of ressies, give a small statistic - by Nogard */
/* spoon additions by trap =) */
/* bug fix by Silver :o) */
/* softmsgfied/cleaned by phypor ;) */

void res_count(player * p, char *str)
{
  int blessed = 0, psu = 0, bsu = 0, asu = 0, banned = 0, ladmin = 0, uadmin = 0,
    hcadmin = 0, coders = 0, gits = 0, players = 0, ressies = 0, staff = 0,
    builder = 0, creators = 0, spod = 0, minister = 0, married = 0, engaged = 0,
    flirt = 0;

  int counter, charcounter;
  saved_player *scanlist, **hlist;
  char *oldstack, top[70];

  oldstack = stack;
  for (charcounter = 0; charcounter < 26; charcounter++)
  {
    /* Nothing like a good bit of hash ... oops! :o) */

    hlist = saved_hash[charcounter];
    for (counter = 0; counter < HASH_SIZE; counter++, hlist++)
      for (scanlist = *hlist; scanlist; scanlist = scanlist->next)
      {
	switch (scanlist->residency)
	{
	  case SYSTEM_ROOM:	/* You forgot this one ppl --Silver */
#ifdef ROBOTS
	  case ROBOT_PRIV:
#endif
	  case BANISHD:
	    banned++;
	    break;
	  default:
	    if (!(scanlist->residency & BANISHD))
	    {
	      if (scanlist->residency & GIT)
		gits++;
	      if (scanlist->residency & SPOD)
		spod++;
	      if (scanlist->residency & MINISTER)
		minister++;
	      if (scanlist->residency & BUILDER)
		builder++;
	      if (scanlist->residency & SPECIALK)
		creators++;
	      if (scanlist->system_flags & MARRIED)
		married++;
	      else if (scanlist->system_flags & ENGAGED)
		engaged++;
	      else if (scanlist->system_flags & FLIRT_BACHELOR)
		flirt++;

	      if (scanlist->residency & CODER)
		coders++;
	      else if (scanlist->residency & HCADMIN)
		hcadmin++;
	      else if (scanlist->residency & ADMIN)
		uadmin++;
	      else if (scanlist->residency & LOWER_ADMIN)
		ladmin++;
	      else if (scanlist->residency & ASU)
		asu++;
	      else if (scanlist->residency & SU)
		bsu++;
	      else if (scanlist->residency & PSU)
		psu++;
	      else if (scanlist->residency & (WARN | ADC | HOUSE | DUMB |
					      SCRIPT | TRACE | PROTECT))
		blessed++;
	      players++;
	    }
	    /* this if statement to weed out banishd players */
	    else
	      banned++;
	    break;
	}
      }
  }
  staff = psu + asu + bsu + ladmin + uadmin + hcadmin + coders;
  ressies = players - staff;

  sprintf(top, "%s Resident and Staff Count", get_config_msg("talker_name"));
  pstack_mid(top);

  if (p->residency & PSU && (*str != '-'))
  {
    stack += sprintf(stack, "\n         %-14s    : %-4d",
		     get_config_msg("hc_name"), hcadmin);

    stack += sprintf(stack, "     %-14s   : %-4d   \n",
		     get_config_msg("coder_name"), coders);

    stack += sprintf(stack, "         %-14s    : %-4d",
		     get_config_msg("admin_name"), uadmin);

    stack += sprintf(stack, "     %-14s   : %-4d   \n",
		     get_config_msg("la_name"), ladmin);

    stack += sprintf(stack, "         %-14s    : %-4d",
		     get_config_msg("asu_name"), asu);

    stack += sprintf(stack, "     %-14s   : %-4d   \n",
		     get_config_msg("su_name"), bsu);

    stack += sprintf(stack, "         %-14s    : %-4d",
		     get_config_msg("psu_name"), psu);

    stack += sprintf(stack, "     Blessed          : %-4d   \n",
		     blessed);

    stack += sprintf(stack, "\n"
	   "         (Builders)        : %-4d     (Creators)       : %-4d\n"
	   "         Spods             : %-4d     Ministers        : %-4d\n"
	 "         Married           : %-4d     Engaged          : %-4d\n\n"
	   "         <Banished>        : %-4d     <Gits>           : %-4d\n"
	 "         Normal Residents  : %-4d     Total Players    : %-4d\n\n"
		     LINE,
		     builder, creators, spod, minister, married, engaged,
		     banned, gits, ressies, players);
  }
  /* else its just a resident - tell them the total # -- astyanax */
  /* Tell them a bit more than just _that_  --Silver :o) */
  else
    sprintf(stack,
     "\n    Builders         : %-4d               Creators         : %-4d\n"
       "    Spods            : %-4d               Ministers        : %-4d\n"
       "    Married          : %-4d               Engaged          : %-4d\n"
     "    Normal Residents : %-4d               Staff and Admin  : %-4d\n\n"
     "                                          Total Residents  : %-4d\n\n"
	    LINE,
	    builder, creators, spod, minister, married, engaged, ressies, staff, players);

  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

/* Search for names that match a subset of a name - by Nogard 8-5-95 
   moved to plists.c -- maybe it'll work here :P -- traP */
void xref_name(player * p, char *str)
{
  saved_player *scanlist, **hlist;
  int counter, charcounter;
  char *oldstack;

  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: xref <string to search for> \n");
    return;
  }
  strcpy(stack, " Names that match ");
  stack = strchr(stack, 0);
  sprintf(stack, "%s :\n", str);

  for (charcounter = 0; charcounter < 26; charcounter++)
  {
    hlist = saved_hash[charcounter];
    for (counter = 0; counter < HASH_SIZE; counter++, hlist++)
      for (scanlist = *hlist; scanlist; scanlist = scanlist->next)
      {
	switch (scanlist->residency)
	{
	  case BANISHD:
	    break;
	  default:
	    if (strstr(scanlist->lower_name, str) ||
		(strstr(str, scanlist->lower_name)))
	    {
	      stack = strchr(stack, 0);
	      sprintf(stack, "%s, ", scanlist->lower_name);
	    }
	    break;
	}
      }
  }
  stack = strchr(stack, 0);
  stack -= 2;
  *stack++ = '.';
  *stack++ = '\n';
  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}


void xref_player_email(player * p, char *str)
{
  saved_player *scanlist, **hlist;
  int counter, charcounter;
  char *oldstack;
  char email[MAX_EMAIL + 2];

  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: etrace <string to search for> \n");
    return;
  }
  strcpy(stack, " Email addresses that match ");
  stack = strchr(stack, 0);
  sprintf(stack, "%s :\n", str);

  for (charcounter = 0; charcounter < 26; charcounter++)
  {
    hlist = saved_hash[charcounter];
    for (counter = 0; counter < HASH_SIZE; counter++, hlist++)
      for (scanlist = *hlist; scanlist; scanlist = scanlist->next)
      {
	switch (scanlist->residency)
	{
	  case BANISHD:
	  case BANISHED:
	    break;
	  default:
	    strncpy(email, scanlist->email, MAX_EMAIL - 3);
	    lower_case(email);
	    if ((scanlist->email) && strstr(email, str))
	    {
	      stack = strchr(stack, 0);
	      sprintf(stack, "%-20s - %s\n", scanlist->lower_name, scanlist->email);
	    }
	    break;
	}
      }
  }
  stack = strchr(stack, 0);
  *stack++ = 0;
  pager(p, oldstack);
  stack = oldstack;
}

void xref_player_site(player * p, char *str)
{
  saved_player *scanlist, **hlist;
  int counter, charcounter;
  char *oldstack;
  char lasthost[MAX_INET_ADDR + 2];

  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: itrace <site to search for> \n");
    return;
  }
  strcpy(stack, " Sites that match ");
  stack = strchr(stack, 0);
  sprintf(stack, "%s :\n", str);

  for (charcounter = 0; charcounter < 26; charcounter++)
  {
    hlist = saved_hash[charcounter];
    for (counter = 0; counter < HASH_SIZE; counter++, hlist++)
      for (scanlist = *hlist; scanlist; scanlist = scanlist->next)
      {
	switch (scanlist->residency)
	{
	  case BANISHD:
	  case BANISHED:
	    break;
	  default:
	    strncpy(lasthost, scanlist->last_host, MAX_INET_ADDR - 3);
	    lower_case(lasthost);
	    if ((scanlist->last_host) && strstr(lasthost, str))
	    {
	      stack = strchr(stack, 0);
	      sprintf(stack, "%-20s - %s\n", scanlist->lower_name, scanlist->last_host);
	    }
	    break;
	}
      }
  }
  stack = strchr(stack, 0);
  *stack++ = 0;
  pager(p, oldstack);
  stack = oldstack;
}


/* Playground to Playground+ conversion patches
   (These add, delete or change stuff in the pfile structure so that
   they are still compatable with PG+) */

void convert_pg_to_pgplus(player * p)
{
  /* If the player has set plural gender we change it to "unknown" */

  if (p->gender == 3)
    p->gender = OTHER;

  /* This is for old colorsets - it sets the colour for zchannels to red
     if it isn't already set (otherwise you'll get no colour!) */

  if (p->colorset[ZCHsc] == '\0')
    p->colorset[ZCHsc] = 'R';

  /* If the player is a builder set this on their residency flags since this
     is where it should have been in the first place!  */

  if (p->system_flags & OLD_BUILDER)
  {
    p->residency |= BUILDER;
    p->system_flags &= ~OLD_BUILDER;
  }
  if (p->system_flags & OLD_MINISTER)
  {
    p->residency |= MINISTER;
    p->system_flags &= ~OLD_MINISTER;
  }
}

/* list people married/engaged to each other */

void list_couples(player * p, char *str)
{
  saved_player *scanlist, **hlist;
  int counter, charcounter, i, flag = 0;
  char *oldstack = stack, top[70];
  player dummy, *p2;

  sprintf(top, "Couples married or engaged on %s",
	  get_config_msg("talker_name"));
  pstack_mid(top);

  for (i = 0; i < 2; i++)
  {
    for (charcounter = 0; charcounter < 26; charcounter++)
    {
      hlist = saved_hash[charcounter];
      for (counter = 0; counter < HASH_SIZE; counter++, hlist++)
	for (scanlist = *hlist; scanlist; scanlist = scanlist->next)
	{
	  switch (scanlist->residency)
	  {
	    case BANISHED:
	    case BANISHD:
	      break;
	    default:
	      p2 = find_player_absolute_quiet(scanlist->lower_name);
	      if (!p2)
	      {
		strcpy(dummy.lower_name, scanlist->lower_name);
		lower_case(dummy.lower_name);
		dummy.fd = p->fd;
		if (!(load_player(&dummy)))
		{
		  LOGF("error", "Error with player in list_couples!");
		  return;
		}
		p2 = &dummy;
	      }

	      if (i == 0 && (p2->system_flags & MARRIED) && strcasecmp(p2->name, p2->married_to) < 0)
	      {
		ADDSTACK("        %-20s is married to %20s\n", p2->name, p2->married_to);
		flag = 1;
	      }
	      else if (i == 1 && (p2->system_flags & ENGAGED) && !(p2->system_flags & MARRIED) && strcasecmp(p2->name, p2->married_to) < 0)
		ADDSTACK("        %-20s is engaged to %20s\n", p2->name, p2->married_to);
	  }
	}
    }
    if (i != 1 && flag == 1)	/* to make sure nicely formatted even when no-one is married */
      ADDSTACK("\n");
  }

  ENDSTACK(LINE);
  pager(p, oldstack);
  stack = oldstack;
}

saved_player *find_matched_sp_guts(char *str, int verb)
{
  saved_player *found = 0, *scan, **hash;
  int i, h = 0;
  char nstr[MAX_NAME];
  char *oldstack = stack;

  if (!*str || strlen(str) < 2 || !isalpha(*str))
  {
    if (current_player && verb)
      TELLPLAYER(current_player, " No match of the name "
		 "'%s' found in files.\n", str);
    return (saved_player *) NULL;
  }

  strncpy(nstr, str, MAX_NAME - 1);
  lower_case(nstr);

  hash = saved_hash[((int) (tolower(*str)) - (int) 'a')];
  for (i = 0; i < HASH_SIZE; i++, hash++)
  {
    for (scan = *hash; scan; scan = scan->next)
    {
      if (scan->residency == BANISHD)
	continue;

      if (!strncmp(scan->lower_name, nstr, strlen(nstr)))
      {
	stack += sprintf(stack, "%s, ", scan->lower_name);
	found = scan;
	h++;
      }
    }
  }

  if (!h)
  {
    if (current_player && verb)
      TELLPLAYER(current_player, " No match of the name "
		 "'%s' found in files.\n", str);
    stack = oldstack;
    return (saved_player *) NULL;
  }
  if (h > 1)
  {
    if (current_player && verb)
    {
      stack = end_string(stack);
      oldstack[strlen(oldstack) - 2] = '.';
      oldstack[strlen(oldstack) - 1] = '\0';
      TELLPLAYER(current_player, " Multiple matches:\n %s\n", oldstack);
    }
    stack = oldstack;
    return (saved_player *) NULL;
  }
  stack = oldstack;
  return found;
}


saved_player *find_matched_sp_quiet(char *str)
{
  return find_matched_sp_guts(str, 0);
}

saved_player *find_matched_sp_verbose(char *str)
{
  return find_matched_sp_guts(str, 1);
}

char *privs_bit_string(int p)
{
  static char buf[160];
  char *ptr;

  memset(buf, 0, 160);
  ptr = buf;

  if (p & NO_SYNC)
    *ptr++ = 'N';
  else
    *ptr++ = '_';
  if (p & BANISHD)
    *ptr++ = 'B';
  else
    *ptr++ = '_';
  if (p & SYSTEM_ROOM)
    *ptr++ = 'S';
  else
    *ptr++ = '_';
  if (p & ROBOT_PRIV)
    *ptr++ = 'R';
  else
    *ptr++ = '_';
  *ptr++ = '|';
  if (p & BASE)
    *ptr++ = 'b';
  else
    *ptr++ = '_';
  if (p & ECHO_PRIV)
    *ptr++ = 'e';
  else
    *ptr++ = '_';
  if (p & MAIL)
    *ptr++ = 'm';
  else
    *ptr++ = '_';
  if (p & LIST)
    *ptr++ = 'l';
  else
    *ptr++ = '_';
  if (p & BUILD)
    *ptr++ = 'r';
  else
    *ptr++ = '_';
  if (p & SESSION)
    *ptr++ = 's';
  else
    *ptr++ = '_';
  *ptr++ = '|';
  if (p & GIT)
    *ptr++ = 'G';
  else
    *ptr++ = '_';
  if (p & NO_TIMEOUT)
    *ptr++ = 'X';
  else
    *ptr++ = '_';
  if (p & SPOD)
    *ptr++ = 'O';
  else
    *ptr++ = '_';
  if (p & SPECIALK)
    *ptr++ = 'K';
  else
    *ptr++ = '_';
  if (p & HOUSE)
    *ptr++ = 'H';
  else
    *ptr++ = '_';
  if (p & MINISTER)
    *ptr++ = 'M';
  else
    *ptr++ = '_';
  if (p & SCRIPT)
    *ptr++ = 'S';
  else
    *ptr++ = '_';
  if (p & BUILDER)
    *ptr++ = 'B';
  else
    *ptr++ = '_';
  *ptr++ = '|';
  if (p & WARN)
    *ptr++ = 'W';
  else
    *ptr++ = '_';
  if (p & DUMB)
    *ptr++ = 'F';
  else
    *ptr++ = '_';
  if (p & PROTECT)
    *ptr++ = 'P';
  else
    *ptr++ = '_';
  if (p & TRACE)
    *ptr++ = 'T';
  else
    *ptr++ = '_';
  *ptr++ = '|';
  if (p & PSU)
    *ptr++ = '1';
  else
    *ptr++ = '_';
  if (p & SU)
    *ptr++ = '2';
  else
    *ptr++ = '_';
  if (p & ASU)
    *ptr++ = '3';
  else
    *ptr++ = '_';
  if (p & ADC)
    *ptr++ = '4';
  else
    *ptr++ = '_';
  if (p & LOWER_ADMIN)
    *ptr++ = '5';
  else
    *ptr++ = '_';
  if (p & ADMIN)
    *ptr++ = '6';
  else
    *ptr++ = '_';
  if (p & CODER)
    *ptr++ = '7';
  else
    *ptr++ = '_';
  if (p & HCADMIN)
    *ptr++ = '8';
  else
    *ptr++ = '_';

  return buf;
}



typedef char *search_func(player *, char *);

search_func
name_search, xref_search, email_search, site_search, url_search,
richest_search, resby_search, staff_stats_search, trouble_search,
swarn_search, git_search;


struct search_type
{
  char *type;
  int priv;
  search_func *search_func;
  char *desc;
}
SearchTypes[] =
{
  {
    "names", SU, name_search, "Search for string in names"
  }
  ,
  {
    "xref", SU, xref_search, "Search for string at beginning of names"
  }
  ,
  {
    "sites", SU, site_search, "Search for string in last site"
  }
  ,
  {
    "emails", ADMIN, email_search, "Search for string in emails"
  }
  ,
  {
    "urls", SU, url_search, "Search for string in urls"
  }
  ,
  {
    "gits", SU, git_search, "Search for string in git message"
  }
  ,
  {
    "swarn", SU, swarn_search, "Search for string in saved warns"
  }
  ,
  {
    "resby", LOWER_ADMIN, resby_search, "Search for players res'd by a certain su"
  }
  ,
  {
    "richest", LOWER_ADMIN, richest_search, "Search for players with excess money"
  }
  ,
  {
    "trouble", SU, trouble_search, "Search for trouble causers"
  }
  ,
  {
    "sstats", LOWER_ADMIN, staff_stats_search, "View staff stats"
  }
  ,
  {
    "", 0, 0, ""
  }
};
typedef struct search_type search_type;


void master_search(player * p, char *str, search_type * st)
{
  char c, *oldstack = stack, *got, mid[160];
  int i, hits = 0, outnow = 0;
  saved_player *scan, **hash;
  player dummy;

  if ((!strcasecmp(st->type, "richest") ||
       !strcasecmp(st->type, "trouble")) && !isdigit(*str))
  {
    tell_player(p, " For this search, you must supply a number\n");
    return;
  }

  if (*str)
    sprintf(mid, "Searching %s for '%s'", st->type, str);
  else
    sprintf(mid, "Output from %s", st->type);
  pstack_mid(mid);
  *stack++ = '\n';

  if (!strcasecmp(st->type, "sstats"))
    stack += sprintf(stack,
	     "                      resd warn boot rm'd ject  main idle\n");
  if (!strcasecmp(st->type, "trouble"))
    stack += sprintf(stack,
	  "                      warn ject boot idle  main idle git ban\n");

  for (c = 'a'; (c <= 'z' && !outnow); c++)
  {
    hash = saved_hash[((int) (tolower(c)) - (int) 'a')];
    for (i = 0; (i < HASH_SIZE && !outnow); i++, hash++)
    {
      for (scan = *hash; (scan && !outnow); scan = scan->next)
      {
	if (scan->residency == BANISHD ||
	    scan->residency & SYSTEM_ROOM)
	  continue;

	if (hits > 99)
	{
	  stack += sprintf(stack, "\n   --- Too many hits, search terminated ---\n\n");
	  outnow++;
	  continue;
	}
	memset(&dummy, 0, sizeof(player));
	strncpy(dummy.lower_name, scan->lower_name, MAX_NAME - 1);
	dummy.fd = p->fd;
	if (!load_player(&dummy))
	{
	  LOGF("error", "No player loaded for a saved player ! [%s]",
	       scan->lower_name);
	  TELLPLAYER(p, "No player loaded for a saved player [%s]",
		     scan->lower_name);
	  continue;
	}
	if ((got = (st->search_func) (&dummy, str)))
	{
	  hits++;
	  strcpy(stack, got);
	  stack = strchr(stack, 0);
	}
      }
    }
  }
  if (!hits)
  {
    stack = oldstack;
    TELLPLAYER(p, " No matches of '%s' found in %s search.\n",
	       str, st->type);
    return;
  }
  if (oldstack[strlen(oldstack) - 1] != '\n')
    *stack++ = '\n';
  *stack++ = '\n';

  if (hits == 1)
    sprintf(mid, "One %s match found", st->type);
  else
    sprintf(mid, "%d %s matches found", hits, st->type);
  pstack_mid(mid);
  *stack++ = 0;
  pager(p, oldstack);
  stack = oldstack;
}

void show_search_types(player * p)
{
  int i;
  char *oldstack = stack;

  pstack_mid("Areas you may search");
  *stack++ = '\n';

  for (i = 0; SearchTypes[i].type[0]; i++)
    if (!SearchTypes[i].priv || p->residency & SearchTypes[i].priv)
      stack += sprintf(stack, " %-20s %s\n", SearchTypes[i].type,
		       SearchTypes[i].desc);

  *stack++ = '\n';
  ENDSTACK(LINE);
  tell_player(p, oldstack);
  stack = oldstack;
}


void master_search_command(player * p, char *str)
{
  int i;
  char *query = "";


  if (strcasecmp(str, "sstats") && !(query = strchr(str, ' ')))
  {
    tell_player(p, " Format: search <type> <query>\n");
    show_search_types(p);
    return;
  }
  if (*query)
    *query++ = 0;

  if (strlen(query) > 30)
  {
    tell_player(p, " Query too long, truncated\n");
    query[30] = 0;
  }
  for (i = 0; SearchTypes[i].type[0]; i++)
    if (!strncasecmp(SearchTypes[i].type, str, strlen(str)))
    {
      if (SearchTypes[i].priv && !(p->residency & SearchTypes[i].priv))
	continue;

      master_search(p, query, &SearchTypes[i]);
      return;
    }

  TELLPLAYER(p, "Couldn't find '%s' to search ...\n", str);
  show_search_types(p);
}



char *percentile(int top, int bot)
{
  static char ret[8];
  float perc;

  if (!bot || bot < 0)
    return " ??%";

  perc = ((float) top / (float) bot) * 100;
  if (perc > 100)
    strcpy(ret, "+100");
  else if (perc == 100)
    strcpy(ret, "100%");
  else if (!perc)
    strcpy(ret, "  0%");
  else
    sprintf(ret, "%3.0f%%", perc);

  return ret;
}


char *name_search(player * p, char *str)
{
  static char clc[MAX_NAME + 5];

  if (!strcasestr(p->lower_name, str))
    return (char *) NULL;

  memset(clc, 0, MAX_NAME + 5);

  if (p->residency & (CODER | ADMIN | LOWER_ADMIN))
    sprintf(clc, "^B%s^N, ", p->lower_name);
  else if (p->residency & (PSU | SU | ASU | ADC))
    sprintf(clc, "^G%s^N, ", p->lower_name);
  else
    sprintf(clc, "%s, ", p->lower_name);

  return clc;
}

char *xref_search(player * p, char *str)
{
  static char clc[MAX_NAME + 5];

  if (strncmp(p->lower_name, str, strlen(str)))
    return (char *) NULL;

  memset(clc, 0, MAX_NAME + 5);

  if (p->residency & (CODER | ADMIN | LOWER_ADMIN))
    sprintf(clc, "^B%s^N, ", p->lower_name);
  else if (p->residency & (PSU | SU | ASU | ADC))
    sprintf(clc, "^G%s^N, ", p->lower_name);
  else
    sprintf(clc, "%s, ", p->lower_name);
  return clc;
}

char *email_search(player * p, char *str)
{
  static char clc[160];

  if (!strcasestr(p->email, str))
    return (char *) NULL;

  memset(clc, 0, 160);

  if (p->residency & (CODER | ADMIN | LOWER_ADMIN))
    sprintf(clc, "  ^B%-20s  %s^N\n", p->lower_name, p->email);
  else if (p->residency & (PSU | SU | ASU | ADC))
    sprintf(clc, "  ^G%-20s  %s^N\n", p->lower_name, p->email);
  else
    sprintf(clc, "  %-20s  %s^N\n", p->lower_name, p->email);
  return clc;
}

char *site_search(player * p, char *str)
{
  static char clc[160];

  if (!p->saved || !strcasestr(p->saved->last_host, str))
    return (char *) NULL;

  memset(clc, 0, 160);
  if (current_player && current_player->residency & (LOWER_ADMIN | ADMIN))
    sprintf(clc, " %-20s %s <%s>\n", p->lower_name, p->saved->last_host,
	    p->email);
  else
    sprintf(clc, " %-20s %s\n", p->lower_name, p->saved->last_host);
  return clc;
}


char *url_search(player * p, char *str)
{
  static char clc[160];

  if (!strcasestr(p->alt_email, str))
    return (char *) NULL;

  memset(clc, 0, 160);
  sprintf(clc, " %-20s %s^N\n", p->lower_name, p->alt_email);
  return clc;

}

char *richest_search(player * p, char *str)
{
  static char clc[160];

  if (p->pennies < 10)
    return (char *) NULL;

  if (p->pennies < atoi(str))
    return (char *) NULL;

  memset(clc, 0, 160);
  sprintf(clc, " %-20s %d %s\n", p->lower_name, p->pennies,
	  get_config_msg("cash_name"));
  return clc;

}

char *resby_search(player * p, char *str)
{
  static char clc[MAX_NAME + 5];

  if (!p->ressied_by[0] || !strcasestr(p->ressied_by, str))
    return (char *) NULL;

  memset(clc, 0, MAX_NAME + 5);
  if (p->residency & (CODER | ADMIN | LOWER_ADMIN))
    sprintf(clc, "^B%s^N, ", p->lower_name);
  else if (p->residency & (PSU | SU | ASU | ADC))
    sprintf(clc, "^G%s^N, ", p->lower_name);
  else
    sprintf(clc, "%s, ", p->lower_name);
  return clc;
}

char *staff_stats_search(player * p, char *str)
{
  static char clc[160];
  char *ptr;

  if (!(p->residency & (PSU | ADC)))
    return (char *) NULL;

  memset(clc, 0, 160);
  ptr = clc;
  ptr += sprintf(ptr, " %-20s %4d %4d %4d %4d %4d  %s", p->lower_name,
		 p->num_ressied, p->num_warned, p->num_booted, p->num_rmd,
	       p->num_ejected, percentile(p->time_in_main, p->total_login));
  sprintf(ptr, " %s\n", percentile(p->total_idle_time, p->total_login));

  return clc;
}

char *trouble_search(player * p, char *str)
{
  static char clc[160];
  char *ptr;

  if ((p->warn_count + p->eject_count + p->idled_out_count + p->booted_count)
      <
      atoi(str)
      &&
      !(p->git_string[0]))
    return (char *) NULL;


  memset(clc, 0, 160);
  ptr = clc;
  ptr += sprintf(ptr, " %-20s %4d %4d %4d %4d  %s", p->lower_name,
		 p->warn_count, p->eject_count, p->booted_count,
		 p->idled_out_count, percentile(p->time_in_main,
						p->total_login));
  sprintf(ptr, " %s  %c   %c\n",
	  percentile(p->total_idle_time, p->total_login),
	  (p->git_string[0] ? '*' : ' '),
	  (p->residency & BANISHD ? '*' : ' '));

  return clc;
}



char *git_search(player * p, char *str)
{
  static char clc[160];


  if (!p->git_string[0])
    return (char *) NULL;

  if (!strcasestr(p->git_string, str) && !strcasestr(p->git_by, str))
    return (char *) NULL;

  memset(clc, 0, 160);
  sprintf(clc, " %-20s (%s) %s^N\n", p->lower_name, p->git_by, p->git_string);
  return clc;
}

char *swarn_search(player * p, char *str)
{
  static char clc[160];


  if (!p->swarn_message[0])
    return (char *) NULL;

  if (!strcasestr(p->swarn_message, str) &&
      !strcasestr(p->swarn_sender, str))
    return (char *) NULL;

  memset(clc, 0, 160);
  sprintf(clc, " %-20s (%s) %s^N\n", p->lower_name, p->swarn_sender,
	  p->swarn_message);
  return clc;
}

void use_search(player * p, char *str)
{
  char squeal[160];

  tell_player(p, " This command has depreciated, use the 'search' command.\n");
  if (!*str)
    return;

  memset(squeal, 0, 160);

  if (strlen(str) > 140)	/* make sure we cant overwrite the lil buffer */
    str[140] = 0;

  if (!strcasecmp(first_char(p), "xref"))
  {
    sprintf(squeal, "xref %s", str);
    master_search_command(p, squeal);
    return;
  }
  if (!strcasecmp(first_char(p), "etrace"))
  {
    sprintf(squeal, "emails %s", str);
    master_search_command(p, squeal);
    return;
  }
  sprintf(squeal, "sites %s", str);
  master_search_command(p, squeal);
  return;
}
