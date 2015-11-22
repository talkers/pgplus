/*
 * Playground+ - proto.h
 * Standard set of prototypes
 * -----------------------------------------------------------------------
 */

#include "dynamic.h"
#ifdef __GNUC__
#define VARG_END1 __attribute__ ((format (printf,1,2)));
#define VARG_END2 __attribute__ ((format (printf,2,3)));
#define VARG_END3 __attribute__ ((format (printf,3,4)));
#else /* __GNUC__ */
#define VARG_END1 ;
#define VARG_END2 ;
#define VARG_END3 ;
#endif /* !__GNUC__ */

extern alias *get_alias(player *, char *);
extern char *birthday_string(time_t bday);
extern char *bit_string(int);
extern char *caps(char *);
extern char *check_legal_entry(player *, char *, int);
extern char *convert_time(time_t);
extern char *do_alias_match(player *, char *);
extern char *do_crypt(char *, player *);
extern char *do_pipe(player *, char *);
extern char *end_string(char *);
extern char *first_char(player *);
extern char *full_name(player *);
extern char *gender_raw(player *);
extern char *get_address (player *, player *);
extern char *get_flag_name(flag_list *, char *);
extern char *get_gender_string(player *);
extern char *get_int(int *, char *);
extern char *get_int_safe(int *, char *, file);
extern char *get_int_safe_explicit(int *, char *, const char *, size_t);
extern char *get_string(char *, char *);
extern char *get_string_safe(char *, char *, file);
extern char *get_word(int *, char *);
extern char *getcolor(player *, char);
extern char *gstring(player *);
extern char *gstring_possessive(player *);
extern char *idlingstring(player *, player **, int);
extern char *isare(player *);
extern char *list_lines(list_ent *);
extern char *name_string(player *, player *);
extern char *next_space(char *);
extern char *number2string(int);
extern char *ping_string(player *);
extern char *privs_bit_string(int);
extern char *replace(char *, char *, char *);
extern char *reserved_names[];
extern char *retrieve_alias_data(saved_player *, char *);
extern char *retrieve_item_data(saved_player *, char *);
extern char *retrieve_list_data(saved_player *, char *);
extern char *retrieve_mail_data(saved_player *, char *);
extern char *retrieve_room_data(saved_player *, char *);
extern char *self_string(player *);
extern char *significant_time(int);
extern char *single_replace(char *, char *, char *);
extern char *splice_argument(player *, char *, char *, int);
extern char *store_int(char *, int);
extern char *store_string(char *, char *);
extern char *store_word(char *, int);
extern char *sys_room_id(char *);
extern char *sys_time(void);
extern char *tag_string(player *, player **, int);
extern char *their_player(player *);
extern char *time_diff(int);
extern char *time_diff_sec(time_t, int);
extern char *unlogged_sites[];
extern char *upper_from_saved(saved_player *);
extern char *word_time(int);
extern command_func newbie_dummy_fn;
extern command_func newbie_start;
extern player_func login_timeout;
extern dfile *dynamic_init(char *, int);
extern file load_file(char *);
extern file load_file_verbose(char *, int);
extern int check_duty ( player * );
extern int check_password(char *, char *, player *);
extern int check_privs(int, int);
extern int count_caps(char *);
extern int count_unread_news(time_t);
extern int count_unread_snews(time_t);
extern int count_owners(struct s_item *);
extern int count_su(void);
extern int decompress_room(room *);
extern int dynamic_load(dfile *, int, char *);
extern int dynamic_save(dfile *, char *, int, int);
extern int emote_no_break(char);
extern int find_spodlist_position(char *);
extern int get_case_flag(flag_list *, char *);
extern int get_flag(flag_list *, char *);
#ifdef ALLOW_MULTIS
extern int global_tag(player *, char *, int);
#else
extern int global_tag(player *, char *);
#endif
extern int isit_hour(int);
extern int ishcadmin(char *);
extern int load_player(player *);
extern int local_tag(player *, char *);
extern int match_banish(player *, char *);
extern int match_gag(player *, player *);
extern int match_player(char *, char *);
extern int people_in_spodlist(void);
extern int possible_move(player *, room *, int);
extern int priv_for_log_str ( char * );
extern int remove_player_file(char *);
extern int reserved_name(char *);
extern int restore_player(player *, char *);
extern int restore_player_title(player *, char *, char *);
extern int save_file(file *, char *);
extern int saved_tag(player *, char *);
extern int send_to_room(player *, char *, char *, int);
extern int strnomatch(char *, char *, int);
extern int test_receive(player *);
extern int true_count_su(void);
extern int unlogged_site(char *);
extern list_ent *find_list_entry(player *, char *);
extern list_ent *fle_from_save(saved_player *, char *);
extern note *find_note(int);
extern note *find_note(int);
extern player *create_player(void);
extern player *find_player_absolute_quiet(char *);
extern player *find_player_global(char *);
extern player *find_player_global_quiet(char *);
extern player *find_screend_player (char *);
extern room *convert_room(player *, char *);
extern room *create_room(player *);
extern saved_player *find_saved_player(char *);
extern saved_player *find_top_player(char, int);
extern ssize_t write_socket(int *, const void *, size_t);
extern struct terminal terms[];
extern void ADDSTACK(char *,...) VARG_END1
extern void AUWALL(char *,...) VARG_END1
extern void AW_BUT(player *, char *,...) VARG_END2
extern void ENDSTACK(char *,...) VARG_END1
extern void HCWALL(char *,...) VARG_END1
extern void LOGF(char *, char *,...) VARG_END2
extern void SEND_TO_DEBUG(char *,...) VARG_END1
extern void SUWALL(char *,...) VARG_END1
extern void SW_BUT(player *, char *,...) VARG_END2
extern void TELLPLAYER(player *, char *,...) VARG_END2
extern void TELLROOM(room *, char *,...) VARG_END2
extern void TELLROOM_BUT(player *, room *, char *,...) VARG_END3
extern void actual_timer(int);
extern void add_to_spodlist(char *, int, float);
extern void alive_connect(void);
extern void all_players_out(saved_player *);
extern void au_wall(char *);
extern void au_wall_but(player *, char *);
extern void banlog(char *, char *);
extern void beep_tell(player *, char *);
extern void begin_ressie(player *, char *);
extern void blocktells(player *, char *);
extern void calc_spodlist(void);
extern void check_clothing(player *);
extern void check_idle (player *, char *);
extern void check_list_newbie(char *);
extern void check_list_resident(player *);
extern void check_special_effect(player *, item *, struct s_item *);
extern void cleanup_tag(player **, int);
extern void clear_gag_logoff(player *);
extern void close_down(void);
extern void close_down_socket(void);
extern void comments(player *, char *);
extern void compress_room(room *);
extern void connect_to_prog(player *);
extern void construct_alias_save(saved_player *);
extern void construct_item_save(saved_player *);
extern void construct_list_save(saved_player *);
extern void construct_mail_save(saved_player *);
extern void construct_name_list(player **, int);
extern void construct_room_save(saved_player *);
extern void create_banish_file(char *);
extern void debug_wall(char *);
extern void decompress_alias(saved_player *);
extern void decompress_item(saved_player *);
extern void decompress_list(saved_player *);
extern void delete_item_from_all_players(int, int);  
extern void delete_received(player *, char *);
extern void delete_sent(player *, char *);
extern void destroy_player(player *);
extern void do_alive_ping(void);
extern void do_backup(player *, char *);
extern void do_birthdays(void);
extern void do_inform(player *, char *);
extern void do_prompt(player *, char *);
extern void do_update(int);
extern void dynamic_defrag_rooms(player *, char *);
extern void dynamic_free(dfile *, int);
extern void dynamic_validate_rooms(player *, char *);
extern void earmuffs(player *, char *);
extern void echo_shout (player *, char *);
extern void emote (player *, char *);
extern void emote_shout (player *, char *);
extern void end_emergency ( player * );
extern void extract_pipe_global(char *);
extern void extract_pipe_local(char *);
extern void finish_edit(player *);
extern void fork_the_thing_and_sync_the_playerfiles(void);
extern void free_room_data(saved_player *);
extern void get_hardware_info(void);
extern void get_post_data(player *, char *);
extern void got_name(player * p, char *str);
extern void handle_error(char *);
extern void handle_get_request(player *, char *);
extern void handle_post_request(player *, char *);
extern void hard_load_one_file(char);
extern void hc_wall(char *);
extern void hitells(player *, char *);
extern void init_help(void);
extern void help ( player *, char * );
#ifdef IDENT
extern void init_ident_server(void);
extern void kill_ident_server(void);
#endif
extern void init_notes(void);
extern void init_parser(void);
extern void init_plist(void);
extern void init_rooms(void);
extern void init_sitems(void);
extern void init_socket(int);
extern void init_tracer(void);
extern void init_messages(void);
extern void initchannels(void);
extern void input_for_one(player *);
extern void log(char *, char *);
extern void look(player *, char *);
extern void lower_case(char *);
extern void lscripto (char *, char *, int);
extern void lsu (player *, char *);
extern void make_reply_list(player *, player **, int);
extern void make_resident(player *);
extern void match_commands(player *, char *);
extern void move_to(player *, char *, int);
extern void newbie_was_screend (player *);
extern void newfinger(player *, char *);
extern void newsetpw0(player *, char *);
extern void newsetpw1(player *, char *);
extern void non_block_tell(player *, char *);
extern void pager(player *, char *);
extern void password_mode_off(player *);
extern void password_mode_on(player *);
extern void pg_version(player *, char *);
extern void ping_respond(player *);
extern void ping_timed(player *);
extern void premote(player *, char *);
extern void process_players(void);
extern void process_review(player *, char *, int);
extern void prs_record_display(player *);
extern void pstack_mid(char *);
extern void purge_gaglist(player *, char *);
extern void quit(player *, char *);
extern void quit_pager(player *p, ed_info *);
extern void qwho(player *, char *);
extern void raw_room_wall(room *, char *);
extern void raw_wall(char *);
extern void read_sent(player *, char *);
extern void recho(player *, char *);
extern void remote(player *, char *);
extern void remote_cmd(player *, char *, int);
extern void remote_think(player *, char *);
extern void remove_entire_list(char);
extern void resident(player *, char *);
extern void rsing(player *, char *);
extern void save_all(void);
extern void save_player(player *);
extern void scan_sockets(void);
extern void sendtofile(char *, char *);
extern void set_talker_addy (void);
extern void set_update(char);
extern void set_yes_session(player *, char *);
extern void show_logs(player *, char *);
extern void sing_shout (player *, char *);
extern void snews_command(player *, char *);
extern void soft_eject(player *, char *);
extern void spodlist(void);
extern void start_edit(player *, int, player_func *, player_func *, char *);
extern void su_wall(char *);
extern void su_wall_but(player *, char *);
extern void sub_command(player *, char *, struct command *);
extern void swap_list_names(char *, char *);
extern void swho(player *, char *);
extern void sync_all(void);
extern void sync_notes(int);
extern void sync_sitems(int);
extern void sync_to_file(char, int);
extern void toggle_view_clock(player *, char *);
extern void tell(player *, char *);
extern void tell_current(char *);
extern void tell_multip(player *, char *);
extern void tell_player(player * p, char *);
extern void tell_room(room *, char *);
extern void tell_room_but(player *, room *, char *);
extern void tell_room_but2(player *, room *, char *);
extern void timeout_items(void);
extern void timer_function(void);
extern void ttt_abort(player *, char *);
extern void to_room1(room *, char *, player *);
extern void to_room2(room *, char *, player *, player *);
extern void toggle_yes_dynatext(player *, char *);
extern void trans_to(player *, char *);
extern void view_session(player *, char *);
extern void view_sub_commands(player *, struct command *);
extern void warn(player *, char *);
extern void who_on_chan(player *, int);
struct s_item *timeout_this_item(struct s_item *);

