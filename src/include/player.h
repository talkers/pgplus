/*
 * Playground+ - player.h
 * Player defines
 * ---------------------------------------------------------------------------
 */

#include <sys/time.h>

/* kludgy macros, there must be a better way to do this */

#define align(p)	p=(void *)(((int)p+3)&-4)
#define CHECK_DUTY(p)	if (!(check_duty((p)))) return;
#define FORMAT(ck,s)	if (!(*(ck)))  { tell_player(current_player, (s)); return; }
#define HAS_PRIV(p,r)	((p)->residency & (r))
#define RES_BIT_HEAD	"Sys |Ressie|MiscRes |Misc|Staff"

#ifdef ROBOTS
#define CHECKROBOT(p)   if (!p->residency & ROBOT_PRIV) \
                        { tell_player(p," This is a command for robots only.\n"); \
                        return; }
#ifdef INTELLIBOTS
#define IR_PRIVATE	1
#define IR_ROOM		2
#define IR_SHOUT	4

/* Number of inteligent robots */

#define NUM_IROBOTS 1

/* Max number of intelligent robots phrases to catch */

#define MAX_IROBOT_PHRASES 20
#endif
#endif

/* modes that players can be in */

#define NONE		0
#define PASSWORD	(1<<0)
#define CONV		(1<<1)
#define ROOMEDIT	(1<<2)
#define MAILEDIT	(1<<3)
#define NEWSEDIT	(1<<4)
#define SUNEWSEDIT	(1<<5)

/* gender types */

#define OTHER		0
#define MALE		1
#define FEMALE		2
#define VOID_GENDER	-1

/* residency types */

#define BANISHED	-1
#define NON_RESIDENT	0
#define BASE		1
#define NO_SYNC		(1<<1)  /* will not save */
#define ECHO_PRIV	(1<<2)  /* can use echo command */
#define NO_TIMEOUT	(1<<3)  /* will not timeout */
#define BANISHD		(1<<4)  /* cannot login */
#define SYSTEM_ROOM	(1<<5)  /* not real player, a system room owner */
#define MAIL		(1<<6)  /* can use mail commands */
#define LIST		(1<<7)  /* can use list commands */
#define BUILD		(1<<8)  /* can use room commands */
#define SESSION		(1<<9)  /* can set the session */
#define SPOD		(1<<10) /* spod channel access */
#define SPECIALK	(1<<11) /* can create socials */
#define DEBUG		(1<<12) /* can view the debug channel */
#define NONCLERGY	(1<<13) /* unused, left for backward compatiblity */
#define PSU		(1<<14) /* can use su channel commands */
#define WARN		(1<<15) /* can use 'warn' */
#define ADC		(1<<16) /* can use admin channel commands */
#define HOUSE		(1<<17) /* extra 'fun' priv */
#define ASU		(1<<18) /* advanced su, can use advanced su commands */
#define MINISTER	(1<<19) /* can do marriages and such */
#define DUMB		(1<<20) /* can use dumb (frog) commands */
#define ROBOT_PRIV	(1<<21) /* is a robot */
#define SCRIPT		(1<<22) /* can use extended scripting */
#define TRACE		(1<<23) /* can see sites and use site type commands */
#define CODER		(1<<24) /* not really used, backward compatiblity */
#define BUILDER		(1<<25) /* can create items */
#define LOWER_ADMIN	(1<<26) /* lower admin, can use minor admin commands */
#define HCADMIN		(1<<27) /* hard coded admin, not Truly, highest priv */
#define PROTECT		(1<<28) /* protected from su commands */
#define SU		(1<<29) /* superuser, can use normal su commands */
#define ADMIN		(1<<30) /* administrator, can use admin commands */
#define GIT		(1<<31) /* gitgitgit, bad player */

/* Inform bits and bobs */

#define INFORM_SU	(1<<1)
#define INFORM_ROBOT	(1<<2)
#define INFORM_NEWBIE	(1<<3)
#define INFORM_FRIEND	(1<<4)


 /* these are the privs that arent counted when privs are compared */
