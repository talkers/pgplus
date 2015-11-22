/*
 * Playground+ - robot_plists.c
 * File to set up/handle/process the robots by Slaine
 * --------------------------------------------------------------------------
 *
 *  Permission has been granted to use this code within PG+
 */


#define ROB_VERS "1.2"

robot *robot_start = NULL;

void init_robots(void)
{
  /* ok, lets set up a structure for all this crap :/ */
  /* stored in player.h by the way */
  char *oldstack, *scan, *scan2;
  char nametoget[MAX_NAME];
  char tmpstring[10];
  file robots;
  robot *current_robot = NULL;
  robot *new_robot;
  move *current_move;
  move *new_move;
  saved_player *sp;

  log("boot", "Initialising robots");
  oldstack = stack;
  memset(nametoget, 0, MAX_NAME);
  memset(tmpstring, 0, 10);
  /* ok, lets try to read the robots file *fear* */
  robots = load_file("files/robots");
  if (!robots.where)
  {
    log("error", "Failed to load robots file.");
    stack = oldstack;
    return;
  }
  /* ok, in theory we have the robots file now, so lets try to read them in */
  /* ah, need to set up a global robots pointer i guess */
  robot_start = 0;

  /* now we look at the file, a # is for a comment line */
  /* format is:
     name (lower case)
     speed of operations
     flags
     # bit
     actions
     # bit */
  scan = robots.where;
  while (*scan)
  {
    /* get to the first name */
    while (scan && *scan && !isalpha(*scan))
      scan++;
    if (scan && *scan)
    {				/* ie we found a name i hope */
      /* this is the bit that reads the records as it were in */
      scan2 = nametoget;
      while (isalpha(*scan))
	*scan2++ = *scan++;
      *scan2 = 0;
      scan++;			/* take it past the newline */
      /* got the name, so now go find it */
      sp = find_saved_player(nametoget);
      if (sp)
      {
	/* we found one! */
	new_robot = MALLOC(sizeof(robot));
	if (!robot_start || robot_start == NULL)
	{
	  robot_start = new_robot;
	  current_robot = new_robot;
	}
	else
	{
	  current_robot->next = new_robot;
	  current_robot = new_robot;
	}
	current_robot->next = 0;
	current_robot->actual_player = 0;
	current_robot->moves_top = 0;
	current_robot->max_moves = 0;
	strcpy(current_robot->lower_name, nametoget);
	/* we -should- be on the speed of ops bit now */
	scan2 = tmpstring;
	while (isdigit(*scan))
	  *scan2++ = *scan++;
	*scan2 = 0;
	scan++;
	current_robot->speed = atoi(tmpstring);
	current_robot->counter = 0;
	/* get the flags */
	scan2 = tmpstring;
	while (isdigit(*scan))
	  *scan2++ = *scan++;
	*scan2 = 0;
	scan++;			/* take it to the comment seperator */
	scan += 2;		/* take it to the first comment */
	current_robot->flags = atoi(tmpstring);
	/* fix stupid flags */
	if (current_robot->flags & LOCAL_WANDER)
	{
	  current_robot->flags &= ~WANDER;
	  current_robot->flags &= ~FIXED;
	}
	else if (current_robot->flags & WANDER)
	{
	  current_robot->flags &= ~LOCAL_WANDER;
	  current_robot->flags &= ~FIXED;
	}
	else
	{
	  current_robot->flags &= ~WANDER;
	  current_robot->flags &= ~LOCAL_WANDER;
	  current_robot->flags |= FIXED;
	}

	/* we are on first char of first action */
	if (*scan && *scan != '#')
	{
	  current_robot->moves_top = MALLOC(sizeof(move));
	  current_robot->moves_top->move_string[0] = 0;
	  current_robot->moves_top->next = 0;
	  current_move = current_robot->moves_top;
	  while (*scan && *scan != '#')
	  {
	    current_robot->max_moves++;
	    /* get the action */
	    scan2 = current_move->move_string;
	    while (*scan != '\n')
	      *scan2++ = *scan++;
	    scan++;
	    *scan2 = 0;		/* terminator */
	    /* check to see if there is a next action */
	    if (*scan != '#')
	    {			/* malloc another one */
	      new_move = MALLOC(sizeof(move));
	      current_move->next = new_move;
	      new_move->move_string[0] = 0;
	      new_move->next = 0;
	      current_move = new_move;
	    }
	  }
	}
	else
	{
	  current_robot->moves_top = 0;
	}
      }
      else
      {
	sprintf(stack, "Failed to find save player for robot: %s", nametoget);
	stack = end_string(stack);
	log("robot", oldstack);
	stack = oldstack;
	/* skip actions for the robot */
	while (*scan && *scan != '#')
	  scan++;
	scan++;
	while (*scan && *scan != '#')
	  scan++;
      }
    }
  }
  if (robots.where)
    FREE(robots.where);
  stack = oldstack;
}