#ifndef BSDISH 
   #if !defined(linux)
     extern char *crypt(char *, char *);
   #endif
#endif

#ifdef INTERCOM
extern int   intercom_fd,intercom_pid,intercom_port,intercom_last;
extern room *intercom_room;

extern void do_intercom_examine(player *, char *);
extern void do_intercom_finger(player *, char *);
extern void do_intercom_idle(player *, char *);
extern void do_intercom_lsu(player *, const char *);
extern void do_intercom_remote(player *, char *);
extern void do_intercom_tell(player *, char *);
extern void do_intercom_who(player *, char *);
extern int establish_intercom_server(void);
extern void kill_intercom(void);
extern void parse_incoming_intercom(void);
extern void intercom_room_look(player *);
extern void do_intercom_say(player *, char *);
extern void do_intercom_emote(player *, char *);
extern void do_intercom_think(player *, char *);
extern void newfinger(player *, char *);
extern void newexamine(player *, char *);
extern void start_intercom(void);
extern void kill_intercom(void);
extern void pg_intercom_version(void);
#endif

#ifdef ROBOTS
extern void init_robots(void);
extern void process_robots(void);
extern void process_robot_counters(void);
extern void trans_fn(player *, char *);
#endif

#ifdef REDHAT5
extern char *crypt(const char *,const char *);
#endif