#define NONSU (BASE + ECHO_PRIV + NO_TIMEOUT + MAIL + LIST + BUILD + SESSION \
               + SCRIPT + TRACE + DUMB + HOUSE + SPOD + WARN + BUILDER + \
               MINISTER + SPECIALK + DEBUG + GIT)

 /* privs a hard coded (in admin.h) player gets on login */
 /* Note, CODER isnt here...to have hcadmins seperate from coders */
#define HCADMIN_INIT (HCADMIN + ADMIN + LOWER_ADMIN + ASU + SU + SPOD  \
                     + TRACE + SCRIPT + DUMB + WARN + PSU + SESSION + SPECIALK \
                     + BUILD + LIST + MAIL + NO_TIMEOUT + ECHO_PRIV + BASE \
                     + MINISTER + BUILDER)

/* #define lengths */

 /* safe to change these arbitilary */
#define MAX_HISTORY_LINES 	8
#define IBUFFER_LENGTH		512
#define MAX_REVIEW		1000
#define MAX_SESSION		60
#define MAX_COMMENT		59
#define MAX_ARTICLE_SIZE	5000
#define MAX_SUH_LEN		2048

/* dont change these less you know what and why you are doing */

#define MAX_NAME		20
#define MAX_INET_ADDR		40
#define MAX_PROMPT		15
#define MAX_ID			15
#define MAX_EMAIL		60
#define MAX_PASSWORD		20
#define MAX_TITLE		65
#define MAX_DESC		300
#define MAX_ALIAS		300
#define MAX_PLAN		300
#define MAX_PRETITLE		19
#define MAX_ENTER_MSG		65
#define MAX_IGNOREMSG		65
#define MAX_REPLY		200
#define MAX_ROOM_CONNECT	35
#define MAX_SPODCLASS		45
#define MAX_ROOM_NAME		50
#define MAX_AUTOMESSAGE		300
#define MAX_ROOM_SIZE		1500
#define MAX_MQUIT		400
#define MAX_SWARN		400
#define MAX_LAST_NEWS_INTS	50
#define MAX_CHANNELS		50
#define MAX_REMOTE_USER		40

/* system flag definitions */

#define PANIC			(1<<0)
#define VERBOSE			(1<<1)
#define SHUTDOWN		(1<<2)
#define EVERYONE_TAG		(1<<3)
#define FAILED_COMMAND		(1<<4)
#define PIPE			(1<<5)
#define CLOSED_TO_NEWBIES	(1<<6)
#define ROOM_TAG		(1<<7)
#define FRIEND_TAG		(1<<8)
#define DO_TIMER		(1<<9)
#define UPDATE			(1<<10)
#define NO_PRINT_LOG		(1<<11)
#define NO_PRETITLES		(1<<12)
#define UPDATEROOMS		(1<<13)
#define UPDATEFLAGS		(1<<14)
#define NEWBIE_TAG		(1<<15)
#define REPLY_TAG		(1<<16)
#define SECURE_DYNAMIC		(1<<17)
#define UPDATE_SPODLIST		(1<<18)
#define ITEM_TAG		(1<<19)
#define OFRIEND_TAG		(1<<20)
#define UPDATE_URLS		(1<<21)
#define CLOSED_TO_RESSIES	(1<<22)
#define UPDATE_INT_DATA		(1<<23)
#define SCREENING_NEWBIES	(1<<24)

/* config type flags (as set in soft/config.msg) */

