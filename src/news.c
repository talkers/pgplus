/*
 * Playground+ - news.c
 * NuNews system, enhanced news with groups and more (c) phypor 1998
 * ---------------------------------------------------------------------------
 *
 * Modifications to original release:
 *  Include paths
 *  varible argument functions
 *  changed ADC to LOWER_ADMIN
 *  changed pager calls to be pg+ conformant
 *  cleaned up presentation (used LINE and pstack_mid)
 *  added number of times read to "news check"
 *  added   news next  command to read next unread article
 */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <memory.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"



/* for sync streamlining, so we dont sync unless there is some change */
#define NEWS_CHANGED		newschanged = 1
#define NEWS_CHANGE_RESET	newschanged = 0
#define NEWS_HAS_CHANGED	newschanged == 1


extern command_func news_command;
extern int news_sync;
extern void help(player *, char *);

int newschanged = 0;


newsgroup NewsGroups[] =
{
  {"main", 0, 7, "Default news group", "files/news/main", 0},
  {"sus", PSU, 10, "Staff news group", "files/news/sus", 0},
  {"admin", LOWER_ADMIN, 15, "Admin news groups", "files/news/admin", 0},
  {"flames", 0, 20, "Flames. keep em to This Board Only", "files/news/flames", 0},
  {"songs", 0, 20, "Songs, Poems, Things of that nature", "files/news/songs", 0},
  {"humor", 0, 20, "Humor, jokes, funny stories.", "files/news/humor", 0},
  {"", 0, 0, "", "", 0}
};




/*** io functs ***/

char *load_file_to_string(char *filename)
{
  int fd, len;
  char *loaded;

  fd = open(filename, O_RDONLY);
  if (fd < 0)
  {
    LOGF("error", "failed to load file to string [%s]", filename);
    return (char *) NULL;
  }
  len = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  loaded = (char *) MALLOC(len + 1);
  if (!loaded)
  {
    close(fd);
    LOGF("error", "failed to malloc in load_file_to_string %d.", len);
    return (char *) NULL;
  }
  memset(loaded, 0, len + 1);
  if (read(fd, loaded, len) < 0)
  {
    close(fd);
    LOGF("error", "failed to read, load_file_to_string [%s]", filename);
    return (char *) NULL;
  }
  close(fd);
  if (sys_flags & VERBOSE)
    LOGF("verbose", "load_file_to_string [%s]", filename);

  return loaded;
}

void sync_news(newsgroup * group, news_header * nh, char *body)
{
  int fd;
  FILE *of;

  /* allows us to just sync header, if no body is passed */
  if (*body)
  {
    sprintf(stack, "%s/%d.body", group->path, nh->id);
#ifdef BSDISH
    fd = open(stack, O_CREAT | O_WRONLY | S_IRUSR | S_IWUSR);
#else
    fd = open(stack, O_CREAT | O_WRONLY | O_SYNC, S_IRUSR | S_IWUSR);
#endif /* BSDISH */
    if (fd < 0)
    {
      LOGF("error", "sync_news() failed to open "
	   "body file, %s", strerror(errno));
      return;
    }
    write(fd, body, strlen(body));
    close(fd);
  }

  sprintf(stack, "%s/%d.head", group->path, nh->id);
  of = fopen(stack, "w");
  if (!of)
  {
    LOGF("error", "sync_news() failed to open "
	 "head file, %s", strerror(errno));
    return;
  }
  fwrite(nh, sizeof(news_header), 1, of);
  fclose(of);
}

