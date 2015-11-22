/*
 * Playground+ - robot_int.c
 * Robot intelligence written and copyright (c) Segtor 1998
 * ---------------------------------------------------------------------------
 *
 *  Modifications:
 *    - Tidily indented
 *    - Prototypes removed since they shadowed proto.h file
 *    - Double spacing removed (soz! hate it!)
 *    - moved config time defines to compile time defines 
 *         for static array sizes (phypor)
 */

/* DO NOT TOUCH THIS VERSION NUMBER */
#define INTELLI_VERSION "0.35.1"

/* The following defines are only used by the example IntelliBots.  You'll
   want to change the bots if you're going to run this code */
#define MAX_ROBOWARNS_SU	10
#define MAX_ROBOWARNS		5
#define MAX_IROBOTS		5
#define MAX_IROBOT_PHRASES	20

/* list of intelligent robots */
struct irobots
{
  char name[MAX_NAME];
  int accfrom;
};

/* Ok, the list of robots.  This looks like
 * {
 *   {"<name>", <what we listen to>}
 * ...
 * The 'what we listen to' can be:
 *   IR_PRIVATE - listen to whispers, tells, etc. private messages
 *   IR_ROOM    - listen to says, excludes and other room messages
 *   IR_SHOUT   - listen to shouts
 */
struct irobots irobot_list[MAX_IROBOTS] =
{
  {"holly", IR_PRIVATE},
  {"angel", (IR_ROOM | IR_SHOUT)}
};


struct irobot_phrases
{
  char text[IBUFFER_LENGTH];
  int hcaction;			/* each robot phrase can have a different hard coded action */
};

/* here is the phrases we listen for.  these are in the format of
 * { -- first defined irobot --
 *   {"<text to listen for>", "<action>"}
 *   ...
 * },
 * { -- next defined irobot --
 * These will let the robots actually react to phrases, and know the action
 * they should use when the recognize a phrase
 */
struct irobot_phrases irobot_phrase_list[MAX_IROBOTS][MAX_IROBOT_PHRASES] =
{
  {
    {" res ", 1},
    {" residency ", 1},
    {" res, please", 2},
    {" residency, please", 2},
      /* enough of {0, 0}'s to fill the MAX_IROBOT_PHRASES */
  },
  {
    {"fuck", 1},
    {"shit", 1},
    {"ass", 1},
    {"pussy", 1},
    {"dick", 1},
    {"cunt", 1},
      /* enough ... */
  }
};


