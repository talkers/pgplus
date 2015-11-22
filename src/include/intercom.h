/*
 * intercom.h
 * Grims intercom header file
 * ---------------------------------------------------------------------------
 */

#ifndef _INTERCOM_H
#define _INTERCOM_H

#ifndef MAX_NAME
#include "player.h"
#endif

#define INTERCOM_VERSION "1.1.1"
#define INTERCOM_STACK 20480

#define COLOUR_TERMINATOR "^N"

#define MAX_TALKER_NAME 40
#define MAX_TALKER_ABBR 10
#define MAX_PACKET 200
#ifdef WALT
#define VARARGS
#endif

typedef struct _packet
{
  char data[MAX_PACKET + 1];
  int length;
  struct _packet *next;
} packet;

typedef struct 
{
  int packets_in;
  int packets_out;
  int chars_in;
  int chars_out;
  int established;
  int up_clicks;
} net_usage;

typedef struct _talker_list
{
  char name[MAX_TALKER_NAME];
  char abbr[MAX_TALKER_ABBR];
  char addr[MAX_INET_ADDR];
  int num[4];
  int port;
  int fd;
  int flags;
  time_t timeout;
  signed int validation;
  time_t last_seen;
  net_usage net_stats;
  packet *packet_anchor;
  struct _talker_list *next;
} talker_list;

typedef struct _job_list
{
  long unsigned int job_id;
  long unsigned int job_ref;
  char sender[MAX_NAME];
  char msg[256];
  time_t timeout;
  int command_type;
  char target[MAX_NAME];
  char originator[MAX_TALKER_ABBR];
  char destination[MAX_TALKER_ABBR];
  struct _job_list *next;
  struct _job_list *prev;
} job_list;

typedef struct _nameban
{
  char name[MAX_NAME];
  short type;
  struct _nameban *next;
} nameban;

/*
 * Protocol for messages between walt and the intercom
 */

#define BANISH_SITE 1
#define OPEN_ALL_LINKS 2
#define CLOSE_ALL_LINKS 3
#define UNBAR_LINK 4
#define CLOSE_LINK 5
#define ADD_NEW_LINK 6
#define USER_COMMAND 7
#define NAME_IGNORED 8
#define NAME_BLOCKED 9
#define REQUEST_PORTNUMBER 10
#define LINK_REQUESTED_UNKNOWN 11
#define SHOW_LINKS 12
#define SU_MESSAGE 13
#define DELETE_LINK 15
#define PERSONAL_MESSAGE 16
#define HIGHLIGHT_RETURN 17
#define HELLO_I_AM 18
#define REQUEST_VALIDATION_AS 19
#define VALIDATION_IS 20
#define BAD_HELLO 21
#define BAD_VALIDATION 22
#define PERSONAL_MESSAGE_TAG 23
#define PERSONAL_MESSAGE_AND_RETURN 24
#define REPLY_IS 25
#define NO_SUCH_PLAYER 26
#define COMMAND_SUCCESSFUL 27
#define PORTNUMBER_FOLLOWS 28
#define OPEN_LINK 29
#define CHANGE_NAME 30
#define CHANGE_ABBR 31
#define CHANGE_ADDRESS 32
#define CHANGE_PORT 33
#define TALKER_IGNORED 34
#define TALKER_BLOCKED 35
#define REQUEST_SERVER_LIST 36
#define I_KNOW_OF 37
#define INTERCOM_DIE 38
#define PING 39
#define REQUEST_STATS 40
#define NAME_BANISHED 41
#define MULTIPLE_NAME_MATCH 42
#define STARTING_CONNECT 43
#define SHOW_ALL_LINKS_SHORT 44
#define HIDE_ENTRY 45
#define FORMAT_MESSAGE_TAG 46
#define UNHIDE_ENTRY 47
#define BARRING_YOU 48
#define INTERCOM_ROOM_MESSAGE 49
#define INTERCOM_ROOM_LOOK 50
#define INTERCOM_ROOM_LIST 51
#define WE_ARE_MOVING 52
/*RESERVED ALL VALUES UP TO &inc 100 FOR CENTRAL INTERCOM DEVELOPMENT*/

/*RESERVED ALL VALUES ABOVE 0xEF FOR CENTRAL DEVELOPMENT*/
#define END_MESSAGE (0xFE)
#define INCOMPLETE_MESSAGE (0xFF)


/*Command codes*/
#define COMMAND_WHO 1
#define COMMAND_FINGER 2
#define COMMAND_EXAMINE 3
#define COMMAND_TELL 4
#define COMMAND_REMOTE 5
#define COMMAND_LSU 6
#define COMMAND_LOCATE 7
/*Ack, buggered up the protocol, have to miss a few out*/
#define COMMAND_IDLE 10
#define COMMAND_SAY 11
#define COMMAND_EMOTE 12
/*RESERVED ALL VALUES UP TO 100 FOR CENTRAL INTERCOM DEVELOPMENT*/

/*List codes*/
#define LIST_ALL 1
#define LIST_HIDDEN 2
#define LIST_UP 3
/*RESERVED ALL VALUES UP TO 50 FOR CENTRAL INTERCOM DEVELOPMENT*/

/*Intercom status*/
#define INTERCOM_HIGHLIGHT (1<<0)
#define BAR_ALL_CONNECTIONS (1<<1)
#define INTERCOM_PERSONAL_MSG (1<<2)
#define INTERCOM_BOOTING (1<<3)
#define INTERCOM_FORMAT_MSG (1<<4)
/*RESERVED ALL VALUES UP TO (1<<25) FOR CENTRAL INTERCOM DEVELOPMENT*/

/*FD status*/
#define NO_CONNECT_TRIED -23
#define ERROR_FD -69
#define BARRED -99
#define BARRED_REMOTE -123
#define P_BARRED -999
#define NO_SYNC_TALKER -9999
/*RESERVED ALL VALUES DOWN TO  -10000 FOR CENTRAL INTERCOM DEVELOPMENT*/

/*talker flags*/
#define WAITING_CONNECT (1<<0)
#define HELLO_AFTER_CONNECT (1<<1)
#define INVIS (1<<2)
#define VALIDATE_AFTER_CONNECT (1<<3)
/*RESERVED ALL VALUES UP TO (1<<25) FOR CENTRAL INTERCOM DEVELOPMENT*/

#endif
