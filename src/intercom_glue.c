/*
 * Playground+ - intercom_glue.c
 * The intercom commands for the talker (by Grim)
 * ---------------------------------------------------------------------------
 */

#include "include/config.h"
#ifdef INTERCOM

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#ifndef BSDISH
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <ctype.h>

#include "include/player.h"
#include "include/intercom.h"
#include "include/intercom_glue.h"
#include "include/proto.h"

extern void close_only_main_fd(void);

#ifdef __GNUC__
static void send_to_intercom(player *, const char *,...) __attribute__((format(printf, 2, 3)));
#else
static void send_to_intercom(player *, const char *,...);
#endif
nameban *nameban_anchor = 0;
extern struct command intercom_list[];

/*INTERNS */
nameban *check_intercom_banished_name(char *);
player *make_dummy_intercom_player(void);

void intercom_command(player * p, char *str)
{
  if (!*str)
  {
    tell_player(p, " Format: intercom <sub command>\n");
    return;
  }
  sub_command(p, str, intercom_list);

  return;
}

void view_intercom_commands(player * p, char *str)
{
  view_sub_commands(p, intercom_list);
}

static void setup_namebans(void)
{
  FILE *fp;
  char *oldstack;
  nameban *new_var;

  oldstack = stack;

  nameban_anchor = 0;
  fp = fopen("files/interban", "r");

  if (!fp)
    return;

  while (!feof(fp))
  {
    *stack = 0;
    fgets(stack, MAX_NAME + 3, fp);

    if (*stack)
    {
      stack = strchr(oldstack, '\n');
      if (stack)
	*stack = 0;

      new_var = (nameban *) MALLOC(sizeof(nameban));
      memset(new_var, 0, sizeof(nameban));
      oldstack[MAX_NAME - 1] = 0;
      strcpy(new_var->name, oldstack);
      new_var->type = 2;
      new_var->next = nameban_anchor;
      nameban_anchor = new_var;

      stack = oldstack;
    }
  }

  fclose(fp);

  return;
}

void start_intercom()
{
  char intercom_name[MAX_TITLE + 10];
  player *scan;

  sprintf(stack, "-=> %s <=- Intercom server on port %d", get_config_msg("talker_name"), active_port - 1);
  stack[MAX_TITLE + 9] = 0;

  strcpy(intercom_name, stack);

  if (intercom_pid > 0 || intercom_fd > 0)
    kill_intercom();

  if (establish_intercom_server() > 0)
    kill_intercom();

  log("intercom", "Forking to boot Intercom Server.");
  intercom_pid = fork();

  switch (intercom_pid)
  {
    case 0:
      /*The child process */

      /*Close all open sockets from this side */
      close_only_main_fd();

      for (scan = flatlist_start; scan; scan = scan->flat_next)
	if (scan->fd > 0)
	{
	  close(scan->fd);
	}
      execlp("bin/intercom", intercom_name, 0);
      log("intercom", "Error, failed to exec intercom");
      exit(1);
      break;
    case -1:
      log("intercom", "Error, failed to fork main server");
      break;
  }

  intercom_fd = -1;

  if (!nameban_anchor)
    setup_namebans();

  intercom_last = time(NULL) + 2400;

  return;
}

void kill_intercom()
{
  if (intercom_fd > 0)
    send_to_intercom(NULL, "%c", INTERCOM_DIE);

  intercom_fd = -1;

  if (!(sys_flags & SHUTDOWN) && (!current_player))
    log("error", "Shutting down intercom server.\n");

  if (intercom_pid > 1)
    /*if (kill(intercom_pid, SIGHUP) < 0) */
    kill(intercom_pid, SIGKILL);

  intercom_pid = -1;

  return;
}

