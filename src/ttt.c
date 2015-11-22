/*
 * Playground+ - ttt.c
 * TicTacToe game written by Bron Gondwana
 * ---------------------------------------------------------------------------
 *
 *  Changes made:
 *    - Modifications to presentation
 *    - Changed to compile cleanly under PG+ flags
 *
 */

/* headers */

#include <stdlib.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"


/* interns */
void ttt_end_game(player * p, int winner);

/* the game!! */

int ttt_is_end(player * p)
{
  int i, board[9], draw = 1;

  for (i = 0; i < 9; i++)
  {
    board[i] = (p->ttt_board >> (i * 2)) % 4;
    if (!(board[i]))
      draw = 0;
  }

  if ((board[0] && ((board[0] == board[1]) && (board[0] == board[2]))) ||
      (((board[0] == board[3]) && (board[0] == board[6]))))
    return board[0];
  if ((board[4] && ((board[4] == board[0]) && (board[4] == board[8]))) ||
      (((board[4] == board[1]) && (board[4] == board[7]))) ||
      (((board[4] == board[2]) && (board[4] == board[6]))) ||
      (((board[4] == board[3]) && (board[4] == board[5]))))
    return board[4];
  if ((board[8] && ((board[8] == board[2]) && (board[8] == board[5]))) ||
      (((board[8] == board[7]) && (board[8] == board[6]))))
    return board[8];
  if (draw)
    return 3;

  return 0;
}

void ttt_print_board(player * p)
{
  int i, temp, gameover = 0;
  char *oldstack, cells[9];

  oldstack = stack;

  if (!(p->ttt_opponent))
  {
    TELLPLAYER(p, " TTT: EEEK.. seems we've lost your opponent!\n");
    return;
  }
  if (((p->ttt_board) % (1 << 17)) != ((p->ttt_opponent->ttt_board) % (1 << 17)))
  {
    TELLPLAYER(p, " TTT: EEEK.. seems like you're playing on different boards!\n");
    ttt_end_game(p, 0);
    return;
  }
  for (i = 0; i < 9; i++)
  {
    temp = ((p->ttt_board) >> (i * 2)) % 4;
    switch (temp)
    {
      case 0:
	cells[i] = ' ';
	break;
      case 1:
	cells[i] = 'O';
	break;
      case 2:
	cells[i] = 'X';
	break;
      default:
	TELLPLAYER(p, " TTT: EEEK.. corrupt board file!!\n");
	return;
    }
  }

  if (p->ttt_board & TTT_MY_MOVE)
    ADDSTACK(" It's your move - with the board as:\n\n");
  else if (p->ttt_opponent->ttt_board & TTT_MY_MOVE)
    ADDSTACK(" %s to move - with the board as:\n\n", p->ttt_opponent->name);
  else
  {
    ADDSTACK(" The final board was:\n");
    gameover++;
  }
  ADDSTACK("  +-------+  +-------+\n");
  for (i = 0; i < 9; i += 3)
  {
    ADDSTACK("  | %c %c %c |  | %d %d %d |\n",
	     cells[i], cells[i + 1], cells[i + 2], i + 1, i + 2, i + 3);
  }
  ADDSTACK("  +-------+  +-------+\n");
  if (!gameover)
  {
    if (p->ttt_board & TTT_AM_NOUGHT)
    {
      ADDSTACK("  You are playing NOUGHTS\n");
    }
    else
    {
      ADDSTACK("  You are playing CROSSES\n");
    }
  }
  stack++;

  TELLPLAYER(p, oldstack);
  stack = oldstack;
}

void ttt_end_game(player * p, int winner)
{
  int plyr = 1;
  if (!(p->ttt_opponent))
  {
    TELLPLAYER(p, " TTT: EEEK.. seems we've lost your opponent!\n");
    return;
  }
  if (!(p->ttt_board & TTT_AM_NOUGHT))
    plyr++;

  if (winner == 3)
  {
    TELLPLAYER(p, " Your TTT game with %s is a draw.\n", p->ttt_opponent->name);
    TELLPLAYER(p->ttt_opponent, " Your TTT game with %s is a draw.\n", p->name);
    p->ttt_draw++;
    (p->ttt_opponent)->ttt_draw++;
  }
  else if (winner && (winner != plyr))
  {
    TELLPLAYER(p, " Doh!! You lost TTT to %s.\n", p->ttt_opponent->name);
    TELLPLAYER(p->ttt_opponent, " Yippee!! You beat %s at TTT.\n", p->name);
    p->ttt_loose++;
    (p->ttt_opponent)->ttt_win++;
  }
  else if (winner)
  {
    TELLPLAYER(p, " Yippee!! You beat %s at TTT.\n", p->ttt_opponent->name);
    TELLPLAYER(p->ttt_opponent, " Doh!! you lost TTT to %s.\n", p->name);
    p->ttt_win++;
    (p->ttt_opponent)->ttt_loose++;
  }
  else
  {
    TELLPLAYER(p, " You abort your TTT game against %s.\n", p->ttt_opponent->name);
    TELLPLAYER(p->ttt_opponent, " %s aborts your TTT game.\n", p->name);
  }

  p->ttt_board &= ~TTT_MY_MOVE;
  p->ttt_opponent->ttt_board &= ~TTT_MY_MOVE;

  ttt_print_board(p);
  ttt_print_board(p->ttt_opponent);

  p->ttt_opponent->ttt_opponent = 0;
  p->ttt_opponent = 0;
}