int intelligent_robot(player * p, player * p2, char *str, int frm)
     /* p   = player calling the tell
      * p2  = player being told to
      * str = the string tell got
      * frm = message type
      *
      * return 0 on continue normal tell, !0 on we handled it
      */
{
  int irobot = -1, i = 0, phrase = -1, ir = -1;
  char *oldstack = stack;
  char msg[IBUFFER_LENGTH];
  player *cp = 0;

  if (current_player)
    cp = current_player;


  /* check p2 to see if online */
  if (!p2)
    return 0;

  current_player = p2;
  strcpy(msg, str);
  lower_case(msg);

  if (frm == IR_PRIVATE)
  {
    /* handle private messages */
    /* check that the person told is a robot */
    if (!(p2->residency & ROBOT_PRIV))
    {
      current_player = cp;
      return 0;
    }

    /* scan irobots for robot to do */
    for (i = 0; i < MAX_IROBOTS; i++)
    {
      if (!&irobot_list[i] || !irobot_list[i].name || !irobot_list[i].name[0])
	break;
      if (!(strcasecmp(p2->lower_name, irobot_list[i].name)))
      {
	irobot = i;
	break;
      }
    }

    if (irobot == -1)
    {
      current_player = cp;
      return 0;
    }

    if (!(irobot_list[irobot].accfrom & IR_PRIVATE))
    {
      sprintf(stack, "%s Sorry, but I don't like private messages.", p->name);
      stack = end_string(stack);
      tell(p2, oldstack);
      stack = oldstack;
      current_player = cp;
      return 0;
    }


    /* scan string for keywords */
    for (i = 0; i < MAX_IROBOT_PHRASES; i++)
    {
      if ((strstr(str, irobot_phrase_list[irobot][i].text)) &&
	  (irobot_phrase_list[irobot][i].hcaction != 0))
      {
	phrase = i;
	break;
      }

    }
    if (phrase == -1)
    {
      current_player = cp;
      return 0;
    }


    /* select robot */
    /* PRIVATE coded actions, you'll need to edit these! */
    switch (irobot)
    {
      case 0:			/* first irobot */
	switch (irobot_phrase_list[irobot][phrase].hcaction)
	{
	  case 1:		/* res without please */
	    sprintf(stack, "%s At least you could say 'please'!", p->name);
	    stack = end_string(stack);
	    tell(p2, oldstack);
	    stack = oldstack;
	    break;
	  case 2:		/* res with please */
	    if (p->residency)
	    {
	      sprintf(stack, "%s But you are already a resident!", p->name);
	      stack = end_string(stack);
	      tell(p2, oldstack);
	      stack = oldstack;
	    }
	    else
	    {
	      sprintf(stack, "%s Ok then :)", p->name);
	      stack = end_string(stack);
	      tell(p2, oldstack);
	      stack = oldstack;
	      resident(p2, p->name);
	    }
	}
	break;

      case 1:			/* second irobot */
	break;
    }

    /* return true if all ok */
    current_player = cp;
    return !0;

  }

  else
  {

    /* handle public messages */
    for (irobot = 0; irobot < MAX_IROBOTS; irobot++)

    {

      /* check if we're p2 or not */
      if (strcasecmp(p2->lower_name, irobot_list[irobot].name))

	continue;


      /* do we accept this kind of messages? */
      if (!(irobot_list[irobot].accfrom & frm))

	continue;


      /* did we come in here, trying to check for us */
      ir = -1;

      for (i = 0; i < MAX_IROBOTS; i++)
      {
	if (!&irobot_list[i] || !irobot_list[i].name || !irobot_list[i].name[0])
	  break;

	if (!(strcasecmp(p2->lower_name, irobot_list[i].name)))
	{
	  ir = i;
	  break;
	}
      }
      if (ir != irobot)
	continue;

      phrase = -1;

      /* scan messages */
      for (i = 0; i < MAX_IROBOT_PHRASES; i++)
      {
	if ((strstr(str, irobot_phrase_list[irobot][i].text)) &&
	    (irobot_phrase_list[irobot][i].hcaction != 0))
	{
	  phrase = i;
	  break;
	}
      }

      if (phrase > -1)
      {
	/* go through bot actions */
	/* ROOM and SHOUT actions, you'll need to change these */
	switch (irobot)
	{
	  case 0:		/* first irobot */
	    /* robores does not like public messages */
	    break;
	  case 1:		/* next irobot */
	    if (p->residency & (SU | ASU | LOWER_ADMIN | ADMIN))
	    {
	      sprintf(stack, "%s looks at you and smacks you.  Act like an SU!", p->name);
	      stack = end_string(stack);
	      remote(p2, oldstack);
	      stack = oldstack;
	      p->robowarns++;
	      if (p->robowarns > MAX_ROBOWARNS_SU)
	      {
		p->robowarns = 0;
		sprintf(stack, "%s Try not to swear anymore!", p->name);
		stack = end_string(stack);
		warn(p2, oldstack);
		stack = oldstack;
	      }
	    }
	    else
	    {
	      sprintf(stack, "%s looks at you and frown.  Watch your language!", p->name);
	      stack = end_string(stack);
	      remote(p2, oldstack);
	      stack = oldstack;
	      p->robowarns++;
	      if (p->robowarns > MAX_ROBOWARNS)
	      {
		p->robowarns = 0;
		sprintf(stack, "%s Please, don't swear anymore!", p->name);
		stack = end_string(stack);
		warn(p2, oldstack);

		soft_eject(p2, p->name);

		stack = oldstack;
	      }
	    }
	    break;
	}
      }
    }
    current_player = cp;
    return !0;
  }
}

void intelli_version(void)
{
  sprintf(stack, " -=*> IntelliBots v%s (by Segtor) active.\n", INTELLI_VERSION);
  stack = strchr(stack, 0);
}
