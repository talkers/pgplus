/*
 * Playground+ - intercom.c
 * The intercom talk server (by Grim)
 * ---------------------------------------------------------------------------
 */

#include "include/config.h"
#include <stdio.h>


#ifdef INTERCOM

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#ifndef BSDISH
#include <malloc.h>
#endif
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <netdb.h>
#include <signal.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

#include "include/intercom.h"
#ifdef ANSI
#include "include/ansi.h"
#endif


/* lets give intercom soft msgs shall we? */
#include "xstring.c"

#ifndef MAX_NAME		/* intercom.h relies on MAX_NAME to decide if player.h
				   should be included, so shall we */
typedef struct file
{
  char *where;
  int length;
}
file;
#endif /* ! MAX_NAME */
file config_msg;
file load_file(char *);
char *get_config_msg(char *);
int max_log_size;




/*Interns */
char *stack, *stack_start, current_name[MAX_NAME];
int closedown, ping_time;
time_t system_time;
int talker_fd, unix_fd, inet_fd, portnumber;
talker_list *talker_list_anchor, *validation_anchor;
unsigned long int intercom_status, job_id;
job_list *job_anchor, *free_jobs_anchor;
packet *free_packet_anchor;
packet *talker_packet_anchor;
net_usage server_net, total_net;

int close_main_socket_now = 0;

/*functions */
static talker_list *match_talker_name(char *);
static talker_list *match_talker_abbr_absolute(char *);
static talker_list *match_talker_abbr_pattern(char *);
static int add_new_talker(char *, int);
static void tell_talker_su(char *);
static int connect_new_talker(talker_list *);
static char *set_name(char *);
static void sync_talkers(void);

#ifdef __GNUC__
static int tell_remote_talker(talker_list *, const char *,...) __attribute__((format(printf, 2, 3)));
     static void tell_personal(const char *,...) __attribute__((format(printf, 1, 2)));
     static void tell_personal_inc_ref(job_list *, const char *,...) __attribute__((format(printf, 2, 3)));
     static void send_to_talker(const char *,...) __attribute__((format(printf, 1, 2)));
#else
static int tell_remote_talker(talker_list *, const char *,...);
static void tell_personal(const char *,...);
static void tell_personal_inc_ref(job_list *, const char *,...);
static void send_to_talker(const char *,...);
#endif

#define I_SHUTDOWN(x,y) if (shutdown(x,y)==-1) fail_shutdown(__LINE__,__FILE__);

static int ATOI(char *str)
{
  return strtol(str, (char **) NULL, 10);
}

/* modified these a bit to go better with the existant stuffs....
   since soft messages wants status quo, and we need soft messages
   here... i undid these a bit.. no harm done at all...  ~phy
 */
char *end_string(char *str)
{
  return (strchr(str, 0) + 1);
}

void lower_case(char *str)
{
  while (*str)
  {
    *str = tolower(*str);
    str++;
  }
}

/* return a string of the system time */

static char *sys_time(time_t t)
{
  static char time_string[25];

  strftime(time_string, 25, "%H:%M:%S - %d/%m/%y", localtime(&t));
  return time_string;
}

/* log errors and things to file */

static void log(const char *filename, const char *string)
{
  int fd, length;

  sprintf(stack, "logs/%s.log", filename);

#ifdef BSDISH
  fd = open(stack, O_CREAT | O_WRONLY | S_IRUSR | S_IWUSR);
#else
  fd = open(stack, O_CREAT | O_WRONLY | O_SYNC, S_IRUSR | S_IWUSR);
#endif
  length = lseek(fd, 0, SEEK_END);
  if (length > max_log_size * 1024)
  {
    close(fd);
#ifdef BSDISH
    fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
    fd = open(stack, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif
  }
  sprintf(stack, "%s - %s\n", sys_time(time(NULL)), string);
  write(fd, stack, strlen(stack));
  close(fd);

  return;
}

static void do_close(talker_list * target)
{
  close(target->fd);

  target->fd = -1;

  target->validation = 0;

  return;
}

static void fail_shutdown(int line, const char *filename)
{
  /*  
     char *oldstack=stack;

     sprintf(stack,"Shutdown failed, %d %s ",line,filename);

     stack=strchr(stack,0);
     switch(errno)
     {
     case EBADF:
     sprintf(stack,"Not a valid fd");
     break;
     case ENOTSOCK:
     sprintf(stack,"Socket is a file");
     break;
     case ENOTCONN:
     sprintf(stack,"Socket not connected");
     break;
     default:
     sprintf(stack,"Unknown error");
     break;
     }
     stack=end_string(stack);
     log("intercom",oldstack);

     stack=oldstack;

   */

  return;
}

static void close_all_sockets(void)
{
  talker_list *scan;

  scan = talker_list_anchor;

  while (scan->next)
  {
    scan = scan->next;

    if (scan->fd > 0)
    {
      if (!(scan->flags & WAITING_CONNECT))
	I_SHUTDOWN(scan->fd, 2);
      do_close(scan);
    }
  }

  return;
}

static void close_intercom_main_socket(void)
{
  if (inet_fd > 0)
  {
    I_SHUTDOWN(inet_fd, 2);
    close(inet_fd);
    log("intercom", "Closing main file descriptor");
    inet_fd = -1;
    close_all_sockets();
  }
  return;
}

static void fatal_error(const char *filename, const char *string)
{
  stack = stack_start;

  log(filename, string);

  close_intercom_main_socket();
  close_all_sockets();
  abort();
}

static void sigpipe(int dummy)
{
  return;
}
static void sighup(int dummy)
{
  log("intercom", "Hangup received. Dying");
  closedown = 1;
  return;
}
static void sigquit(int dummy)
{
  fatal_error("intercom", "Quit signal received.");
}
static void sigill(int dummy)
{
  fatal_error("intercom", "Illegal instruction.");
}
static void sigfpe(int dummy)
{
  fatal_error("intercom", "Floating Point Error.");
}
static void sigbus(int dummy)
{
  fatal_error("intercom", "Bus Error.");
}
static void sigsegv(int dummy)
{
  fatal_error("intercom", "Segmentation Violation.");
}
static void sigterm(int dummy)
{
  fatal_error("intercom", "Terminate signal received.");
}
static void sigxfsz(int dummy)
{
  fatal_error("intercom", "File descriptor limit exceeded.");
}
static void sigchld(int dummy)
{
  fatal_error("intercom", "Received SIGCHLD");
}
static void sigusr1(int dummy)
{
  close_main_socket_now = 1;

  return;
}

/**********************JOBS HANDLING****************/

static job_list *return_job(char *str)
{
  long unsigned int job_num;
  job_list *scan;

  if (!str || !*str)
    return 0;

  job_num = atoi(str);
  scan = job_anchor;
  while (scan->next != job_anchor && scan->next->job_id <= job_num)
  {
    scan = scan->next;
    if (scan->job_id == job_num)
      return scan;
  }

  return 0;
}

static void add_new_jobs(void)
{
  int loopa;
  job_list *new_job;

  /*Add 10 new jobs in */

  for (loopa = 0; loopa < 10; loopa++)
  {
    new_job = (job_list *) calloc(1, sizeof(job_list));
    new_job->next = free_jobs_anchor->next;
    new_job->prev = free_jobs_anchor;
    free_jobs_anchor->next = new_job;
    new_job->next->prev = new_job;
  }

  return;
}

static job_list *make_job_entry(void)
{
  job_list *target_job;

  if (free_jobs_anchor->next == NULL ||
      free_jobs_anchor->next == free_jobs_anchor)
    add_new_jobs();

  target_job = free_jobs_anchor->next;
  target_job->next->prev = target_job->prev;
  target_job->prev->next = target_job->next;

  target_job->next = job_anchor;
  target_job->prev = job_anchor->prev;
  job_anchor->prev = target_job;
  target_job->prev->next = target_job;

  target_job->job_id = ++job_id;
  target_job->timeout = system_time + 120;

  return target_job;
}

static void free_job(job_list * target_job)
{
  target_job->next->prev = target_job->prev;
  target_job->prev->next = target_job->next;

  /*Wipe all info */
  memset(target_job, 0, sizeof(job_list));

  target_job->next = free_jobs_anchor->next;
  target_job->prev = free_jobs_anchor;
  target_job->next->prev = target_job;
  free_jobs_anchor->next = target_job;

  return;
}

static void setup_jobs_list(void)
{
  job_anchor = (job_list *) calloc(1, sizeof(job_list));
  free_jobs_anchor = (job_list *) calloc(1, sizeof(job_list));
  job_anchor->next = job_anchor;
  job_anchor->prev = job_anchor;
  free_jobs_anchor->next = free_jobs_anchor;
  free_jobs_anchor->prev = free_jobs_anchor;

  add_new_jobs();

  job_id = 0;

  return;
}

/*****************PACKET HANDLING********************/

static void make_new_packets(void)
{
  packet *new_var;
  int loop;

  for (loop = 0; loop < 10; loop++)
  {
    new_var = (packet *) calloc(1, sizeof(packet));
    new_var->next = free_packet_anchor->next;
    free_packet_anchor->next = new_var;
  }

  return;
}

static packet *get_new_packet(void)
{
  packet *target;

  if (!(free_packet_anchor->next))
    make_new_packets();

  target = free_packet_anchor->next;
  free_packet_anchor->next = target->next;
  target->next = 0;

  return target;
}

static packet *add_packet_to_talker(talker_list * remote_talker)
{
  packet *new_var, *scan;

  new_var = get_new_packet();
  remote_talker->net_stats.packets_in++;
  total_net.packets_in++;

  if (!(remote_talker->packet_anchor))
  {
    remote_talker->packet_anchor = new_var;
    return new_var;
  }
  scan = remote_talker->packet_anchor;
  while (scan->next)
    scan = scan->next;

  scan->next = new_var;

  return new_var;
}

static packet *add_packet_to_list(void)
{
  packet *new_var, *scan;

  new_var = get_new_packet();

  server_net.packets_in++;

  if (!talker_packet_anchor)
  {
    talker_packet_anchor = new_var;
    return new_var;
  }
  scan = talker_packet_anchor;
  while (scan->next)
    scan = scan->next;

  scan->next = new_var;

  return new_var;
}

static void free_packet(packet * target)
{
  memset(target, 0, sizeof(packet));

  target->next = free_packet_anchor->next;
  free_packet_anchor->next = target;

  return;
}

static void free_list_packets(void)
{
  packet *target;

  target = talker_packet_anchor;
  while (target)
  {
    talker_packet_anchor = target->next;
    free_packet(target);
    target = talker_packet_anchor;
  }

  return;
}

static void free_talker_packets(talker_list * remote_talker)
{
  packet *target;

  target = remote_talker->packet_anchor;
  while (target)
  {
    remote_talker->packet_anchor = target->next;
    free_packet(target);
    target = remote_talker->packet_anchor;
  }

  return;
}

static void setup_free_packets(void)
{
  free_packet_anchor = (packet *) calloc(1, sizeof(packet));

  return;
}

static talker_list *match_address(talker_list * remote_talker)
{
  talker_list *scan;

  scan = talker_list_anchor;

  while (scan->next)
  {
    scan = scan->next;
    if (scan != remote_talker)
    {
      if (remote_talker->num[0] || remote_talker->num[1] ||
	  remote_talker->num[2] || remote_talker->num[3])
	if (scan->num[0] == remote_talker->num[0] &&
	    scan->num[1] == remote_talker->num[1] &&
	    scan->num[2] == remote_talker->num[2] &&
	    scan->num[3] == remote_talker->num[3] &&
	    scan->port == remote_talker->port)
	  return scan;

      if (remote_talker->addr[0])
	if (scan->port == remote_talker->port &&
	    !strcasecmp(scan->addr, remote_talker->addr))
	  return scan;
    }
  }

  return 0;
}

static void actual_validate_send(talker_list * known_talker)
{
  known_talker->validation = -(abs((rand() % 100000) + 1));

  tell_remote_talker(known_talker, "%c%d:%d", REQUEST_VALIDATION_AS,
		     portnumber, abs(known_talker->validation));

  /*We've sent the validation request, disconnect */
  I_SHUTDOWN(known_talker->fd, 2);
  close(known_talker->fd);
  known_talker->fd = -1;

  return;
}

static void request_validation(talker_list * known_talker)
{
  if (!connect_new_talker(known_talker))
  {
    known_talker->flags |= VALIDATE_AFTER_CONNECT;
    return;
  }
  if (known_talker->flags & WAITING_CONNECT)
  {
    known_talker->flags |= VALIDATE_AFTER_CONNECT;
    return;
  }
  actual_validate_send(known_talker);

  return;
}

static void rerequest_validation(talker_list * remote_talker)
{
  talker_list *known;

  known = match_address(remote_talker);

  if (known)
    request_validation(known);
  else
  {
    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);
  }

  /*We have requested the validation, now we wait for its reply */
  return;
}

static void make_unique(talker_list * remote_talker)
{
  char dummy_name[MAX_TALKER_NAME];
  char dummy_abbr[MAX_TALKER_ABBR];
  int loopa;
  int length;

  loopa = 0;
  strcpy(dummy_name, remote_talker->name);
  while (match_talker_name(dummy_name))
  {
    loopa++;
    strcpy(dummy_name, remote_talker->name);
    length = strlen(dummy_name);
    dummy_name[MAX_TALKER_NAME - 2] = 0;
    if (loopa > 9)
      dummy_name[MAX_TALKER_NAME - 3] = 0;
    if (loopa > 99)
      dummy_name[MAX_TALKER_NAME - 4] = 0;
    if (loopa > 999)
      dummy_name[MAX_TALKER_NAME - 5] = 0;

    sprintf(dummy_name, "%s%d", dummy_name, loopa);
  }
  strcpy(remote_talker->name, dummy_name);

  loopa = 0;
  strcpy(dummy_abbr, remote_talker->abbr);
  while (match_talker_abbr_absolute(dummy_abbr))
  {
    loopa++;
    strcpy(dummy_abbr, remote_talker->abbr);
    length = strlen(dummy_abbr);
    dummy_abbr[MAX_TALKER_ABBR - 2] = 0;
    if (loopa > 9)
      dummy_abbr[MAX_TALKER_ABBR - 3] = 0;
    if (loopa > 99)
      dummy_abbr[MAX_TALKER_ABBR - 4] = 0;
    if (loopa > 999)
      dummy_abbr[MAX_TALKER_ABBR - 5] = 0;

    sprintf(dummy_abbr, "%s%d", dummy_abbr, loopa);
  }
  strcpy(remote_talker->abbr, dummy_abbr);
  lower_case(remote_talker->abbr);

  return;
}