int establish_intercom_server()
{
  int this_fd;
  struct sockaddr_un sa;

  if (intercom_fd < -100)
  {
    /*FAiled to connect 100 times, thats for about 30 seconds. Kill the
       server and reboot it */
    kill_intercom();
    start_intercom();
  }
  /*Connect to the intercom's unix socket */
  this_fd = socket(PF_UNIX, SOCK_STREAM, 0);

  if (this_fd < 0)
  {
    log("error", "Failed to create talker to intercoms unix socket");
    return intercom_fd - 1;
  }
  sa.sun_family = AF_UNIX;
  strcpy(sa.sun_path, INTERCOM_SOCKET);

  if (connect(this_fd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
  {
    if (intercom_fd == -2)
      log("error", "Failed to connect to intercom unix socket");
    close(this_fd);
    return intercom_fd - 1;
  }
  return this_fd;
}

void send_to_intercom(player * p, const char *fmt,...)
{
  va_list varlist;
  char *oldstack;

  oldstack = stack;

  if (p && intercom_fd < 1)
  {
    tell_player(p, " The intercom is currently down.\n");
    return;
  }
  va_start(varlist, fmt);
  vsprintf(stack, fmt, varlist);
  va_end(varlist);

  if (oldstack[strlen(oldstack) - 1] == (char) INCOMPLETE_MESSAGE ||
      oldstack[strlen(oldstack) - 1] == (char) END_MESSAGE)
    stack = end_string(oldstack);
  else
  {
    stack = strchr(oldstack, 0);
    *stack++ = (char) END_MESSAGE;
    *stack++ = '\0';
  }

  if (intercom_fd > 0)
    write(intercom_fd, oldstack, strlen(oldstack));

  stack = oldstack;
}


static void send_intercom_portnumber(void)
{
  send_to_intercom(NULL, "%c%d", PORTNUMBER_FOLLOWS, intercom_port);

  return;
}

static void intercom_su_wall(char *str)
{
  char *oldstack;

  if (!str || !*str)
    return;

  oldstack = stack;
  sprintf(oldstack, "-=> %s", str);
  stack = end_string(oldstack);
  if (*(stack - 2) != '\n')
  {
    /*Late termination code for Nevyn */
    sprintf((stack - 1), "%s\n", COLOUR_TERMINATOR);
    stack = end_string(stack);
  }
  su_wall(oldstack);
  stack = oldstack;

  return;
}

static char *format_output(player * p, char *str)
{
  char *ptr, *end_word, *oldstack = stack;
  int length = 0, longest = 0, line_pos;

  /*Firstly find the length of the longest element. NOTE we only format the
     first line returned to us */
  ptr = str;

  if (!ptr)
    return str;

  while (*ptr && *ptr != '\n')
  {
    if (*ptr == ':')
    {
      if (length > longest)
	longest = length;
      length = 0;
    }
    else
      length++;

    ptr++;
  }

  if (length > longest)
    longest = length;

  /*To add padding, incriment the value of longest */
  longest += 3;

  /*Now write everything to the stack */
  line_pos = 0;
  ptr = str;

  while (*ptr && *ptr != '\n')
  {
    if (longest + line_pos + 2 > p->term_width)
    {
      *stack++ = '\n';
      line_pos = 0;
    }
    end_word = ptr;
    /*Check if end_word is not : \0 or \n */
    while (*end_word && !strchr("\n:", *end_word))
      end_word++;

    /*We need to have longest characters added, so add space padding */
    length = longest - (end_word - ptr);
    while (length)
    {
      length--;
      *stack++ = ' ';
    }

    /*Copy the name in */
    memcpy(stack, ptr, end_word - ptr);
    stack += (end_word - ptr);
    line_pos += longest;

    /*Now move the pointer on... */
    if (*end_word == ':')
      ptr = end_word + 1;
    else
      ptr = end_word;
  }

  *stack = 0;

  strcat(oldstack, ptr);
  stack = end_string(stack);

  return oldstack;
}


static void intercom_tell_player(char *str, int format)
{
  char *name, *msg;
  player *p;
  char *oldstack = stack, *stack_ptr, *end_ptr;

  name = str;

  if (!str || !*str)
  {
    log("error", "Empty message in intercom_tell_player");
    return;
  }
  msg = strchr(str, ':');
  if (msg)
    *msg++ = '\0';

  if (!msg || !*msg)
    return;

  p = find_player_global_quiet(name);

  if (!p)
    return;

  /*We have a player, p, so if we need to, format the output */
  if (format)
    msg = format_output(p, msg);

  /*Put the whole thing on the stack, so we can add a \n on it. This is thanks
     to Nevyn at Crazylands, cos he threw away process output and now Crazylands
     cant handle the \n at the end cos of colour code methods. */

  stack_ptr = stack;
  strcpy(stack_ptr, msg);
  stack = end_string(stack_ptr);
  end_ptr = stack - 1;

  if (end_ptr >= stack_ptr && *end_ptr != '\n')
  {
    sprintf(end_ptr, "%s\n", COLOUR_TERMINATOR);
    stack = end_string(end_ptr);
  }
  if (p->flags & PROMPT)
    pager(p, stack_ptr);
  else
    tell_player(p, stack_ptr);

  stack = oldstack;

  command_type = 0;

  return;
}

nameban *check_intercom_banished_name(char *str)
{
  nameban *scan;

  scan = nameban_anchor;

  while (scan)
  {
    if (scan->name[0] && !strcasecmp(scan->name, str))
      return scan;
    scan = scan->next;
  }

  return 0;
}

static void intercom_tell_player_and_return(char *str)
{
  char *job_id, *name, *msg = 0, *ptr, *oldstack;
  player *p = 0;
  char end_was;
  char *end_pos;
  list_ent *l;
  saved_player *sp;
  char *end_ptr;

  if (!str || !*str)
  {
    log("error", "Empty message in intercom_tell_player");
    return;
  }
  job_id = str;

  name = strchr(job_id, ':');
  if (name)
  {
    *name++ = '\0';
    msg = strchr(name, ':');
    if (msg)
      *msg++ = '\0';
  }
  if (!msg || !*msg || !*name || !*job_id)
    return;

  if (strcasecmp(name, "me"))
    p = find_player_global_quiet(name);

  if (!p)
  {
    send_to_intercom(NULL, "%c%c%s:%s", REPLY_IS, NO_SUCH_PLAYER, job_id, name);
    command_type = 0;
    return;
  }
  if (p->tag_flags & BLOCK_TELLS)
  {
    send_to_intercom(NULL, "%c%c%s:%s", REPLY_IS, TALKER_BLOCKED, job_id,
		     p->name);
    return;
  }
  /*Check the list entry isnt an ignore */
  ptr = strchr(msg, '@');
  if (!ptr)
  {
    log("error", "intercom tried to send to player without a valid name.");
    return;
  }
  end_pos = ptr + 1;
  while (*end_pos && *end_pos != ',' && *end_pos != '\'' && *end_pos != ' ')
    end_pos++;

  end_was = *end_pos;
  *end_pos = '\0';

  /*Now at the start of the message we have the name and address and thats it */
  l = find_list_entry(p, msg);
  if (!l)
    l = find_list_entry(p, ptr);
  if (!l)
    l = find_list_entry(p, "@");
  if (!l)
  {
    *ptr = '\0';
    l = find_list_entry(p, msg);
    *ptr = '@';
  }
  /*One quick check, is the sender a banished name here */
  oldstack = stack;
  *ptr = '\0';
  strcpy(oldstack, msg);
  *ptr = '@';
  stack = end_string(oldstack);
  lower_case(oldstack);
  sp = find_saved_player(oldstack);
  if (sp)
    if (sp->residency == BANISHED || sp->residency & BANISHD)
    {
      send_to_intercom(NULL, "%c%c%s:%s", REPLY_IS, NAME_BANISHED,
		       job_id, p->name);
      stack = oldstack;
      return;
    }
  if (check_intercom_banished_name(oldstack))
  {
    send_to_intercom(NULL, "%c%c%s:%s", REPLY_IS, NAME_BANISHED,
		     job_id, p->name);
    stack = oldstack;
    return;
  }
  stack = oldstack;

  *end_pos = end_was;

  if (l)
  {
    if (l->flags & IGNORE)
    {
      if (*(l->name) == '@')
	send_to_intercom(NULL, "%c%c%s:%s", REPLY_IS, TALKER_IGNORED, job_id,
			 p->name);
      else
	send_to_intercom(NULL, "%c%c%s:%s", REPLY_IS, NAME_IGNORED, job_id,
			 p->name);
      return;
    }
    if (l->flags & BLOCK)
    {
      if (*(l->name) == '@')
	send_to_intercom(NULL, "%c%c%s:%s", REPLY_IS, TALKER_BLOCKED, job_id,
			 p->name);
      else
	send_to_intercom(NULL, "%c%c%s:%s", REPLY_IS, NAME_BLOCKED,
			 job_id, p->name);
      return;
    }
  }
  /*Put the whole thing on the stack, so we can add a \n on it. This is thanks
     to Nevyn at Crazylands, cos he threw away process output and now Crazylands
     cant handle the \n at the end cos of colour code methods. */
  strcpy(oldstack, msg);
  stack = end_string(oldstack);
  end_ptr = stack - 1;

  if (end_ptr >= oldstack && *end_ptr != '\n')
  {
    sprintf(end_ptr, "%s\n", COLOUR_TERMINATOR);
    stack = end_string(end_ptr);
  }
  if (p->flags & PROMPT)
    pager(p, oldstack);
  else
    tell_player(p, oldstack);

  send_to_intercom(NULL, "%c%c%s:%s", REPLY_IS, COMMAND_SUCCESSFUL,
		   job_id, p->name);

  return;
}

player *make_dummy_intercom_player(void)
{
  static player dummy;

  memset(&dummy, 0, sizeof(dummy));

  current_player = &dummy;

  strcpy(dummy.name, "someone@intercom");
  dummy.fd = intercom_fd;
  dummy.residency = NON_RESIDENT;

  return (&dummy);
}


static void parse_user_command_examine_in(char *str)
{
  char *oldstack, *job_id, *name = 0;
  player *p;

  oldstack = stack;

  /*Format will be job_id:name */

  job_id = str;
  if (job_id && *job_id)
  {
    name = strchr(job_id, ':');
    if (name)
      *name++ = '\0';
  }
  if (!name || !*name || !job_id || !*job_id)
  {
    log("error", "Bad format in examine from intercom.\n");
    return;
  }
  if (!strcasecmp(name, "me"))
  {
    send_to_intercom(NULL, "%c%c%s:me", REPLY_IS, NO_SUCH_PLAYER, job_id);
    return;
  }
  /*We know they are here, and exist, so we can examine, otherwise end */

  send_to_intercom(NULL, "%c%c%s:x:%c", REPLY_IS, COMMAND_EXAMINE,
		   job_id, (char) INCOMPLETE_MESSAGE);

  p = make_dummy_intercom_player();
  newexamine(p, name);
  send_to_intercom(NULL, "%c", (char) END_MESSAGE);

  stack = oldstack;

  return;

}

static void parse_user_command_finger_in(char *str)
{
  char *oldstack, *job_id, *name = 0;
  player *p;

  oldstack = stack;

  /*Format will be job_id:name */

  job_id = str;
  if (job_id && *job_id)
  {
    name = strchr(job_id, ':');
    if (name)
      *name++ = '\0';
  }
  if (!name || !*name || !job_id || !*job_id)
  {
    log("error", "Bad format in finger from intercom.\n");
    return;
  }
  if (!strcasecmp(name, "me"))
  {
    send_to_intercom(NULL, "%c%c%s:me", REPLY_IS, NO_SUCH_PLAYER, job_id);
    return;
  }
  if (!strcasecmp(name, "friends"))
  {
    send_to_intercom(NULL, "%c%c%s:friends", REPLY_IS, NO_SUCH_PLAYER, job_id);
    return;
  }
  send_to_intercom(NULL, "%c%c%s:x:%c", REPLY_IS, COMMAND_FINGER,
		   job_id, (char) INCOMPLETE_MESSAGE);

  p = make_dummy_intercom_player();
  newfinger(p, name);
  send_to_intercom(NULL, "%c", (char) END_MESSAGE);

  stack = oldstack;

  return;

}

static void parse_user_command_who_in(char *str)
{
  char *oldstack;
  player *scan;
  int count = 0;

  if (!str || !*str)
    return;

  oldstack = stack;

  scan = flatlist_start;

  while (scan)
  {
    if (scan->location)
    {
      sprintf(stack, "%s%s:", scan->location == intercom_room ? "*" : "",
	      scan->name);
      stack = strchr(stack, 0);
      count++;
    }
    scan = scan->flat_next;
  }

  if (count > 0)
  {
    stack--;
    *stack = 0;
  }
  sprintf(stack, "\nThere %s currently %d user%s connected to ",
	  count != 1 ? "are" : "is", count, count != 1 ? "s" : "");

  stack = end_string(stack);

  send_to_intercom(NULL, "%c%c%s:x:%s", REPLY_IS, COMMAND_WHO, str, oldstack);

  stack = oldstack;

  return;
}

static void parse_user_command_lsu_in(char *str)
{
  char *oldstack, *job_id;
  player *p;
  char empty_string[2];

  empty_string[0] = 0;

  oldstack = stack;

  /*Format will be job_id */

  job_id = str;

  if (!job_id || !*job_id)
  {
    log("error", "Bad format in lsu from intercom.\n");
    return;
  }
  send_to_intercom(NULL, "%c%c%s:x:%c", REPLY_IS, COMMAND_LSU,
		   job_id, (char) INCOMPLETE_MESSAGE);

  p = make_dummy_intercom_player();

  lsu(p, empty_string);
  send_to_intercom(NULL, "%c", (char) END_MESSAGE);

  stack = oldstack;

  return;
}

static void parse_user_command_locate_in(char *str)
{
  char *job_id, *name = 0, *ptr;
  player *p;

  if (!str || !*str)
    return;

  job_id = str;

  if (job_id && *job_id)
  {
    name = strchr(job_id, ':');
    if (name)
    {
      *name++ = 0;
      if (*name)
      {
	ptr = strchr(name, ':');
	if (ptr)
	  *ptr = 0;
      }
    }
  }
  if (!name || !*name || !job_id || !*job_id)
  {
    log("error", "Bad format in parse_user_command_locate_in");
    return;
  }
  p = find_player_absolute_quiet(name);

  if (p)
    send_to_intercom(NULL, "%c%c%s:%c", REPLY_IS, COMMAND_LOCATE,
		     job_id, COMMAND_SUCCESSFUL);

  return;
}

static void parse_user_command_idle_in(char *str)
{
  char *oldstack, *job_id, *name = 0;
  player *p;

  oldstack = stack;

  /*Format will be job_id:name */

  job_id = str;
  if (job_id && *job_id)
  {
    name = strchr(job_id, ':');
    if (name)
      *name++ = '\0';
  }
  if (!name || !*name || !job_id || !*job_id)
  {
    log("error", "Bad format in idle from intercom.\n");
    return;
  }
  if (!strcasecmp(name, "me"))
  {
    send_to_intercom(NULL, "%c%c%s:me", REPLY_IS, NO_SUCH_PLAYER, job_id);
    return;
  }
  /*We know they are here, and exist, so we can examine, otherwise end */

  send_to_intercom(NULL, "%c%c%s:x:%c", REPLY_IS, COMMAND_IDLE,
		   job_id, (char) INCOMPLETE_MESSAGE);

  p = make_dummy_intercom_player();
  check_idle(p, name);
  send_to_intercom(NULL, "%c", (char) END_MESSAGE);

  stack = oldstack;

  return;
}

static void parse_user_command_in(char *str)
{
  if (!str || !*str)
    return;

  switch (*str)
  {
    case COMMAND_WHO:
      parse_user_command_who_in(str + 1);
      break;
    case COMMAND_EXAMINE:
      parse_user_command_examine_in(str + 1);
      break;
    case COMMAND_FINGER:
      parse_user_command_finger_in(str + 1);
      break;
    case COMMAND_LSU:
      parse_user_command_lsu_in(str + 1);
      break;
    case COMMAND_LOCATE:
      parse_user_command_locate_in(str + 1);
      break;
    case COMMAND_IDLE:
      parse_user_command_idle_in(str + 1);
      break;
  }

  return;
}

static void tell_intercom_room(char *str)
{
  char *oldstack = stack, *ptr;

  strcpy(oldstack, str);

  ptr = strchr(oldstack, 0);

  /*Late termination code for Nevyn */
  if (*(ptr - 1) != '\n')
    sprintf(ptr, "%s\n", COLOUR_TERMINATOR);

  stack = end_string(oldstack);

  command_type |= ROOM;
  tell_room(intercom_room, oldstack);
  command_type &= ~ROOM;

  stack = oldstack;

  return;
}

static void return_intercom_room_users(char *str)
{
  player *scan;
  int count = 0;
  char *oldstack = stack;

  /*str will be the job ID */
  scan = intercom_room->players_top;

  while (scan)
  {
    if (!(scan->room_next) && count > 0)
    {
      stack -= 2;
      strcpy(stack, " and ");
      stack += 5;
    }
    sprintf(stack, "%s, ", scan->name);
    stack = strchr(stack, 0);

    scan = scan->room_next;
    count++;
  }

  if (count == 0)
    /*There is nobody in the room, dont even reply */
    return;

  /*There was someone here. We will have an extra ,_ at the end, so lets
     lose those */
  stack -= 2;
  *stack++ = '.';
  *stack++ = 0;

  send_to_intercom(NULL, "%c%s:%d:%s", INTERCOM_ROOM_LIST, str, count, oldstack);

  stack = oldstack;

  return;
}

void parse_incoming_intercom()
{
  char c;
  int chars_left;
  char *oldstack, *ptr;
  int make_formatted = 0;

  if (ioctl(intercom_fd, FIONREAD, &chars_left) == -1)
  {
    shutdown(intercom_fd, 2);
    close(intercom_fd);
    intercom_fd = -1;

    log("error", "PANIC on FIONREAD on intercom_fd.");

    return;
  }
  if (!chars_left)
  {
    shutdown(intercom_fd, 2);
    close(intercom_fd);
    intercom_fd = -1;
    log("error", "Link died on intercom_fd");
    return;
  }
  oldstack = stack;

  c = (char) (END_MESSAGE - 1);
  while (chars_left && c != (char) END_MESSAGE)
  {
    chars_left--;
    if (read(intercom_fd, &c, 1) != 1)
    {
      log("error", "Read error on intercom unix socket");
      shutdown(intercom_fd, 2);
      close(intercom_fd);
      intercom_fd = -1;
      return;
    }
    if (c != (char) END_MESSAGE)
      *stack++ = c;
  }
  *stack++ = '\0';

  intercom_last = time(NULL) + 1800;

  ptr = oldstack;

  command_type = 0;

  if (*ptr == HIGHLIGHT_RETURN)
  {
    command_type |= HIGHLIGHT;
    ptr++;
  }
  if (*ptr == PERSONAL_MESSAGE_TAG)
  {
    command_type |= PERSONAL;
    ptr++;
  }
  if (*ptr == FORMAT_MESSAGE_TAG)
  {
    make_formatted = 1;
    ptr++;
  }
  if (*ptr)
  {
    switch (*ptr)
    {
      case USER_COMMAND:
	parse_user_command_in(ptr + 1);
	break;
      case REQUEST_PORTNUMBER:
	send_intercom_portnumber();
	break;
      case SU_MESSAGE:
	intercom_su_wall(ptr + 1);
	break;
      case PERSONAL_MESSAGE:
	intercom_tell_player(ptr + 1, make_formatted);
	break;
      case PERSONAL_MESSAGE_AND_RETURN:
	intercom_tell_player_and_return(ptr + 1);
	break;
      case PING:
	send_to_intercom(NULL, "%c", PING);
	break;
      case STARTING_CONNECT:
	break;
      case INTERCOM_ROOM_MESSAGE:
	tell_intercom_room(ptr + 1);
	break;
      case INTERCOM_ROOM_LOOK:
	return_intercom_room_users(ptr + 1);
	break;
    }
  }
  stack = oldstack;

  if (chars_left > 0)
    parse_incoming_intercom();

  command_type = 0;

  return;
}

void add_intercom_server(player * p, char *str)
{
  char *name, *abbr, *addr, *port, *oldstack;

  name = str;
  abbr = 0;
  addr = 0;
  port = 0;

  if (*name)
  {
    abbr = strchr(name, ':');
    if (abbr)
    {
      *abbr++ = '\0';
      if (*abbr)
      {
	addr = strchr(abbr, ':');
	if (addr)
	{
	  *addr++ = '\0';
	  if (*addr)
	  {
	    port = strchr(addr, ':');
	    if (port)
	      *port++ = '\0';
	  }
	}
      }
    }
  }
  if (!port || !*port || !*addr || !*abbr || !name || !*name)
  {
    tell_player(p, "Format: add_server "
		"<name>:<abbreviation>:<address>:<port>\n");
    return;
  }
  oldstack = stack;

  sprintf(oldstack, " Sending request to intercom to add %s to the database, "
	  "at address %s %s, abbreviated to %s.\n", name, addr, port, abbr);
  stack = end_string(oldstack);

  tell_player(p, oldstack);

  stack = oldstack;

  send_to_intercom(p, "%c%s:%s:%s:%s:%s:O",
		   ADD_NEW_LINK, p->name, name, abbr, addr, port);

  return;
}

void list_intercom_servers(player * p, char *str)
{
  if (str && *str)
  {
    if (!strcasecmp(str, "hidden") && p->residency & PSU)
    {
      send_to_intercom(p, "%c%c%s:", SHOW_LINKS, LIST_HIDDEN, p->name);
      return;
    }
    if (!strcasecmp(str, "up"))
    {
      send_to_intercom(p, "%c%c%s:", SHOW_LINKS, LIST_UP, p->name);
      return;
    }
    if (isalpha(*str))
    {
      send_to_intercom(p, "%c%c%s:", SHOW_LINKS, tolower(*str), p->name);
    }
  }
  send_to_intercom(p, "%c%c%s:", SHOW_LINKS, LIST_ALL, p->name);

  return;
}

void bar_talker(player * p, char *str)
{
  if (!*str)
  {
    tell_player(p, " Format: bar <name>\n");
    return;
  }
  send_to_intercom(p, "%c%s:%s", CLOSE_LINK, p->name, str);

  return;
}

void unbar_talker(player * p, char *str)
{
  if (!*str)
  {
    tell_player(p, " Format: unbar <name>\n");
    return;
  }
  send_to_intercom(p, "%c%s:%s", UNBAR_LINK, p->name, str);

  return;
}

void do_intercom_tell(player * p, char *str)
{
  char *name, *talker, *msg, *oldstack;
  list_ent *l = 0;

  /*Assume we have a str and a space and
     a message, as we checked it before calling it */

  name = str;

  msg = strchr(name, ' ');

  *msg++ = '\0';

  if (strchr(name, ','))
  {
    tell_player(p, " You cannot do chain tells to people and include remote"
		" talkers.\n");
    return;
  }
  talker = strchr(name, '@');
  if (talker)
  {
    l = find_list_entry(p, name);
    if (!l)
      l = find_list_entry(p, talker);
    *talker++ = '\0';
  }
  if (!name || !*name || !talker || !*talker)
  {
    tell_player(p, " Badly formed remote address.\n");
    return;
  }
  if (!l)
    l = find_list_entry(p, "@");
  if (!l)
    l = find_list_entry(p, name);

  oldstack = stack;

  if (l)
  {
    if (l->flags & (IGNORE | BLOCK))
    {
      sprintf(oldstack, " You cannot tell to them, as you are %sing"
	      " %s.\n", l->flags & IGNORE ? "ignor" : "block", l->name);
      stack = end_string(oldstack);

      tell_player(p, oldstack);

      return;
    }
  }
  send_to_intercom(p, "%c%c%s:%s:%s:%s", USER_COMMAND, COMMAND_TELL,
		   p->name, talker, name, msg);

  command_type = 0;

  return;
}

void do_intercom_remote(player * p, char *str)
{
  char *name, *talker, *msg, *oldstack;
  list_ent *l = 0;

  /*Assume we have a str and a space and
     a message, as we checked it before calling it */

  name = str;

  msg = strchr(name, ' ');

  *msg++ = '\0';

  if (strchr(name, ','))
  {
    tell_player(p, " You cannot do chain remotes to people and include remote"
		" talkers.\n");
    return;
  }
  talker = strchr(name, '@');
  if (talker)
  {
    l = find_list_entry(p, name);
    if (!l)
      l = find_list_entry(p, talker);
    *talker++ = '\0';
  }
  if (!name || !*name || !talker || !*talker)
  {
    tell_player(p, " Badly formed remote address.\n");
    return;
  }
  if (!l)
    l = find_list_entry(p, "@");
  if (!l)
    l = find_list_entry(p, name);

  oldstack = stack;

  if (l)
  {
    if (l->flags & (IGNORE | BLOCK))
    {
      sprintf(oldstack, " You cannot remote to them, as you are %sing"
	      " %s.\n", l->flags & IGNORE ? "ignor" : "block", l->name);
      stack = end_string(oldstack);

      tell_player(p, oldstack);

      return;
    }
  }
  send_to_intercom(p, "%c%c%s:%s:%s:%s", USER_COMMAND, COMMAND_REMOTE,
		   p->name, talker, name, msg);

  return;
}

void do_intercom_who(player * p, char *str)
{
  /*Sanity check */
  if (!*str || *str != '@' || !*(str + 1))
  {
    log("error", "Intercom who called with invalid arg");
    return;
  }
  send_to_intercom(p, "%c%c%s:%s:%s:%s", USER_COMMAND, COMMAND_WHO,
		   p->name, str + 1, "x", "x");

  return;
}

void do_intercom_examine(player * p, char *str)
{
  char *name, *location;

  name = str;
  location = 0;

  if (name && *name)
  {
    location = strchr(name, '@');
    if (location)
      *location++ = '\0';
  }
  if (!location || !*location || !name || !*name)
  {
    tell_player(p, " Format: examine user@location\n");
    return;
  }
  send_to_intercom(p, "%c%c%s:%s:%s:%s", USER_COMMAND, COMMAND_EXAMINE, p->name,
		   location, name, "x");

  return;
}

void do_intercom_finger(player * p, char *str)
{
  char *name, *location;

  name = str;
  location = 0;

  if (name && *name)
  {
    location = strchr(name, '@');
    if (location)
      *location++ = '\0';
  }
  if (!location || !*location || !name || !*name)
  {
    tell_player(p, " Format: finger user@location\n");
    return;
  }
  send_to_intercom(p, "%c%c%s:%s:%s:%s", USER_COMMAND, COMMAND_FINGER, p->name,
		   location, name, "x");

  return;
}

void intercom_ping(player * p, char *str)
{
  if (!*str)
  {
    tell_player(p, " Format: ping <talker>\n");
    return;
  }
  send_to_intercom(p, "%c%s:%s", OPEN_LINK, p->name, str);

  return;
}

void close_intercom(player * p, char *str)
{
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, "Please go on duty to do that.\n");
    return;
  }
  send_to_intercom(p, "%c%s:", CLOSE_ALL_LINKS, p->name);
}

