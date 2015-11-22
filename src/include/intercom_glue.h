/*
 * Playground+ - intercom_glue.c
 * Grims intercom_glue header file
 * ---------------------------------------------------------------------------
 */

#ifndef _INTERCOM_GLUE_H
#define _INTERCOM_GLUE_H

extern int establish_intercom_server(void);

extern void add_intercom_server(player *,char *);
extern void bar_talker(player *,char *);
extern void close_intercom(player *,char *);
extern void delete_intercom_server(player *,char *);
extern void do_intercom_emote(player *,char *);
extern void do_intercom_examine(player *,char *);
extern void do_intercom_finger(player *,char *);
extern void do_intercom_idle(player *,char *);
extern void do_intercom_lsu(player *,const char *);
extern void do_intercom_remote(player *,char *);
extern void do_intercom_say(player *,char *);
extern void do_intercom_tell(player *,char *);
extern void do_intercom_think(player *,char *);
extern void do_intercom_who(player *,char *);
extern void intercom_banish(player *,char *);
extern void intercom_banish_name(player *,char *);
extern void intercom_bar_name(player *,char *);
extern void intercom_change_address(player *,char *);
extern void intercom_change_alias(player *,char *);
extern void intercom_change_name(player *,char *);
extern void intercom_change_port(player *,char *);
extern void intercom_command(player *,char *);
extern void intercom_hide(player *,char *);
extern void intercom_home(player *,char *);
extern void intercom_locate_name(player *,char *);
extern void intercom_ping(player *,char *);
extern void intercom_reboot(player *,char *);
extern void intercom_request_stats(player *,char *);
extern void intercom_room_look(player *);
extern void intercom_unbanish_name(player *,char *);
extern void intercom_site_move(player *,char *);
extern void intercom_slist(player *,char *);
extern void intercom_unbar_name(player *,char *);
extern void intercom_unhide(player *,char *);
extern void intercom_update_servers(player *,char *);
extern void intercom_version(player *,char *);
extern void kill_intercom(void);
extern void list_intercom_servers(player *,char *);
extern void open_intercom(player *,char *);
extern void parse_incoming_intercom(void);
extern void start_intercom(void);
extern void unbar_talker(player *,char *);
extern void view_intercom_commands(player *,char *);

#endif
