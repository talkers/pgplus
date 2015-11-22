/*
 * Playground+ - shortcut.c
 * All shortcut commands (TELLPLAYER, ADDSTACK, ENDSTACK, LOGF etc.)
 * --------------------------------------------------------------------------- 
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"

void LOGF(char *fil, char *format,...)
{
  va_list argum;
  char *oldstack;

  oldstack = stack;
  va_start(argum, format);
  vsprintf(stack, format, argum);
  va_end(argum);
  stack = end_string(stack);
  log(fil, oldstack);
  stack = oldstack;
}

void TELLPLAYER(player * pl, char *format,...)
{
  va_list argum;
  char *oldstack;

  oldstack = stack;
  va_start(argum, format);
  vsprintf(stack, format, argum);
  va_end(argum);
  stack = end_string(stack);
  tell_player(pl, oldstack);
  stack = oldstack;
}

void SUWALL(char *format,...)
{
  va_list argum;
  char *oldstack;

  oldstack = stack;
  va_start(argum, format);
  vsprintf(stack, format, argum);
  va_end(argum);
  stack = end_string(stack);
  su_wall(oldstack);
  stack = oldstack;
}

void AUWALL(char *format,...)
{
  va_list argum;
  char *oldstack;

  oldstack = stack;
  va_start(argum, format);
  vsprintf(stack, format, argum);
  va_end(argum);
  stack = end_string(stack);
  au_wall(oldstack);
  stack = oldstack;
}

void SEND_TO_DEBUG(char *format,...)
{
  va_list argum;
  char *oldstack;

  oldstack = stack;
  va_start(argum, format);
  vsprintf(stack, format, argum);
  va_end(argum);
  stack = end_string(stack);
  debug_wall(oldstack);
  stack = oldstack;
}

#ifdef HC_CHANNEL
void HCWALL(char *format,...)
{
  va_list argum;
  char *oldstack;

  oldstack = stack;
  va_start(argum, format);
  vsprintf(stack, format, argum);
  va_end(argum);
  stack = end_string(stack);
  hc_wall(oldstack);
  stack = oldstack;
}

#endif

void SW_BUT(player * but, char *format,...)
{
  va_list argum;
  char *oldstack;

  oldstack = stack;
  va_start(argum, format);
  vsprintf(stack, format, argum);
  va_end(argum);
  stack = end_string(stack);
  su_wall_but(but, oldstack);
  stack = oldstack;
}

void AW_BUT(player * but, char *format,...)
{
  va_list argum;
  char *oldstack;

  oldstack = stack;
  va_start(argum, format);
  vsprintf(stack, format, argum);
  va_end(argum);
  stack = end_string(stack);
  au_wall_but(but, oldstack);
  stack = oldstack;
}
void TELLROOM(room * here, char *format,...)
{
  va_list argum;
  char *oldstack;

  oldstack = stack;
  va_start(argum, format);
  vsprintf(stack, format, argum);
  va_end(argum);
  stack = end_string(stack);
  tell_room(here, oldstack);
  stack = oldstack;
}
void TELLROOM_BUT(player * p, room * here, char *format,...)
{
  va_list argum;
  char *oldstack;

  oldstack = stack;
  va_start(argum, format);
  vsprintf(stack, format, argum);
  va_end(argum);
  stack = end_string(stack);
  tell_room_but2(p, here, oldstack);
  stack = oldstack;
}

/* these two are a bit different, no? Well, um.. yeah, kinda... */

void ADDSTACK(char *format,...)
{
  va_list argum;

  va_start(argum, format);
  vsprintf(stack, format, argum);
  va_end(argum);
  stack = strchr(stack, 0);
}

void ENDSTACK(char *format,...)
{
  va_list argum;

  va_start(argum, format);
  vsprintf(stack, format, argum);
  va_end(argum);
  stack = end_string(stack);
}