void open_intercom(player * p, char *str)
{
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, "Please go on duty to do that.\n");
    return;
  }
  send_to_intercom(p, "%c%s:", OPEN_ALL_LINKS, p->name);
}

void delete_intercom_server(player * p, char *str)
{
  if (!*str)
  {
    tell_player(p, " Format: delete_server <name|alias>\n");
    return;
  }
  send_to_intercom(p, "%c%s:%s", DELETE_LINK, p->name, str);

  return;
}

void intercom_change_name(player * p, char *str)
{
  char *new_var;

  new_var = 0;

  if (*str)
  {
    new_var = strchr(str, ':');
    if (new_var)
      *new_var++ = '\0';
  }
  if (!new_var || !*new_var || !*str)
  {
    tell_player(p, " Format: change_name <old name>:<new name>\n");
    return;
  }
  send_to_intercom(p, "%c%s:%s:%s", CHANGE_NAME, p->name, str, new_var);

  return;
}

void intercom_change_alias(player * p, char *str)
{
  char *new_var;

  new_var = 0;

  if (*str)
  {
    new_var = strchr(str, ':');
    if (new_var)
      *new_var++ = '\0';
  }
  if (!new_var || !*new_var || !*str)
  {
    tell_player(p, " Format: change_alias <old alias>:<new alias>\n");
    return;
  }
  send_to_intercom(p, "%c%s:%s:%s", CHANGE_ABBR, p->name, str, new_var);

  return;
}