#define cfUSUK			(1<<0)    /* time_format */
#define cfNOSPAM		(1<<1)    /* nospam */
#define cfPRIVCHANGE		(1<<2)    /* privs_change */
#define cfNOSWEAR		(1<<3)    /* swear_filter */
#define cfFINGERLOGIN		(1<<4)    /* login_finger */
#define cfWALLPROPOSE		(1<<5)    /* wall_propose */
#define cfWALLMARRIGE		(1<<6)    /* wall_marriage */
#define cfWALLREJECT		(1<<7)    /* wall_reject */
#define cfWALLDIVORCE		(1<<8)    /* wall_divorce */
#define cfWALLACCEPT		(1<<9)    /* wall_accept */
#define cfVEGASLOTS		(1<<10)   /* vegas_style */
#define cfIDLEBAD		(1<<11)   /* idle_bad */
#define cfADMINIDLE		(1<<12)   /* admin_ok_idle */
#define cfFORCEABLE		(1<<13)   /* can_force */
#define cfMULTI_INFORM		(1<<14)   /* multi_inform */
#define cfNOSUNONEW		(1<<15)   /* no_sus_no_new */
#define cfMULTIVERBOSE		(1<<16)   /* verbose_multis */
#define cfSELFRES		(1<<17)   /* allow_selfres */
#define cfUSETRUESPOD		(1<<18)   /* use_truespod */
#define cfRECONNECTLOOK		(1<<19)   /* reconnect_look */
#define cfCANEDITFILES		(1<<20)   /* can_edit_files */
#define cfSHOWXED		(1<<21)   /* show_when_xd */
#define cfWELCOMEMAIL		(1<<22)   /* do_email_check */
#define cfCAPPEDNAMES		(1<<23)   /* capped_names */
#define cfSUSCANRECAP		(1<<24)   /* sus_can_recap */

/* player flag defs */

/* uses sys_flags #define PANIC (1<<0) */
#define INPUT_READY		(1<<1)
#define LAST_CHAR_WAS_N		(1<<2)
#define LAST_CHAR_WAS_R		(1<<3)
#define DO_LOCAL_ECHO		(1<<4)
#define PASSWORD_MODE		(1<<5)
/* uses sys_flag #define CLOSED_TO_NEWBIES (1<<6) */
#define PROMPT			(1<<7)
#define TAGGED			(1<<8)
#define LOGIN			(1<<9)
#define CHUCKOUT		(1<<10)
#define EOR_ON			(1<<11)
#define IAC_GA_DO		(1<<12)
#define SITE_LOG		(1<<13)
#define DONT_CHECK_PIPE		(1<<14)
#define RECONNECTION		(1<<15)
#define NO_UPDATE_L_ON		(1<<16)
#define BLOCK_SU		(1<<17)
#define NO_SAVE_LAST_ON		(1<<18)
#define NO_SU_WALL		(1<<19)
#define ASSISTED		(1<<20)
#define FROGGED			(1<<21)
#define SCRIPTING		(1<<22)
#define OFF_LSU			(1<<23)
#define UNUSED			(1<<24)
#define UNUSED2			(1<<25)
#define WAITING_ENGAGE		(1<<26)
#define ROBOT			(1<<27)
#define IN_EDITOR		(1<<28)

/* ones that get saved */
/* lower block, system flags */

#define SAVENOSHOUT		(1<<0)
#define SAVEDFROGGED		(1<<1)
#define SAVE_NO_SING		(1<<2)
#define SAVE_LAGGED		(1<<3)
#define DECAPPED		(1<<4)
#define SAVEDJAIL		(1<<5)
#define NO_MSGS			(1<<6)
#define SAVED_RM_MOVE		(1<<7)

/* upper block, system flags */

#define FLIRT_BACHELOR		(1<<10)
#define BACHELOR_HIDE		(1<<11)
#define ENGAGED			(1<<12)
#define MARRIED			(1<<13)
#define OLD_MINISTER		(1<<14)
#define NEW_MAIL		(1<<15)
#define COMPRESSED_LIST		(1<<16)
#define COMPRESSED_ALIAS	(1<<17)
#define COMPRESSED_ITEMS	(1<<18) 
#define IAC_GA_ON		(1<<19)
#define AGREED_DISCLAIMER	(1<<20)
#define NEW_SITE		(1<<21)
#define OLD_BUILDER		(1<<22)
#define TIMEPROMPT		(1<<23)

/* lower tag flags */

#define TAG_PERSONAL		(1<<0)
#define TAG_ROOM		(1<<1)
#define TAG_SHOUT		(1<<2)
#define TAG_LOGINS		(1<<3)
#define TAG_ECHO		(1<<4)
#define SEEECHO			(1<<5)
#define TAG_AUTOS		(1<<6)
#define TAG_ITEMS		(1<<7)
#define TAG_DYNATEXT		(1<<8)

/* upper tag flags */