static void parse_hello(talker_list * remote_talker, char *str)
{
  char *oldstack;
  char *name, *abbr, *port_str;
  talker_list *known;
  int name_match;
  char dummy_name[MAX_TALKER_NAME + 1];

  oldstack = stack;

  if (!str || !*str)
  {
    /*Bad hello sent */
    tell_remote_talker(remote_talker, "%c", BAD_HELLO);

    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);
    return;
  }
  name = str;
  abbr = 0;
  port_str = 0;

  abbr = strchr(name, ':');
  if (abbr)
  {
    *abbr++ = '\0';
    if (*abbr)
    {
      port_str = strchr(abbr, ':');
      if (port_str)
	*port_str++ = '\0';
    }
  }
  if (!port_str || !*port_str || !*abbr || !*name)
  {
    /*Bad hello sent */
    tell_remote_talker(remote_talker, "%c", BAD_HELLO);

    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);

    return;
  }
  if (strlen(name) > MAX_TALKER_NAME - 1)
    name[MAX_TALKER_NAME - 1] = '\0';
  if (strlen(abbr) > MAX_TALKER_ABBR - 1)
    abbr[MAX_TALKER_ABBR - 1] = '\0';

  /*We have a hello message */

  /*Add the HELLO info into the struct we have */
  strcpy(remote_talker->name, name);
  strcpy(remote_talker->abbr, abbr);
  lower_case(remote_talker->abbr);
  remote_talker->port = ATOI(port_str);

  known = match_address(remote_talker);

  if (!known)
  {
    known = match_talker_name(remote_talker->name);

    name_match = 1;
    if (known)
    {
      /*If it is connected */
      if (known->fd < -1 || known->fd > 0 ||
          (time(0) - known->last_seen) < 7 * ONE_DAY ||
          known->port != remote_talker->port)
        known = 0;
      else
        name_match = 0;
    }
    else
      name_match = 0;

    while (!known && name_match)
    {
      strcpy(dummy_name, remote_talker->name);
      dummy_name[MAX_TALKER_NAME - 2] = 0;
      if (name_match > 9)
        dummy_name[MAX_TALKER_NAME - 3] = 0;
      if (name_match > 99)
        dummy_name[MAX_TALKER_NAME - 4] = 0;
      if (name_match > 999)
        dummy_name[MAX_TALKER_NAME - 5] = 0;

      sprintf(dummy_name, "%s%d", dummy_name, name_match);

      known = match_talker_name(dummy_name);

      if (known)
      {
        /*If it is connected */
        if (known->fd < -1 || known->fd > 0 ||
            (time(0) - known->last_seen) < 7 * ONE_DAY ||
            known->port != remote_talker->port)
        {
          known = 0;
          name_match++;
        }
      }
      else
        name_match = 0;
    }

    /*We have found a match that we will use now */
    if (known)
      /*Copy the address into the struct we have */
    {
      strcpy(known->addr, remote_talker->addr);
      known->num[0] = remote_talker->num[0];
      known->num[1] = remote_talker->num[1];
      known->num[2] = remote_talker->num[2];
      known->num[3] = remote_talker->num[3];
    }
  }

  if (!known)
  {
    /*Its a new talker requesting connection */
    if (match_talker_name(remote_talker->name) ||
	match_talker_abbr_absolute(remote_talker->abbr))
      make_unique(remote_talker);

    sprintf(oldstack, "%s:%s:%d.%d.%d.%d:%d:B", remote_talker->name,
	  remote_talker->abbr, remote_talker->num[0], remote_talker->num[1],
	    remote_talker->num[2], remote_talker->num[3],
	    remote_talker->port);
    stack = end_string(oldstack);
    add_new_talker(oldstack, 0);
    sprintf(oldstack, " A new talker '%s' tried to connect from '%s %d'. "
	    "Added it to the database as a banished talker.",
	    remote_talker->name, remote_talker->addr, remote_talker->port);
    stack = end_string(oldstack);
    tell_talker_su(oldstack);
    stack = oldstack;
    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);

    sync_talkers();

    return;
  }
  /*If we get here, known is pointing at the existing talker at that address
     that we know about. Thus, we work with 'known' and send a request
     to ask it to validate for us */

  /*If it is a banished talker, tell it so, and close its connection */
  if (known->fd == BARRED || known->fd == P_BARRED)
  {
    tell_remote_talker(remote_talker, "%c", BARRING_YOU);

    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);

    return;
  }
  request_validation(known);

  /*We have requested the validation, now we wait for its reply */
  return;

}

static void reply_validation(talker_list * known_talker)
{
  if (known_talker->fd < 1)
    return;

  tell_remote_talker(known_talker, "%c%d:%d", VALIDATION_IS,
		     portnumber, known_talker->validation);

  known_talker->last_seen = system_time;

  return;
}

static void parse_validation_request(talker_list * remote_talker, char *str)
{
  char *port_str, *validation_str;
  talker_list *known;

  if (!str || !*str)
  {
    log("intercom", "Empty validation string");
    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);

    return;
  }
  port_str = str;
  validation_str = strchr(port_str, ':');
  if (validation_str)
    *validation_str++ = '\0';

  if (!validation_str || !*validation_str || !*port_str)
  {
    log("intercom", "Incorrect validation string");
    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);

    return;
  }
  remote_talker->port = ATOI(port_str);

  known = match_address(remote_talker);

  if (!known)
  {
    /*This was a validation request from an unknown server. Kill it */
    log("intercom", "Unknown talker sent validation. Killed.");

    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);

    return;
  }
  known->validation = ATOI(validation_str);
  reply_validation(known);

}

static void check_validation(talker_list * remote_talker, char *str)
{
  char *port_str, *validation_str;
  talker_list *known;

  if (!str || !*str)
  {
    log("intercom", "Empty validation reply string");
    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);

    return;
  }
  port_str = str;
  validation_str = strchr(port_str, ':');
  if (validation_str)
    *validation_str++ = '\0';

  if (!validation_str || !*validation_str || !*port_str)
  {
    log("intercom", "Incorrect validation reply string");
    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);

    return;
  }
  remote_talker->port = ATOI(port_str);

  known = match_address(remote_talker);

  if (!known)
  {
    /*This was a validation request from an unknown server. Kill it */
    log("intercom", "Unknown talker sent reply validation. Killed.");

    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);

    return;
  }
  /*Check the validation is OK */
  if (abs(ATOI(validation_str)) != abs(known->validation))
  {
    /*The validation was bad, kill it */
    tell_remote_talker(remote_talker, "%c", BAD_VALIDATION);
    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);

    known->validation = 0;
    return;
  }
  /*The validation was SUCCESSFUL!! */
  known->fd = remote_talker->fd;
  known->net_stats.established = system_time;
  remote_talker->fd = -1;

  known->validation = abs(known->validation);
  known->last_seen = system_time;

  return;
}

static void tell_intercom_room(char *msg)
{
  send_to_talker("%c%s", INTERCOM_ROOM_MESSAGE, msg);

  return;
}

static void parse_in_user_tell_command(talker_list * remote_talker, char *str)
{
  char *job_str, *from_name, *msg = 0, *oldstack;
  job_list *this_job;

  oldstack = stack;

  if (!str || !*str)
  {
    log("intercom", "Bad user message sent");
    return;
  }
  job_str = str;
  str = strchr(job_str, ':');
  if (str)
    *str++ = '\0';
  else
  {
    log("intercom", "Bad user message sent");
    return;
  }

  from_name = set_name(str);

  if (from_name && *from_name)
  {
    msg = strchr(from_name, ':');
    if (msg)
      *msg++ = '\0';
  }
  if (!from_name || !*from_name || !job_str || !*job_str || !msg || !*msg)
  {
    log("intercom", "Bad user message sent");
    return;
  }
  this_job = make_job_entry();
  this_job->job_ref = ATOI(job_str);
  strcpy(this_job->originator, remote_talker->abbr);

  intercom_status |= INTERCOM_HIGHLIGHT | INTERCOM_PERSONAL_MSG;

  sprintf(oldstack, "%s@%s ", from_name, remote_talker->abbr);
  stack = strchr(oldstack, '\0');
  switch (msg[strlen(msg) - 1])
  {
    case '!':
      sprintf(stack, "exclaims to you, '%s'", msg);
      break;
    case '?':
      sprintf(stack, "asks of you, '%s'", msg);
      break;
    default:
      sprintf(stack, "tells you, '%s'", msg);
      break;
  }

  stack = end_string(oldstack);

  tell_personal_inc_ref(this_job, "%s", oldstack);

  stack = oldstack;

  intercom_status &= ~(INTERCOM_HIGHLIGHT | INTERCOM_PERSONAL_MSG);

  return;
}

static void parse_in_user_remote_command(talker_list * remote_talker, char *str)
{
  char *job_str, *from_name, *msg = 0;
  job_list *this_job;

  if (!str || !*str)
  {
    log("intercom", "Bad user message sent");
    return;
  }
  job_str = str;
  str = strchr(job_str, ':');
  if (str)
    *str++ = '\0';
  else
  {
    log("intercom", "Bad user message sent");
    return;
  }

  from_name = set_name(str);

  if (from_name && *from_name)
  {
    msg = strchr(from_name, ':');
    if (msg)
      *msg++ = '\0';
  }
  if (!from_name || !*from_name || !job_str || !*job_str || !msg || !*msg)
  {
    log("intercom", "Bad user message sent");
    return;
  }
  this_job = make_job_entry();
  this_job->job_ref = ATOI(job_str);
  strcpy(this_job->originator, remote_talker->abbr);

  intercom_status |= INTERCOM_HIGHLIGHT | INTERCOM_PERSONAL_MSG;

  tell_personal_inc_ref(this_job, "%s@%s %s", from_name, remote_talker->abbr,
			msg);

  intercom_status &= ~(INTERCOM_HIGHLIGHT | INTERCOM_PERSONAL_MSG);

  return;
}

static void parse_in_user_examine_command(talker_list * remote_talker, char *str)
{
  char *job_str, *name, *ptr;
  job_list *this_job;

  job_str = str;
  name = strchr(job_str, ':');
  if (name)
  {
    *name++ = '\0';
    ptr = strchr(name, ':');
    if (ptr)
      *ptr = '\0';
  }
  if (!job_str || !*job_str || !name || !*name)
    return;

  this_job = make_job_entry();
  this_job->job_ref = ATOI(job_str);
  strcpy(this_job->originator, remote_talker->abbr);

  send_to_talker("%c%c%ld:%s", USER_COMMAND, COMMAND_EXAMINE, this_job->job_id,
		 name);

  return;
}

static void parse_in_user_finger_command(talker_list * remote_talker, char *str)
{
  char *job_str, *name, *ptr;
  job_list *this_job;

  job_str = str;
  name = strchr(job_str, ':');
  if (name)
  {
    *name++ = '\0';
    ptr = strchr(name, ':');
    if (ptr)
      *ptr = '\0';
  }
  if (!job_str || !*job_str || !name || !*name)
    return;

  this_job = make_job_entry();
  this_job->job_ref = ATOI(job_str);
  strcpy(this_job->originator, remote_talker->abbr);

  send_to_talker("%c%c%ld:%s", USER_COMMAND, COMMAND_FINGER, this_job->job_id,
		 name);

  return;
}

static void parse_in_user_who_command(talker_list * remote_talker, char *str)
{
  char *job_str, *ptr;
  job_list *this_job;

  job_str = str;
  ptr = strchr(job_str, ':');
  if (ptr)
    *ptr++ = '\0';

  if (!*job_str)
    return;

  this_job = make_job_entry();
  this_job->job_ref = ATOI(job_str);
  strcpy(this_job->originator, remote_talker->abbr);

  send_to_talker("%c%c%ld", USER_COMMAND, COMMAND_WHO, this_job->job_id);

  return;
}

static void parse_in_user_lsu_command(talker_list * remote_talker, char *str)
{
  char *job_str, *ptr;
  job_list *this_job;

  job_str = str;
  ptr = strchr(job_str, ':');
  if (ptr)
    *ptr++ = '\0';

  if (!*job_str)
    return;

  this_job = make_job_entry();
  this_job->job_ref = ATOI(job_str);
  strcpy(this_job->originator, remote_talker->abbr);

  send_to_talker("%c%c%ld", USER_COMMAND, COMMAND_LSU, this_job->job_id);

  return;
}

static void parse_in_user_locate_command(talker_list * remote_talker, char *str)
{
  char *job_str, *name;
  job_list *this_job;

  job_str = str;
  name = strchr(job_str, ':');
  if (name)
    *name++ = '\0';

  if (!*job_str || !name || !*name)
    return;

  this_job = make_job_entry();
  this_job->job_ref = ATOI(job_str);
  strcpy(this_job->originator, remote_talker->abbr);

  send_to_talker("%c%c%ld:%s", USER_COMMAND, COMMAND_LOCATE, this_job->job_id,
		 name);

  return;
}

static void parse_in_user_idle_command(talker_list * remote_talker, char *str)
{
  char *job_str, *name, *ptr;
  job_list *this_job;

  job_str = str;
  name = strchr(job_str, ':');
  if (name)
  {
    *name++ = '\0';
    ptr = strchr(name, ':');
    if (ptr)
      *ptr = '\0';
  }
  if (!job_str || !*job_str || !name || !*name)
    return;

  this_job = make_job_entry();
  this_job->job_ref = ATOI(job_str);
  strcpy(this_job->originator, remote_talker->abbr);

  send_to_talker("%c%c%ld:%s", USER_COMMAND, COMMAND_IDLE, this_job->job_id,
		 name);

  return;
}

static void parse_in_user_say_command(talker_list * remote_talker, char *str)
{
  char *name, *msg = 0, *ptr, *oldstack;
  const char *method;

  name = str;
  if (name && *name)
  {
    msg = strchr(name, ':');
    if (msg)
      *msg++ = 0;
  }
  if (!name || !*name || !msg || !*msg)
  {
    log("intercom", "Invalid string in parse_in_user_say_command");
    return;
  }
  /*Build a reply string */
  oldstack = stack;

  ptr = (strchr(msg, 0)) - 1;

  switch (*ptr)
  {
    case '!':
      method = "exclaims";
      break;
    case '?':
      method = "asks";
      break;
    default:
      method = "says";
      break;
  }

  sprintf(oldstack, " %s@%s %s '%s'", name, remote_talker->abbr, method, msg);
  stack = end_string(oldstack);

  tell_intercom_room(oldstack);

  stack = oldstack;

  return;
}

static void parse_in_user_emote_command(talker_list * remote_talker, char *str)
{
  char *name, *msg = 0, *oldstack;

  name = str;
  if (name && *name)
  {
    msg = strchr(name, ':');
    if (msg)
      *msg++ = 0;
  }
  if (!name || !*name || !msg || !*msg)
  {
    log("intercom", "Invalid string in parse_in_user_emote_command");
    return;
  }
  /*Build a reply string */
  oldstack = stack;

  if (*msg == '\'')
    sprintf(oldstack, "%s@%s%s", name, remote_talker->abbr, msg);
  else
    sprintf(oldstack, "%s@%s %s", name, remote_talker->abbr, msg);

  stack = end_string(oldstack);

  tell_intercom_room(oldstack);

  stack = oldstack;

  return;
}