void intercom_change_address(player * p, char *str)
{
  char *new_var;

  new_var = 0;

  if (*str)
  {
    new_var = strchr(str, ':');
    if (new_var)
      *new_var++ = '\0';
  }
  if (!new_var || !*new_var || !*str)
  {
    tell_player(p, " Format: change_address <name|alias>:<new address>\n");
    return;
  }
  send_to_intercom(p, "%c%s:%s:%s", CHANGE_ADDRESS, p->name, str, new_var);

  return;
}

void intercom_change_port(player * p, char *str)
{
  char *new_var;

  new_var = 0;

  if (*str)
  {
    new_var = strchr(str, ':');
    if (new_var)
      *new_var++ = '\0';
  }
  if (!new_var || !*new_var || !*str)
  {
    tell_player(p, " Format: change_port <name|alias>:<new port>\n");
    return;
  }
  send_to_intercom(p, "%c%s:%s:%s", CHANGE_PORT, p->name, str, new_var);

  return;
}

void intercom_reboot(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;
  tell_player(p, " Attempting to reboot intercom.\n");

  sprintf(oldstack, "-=> %s reboots the intercom server.\n", p->name);
  stack = end_string(oldstack);

  su_wall_but(p, oldstack);
  stack = oldstack;

  kill_intercom();
  start_intercom();

  return;
}