void sync_group_news_headers(newsgroup * group)
{
  news_header *scan;
  int fd;
  char *oldstack = stack;
  char path[160];

  *stack = '\0';

  for (scan = group->top; scan; scan = scan->next)
    sync_news(group, scan, "");

  /* update the .newslist file */
  for (scan = group->top; scan; scan = scan->next)
    stack += sprintf(stack, "%d.head\n", scan->id);
  stack = end_string(stack);

  memset(path, 0, 160);
  sprintf(path, "%s/.newslist", group->path);
#ifdef BSDISH
  fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
  fd = open(path, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif /* BSDISH */
  if (fd < 0)
  {
    LOGF("error", "failed to open .newslist in "
	 "sync_group_news_headers() [%s], %s", group->name,
	 strerror(errno));
    return;
  }

  write(fd, oldstack, strlen(oldstack));
  close(fd);
  stack = oldstack;
}


void sync_all_news_headers(void)
{
  int i;

  if (!(NEWS_HAS_CHANGED))
    return;
  for (i = 0; NewsGroups[i].name[0]; i++)
    sync_group_news_headers(&NewsGroups[i]);

  NEWS_CHANGE_RESET;

}





news_header *load_news_header(newsgroup * group, char *fname)
{
  FILE *ni;
  news_header *nh;


  sprintf(stack, "%s/%s", group->path, fname);
  ni = fopen(stack, "r");
  if (!ni)
  {
    LOGF("error", "failed to fopen() in load_news_article() [%s:%s], %s",
	 group->name, fname, strerror(errno));
    return (news_header *) NULL;
  }
  nh = (news_header *) MALLOC(sizeof(news_header));
  if (!nh)
  {
    LOGF("error", "failed to malloc() in load_news_article() [%s], %s",
	 fname, strerror(errno));
    return (news_header *) NULL;
  }
  fread(nh, sizeof(news_header), 1, ni);
  fclose(ni);

  return nh;
}

void load_all_news_for_group(newsgroup * group)
{
  news_header *nh = 0;
  news_header *pe = 0;
  FILE *lf;
  char li[160];

  /* open the file with news list */
  memset(li, 0, 160);
  sprintf(li, "%s/.newslist", group->path);
  lf = fopen(li, "r");
  if (!lf)
  {
    LOGF("error", "failed to fopen() in load_all_news_for_group()[%s], %s",
	 group->name, strerror(errno));
    return;
  }

  /* read through each line of file and load that header */
  while (fgets(li, 159, lf))
  {
    if (li[strlen(li) - 1] == '\n')
      li[strlen(li) - 1] = '\0';	/* spank off the newlines */

    nh = load_news_header(group, li);
    if (nh)
    {
      if (!(group->top))
	group->top = nh;
      else
	pe->next = nh;
      pe = nh;			/* set this one to the previous article */
    }
    memset(li, 0, 160);
  }
}


void init_news(void)
{
  struct stat sbuf;
  char *oldaction = action;
  char *oldstack = stack;
  int i, fd;

  action = "News Initation";
  for (i = 0; NewsGroups[i].name[0]; i++)
  {
    /* make sure the directory for the group exists */
    if (stat(NewsGroups[i].path, &sbuf) < 0)
    {
      if (mkdir(NewsGroups[i].path, S_IRUSR | S_IWUSR | S_IXUSR) < 0)
      {
	LOGF("error", "failed to create diretory for news [%s], %s",
	     NewsGroups[i].path, strerror(errno));
	continue;
      }
      sprintf(stack, "%s/.newslist", NewsGroups[i].path);
      stack = end_string(stack);
      fd = open(oldstack, O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
      if (fd < 0)
	LOGF("error", "failed to create dummy newslist file [%s], %s",
	     NewsGroups[i].name, strerror(errno));
      else
	close(fd);
      stack = oldstack;

      LOGF("boot", "Created new directory for newsgroup '%s'",
	   NewsGroups[i].name);
    }

    load_all_news_for_group(&NewsGroups[i]);
  }
  action = oldaction;

}


/*** misc functions ***/

void destroy_news(newsgroup * group, news_header * nh)
{
  char path[160];

  NEWS_CHANGED;

  memset(path, 0, 160);
  sprintf(path, "%s/%d.body", group->path, nh->id);
  unlink(path);
  memset(path, 0, 160);
  sprintf(path, "%s/%d.head", group->path, nh->id);
  unlink(path);

  memset(nh, 0, sizeof(news_header));
  FREE(nh);
}


void scan_news(void)
{
  news_header *scan, *current;
  int t, i;
  int d = 0;			/* for sanity, only delete 5 articles per loop */


  t = time(0);

  for (i = 0; NewsGroups[i].name[0]; i++)
  {
    while (NewsGroups[i].top && NewsGroups[i].top->flags & DELETE_ME && d < 5)
    {
      current = NewsGroups[i].top;
      NewsGroups[i].top = NewsGroups[i].top->next;
      destroy_news(&NewsGroups[i], current);
      d++;
    }
    for (scan = NewsGroups[i].top; (d < 5 && scan); scan = scan->next)
    {
      if (((t - (scan->date)) > NEWS_TIMEOUT) &&
          !(scan->flags & STICKY_ARTICLE))
      {
        LOGF("news", "Timeout of %s %s posting, %s", scan->name,
             NewsGroups[i].name, scan->header);
        scan->flags |= DELETE_ME;
      }


      /* keep this the last thing done, so we can have other
         things in the loop set the DELETE_ME flag and handle it promptly
       */
      if (scan->next && scan->next->flags & DELETE_ME)
      {
	NEWS_CHANGED;
	current = scan->next;
	scan->next = scan->next->next;
	destroy_news(&NewsGroups[i], current);
	d++;
      }
    }
  }
}

int remove_all_news(player * p, char *rm_name)
{
  news_header *scan;
  int rmd = 0, i;

  for (i = 0; NewsGroups[i].name[0]; i++)
    for (scan = NewsGroups[i].top; scan; scan = scan->next)
      if (!strcasecmp(scan->name, rm_name))
      {
	scan->flags |= DELETE_ME;
	rmd++;
      }
  return rmd;
}

newsgroup *find_news_group(char *str)
{
  int i;

  for (i = 0; NewsGroups[i].name[0]; i++)
    if (!strcasecmp(str, NewsGroups[i].name))
      if (!NewsGroups[i].required_priv || (current_player &&
		   current_player->residency & NewsGroups[i].required_priv))
	return &NewsGroups[i];

  return (newsgroup *) NULL;
}

int find_news_group_number(newsgroup * group)
{
  int i;

  for (i = 0; NewsGroups[i].name[0]; i++)
    if (&NewsGroups[i] == group)
      return i;

  LOGF("error", "news group not in NewsGroups array?!? [%s]", group->name);
  return -1;
}


int count_player_postings(player * p, newsgroup * group)
{
  int i = 0;
  news_header *scan;

  for (scan = group->top; scan; scan = scan->next)
    if (!strcasecmp(scan->name, p->name))
      i++;
  return i;
}



int get_next_news_id(newsgroup * group)
{
  int lowest = 1;
  news_header *scan;

  scan = group->top;
  while (scan)
  {
    for (scan = group->top; scan; scan = scan->next)
      if (scan->id == lowest)
      {
	lowest++;
	break;
      }
  }
  return lowest;
}

int count_news_articles(newsgroup * group)
{
  news_header *nh = group->top;
  int i = 0;

  while (nh)
  {
    i++;
    nh = nh->next;
  }
  return i;
}

news_header *find_news_article(newsgroup * group, int i)
{
  news_header *scan = group->top;
  int ret = 1;

  for (; (scan && ret < i); ret++, scan = scan->next);

  return scan;
}



char *id_or_not(player * p, news_header * nh)
{
  static char fawn[40];

  memset(fawn, 0, 40);
  if (p->residency & ADMIN)
    sprintf(fawn, "(%d)", nh->id);
  else
    return "";
  return fawn;
}

char *spaces10(int i)
{
  if (i < 10)
    return " ";
  return "";
}

char *spaces_followups(news_header * nh)
{
  switch (nh->followups)
  {
    case 0:
      return "";
    case 1:
      return " ";
    case 2:
      return "  ";
    case 3:
      return "   ";
    case 4:
      return "    ";
    case 5:
      return "     ";
    case 6:
      return "      ";
    case 7:
      return "       ";
    case 8:
      return "        ";
    case 9:
      return "         ";
  }
  return "         ";
}


char *sender_str(player * p, news_header * nh)
{
  static char isat[80];

  if (nh->flags & ANONYMOUS)
  {
    if (p && (p->residency & ADMIN || !strcasecmp(nh->name, p->name)))
      sprintf(isat, "?%s?", nh->name);
    else
      sprintf(isat, "(?????)");
  }
  else
    sprintf(isat, "(%s)", nh->name);
  return isat;
}

char *read_count_string(int i)
{
  static char s[16];

  memset(s, 0, 16);
  sprintf(s, "<%d>", i);
  for (i = strlen(s); i < 5; i++)
    strcat(s, " ");

  return s;
}

int next_unread(player * p, newsgroup * g)
{
  news_header *nh = g->top;
  int i = 0, gn = find_news_group_number(g);

  if (gn < 0)
    return 0;

  while (nh && nh->date > p->news_last[gn])
  {
    nh = nh->next;
    i++;
  }
  return i;
}



void save_str_public(char *filename, char *str)
{
  int fd;

#ifdef BSDISH
  fd = open(filename, O_CREAT | O_WRONLY |
	    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#else
  fd = open(filename, O_CREAT | O_WRONLY | O_SYNC,
	    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif /* BSDISH */

  if (fd < 0)
  {
    LOGF("error", "save_str() failed to open [%s], %s", filename,
	 strerror(errno));
    return;
  }
  write(fd, str, strlen(str));
  /* make sure we get the right privs ..no matter whut umask */
  fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  close(fd);
}


/* new version for walling to news inform And enough privs */

void news_wall_priv_but(player * p, char *str, int priv)
{
  player *scan;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (scan->custom_flags & NEWS_INFORM && scan != p &&
	(!priv || scan->residency & priv))
    {
      command_type |= HIGHLIGHT;
      tell_player(scan, str);
      command_type &= ~HIGHLIGHT;
    }
  }
}

int count_new_news(newsgroup * group, int cutoff)
{
  news_header *scan;
  int newcnt = 0;

  for (scan = group->top; scan; scan = scan->next)
    if (cutoff < scan->date)
      newcnt++;

  return newcnt;
}




void new_news_inform(player * p)
{
  int i, h = 0;
  char *oldstack = stack;

  stack += sprintf(stack, " Unread news : ");

  for (i = 0; (NewsGroups[i].name[0] && i < MAX_LAST_NEWS_INTS); i++)
    if (!(NewsGroups[i].required_priv) ||
	p->residency & NewsGroups[i].required_priv)
      if (NewsGroups[i].top && NewsGroups[i].top->date > p->news_last[i])
      {
	h++;
	stack += sprintf(stack, "%s (%d)   ", NewsGroups[i].name,
			 count_new_news(&NewsGroups[i], p->news_last[i]));
      }

  stack += sprintf(stack, "\n");
  stack = end_string(stack);
  if (h)
    tell_player(p, oldstack);
  stack = oldstack;
}



/*** final functs ***/
void quit_news_posting(player * p)
{
  tell_player(p, " Article NOT posted.\n");
  FREE(p->edit_info->misc);
  p->mode &= ~NEWSEDIT;
}

void end_news_posting(player * p)
{
  news_header *nh;
  char *oldstack = stack;
  int ngn;

  if (!*(p->edit_info->buffer))
  {
    quit_news_posting(p);
    return;
  }
  NEWS_CHANGED;

  nh = (news_header *) p->edit_info->misc;

  nh->id = get_next_news_id(nh->group);
  nh->date = time(0);

  /* add it to the linked news list */
  if (nh->group->top)
    nh->next = nh->group->top;
  nh->group->top = nh;

  /* save it to disk */
  sync_news(nh->group, nh, p->edit_info->buffer);

  /* sync the news to disk */
  sync_group_news_headers(nh->group);

  /* inform the player */
  tell_player(p, " Article posted ...\n");

  /* inform the masses */
  if (nh->group != &NewsGroups[0])
    stack += sprintf(stack, " -=*> A new news article has been posted in %s",
		     nh->group->name);
  else
    stack += sprintf(stack, " -=*> A new news article has been posted");

  if (nh->flags & ANONYMOUS)
    stack += sprintf(stack, " anonymously");
  else
    stack += sprintf(stack, " by %s", p->name);

  stack += sprintf(stack, " entitled ...\n      %s^N\n", nh->header);

  stack = end_string(oldstack);
  news_wall_priv_but(p, oldstack, nh->group->required_priv);
  stack = oldstack;

  /* update the players read count, so they dont get it as unread */
  ngn = find_news_group_number(nh->group);
  if (ngn > -1 && ngn < MAX_LAST_NEWS_INTS)
    p->news_last[ngn] = nh->date;

  /* just make a log for whutever */
  LOGF("news", "%s posts in %s [%s]", nh->name,
       nh->group->name, nh->header);

  /* preserve (or not) the players mode */
  if (p->edit_info->input_copy == news_command)
  {
    do_prompt(p, "News Mode >");
    p->mode |= NEWSEDIT;
  }
  else
    p->mode &= ~NEWSEDIT;

}




/*** command functs ***/


/* the news command */

void news_command(player * p, char *str)
{


  if (p->edit_info)
  {
    tell_player(p, " Can't do news commands whilst in the editor.\n");
    return;
  }
  if ((*str == '/') && (p->input_to_fn == news_command))
  {
    match_commands(p, str + 1);
    if (!(p->flags & PANIC) && (p->input_to_fn == news_command))
    {
      do_prompt(p, "News Mode >");
      p->mode |= NEWSEDIT;
    }
    return;
  }
  if (!*str)
  {
    if (p->input_to_fn == news_command)
    {
      tell_player(p, " Format : news <action>\n");
      if (!(p->flags & PANIC) && (p->input_to_fn == news_command))
      {
	do_prompt(p, "News Mode >");
	p->mode |= NEWSEDIT;
      }
      return;
    }
    else
    {
      tell_player(p, " Entering news mode. Use 'end' to leave.\n"
		  " '/<command>' does normal commands.\n");
      p->flags &= ~PROMPT;
      p->input_to_fn = news_command;
    }
  }
  else
    sub_command(p, str, news_list);

  if (!(p->flags & PANIC) && (p->input_to_fn == news_command))
  {
    do_prompt(p, "News Mode >");
    p->mode |= NEWSEDIT;
  }
}

/* exit news mode */

void exit_news_mode(player * p, char *str)
{
  if (p->input_to_fn != news_command)
  {
    tell_player(p, " Idle git! ;)\n");
    return;
  }

  tell_player(p, " Leaving news mode.\n");
  p->input_to_fn = 0;
  p->flags |= PROMPT;
  p->mode &= ~NEWSEDIT;
}

void view_news_commands(player * p, char *str)
{
  view_sub_commands(p, news_list);
}


/* toggle whether someone gets informed of news */

void toggle_news_inform(player * p, char *str)
{
  if (!strcasecmp("off", str))
    p->custom_flags &= ~NEWS_INFORM;
  else if (!strcasecmp("on", str))
    p->custom_flags |= NEWS_INFORM;
  else
    p->custom_flags ^= NEWS_INFORM;

  if (p->custom_flags & NEWS_INFORM)
    tell_player(p, " You will be informed of new news when posted.\n");
  else
    tell_player(p, " You will not be informed of new news when posted.\n");
}

/* toggle seeing unread news postings on login */

void toggle_news_login(player * p, char *str)
{
  if (!strcasecmp("off", str))
    p->custom_flags |= NO_NEW_NEWS_INFORM;
  else if (!strcasecmp("on", str))
    p->custom_flags &= ~NO_NEW_NEWS_INFORM;
  else
    p->custom_flags ^= NO_NEW_NEWS_INFORM;

  if (p->custom_flags & NO_NEW_NEWS_INFORM)
    tell_player(p, " You will not see unread news on login.\n");
  else
    tell_player(p, " On login, you will be informed of unread news.\n");
}

/* new news post */
void post_news(player * p, char *str)
{
  newsgroup *group;
  news_header *nh;
  char *scan, *first, word[80];

  if (!*str)
  {
    tell_player(p, " Format : post [group] <header>\n");
    return;
  }
  strncpy(word, str, 79);	/* get the first word, for checking group */
  for (scan = word; (*scan && *scan != ' '); scan++);
  *scan = '\0';
  if ((group = find_news_group(word)))
  {
    while (*str && *str != ' ')
      str++;			/* cut off the first word */

    if (!*str)			/* they tried to trick us */
    {
      tell_player(p, " Format : news post [group] <header>\n");
      return;
    }
    str++;			/* get past the space we stopped on */
  }
  else
    group = &NewsGroups[0];	/* use the first group, by default for default */


  if ((count_player_postings(p, group) >= group->max) &&
      !(p->residency & ADMIN))
  {
    tell_player(p, " You have posted your maximum amount of articles in that group.\n");
    return;
  }

  /* get a new news_header */
  nh = (news_header *) MALLOC(sizeof(news_header));
  memset(nh, 0, sizeof(news_header));

  /* setup its stuffs */
  strncpy(nh->name, p->name, MAX_NAME - 1);
  strncpy(nh->header, str, MAX_TITLE - 1);

  nh->flags |= NEWS_ARTICLE;
  nh->read_count = 0;
  nh->group = group;

  first = first_char(p);
  scan = strstr(first, "post");
  if (scan && (scan != first_char(p)) && (*(scan - 1) == 'a'))
    nh->flags |= ANONYMOUS;

  tell_player(p, " Now enter the body for the article.\n");
  *stack = 0;
  start_edit(p, MAX_ARTICLE_SIZE, end_news_posting,
	     quit_news_posting, stack);

  if (p->edit_info)
    p->edit_info->misc = (void *) nh;
  else
  {
    LOGF("error", "failed to enter the editor "
	 "in post_news() for %s", p->name);
    FREE(nh);

  }
}

/* follow up an article */

void followup(player * p, char *str)
{
  newsgroup *group;
  news_header *scan, *nh;
  char *oldstack = stack;
  char *newbody, *body, *indent, *ptr, *first, *word;
  int which;


  if (!*str)
  {
    tell_player(p, " Format : news followup [group] #\n");
    return;
  }
  if (isalpha(*str))		/* they want a news group */
  {
    word = next_space(str);
    *word++ = 0;
    group = find_news_group(str);
    if (!group)
    {
      TELLPLAYER(p, " There doesn't appear to be a group '%s'\n", str);
      return;
    }
    which = atoi(word);
  }
  else
  {
    which = atoi(str);
    group = &NewsGroups[0];
  }

  if (which < 1)
  {
    tell_player(p, " Format : news followup [group] #\n");
    return;
  }

  if ((count_player_postings(p, group) >= group->max) &&
      !(p->residency & ADMIN))
  {
    tell_player(p, " You have posted your maximum amount of articles in that group.\n");
    return;
  }

  scan = find_news_article(group, which);
  if (!scan)
  {
    if (group != &NewsGroups[0])
      TELLPLAYER(p, " No such news posting '%d' in the %s group\n",
		 which, group->name);
    else
      TELLPLAYER(p, " No such news posting '%d'\n", which);
    return;
  }

  sprintf(stack, "%s/%d.body", group->path, scan->id);
  stack = end_string(stack);
  body = load_file_to_string(oldstack);
  stack = oldstack;

  if (!body)
  {
    tell_player(p, " Ergs, no body file found for that news posting.\n");
    LOGF("error", "No news body for header, id %d, group %s",
	 scan->id, group->name);
    return;
  }

  body[strlen(body)] = 0;

  nh = (news_header *) MALLOC(sizeof(news_header));
  if (!nh)
  {
    tell_player(p, " Erg, malloc fails, try again ...\n");
    log("error", "malloc failed in news followup");
    return;
  }
  memset(nh, 0, sizeof(news_header));

  /* setup its stuffs */
  strncpy(nh->name, p->name, MAX_NAME - 1);

  nh->flags |= NEWS_ARTICLE;
  nh->read_count = 0;
  nh->group = group;
  nh->followups = scan->followups + 1;


  strncpy(nh->header, str, MAX_TITLE - 1);

  if (strstr(scan->header, "Re: ") == scan->header)
    strncpy(nh->header, scan->header, MAX_TITLE - 1);
  else
  {
    sprintf(stack, "Re: %s", scan->header);
    strncpy(nh->header, stack, MAX_TITLE - 1);
  }
  nh->flags |= NEWS_ARTICLE;

  first = first_char(p);
  ptr = strstr(first, "followup");
  if (ptr && (ptr != first_char(p)) && (*(ptr - 1) == 'a'))
    nh->flags |= ANONYMOUS;



  indent = body;
  newbody = stack;

  if (scan->flags & ANONYMOUS)
    stack += sprintf(stack, "\nFrom anonymous article written on %s ...\n",
		     convert_time(scan->date));
  else
    stack += sprintf(stack, "\nOn %s, %s wrote ...\n",
		     convert_time(scan->date), scan->name);

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

  tell_player(p, " Please trim article as much as possible ...\n");
  start_edit(p, MAX_ARTICLE_SIZE, end_news_posting,
	     quit_news_posting, newbody);
  if (p->edit_info)
    p->edit_info->misc = (void *) nh;
  else
  {
    tell_player(p, " Erg, failed to enter the editor ...\n");
    FREE(nh);
  }
  stack = oldstack;
  FREE(body);
}

/* list news articles */

void list_news(player * p, char *str)
{
  newsgroup *group;
  news_header *scan;
  char *oldstack = stack;
  char middle[80], *which;
  int amt = 0;
  int page, pages;
  int count, ncount = 1;

  if (*str && isalpha(*str))	/* they want a specfic group */
  {
    which = next_space(str);	/* maybe they want a page too */
    *which++ = 0;
    group = find_news_group(str);
    if (!group)
    {
      TELLPLAYER(p, " There doesn't appear to be a group '%s'\n", str);
      return;
    }
  }
  else
  {
    which = str;
    group = &NewsGroups[0];
  }

  amt = count_news_articles(group);

  if (!amt)
  {
    if (group != &NewsGroups[0])
      TELLPLAYER(p, " There appears to be no news in the %s group.\n",
		 group->name);
    else
      tell_player(p, " There doesn't seem to be any news at all.\n");
    return;
  }

  page = atoi(which);
  if (page <= 0)
    page = 1;
  page--;

  pages = (amt - 1) / (TERM_LINES - 2);
  if (page > pages)
    page = pages;

  /* setup info thinger */
  if (group != &NewsGroups[0])
  {
    if (amt == 1)
      sprintf(middle, "1 article in group %s", group->name);
    else
      sprintf(middle, "%d articles in group %s", amt, group->name);
  }
  else
  {
    if (amt == 1)
      strcpy(middle, "There is one news article");
    else
      sprintf(middle, "There are %s articles", number2string(amt));
  }
  pstack_mid(middle);


  /* set our first article to be the top if the page to be viewed */
  count = page * (TERM_LINES - 2);
  for (scan = group->top; count; count--, ncount++)
    scan = scan->next;


  for (count = 0; ((count < (TERM_LINES - 1)) && scan); count++, ncount++)
  {
    stack += sprintf(stack, "%s [%d] %s%s%s%s^N %s\n", id_or_not(p, scan),
	      ncount, read_count_string(scan->read_count), spaces10(ncount),
		 spaces_followups(scan), scan->header, sender_str(p, scan));

    scan = scan->next;
  }
  sprintf(middle, "Page %d of %d", page + 1, pages + 1);
  pstack_mid(middle);

  *stack++ = 0;
  tell_player(p, oldstack);

  stack = oldstack;
}

void sync_news_command(player * p, char *str)
{
  if (!(NEWS_HAS_CHANGED))
  {
    tell_player(p, " No news changes since last sync ...\n");
    return;
  }
  tell_player(p, "Syncing news ...\n");
  sync_all_news_headers();
  tell_player(p, "Done ...\n");
}


void read_article(player * p, char *str)
{
  newsgroup *group;
  news_header *scan;
  int which, t, ngn;
  char *oldstack = stack;
  char *body, lastreader[MAX_INET_ADDR], head[MAX_TITLE], *word;

  if (!*str)
  {
    tell_player(p, " Format : news read [group] #\n");
    return;
  }
  if (isalpha(*str))		/* they want a news group */
  {
    word = next_space(str);
    *word++ = 0;
    group = find_news_group(str);
    if (!group)
    {
      TELLPLAYER(p, " There doesn't appear to be a group '%s'\n", str);
      return;
    }
    which = atoi(word);
  }
  else
  {
    which = atoi(str);
    group = &NewsGroups[0];
  }

  if (which < 1)
  {
    tell_player(p, " Format : news read [group] #\n");
    return;
  }

  scan = find_news_article(group, which);
  if (!scan)
  {
    if (group != &NewsGroups[0])
      TELLPLAYER(p, " No such news posting '%d' in the %s group\n",
		 which, group->name);
    else
      TELLPLAYER(p, " No such news posting '%d'\n", which);
    return;
  }

  sprintf(stack, "%s/%d.body", group->path, scan->id);
  stack = end_string(stack);
  body = load_file_to_string(oldstack);
  stack = oldstack;

  if (!body)
  {
    tell_player(p, " Ergs, no body file found for that news posting.\n");
    LOGF("error", "No news body for header, id %d, group %s",
	 scan->id, group->name);
    return;
  }

  NEWS_CHANGED;

  ngn = find_news_group_number(group);
  if (ngn > -1 && ngn < MAX_LAST_NEWS_INTS)
  {
    if (p->news_last[ngn] < scan->date)
      p->news_last[ngn] = scan->date;
  }


  strncpy(lastreader, scan->lastreader, MAX_INET_ADDR - 1);
  if (!*scan->lastreader || strcasecmp(p->inet_addr, scan->lastreader))
  {
    scan->read_count++;
    strncpy(scan->lastreader, p->inet_addr, MAX_INET_ADDR - 1);
  }

  t = time(0);
  memset(head, 0, MAX_TITLE);
  if (group != &NewsGroups[0])	/* is it the the defaault ? */
  {
    strncpy(head, group->name, MAX_TITLE - 1);	/* put the group up top */
    pstack_mid(head);
  }
  else				/* just a line */
    stack += sprintf(stack, LINE);

  stack += sprintf(stack, "    Subject: %s^N\n", scan->header);

  if (scan->flags & ANONYMOUS)
  {
    if (!strcasecmp(scan->name, p->name))
      stack += sprintf(stack, "    Posted anonymously by you on %s\n",
		       convert_time(scan->date));
    else if (p->residency & ADMIN)
      stack += sprintf(stack, "    Posted anonymously by %s on %s\n",
		       scan->name, convert_time(scan->date));
    else
      stack += sprintf(stack, "    Posted anonymously on %s\n",
		       convert_time(scan->date));
  }
  else
    stack += sprintf(stack, "    Posted by %s on %s\n",
		     scan->name, convert_time(scan->date));
  if (scan->read_count == 1)
    stack += sprintf(stack, "    Article has been read one time.\n");
  else
  {
    stack += sprintf(stack, "    Article has been read %s times.\n",
		     number2string(scan->read_count));
    if (p->residency & (LOWER_ADMIN | ADMIN))
      stack += sprintf(stack, "    Last read by someone from %s.\n",
		       lastreader);
  }
  if (p->residency & (LOWER_ADMIN | ADMIN))
  {
    if (!(scan->flags & STICKY_ARTICLE))
      stack += sprintf(stack, "    Times out in %s.\n",
		       word_time(NEWS_TIMEOUT + (scan->date - t)));
    else
      stack += sprintf(stack, "    Article will never timeout.\n");
  }


  stack += sprintf(stack, LINE);

  sprintf(stack, "%s\n", body);
  stack = end_string(stack);
  pager(p, oldstack);
  stack = oldstack;
  FREE(body);
}

void remove_article(player * p, char *str)
{
  newsgroup *group;
  news_header *nh;
  char *word;
  int which;

  if (!*str)
  {
    tell_player(p, " Format : news remove #\n");
    return;
  }
  if (isalpha(*str))		/* they want a news group */
  {
    word = next_space(str);
    *word++ = 0;
    group = find_news_group(str);
    if (!group)
    {
      TELLPLAYER(p, " There doesn't appear to be a group '%s'\n", str);
      return;
    }
    which = atoi(word);
  }
  else
  {
    which = atoi(str);
    group = &NewsGroups[0];
  }
  if (which < 1)
  {
    tell_player(p, " Format : news remove [group] #\n");
    return;
  }

  nh = find_news_article(group, which);
  if (!nh)
  {
    TELLPLAYER(p, " No such news posting '%d'\n", which);
    return;
  }
  if (strcasecmp(nh->name, p->name) && !(p->residency & ADMIN))
  {
    tell_player(p, " You may only remove posts you yourself have made.\n");
    return;
  }
  NEWS_CHANGED;

  nh->flags |= DELETE_ME;
  if (strcasecmp(nh->name, p->name))
    LOGF("remove", "%s removed news in %s from %s entitled %s", p->name,
	 group->name, nh->name, nh->header);

  tell_player(p, " Article removed ...\n");
}


void news_setsticky_command(player * p, char *str)
{
  newsgroup *group;
  news_header *nh;
  char *word;
  int which;

  if (!*str)
  {
    tell_player(p, " Format : news sticky #\n");
    return;
  }
  if (isalpha(*str))		/* they want a news group */
  {
    word = next_space(str);
    *word++ = 0;
    group = find_news_group(str);
    if (!group)
    {
      TELLPLAYER(p, " There doesn't appear to be a group '%s'\n", str);
      return;
    }
    which = atoi(word);
  }
  else
  {
    which = atoi(str);
    group = &NewsGroups[0];
  }
  if (which < 1)
  {
    tell_player(p, " Format : news sticky [group] #\n");
    return;
  }

  nh = find_news_article(group, which);
  if (!nh)
  {
    TELLPLAYER(p, " No such news posting '%d'\n", which);
    return;
  }
  nh->flags |= STICKY_ARTICLE;

  tell_player(p, " Sticky bit set ...\n");
}


void list_news_groups(player * p, char *str)
{
  int i;
  char *oldstack = stack;
  char temp[70];

  sprintf(temp, "%s News Groups", get_config_msg("talker_name"));
  pstack_mid(temp);

  for (i = 1; NewsGroups[i].name[0]; i++)
  {
    if (NewsGroups[i].required_priv &&
	!(p->residency & NewsGroups[i].required_priv))
      continue;

    stack += sprintf(stack, "%-20s %s\n", NewsGroups[i].name,
		     NewsGroups[i].desc);
  }
  stack += sprintf(stack, LINE "\n");
  stack = end_string(stack);
  pager(p, oldstack);
  stack = oldstack;
}

void news_checkown_command(player * p, char *str)
{
  newsgroup *group;
  news_header *scan;
  int cnt, i, tot = 0;
  char *oldstack = stack;

  if (!*str || (*str && strcasecmp(str, "all")))
  {
    if (*str)
      group = find_news_group(str);
    else
      group = &NewsGroups[0];

    if (!group)
    {
      TELLPLAYER(p, " There doesn't appear to be a newsgroup '%s'.\n",
		 str);
      return;
    }
    for (scan = group->top, cnt = 1; scan; scan = scan->next, cnt++)
      if (!strcasecmp(scan->name, p->name))
      {
	stack += sprintf(stack, "[%d] %s\n", cnt, scan->header);
	tot++;
      }
    stack = end_string(stack);
    if (tot)
      tell_player(p, oldstack);
    else
      tell_player(p, " You have posted no news ...\n");
    stack = oldstack;
    return;
  }
  for (i = 0; NewsGroups[i].name[0]; i++)
  {
    if (NewsGroups[i].required_priv &&
	!(p->residency & NewsGroups[i].required_priv))
      continue;

    stack += sprintf(stack, "---- %s group\n", NewsGroups[i].name);

    for (scan = NewsGroups[i].top, cnt = 1; scan; scan = scan->next, cnt++)
      if (!strcasecmp(scan->name, p->name))
      {
	stack += sprintf(stack, "[%d] %s\n", cnt, scan->header);
	tot++;
      }
  }

  stack = end_string(stack);
  if (tot)
    pager(p, oldstack);
  else
    tell_player(p, " You have posted no news ...\n");
  stack = oldstack;
}


void remove_all_news_command(player * p, char *str)
{
  int rmd;

  if (!*str)
  {
    tell_player(p, " Format : remove_all_news <player>\n");
    return;
  }
  rmd = remove_all_news(p, str);
  if (rmd)
  {
    NEWS_CHANGED;
    TELLPLAYER(p, " Removed %d articles ...\n", rmd);
    LOGF("remove", "%s removed %d postings of %s", p->name, rmd, str);
  }
  else
    TELLPLAYER(p, " No articles found posted by %s to be removed.\n",
	       str);
}

void news_help(player * p, char *str)
{
  char holder[15];		/* use instead of taking a chance with a constant */

  memset(holder, 0, 15);
  strcpy(holder, "news");
  help(p, holder);
}


 /* lil stuffs for sus shortcutting */
void sus_news_post(player * p, char *str)
{
  char *oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format : spost <subject>\n");
    return;
  }
  sprintf(stack, "sus %s", str);
  stack = end_string(stack);

  post_news(p, oldstack);
  stack = oldstack;
}

void sus_news_list(player * p, char *str)
{
  char *oldstack = stack;

  if (!*str)
  {
    sprintf(stack, "sus");	/* use stack so we dont muck constants */
    stack = end_string(stack);
    list_news(p, oldstack);
    stack = oldstack;
    return;
  }
  sprintf(stack, "sus %s", str);
  stack = end_string(stack);

  list_news(p, oldstack);
  stack = oldstack;
}

void sus_news_read(player * p, char *str)
{
  char *oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format : sread #\n");
    return;
  }
  sprintf(stack, "sus %s", str);
  stack = end_string(stack);

  read_article(p, oldstack);
  stack = oldstack;
}

 /* and for admin ... */

void ad_news_post(player * p, char *str)
{
  char *oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format : adpost <subject>\n");
    return;
  }
  sprintf(stack, "admin %s", str);
  stack = end_string(stack);

  post_news(p, oldstack);
  stack = oldstack;
}