static void parse_in_user_command(talker_list * remote_talker, char *str)
{
  if (!str || !*str)
    return;

  switch (*str)
  {
    case COMMAND_WHO:
      parse_in_user_who_command(remote_talker, str + 1);
      break;
    case COMMAND_EXAMINE:
      parse_in_user_examine_command(remote_talker, str + 1);
      break;
    case COMMAND_FINGER:
      parse_in_user_finger_command(remote_talker, str + 1);
      break;
    case COMMAND_TELL:
      parse_in_user_tell_command(remote_talker, str + 1);
      break;
    case COMMAND_REMOTE:
      parse_in_user_remote_command(remote_talker, str + 1);
      break;
    case COMMAND_LSU:
      parse_in_user_lsu_command(remote_talker, str + 1);
      break;
    case COMMAND_LOCATE:
      parse_in_user_locate_command(remote_talker, str + 1);
      break;
    case COMMAND_IDLE:
      parse_in_user_idle_command(remote_talker, str + 1);
      break;
    case COMMAND_SAY:
      parse_in_user_say_command(remote_talker, str + 1);
      break;
    case COMMAND_EMOTE:
      parse_in_user_emote_command(remote_talker, str + 1);
      break;
  }

  return;
}

static void return_good_tell_command(talker_list * remote_talker,
				     job_list * this_job)
{
  switch (this_job->msg[strlen(this_job->msg) - 1])
  {
    case '?':
      tell_personal(" You ask of %s@%s, '%s'", this_job->target,
		    remote_talker->abbr, this_job->msg);
      break;
    case '!':
      tell_personal(" You exclaim to %s@%s, '%s'", this_job->target,
		    remote_talker->abbr, this_job->msg);
      break;
    default:
      tell_personal(" You tell %s@%s, '%s'", this_job->target,
		    remote_talker->abbr, this_job->msg);
      break;
  }

  return;
}

static void return_good_remote_command(talker_list * remote_talker,
				       job_list * this_job)
{
  if (*(this_job->msg) == 39)
    tell_personal(" You emote: '%s%s' to %s@%s", current_name,
		  this_job->msg, this_job->target, remote_talker->abbr);
  else
    tell_personal(" You emote: '%s %s' to %s@%s", current_name,
		  this_job->msg, this_job->target, remote_talker->abbr);

  return;
}

static void return_good_command(talker_list * remote_talker, job_list * this_job)
{
  switch (this_job->command_type)
  {
    case COMMAND_TELL:
      return_good_tell_command(remote_talker, this_job);
      break;
    case COMMAND_REMOTE:
      return_good_remote_command(remote_talker, this_job);
      break;
  }

  return;
}

static void send_who_reply_to_server(talker_list * remote_talker,
				     job_list * this_job, char *str)
{
  intercom_status |= INTERCOM_FORMAT_MSG;

  tell_personal("%s%s", str, remote_talker->name);

  intercom_status &= ~INTERCOM_FORMAT_MSG;

  return;
}

static void send_string_reply_to_server(talker_list * remote_talker,
					job_list * this_job, char *str)
{
  tell_personal("%s", str);

  return;
}

static void parse_reply_from_remote(talker_list * remote_talker, char *str)
{
  job_list *this_job;
  char *ptr = 0, *name;
  char blank_field[2];

  blank_field[0] = 'x';
  blank_field[1] = '\0';

  if (!str || !*str || !*(str + 1))
  {
    log("intercom", "Bad reply received for remote command result.");
    return;
  }
  name = strchr(str + 1, ':');
  if (name)
  {
    *name++ = '\0';
    if (*name)
    {
      ptr = strchr(name, ':');
      if (ptr)
	*ptr++ = 0;
    }
  }
  if (!name || !*name)
  {
    log("error", "Invalid format in parse_reply_from_remote");
    return;
  }
  if (!ptr || !*ptr)
    ptr = blank_field;

  this_job = return_job(str + 1);

  if (!this_job)
    /*The job must have expired */
    return;

  if (this_job->command_type != COMMAND_LOCATE)
    strcpy(this_job->target, name);

  strcpy(current_name, this_job->sender);

  /*Here we have the job detailing what was sent by who. Build a reply and
     finish */
  switch (*str)
  {
    case COMMAND_WHO:
      send_who_reply_to_server(remote_talker, this_job, ptr);
      break;

    case COMMAND_EXAMINE:
    case COMMAND_FINGER:
    case COMMAND_LSU:
    case COMMAND_IDLE:
      send_string_reply_to_server(remote_talker, this_job, ptr);
      break;

    case COMMAND_LOCATE:
      strcpy(current_name, this_job->sender);
      if (*name == COMMAND_SUCCESSFUL)
	tell_personal(" Talker %s (%s) reports %s is logged in there.",
		      remote_talker->name, remote_talker->abbr,
		      this_job->target);

      current_name[0] = 0;

      /*Now return so it doenst free the job here cos we have more replies
         possibly stuill to come */
      return;

      break;

    case NO_SUCH_PLAYER:
      strcpy(current_name, this_job->sender);
      if (this_job->command_type == COMMAND_FINGER)
	tell_personal(" No such person in saved files.");
      else
	tell_personal(" No-one of the name '%s' on %s at the moment.",
		      this_job->target, remote_talker->name);
      break;

    case NAME_IGNORED:
      strcpy(current_name, this_job->sender);
      tell_personal(" %s is ignoring you.", this_job->target);
      break;

    case NAME_BANISHED:
      strcpy(current_name, this_job->sender);
      tell_personal(" Your name is banished on %s.", remote_talker->name);
      break;

    case NAME_BLOCKED:
      strcpy(current_name, this_job->sender);
      tell_personal(" %s is blocking you.", this_job->target);
      break;

    case TALKER_IGNORED:
      strcpy(current_name, this_job->sender);
      tell_personal(" %s is ignoring %s.", this_job->target,
		    get_config_msg("talker_name"));
      break;

    case TALKER_BLOCKED:
      strcpy(current_name, this_job->sender);
      tell_personal(" %s is blocking %s.", this_job->target,
		    get_config_msg("talker_name"));
      break;

    case COMMAND_SUCCESSFUL:
      return_good_command(remote_talker, this_job);
      break;

    default:
      strcpy(current_name, this_job->sender);
      tell_personal(" Unknown reply from remote intercom.");
      break;

  }

  current_name[0] = '\0';
  free_job(this_job);

  return;
}

static void get_remote_list(talker_list * remote_talker, char *str)
{
  char *name, *addr, *abbr, *port_str, *ptr, *oldstack;
  int port, all_num, dots, num[4];
  struct hostent *hp;
  struct in_addr inet_address;
  char numeric_string[MAX_INET_ADDR];
  talker_list *scan;

  name = str;
  abbr = 0;
  addr = 0;
  port_str = 0;
  if (name && *name)
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
	    port_str = strchr(addr, ':');
	    if (port_str)
	      *port_str++ = '\0';
	  }
	}
      }
    }
  }
  if (!port_str || !*port_str || !*addr || !*abbr || !*name || !name)
    return;

  if (match_talker_name(name) || match_talker_abbr_absolute(name) ||
      match_talker_name(abbr) || match_talker_abbr_absolute(abbr))
    return;

  if (!strcasecmp(name, get_config_msg("talker_name")) ||
      !strcasecmp(abbr, get_config_msg("intercom_abbr")))
    return;

  /*We need to check its not a duplicate name */
  /*Test whether the address we have is all in numeric */
  dots = 0;
  all_num = 1;
  ptr = addr;
  while (all_num && *ptr)
  {
    if (!isdigit(*ptr) && *ptr != '.')
      all_num = 0;
    if (*ptr++ == '.')
      dots++;
  }

  if (all_num)
  {
    if (dots != 3)
      return;
    strncpy(numeric_string, addr, MAX_INET_ADDR - 1);
    numeric_string[MAX_INET_ADDR - 1] = 0;
  }
  else
  {
    /*Its an alpha address, we need to convert */
    hp = gethostbyname(addr);
    if (!hp)
      return;
    memcpy((char *) &inet_address, hp->h_addr, sizeof(struct in_addr));
    strcpy(numeric_string, inet_ntoa(inet_address));
  }

  /*We have a dot notation address in numeric_string, so lets
     paste this into the struct */
  sscanf(numeric_string, "%d.%d.%d.%d",
	 &(num[0]), &(num[1]), &(num[2]), &(num[3]));
  port = ATOI(port_str);

  /*We have an address we can use now. */
  /*Check it against all other addresses.... */
  scan = talker_list_anchor;
  while (scan->next)
  {
    scan = scan->next;
    if (((scan->num[0] == num[0] &&
	  scan->num[1] == num[1] &&
	  scan->num[2] == num[2] &&
	  scan->num[3] == num[3]) ||
	 !strcasecmp(scan->addr, addr)) &&
	scan->port == port)
      /*Its the same site, so we dont want it */
      return;
  }

  /*This is a new talker we DO want, so add it */
  oldstack = stack;
  sprintf(oldstack, "%s:%s:%d.%d.%d.%d:%s:B", name, abbr, num[0],
	  num[1], num[2], num[3], port_str);
  stack = end_string(oldstack);
  add_new_talker(oldstack, 0);

  sprintf(oldstack, "Talker '%s' just informed us of talker '%s'. Added to the"
	  " database as a banished talker.", remote_talker->name, name);
  stack = end_string(oldstack);
  tell_talker_su(oldstack);
  stack = oldstack;

  sync_talkers();

  return;

}

static void send_server_list(talker_list * remote_server)
{
  talker_list *scan;
  char address[MAX_INET_ADDR];

  scan = talker_list_anchor;

  while (scan->next)
  {
    scan = scan->next;
    if (scan->fd > 0 && scan->validation > 0 && !(scan->flags & WAITING_CONNECT))
    {
      if (scan->num[0] == 0 && scan->num[1] == 0 &&
	  scan->num[2] == 0 && scan->num[3] == 0)
	strcpy(address, scan->addr);
      else
	sprintf(address, "%d.%d.%d.%d", scan->num[0], scan->num[1],
		scan->num[2], scan->num[3]);

      tell_remote_talker(remote_server, "%c%s:%s:%s:%d", I_KNOW_OF,
			 scan->name, scan->abbr, address, scan->port);
    }
  }

  return;
}

static void remote_has_barred(talker_list * remote_talker)
{
  I_SHUTDOWN(remote_talker->fd, 2);

  do_close(remote_talker);

  remote_talker->fd = BARRED_REMOTE;
}

static void do_room_look(talker_list * remote_talker, char *str)
{
  job_list *this_job;

  this_job = make_job_entry();
  this_job->job_ref = ATOI(str);
  strcpy(this_job->originator, remote_talker->abbr);

  send_to_talker("%c%ld", INTERCOM_ROOM_LOOK, this_job->job_id);

  return;
}

static void return_room_look(talker_list * remote_talker, char *str)
{
  char *user_list = 0, *count_str = 0, *job_str;
  job_list *this_job;

  /*Format of str is job_id:count:list */

  job_str = str;
  if (job_str && *job_str)
  {
    count_str = strchr(job_str, ':');
    if (count_str)
    {
      *count_str++ = 0;
      if (*count_str)
      {
	user_list = strchr(count_str, ':');
	if (user_list)
	  *user_list++ = 0;
      }
    }
  }
  if (!user_list || !*user_list || !*count_str || !job_str || !*job_str)
  {
    log("intercom", "Bad string in return_room_look");
    return;
  }
  this_job = return_job(job_str);

  strcpy(current_name, this_job->sender);

  tell_personal(" Talker '%s' has %s user%s here: %s",
	    remote_talker->name, count_str, ATOI(count_str) == 1 ? "" : "s",
		user_list);

  return;
}

static void informed_move(talker_list * remote_talker, char *str)
{
  int dots, all_num;
  char *site, *port = 0, *ptr, numeric_string[MAX_INET_ADDR], *oldstack;
  struct hostent *hp;
  struct in_addr inet_address;
  struct sockaddr_in sa;

  /*Split this into site and port */
  site = str;
  if (site && *site)
  {
    port = strchr(site, ':');
    if (port)
      *port++ = 0;
  }
  if (!port || !*port || !site || !*site)
  {
    log("intercom", "Bad string from informed_move");
    return;
  }
  I_SHUTDOWN(remote_talker->fd, 2);
  do_close(remote_talker);

  /*Test whether the address we have is all in numeric */
  dots = 0;
  all_num = 1;
  ptr = site;
  while (all_num && *ptr)
  {
    if (!isdigit(*ptr) && *ptr != '.')
      all_num = 0;
    if (*ptr++ == '.')
      dots++;
  }

  if (all_num)
  {
    if (dots != 3 || !strcmp(site, "0.0.0.0"))
      return;
    strncpy(numeric_string, site, MAX_INET_ADDR - 1);
    numeric_string[MAX_INET_ADDR - 1] = 0;

    /*Now see if we can lookup the name address */
    hp = 0;
    inet_address.s_addr = inet_addr(numeric_string);

    if (inet_address.s_addr != -1)
    {
      sa.sin_addr = inet_address;
      sa.sin_family = AF_INET;

      /*Do the name  lookup */
      hp = gethostbyaddr((char *) &(sa.sin_addr.s_addr),
			 sizeof(sa.sin_addr.s_addr), AF_INET);
    }
    /*If we found it, use it */
    if (hp)
      strcpy(remote_talker->addr, (const char *) hp->h_name);
    else
      strcpy(remote_talker->addr, inet_ntoa(sa.sin_addr));

  }
  else
  {
    /*Its an alpha address, we need to convert */
    hp = gethostbyname(site);
    if (!hp)
      return;
    else
    {
      memcpy((char *) &inet_address, hp->h_addr, sizeof(struct in_addr));
      strcpy(numeric_string, inet_ntoa(inet_address));
    }
  }

  /*We have a dot notation address in numeric_string, so lets
     paste this into the struct */
  sscanf(numeric_string, "%d.%d.%d.%d",
	 &(remote_talker->num[0]), &(remote_talker->num[1]),
	 &(remote_talker->num[2]), &(remote_talker->num[3]));

  remote_talker->port = ATOI(port);

  sync_talkers();

  /*Announce the change */
  oldstack = stack;

  sprintf(oldstack, "Talker '%s' just changed its address to %s %d",
	  remote_talker->name, remote_talker->addr, remote_talker->port);

  stack = end_string(oldstack);
  tell_talker_su(oldstack);
  stack = oldstack;

  return;
}