void intercom_banish(player * p, char *str)
{
  if (!*str)
  {
    tell_player(p, " Format: banish <name|alias>\n");
    return;
  }
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, "Please go on duty to do that.\n");
    return;
  }
  send_to_intercom(p, "%c%s:%s", BANISH_SITE, p->name, str);
  return;
}

void intercom_update_servers(player * p, char *str)
{
  tell_player(p, " Requesting talker lists from all connected talkers.\n");

  send_to_intercom(p, "%c", REQUEST_SERVER_LIST);

  return;
}

void intercom_request_stats(player * p, char *str)
{
  send_to_intercom(p, "%c%s:%s", REQUEST_STATS, p->name, str);

  return;
}

void do_intercom_lsu(player * p, const char *str)
{
  char empty_string[2];

  empty_string[0] = 0;

  if (!(p->residency & PSU))
    /*Only an SU can see SUs on another talker */
  {
    lsu(p, empty_string);
    return;
  }
  /*Sanity check */
  if (!*str || *str != '@' || !*(str + 1))
  {
    lsu(p, empty_string);
    return;
  }
  send_to_intercom(p, "%c%c%s:%s:x:x", USER_COMMAND, COMMAND_LSU,
		   p->name, str + 1);

  return;
}

void intercom_bar_name(player * p, char *str)
{
  nameban *new_var;
  char *oldstack;

  if (!*str)
  {
    tell_player(p, " Format: intercom bar_name <name>.\n");
    return;
  }
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, "Please go on duty to do that.\n");
    return;
  }
  if (strlen(str) > MAX_NAME - 1)
    str[MAX_NAME - 1] = 0;

  if (check_intercom_banished_name(str))
  {
    tell_player(p, " That name is already barred.\n");
    return;
  }
  new_var = (nameban *) MALLOC(sizeof(nameban));
  memset(new_var, 0, sizeof(nameban));

  if (nameban_anchor)
    new_var->next = nameban_anchor;

  nameban_anchor = new_var;

  strcpy(new_var->name, str);
  new_var->type = 1;

  oldstack = stack;
  sprintf(oldstack, " The name '%s' is now barred from the intercom.\n", str);
  stack = end_string(oldstack);

  tell_player(p, oldstack);

  sprintf(oldstack, "-=> %s bars the name '%s' from using the intercom.\n",
	  p->name, str);
  stack = end_string(oldstack);
  su_wall(oldstack);
  stack = oldstack;

  return;
}