#define BLOCK_SHOUT		(1<<10)
#define BLOCK_TELLS		(1<<11)
#define BLOCK_FRIENDS		(1<<12)
#define BLOCK_ECHOS		(1<<13)
#define BLOCK_ROOM_DESC		(1<<14)
#define BLOCK_FRIEND_MAIL	(1<<15)
#define SINGBLOCK		(1<<16)
#define BLOCKCHANS		(1<<17)
#define NO_FACCESS		(1<<18)
#define NOBEEPS			(1<<19)
#define BLOCK_AUTOS		(1<<20)
#define NO_ANONYMOUS		(1<<21)
#define NO_PROPOSE		(1<<22)
#define BLOCK_ITEMS		(1<<23)
#define BLOCK_LOGINS		(1<<24)
#define BLOCK_BOPS		(1<<25)
#ifdef ALLOW_MULTIS
#define BLOCK_MULTIS		(1<<26)
#endif

/* custom flags, lower */

#define HIDING			(1<<0)
#define PRIVATE_EMAIL		(1<<1)
#define PUBLIC_SITE		(1<<2)
#define FRIEND_SITE		(1<<3)
#define FRIEND_EMAIL		(1<<4)

/* upper custom flags */

#define TRANS_TO_HOME		(1<<10)
#define MAIL_INFORM		(1<<11)
#define NEWS_INFORM		(1<<12)
#define NOPREFIX		(1<<13)
#define NOEPREFIX		(1<<14)
#define YES_SESSION		(1<<15)
#define NO_PAGER		(1<<16)
#define ROOM_ENTER		(1<<17)
#define YES_QWHO_LOGIN		(1<<18)
#define SHOW_EXITS		(1<<19)
#define CONVERSE		(1<<20)
#define QUIET_EDIT		(1<<21)
#define NO_CLOCK		(1<<22)
#define NO_DYNATEXT		(1<<23)
#define NO_NEW_NEWS_INFORM	(1<<24)

/* misc flags, lower */

#define NO_PRS			(1<<0)
#define NO_GIFT			(1<<1)

/* misc flags, upper */

#define CHAN_HI			(1<<10)
#define SU_HILITED		(1<<11)
#define NOCOLOR			(1<<12)
#define SYSTEM_COLOR		(1<<13)
#define GAME_HI			(1<<14)
#define STOP_BAD_COLORS		(1<<15)
#define NO_MAIN_CHANNEL		(1<<16)
#define NO_SPOD_CHANNEL		(1<<17)
#define SEE_DEBUG_CHANNEL	(1<<18)

/* list flags */

#define NOISY			(1<<0)
#define IGNORE			(1<<1)
#define INFORM			(1<<2)
#define GRAB			(1<<3)
#define FRIEND			(1<<4)
#define BAR			(1<<5)
#define INVITE			(1<<6)
#define BEEP			(1<<7)
#define BLOCK			(1<<8)
#define KEY			(1<<9)
#define FIND			(1<<10)
#define FRIENDBLOCK		(1<<11)
#define MAILBLOCK		(1<<12)
#define SHARE_ROOM		(1<<13)
#define NO_FACCESS_LIST		(1<<14)
#define PREFERRED		(1<<15)

/* command types */

#define VOID			0
#define SEE_ERROR		(1<<0)
#define PERSONAL		(1<<1)
#define ROOM			(1<<2)
#define EVERYONE		(1<<3)
#define ECHO_COM		(1<<4)
#define EMERGENCY		(1<<5)
#define AUTO			(1<<6)
#define HIGHLIGHT		(1<<7)
#define NO_P_MATCH		(1<<8)
#define TAG_INFORM		(1<<9)
#define LIST_EVERYONE		(1<<10)
#define ADMIN_BARGE		(1<<11)
#define SORE			(1<<12)
#define WARNING			(1<<13)
#define EXCLUDE			(1<<14)
#define BAD_MUSIC		(1<<15)
#define NO_HISTORY		(1<<16)
#define LOGIN_TAG		(1<<17)
#define LOGOUT_TAG		(1<<18)
#define RECON_TAG		(1<<19)
#ifdef ALLOW_MULTIS
#define MULTI_COM		(1<<20)
#endif
#define BLOCK_ALL_CHANS		(1<<21)
#define HI_CHANS		(1<<22)


/* color modes */

