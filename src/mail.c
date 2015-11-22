/*
 * Playground+ - mail.c
 * All mail code
 * ---------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/time.h>

#include "include/fix.h"
#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"

#define NEWS_NO_TIMEOUT 2

/* interns */

note *n_hash[NOTE_HASH_SIZE];
int nhash_update[NOTE_HASH_SIZE];
int unique = 1;
  /* lowlevel news stuff left in for backwards compatiblity */
int news_start = 0, news_count = 0;
int snews_start = 0, snews_count = 0;

void unlink_mail(note *);

/* find note in hash */

note *find_note(int id)
{
  note *scan;
  if (!id)
    return 0;
  for (scan = n_hash[id % NOTE_HASH_SIZE]; scan; scan = scan->hash_next)
    if (scan->id == id)
      return scan;
  return 0;
}

/* create a new note */

note *create_note(void)
{
  note *n;
  int num;
  n = (note *) MALLOC(sizeof(note));
  memset(n, 0, sizeof(note));
  n->flags = NOT_READY;
  n->date = time(0);
  n->next_sent = 0;
  n->text.where = 0;
  n->text.length = 0;
  strcpy(n->header, "DEFUNCT");
  strcpy(n->name, "system");
  while (find_note(unique))
  {
    ++unique;
    unique &= 65535;
  }
  n->id = unique++;
  unique &= 65535;
  num = (n->id) % NOTE_HASH_SIZE;
  n->hash_next = n_hash[num];
  n_hash[num] = n;
  nhash_update[num] = 1;
  return n;
}

/* remove a note */

void remove_note(note * n)
{
  note *scan, *prev;
  int num;
  if (n->text.where)
    FREE(n->text.where);
  num = (n->id) % NOTE_HASH_SIZE;
  scan = n_hash[num];
  if (scan == n)
    n_hash[num] = n->hash_next;
  else
  {
    do
    {
      prev = scan;
      scan = scan->hash_next;
    }
    while (scan != n);
    prev->hash_next = n->hash_next;
  }
  FREE(n);
  nhash_update[num] = 1;
}

/* remove various types of note */

void remove_any_note(note * n)
{
  saved_player *sp;
  note *scan = 0;
  char *oldstack;
  int *change = 0;
  oldstack = stack;

  if (n->flags & SUNEWS_ARTICLE)
  {
    scan = find_note(snews_start);
    change = &snews_start;
    while (scan && scan != n)
    {
      change = &(scan->next_sent);
      scan = find_note(scan->next_sent);
    }
    if (scan == n)
      snews_count--;
  }
  else if (n->flags & NEWS_ARTICLE)
  {
    scan = find_note(news_start);
    change = &news_start;
    while (scan && scan != n)
    {
      change = &(scan->next_sent);
      scan = find_note(scan->next_sent);
    }
    if (scan == n)
      news_count--;
  }
  else
  {
    strcpy(stack, n->name);
    lower_case(stack);
    stack = end_string(stack);
    sp = find_saved_player(oldstack);
    if (!sp)
    {
      log("mail", "Bad owner name in mail(1)");
      log("mail", oldstack);
      unlink_mail(find_note(n->next_sent));
      scan = 0;
    }
    else
    {
      change = &(sp->mail_sent);
      scan = find_note(sp->mail_sent);
      while (scan && scan != n)
      {
	change = &(scan->next_sent);
	scan = find_note(scan->next_sent);
      }
    }
  }
  if (scan == n)
    (*change) = n->next_sent;
  remove_note(n);
  stack = oldstack;
}



/* dump one note onto the stack */

int save_note(note * d)
{
  if (d->flags & NOT_READY)
    return 0;

  stack = store_int(stack, d->id);
  stack = store_int(stack, d->flags);
  stack = store_int(stack, d->date);
  stack = store_int(stack, d->read_count);
  stack = store_int(stack, d->next_sent);
  stack = store_string(stack, d->header);
  stack = store_int(stack, d->text.length);
  memcpy(stack, d->text.where, d->text.length);
  stack += d->text.length;
  stack = store_string(stack, d->name);
  return 1;
}


/* sync one hash bucket to disk */