void intercom_unbar_name(player * p, char *str)
{
  nameban *scan, *prev;
  char *oldstack;

  if (!*str)
  {
    tell_player(p, " Format: intercom unbar_name <name>.\n");
    return;
  }
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, "Please go on duty to do that.\n");
    return;
  }
  scan = nameban_anchor;
  prev = 0;
  while (scan)
  {
    if (scan->name[0] && !strcasecmp(scan->name, str))
    {
      if (scan->type == 2)
      {
	tell_player(p, " They are banished, you need to use intercom "
		    "unbanish_name to re-permit them.\n");
	return;
      }
      tell_player(p, " Name unbanished.\n");
      oldstack = stack;
      sprintf(oldstack, "-=> %s allows the name '%s' to use the "
	      "intercom.\n", p->name, str);
      stack = end_string(oldstack);
      su_wall(oldstack);
      stack = oldstack;

      if (prev)
	prev->next = scan->next;
      else
	nameban_anchor = scan->next;

      FREE(scan);

      return;
    }
    prev = scan;
    scan = scan->next;
  }

  tell_player(p, " No such name in the intercom barred lists.\n");

  return;
}

void intercom_banish_name(player * p, char *str)
{
  nameban *new_var;
  FILE *fp;
  char *oldstack;

  if (!*str)
  {
    tell_player(p, " Format: intercom banish_name <name>\n");
    return;
  }
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, "Please go on duty to do that.\n");
    return;
  }
  new_var = check_intercom_banished_name(str);
  if (new_var)
  {
    if (new_var->type == 2)
    {
      tell_player(p, " That name is already banished.\n");
      return;
    }
    tell_player(p, " Barred name, attempting to banish...\n");
  }
  else
  {
    new_var = (nameban *) MALLOC(sizeof(nameban));
    memset(new_var, 0, sizeof(nameban));
    strcpy(new_var->name, str);
    new_var->type = 2;
    new_var->next = nameban_anchor;
    nameban_anchor = new_var;
  }

  /*Here we have new_var, and we need to just append it to the banished lists */
  fp = fopen("files/interban", "a");

  if (!fp)
  {
    log("error", "Error opening intercom banish file for writing.\n");
    return;
  }
  fprintf(fp, "%s\n", str);
  fclose(fp);

  tell_player(p, " Name banished.\n");

  oldstack = stack;
  sprintf(oldstack, "-=> %s banishes the name '%s' from using the intercom.\n",
	  p->name, str);
  stack = end_string(oldstack);

  su_wall(oldstack);

  stack = oldstack;

  return;
}