static void parse_remote_talker_input(talker_list * remote_talker, int validated)
{
  char c;
  int chars_left;
  char *oldstack, *ptr;
  int parse_ok = 0;
  packet *this_packet;

  /*Firstly, if we were waiting for a connect, this means it failed if we get
     here */
  if (remote_talker->flags & WAITING_CONNECT)
  {
    do_close(remote_talker);

    remote_talker->flags &= ~WAITING_CONNECT;
    remote_talker->flags &= ~HELLO_AFTER_CONNECT;

    return;
  }
  if (ioctl(remote_talker->fd, FIONREAD, &chars_left) == -1)
  {
    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);

    log("intercom", "PANIC on FIONREAD on remote_talker->fd.");

    return;
  }
  if (!chars_left)
  {
    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);

    return;
  }
  this_packet = add_packet_to_talker(remote_talker);
  oldstack = stack;

  c = (char) (END_MESSAGE - 1);
  while (chars_left && c != (char) END_MESSAGE)
  {
    chars_left--;
    remote_talker->net_stats.chars_in++;
    total_net.chars_in++;
    if (read(remote_talker->fd, &c, 1) != 1)
    {
      I_SHUTDOWN(remote_talker->fd, 2);
      do_close(remote_talker);

      return;
    }
    if (c != (char) END_MESSAGE)
    {
      this_packet->data[this_packet->length] = c;
      this_packet->length++;
      if (this_packet->length >= MAX_PACKET)
	this_packet = add_packet_to_talker(remote_talker);
    }
    else
      parse_ok = 1;
  }

  if (intercom_status & BAR_ALL_CONNECTIONS)
  {
    stack = oldstack;
    if (chars_left > 0)
      parse_remote_talker_input(remote_talker, validated);
    return;
  }
  if (!parse_ok)
    return;

  this_packet = remote_talker->packet_anchor;
  strcpy(oldstack, this_packet->data);
  stack = strchr(oldstack, 0);

  while (this_packet->next)
  {
    this_packet = this_packet->next;
    strcpy(stack, this_packet->data);
    stack = strchr(stack, 0);
  }
  stack++;

  ptr = oldstack;

  /*HERE note we HAVE seen this talker recently */
  remote_talker->last_seen = system_time;

  if (*ptr)
  {
    if (validated)
    {
      switch (*ptr)
      {
	case USER_COMMAND:
	  parse_in_user_command(remote_talker, ptr + 1);
	  break;
	case REPLY_IS:
	  parse_reply_from_remote(remote_talker, ptr + 1);
	  break;
	case REQUEST_SERVER_LIST:
	  send_server_list(remote_talker);
	  break;
	case I_KNOW_OF:
	  get_remote_list(remote_talker, ptr + 1);
	  break;
	case BARRING_YOU:
	  remote_has_barred(remote_talker);
	  break;
	case INTERCOM_ROOM_LOOK:
	  do_room_look(remote_talker, ptr + 1);
	  break;
	case INTERCOM_ROOM_LIST:
	  return_room_look(remote_talker, ptr + 1);
	  break;
	case WE_ARE_MOVING:
	  informed_move(remote_talker, ptr + 1);
	  break;
      }
    }
    else
    {
      switch (*ptr)
      {
	case HELLO_I_AM:
	  parse_hello(remote_talker, ptr + 1);
	  break;
	case REQUEST_VALIDATION_AS:
	  parse_validation_request(remote_talker, ptr + 1);
	  break;
	case VALIDATION_IS:
	  check_validation(remote_talker, ptr + 1);
	  break;
	default:
	  rerequest_validation(remote_talker);
      }
    }
  }
  stack = oldstack;

  free_talker_packets(remote_talker);

  if (chars_left > 0)
    parse_remote_talker_input(remote_talker, validated);

  return;
}

static int get_new_talker_connection(void)
{
#ifdef REDHAT5
  unsigned int incoming_length;
#else /* REDHAT5 */
  int incoming_length;
#endif /* !REDHAT5 */
  struct sockaddr_in sa;
  int new_fd, dummy = 0;
  talker_list *new_var, *scan;
  struct hostent *hp;
  struct linger lingerval;

  incoming_length = sizeof(sa);
  new_fd = accept(inet_fd, (struct sockaddr *) &sa, &incoming_length);

  if (new_fd < 0)
  {
    log("intercom", "Failed to accept on inet_fd");
    return -1;
  }
  if (ioctl(new_fd, FIONBIO, &dummy) < 0)
  {
    log("intercom", "Failed to set non-blocking on incoming"
	" remote talker.");
    I_SHUTDOWN(new_fd, 2);
    close(new_fd);
    return 0;
  }
  dummy = 1;
  setsockopt(new_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &dummy,
	     sizeof(dummy));

  lingerval.l_onoff = 1;
  lingerval.l_linger = 0;
  setsockopt(new_fd, SOL_SOCKET, SO_LINGER, (struct linger *) &lingerval,
	     sizeof(struct linger));

  /*if the intercom is closed, bar all connection attempts */
  if (intercom_status & BAR_ALL_CONNECTIONS)
  {
    I_SHUTDOWN(new_fd, 2);
    close(new_fd);
    return 0;
  }
  /*We have a new connection, so make a new struct for it */
  scan = validation_anchor;
  while (scan->next)
    scan = scan->next;

  new_var = (talker_list *) calloc(1, sizeof(talker_list));
  scan->next = new_var;
  new_var->fd = new_fd;
  new_var->timeout = system_time + 240;

  /*Extract their address from the socket info */
  sscanf(inet_ntoa(sa.sin_addr), "%d.%d.%d.%d",
	 &(new_var->num[0]), &(new_var->num[1]), &(new_var->num[2]),
	 &(new_var->num[3]));

  /*If possible, get the name address too */
  hp = gethostbyaddr((char *) &(sa.sin_addr.s_addr),
		     sizeof(sa.sin_addr.s_addr), AF_INET);
  if (hp)
    strcpy(new_var->addr, (const char *) hp->h_name);
  else
    strcpy(new_var->addr, inet_ntoa(sa.sin_addr));

  /*We have as much as we can get now. Lets wait for the HELLO */

  return talker_fd;
}

