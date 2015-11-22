/*
 * Playground+ - anti-crash.c
 * Attempts to keep the talker up during minor problems (ie. bad code)
 * -----------------------------------------------------------------------
 */

#include <setjmp.h>

/* somewhere near the top of glue.c  */
jmp_buf recover_jmp_env;
int total_recovers;

void mid_program_error(int dummy)
{
  int meep;

  total_recovers++;
  if (MAX_CRASH == 0)
    meep = 0;
  else
    meep = MAX_CRASH - total_recovers;

  if (!meep)
    handle_error("Too many crash recoveries");

  if (current_player)
  {
    SUWALL(" -=*> %s just caused a program error, recovering...\n", current_player->name);
    if (*(current_player->ibuffer))
      LOGF("recover", "%s caused program error using: %s", current_player->name, current_player->ibuffer);
    else
      LOGF("recover", "%s caused program error with no buffer", current_player->name);
    current_player->flags |= CHUCKOUT;
    tell_player(current_player, "\n\n"
		LINE
		"\nThis command has caused a system error to occur.\n"
       "Your connection has been terminated to prevent the code crashing.\n"
      "Please avoid usage of this command until we get it fixed. Sorry!\n\n"
		LINE
		"\n\007\n");
    quit(current_player, 0);
  }
  else
  {
    su_wall(" -=*> Program error occurred, recovering...\n");
    LOGF("recover", "Program error recovered, action: %s", action);
  }
  longjmp(recover_jmp_env, 0);
}


void crashrec_version(void)
{
  sprintf(stack, " -=*> Anti-crash code v1.0 (by subtle) enabled.\n");
  stack = strchr(stack, 0);
}