static void sync_banish_names(void)
{
  FILE *fp;
  nameban *scan;

  fp = fopen("files/interban", "w");

  if (!fp)
  {
    log("error", " Error opening interban file for writing.\n");
    return;
  }
  scan = nameban_anchor;

  while (scan)
  {
    if (scan->type == 2)
      fprintf(fp, "%s\n", scan->name);
    scan = scan->next;
  }

  fclose(fp);

  return;
}

void intercom_unbanish_name(player * p, char *str)
{
  nameban *prev, *scan;
  char *oldstack;

  if (!*str)
  {
    tell_player(p, "Format: intercom unbanish_name <name>\n");
    return;
  }
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, "Please go on duty to do that.\n");
    return;
  }
  scan = nameban_anchor;
  prev = 0;

  while (scan)
  {
    if (scan->name[0] && !strcasecmp(scan->name, str))
    {
      if (scan->type == 1)
	tell_player(p, " Name unbarred.\n");
      else
	tell_player(p, " Name unbanished.\n");

      oldstack = stack;
      sprintf(oldstack, "-=> %s allows the name '%s' to use the "
	      "intercom.\n", p->name, str);
      stack = end_string(oldstack);
      su_wall(oldstack);
      stack = oldstack;

      if (prev)
	prev->next = scan->next;
      else
	nameban_anchor = scan->next;

      if (scan->type == 2)
	sync_banish_names();

      FREE(scan);

      return;
    }
    prev = scan;
    scan = scan->next;
  }

  tell_player(p, " That name is not in the banished list.\n");

  return;
}