void sync_note_hash(int number)
{
  char *oldstack, name[MAX_NAME + 2];
  note *scan, *check;
  int length, count = 0, fd, t;
  saved_player *sp;

  oldstack = stack;

  if (sys_flags & VERBOSE)
  {
    sprintf(stack, "Syncing note hash %d.", number);
    stack = end_string(stack);
    log("sync", oldstack);
    stack = oldstack;
  }
  t = time(0);

  for (scan = n_hash[number]; scan; scan = check)
  {
    if (scan->flags & (NEWS_ARTICLE | SUNEWS_ARTICLE))
    {
      strcpy(name, scan->name);
      lower_case(name);
      sp = find_saved_player(name);
      if ((t - (scan->date)) > NEWS_TIMEOUT && !(scan->flags & NEWS_NO_TIMEOUT))
      {
	if (!(sp && sp->residency & ADMIN))
	{
	  check = scan->hash_next;
	  remove_any_note(scan);
	  /* scan=n_hash[number];  */
	}
      }
    }
    else if ((t - (scan->date)) > MAIL_TIMEOUT && !(scan->flags & NEWS_NO_TIMEOUT))
    {
      check = scan->hash_next;
      remove_any_note(scan);
      /* scan=n_hash[number];  */
    }
    check = scan->hash_next;
  }

  stack = store_int(stack, 0);

  for (scan = n_hash[number]; scan; scan = scan->hash_next)
    count += save_note(scan);

  store_int(oldstack, count);
  length = (int) stack - (int) oldstack;

  sprintf(stack, "files/notes/hash%d", number);
#ifdef BSDISH
  fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
  fd = open(stack, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif

  if (fd < 0)
    handle_error("Failed to open note file.");
  if (write(fd, oldstack, length) < 0)
    handle_error("Failed to write note file.");
  close(fd);

  nhash_update[number] = 0;
  stack = oldstack;
}

/* throw all the notes to disk */

void sync_notes(int background)
{
  int n, fd;
  char *oldstack;
  oldstack = stack;

  if (background && fork())
    return;

  if (sys_flags & VERBOSE || sys_flags & PANIC)
    log("sync", "Dumping notes to disk");
  for (n = 0; n < NOTE_HASH_SIZE; n++)
    sync_note_hash(n);
  if (sys_flags & VERBOSE || sys_flags & PANIC)
    log("sync", "Note dump completed");

#ifdef BSDISH
  fd = open("files/notes/track", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
  fd = open("files/notes/track", O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif

  if (fd < 0)
    handle_error("Failed to open track file.");
  stack = store_int(stack, unique);
  stack = store_int(stack, news_start);
  stack = store_int(stack, news_count);
  stack = store_int(stack, snews_start);
  stack = store_int(stack, snews_count);
  if (write(fd, oldstack, 20) < 0)
    handle_error("Failed to write track file.");
  close(fd);

  stack = oldstack;

  if (background)
    exit(0);
}

/* su command to dump all or single notes to disk */

void dump_com(player * p, char *str)
{
  int num;
  char *oldstack;
  oldstack = stack;

  if (!isdigit(*str))
  {
    if (p != NULL)
      tell_player(p, " Dumping all notes to disk.\n");
    sync_notes(1);
    return;
  }
  num = atoi(str);

  if (num < 0 || num >= NOTE_HASH_SIZE)
  {
    if (p != NULL)
      tell_player(p, " Argument not in hash range !\n");
    return;
  }
  sprintf(stack, " Dumping hash %d to disk.\n", num);
  stack = end_string(stack);
  if (p != NULL)
    tell_player(p, oldstack);
  sync_note_hash(num);
  stack = oldstack;
}


/* throw away a hash of notes */

void discard_hash(int num)
{
  note *scan, *next;
  for (scan = n_hash[num]; scan; scan = next)
  {
    next = scan->hash_next;
    if (scan->text.where)
      FREE(scan->text.where);
    FREE(scan);
  }
}


/* extracts one note into the hash list */

char *extract_note(char *where)
{
  int num;
  note *d;
  d = (note *) MALLOC(sizeof(note));
  memset(d, 0, sizeof(note));
  where = get_int(&d->id, where);
  where = get_int(&d->flags, where);
  where = get_int(&d->date, where);
  where = get_int(&d->read_count, where);
  where = get_int(&d->next_sent, where);
  where = get_string(d->header, where);
  where = get_int(&d->text.length, where);
  d->text.where = (char *) MALLOC(d->text.length);
  memcpy(d->text.where, where, d->text.length);
  where += d->text.length;
  where = get_string(d->name, where);
  num = (d->id) % NOTE_HASH_SIZE;
  if (n_hash[num])
    d->hash_next = n_hash[num];
  else
    d->hash_next = 0;
  n_hash[num] = d;
  return where;
}

/*
 * load all notes from disk this should be changed for arbitary hashes
 */

void init_notes()
{
  int n, length, fd, count;
  char *oldstack, *where, *scan;
  oldstack = stack;

  if (sys_flags & VERBOSE || sys_flags & PANIC)
    log("boot", "Loading notes from disk");

#ifdef PC
  fd = open("files\\notes\\track", O_RDONLY | O_BINARY);
#else
  fd = open("files/notes/track", O_RDONLY | O_NDELAY);
#endif
  if (fd < 0)
  {
    sprintf(oldstack, "Failed to load track file");
    stack = end_string(oldstack);
    log("error", oldstack);
    unique = 1;
    news_start = 0;
    snews_start = 0;
    news_count = 0;
    snews_count = 0;
    stack = oldstack;
  }
  else
  {
    if (read(fd, oldstack, 20) < 0)
      handle_error("Can't read track file.");
    stack = get_int(&unique, stack);
    stack = get_int(&news_start, stack);
    stack = get_int(&news_count, stack);
    stack = get_int(&snews_start, stack);
    stack = get_int(&snews_count, stack);
    close(fd);
    stack = oldstack;
  }


  for (n = 0; n < NOTE_HASH_SIZE; n++)
  {

    discard_hash(n);
    nhash_update[n] = 0;

#ifdef PC
    sprintf(oldstack, "files\\notes\\hash%d", n);
    fd = open(oldstack, O_RDONLY | O_BINARY);
#else
    sprintf(oldstack, "files/notes/hash%d", n);
    fd = open(oldstack, O_RDONLY | O_NDELAY);
#endif
    if (fd < 0)
    {
      sprintf(oldstack, "Failed to load note hash%d", n);
      stack = end_string(oldstack);
      log("error", oldstack);
      stack = oldstack;
    }
    else
    {
      length = lseek(fd, 0, SEEK_END);
      lseek(fd, 0, SEEK_SET);
      if (length)
      {
	where = (char *) MALLOC(length);
	if (read(fd, where, length) < 0)
	  handle_error("Can't read note file.");
	scan = get_int(&count, where);
	for (; count; count--)
	  scan = extract_note(scan);
	FREE(where);
      }
      close(fd);
    }
  }
  stack = oldstack;
}



/* store info for a player save */

void construct_mail_save(saved_player * sp)
{
  int count = 0, *scan;
  char *oldstack;
  stack = store_int(stack, sp->mail_sent);
  if (!(sp->mail_received))
    stack = store_int(stack, 0);
  else
  {
    oldstack = stack;
    stack = store_int(oldstack, 0);
    for (scan = sp->mail_received; *scan; scan++, count++)
      stack = store_int(stack, *scan);
    store_int(oldstack, count);
  }
}


/* get info back from a player save */

char *retrieve_mail_data(saved_player * sp, char *where)
{
  int count = 0, *fill;
  where = get_int(&sp->mail_sent, where);
  where = get_int(&count, where);
  if (count)
  {
    fill = (int *) MALLOC((count + 1) * sizeof(int));
    sp->mail_received = fill;
    for (; count; count--, fill++)
      where = get_int(fill, where);
    *fill++ = 0;
  }
  else
    sp->mail_received = 0;
  return where;
}


/* su command to check out note hashes */

void list_notes(player * p, char *str)
{
  int num;
  note *scan;
  char *oldstack;

  oldstack = stack;

  num = atoi(str);
  if (num < 0 || num >= NOTE_HASH_SIZE)
  {
    tell_player(p, " Number not in range.\n");
    return;
  }
  strcpy(stack, " Notes:\n");
  stack = strchr(stack, 0);
  for (scan = n_hash[num]; scan; scan = scan->hash_next)
  {
    if (scan->flags & SUNEWS_ARTICLE)
      sprintf(stack, "  [%d] %s = %s", scan->id,
	      scan->name, scan->header);
    else if (scan->flags & NEWS_ARTICLE)
      sprintf(stack, "  [%d] %s - %s", scan->id,
	      scan->name, scan->header);
    else
      sprintf(stack, "  [%d] %s * %s", scan->id, scan->name,
	      bit_string(scan->flags));
    stack = strchr(stack, 0);
    if (scan->flags & NOT_READY)
    {
      strcpy(stack, "(DEFUNCT)\n");
      stack = strchr(stack, 0);
    }
    else
      *stack++ = '\n';
  }
  strcpy(stack, " --\n");
  stack = strchr(stack, 0);
  stack++;
  tell_player(p, oldstack);
  stack = oldstack;
}


void list_all_notes(player * p, char *str)
{
  char *oldstack;
  int count = 0, hash;
  note *scan;

  oldstack = stack;

  strcpy(stack, " All notes --\n");
  stack = strchr(stack, 0);
  for (hash = 0; hash <= NOTE_HASH_SIZE; count = 0, hash++)
  {
    for (scan = n_hash[hash]; scan; scan = scan->hash_next)
      count++;
    sprintf(stack, "  %3d notes in bucket %2d", count, hash);
    stack = strchr(stack, 0);
    if (hash & 1)
      strcpy(stack, "   --   ");
    else
      strcpy(stack, "\n");
    stack = strchr(stack, 0);
  }
  strcpy(stack, " --\n");
  stack = strchr(stack, 0);
  stack++;
  tell_player(p, oldstack);
  stack = oldstack;
}


/* the mail command */

void mail_command(player * p, char *str)
{
  if (p->edit_info)
  {
    tell_player(p, " Can't do mail commands whilst in editor.\n");
    return;
  }
  if (!p->saved)
  {
    tell_player(p, " You have no save information, and so can't use "
		"mail.\n");
    return;
  }
  if ((*str == '/') && (p->input_to_fn == mail_command))
  {
    match_commands(p, str + 1);
    if (!(p->flags & PANIC) && (p->input_to_fn == mail_command))
    {
      do_prompt(p, "Mail Mode >");
      p->mode |= MAILEDIT;
    }
    return;
  }
  if (!*str)
    if (p->input_to_fn == mail_command)
    {
      tell_player(p, " Format: mail <action>\n");
      if (!(p->flags & PANIC) && (p->input_to_fn == mail_command))
      {
	do_prompt(p, "Mail Mode >");
	p->mode |= MAILEDIT;
      }
      return;
    }
    else
    {
      tell_player(p, " Entering mail mode. Use 'end' to leave.\n"
		  " '/<command>' does normal commands.\n");
      p->flags &= ~PROMPT;
      p->input_to_fn = mail_command;
    }
  else
    sub_command(p, str, mail_list);
  if (!(p->flags & PANIC) && (p->input_to_fn == mail_command))
  {
    do_prompt(p, "Mail Mode >");
    p->mode |= MAILEDIT;
  }
}


/* view mail commands */

void view_mail_commands(player * p, char *str)
{
  view_sub_commands(p, mail_list);
}

/* exit mail mode */

void exit_mail_mode(player * p, char *str)
{
  if (p->input_to_fn != mail_command)
    return;
  tell_player(p, " Leaving mail mode.\n");
  p->input_to_fn = 0;
  p->flags |= PROMPT;
  p->mode &= ~MAILEDIT;
}



/* mail stuff */



/* count how many mails have been posted */

int posted_count(player * p)
{
  int scan, count = 0;
  note *mail, *next;

  if (!p->saved)
    return 0;
  scan = p->saved->mail_sent;
  mail = find_note(scan);
  if (!mail)
  {
    p->saved->mail_sent = 0;
    return 0;
  }
  while (mail)
  {
    count++;
    scan = mail->next_sent;
    next = find_note(scan);
    if (!next && scan)
    {
      mail->next_sent = 0;
      mail = 0;
    }
    else
      mail = next;
  }
  return count;
}



/* view mail that has been sent */

void view_sent(player * p, char *str)
{
  char *oldstack;
  note *mail, *next;
  int count = 1, scan;
  saved_player *sp;
  char temp[70];

  oldstack = stack;
  if (*str && p->residency & ADMIN)
  {
    sp = find_saved_player(str);
    if (!sp)
    {
      tell_player(p, " No such player in save files.\n");
      return;
    }
    else
    {
      scan = sp->mail_sent;
      mail = find_note(scan);
      if (!mail)
      {
	sp->mail_sent = 0;
	tell_player(p, " They have sent no mail.\n");
	return;
      }
    }
  }
  else
  {
    if (!p->saved)
    {
      tell_player(p, " You have no save file, and therefore no mail "
		  "either.\n");
      return;
    }
    scan = p->saved->mail_sent;
    mail = find_note(scan);
    if (!mail)
    {
      p->saved->mail_sent = 0;
      tell_player(p, " You have sent no mail.\n");
      return;
    }
  }

  pstack_mid("Sent mail");
  while (mail)
  {
    if (p->residency & ADMIN)
      sprintf(stack, "(%d) [%d] %s^N\n", mail->id, count, mail->header);
    else
      sprintf(stack, "[%d] %s^N\n", count, mail->header);

    stack = strchr(stack, 0);
    scan = mail->next_sent;
    count++;
    next = find_note(scan);
    if (!next && scan)
    {
      mail->next_sent = 0;
      mail = 0;
    }
    else
    {
      mail = next;
    }
  }
  sprintf(temp, "You have sent %d out of your maximum of %d mails", count - 1, p->max_mail);
  pstack_mid(temp);
  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}


/* this fn goes through the received list and removes dud note pointers */

void reconfigure_received_list(saved_player * sp)
{
  int count = 0;
  int *scan, *fill;
  char *oldstack;
  oldstack = stack;

  align(stack);
  fill = (int *) stack;
  scan = sp->mail_received;
  if (!scan)
    return;
  for (; *scan; scan++)
    if (((*scan) < 0) || ((*scan) > 65536))
    {
      count = 0;
      break;
    }
    else if (find_note(*scan))
    {
      *(int *) stack = *scan;
      stack += sizeof(int);
      count++;
    }
  FREE(sp->mail_received);
  if (count)
  {
    *(int *) stack = 0;
    stack += sizeof(int);
    count++;
    sp->mail_received = (int *) MALLOC(count * sizeof(int));
    memcpy(sp->mail_received, oldstack, count * sizeof(int));
  }
  else
    sp->mail_received = 0;
  stack = oldstack;
}


/* view mail that has been received */

void view_received(player * p, char *str)
{
  char *oldstack, middle[80], *page_input;
  int page, pages, *scan, *scan_count, mcount = 0, ncount = 1, n;
  note *mail;
  saved_player *sp;
  oldstack = stack;

  if (*str && !isdigit(*str) && p->residency & ADMIN)
  {
    page_input = next_space(str);
    if (*page_input)
      *page_input++ = 0;
    sp = find_saved_player(str);
    if (!sp)
    {
      tell_player(p, " No such player in save files.\n");
      return;
    }
    else
      scan = sp->mail_received;
    str = page_input;
  }
  else
  {
    if (!p->saved)
    {
      tell_player(p, " You have no save file, "
		  "and therefore no mail either.\n");
      return;
    }
    sp = p->saved;
    scan = sp->mail_received;
  }

  if (!scan)
  {
    tell_player(p, " You have received no mail.\n");
    return;
  }
  p->system_flags &= ~NEW_MAIL;

  for (scan_count = scan; *scan_count; scan_count++)
    mcount++;

  page = atoi(str);
  if (page <= 0)
    page = 1;
  page--;

  pages = (mcount - 1) / (TERM_LINES - 2);
  if (page > pages)
    page = pages;

  if (mcount == 1)
    strcpy(middle, "You have received one letter");
  else
    sprintf(middle, "You have received %s letters",
	    number2string(mcount));
  pstack_mid(middle);

  ncount = page * (TERM_LINES - 2);

  scan += ncount;

  for (n = 0, ncount++; n < (TERM_LINES - 1); n++, ncount++, scan++)
  {
    if (!*scan)
      break;
    mail = find_note(*scan);
    if (!mail)
    {
      stack = oldstack;
      tell_player(p, " Found mail that owner had deleted ...\n");
      reconfigure_received_list(sp);
      if (sp != p->saved)
      {
	tell_player(p, " Reconfigured ... try again\n");
	return;
      }
      view_received(p, str);
      return;
    }
    if (p->residency & ADMIN)
      sprintf(stack, "(%d) [%d] ", mail->id, ncount);
    else
      sprintf(stack, "[%d] ", ncount);
    while (*stack)
      stack++;
    if (ncount < 10)
      *stack++ = ' ';
    strcpy(stack, mail->header);
    while (*stack)
      stack++;
    if (mail->flags & ANONYMOUS)
    {
      if (p->residency & ADMIN)
	sprintf(stack, "^N <<<%s>>>\n", mail->name);
      else
	strcpy(stack, "^N\n");
    }
    else
      sprintf(stack, "^N (%s)\n", mail->name);
    while (*stack)
      stack++;
  }

  sprintf(middle, "Page %d of %d", page + 1, pages + 1);
  pstack_mid(middle);

  *stack++ = 0;
  tell_player(p, oldstack);

  stack = oldstack;
}


/* send a letter */


void quit_mail(player * p)
{
  tell_player(p, " Letter NOT posted.\n");
  remove_note((note *) p->edit_info->misc);
}

void end_mail(player * p)
{
  note *mail;
  char *oldstack, *name_list, *body, *tcpy, *comp, *text;
  saved_player **player_list, **pscan, **pfill;
  player *on;
  int receipt_count, n, m, *received_lists, *r;
  list_ent *l;
  oldstack = stack;

  mail = (note *) p->edit_info->misc;

  align(stack);
  player_list = (saved_player **) stack;
  receipt_count = saved_tag(p, mail->text.where);
  if (mail->text.where)
    FREE(mail->text.where);
  mail->text.where = 0;
  if (!receipt_count)
  {
    tell_player(p, " No one to send the letter to !\n");
    remove_note(mail);
    stack = oldstack;
    if (p->edit_info->input_copy == mail_command)
    {
      do_prompt(p, "Mail Mode >");
      p->mode |= MAILEDIT;
    }
    return;
  }
  if (!p->saved)
  {
    tell_player(p, " Eeek, no save file !\n");
    remove_note(mail);
    stack = oldstack;
    if (p->edit_info->input_copy == mail_command)
    {
      do_prompt(p, "Mail Mode >");
      p->mode |= MAILEDIT;
    }
    return;
  }
  pscan = player_list;
  pfill = player_list;
  m = receipt_count;
  for (n = 0; n < m; n++)
  {
    if (((*pscan)->residency == SYSTEM_ROOM)
	|| ((*pscan)->residency & BANISHD))
    {
      tell_player(p, " You can't send mail to a room, "
		  "or someone who is banished.\n");
      receipt_count--;
      pscan++;
    }
    else
    {
      l = fle_from_save((*pscan), p->lower_name);
      if (!l)
	l = fle_from_save((*pscan), "everyone");
      if (l && l->flags & MAILBLOCK)
      {
	text = stack;
	sprintf(stack, " %s is blocking all your mail.\n",
		(*pscan)->lower_name);
	stack = end_string(stack);
	tell_player(p, text);
	stack = text;
	receipt_count--;
	pscan++;
      }
      else if (mail->flags & NOTE_FRIEND_TAG && (*pscan)->tag_flags & BLOCK_FRIEND_MAIL)
      {
	text = stack;
	sprintf(stack, " %s is blocking friendmail.\n",
		(*pscan)->lower_name);
	stack = end_string(stack);
	tell_player(p, text);
	stack = text;
	receipt_count--;
	pscan++;
      }
      else if ((mail->flags & ANONYMOUS)
	       && ((*pscan)->tag_flags & NO_ANONYMOUS))
      {
	text = stack;
	sprintf(stack, " %s is not receiving anonymous mail.\n",
		(*pscan)->lower_name);
	stack = end_string(stack);
	tell_player(p, text);
	stack = text;
	receipt_count--;
	pscan++;
      }
      else
	*pfill++ = *pscan++;
    }
  }

  if (receipt_count > 0)
  {
    pscan = player_list;
    name_list = stack;
    if (mail->flags & SUPRESS_NAME)
    {
      strcpy(stack, "Anonymous");
      stack = end_string(stack);
    }
    else
    {
      strcpy(stack, (*pscan)->lower_name);
      stack = strchr(stack, 0);
      if (receipt_count > 1)
      {
	pscan++;
	for (n = 2; n < receipt_count; n++, pscan++)
	{
	  sprintf(stack, ", %s", (*pscan)->lower_name);
	  stack = strchr(stack, 0);
	}
	sprintf(stack, " and %s", (*pscan)->lower_name);
	stack = strchr(stack, 0);
      }
      *stack++ = 0;
    }

    body = stack;
    if (mail->flags & NOTE_FRIEND_TAG)
      sprintf(stack, " Sending mail to your friends.\n");
    else
      sprintf(stack, " Sending mail to %s.\n", name_list);
    stack = end_string(stack);
    tell_player(p, body);
    stack = body;

    if (mail->flags & NOTE_FRIEND_TAG)
    {
      if (mail->flags & ANONYMOUS)
	sprintf(stack, "\nDate: %s"
		"\nFrom: <anonymous>"
		"\nTo: <anonymous>'s friends"
	     "\nSubject: %s^N\n\n", convert_time(mail->date), mail->header);
      else
	sprintf(stack, "\nDate: %s"
		"\nFrom: %s"
		"\nTo: %s's friends"
		"\nSubject: %s^N\n\n", convert_time(mail->date), mail->name, mail->name, mail->header);
    }
    else
    {
      if (mail->flags & ANONYMOUS)
	sprintf(stack, "\nDate: %s"
		"\nFrom: <anonymous>"
		"\nTo: %s"
		"\nSubject: %s^N\n\n", convert_time(mail->date), name_list, mail->header);
      else
	sprintf(stack, "\nDate: %s"
		"\nFrom: %s"
		"\nTo: %s"
		"\nSubject: %s^N\n\n", convert_time(mail->date), mail->name, name_list, mail->header);
    }
    stack = strchr(stack, 0);

    tcpy = p->edit_info->buffer;
    for (n = 0; n < p->edit_info->size; n++)
      *stack++ = *tcpy++;

    comp = stack;
    stack = store_string(stack, body);
    mail->text.length = (int) stack - (int) comp;
    mail->text.where = (char *) MALLOC(mail->text.length);
    memcpy(mail->text.where, comp, mail->text.length);

    mail->next_sent = p->saved->mail_sent;
    p->saved->mail_sent = mail->id;

    text = stack;
    command_type |= HIGHLIGHT;

    if (mail->flags & ANONYMOUS)
      sprintf(stack, " -=*> New mail, '%s^N' sent anonymously\n\n",
	      mail->header);
    else
      sprintf(stack, " -=*> New mail, '%s^N' from %s.\n\n",
	      mail->header, mail->name);
    stack = end_string(stack);

    align(stack);
    received_lists = (int *) stack;

    pscan = player_list;
    for (n = 0; n < receipt_count; n++, pscan++)
    {
      stack = (char *) received_lists;
      r = (*pscan)->mail_received;
      if (!r)
	m = 2 * sizeof(int);
      else
      {
	for (m = 2 * sizeof(int); *r; r++, m += sizeof(int))
	{
	  *(int *) stack = *r;
	  stack += sizeof(int);
	}
      }
      *(int *) stack = mail->id;
      stack += sizeof(int);
      *(int *) stack = 0;
      if ((*pscan)->mail_received)
	FREE((*pscan)->mail_received);
      r = (int *) MALLOC(m);
      memcpy(r, received_lists, m);
      (*pscan)->mail_received = r;
      on = find_player_absolute_quiet((*pscan)->lower_name);
      if (on && on->custom_flags & MAIL_INFORM)
      {
	tell_player(on, "\007\n");
	tell_player(on, text);
      }
      else
	(*pscan)->system_flags |= NEW_MAIL;
    }

    command_type &= ~HIGHLIGHT;

    mail->read_count = receipt_count;
    mail->flags &= ~NOT_READY;
    tell_player(p, " Mail posted....\n");
  }
  else
    tell_player(p, " No mail posted.\n");
  p->mode &= ~MAILEDIT;
  if (p->edit_info->input_copy == mail_command)
  {
    do_prompt(p, "Mail Mode >");
    p->mode |= MAILEDIT;
  }
  stack = oldstack;
}


void send_letter(player * p, char *str)
{
  note *mail;
  char *subject, *scan, *first, *oldstack = stack;
  int length, fr = 0;
  list_ent *l;

  if (posted_count(p) >= p->max_mail)
  {
    tell_player(p, " Sorry, you have reached your mail limit.\n");
    return;
  }
  subject = next_space(str);

  if (!*subject)
  {
    tell_player(p, " Format: post <character(s)> <subject>\n");
    return;
  }
  *subject++ = 0;

  mail = create_note();

  first = first_char(p);
  scan = strstr(first, "post");
  if (scan && (scan != first_char(p)) && (*(scan - 1) == 'a'))
    mail->flags |= ANONYMOUS;
  if (scan && (scan != first_char(p)) && (*(scan - 1) == 'x') && p->residency & ADMIN)
    mail->flags |= NEWS_NO_TIMEOUT;

  strcpy(mail->name, p->name);
  strncpy(mail->header, subject, MAX_TITLE - 3);
  /* check for friendpost here... */
  if (!strcasecmp(str, "friends"))
  {
    mail->flags |= NOTE_FRIEND_TAG;
    if (!p->saved)
    {
      tell_player(p, " But you have no list? Oooer..\n");
      return;
    }
    l = p->saved->list_top;
    if (!l)
    {
      tell_player(p, " But you have no list.\n");
      return;
    }
    oldstack = stack;
    sprintf(stack, "%s", p->name);	/* dummy head, and keep personal copy */
    stack = strchr(stack, 0);
    do
    {
      if (l->flags & FRIEND)
      {
	fr = 1;
	sprintf(stack, ",%s", l->name);
	stack = strchr(stack, 0);
      }
      l = l->next;
    }
    while (l);
    stack = end_string(stack);
    if (!fr)
    {
      tell_player(p, " You have no friends...\n");
      return;
    }
  }
  /* end friend check */
  tell_player(p, " Enter main text of the letter...\n");
  *stack = 0;
  start_edit(p, MAX_ARTICLE_SIZE, end_mail, quit_mail, stack);
  if (p->edit_info)
  {
    p->edit_info->misc = (void *) mail;
    if (mail->flags & NOTE_FRIEND_TAG)
    {
      length = strlen(oldstack) + 1;
      mail->text.where = (char *) MALLOC(length);
      memcpy(mail->text.where, oldstack, length);
      stack = oldstack;
    }
    else
    {
      length = strlen(str) + 1;
      mail->text.where = (char *) MALLOC(length);
      memcpy(mail->text.where, str, length);
    }
  }
  else
    FREE(mail);
}


/* find the note corresponing to letter number */

note *find_received(saved_player * sp, int n)
{
  int *scan, count = 1;
  note *mail;
  scan = sp->mail_received;
  if (!scan)
    return 0;
  for (; *scan; count++, scan++)
    if (count == n)
    {
      mail = find_note(*scan);
      if (!mail)
      {
	reconfigure_received_list(sp);
	return find_received(sp, n);
      }
      return mail;
    }
  return 0;
}

/* view a letter */

void read_letter(player * p, char *str)
{
  char *oldstack, *which;
  saved_player *sp;
  note *mail;
  oldstack = stack;

  which = str;
  sp = p->saved;
  if (!sp)
  {
    tell_player(p, " You have no save info.\n");
    return;
  }
  mail = find_received(sp, atoi(which));
  if (!mail)
  {
    sprintf(stack, " No such letter '%s'\n", which);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  get_string(stack, mail->text.where);
  stack = end_string(stack);
  pager(p, oldstack);
  p->system_flags &= ~NEW_MAIL;
  stack = oldstack;
}


/* reply to a letter */

void reply_letter(player * p, char *str)
{
  note *mail, *old;
  char *indent, *body, *oldstack, *scan, *first;
  int length;

  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: reply <number>\n");
    return;
  }
  if (posted_count(p) >= p->max_mail)
  {
    tell_player(p, " Sorry, you have reached your mail limit.\n");
    return;
  }
  if (!p->saved)
  {
    tell_player(p, " Eeek, no saved bits.\n");
    return;
  }
  old = find_received(p->saved, atoi(str));
  if (!old)
  {
    sprintf(stack, " Can't find letter '%s'\n", str);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  mail = create_note();

  first = first_char(p);
  scan = strstr(first, "reply");
  if (scan && (scan != first_char(p)) && (*(scan - 1) == 'a'))
    mail->flags |= ANONYMOUS;
  if (scan && (scan != first_char(p)) && (*(scan - 1) == 'x') && p->residency & ADMIN)
    mail->flags |= NEWS_NO_TIMEOUT;

  if (old->flags & ANONYMOUS)
    mail->flags |= SUPRESS_NAME;
  strcpy(mail->name, p->name);

  if (strstr(old->header, "Re: ") == old->header)
    strcpy(mail->header, old->header);
  else
  {
    sprintf(stack, "Re: %s", old->header);
    strncpy(mail->header, stack, MAX_TITLE - 3);
  }

  indent = stack;
  get_string(stack, old->text.where);
  stack = end_string(stack);
  body = stack;

  sprintf(stack, "In reply to '%s'\n", old->header);
  stack = strchr(stack, 0);

  while (*indent)
  {
    *stack++ = '>';
    *stack++ = ' ';
    while (*indent && *indent != '\n')
      *stack++ = *indent++;
    *stack++ = '\n';
    indent++;
  }
  *stack++ = '\n';
  *stack++ = 0;

  tell_player(p, " Please trim letter as much as possible...\n");
  start_edit(p, MAX_ARTICLE_SIZE, end_mail, quit_mail, body);
  if (p->edit_info)
  {
    p->edit_info->misc = (void *) mail;
    length = strlen(old->name) + 1;
    mail->text.where = (char *) MALLOC(length);
    memcpy(mail->text.where, old->name, length);
  }
  else
    FREE(mail);
  stack = oldstack;
}




/* unlink and remove a mail note */

void unlink_mail(note * m)
{
  char *oldstack, *temp;
  saved_player *sp;
  int *change;
  note *scan, *tmp;
  if (!m)
    return;
  oldstack = stack;
  strcpy(stack, m->name);
  lower_case(stack);
  stack = end_string(stack);
  sp = find_saved_player(oldstack);
  if (!sp)
  {
    temp = stack;
    if (current_player)
    {
      sprintf(stack, "(2) mail: %s - current: %s", m->name,
	      current_player->name);
    }
    else
    {
      sprintf(stack, "(2) mail, %s - no current", m->name);
    }
    stack = end_string(stack);
    log("mail", temp);
    stack = temp;
    unlink_mail(find_note(m->next_sent));
  }
  else
  {
    change = &(sp->mail_sent);
    scan = find_note(sp->mail_sent);
    while (scan && scan != m)
    {
      change = &(scan->next_sent);
      scan = find_note(scan->next_sent);
    }
    tmp = find_note(m->next_sent);
    if (tmp)
      *change = tmp->id;
    else
      *change = 0;
  }
  remove_note(m);
  stack = oldstack;
}


/* remove sent mail */

void delete_sent(player * p, char *str)
{
  int number;
  note *scan;
  number = atoi(str);
  if (number <= 0)
  {
    tell_player(p, " Format: remove <mail number>\n");
    return;
  }
  if (!(p->saved))
  {
    tell_player(p, " You have no save information, "
		"and therefore no mail either.\n");
    return;
  }
  scan = find_note(p->saved->mail_sent);
  for (number--; number; number--)
    if (scan)
      scan = find_note(scan->next_sent);
    else
      break;
  if (!scan)
  {
    if (p->residency)
      tell_player(p, " You haven't sent that many mails.\n");
    return;
  }
  unlink_mail(scan);
  if (p->residency)		/* this gets called in nuke -- blimey */
    tell_player(p, " Removed mail ...\n");
}


/* remove received mail */

void delete_received(player * p, char *str)
{
  int number, count = 0;
  int *make, *scan;
  char *oldstack;
  note *deleted;
  number = atoi(str);
  if (number <= 0)
  {
    tell_player(p, " Format: delete <mail number>\n");
    return;
  }
  if (!(p->saved))
  {
    tell_player(p, " You have no save information, "
		"and therefore no mail either.\n");
    return;
  }
  scan = p->saved->mail_received;
  if (!scan)
  {
    tell_player(p, " You have recieved no mail to delete.\n");
    return;
  }
  oldstack = stack;
  align(stack);
  make = (int *) stack;
  for (number--; number; number--)
    if (*scan)
    {
      *make++ = *scan++;
      count++;
    }
    else
      break;
  if (!*scan)
  {
    tell_player(p, " Not that many pieces of mail.\n");
    stack = oldstack;
    return;
  }
  deleted = find_note(*scan++);
  while (*scan)
  {
    *make++ = *scan++;
    count++;
  }
  *make++ = 0;
  count++;
  if (p->saved->mail_received)
    FREE(p->saved->mail_received);
  if (count != 1)
  {
    p->saved->mail_received = (int *) MALLOC(sizeof(int) * count);
    memcpy(p->saved->mail_received, stack, sizeof(int) * count);
  }
  else
    p->saved->mail_received = 0;
  if (deleted)
  {
    deleted->read_count--;
    if (!(deleted->read_count))
      unlink_mail(deleted);
  }
  if (p->residency)		/* this gets called in nuke -- blimey */
    tell_player(p, " Mail deleted....\n");
  stack = oldstack;
}


/* admin ability to view notes */

void view_note(player * p, char *str)
{
  note *n;
  char *oldstack;
  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: view_note <number>\n");
    return;
  }
  n = find_note(atoi(str));
  if (!n)
  {
    tell_player(p, " Can't find note with that number.\n");
    return;
  }
  strcpy(stack, " Mail (?).\n");
  stack = strchr(stack, 0);
  sprintf(stack, " Posted on %s, by %s.\n Read count: %d\n",
	  convert_time(n->date), n->name, n->read_count);
  stack = strchr(stack, 0);
  if (n->flags & ANONYMOUS)
  {
    strcpy(stack, " Posted anonymously.\n");
    stack = strchr(stack, 0);
  }
  if (n->flags & NOT_READY)
  {
    strcpy(stack, " Not ready flag set.\n");
    stack = strchr(stack, 0);
  }
  if (n->flags & SUPRESS_NAME)
  {
    strcpy(stack, " Name suppressed.\n");
    stack = strchr(stack, 0);
  }
  sprintf(stack, " Next link -> %d\n", n->next_sent);
  stack = end_string(stack);
  pager(p, oldstack);
  stack = oldstack;
}



/* remove a note from */

void dest_note(player * p, char *str)
{
  note *n;

  if (!*str)
  {
    tell_player(p, " Format: dest_note <number>\n");
    return;
  }
  n = find_note(atoi(str));
  if (!n)
  {
    tell_player(p, " Can't find note with that number.\n");
    return;
  }
  remove_any_note(n);
  tell_player(p, " Note removed\n");
}



/* relink emergancy command */

void relink_note(player * p, char *str)
{
  char *to;
  int id1, id2;
  note *n;
  to = next_space(str);
  *to++ = 0;
  id1 = atoi(str);
  id2 = atoi(to);
  /*
   * if (!id1 || !id2) { tell_player(p,"Format : relink <number>
   * <number>\n"); return; }
   */
  n = find_note(id1);
  if (!n)
  {
    tell_player(p, " Can't find first note\n");
    return;
  }
  n->next_sent = id2;
  tell_player(p, " next_sent pointer twiddled.\n");
  return;
}



/* forward a letter */

void forward_letter(player * p, char *str)
{
  note *mail, *old;
  char *recip, *indent, *body, *oldstack, *scan, *first;
  int length, fr = 0;
  list_ent *l;

  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: forward <number> <player(s)>\n");
    return;
  }
  recip = next_space(str);
  if (*recip)
    *recip++ = 0;
  else
  {
    tell_player(p, " Format: forward <number> <player(s)>\n");
    return;
  }
  if (posted_count(p) >= p->max_mail)
  {
    tell_player(p, " Sorry, you have reached your mail limit.\n");
    return;
  }
  if (!p->saved)
  {
    tell_player(p, " Eeek, no saved bits.\n");
    return;
  }
  old = find_received(p->saved, atoi(str));
  if (!old)
  {
    sprintf(stack, " Can't find letter '%s'\n", str);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  mail = create_note();

  first = first_char(p);
  scan = strstr(first, "forward");
  if (scan && (scan != first_char(p)) && (*(scan - 1) == 'a'))
    mail->flags |= ANONYMOUS;
  if (scan && (scan != first_char(p)) && (*(scan - 1) == 'x') && p->residency & ADMIN)
    mail->flags |= NEWS_NO_TIMEOUT;

  if (old->flags & ANONYMOUS)
    mail->flags |= SUPRESS_NAME;
  strcpy(mail->name, p->name);

  if (strstr(old->header, "Fwd: ") == old->header)
    strcpy(mail->header, old->header);
  else
  {
    sprintf(stack, "Fwd: %s", old->header);
    strncpy(mail->header, stack, MAX_TITLE - 3);
  }

  indent = stack;
  get_string(stack, old->text.where);
  stack = end_string(stack);
  body = stack;

  sprintf(stack, "In reply to '%s'\n", old->header);
  stack = strchr(stack, 0);

  while (*indent)
  {
    *stack++ = '>';
    *stack++ = ' ';
    while (*indent && *indent != '\n')
      *stack++ = *indent++;
    *stack++ = '\n';
    indent++;
  }
  *stack++ = '\n';
  *stack++ = 0;

  tell_player(p, " Please trim letter as much as possible...\n");
  start_edit(p, MAX_ARTICLE_SIZE, end_mail, quit_mail, body);
  /* check for friendpost here... */
  if (!strcasecmp(recip, "friends"))
  {
    mail->flags |= NOTE_FRIEND_TAG;
    if (!p->saved)
    {
      tell_player(p, " But you have no list? Oooer..\n");
      return;
    }
    l = p->saved->list_top;
    if (!l)
    {
      tell_player(p, " But you have no list.\n");
      return;
    }
    oldstack = stack;
    sprintf(stack, "%s", p->name);	/* dummy head, and keep personal copy */
    stack = strchr(stack, 0);
    do
    {
      if (l->flags & FRIEND)
      {
	fr = 1;
	sprintf(stack, ",%s", l->name);
	stack = strchr(stack, 0);
      }
      l = l->next;
    }
    while (l);
    stack = end_string(stack);
    if (!fr)
    {
      tell_player(p, " You have no friends...\n");
      return;
    }
  }
  /* end friend check */

  if (p->edit_info)
  {
    p->edit_info->misc = (void *) mail;
    if (mail->flags & NOTE_FRIEND_TAG)
    {
      length = strlen(oldstack) + 1;
      mail->text.where = (char *) MALLOC(length);
      memcpy(mail->text.where, oldstack, length);
      stack = oldstack;
    }
    else
    {
      length = strlen(recip) + 1;
      mail->text.where = (char *) MALLOC(length);
      memcpy(mail->text.where, recip, length);
    }
  }
  else
    FREE(mail);
  stack = oldstack;
}


/* the su news command */

/*
 * View the mail you've sent
 * A quick spoon by astyanax
 */

void read_sent(player * p, char *str)
{
  char *oldstack;
  note *mail, *next;
  int count = 1, scan, i;

  oldstack = stack;
  i = atoi(str);
  if (i < 1 || !str[0])
  {
    tell_player(p, " Format: mail readsent <number>\n");
    return;
  }

  if (!p->saved)
  {
    tell_player(p, " You have no save file, and therefore no mail "
		"either.\n");
    return;
  }
  scan = p->saved->mail_sent;
  mail = find_note(scan);
  if (!mail)
  {
    p->saved->mail_sent = 0;
    tell_player(p, " You have sent no mail.\n");
    return;
  }

  count = 1;
  while (count < i && mail)
  {
    scan = mail->next_sent;
    next = find_note(scan);
    if (!next && scan)
    {
      mail->next_sent = 0;
      mail = 0;
      break;
    }
    else
    {
      mail = next;
    }
    count++;
  }

  if (mail)
  {
    get_string(stack, mail->text.where);
    stack = end_string(stack);
    pager(p, oldstack);
  }
  else
  {
    sprintf(stack, "No such sent letter #%d to read\n", i);
    stack = end_string(stack);
    tell_player(p, oldstack);
  }
  stack = oldstack;
}
