#define stSIMPLE		(1<<1)
#define stCOMPLEX		(1<<2)
#define stPRIVATE		(1<<3)
#define stINC			(1<<10)

#define atROOM_MSG		1
#define atUSED_ROOM_MSG		2
#define atDIRECT_MSG		3
#define atUSED_DIRECT_MSG	4
#define atFORMAT		5
#define atDONE			6
#define atABORT			7


file       SocialTypes[] =
{
   {"simple", stSIMPLE},
   {"complex", stCOMPLEX},
   {"private", stPRIVATE},
   {"", 0}
};



struct   simple_social
{
   char     *command;
   char     *outmsg;
   char     *workmsg;
}
SimpleSocials[] =
{
   {"afk",
    "goes afk.",
    " You go afk.\n"},
   
   {"quiver",
    "quivers softly.",
    " You quiver softly.\n"},
    
    {"", "", ""}
};
typedef struct simple_social simple_social;


struct   compound_social
{
   char     *command;
   char     *nostr_outmsg;
   char     *nostr_workmsg;
   char     *str_outmsg;
   char     *str_workmsg;
}
CompoundSocials[] =
{
   {"smile",
    "smiles happily.",
    " You smile happily.\n",
    "smiles happily at you.",
    " You smile at %s, big and good.\n"},
    
    {"", "", ""}
};
typedef struct compound_social compound_social;


struct   private_social
{
   char     *command;
   char     *format;
   char     *outmsg;
   char     *workmsg;
}
PrivateSocials[] =
{
   {"snog", 
    " Yes, great, but you have to actually have a snogee.\n",
    "snogs you, leaving you breathless and wanting more.",
    " You snog on %s.\n"},
    
    {"spank",
    " You spank yourself?\n",
    "spanks you til your bottom is all pink!.",
    " You spank %s ferverishly.\n"},
    
     {"", "", "", ""}
};
typedef struct private_social private_social;