#define TELsc			0
#define SUCsc			1
#define ADCsc			2
#define FRTsc			3
#define ROMsc			4
#define SHOsc			5
#define UCEsc			6
#define UCOsc			7
#define SYSsc			8
#define ZCHsc			9

/* For TTT */

#define TTT_MY_MOVE (1<<18)
#define TTT_AM_NOUGHT (1<<19)

/* definitions and structures needed by the multi code */

#ifdef ALLOW_MULTIS
#define MULTI_FRIENDSLIST	(1<<0)
#define MULTI_NOIDLEOUT		(1<<1)
#define MULTI_WARNED		(1<<2)
#endif

/* room bits */

#define HOME_ROOM		(1<<0)
#define COMPRESSED		(1<<1)
#define AUTO_MESSAGE		(1<<2)
#define AUTOS_TAG		(1<<3)
#define LOCKABLE		(1<<4)
#define LOCKED			(1<<5)
#define OPEN			(1<<6)
#define LINKABLE		(1<<7)
#define KEYLOCKED		(1<<8)
#define CONFERENCE		(1<<9)
#define ROOM_UPDATED		(1<<10)
#define EXITMSGS_OK		(1<<11)
#define SOUNDPROOF		(1<<12)
#define ISOLATED_ROOM		(1<<13)
#define ANTISING		(1<<14)

/* note bits */

#define NEWS_ARTICLE		(1<<0)
#define ANONYMOUS		(1<<1)
#define NOT_READY		(1<<2)
#define SUPRESS_NAME		(1<<3)
#define STICKY_ARTICLE		(1<<4)
#define NOTE_FRIEND_TAG		(1<<5)
#define SUNEWS_ARTICLE		(1<<6)
#define DELETE_ME		(1<<7)

/* command bits */

#define THE_EVIL_Q		(1<<1)
#define COMMc			(1<<2)
#define LISTc			(1<<3)
#define ROOMc			(1<<4)
#define MOVEc			(1<<5)
#define INFOc			(1<<6)
#define SYSc			(1<<7)
#define DESCc			(1<<8)
#define SOCIALc			(1<<9)
#define MISCc			(1<<10)
#define SUPERc			(1<<11)
#define ADMINc			(1<<12)
#define ITEMc			(1<<13)
#define INVISc			(1<<14) /* invis on 'all' or not */
#define SPAMc			(1<<15) /* command causes spam */
#define TAGGEDc			(1<<16) /* command tagged as a match */
#define NOMATCHc		(1<<17) /* command cant be matched */
#define F_SWEARc		(1<<18) /* swear catch anywhere */
#define M_SWEARc		(1<<19) /* swear catch only in main rooms */

/* Function passing */

struct p_struct;
typedef void        player_func (struct p_struct *);
typedef void        command_func (struct p_struct *, char *);


/* the generic_social struct for kRad Soshuls */

typedef struct generic_social
{
   int
       flags,
       date;

   char
       creator[MAX_NAME],
       command[MAX_NAME],
      *format,
      *room_msg,
      *used_room_msg,
      *direct_msg,
      *used_direct_msg;

   struct generic_social
      *next,
      *prev;
} generic_social;


/* the multi structs for segtor's multis/chains */

#ifdef ALLOW_MULTIS
struct multi_player_struct
{
   struct p_struct            *the_player;
   struct multi_player_struct *next_player;
};
typedef struct multi_player_struct multiplayer;

struct multi_struct
{
   int                  number;
   int                  multi_flags;
   int                  multi_idle;
   multiplayer         *players_list;
   struct multi_struct *next_multi;
};
typedef struct multi_struct multi;
#endif


/* for Mantis' reportto and history shit */

struct rev_struct 
{
   char review[MAX_REVIEW];
};

/* files'n'things */

typedef struct
{
   char           *where;
   int             length;
} file;

typedef struct r_struct
{
   char
     name[MAX_ROOM_NAME],
     id[MAX_ID],
     enter_msg[MAX_ENTER_MSG];

   int
     flags,
     data_key,
     auto_count,
     auto_base;
   file
     text,
     exits,
     automessage;

   struct s_struct *owner;
   struct p_struct *players_top;
   struct r_struct *next;
} room;