void intercom_slist(player * p, char *str)
{
  send_to_intercom(p, "%c%s:", SHOW_ALL_LINKS_SHORT, p->name);

  return;
}

void intercom_hide(player * p, char *str)
{
  if (!*str)
  {
    tell_player(p, " Format: intercom hide <talker>\n");
    return;
  }
  send_to_intercom(p, "%c%s:%s", HIDE_ENTRY, p->name, str);

  return;
}

void intercom_unhide(player * p, char *str)
{
  if (!*str)
  {
    tell_player(p, " Format: intercom unhide <talker>\n");
    return;
  }
  send_to_intercom(p, "%c%s:%s", UNHIDE_ENTRY, p->name, str);

  return;
}

void intercom_version(player * p, char *str)
{
  TELLPLAYER(p, " Intercom version %s (%s)\n", INTERCOM_VERSION, __DATE__);
  return;
}

void intercom_locate_name(player * p, char *str)
{
  /*Sanity check */
  if (!str || !*str)
  {
    tell_player(p, " Format: intercom locate <name>\n");
    return;
  }
  send_to_intercom(p, "%c%c%s:x:%s:x", USER_COMMAND, COMMAND_LOCATE,
		   p->name, str);

  return;
}

void do_intercom_idle(player * p, char *str)
{
  char *name, *location = 0;

  name = str;

  if (name && *name)
  {
    location = strchr(name, '@');
    if (location)
      *location++ = '\0';
  }
  if (!location || !*location || !name || !*name || !(isalpha(*name)))
  {
    tell_player(p, " Format: idle user@location\n");
    return;
  }
  send_to_intercom(p, "%c%c%s:%s:%s:%s", USER_COMMAND, COMMAND_IDLE, p->name,
		   location, name, "x");

  return;
}

void intercom_home(player * p, char *str)
{

  /*If they are already there.... */
  if (p->location == intercom_room)
  {
    tell_player(p, " You're already in the Intercom room!\n");
    return;
  }
  /* or if they're stuck */
  if (p->no_move)
  {
    tell_player(p, " You seem to be stuck to the ground.\n");
    return;
  }
  /* otherwise move them */
  move_to(p, "intercom.external", 0);
}

void do_intercom_say(player * p, char *str)
{
  char *ptr, *oldstack;
  const char *method;

  oldstack = stack;

  ptr = strchr(str, 0);
  ptr--;

  if (ptr < str)
  {
    log("error", "Zero length string passed to do_intercom_say\n");
    return;
  }
  switch (*ptr)
  {
    case '!':
      method = "exclaim";
      break;
    case '?':
      method = "ask";
      break;
    default:
      method = "say";
      break;
  }

  TELLPLAYER(p, " You %s '%s'\n", method, str);

  sprintf(oldstack, "%s %ss '%s'\n", p->name, method, str);

  stack = end_string(oldstack);

  tell_room_but(p, intercom_room, oldstack);

  stack = oldstack;

  send_to_intercom(p, "%c%c%s:x:x:%s", USER_COMMAND, COMMAND_SAY,
		   p->name, str);

  return;
}

void do_intercom_emote(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;

  if (!str || !*str)
  {
    log("error", "Zero length string passed to do_intercom_emote\n");
    return;
  }
  if (*str == '\'')
    TELLPLAYER(p, " You emote: %s%s\n", p->name, str);
  else
    TELLPLAYER(p, " You emote: %s %s\n", p->name, str);


  if (*str == '\'')
    sprintf(oldstack, "%s%s\n", p->name, str);
  else
    sprintf(oldstack, "%s %s\n", p->name, str);
  stack = end_string(oldstack);

  tell_room_but(p, intercom_room, oldstack);

  stack = oldstack;

  send_to_intercom(p, "%c%c%s:x:x:%s", USER_COMMAND, COMMAND_EMOTE,
		   p->name, str);

  return;
}

void intercom_room_look(player * p)
{
  send_to_intercom(p, "%c%s:", INTERCOM_ROOM_LOOK, p->name);

  return;
}

void intercom_site_move(player * p, char *str)
{
  char *site = 0, *port = 0;
  char *oldstack;

  oldstack = stack;
  site = str;

  if (p->flags & BLOCK_SU)
  {
    tell_player(p, "Please go on duty to do that.\n");
    return;
  }
  if (site && *site)
  {
    port = strchr(site, ':');
    if (port)
      *port++ = 0;
  }
  if (!port || !*port || !site || !*site)
  {
    tell_player(p, " Format: intercom announce_move sitename:portnumber\n");
    return;
  }
  TELLPLAYER(p, " Informing all remote talkers of the address change to "
	     "%s %s\n", site, port);

  send_to_intercom(p, "%c%s:%s", WE_ARE_MOVING, site, port);

  close_intercom(p, str);

  return;
}

void do_intercom_think(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;

  if (!str || !*str)
  {
    log("error", "Zero length string passed to do_intercom_think\n");
    return;
  }
  TELLPLAYER(p, " You think . o O ( %s )\n", str);

  sprintf(oldstack, "%s thinks . o O ( %s )\n", p->name, str);
  stack = end_string(oldstack);

  tell_room_but(p, intercom_room, oldstack);

  stack = oldstack;

  send_to_intercom(p, "%c%c%s:x:x:thinks . o O ( %s )", USER_COMMAND,
		   COMMAND_EMOTE, p->name, str);

  return;
}

/* version stuff for pg_version */

void pg_intercom_version(void)
{
  sprintf(stack, " -=*> Intercom server v%s (by Grim) enabled.\n", INTERCOM_VERSION);
  stack = strchr(stack, 0);
}

#endif