/* list all the robot sections */
void list_robots(player * p, char *str)
{
  char *oldstack;
  robot *scan;
  move *movescan;
  int count = 0;

  oldstack = stack;
  if (robot_start == NULL)
  {
    tell_player(p, " There are no robots.\n");
    return;
  }
  pstack_mid("Robot Information");
  for (scan = robot_start; scan; scan = scan->next)
  {
    count++;

    if (count > 1)
    {
      sprintf(stack, LINE "\n\n");
      stack = strchr(stack, 0);
    }
    if (scan->flags & STORED)
      sprintf(stack, "Name: %s (STORED)\n", scan->lower_name);
    else
      sprintf(stack, "Name: %s\n", scan->lower_name);
    stack = strchr(stack, 0);
    /* action times */
    sprintf(stack, "Action every %d seconds, counter at %d\n", scan->speed, scan->counter);
    stack = strchr(stack, 0);
    /* flags */
    if (scan->flags)
    {
      strcpy(stack, "Robot is ");
      stack = strchr(stack, 0);
      if (scan->flags & WANDER)
      {
	strcpy(stack, "fully wandering, ");
	stack = strchr(stack, 0);
      }
      else if (scan->flags & LOCAL_WANDER)
      {
	strcpy(stack, "restricted to local rooms, ");
	stack = strchr(stack, 0);
      }
      if (scan->flags & INTELLIGENT)
      {
	strcpy(stack, "artificially intelligent, ");
	stack = strchr(stack, 0);
      }
      if (scan->flags & STORED)
      {
	strcpy(stack, "in storage, ");
	stack = strchr(stack, 0);
      }
      if (scan->flags & FIXED)
      {
	strcpy(stack, "fixed in location, ");
	stack = strchr(stack, 0);
      }
      /* fix trailing ,'s */
      stack -= 2;
      *stack++ = '.';
      *stack++ = '\n';
      *stack = 0;
    }
    else
    {
      strcpy(stack, "Robot has no flag settings.\n");
      stack = strchr(stack, 0);
    }

    /* actions */
    if (scan->max_moves)
    {
      sprintf(stack, "Robot has %d defined moves, which are:\n", scan->max_moves);
      stack = strchr(stack, 0);
      for (movescan = scan->moves_top; movescan; movescan = movescan->next)
      {
	sprintf(stack, "  %s\n", movescan->move_string);
	stack = strchr(stack, 0);
      }
    }
    else
    {
      strcpy(stack, "Robot has no defined moves.\n");
      stack = strchr(stack, 0);
    }
    *stack++ = '\n';
  }
  sprintf(stack, LINE "\n");
  stack = end_string(stack);
  if (count == 0)
  {
    tell_player(p, " Sorry - No robots found!\n");
    stack = oldstack;
    return;
  }
  pager(p, oldstack);
  stack = oldstack;
}


/* routine for finding a random adjacent room */
int go_random_exit(player * p)
{
  char *exits, *scan, *oldstack;
  int count = 0, temp = 0;

  oldstack = stack;
  if (!p->location)
    return 0;
  if (!decompress_room(p->location))
    return 0;

  exits = p->location->exits.where;
  if (!exits)
    return 0;

  while (*exits)
  {
    while (*exits && *exits != '\n')
      *stack++ = *exits++;
    if ((*exits) == '\n')
      exits++;
    *stack++ = 0;
    count++;
  }
  temp = count;
  /* now we choose the nTH exits */
  temp = rand() % count;

  for (scan = oldstack; temp > 0; temp--)
    scan = end_string(scan);
  trans_fn(p, scan);
  stack = oldstack;
  return 1;
}


/* routine for finding a random adjacent room */
int go_local_random_exit(player * p)
{
  char *exits, *scan, *oldstack, *tstack;
  int count = 0, temp = 0;

  oldstack = stack;
  if (!p->location)
    return 0;
  if (!decompress_room(p->location))
    return 0;

  exits = p->location->exits.where;
  if (!exits)
    return 0;
  /* same as above routine, but only counts the same-owner type rooms */
  while (*exits)
  {
    tstack = stack;
    /* get the room owner onto the stack */
    while (*exits && *exits != '.')
      *stack++ = *exits++;
    *stack = 0;
    /* check the name */
    if (!strcasecmp(tstack, p->location->owner->lower_name))
    {
      /* now the actual room name part */
      while (*exits && *exits != '\n')
	*stack++ = *exits++;
      if ((*exits) == '\n')
	exits++;
      *stack++ = 0;
      count++;
    }
    else
    {
      while (*exits && *exits != '\n')
	exits++;
      if (*exits == '\n')
	exits++;
      stack = tstack;
    }
  }
  temp = count;
  if (temp == 0)
    return 0;
  /* now we choose the nTH exits */
  temp = rand() % count;

  for (scan = oldstack; temp > 0; temp--)
    scan = end_string(scan);
  trans_fn(p, scan);
  stack = oldstack;
  return 1;
}