typedef struct n_struct
{
   char
     header[MAX_TITLE],
     name[MAX_NAME];
   int
     id,
     flags,
     date,
     next_sent,
     read_count;

   file            text;
   struct n_struct *hash_next;
} note;

/* NuNews defs */

typedef struct     newsgroup
{
   char                  *name;           /* name of group */
   int                    required_priv;  /* min priv if any to see group */
   int                    max;            /* max posts per non admin player */
   char                  *desc;           /* desc shown on   news groups */
   char                  *path;           /* path to the dir the group files */
   struct news_header    *top;            /* top of news_header list for group
                                                         (leave 0 in array) */
} newsgroup;


typedef struct    news_header
{
    int          id;                   /* unique id, file name for it */
    char         header[MAX_TITLE];    /* title of the news */
    char         name[MAX_NAME];       /* who posted it */
    char         lastreader[MAX_INET_ADDR];   /* site of who last read it */
    int          date;                 /* when it was posted */
    int          read_count;           /* how many times its been read */
    int          flags;                /* flags for it */
    struct news_header  *next;         /* next header in linked list */
    int          followups;            /* number of followups */
    newsgroup   *group;                /* needed for reference in posting */
}  news_header;


/* history struct */

typedef struct history
{
   char    **hist;
   int       max;
} history;


/* list struct */

typedef struct l_struct
{
   char            name[MAX_NAME];
   int             flags;
   struct l_struct *next;
} list_ent;


/* alias struct */

typedef struct al_struct
{
	char 	cmd[MAX_NAME];
	char    sub[MAX_DESC];
	struct al_struct *next;
} alias;

/* library alias struct */

typedef struct library_alias
{
	char *command;
	char *alias_string;
	char *description;
	char *author;
	int privs;
} alias_library;


#ifdef ROBOTS
 #include "robot_player.h"
#endif

/* saved player struct */

typedef struct s_struct
{
   char
     lower_name[MAX_NAME],
     last_host[MAX_INET_ADDR],
     email[MAX_EMAIL];
   int             
     last_on,
     residency,
     system_flags,
     misc_flags,
     custom_flags,
     tag_flags,
     pennies,
     mail_sent,
    *mail_received;

   file              data;
   struct l_struct  *list_top;
   struct r_struct  *rooms;
   struct al_struct *alias_top;
   struct p_item    *item_top; 
   struct s_struct  *next;

} saved_player;

/* editor info structure */
typedef struct
{
   char
     *buffer,
     *current;
   int
     max_size,
     size,
     flag_copy,
     sflag_copy,
     tflag_copy,
     cflag_copy,
     mflag_copy;

   player_func    *finish_func;
   player_func    *quit_func;
   command_func   *input_copy;
   void           *misc;
} ed_info;


/* terminal defs */

struct terminal
{
   char           
     *name,
     *bold,
     *off,
     *cls;
};

/* the player structure (ala phypor) */