/* These are all used for pg_version */
extern void crashrec_version(void);
extern void channel_version(void);
extern void command_stats_version(void);
extern void last_version(void);
extern void socials_version(void);
extern void dynatext_version(void);
extern void robot_version(void);
extern void slots_version(void);
extern void swear_version(void);
extern void spodlist_version(void);
extern void nunews_version(void);
#ifdef SEAMLESS_REBOOT
extern void reboot_version(void);
#endif
#ifdef IDENT
extern void ident_version(void);
#endif
extern void softmsg_version(void);
extern void change_command_privs(player *, char *);
#ifdef INTELLIBOTS
extern void intelli_version(void);
#endif
#ifdef ALLOW_MULTIS
extern void multis_version(void);
#endif

#ifdef SEAMLESS_REBOOT
extern int possibly_reboot ( void );
extern int awaiting_reboot;
extern void do_reboot(void);
#endif

char *filter_rude_words(char *);
extern const char *swear_words[];
extern const int num_swear_words;

#ifdef COMMAND_STATS
extern void commandUsed(char *);
#endif

#ifdef AUTOSHUTDOWN
extern int auto_sd;
#endif

#ifdef LAST
extern void stampLogout(char *);
#endif

extern void          connect_channels (player *);
extern void          disconnect_channels (player *);
extern int           tell_command_channel (player *, char *, char *);
extern int           remote_command_channel (player *, char *, char *);