/* we want p for location and nonselection */
player *random_player_in_room(player * p)
{
  int count = 0, chosen;
  player *scan;

  /* is there a location to check? */
  if (!p->location || !p->location->players_top)
    return p;

  /* count the non-me players */
  for (scan = p->location->players_top; scan; scan = scan->room_next)
    if (scan != p)
      count++;

  /* return p if there isnt anyone else */
  if (count < 1)
    return p;

  /* choose a random one */
  chosen = (rand() % count);
  scan = p->location->players_top;
  if (scan == p)
    scan = scan->room_next;
  while (chosen)
  {
    chosen--;
    scan = scan->room_next;
    if (scan == p)
      scan = scan->room_next;
  }

  if (scan)
    return scan;
  return p;
}


/* we want p for location and nonselection */
player *random_player_on_program(player * p)
{
  int chosen;
  player *scan;

  /* count the non-me players */
  /*for(scan = flatlist_start; scan; scan = scan->flat_next)
     if(scan!=p)
     count++; use this if the below code fails */


  /* return p if there isnt anyone else */
  if ((current_players - 1) < 1)
    return p;

  /* choose a random one */
  chosen = (rand() % (current_players - 1));
  scan = flatlist_start;
  if (scan == p)
    scan = scan->flat_next;
  while (chosen)
  {
    chosen--;
    scan = scan->flat_next;
    if (scan == p)
      scan = scan->flat_next;
  }

  if (scan)
    return scan;
  return p;
}


/* truly nasty.  a thing to parse a robot string now that it can choose a
   local or global user at random into its ibuffer.  dodgy. */
/* actually, dodgy are a good band, but im listening to the cure right now
   flicker flicker flicker flicker here you are
   flicker flicker flicker flicker cateripillar girl. etc. */

/* yeah James, you always were a bit of a nutter :o) -- Silver */

void parse_move_into_ibuffer(player * p, char *str)
{
  char *scan, *oldstack;
  player *chosen;

  oldstack = stack;

  /* and - the immortal for statement! */
  for (scan = str; scan && *scan; scan++)
  {				/* check it all. */
    switch (*scan)
    {
      case '*':		/* i dont care if this means robots cannot do * signs. */
	/* random user on program */
	chosen = random_player_on_program(p);
	if (chosen)
	{
	  strcpy(stack, chosen->name);
	  stack = strchr(stack, 0);
	}
	break;
      case '~':
	/* random user in same location */
	chosen = random_player_in_room(p);
	if (chosen)
	{
	  strcpy(stack, chosen->name);
	  stack = strchr(stack, 0);
	}
	break;
      default:
	*stack++ = *scan;
    }
  }
  *stack++ = 0;
  strncpy(p->ibuffer, oldstack, IBUFFER_LENGTH - 2);
  stack = oldstack;
}


/* choose a random action according to flags and do it */
void run_action(robot * robby)
{
  int count;
  move *scan;

  /* its time for the robot to -do- something */
  /* will it be a move or something said? */
  /* 50/50 chance methinks */
  if (((rand() % 3 > 0) || (robby->flags & FIXED)) && robby->max_moves > 0)
  {
    for (count = (rand() % robby->max_moves), scan = robby->moves_top; count > 0; count--, scan = scan->next);
    /* new function!
       strcpy(robby->actual_player->ibuffer, scan->move_string); */
    parse_move_into_ibuffer(robby->actual_player, scan->move_string);
    input_for_one(robby->actual_player);
  }
  else
    /* ie a move to a random adjacent room */
  {
    if (robby->flags & WANDER)
      go_random_exit(robby->actual_player);
    else
      go_local_random_exit(robby->actual_player);
  }
}