typedef struct p_struct
{
   int
             /* saved */
          term,                  /* terminal type of player */
          term_width,            /* width in chars of players terminal */
          word_wrap,             /* min length of word wrapped */
          max_rooms,             /* max rooms player may have */
          max_exits,             /* max exits per room of player */
          max_autos,             /* max autos per room of player */
          max_list,              /* max list entries */
          max_mail,              /* max mails player may send */
          max_alias,             /* max aliases a player may have */
          max_items,             /* max items player may create */
          gender,                /* gender int */
          no_shout,              /* seconds til player my shout */
          total_login,           /* total player login, in seconds */
          birthday,              /* birthday player sets for emself */
          age,                   /* age the player sets for emself */
          jetlag,                /* hours +/- of the player from talker time */
          sneezed,               /* timestamp of player sneezed */
          time_in_main,          /* seconds player has spent in main room */
          no_sing,               /* seconds til player my sing again */
          total_idle_time,       /* seconds player has spent idle */
          warn_count,            /* how many times player has been warned */
          eject_count,           /*                                removed */
          idled_out_count,       /*                                idled out */
          booted_count,          /*                                booted */
          num_ressied,           /* how many ressies player has done */
          num_warned,            /* how many warns */
          num_ejected,           /* how many ejections (sneeze,drag) */
          num_rmd,               /* how many rm_shout, sing, etc */
          num_booted,            /* how many ppl player has booted/jailed */
          first_login_date,      /* timestamp when player first logged on */
          prs_record,            /* record wins, loses, ties for prs */
          ttt_board,             /* tic tac toe board */
          ttt_win,               /* tic tac toe wins */
          ttt_loose,             /*             looses;) */
          ttt_draw,              /*             draws */
          icq,                   /* ICQ number */
          news_last[MAX_LAST_NEWS_INTS], /* int array of last read news */

             /* saved in saved_player */
          residency,             /* residency, privs and such */
          system_flags,          /* saved flags of misc system stuff */
          misc_flags,            /* saved flags of misc stuff */
          custom_flags,          /* saved flags for players customization */
          tag_flags,             /* saved flags for players show tags, etc */
          pennies,               /* players money */

             /* non saved */
          fd,                    /* players open file descriptor (socket) */
          hash_top,              /* for which running (not saved) hashlist */
          flags,                 /* nonsaved misc flags for player */
          saved_residency,       /* backup int for residency */
          column,                /* used in output, keeps up with coloumn */
          idle,                  /* seconds since input last recieved */
          shout_index,           /* used to keep up with shouts by player */
          jail_timeout,          /* seconds before player may leave jail */
          no_move,               /* seconds before player may change room */
          lagged,                /* seconds before input from player parsed */
          script,                /* time left for player emergency */
          on_since,              /* time stamp when player logged in */
          reply_time,            /* time til player may reply */
          timer_count,           /* seconds til timer_fn for player is used */
          ibuff_pointer,         /* where in p->ibuffer reading is */
          logged_in,             /* used as bool, if player is logged in */
          mode,                  /* news,mail,etc mode for player */
          last_remote_command,   /* type of commands last used, for repeat */
          idle_index,            /* used to calculate total_idle_time online */
          newbieinform,          /* player gets informed as a newbie to sus */
          password_fail,         /* used to track password attempts at login */
          antispam,              /* used to track player spamming */
          suh_seen,              /* parts of the su history seen by player */
          zchannellisted,        /* how many ppl are on the same zchannel? */
          zcidle,                /* how idle player is on zchannel */
          awaiting_residency,    /* how many seconds til player has res */
          validated_email,       /* player has validated email in res process */
          next_ping,             /* seconds til player gets next ping */
          social_index;          /* seconds player must wait to use a social */

   char
             /* saved */

          name[MAX_NAME],            /* players effective name */
          lower_name[MAX_NAME],      /* refrence name */
          prompt[MAX_PROMPT],        /* normal prompt */
          converse_prompt[MAX_PROMPT],    /* prompt for converse mode */
          email[MAX_EMAIL],          /* players email */
          password[MAX_PASSWORD],    /* crypted password */
          title[MAX_TITLE],          /* title, shown in examine, finger, etc */
          pretitle[MAX_PRETITLE],    /* prefix */
          description[MAX_DESC],     /* description, shown on examine */
          plan[MAX_PLAN],            /* plan, shown on finger */
          enter_msg[MAX_ENTER_MSG],  /* msg seen when entering a room */
          room_connect[MAX_ROOM_CONNECT], /* owner.id of room for login */
          logonmsg[MAX_ENTER_MSG],   /* msg seen in room when player logs in */
          logoffmsg[MAX_ENTER_MSG],  /* msg seen in room when player quits */
          blockmsg[MAX_IGNOREMSG],   /* uneffective, but setable */
          exitmsg[MAX_ENTER_MSG],    /* msg seen when player leaves a room */
          married_to[MAX_NAME],      /* spouse / fiencee */
          irl_name[MAX_NAME],        /* in real life name */
          alt_email[MAX_EMAIL],      /* url for players homepage */
          hometown[MAX_SPODCLASS],   /* location in the world */
          spod_class[MAX_SPODCLASS], /* spod class shown on lss */
          favorite1[MAX_SPODCLASS],  /* favorite for examine */
          favorite2[MAX_SPODCLASS],  /*   '       '     '    */
          favorite3[MAX_SPODCLASS],  /*   '       '     '    */
          colorset[10],              /* color settings for player */
          ressied_by[MAX_NAME],      /* whut su made this player a ressie */
          git_string[MAX_DESC],      /* nasty comments su sets about player */
          git_by[MAX_NAME],          /* su that made this player a git */
          ingredients[MAX_SPODCLASS],/* Made From on examine */
          finger_message[MAX_MQUIT + 2], /* mquit message, shown on finger */
          swarn_sender[MAX_NAME + 2],    /* su that made saved warn */
          swarn_message[MAX_SWARN + 2],  /* saved warn message */

             /* non saved */

          inet_addr[MAX_INET_ADDR],  /* alpha site (somewhere.onda.net) */
          num_addr[MAX_INET_ADDR],   /* ip site (123.45.67.89) */
          idle_msg[MAX_TITLE],       /* message set to show when idle */
          ignore_msg[MAX_IGNOREMSG], /* others see when ignored and try tells */
          comment[MAX_COMMENT],      /* comment on the session, shown on who */
          reply[MAX_REPLY],          /* players in reply list */
          ibuffer[IBUFFER_LENGTH],   /* buffer read from input socket */
          password_cpy[MAX_PASSWORD],/* used to enter password twice */
          script_file[MAX_NAME + 16],/* filename for extended scripting */
          assisted_by[MAX_NAME],     /* su that assisted this player */
          last_remote_msg[MAX_REPLY],/* last msg sent, for repeat */
          zchannel[15],              /* the z channel this player is on */
          proposed_by[MAX_NAME],     /* su making this player a res */
          current_file[MAX_DESC];    /* file that is currently being edited */


   unsigned int
          prs;                      /* current prs game info */

   long
          last_ping;                /* lag time registered by last ping */

   struct timeval
         ping_timer;                /* for ping, when the ping was sent */

   struct p_struct 
         *hash_next,                /* next player in haslist linked list */
         *flat_next,                /* next player in flatlist */
         *flat_previous,            /* previous player in flatlist */
         *room_next,                /* next player in room */
         *ttt_opponent;             /* tic tac toe opponent */

   saved_player
         *saved;                    /* pointer to saved_player of player */
   room
         *location;                 /* pointer to room where player is */

   ed_info
         *edit_info;                /* pointer to editor info struct */

   command_func
         *input_to_fn;              /* pointer to input func, logins etc */

   player_func
         *timer_fn;                 /* pointer to func executed on timer */

   struct rev_struct 
         rev[MAX_HISTORY_LINES];    /* review array for review and reportto */
   
   struct gag_struct 
        *gag_top;                   /* pointer to top of players gags */

   struct command 
        *command_used;              /* command currently executed */

   generic_social
        *social;                    /* social player is creating atm */
         


#ifdef SEAMLESS_REBOOT
    char 
         location_string[MAX_NAME + MAX_ID + 2]; /* For seemless rebooting */
#endif

#ifdef IDENT
    char     
         remote_user[MAX_REMOTE_USER + 2];       /* Return from ident */
    int      
         ident_id;                               /* Ident request id */
#endif

    char
         channels[MAX_CHANNELS][MAX_NAME];       /* channels array */
    int
         dsc_flags;                              /* flags for ds channels */

#ifdef INTELLIBOTS
    int
         robowarns;                              /* warnings from bots */
#endif

} player;


/* flag list struct */

typedef struct
{
   char           *text;
   int             change;
} flag_list;

/* gag struct */

typedef struct gag_struct 
{
   player            *gagged;
   struct gag_struct *next;
} gag_entry;

/* items structs */

struct s_item 
{
	int	id;
	int	sflags;
	int	value;
	char	desc[MAX_TITLE];
	char	name[MAX_NAME];
	char	author[MAX_NAME];
	struct s_item *next;
};

typedef struct p_item
{
	int	id;
	int 	number;
	int 	flags;
	saved_player  *owner;
	struct p_item *loc_next;
	struct p_item *next;
	struct s_item *root;
} item;


/* structure for commands */

struct command
{
   char           *text;
   command_func   *function;
   int             level;
   int             andlevel;
   int             space;
   char           *help;
   int	           section; 

};