void ttt_new_game(player * p, char *str)
{
  player *p2;

  if (!*str)
  {
    TELLPLAYER(p, " Format:  ttt <player>\n");
    return;
  }
  p2 = find_player_global(str);
  if (!p2)
    return;

#ifdef ROBOTS
  if (p2->residency & ROBOT_PRIV)
  {
    tell_player(p, " You cannot play TTT against a robot\n");
    return;
  }
#endif

  if (p == p2)
  {
    TELLPLAYER(p, " Do you have NO life???  Sheesh - Forget it...\n");
    return;
  }
  if (p2->ttt_opponent)
  {
    TELLPLAYER(p, " Sorry, but %s is currently playing someone else\n", p2->name);
    return;
  }
  if (!p2->residency)
  {
    TELLPLAYER(p, " You can't play %s, because %s is not resident\n", p2->name, gstring(p2));
    return;
  }
  p->ttt_board = 0;
  p2->ttt_board = TTT_MY_MOVE + TTT_AM_NOUGHT;
  p->ttt_opponent = p2;
  p2->ttt_opponent = p;

  TELLPLAYER(p2, " %s has offered a game of Tic Tac Toe (\"ttt abort\" to cancel)\n", p->name);
  ttt_print_board(p2);
  ttt_print_board(p);
}

void ttt_make_move(player * p, char *str)
{
  int winner, temp;

  if (!*str)
  {
    TELLPLAYER(p, " You're playing a game, so\n Format:  ttt <square to play>\n"
	       " Type \"aborttt\" to abort this game\n");
    return;
  }
  if (((p->ttt_board) % (1 << 17)) != ((p->ttt_opponent->ttt_board) % (1 << 17)))
  {
    TELLPLAYER(p, " Seems you're playing on different boards!");
    ttt_end_game(p, 0);
  }
  temp = atoi(str);

  if ((temp < 1) || (temp > 9))
  {
    TELLPLAYER(p, " Please play to a square on the board!!\n");
    ttt_print_board(p);
    return;
  }
  temp--;
  temp *= 2;

  if (p->ttt_board & (3 << temp))
  {
    TELLPLAYER(p, " Sorry, that square is already taken.. please choose another\n");
    ttt_print_board(p);
    return;
  }
  if (p->ttt_board & TTT_AM_NOUGHT)
  {
    p->ttt_board |= (1 << temp);
    p->ttt_opponent->ttt_board |= (1 << temp);
  }
  else
  {
    p->ttt_board |= (2 << temp);
    p->ttt_opponent->ttt_board |= (2 << temp);
  }

  p->ttt_board &= ~TTT_MY_MOVE;
  p->ttt_opponent->ttt_board |= TTT_MY_MOVE;

  winner = ttt_is_end(p);
  if (winner)
  {
    ttt_end_game(p, winner);
  }
  else
  {
    ttt_print_board(p);
    ttt_print_board(p->ttt_opponent);
  }
}

/* wrappers */

void ttt_print(player * p, char *str)
{
  if (!p->ttt_opponent)
    TELLPLAYER(p, " But you aren't playing a game!");
  else
    ttt_print_board(p);
}

void ttt_abort(player * p, char *str)
{
  if (!p->ttt_opponent)
    TELLPLAYER(p, " But you aren't playing a game!");
  else
    ttt_end_game(p, 0);
}

/* Yuk!!! This should be written as ttt <sub_command>!
   Oh well, some peoples programing style ... --Silver */

void ttt_cmd(player * p, char *str)
{
  if (!(p->ttt_opponent))
    ttt_new_game(p, str);
  else if (!strcmp(str, "abort"))
    ttt_abort(p, "");
  else if (!strcmp(str, "print"))
    ttt_print(p, "");
  else if (p->ttt_board & TTT_MY_MOVE)
    ttt_make_move(p, str);
  else
  {
    TELLPLAYER(p, " Sorry, but %s gets to make the next move\n", p->ttt_opponent->name);
    ttt_print_board(p);
  }
}

/* Mind you I suppose it does have its advantages ... */