/* NuNews externs */
extern command_func  news_command;
extern int           news_sync;
extern void          init_news ( void );
extern void          new_news_inform ( player * p );
extern void          sync_all_news_headers ( void );
extern void          scan_news ( void );

/* xstring externs */
extern char      *strcasestr ( char *, char * );
extern char      *strcaseline ( char *, char * );
extern char      *strline ( char *, char * );
extern char      *single_replace ( char *, char *, char * );
extern char      *replace ( char *, char *, char * );
extern char      *linestr ( char *, char * );
extern char      *lineval ( char *, char * );

/* soshuls externs */
extern void       init_socials ( void );
extern int        list_socials ( player * );
extern int        match_social ( player *, char * );

/* historee externs */
extern history     *init_hist ( int );
extern void         add_to_hist ( history *, char * );
extern int          stack_hist ( history *, int, char * );
extern void         clean_hist ( history * );
extern void         free_hist ( history * );
extern void         init_global_histories ( void );

/* soft message externs */
extern char        *get_config_msg ( char * );
extern char        *get_frog_msg ( char * );
extern char        *get_admin_msg ( char * );
extern char        *get_shutdowns_msg ( char * );
extern char        *get_plists_msg ( char * );
extern char        *get_pdefaults_msg ( char * );
extern char        *get_session_msg ( char * );
extern char        *get_deflog_msg ( char * );
extern char        *get_rooms_msg ( char * );


extern void grant_me(player *, char *);