void ad_news_list(player * p, char *str)
{
  char *oldstack = stack;

  if (!*str)
  {
    sprintf(stack, "admin");
    stack = end_string(stack);
    list_news(p, oldstack);
    stack = oldstack;
    return;
  }
  sprintf(stack, "admin %s", str);
  stack = end_string(stack);

  list_news(p, oldstack);
  stack = oldstack;
}

void ad_news_read(player * p, char *str)
{
  char *oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format : adread #\n");
    return;
  }
  sprintf(stack, "admin %s", str);
  stack = end_string(stack);

  read_article(p, oldstack);
  stack = oldstack;
}




void news_stats(player * p, char *str)
{
  char *oldstack = stack;
  int curcnt, totalcnt = 0, i;

  stack += sprintf(stack, " The NewsGroups array is %d bytes in size.\n",
		   sizeof(NewsGroups));

  for (i = 0; NewsGroups[i].name[0]; i++)
  {
    curcnt = count_news_articles(&NewsGroups[i]);
    stack += sprintf(stack, " Newsgroup %-20s - %6d posting/s, %d bytes\n",
		  NewsGroups[i].name, curcnt, curcnt * sizeof(news_header));

    totalcnt += sizeof(news_header) * curcnt;
  }
  stack += sprintf(stack, " %d bytes is total resident memory used"
		   " by all news headers\n", totalcnt);
  stack += sprintf(stack, " %s til the next news sync",
		   word_time(news_sync));
  stack += sprintf(stack, ", %s interval.\n", word_time(NEWS_SYNC_INTERVAL));


  stack = end_string(stack);
  pager(p, oldstack);
  stack = oldstack;

}

void news_read_next(player * p, char *str)
{
  newsgroup *ng;
  int n;
  char poppy[16];

  if (*str)
  {
    ng = find_news_group(str);
    if (!ng)
    {
      TELLPLAYER(p, " No such news group '%s' ...\n", str);
      return;
    }
  }
  else
    ng = &NewsGroups[0];

  n = next_unread(p, ng);

  if (!n)
  {
    if (ng == &NewsGroups[0])
      tell_player(p, " No more unread articles.\n");
    else
      TELLPLAYER(p, " No more unread articles in '%s' group.\n", str);
    return;
  }
  sprintf(poppy, "%d", n);
  read_article(p, poppy);
}


void nunews_version(void)
{
  stack += sprintf(stack, " -=*> NuNews v0.5 (by phypor) enabled.\n");
}