/* disconnect a robot and stop it running :-) */
void store_robot(player * p, char *str)
{
  robot *robscan;
  char *oldstack;
  player *robby;

  if (!*str)
  {
    tell_player(p, " Format: store <robot name>\n");
    return;
  }
  oldstack = stack;
  robscan = robot_start;

  while (robscan)
  {
    if (!strcasecmp(robscan->lower_name, str))
    {
      if (robscan->flags & STORED)
      {
	tell_player(p, " That robot is already in storage.\n");
	return;
      }
      robscan->flags |= STORED;
      /* if the robot is connected, kill it */
      robby = find_player_absolute_quiet(robscan->lower_name);
      if (robby)
      {
	quit(robby, "");
	TELLPLAYER(p, " Robot '%s' is now in storage.\n", robscan->lower_name);
	SW_BUT(p, " -=*> %s stores robot '%s'\n", p->name, robscan->lower_name);
	LOGF("robot", "%s stores '%s'", p->name, robscan->lower_name);
	return;
      }
    }
    robscan = robscan->next;
  }

  /* no robot was found */
  sprintf(stack, " Sorry, no such robot '%s'.\n", str);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

/* connect a robot and start it running :-) */
void unstore_robot(player * p, char *str)
{
  robot *robscan;
  char *oldstack;

  if (!*str)
  {
    tell_player(p, " Format: unstore <robot name>\n");
    return;
  }
  oldstack = stack;
  robscan = robot_start;

  while (robscan)
  {
    if (!strcasecmp(robscan->lower_name, str))
    {
      robscan->counter = 1;
      if (!(robscan->flags & STORED))
      {
	tell_player(p, " That robot is already running.\n");
	return;
      }
      robscan->flags &= ~STORED;
      TELLPLAYER(p, " Robot '%s' is now out of storage.\n", robscan->lower_name);
      SW_BUT(p, " -=*> %s unstores robot '%s'\n", p->name, robscan->lower_name);
      LOGF("robot", "%s unstores '%s'", p->name, robscan->lower_name);
      return;
    }
    robscan = robscan->next;
  }

  /* no robot was found */
  sprintf(stack, " Sorry, no such robot '%s'.\n", str);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}


/* a version of connect_to_prog specifically for robots :-) */
void connect_robot_to_prog(player * p, char *name)
{
  player *cp;
  cp = current_player;
  current_player = p;
  tell_player(p, "\377\373\031");	/* send will EOR */
  p->timer_count = 60;
  p->timer_fn = login_timeout;
  current_player = cp;
  restore_player(p, name);
  p->residency |= ROBOT_PRIV;
  p->flags |= ROBOT;
  strcpy(p->inet_addr, "0.0.0.0");
  strcpy(p->num_addr, "0.0.0.0");
  link_to_program(p);
}


/* next two functions are for parse.c */
void process_robot_counters(void)
{
  robot *rscan;

  if (sys_flags & PANIC)
    return;

  for (rscan = robot_start; rscan; rscan = rscan->next)
    if (rscan->counter > 0)
      rscan->counter--;
}


/* scan all robots and run them if necessary - this is a parse.c bit that
   gets called in timer_function */
void process_robots(void)
{
  robot *rscan;
  player *rc;

  if (sys_flags & PANIC)
    return;

  for (rscan = robot_start; rscan; rscan = rscan->next)
  {
    /* SPANG - MOVE THIS SOMEWHERE ELSE AND DO IT JUST ONCE? */
    /* check to make sure robot hasnt fallen off :-) */
    if (rscan->counter == 0 && !(rscan->flags & STORED))
    {
      rc = find_player_absolute_quiet(rscan->lower_name);
      /* check for nightmare crashes/etc ;) */
      if (!rc)
      {
	rc = create_player();
	rc->fd = -1;
	connect_robot_to_prog(rc, rscan->lower_name);
	rscan->actual_player = rc;

      }
      else if (!(rc->flags & PANIC))
      {
	/* be cheeky here, safetly thing really.  set actual_player to rc!
	   this is actually because i --KNOW-- someone will try logging in
	   as a robot and that could conceivably blow it away. */
	rscan->actual_player = rc;
	rscan->counter = rscan->speed;
	current_player = rscan->actual_player;
	current_room = rscan->actual_player->location;
	run_action(rscan);
	current_player = 0;
	current_room = 0;
      }
    }
  }
}


void robot_version(void)
{
  sprintf(stack, " -=*> Robots v%s (by Slaine) enabled.\n", ROB_VERS);
  stack = strchr(stack, 0);
#ifdef INTELLIBOTS
  intelli_version();
#endif
}

void bot_unstore_robot(char *str)
{
  robot *robscan;
  char *oldstack;

  oldstack = stack;
  robscan = robot_start;

  while (robscan)
  {
    if (!strcasecmp(robscan->lower_name, str))
    {
      robscan->counter = 1;
      if (!(robscan->flags & STORED))
	return;
      robscan->flags &= ~STORED;
      return;
    }
    robscan = robscan->next;
  }
}