static int create_main_intercom_socket(const char *path)
{
  struct sockaddr_un sa;

  /*Unlink the path in case its still there */
  unlink(path);

  unix_fd = socket(PF_UNIX, SOCK_STREAM, 0);

  if (unix_fd < 0)
  {
    log("intercom", "Failed to create unix socket.");
    return -1;
  }
  sa.sun_family = AF_UNIX;
  strcpy(sa.sun_path, path);

  if (bind(unix_fd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
  {
    log("intercom", "Failed to bind to unix socket.");
    return -1;
  }
  if (listen(unix_fd, 1) < 0)
  {
    log("intercom", "Failed to listen at unix socket");
    I_SHUTDOWN(unix_fd, 2);
    close(unix_fd);
    unix_fd = -1;
    unlink(path);
    return -1;
  }
  log("intercom", "Unix socket bound and listening.");

  return unix_fd;
}

static int get_unix_fd_input(void)
{
#ifdef REDHAT5
  unsigned int incoming_length;
#else /* REDHAT5 */
  int incoming_length;
#endif /* !REDHAT5 */
  struct sockaddr_un sa;

  incoming_length = sizeof(sa);
  talker_fd = accept(unix_fd, (struct sockaddr *) &sa, &incoming_length);

  if (talker_fd < 0)
  {
    log("intercom", "Failed to accept on unix_fd");
    return -1;
  }
  log("intercom", "Connected intercom to talker");

  server_net.established = system_time;

  return talker_fd;
}

void send_to_talker(const char *fmt,...)
{
  va_list varlist;
  char *oldstack;

  oldstack = stack;

  if (!fmt || !*fmt)
  {
    log("intercom", "Empty string in send_to_talker");
    return;
  }
  va_start(varlist, fmt);
  vsprintf(stack, fmt, varlist);
  va_end(varlist);

  if (!*oldstack)
  {
    log("intercom", "Tried to send null string to server.");
    return;
  }
  stack = strchr(oldstack, '\0');
  *stack++ = (char) END_MESSAGE;
  *stack++ = '\0';

  server_net.chars_out += strlen(oldstack);
  server_net.packets_out++;

  if (write(talker_fd, oldstack, strlen(oldstack)) == -1)
    /*Write to the talker failed. Die controlled */
    closedown = 1;

  stack = oldstack;

  return;
}

static void request_port_from_talker(void)
{
  /*Send message to the talker requesting info on what port to run on */
  log("intercom", "Requesting port number from talker");

  send_to_talker("%c", REQUEST_PORTNUMBER);

  return;
}

int tell_remote_talker(talker_list * remote_talker, const char *fmt,...)
{
  va_list varlist;
  char *oldstack, *ptr;
  int fail = 0;

  if (!fmt || !*fmt)
  {
    log("intercom", "Tried to write NULL to remote talker");
    return -1;
  }
  if (remote_talker->fd < 1)
    return -1;

  oldstack = stack;

  va_start(varlist, fmt);
  vsprintf(oldstack, fmt, varlist);
  va_end(varlist);

  stack = strchr(oldstack, 0);
  *stack++ = (char) END_MESSAGE;
  *stack++ = '\0';

  ptr = oldstack;
  while (*ptr && (strchr(ptr, 0) - ptr) >= MAX_PACKET)
  {
    remote_talker->net_stats.chars_out += MAX_PACKET;
    remote_talker->net_stats.packets_out++;
    total_net.chars_out += MAX_PACKET;
    total_net.packets_out++;
    if (write(remote_talker->fd, ptr, MAX_PACKET) == -1)
      fail = 1;
    ptr += MAX_PACKET;
  }

  if (*ptr)
  {
    remote_talker->net_stats.chars_out += (strchr(ptr, 0) - ptr);
    remote_talker->net_stats.packets_out++;
    total_net.chars_out += (strchr(ptr, 0) - ptr);
    total_net.packets_out++;
    if (write(remote_talker->fd, ptr, (strchr(ptr, 0) - ptr)) == -1)
      fail = 1;
  }
  /*If any writes failed, dump the connection */
  if (fail)
  {
    I_SHUTDOWN(remote_talker->fd, 2);
    do_close(remote_talker);
  }
  stack = oldstack;

  return 1;
}

static void send_hello(talker_list * remote_talker)
{
  char *oldstack = stack;

  stack += sprintf(stack, "%c%s:", HELLO_I_AM, get_config_msg("talker_name"));

  stack += sprintf(stack, "%s:%d", get_config_msg("intercom_abbr"),
		   portnumber);
  stack = end_string(stack);

  tell_remote_talker(remote_talker, oldstack);

  stack = oldstack;
  return;
}

int connect_new_talker(talker_list * new_link)
{
  char numeric_string[MAX_INET_ADDR];
  struct in_addr inet_address;
  struct sockaddr_in sa;
  int flags, dummy = 1;
  struct linger lingerval;

  if (new_link->flags & WAITING_CONNECT)
    new_link->flags &= ~WAITING_CONNECT;
  if (new_link->flags & HELLO_AFTER_CONNECT)
    new_link->flags &= ~HELLO_AFTER_CONNECT;
  if (new_link->flags & VALIDATE_AFTER_CONNECT)
    new_link->flags &= ~VALIDATE_AFTER_CONNECT;

  if (new_link->fd == BARRED || new_link->fd == P_BARRED || new_link->flags & INVIS)
    return 0;

  if (inet_fd < 1)
  {
    new_link->fd = -1;
    return 0;
  }
  if (new_link->fd > 0)
  {
    I_SHUTDOWN(new_link->fd, 2);
    do_close(new_link);
  }
  if (!(new_link->num[0] || new_link->num[1] || new_link->num[2] ||
	new_link->num[3]))
  {
    new_link->fd = ERROR_FD;
    return 0;
  }
  if (intercom_status & INTERCOM_BOOTING)
  {
    new_link->fd = NO_CONNECT_TRIED;
    return 0;
  }
  sprintf(numeric_string, "%d.%d.%d.%d", new_link->num[0], new_link->num[1],
	  new_link->num[2], new_link->num[3]);

  inet_address.s_addr = inet_addr(numeric_string);

  if (inet_address.s_addr == -1)
  {
    new_link->fd = -1;
    return 0;
  }
  new_link->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (new_link->fd < 0)
  {
    log("intercom", "Failed to create socket for remote link");
    return 0;
  }
  dummy = 1;
  setsockopt(new_link->fd, SOL_SOCKET, SO_REUSEADDR, (char *) &dummy,
	     sizeof(dummy));
  lingerval.l_onoff = 1;
  lingerval.l_linger = 0;
  setsockopt(new_link->fd, SOL_SOCKET, SO_LINGER, (struct linger *) &lingerval,
	     sizeof(struct linger));


  sa.sin_family = AF_INET;
  sa.sin_port = htons((new_link->port) - 1);
  sa.sin_addr = inet_address;

  ioctl(new_link->fd, FIONBIO, &dummy);
  flags = 0;

  send_to_talker("%c", STARTING_CONNECT);

  /*Now connect to the remote server */
  if (connect(new_link->fd, (struct sockaddr *) &sa, sizeof(sa)) == 0)
  {
    /*Connect was successful */

    /*Let the server know we have finished */
    send_to_talker("%c", PING);

    /*Here, we have connected */
    new_link->net_stats.established = system_time;

    return 1;
  }
  else if (errno == EINPROGRESS)
  {
    /*The connection needs more time.... */
    new_link->flags |= WAITING_CONNECT;

    return 0;
  }
  /*It failed. */
  do_close(new_link);

  send_to_talker("%c", PING);
  return 0;

}

static void create_inet_socket(char *str)
{
  struct sockaddr_in main_socket;
  int dummy = 0;
  char hostname[101], *oldstack;
  struct hostent *hp;
  talker_list *scan;
  int local_errno;
  struct linger lingerval;

  oldstack = stack;

  if (!*str)
  {
    log("intercom", "Talker sent zero length portnumber. Re-requesting.");
    request_port_from_talker();
    return;
  }
  portnumber = ATOI(str);
  if (portnumber < 1)
  {
    sprintf(oldstack, "Talker sent port under 1 (%d), Re-requesting.",
	    portnumber);
    stack = end_string(oldstack);
    log("intercom", oldstack);
    stack = oldstack;
    request_port_from_talker();
    return;
  }
  /*We have a portnumber and its over 0 */
  bzero((char *) &main_socket, sizeof(struct sockaddr_in));
  gethostname(hostname, 100);

  hp = gethostbyname(hostname);
  if (hp == NULL)
  {
    log("intercom", "Host machine does not exist");
    return;
  }
  main_socket.sin_family = hp->h_addrtype;
  main_socket.sin_port = htons(portnumber);
  /*Add 1 to portnumber, as this will now refer to the main talkers port */
  portnumber++;

  inet_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (inet_fd < 0)
  {
    log("intercom", "Couldnt grab inet_fd socket");
    inet_fd = -1;
    return;
  }
  dummy = 1;
  if (setsockopt(inet_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &dummy, sizeof(dummy))
      < 0)
    log("intercom", "Couldnt set reuse address on inet_fd, ignoring.");
  lingerval.l_onoff = 1;
  lingerval.l_linger = 0;
  setsockopt(inet_fd, SOL_SOCKET, SO_LINGER, (struct linger *) &lingerval,
	     sizeof(struct linger));




  if (ioctl(inet_fd, FIONBIO, &dummy) < 0)
  {
    log("intercom", "Couldnt set non-blocking on inet_fd.");
    close(inet_fd);
    inet_fd = -1;
    return;
  }
  if (bind(inet_fd, (struct sockaddr *) &main_socket, sizeof(main_socket)) < 0)
  {
    local_errno = errno;
    log("intercom", "Couldnt bind inet_fd to port");
    switch (local_errno)
    {
      case EBADF:
	log("intercom", "Bad file descriptor");
	break;
      case EINVAL:
	log("intercom", "Already bound.");
	break;
      case EACCES:
	log("intercom", "Privillaged port");
	break;
      case EADDRINUSE:
	log("intercom", "Address in use.");
	break;
      default:
	log("intercom", "errno failed.");
	break;
    }
    I_SHUTDOWN(inet_fd, 2);
    close(inet_fd);
    inet_fd = -1;
    close_all_sockets();
    return;
  }
  if (listen(inet_fd, 5) < 0)
  {
    log("intercom", "Failed on listen. Closing inet_fd.");
    I_SHUTDOWN(inet_fd, 2);
    close(inet_fd);
    inet_fd = -1;
    close_all_sockets();
    return;
  }
  sprintf(oldstack, "inet_fd bound and listening to port %d", portnumber - 1);
  stack = end_string(oldstack);
  log("intercom", oldstack);
  stack = oldstack;

  total_net.established = system_time;

  scan = talker_list_anchor;

  intercom_status |= INTERCOM_BOOTING;

  while (scan->next)
  {
    scan = scan->next;

    /*Connect the talker */
    if (connect_new_talker(scan))
      /*Send a HELLO */
      send_hello(scan);
    else
      scan->flags |= HELLO_AFTER_CONNECT;
  }

  intercom_status &= ~INTERCOM_BOOTING;

  return;
}

void tell_personal_inc_ref(job_list * this_job, const char *fmt,...)
{
  char *oldstack, *str;
  va_list varlist;

  if (!fmt || !*fmt)
    return;

  if (current_name[0] == '\0')
  {
    log("intercom", "No name to send personal message to.");
    return;
  }
  oldstack = stack;

  va_start(varlist, fmt);
  vsprintf(stack, fmt, varlist);
  va_end(varlist);

  stack = end_string(oldstack);
  str = stack;

  if (intercom_status & INTERCOM_HIGHLIGHT)
    *stack++ = HIGHLIGHT_RETURN;
  if (intercom_status & INTERCOM_PERSONAL_MSG)
    *stack++ = PERSONAL_MESSAGE_TAG;

  sprintf(stack, "%c%ld:%s:%s", PERSONAL_MESSAGE_AND_RETURN, this_job->job_id,
	  current_name, oldstack);

  stack = end_string(str);
  send_to_talker("%s", str);
  stack = oldstack;

  return;
}

void tell_personal(const char *fmt,...)
{
  char *oldstack, *str, *ptr;
  va_list varlist;

  if (!fmt || !*fmt)
    return;

  if (current_name[0] == '\0')
  {
    log("intercom", "No name to send personal message to.");
    return;
  }
  oldstack = stack;

  va_start(varlist, fmt);
  vsprintf(stack, fmt, varlist);
  va_end(varlist);

  stack = end_string(oldstack);
  str = stack;

  if (intercom_status & INTERCOM_HIGHLIGHT)
    *stack++ = HIGHLIGHT_RETURN;
  if (intercom_status & INTERCOM_PERSONAL_MSG)
    *stack++ = PERSONAL_MESSAGE_TAG;
  if (intercom_status & INTERCOM_FORMAT_MSG)
    *stack++ = FORMAT_MESSAGE_TAG;

  sprintf(stack, "%c%s:", PERSONAL_MESSAGE, current_name);
  stack = strchr(stack, '\0');
  ptr = oldstack;
  while (*ptr)
  {
    if (*ptr != '\r')
      *stack++ = *ptr++;
    else
      ptr++;
  }
  *stack++ = '\0';

  send_to_talker("%s", str);
  stack = oldstack;

  return;
}


void tell_talker_su(char *str)
{
  if (!str || !*str)
    return;

  send_to_talker("%c%s", SU_MESSAGE, str);

  return;
}

int add_new_talker(char *str, int verbose)
{
  talker_list *scan, *new_var, *prev;
  char *name, *abbr, *addr, *port, *status, *oldstack, *ptr, *last_seen;
  char numeric_string[MAX_INET_ADDR];
  int all_num, dots;
  struct hostent *hp;
  struct in_addr inet_address;
  struct sockaddr_in sa;

  ptr = str;
  while (*ptr)
  {
    if (iscntrl(*ptr))
      *ptr='.';
    ptr++;
  }

  oldstack = stack;

  name = str;
  abbr = 0;
  addr = 0;
  port = 0;
  status = 0;
  last_seen = 0;

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
	    {
	      *port++ = '\0';
	      if (*port)
	      {
		status = strchr(port, ':');
		if (status)
		{
		  *status++ = '\0';
		  if (*status)
		  {
		    last_seen = strchr(status, ':');
		    if (last_seen)
		      *last_seen++ = '\0';
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }
  if (!status || !*status || !*port || !*addr || !*abbr || !name || !*name)
  {
    return 0;
  }
  new_var = (talker_list *) calloc(1, sizeof(talker_list));

  if (strlen(name) > MAX_TALKER_NAME - 1)
    name[MAX_TALKER_NAME - 1] = '\0';
  if (strlen(abbr) > MAX_TALKER_ABBR - 1)
    abbr[MAX_TALKER_ABBR - 1] = '\0';
  if (strlen(addr) > (MAX_INET_ADDR - 1))
    addr[MAX_INET_ADDR - 1] = '\0';

  strcpy(new_var->name, name);
  strcpy(new_var->abbr, abbr);
  lower_case(new_var->abbr);
  strcpy(new_var->addr, addr);
  new_var->port = ATOI(port);

  if (last_seen && *last_seen)
    new_var->last_seen = ATOI(last_seen);
  else
    new_var->last_seen = system_time;

  if (*status == 'B')
    new_var->fd = P_BARRED;
  if (*status == 'I')
  {
    new_var->fd = P_BARRED;
    new_var->flags |= INVIS;
  }
  /*TIMEOUTS */

  /*If invisible and not seen for 6 months */
  if (new_var->fd == P_BARRED)
  {
    if (new_var->flags & INVIS && system_time > new_var->last_seen + (14515200))
    {
      FREE(new_var);
      return 0;
    }
  }
  else
  {
    /*If visible and not seen for 3 months */
    if (system_time > new_var->last_seen + (7257600))
    {
      FREE(new_var);
      return 0;
    }
  }

  make_unique(new_var);

  /*Add it to the list */
  scan = talker_list_anchor;
  while (scan->next && strcmp(scan->next->abbr, new_var->abbr) < 0)
    scan = scan->next;

  new_var->next = scan->next;
  scan->next = new_var;
  prev = scan;

  /*Test whether the address we have is all in numeric */
  dots = 0;
  all_num = 1;
  ptr = new_var->addr;
  while (all_num && *ptr)
  {
    if (!isdigit(*ptr) && *ptr != '.')
      all_num = 0;
    if (*ptr++ == '.')
      dots++;
  }

  if (all_num)
  {
    if (dots != 3 || !strcmp(new_var->addr, "0.0.0.0"))
    {
      prev->next = new_var->next;
      FREE(new_var);
      if (verbose > 0)
	tell_personal(" Invalid address, '%s' supplied.", addr);
      return 0;
    }
    strncpy(numeric_string, new_var->addr, MAX_INET_ADDR - 1);
    numeric_string[MAX_INET_ADDR - 1] = 0;

    /*Now see if we can lookup the name address */
    hp = 0;
    inet_address.s_addr = inet_addr(numeric_string);

    if (inet_address.s_addr != -1)
    {
      sa.sin_addr = inet_address;
      sa.sin_family = AF_INET;

      /*Do the name  lookup */
      hp = gethostbyaddr((char *) &(sa.sin_addr.s_addr),
			 sizeof(sa.sin_addr.s_addr), AF_INET);
    }
    /*If we found it, use it */
    if (hp)
      strcpy(new_var->addr, (const char *) hp->h_name);
    else
      strcpy(new_var->addr, inet_ntoa(sa.sin_addr));

  }
  else
  {
    /*Its an alpha address, we need to convert */
    hp = gethostbyname(new_var->addr);
    if (!hp)
    {
      new_var->fd = -1;
      if (verbose > 0)
	tell_personal(" Cannot resolve hostname to connect '%s %d'"
		      ,new_var->addr, new_var->port);
      return 0;
    }
    else
    {
      memcpy((char *) &inet_address, hp->h_addr, sizeof(struct in_addr));
      strcpy(numeric_string, inet_ntoa(inet_address));
    }
  }

  /*We have a dot notation address in numeric_string, so lets
     paste this into the struct */
  sscanf(numeric_string, "%d.%d.%d.%d",
	 &(new_var->num[0]), &(new_var->num[1]),
	 &(new_var->num[2]), &(new_var->num[3]));



  /*We have an address we can use now. */
  if (match_address(new_var))
    /*We have an address clash */
  {
    if (verbose > 0)
    {
      tell_personal("Already have that address in the database.");
      stack = oldstack;
      prev->next = new_var->next;
      FREE(new_var);
      return 0;
    }
  }
  if (verbose > 0)
  {
    sprintf(oldstack, "%s just added %s to the intercom server list.",
	    current_name, new_var->name);
    stack = end_string(oldstack);
    tell_talker_su(oldstack);
    stack = oldstack;
  }
  if (verbose != -1 && new_var->fd != P_BARRED)
  {
    if (connect_new_talker(new_var))
      send_hello(new_var);
    else
      new_var->flags |= HELLO_AFTER_CONNECT;
  }
  return 1;
}

talker_list *match_talker_name(char *str)
{
  talker_list *scan;

  if (!str || !*str)
    return 0;

  scan = talker_list_anchor;

  while (scan->next)
  {
    scan = scan->next;

    if (!strcasecmp(str, scan->name))
      return scan;
  }

  return 0;
}

talker_list *match_talker_abbr_absolute(char *str)
{
  talker_list *scan;

  if (!str || !*str)
    return 0;

  scan = talker_list_anchor;

  while (scan->next)
  {
    scan = scan->next;

    if (!strcasecmp(str, scan->abbr))
      return scan;
  }

  return 0;
}

talker_list *match_talker_abbr_pattern(char *str)
{
  talker_list *match = 0;
  talker_list *scan;
  char *oldstack, *name, *match_string, *ptr;
  int matches = 0;

  if (!str || !*str)
    return 0;

  oldstack = stack;
  name = stack;
  strcpy(name, str);
  ptr = name;
  while (*ptr)
  {
    *ptr = tolower(*ptr);
    ptr++;
  }

  stack = end_string(oldstack);
  match_string = stack;

  scan = talker_list_anchor;

  while (scan->next)
  {
    scan = scan->next;

    if (!(scan->flags & INVIS))
    {
      strcpy(stack, scan->abbr);
      ptr = stack;
      while (*ptr)
      {
	*ptr = tolower(*ptr);
	ptr++;
      }

      if (!strcmp(stack, name))
      {
	stack = oldstack;
	return scan;
      }
      if (strstr(stack, name) == stack)
      {
	matches++;
	stack = strchr(stack, 0);
	*stack++ = ',';
	*stack++ = ' ';
	match = scan;
      }
    }
  }

  if (matches == 0)
  {
    if (current_name[0])
      tell_personal("Cannot find server aliased to '%s'.", str);

    stack = oldstack;
    return 0;
  }
  if (matches > 1)
  {
    if (stack > match_string + 2)
    {
      stack -= 2;
      *stack++ = 0;
    }
    if (current_name[0])
      tell_personal("Multiple matches: %s.", match_string);

    stack = oldstack;
    return 0;
  }
  stack = oldstack;
  return match;
}

static void add_new_server_to_file(char *str)
{
  /*The talker has requested we add a new talker to the database */
  FILE *fp;
  char *name, *abbr, *addr, *oldstack;

  name = str;
  abbr = 0;
  addr = 0;

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
	  *addr++ = '\0';
      }
    }
  }
  oldstack = stack;

  if (match_talker_name(name))
  {
    tell_personal(" Intercom error: Talker '%s' already has an "
		  "entry.", name);
    return;
  }
  if (match_talker_abbr_absolute(abbr))
  {
    tell_personal(" Intercom error: Abbreviation '%s' already "
		  "has an entry.", name);
    return;
  }
  addr--;
  *addr = ':';
  abbr--;
  *abbr = ':';

  if (!add_new_talker(name, 1))
    return;

  fp = fopen("files/intercom.dbase", "a");
  if (fp)
  {
    fprintf(fp, "%s\n", name);
    fclose(fp);
  }
  else
    log("intercom", "Failed to open database list for addition");

  return;
}

static void read_talker_database(void)
{
  FILE *fp;
  char buffer[255];
  char *ptr;

  talker_list_anchor = (talker_list *) calloc(1, sizeof(talker_list));
  validation_anchor = (talker_list *) calloc(1, sizeof(talker_list));

  fp = fopen("files/intercom.dbase", "r");
  if (fp)
  {
    while (!feof(fp))
    {
      buffer[0] = '\0';
      fgets(buffer, 254, fp);
      ptr = strchr(buffer, '\n');
      if (ptr)
	*ptr = '\0';
      if (buffer[0])
      {
	/*Add the entry we are given */
	add_new_talker(buffer, -1);
      }
    }
    fclose(fp);
  }
  else
    log("intercom", "Tried to open list of talkers but failed");

  return;
}

char *set_name(char *str)
{
  char *ptr;

  if (!str || !*str)
    return 0;

  ptr = strchr(str, ':');

  if (ptr)
  {
    *ptr++ = '\0';
    strcpy(current_name, str);
    return ptr;
  }
  return str;
}

static void show_all_links(int hidden, int up_only)
{
  char *buffer, *oldstack;
  talker_list *scan;
  int count, ucount;
  int show = 0;

  if (!(talker_list_anchor->next))
  {
    tell_personal(" There are no links in the intercom database.");
    return;
  }
  oldstack = stack;

  sprintf(stack, "Main server connection status : %s----------------------"
	  "------------------\n",
	  (inet_fd < 1 || intercom_status & BAR_ALL_CONNECTIONS) ? "DOWN " : "UP --");
  stack = strchr(stack, 0);

  scan = talker_list_anchor->next;
  ucount = 0;

  if (!hidden && inet_fd > 1)
  {
    while (scan && (stack + 200 < stack_start + INTERCOM_STACK))
    {
      if (!(scan->flags & INVIS) && scan->fd > 0 &&
	  scan->validation > 0 && !(scan->flags & WAITING_CONNECT))
      {
	ucount++;
	sprintf(stack, "%-10s : UP : %s running on %s %d\n",
		scan->abbr, scan->name, scan->addr, scan->port);

	stack = strchr(stack, 0);

      }
      scan = scan->next;
    }
    if (ucount > 0)
    {
      sprintf(stack, "---------------------------- %2d talker%s up %s------"
	      "----------------------------\n", ucount,
	      ucount == 1 ? "" : "s", ucount == 1 ? "-" : "");
      stack = strchr(stack, 0);
    }
  }
  scan = talker_list_anchor->next;
  count = 0;

  if (up_only)
    scan = 0;
  else
    while (scan && (stack + 200 < stack_start + INTERCOM_STACK))
    {
      show = 1;

      /*If 'hidden', only show invisible or old */
      if (hidden)
      {
	if (!(scan->flags & INVIS) &&
	    system_time < scan->last_seen + 2419200)
	  show = 0;
      }
      else
      {
	if (scan->flags & INVIS)
	  show = 0;

	else if (!(
		    scan->fd < 1 ||
		    scan->flags & WAITING_CONNECT ||
		    scan->validation < 1))
	  show = 0;

	else if (system_time > scan->last_seen + 2419200)
	  show = 0;

      }

      if (show)
      {
	count++;
	sprintf(stack, "%-10s :", scan->abbr);
	stack = strchr(stack, 0);

	if (scan->fd == BARRED)
	  strcpy(stack, " BAR");
	else if (scan->fd == P_BARRED)
	  strcpy(stack, " BAN");
	else if (scan->fd == ERROR_FD)
	  strcpy(stack, " ERR");
	else if (scan->fd == BARRED_REMOTE)
	  strcpy(stack, "RBLK");
	else if (scan->flags & INVIS)
	  strcpy(stack, "INVS");
	else if (scan->fd < 1)
	  strcpy(stack, "DOWN");
	else if (scan->flags & WAITING_CONNECT)
	  strcpy(stack, "WAIT");
	else if (scan->validation < 1)
	  strcpy(stack, "UNCF");
	else
	  strcpy(stack, " UP ");

	stack += 4;

	sprintf(stack, ": %s running on %s %d\n", scan->name, scan->addr,
		scan->port);

	stack = strchr(stack, 0);

      }
      scan = scan->next;
    }

  if (ucount == 0 || count > 0)
  {
    if (scan)
      sprintf(stack, "MORE ROOM NEEDED IN INTERCOM_STACK\n");
    else
    {
      sprintf(stack, "---------------------------- %2d talker%s connected "
	      "%s%s-----------------", count + ucount,
       count + ucount == 1 ? "" : "s", hidden ? "invisibly " : "----------",
	      count + ucount == 1 ? "-" : "");
    }
  }
  stack = end_string(oldstack);

  buffer = (char *) calloc(1, strlen(oldstack) + 2);
  strcpy(buffer, oldstack);
  stack = oldstack;
  tell_personal("%s", buffer);

  FREE(buffer);

  return;
}

static void show_all_links_short(void)
{
  char *oldstack;
  talker_list *scan;
  int count = 0;

  oldstack = stack;
  scan = talker_list_anchor->next;

  while (scan)
  {
    count++;
    sprintf(stack, "%s:", scan->abbr);

    if (scan->fd > 0 && !(scan->flags & INVIS) &&
	scan->validation > 0 && !(scan->flags & WAITING_CONNECT))
    {
      while (*stack)
      {
	*stack = toupper(*stack);
	stack++;
      }
    }
    else if (scan->fd < -1)
      count--;
    else
    {
      while (*stack)
      {
	*stack = tolower(*stack);
	stack++;
      }
    }
    scan = scan->next;
  }

  if (count == 0)
  {
    tell_personal(" There are no active links in the intercom database.");
    stack = oldstack;
    return;
  }
  /*There are some links, the stack must reflect this... */
  stack--;
  *stack++ = 0;

  intercom_status |= INTERCOM_FORMAT_MSG;

  tell_personal("%s\n\nThere are %d servers active.", oldstack, count);

  intercom_status &= ~INTERCOM_FORMAT_MSG;

  stack = oldstack;
  return;
}

static void open_talker_link(char *str)
{
  talker_list *open_site;
  char *oldstack;

  if (!str || !*str)
    return;

  open_site = match_talker_abbr_absolute(str);
  if (!open_site)
    open_site = match_talker_name(str);

  oldstack = stack;

  if (!open_site)
  {
    tell_personal(" Cannot match talker name or abbreviation '%s'.",
		  str);
    return;
  }
  if (open_site->fd > 0)
  {
    I_SHUTDOWN(open_site->fd, 2);
    do_close(open_site);
  }
  if (connect_new_talker(open_site))
    send_hello(open_site);
  else
    open_site->flags |= HELLO_AFTER_CONNECT;

  tell_personal("Attempting to establish link to '%s'.", open_site->name);

  return;
}

static void unbar_talker(char *str)
{
  talker_list *open_site;
  char *oldstack;

  if (!str || !*str)
    return;

  open_site = match_talker_abbr_absolute(str);
  if (!open_site)
    open_site = match_talker_name(str);

  oldstack = stack;

  if (!open_site)
  {
    tell_personal(" Cannot match talker name or abbreviation '%s'.",
		  str);
    return;
  }
  if (open_site->flags & INVIS)
  {
    tell_personal(" %s is not visible to unbar.", open_site->name);
    return;
  }
  if (open_site->fd == BARRED || open_site->fd == P_BARRED)
  {
    sprintf(oldstack, "%s unbars talker '%s'.",
	    current_name, open_site->name);
    stack = end_string(oldstack);
    tell_talker_su(oldstack);
    stack = oldstack;
  }
  open_site->fd = -1;

  if (connect_new_talker(open_site))
    send_hello(open_site);
  else
    open_site->flags |= HELLO_AFTER_CONNECT;

  sync_talkers();

  return;
}

static void close_talker_link(char *str)
{
  talker_list *close_site;
  char *oldstack;

  if (!str || !*str)
    return;

  close_site = match_talker_name(str);
  if (!close_site)
    close_site = match_talker_abbr_absolute(str);

  oldstack = stack;

  if (!close_site)
  {
    tell_personal(" Cannot match talker name or abbreviation '%s'.",
		  str);
    return;
  }
  if (close_site->fd == BARRED || close_site->fd == P_BARRED)
  {
    tell_personal(" Talker '%s' is already barred.", str);
    return;
  }
  if (close_site->fd > 0)
  {
    tell_remote_talker(close_site, "%c", BARRING_YOU);

    I_SHUTDOWN(close_site->fd, 2);
    do_close(close_site);
  }
  close_site->fd = BARRED;

  sprintf(oldstack, "Talker '%s' barred by %s.",
	  close_site->name, current_name);
  stack = end_string(oldstack);
  tell_talker_su(oldstack);
  stack = oldstack;

  return;
}

static void do_out_user_command_locate(char *target_name)
{
  job_list *this_job;
  talker_list *scan;
  int count = 0;

  this_job = make_job_entry();

  strcpy(this_job->sender, current_name);
  this_job->command_type = COMMAND_LOCATE;
  if (strlen(target_name) > MAX_NAME - 1)
    target_name[MAX_NAME - 1] = '\0';
  strcpy(this_job->target, target_name);

  scan = talker_list_anchor->next;

  while (scan)
  {
    if (scan->fd > 0 && !(scan->flags & INVIS) &&
	scan->validation > 0 && !(scan->flags & WAITING_CONNECT))
    {
      count++;

      tell_remote_talker(scan, "%c%c%ld:%s:x:x", USER_COMMAND,
			 COMMAND_LOCATE, this_job->job_id,
			 target_name);
    }
    scan = scan->next;
  }

  if (count > 0)
    tell_personal(" Searching for %s on %d site%s.", target_name,
		  count, count == 1 ? "" : "s");
  else
    tell_personal(" No sites connected to search.");

  return;
}

static void do_out_user_command_room(char command_style, char *msg)
{
  talker_list *scan;

  scan = talker_list_anchor->next;

  while (scan)
  {
    if (scan->fd > 0 && !(scan->flags & INVIS) &&
	scan->validation > 0 && !(scan->flags & WAITING_CONNECT))
    {
      tell_remote_talker(scan, "%c%c%s:%s", USER_COMMAND,
			 command_style, current_name, msg);
    }
    scan = scan->next;
  }

  return;
}

static void do_out_user_command(char *str)
{
  char *target_talker, *target_name, *msg = 0, *oldstack;
  talker_list *destination_talker;
  job_list *this_job;
  char *command_style;

  if (!str || !*str || !*(str + 1))
  {
    log("intercom", "User command with no body");
    return;
  }
  command_style = str;
  str++;

  target_talker = set_name(str);

  target_name = strchr(target_talker, ':');
  if (target_name)
  {
    *target_name++ = '\0';
    if (*target_name)
    {
      msg = strchr(target_name, ':');
      if (msg)
	*msg++ = '\0';
    }
  }
  while (*msg && isspace(*msg))
    msg++;

  if (!msg || !*msg || !*target_name || !*target_talker)
  {
    tell_personal(" Badly formed string for intercom message.");
    return;
  }
  if (*command_style == COMMAND_LOCATE)
  {
    do_out_user_command_locate(target_name);
    return;
  }
  if (*command_style == COMMAND_SAY || *command_style == COMMAND_EMOTE)
  {
    do_out_user_command_room(*command_style, msg);
    return;
  }
  oldstack = stack;

  destination_talker = match_talker_abbr_pattern(target_talker);

  if (!destination_talker)
    return;

  if (destination_talker->fd < 1 || destination_talker->flags & WAITING_CONNECT)
  {
    tell_personal(" '%s' currently has no link.", destination_talker->name);
    return;
  }
  this_job = make_job_entry();

  strcpy(this_job->sender, current_name);
  if (strlen(msg) > 255)
    msg[255] = '\0';
  strcpy(this_job->msg, msg);
  this_job->command_type = *command_style;
  if (strlen(target_name) > MAX_NAME - 1)
    target_name[MAX_NAME - 1] = '\0';
  strcpy(this_job->target, target_name);
  strcpy(this_job->destination, destination_talker->abbr);

  tell_remote_talker(destination_talker,
		     "%c%c%ld:%s:%s:%s", USER_COMMAND, *command_style,
		     this_job->job_id, target_name, current_name, msg);

  return;
}

static void return_command_with_string(talker_list * remote_server,
				       job_list * this_job, char *name,
				       char *str, char c_type)
{
  if (!str || !*str)
  {
    log("intercom", "Returned empty who list from server");
    return;
  }
  tell_remote_talker(remote_server, "%c%c%ld:%s:%s", REPLY_IS, c_type,
		     this_job->job_ref, name, str);

  return;
}


static void parse_server_reply(char *str)
{
  job_list *this_job;
  talker_list *remote_server;
  char *name, *ptr;
  char blank_field[2];

  blank_field[0] = 'x';
  blank_field[1] = '\0';

  if (!str || !*str)
  {
    log("intercom", "Talker sent empty reply to request");
    return;
  }
  ptr = 0;

  name = strchr(str + 1, ':');
  if (name)
  {
    *name++ = '\0';
    if (*name)
    {
      ptr = strchr(name, ':');
      if (ptr)
	*ptr++ = '\0';
    }
  }
  if (!name || !*name)
  {
    log("intercom", " Invalid line to parse in parse_server_reply");
    return;
  }
  if (!ptr || !*ptr)
    ptr = blank_field;

  this_job = return_job(str + 1);
  if (!this_job)
    return;

  strcpy(this_job->target, name);

  /*We now know which job was related to this */

  /*Send a message to the remote server */
  if (!this_job->originator[0])
  {
    log("intercom", "No return talker address in reply request.");
    return;
  }
  remote_server = match_talker_abbr_absolute(this_job->originator);
  if (!remote_server)
    return;

  switch (*str)
  {
    case COMMAND_WHO:
    case COMMAND_EXAMINE:
    case COMMAND_FINGER:
    case COMMAND_LSU:
    case COMMAND_IDLE:
      return_command_with_string(remote_server, this_job, name, ptr, *str);
      break;
    default:
      tell_remote_talker(remote_server, "%c%c%ld:%s", REPLY_IS,
			 *str, this_job->job_ref, this_job->target);
      break;
  }

  free_job(this_job);

  return;
}

static void close_all_links(void)
{
  char *oldstack;
  talker_list *current;

  oldstack = stack;

  if (!(intercom_status & BAR_ALL_CONNECTIONS))
  {
    tell_personal(" You close %s to external messages.",
		  get_config_msg("talker_name"));
    sprintf(oldstack, "%s closes the intercom to external messages.",
	    current_name);
    stack = end_string(oldstack);
    tell_talker_su(oldstack);
    stack = oldstack;
  }
  intercom_status |= BAR_ALL_CONNECTIONS;

  current = talker_list_anchor;
  while (current->next)
  {
    current = current->next;

    if (current->fd > 0)
    {
      I_SHUTDOWN(current->fd, 2);
      do_close(current);
    }
  }
  return;
}

static void open_all_links(void)
{
  char *oldstack;
  talker_list *current;

  oldstack = stack;

  if (intercom_status & BAR_ALL_CONNECTIONS)
  {
    tell_personal(" You open %s to external messages.",
		  get_config_msg("talker_name"));
    sprintf(oldstack, "%s opens the intercom to external messages.",
	    current_name);
    stack = end_string(oldstack);
    tell_talker_su(oldstack);
    stack = oldstack;
  }
  intercom_status &= ~BAR_ALL_CONNECTIONS;

  intercom_status |= INTERCOM_BOOTING;

  current = talker_list_anchor;
  while (current->next)
  {
    current = current->next;

    if (current->fd != BARRED && current->fd != P_BARRED && current->fd < 1)
    {
      if (connect_new_talker(current))
	/*Send a HELLO */
	send_hello(current);
      else
	current->flags |= HELLO_AFTER_CONNECT;
    }
  }

  intercom_status &= ~INTERCOM_BOOTING;

  return;
}

void sync_talkers()
{
  FILE *fp;
  talker_list *scan, *prev;
  char address[MAX_INET_ADDR];

  fp = fopen("files/intercom.dbak", "w");
  if (!fp)
  {
    log("intercom", "ERROR: Couldnt open intercom database file.");
    return;
  }
  scan = talker_list_anchor;

  while (scan->next)
  {
    prev = scan;
    scan = scan->next;
    if (scan->fd != NO_SYNC_TALKER)
    {
      if (scan->num[0] == 0 && scan->num[1] == 0 &&
	  scan->num[2] == 0 && scan->num[3] == 0)
	strcpy(address, scan->addr);
      else
	sprintf(address, "%d.%d.%d.%d", scan->num[0], scan->num[1],
		scan->num[2], scan->num[3]);

      if (scan->flags & INVIS)
	fprintf(fp, "%s:%s:%s:%d:I:%ld\n", scan->name, scan->abbr, address,
		scan->port, (long int) scan->last_seen);
      else if (scan->fd != P_BARRED)
	fprintf(fp, "%s:%s:%s:%d:O:%ld\n", scan->name, scan->abbr, address,
		scan->port, (long int) scan->last_seen);
      else
	fprintf(fp, "%s:%s:%s:%d:B:%ld\n", scan->name, scan->abbr, address,
		scan->port, (long int) scan->last_seen);
    }
    else
    {
      prev->next = scan->next;
      FREE(scan);
      scan = prev;
    }
  }

  fclose(fp);

  rename("files/intercom.dbak", "files/intercom.dbase");

  return;
}

static void delete_link(char *str)
{
  char *oldstack;
  talker_list *target;

  if (!str || !*str)
  {
    log("intercom", "No talker name specified to delete");
    return;
  }
  target = match_talker_abbr_absolute(str);
  if (!target)
  {
    target = match_talker_name(str);

    if (!target)
    {
      tell_personal(" Cannot find talker '%s' to delete.", str);
      return;
    }
  }
  if (target->fd > 0)
  {
    I_SHUTDOWN(target->fd, 2);
    do_close(target);
  }
  target->fd = NO_SYNC_TALKER;

  tell_personal(" Talker deleted.");
  oldstack = stack;
  sprintf(oldstack, "%s removed '%s' from the intercom server list.",
	  current_name, target->name);
  stack = end_string(oldstack);
  tell_talker_su(oldstack);
  stack = oldstack;

  sync_talkers();

  return;
}

static void change_name(char *str)
{
  char *new_var = 0;
  talker_list *scan, *target;

  if (str && *str)
  {
    new_var = strchr(str, ':');
    if (new_var)
      *new_var++ = '\0';
  }
  if (!new_var || !*new_var || !str || !*str)
  {
    log("intercom", "Bad message passed to change_name");
    return;
  }
  if (strlen(new_var) > MAX_TALKER_NAME - 1)
    new_var[MAX_TALKER_NAME - 1] = '\0';

  scan = match_talker_name(str);
  if (!scan)
  {
    tell_personal(" Cannot find a talker with the name '%s'.", str);
    return;
  }
  target = match_talker_name(new_var);
  if (target && target != scan)
  {
    tell_personal(" There is already a talker with the name '%s'.",
		  new_var);
    return;
  }
  target = match_talker_abbr_absolute(new_var);
  if (target && target != scan)
  {
    tell_personal(" There is already a talker with the alias '%s'.",
		  new_var);
    return;
  }
  tell_personal(" You change the name of '%s' to '%s'.", scan->name, new_var);

  strcpy(scan->name, new_var);
  sync_talkers();

  return;
}

static void change_abbr(char *str)
{
  char *new_var = 0;
  talker_list *scan, *target;

  if (str && *str)
  {
    new_var = strchr(str, ':');
    if (new_var)
      *new_var++ = '\0';
  }
  if (!new_var || !*new_var || !str || !*str)
  {
    log("intercom", "Bad message passed to change_abbr");
    return;
  }
  if (strlen(new_var) > MAX_TALKER_ABBR - 1)
    new_var[MAX_TALKER_ABBR - 1] = '\0';

  scan = match_talker_abbr_absolute(str);
  if (!scan)
  {
    tell_personal(" Cannot find a talker with the alias '%s'.", str);
    return;
  }
  target = match_talker_name(new_var);
  if (target && target != scan)
  {
    tell_personal(" There is already a talker with the name '%s'.",
		  new_var);
    return;
  }
  target = match_talker_abbr_absolute(new_var);
  if (target && target != scan)
  {
    tell_personal(" There is already a talker with the alias '%s'.",
		  new_var);
    return;
  }
  tell_personal(" You change the alias of '%s' to '%s'.", scan->abbr, new_var);

  strcpy(scan->abbr, new_var);
  lower_case(scan->abbr);
  sync_talkers();

  return;
}

static void change_addr(char *str)
{
  char *new_var = 0, *ptr;
  talker_list *target, *scan;
  char new_ip[MAX_INET_ADDR];
  char existing_ip[MAX_INET_ADDR];
  struct hostent *hp;
  struct in_addr inet_address;
  int dots, all_num;

  if (str && *str)
  {
    new_var = strchr(str, ':');
    if (new_var)
      *new_var++ = '\0';
  }
  if (!new_var || !*new_var || !str || !*str)
  {
    log("intercom", "Bad message passed to change_address");
    return;
  }
  target = match_talker_name(str);
  if (!target)
    target = match_talker_abbr_absolute(str);

  if (!target)
  {
    tell_personal(" Cannot find talker '%s'.", str);
    return;
  }
  /*Test whether the address we have is all in numeric */
  dots = 0;
  all_num = 1;
  ptr = new_var;
  while (all_num && *ptr)
  {
    if (!isdigit(*ptr) && *ptr != '.')
      all_num = 0;
    if (*ptr++ == '.')
      dots++;
  }

  if (all_num)
  {
    if (dots != 3)
    {
      tell_personal(" Invalid address, '%s' supplied.", new_var);
      return;
    }
    strncpy(new_ip, new_var, MAX_INET_ADDR - 1);
    new_ip[MAX_INET_ADDR - 1] = 0;
  }
  else
  {
    /*Its an alpha address, we need to convert */
    hp = gethostbyname(new_var);
    if (!hp)
    {
      tell_personal(" Cannot resolve hostname '%s'.", new_var);
      return;
    }
    else
    {
      memcpy((char *) &inet_address, hp->h_addr, sizeof(struct in_addr));
      strcpy(new_ip, inet_ntoa(inet_address));
    }
  }

  /*We now have an IP address in dot notation */

  scan = talker_list_anchor;
  while (scan->next)
  {
    scan = scan->next;
    if (scan != target && scan->port == target->port)
    {
      sprintf(existing_ip, "%d.%d.%d.%d", scan->num[0], scan->num[1],
	      scan->num[2], scan->num[3]);
      if (!strcmp(existing_ip, new_ip))
      {
	tell_personal(" But %s already has that address.", scan->name);
	return;
      }
    }
  }

  /*Its a unique address, so let them have it */
  tell_personal(" You change the address of '%s' from %s to %s.",
		target->name, target->addr, new_var);

  strcpy(target->addr, new_var);
  if (target->fd > 0)
  {
    I_SHUTDOWN(target->fd, 2);
    do_close(target);
  }
  sscanf(new_ip, "%d.%d.%d.%d",
	 &(target->num[0]), &(target->num[1]), &(target->num[2]),
	 &(target->num[3]));

  if (connect_new_talker(target))
    send_hello(target);
  else
    target->flags |= HELLO_AFTER_CONNECT;

  sync_talkers();

  return;
}

static void change_port(char *str)
{
  char *new_var = 0;
  talker_list *target, *scan;
  int port;

  if (str && *str)
  {
    new_var = strchr(str, ':');
    if (new_var)
      *new_var++ = '\0';
  }
  if (!new_var || !*new_var || !str || !*str)
  {
    log("intercom", "Bad message passed to change_port");
    return;
  }
  port = ATOI(new_var);
  if (port < 1)
  {
    tell_personal(" Bad portnumber (%d) given.", port);
    return;
  }
  target = match_talker_name(str);
  if (!target)
    target = match_talker_abbr_absolute(str);

  if (!target)
  {
    tell_personal(" Cannot find talker '%s'.", str);
    return;
  }
  scan = talker_list_anchor;
  while (scan->next)
  {
    scan = scan->next;
    if (scan != target)
      if (scan->num[0] == target->num[0] &&
	  scan->num[1] == target->num[1] &&
	  scan->num[2] == target->num[2] &&
	  scan->num[3] == target->num[3] &&
	  scan->port == port)
      {
	tell_personal(" But %s already has that address.", scan->name);
	return;
      }
  }

  /*Its a unique address, so let them have it */
  tell_personal(" You change the port of '%s' from %d to %d.", target->name,
		target->port, port);
  target->port = port;
  sync_talkers();

  if (target->fd > 0)
  {
    I_SHUTDOWN(target->fd, 2);
    do_close(target);
  }
  if (connect_new_talker(target))
    send_hello(target);
  else
    target->flags |= HELLO_AFTER_CONNECT;

  return;
}

static void banish_site(char *str)
{
  talker_list *target;
  char *oldstack;

  if (!str || !*str)
  {
    log("intercom", "Sent bad string to banish_site()");
    return;
  }
  target = match_talker_name(str);
  if (!target)
    target = match_talker_abbr_absolute(str);

  if (!target)
  {
    tell_personal(" Cannot locate talker '%s'.", str);
    return;
  }
  if (target->fd == P_BARRED)
  {
    tell_personal(" '%s' is already banished.", target->name);
    return;
  }
  if (target->fd > 0)
  {
    tell_remote_talker(target, "%c", BARRING_YOU);

    I_SHUTDOWN(target->fd, 2);
    do_close(target);
  }
  target->fd = P_BARRED;

  sync_talkers();

  oldstack = stack;
  sprintf(oldstack, "%s banishes talker '%s'.", current_name, target->name);
  stack = end_string(oldstack);
  tell_talker_su(oldstack);
  stack = oldstack;

  return;
}

static void unhide_entry(char *str)
{
  talker_list *target;

  if (!str || !*str)
  {
    log("intercom", "Sent bad string to unhide_entry()");
    return;
  }
  target = match_talker_name(str);
  if (!target)
    target = match_talker_abbr_absolute(str);

  if (!target)
  {
    tell_personal(" Cannot locate talker '%s'.", str);
    return;
  }
  if (!(target->flags & INVIS))
  {
    tell_personal(" %s doesnt appear to be invisible.", target->name);
    return;
  }
  target->flags &= ~INVIS;

  tell_personal(" %s is now visible (but still banished).", target->name);

  return;
}

static void hide_entry(char *str)
{
  talker_list *target;
  char *oldstack;

  if (!str || !*str)
  {
    log("intercom", "Sent bad string to hide_entry()");
    return;
  }
  target = match_talker_name(str);
  if (!target)
    target = match_talker_abbr_absolute(str);

  if (!target)
  {
    tell_personal(" Cannot locate talker '%s'.", str);
    return;
  }
  if (target->flags & INVIS)
  {
    tell_personal(" %s is already invisible.", target->name);
    return;
  }
  if (target->fd > 0)
  {
    tell_remote_talker(target, "%c", BARRING_YOU);

    I_SHUTDOWN(target->fd, 2);
    do_close(target);
  }
  target->fd = P_BARRED;

  target->flags |= INVIS;

  sync_talkers();

  oldstack = stack;
  sprintf(oldstack, "%s banishes and removes talker '%s' from the visible "
	  "database.", current_name, target->name);
  stack = end_string(oldstack);
  tell_talker_su(oldstack);
  stack = oldstack;

  return;
}

static void request_server_list(void)
{
  talker_list *scan;

  scan = talker_list_anchor;
  while (scan->next)
  {
    scan = scan->next;
    if (scan->fd > 0)
      tell_remote_talker(scan, "%c", REQUEST_SERVER_LIST);
  }

  return;
}

static void expire_jobs(void)
{
  job_list *scan;

  scan = job_anchor;

  while (scan->next != job_anchor && scan->next->timeout < system_time)
  {
    scan = scan->next;

    if (scan->timeout < system_time)
    {
      if (*(scan->sender))
      {
	sprintf(current_name, scan->sender);
	switch (scan->command_type)
	{
	  case COMMAND_TELL:
	    tell_personal(" Your tell command to %s@%s expired.",
			  scan->target, scan->destination);
	    break;
	  case COMMAND_REMOTE:
	    tell_personal(" Your remote command to %s@%s expired.",
			  scan->target, scan->destination);
	    break;
	  case COMMAND_WHO:
	    tell_personal(" Your who command at %s expired.",
			  scan->destination);
	    break;
	  case COMMAND_EXAMINE:
	    tell_personal(" Your examine command on %s@%s expired.",
			  scan->target, scan->destination);
	    break;
	  case COMMAND_FINGER:
	    tell_personal(" Your finger command on %s@%s expired.",
			  scan->target, scan->destination);
	    break;
	  case COMMAND_IDLE:
	    tell_personal(" Your idle command on %s@%s expired.",
			  scan->target, scan->destination);
	    break;
	}
      }
      scan = scan->prev;
      free_job(scan->next);
    }
  }

  return;
}

static void request_stats(char *str)
{
  talker_list *target = 0;
  net_usage *stats;
  char *oldstack;

  oldstack = stack;

  if (!str || !*str)
  {
    stats = &total_net;
    sprintf(oldstack, "Total intercom network statistics:\n");
  }
  else if (!strcasecmp(str, get_config_msg("talker_name")) ||
	   !strcasecmp(str, get_config_msg("intercom_abbr")))
  {
    stats = &server_net;
    sprintf(oldstack, "Intercom from Walt link statistics:\n");
  }
  else
  {
    target = match_talker_abbr_absolute(str);
    if (!target)
      target = match_talker_name(str);

    if (!target)
    {
      tell_personal(" Cannot find talker '%s' in the database.", str);
      return;
    }
    stats = &(target->net_stats);
    sprintf(oldstack, "Intercom statistics for talker '%s'.\n", target->name);
  }

  stack = strchr(oldstack, 0);

  if (target)
  {
    sprintf(stack, "\nLink status is ");
    stack = strchr(stack, 0);

    if (target->fd == BARRED)
      strcpy(stack, "Barred link");
    else if (target->fd == P_BARRED)
      strcpy(stack, "Banished link");
    else if (target->fd == BARRED_REMOTE)
      strcpy(stack, "Refused Link");
    else if (target->fd == ERROR_FD)
      strcpy(stack, "Unknown error");
    else if (target->fd < 1)
      strcpy(stack, "Down");
    else if (target->flags & WAITING_CONNECT)
      strcpy(stack, "Waiting for response to connection request");
    else if (target->validation < 0)
      strcpy(stack, "Connected but not securely validated");
    else
      strcpy(stack, "Up and running.");
    stack = strchr(stack, 0);

  }
  sprintf(stack, "\n\n"
	  "                   IN        OUT\n"
	  "Bytes/Sec         %7.3f    %7.3f\n"
	  "Total Bytes    %6d     %6d\n"
	  "Packets/Sec       %7.3f    %7.3f\n"
	  "Total Packets  %6d     %6d\n"
	  "\nLink has been alive for a total of %d:%02d\n"
	  "This link was established: %s",
	  stats->chars_in == 0 ? 0 :
	  (float) (stats->chars_in) / (float) (stats->up_clicks),

	  stats->chars_out == 0 ? 0 :
	  (float) (stats->chars_out) / (float) (stats->up_clicks),

	  stats->chars_in, stats->chars_out,

	  stats->packets_in == 0 ? 0 :
	  (float) (stats->packets_in) / (float) (stats->up_clicks),

	  stats->packets_out == 0 ? 0 :
	  (float) (stats->packets_out) / (float) (stats->up_clicks),
	  stats->packets_in, stats->packets_out,

	  stats->up_clicks == 0 ? 0 :
	  (stats->up_clicks) / 60,

	  stats->up_clicks == 0 ? 0 :
	  (stats->up_clicks) % 60, sys_time(stats->established));

  stack = end_string(oldstack);
  tell_personal("%s", oldstack);
  stack = oldstack;

  return;
}

static void request_room_look_global(void)
{
  talker_list *scan;
  job_list *this_job;

  this_job = make_job_entry();

  strcpy(this_job->sender, current_name);


  scan = talker_list_anchor;
  while (scan->next)
  {
    scan = scan->next;
    if (scan->fd > 0)
      tell_remote_talker(scan, "%c%ld", INTERCOM_ROOM_LOOK, this_job->job_id);
  }

  return;
}


static void parse_show_links_request(char *str)
{
  set_name(str + 1);

  switch (*str)
  {
    case LIST_ALL:
      show_all_links(0, 0);
      break;
    case LIST_HIDDEN:
      show_all_links(1, 0);
      break;
    case LIST_UP:
      show_all_links(0, 1);
      break;
  }

  return;
}

static void reply_room_look_global(char *str)
{
  job_list *this_job;
  talker_list *remote_talker = 0;
  char *job_str, *msg = 0;

  job_str = str;
  if (job_str && *job_str)
  {
    msg = strchr(job_str, ':');
    if (msg)
      *msg++ = 0;
  }
  if (!job_str || !*job_str || !msg || !*msg)
  {
    log("intercom", "Invalid message sent to reply_room_look_global");
    return;
  }
  this_job = return_job(job_str);
  if (!this_job)
    return;


  if (this_job->originator[0])
    remote_talker = match_talker_abbr_absolute(this_job->originator);

  if (!remote_talker)
    return;

  tell_remote_talker(remote_talker, "%c%ld:%s", INTERCOM_ROOM_LIST,
		     this_job->job_ref, msg);

  return;
}

static void inform_connected_move(char *str)
{
  talker_list *scan;

  scan = talker_list_anchor;
  while (scan->next)
  {
    scan = scan->next;
    if (scan->fd > 0)
      tell_remote_talker(scan, "%c%s", WE_ARE_MOVING, str);
  }

  return;
}


static void parse_talker_input(void)
{
  char c;
  int chars_left;
  char *oldstack, *ptr;
  packet *this_packet;
  int parse_ok = 0;

  if (ioctl(talker_fd, FIONREAD, &chars_left) == -1)
  {
    I_SHUTDOWN(talker_fd, 2);
    close(talker_fd);
    talker_fd = -1;
    closedown = 1;

    log("intercom", "PANIC on FIONREAD on talker_fd.");

    return;
  }
  if (!chars_left)
  {
    I_SHUTDOWN(talker_fd, 2);
    close(talker_fd);
    talker_fd = -1;
    closedown = 1;
    log("intercom", "Link died on talker_fd");
    return;
  }
  this_packet = add_packet_to_list();
  oldstack = stack;

  ptr = oldstack;

  ping_time = system_time + 600;

  c = (char) (END_MESSAGE - 1);
  while (chars_left && c != (char) END_MESSAGE)
  {
    chars_left--;
    server_net.chars_in++;
    if (read(talker_fd, &c, 1) != 1)
    {
      log("intercom", "Read error on talker_fd socket");
      I_SHUTDOWN(talker_fd, 2);
      close(talker_fd);
      talker_fd = -1;
      closedown = 1;
      return;
    }
    if (c != (char) END_MESSAGE)
    {
      if (c != (char) INCOMPLETE_MESSAGE)
      {
	this_packet->data[this_packet->length] = c;
	this_packet->length++;
	if (this_packet->length >= MAX_PACKET)
	  this_packet = add_packet_to_list();
      }
    }
    else
      parse_ok = 1;
  }

  if (!parse_ok)
    return;

  this_packet = talker_packet_anchor;
  strcpy(oldstack, this_packet->data);
  stack = strchr(oldstack, 0);

  while (this_packet->next)
  {
    this_packet = this_packet->next;
    strcpy(stack, this_packet->data);
    stack = strchr(stack, 0);
  }
  stack++;

  ptr = oldstack;

  if (*ptr)
  {
    switch (*ptr)
    {
      case BANISH_SITE:
	ptr = set_name(ptr + 1);
	banish_site(ptr);
	break;
      case OPEN_ALL_LINKS:
	ptr = set_name(ptr + 1);
	open_all_links();
	break;
      case CLOSE_ALL_LINKS:
	ptr = set_name(ptr + 1);
	close_all_links();
	break;
      case PORTNUMBER_FOLLOWS:
	create_inet_socket(ptr + 1);
	break;
      case UNBAR_LINK:
	ptr = set_name(ptr + 1);
	unbar_talker(ptr);
	break;
      case CLOSE_LINK:
	ptr = set_name(ptr + 1);
	close_talker_link(ptr);
	break;
      case ADD_NEW_LINK:
	ptr = set_name(ptr + 1);
	add_new_server_to_file(ptr);
	break;
      case USER_COMMAND:
	do_out_user_command(ptr + 1);
	break;
      case SHOW_LINKS:
	parse_show_links_request(ptr + 1);
	break;
      case DELETE_LINK:
	ptr = set_name(ptr + 1);
	delete_link(ptr);
	break;
      case REPLY_IS:
	parse_server_reply(ptr + 1);
	break;
      case OPEN_LINK:
	ptr = set_name(ptr + 1);
	open_talker_link(ptr);
	break;
      case CHANGE_NAME:
	ptr = set_name(ptr + 1);
	change_name(ptr);
	break;
      case CHANGE_ABBR:
	ptr = set_name(ptr + 1);
	change_abbr(ptr);
	break;
      case CHANGE_ADDRESS:
	ptr = set_name(ptr + 1);
	change_addr(ptr);
	break;
      case CHANGE_PORT:
	ptr = set_name(ptr + 1);
	change_port(ptr);
	break;
      case REQUEST_SERVER_LIST:
	request_server_list();
	break;
      case INTERCOM_DIE:
	closedown = 1;
	break;
      case REQUEST_STATS:
	ptr = set_name(ptr + 1);
	request_stats(ptr);
	break;
      case SHOW_ALL_LINKS_SHORT:
	ptr = set_name(ptr + 1);
	show_all_links_short();
	break;
      case HIDE_ENTRY:
	ptr = set_name(ptr + 1);
	hide_entry(ptr);
	break;
      case UNHIDE_ENTRY:
	ptr = set_name(ptr + 1);
	unhide_entry(ptr);
	break;
      case INTERCOM_ROOM_LOOK:
	ptr = set_name(ptr + 1);
	request_room_look_global();
	break;
      case INTERCOM_ROOM_LIST:
	reply_room_look_global(ptr + 1);
	break;
      case WE_ARE_MOVING:
	inform_connected_move(ptr + 1);
	break;
    }
  }
  stack = oldstack;
  current_name[0] = '\0';
  free_list_packets();

  if (chars_left > 0)
    parse_talker_input();

  return;
}

static void ping_all_down_talkers(void)
{
  talker_list *scan;

  scan = talker_list_anchor;

  while (scan->next)
  {
    scan = scan->next;

    if (scan->fd < 1)
    {
      /*Connect the talker */
      if (connect_new_talker(scan))
	/*Send a HELLO */
	send_hello(scan);
      else
	scan->flags |= HELLO_AFTER_CONNECT;
    }
  }

  return;
}

void get_config(void)
{
  if (config_msg.where)
    FREE(config_msg.where);

  config_msg = load_file("soft/config.msg");


  log("intercom", "Intercom booting");

  if (strchr(get_config_msg("talker_name"), ':'))
  {
    log("intercom", "In soft/config.msg, talker_name may NOT have a : in it. Not booted");
    exit(-1);
  }
  if (strchr(get_config_msg("intercom_abbr"), ':'))
  {
    log("intercom", "In soft/config.msg, intercom_abbr may NOT have a : in it. Not booted");
    exit(-1);
  }
  max_log_size = atoi(get_config_msg("max_log_size"));
}
extern int main(int argc, char **argv)
{
  int dummy;
  fd_set fd_list, connect_list;
  int failed, connects_this_loop;
  struct timeval timeout;
  struct sigaction siga;
  talker_list *current_talker, *prev;
  time_t last_click, now = 0;

  intercom_status = INTERCOM_BOOTING;
  closedown = 0;

  srand(time(NULL));

  stack_start = (char *) calloc(1, INTERCOM_STACK);
  stack = stack_start;

  get_config();

/**********SIGNALS***********/

  siga.sa_handler = sigpipe;
  siga.sa_flags = 0;
  sigaction(SIGPIPE, &siga, 0);
  siga.sa_handler = sighup;
  sigaction(SIGHUP, &siga, 0);
  siga.sa_handler = sigquit;
  sigaction(SIGQUIT, &siga, 0);
  siga.sa_handler = sigill;
  sigaction(SIGILL, &siga, 0);
  siga.sa_handler = sigfpe;
  sigaction(SIGFPE, &siga, 0);
  siga.sa_handler = sigbus;
  sigaction(SIGBUS, &siga, 0);
  siga.sa_handler = sigsegv;
  sigaction(SIGSEGV, &siga, 0);
  siga.sa_handler = sigterm;
  sigaction(SIGTERM, &siga, 0);
  siga.sa_handler = sigxfsz;
  sigaction(SIGXFSZ, &siga, 0);
  siga.sa_handler = sigchld;
  sigaction(SIGCHLD, &siga, 0);
  siga.sa_handler = sigusr1;
  sigaction(SIGUSR1, &siga, 0);

/****END SIGNALS*********/

  system_time = time(NULL);
  ping_time = system_time + 600;

  if (chdir(ROOT))
  {
    log("intercom", " Cannot change to root directory.");
    exit(1);
  }
  /*Bind main unix socket */
  failed = 0;
  unix_fd = -1;
  talker_fd = -1;
  inet_fd = -1;
  while (unix_fd < 0 && failed < 5)
  {
    unix_fd = create_main_intercom_socket(INTERCOM_SOCKET);
    if (unix_fd < 0)
      failed++;
  }

  if (failed == 5)
  {
    log("intercom", "Repeated fails to create unix socket, aborting.");
    exit(1);
  }
  setup_jobs_list();
  setup_free_packets();
  memset(&server_net, 0, sizeof(net_usage));
  memset(&total_net, 0, sizeof(net_usage));

  while (!closedown)
  {
    if (getppid() == 1)
      break;

    last_click = now;
    now = time(NULL);
    if (now != last_click)
      system_time++;

    connects_this_loop = 0;

    intercom_status &= ~INTERCOM_HIGHLIGHT;
    intercom_status &= ~INTERCOM_PERSONAL_MSG;

    current_name[0] = '\0';
    if (stack != stack_start)
    {
      stack = stack_start;
      log("intercom", "Bad stack in main loop. Recovered.");
    }
    if ((now != last_click) && system_time % 10 == 0)
    {
      if (inet_fd < 1 && talker_fd > 0)
	request_port_from_talker();
      if (ping_time - system_time < 300)
	send_to_talker("%c", PING);
      if (system_time % 300 == 0)
	/*Every 5 minutes, ping all down talkers */
	ping_all_down_talkers();
    }
    if (ping_time < system_time)
      closedown = 1;

    /*Clear out the old jobs stuff */
    expire_jobs();

    FD_ZERO(&fd_list);
    FD_ZERO(&connect_list);
    FD_SET(unix_fd, &fd_list);

    if (talker_fd > -1)
    {
      FD_SET(talker_fd, &fd_list);
      if (now != last_click)
	server_net.up_clicks += (now - last_click);
    }
    if (inet_fd > -1)
    {
      FD_SET(inet_fd, &fd_list);
      if (now != last_click)
	total_net.up_clicks += (now - last_click);

      current_talker = talker_list_anchor;
      while (current_talker->next)
      {
	current_talker = current_talker->next;
	if (current_talker->fd > 0)
	{
	  /*If waiting connect() set to both lists */
	  if (current_talker->flags & WAITING_CONNECT)
	    FD_SET(current_talker->fd, &connect_list);
	  FD_SET(current_talker->fd, &fd_list);

	  if (now != last_click)
	    current_talker->net_stats.up_clicks += (now - last_click);
	}
	else if (current_talker->fd == NO_CONNECT_TRIED &&
		 connects_this_loop == 0)
	{
	  connects_this_loop = 1;
	  if (connect_new_talker(current_talker))
	    send_hello(current_talker);
	  else
	    current_talker->flags |= HELLO_AFTER_CONNECT;
	}
      }

      current_talker = validation_anchor;
      while (current_talker->next)
      {
	current_talker = current_talker->next;
	if (current_talker->fd > 0)
	  FD_SET(current_talker->fd, &fd_list);
      }
    }
    timeout.tv_sec = 0;
    timeout.tv_usec = (1000000 / TIMER_CLICK);

    if (select(FD_SETSIZE, &fd_list, &connect_list, 0, &timeout) > 0)
    {
      if (FD_ISSET(unix_fd, &fd_list))
      {
	dummy = get_unix_fd_input();
	if (dummy > -1)
	{
	  talker_fd = dummy;
	  read_talker_database();
	  request_port_from_talker();
	}
      }
      if (talker_fd > -1)
	if (FD_ISSET(talker_fd, &fd_list))
	  parse_talker_input();

      if (inet_fd > -1)
	if (FD_ISSET(inet_fd, &fd_list))
	  get_new_talker_connection();

      current_talker = talker_list_anchor;
      while (current_talker->next)
      {
	current_talker = current_talker->next;
	if (current_talker->fd > 0)
	{
	  if (current_talker->flags & WAITING_CONNECT &&
	      FD_ISSET(current_talker->fd, &connect_list))
	  {
	    /*A connect just succeded */
	    current_talker->flags &= ~WAITING_CONNECT;

	    /*If we need a hello sent, send it */
	    if (current_talker->flags & HELLO_AFTER_CONNECT)
	    {
	      current_talker->flags &= ~HELLO_AFTER_CONNECT;

	      send_hello(current_talker);
	    }
	    if (current_talker->flags & VALIDATE_AFTER_CONNECT)
	    {
	      current_talker->flags &= ~VALIDATE_AFTER_CONNECT;

	      actual_validate_send(current_talker);
	    }
	  }
	  if (current_talker->fd > 0 &&
	      FD_ISSET(current_talker->fd, &fd_list))
	    parse_remote_talker_input(current_talker, 1);
	}
      }

      current_talker = validation_anchor;
      while (current_talker->next)
      {
	current_talker = current_talker->next;
	if (current_talker->fd > 0)
	{
	  if (FD_ISSET(current_talker->fd, &fd_list))
	    parse_remote_talker_input(current_talker, 0);
	}
      }
      current_talker = validation_anchor;
      while (current_talker->next)
      {
	prev = current_talker;
	current_talker = current_talker->next;
	if (current_talker->fd < 1 ||
	    current_talker->timeout < system_time)
	{
	  prev->next = current_talker->next;
	  FREE(current_talker);
	  current_talker = prev;
	}
      }
    }
  }

  close_all_sockets();
  close_intercom_main_socket();
  sync_talkers();

  log("intercom", "Shutdown complete.");

  return 0;
}

file load_file(char *filename)
{
  file f;
  int d;
  char *oldstack;

  oldstack = stack;

  d = open(filename, O_RDONLY);
  if (d < 0)
  {
    sprintf(oldstack, "Intercom can't find file:%s", filename);
    stack = end_string(oldstack);
    log("error", oldstack);
    f.where = (char *) MALLOC(1);
    *(char *) f.where = 0;
    f.length = 0;
    stack = oldstack;
    return f;
  }
  f.length = lseek(d, 0, SEEK_END);
  lseek(d, 0, SEEK_SET);
  f.where = (char *) MALLOC(f.length + 1);
  memset(f.where, 0, f.length + 1);
  if (read(d, f.where, f.length) < 0)
  {
    sprintf(oldstack, "Intercom error reading file:%s", filename);
    stack = end_string(oldstack);
    log("error", oldstack);
    f.where = (char *) MALLOC(1);
    *(char *) f.where = 0;
    f.length = 0;
    stack = oldstack;
    return f;
  }
  close(d);
  stack = oldstack;
  *(f.where + f.length) = 0;
  return f;
}

char *get_config_msg(char *type)
{
  char *got;
  static char smbuf[1024];

  memset(smbuf, 0, 1024);
  if (!config_msg.where || !*config_msg.where)
  {
    log("error", "Intercom : Softmsg file for config_msg aint loaded!");
    return "error";
  }
  got = lineval(config_msg.where, type);
  if (!got || !*got)
  {
    log("error", "Intercom : Softmsg in config_msg isnt there!");
    return "error";
  }

  strncpy(smbuf, got, 1023);

  return smbuf;
}


#else
/* another case where we must sacrifice a tad of convience in
   one place to get loads of it else where...
 */
#define TALKER_NAME "Playground+"

int main(void)
{
  printf("\nPlayground+ Intercom Server\n"
	 LINE
	 "This code is currently disabled. If you wish to have the intercom "
   "included\nwithin %s you need to re-run 'make config' and select it.\n\n"
      "Once selected, the intercom is run as part of the talker and should "
	 "not be\nrun on its own.\n\n", TALKER_NAME);
  exit(0);
}

#endif