#ifdef INTELLIBOTS
extern int          intelligent_robot(player *, player *, char *, int);
#endif

#ifdef ALLOW_MULTIS
extern multi       *all_multis;

extern void         init_multis (void);
extern void         destroy_all_multis (void);
extern void         multi_change_entries (player *p1, player *p2);
extern int          get_number (int real_number);
extern int          get_bit (int real_number);
extern int          get_real_number (int number, int bit);
extern int          multi_get_new_number (void);
extern void         tell_multi (int number, player *p, char *str);
extern void         vtell_multi (int number, player *p, char *format, ...);
extern multi       *remove_multi (multi *m1, multi *m2, char *reason);
extern void         update_multis (void);
extern multi       *find_multi_from_number (int number);
extern int          find_friend_multi_number (player *);
extern int          find_friend_multi_number_name (char *pl_name);
extern int          player_on_multi (player *p, multi *m);
extern int          players_on_multi (multi *m);
extern void         remove_from_multis (player *p);
extern void         remove_from_multi (player *p, multi *m);
extern void         add_to_multi (player *p, multi *m);
extern void         add_to_friends_multis (player *p);
extern int          multi_count (void);
extern int          assign_friends_multi (player *p);
extern void         create_friends_multi (player *p, int number);
extern void         multi_tell (player *p, char *str, char *msg);
extern void         multi_remote (player *p, char *str, char *msg);
extern void         multi_rsing (player *p, char *str, char *msg);
extern void         multi_recho (player *p, char *str, char *msg);
extern void         multi_yell (player *p, char *str, char *msg);
extern void         multi_block (player *p, char *str);
extern void         multi_list (player *p, char *str);
extern void         multi_idle (player *p, char *str);

#ifdef SEAMLESS_REBOOT
extern int          reboot_save_multis (void);
extern void         reboot_load_multis (void);
#endif /* SEAMLESS_REBOOT */

#endif /* ALLOW_MULTIS */








  /** varible prototypes **/

extern char    *stack, *stack_start;
extern char    *action;
extern char     talker_name[], talker_email[], talker_alpha[], talker_ip[];
extern char     longest_player[];
extern char     sess_name[], session[];
extern char     suhistory[SUH_NUM][MAX_SUH_LEN];
extern char     shutdown_reason[];

extern int      sys_flags, max_players, current_players, command_type, up_time,
                up_date, logins, sys_color_atm, max_ppl_on_so_far, config_flags;
extern int      pipe_matches, backup, active_port;
extern int      splat1, splat2, splat_timeout;
extern int      soft_splat1, soft_splat2, soft_timeout;
extern int      in_total, out_total, in_current, out_current, in_average,
                out_average, net_count, in_bps, out_bps, in_pack_total,
                out_pack_total, in_pack_current, out_pack_current, in_pps,
                out_pps, in_pack_average, out_pack_average;
extern int      backup_hour, backup_time, longest_time, pot, reboot_date,
                session_reset, shutdown_count, social_index;
extern int      update[];


extern player  *flatlist_start, *hashlist[], *current_player, *p_sess,
               *stdout_player, *input_player, *otherfriend, *this_rand;
extern player **pipe_list;

extern saved_player **saved_hash[];

extern room    *comfy, *boot_room, *relaxed, *current_room, *entrance_room, 
               *prison;

extern struct command check_list[], editor_list[], keyroom_list[], mail_list[],
                      news_list[], room_list[], *coms[];


extern file     idle_string_list[];
extern file     newban_msg, nonewbies_msg, connect_msg, motd_msg, version_msg,
                quit_msg, nuke_msg, sneeze_msg, banned_msg, banish_file,
                banish_msg, full_msg, newbie_msg, newpage1_msg, newpage2_msg,
                disclaimer_msg, splat_msg, connect2_msg, connect3_msg, rude_msg,
                noressies_msg, hitells_msg, sumotd_msg,
                fingerpaint_msg;
extern file     config_msg, admin_msg, shutdowns_msg, frogs_msg, plists_msg, 
                pdefaults_msg, session_msg, deflog_msg, rooms_msg;




