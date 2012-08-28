/*
 * Our Own Version of Mahjongg Solitare
 * Copyright (c) 2001 Eric Mulvaney, Michelle Bondy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <allegro.h>

#include "mahjongg.h"

#define SCREEN_WIDTH  (640)
#define SCREEN_HEIGHT (480)
#define SCREEN_DEPTH  (32)

#define SPRITE_WIDTH  (44)
#define SPRITE_HEIGHT (54)
#define EDGE_WIDTH    (5)
#define EDGE_HEIGHT   (5)
#define FACE_WIDTH    (SPRITE_WIDTH  - EDGE_WIDTH )
#define FACE_HEIGHT   (SPRITE_HEIGHT - EDGE_HEIGHT)

#define BOARD_XOFF   (EDGE_WIDTH * 2)
#define BOARD_YOFF   (SCREEN_HEIGHT - FACE_HEIGHT * 9 - EDGE_HEIGHT)
#define BOARD_WIDTH  (SPRITE_WIDTH * 16)
#define BOARD_HEIGHT (SPRITE_HEIGHT * 9)

#define BUTTON_XOFF    (2)
#define BUTTON_YOFF    (2)
#define BUTTON_SPACING (3)
#define BUTTON_HEIGHT  (20)

#define NEW_GAME_BUTTON_WIDTH (74)
#define UNDO_BUTTON_WIDTH     (44)
#define HELP_BUTTON_WIDTH     (44)
#define QUIT_BUTTON_WIDTH     (44)

#define NEW_GAME_BUTTON_X1 (BUTTON_XOFF)
#define NEW_GAME_BUTTON_X2 (NEW_GAME_BUTTON_X1 + NEW_GAME_BUTTON_WIDTH)
#define     UNDO_BUTTON_X1 (NEW_GAME_BUTTON_X2 + BUTTON_SPACING)
#define     UNDO_BUTTON_X2 (	UNDO_BUTTON_X1 + UNDO_BUTTON_WIDTH)

#define QUIT_BUTTON_X2 (SCREEN_WIDTH - BUTTON_XOFF)
#define QUIT_BUTTON_X1 (QUIT_BUTTON_X2 - QUIT_BUTTON_WIDTH)
#define HELP_BUTTON_X2 (QUIT_BUTTON_X1 - BUTTON_SPACING)
#define HELP_BUTTON_X1 (HELP_BUTTON_X2 - HELP_BUTTON_WIDTH)

int BACKGROUND_COLOR, SELECTED_COLOR, OLD_SELECTED_COLOR, BLACK;

BITMAP *current_view, *left_view, *right_view;
BITMAP *tile, *number[18], *suit[4], *new_game, *undo, *help, *quit;

// data for editing files.
char *editing = NULL, layout_title[56] = "";
int n_pieces_left = 144;

typedef struct
{
  char title[55];
  unsigned char complete;
  unsigned char board[16][9];
} Layout;

// ***********************************************************************

typedef struct
{
  unsigned int value;
  int mode, x, y, z;
} Piece;

Piece board[16][9][3];

#define EMPTY ((unsigned int) 144)
#define BLANK ((unsigned int) 145)
#define EDGE  ((unsigned int) 146)

#define NORMAL	     (0)
#define SELECTED     (	  SELECTED_COLOR)
#define OLD_SELECTED (OLD_SELECTED_COLOR)

#define pieceInit(p,a,b,c) { (p).value = EMPTY; (p).mode = NORMAL; \
			     (p).x = (a); (p).y = (b); (p).z = (c); }

#define pieceSuit(p)	(((p).value >> 2) & 3)
#define pieceNumber(p)	((p).value >> 3)
#define pieceMatch(p,q) (!(((p).value ^ (q).value) >> 2))
#define pieceKind(p)	((p).value >> 2)

void paintPiece(BITMAP *scr, int x, int y, Piece *p)
{
  if(p->value < EMPTY)
  {
    draw_sprite(scr, tile, x, y);
    draw_sprite(scr, number[pieceNumber(*p)], x +  5, y +  7);
    draw_sprite(scr, suit[pieceSuit(*p)],     x + 15, y + 25);
  }
  else if(p->value > EMPTY)
    draw_sprite(scr, tile, x, y);
  else if(editing && p->z == 0)
  {
    line(scr, x, y, x + (FACE_WIDTH - 1),  y, SELECTED);
    line(scr, x, y, x, y + (FACE_HEIGHT - 1), SELECTED);
  }
}

void drawPiece(BITMAP *scr, Piece *p)
{
  int x = p->x * FACE_WIDTH  - p->z * EDGE_WIDTH  + BOARD_XOFF;
  int y = p->y * FACE_HEIGHT - p->z * EDGE_HEIGHT + BOARD_YOFF;

  paintPiece(scr, x, y, p);

  if(p->mode)
  {
    int i;

    for(i = 3; --i;)
      rect(scr, x + 5 - i,		  y + 5 - i,
		x + (FACE_WIDTH - 5) + i, y + (FACE_HEIGHT - 5) + i,
		p->mode);
  }

  if(editing && p->z == 0)
  {
    if(p->x == 15)
      line(scr, x + FACE_WIDTH, y,
		x + FACE_WIDTH, y + FACE_HEIGHT,
		SELECTED);
    if(p->y ==	8)
      line(scr, x,		y + FACE_HEIGHT,
		x + FACE_WIDTH, y + FACE_HEIGHT,
		SELECTED);
  }
}

void updatePiece(Piece *p)
{
  BITMAP *work = (current_view == left_view) ? right_view : left_view;

  int i, j, k;
  int lo_i = (p->x >  0) ? p->x - 1 :  0;
  int hi_i = (p->x < 15) ? p->x + 1 : 15;
  int lo_j = (p->y >  0) ? p->y - 1 :  0;
  int hi_j = (p->y <  8) ? p->y + 1 :  8;

  int x = p->x * FACE_WIDTH  - p->z * EDGE_WIDTH  + BOARD_XOFF;
  int y = p->y * FACE_HEIGHT - p->z * EDGE_HEIGHT + BOARD_YOFF;

  rectfill(work, x, y, x + (SPRITE_WIDTH - 1), y + (SPRITE_HEIGHT - 1),
      BACKGROUND_COLOR);

  for(k =    0; k <	3; k++)
  for(j = lo_j; j <= hi_j; j++)
  for(i = lo_i; i <= hi_i; i++)
    if(board[i][j][k].value < EMPTY || editing)
      drawPiece(work, &board[i][j][k]);

  scare_mouse();
  blit(work, current_view, x, y, x, y, SPRITE_WIDTH, SPRITE_HEIGHT);
  if(editing) textprintf(current_view, font, 200, 12, SELECTED,
			 "%3i pieces left", n_pieces_left);
  unscare_mouse();
}

void update()
{
  int x, y, z;

  current_view = (current_view == left_view) ? right_view : left_view;

  // draw complete background
  rectfill(current_view, 0, 0, 639, 479, BACKGROUND_COLOR);

  draw_sprite(current_view, new_game, NEW_GAME_BUTTON_X1, BUTTON_YOFF);
  draw_sprite(current_view,	undo,	  UNDO_BUTTON_X1, BUTTON_YOFF);
  draw_sprite(current_view,	help,	  HELP_BUTTON_X1, BUTTON_YOFF);
  draw_sprite(current_view,	quit,	  QUIT_BUTTON_X1, BUTTON_YOFF);

  if(editing)
  {
    textprintf(current_view, font, 200,  2, SELECTED,
	       "Editing: '%s' (%s)", layout_title, get_filename(editing));
    textprintf(current_view, font, 200, 12, SELECTED,
	       "%3i pieces left", n_pieces_left);
  }

  for(z = 0; z <  3; z++)
  for(y = 0; y <  9; y++)
  for(x = 0; x < 16; x++)
    if(board[x][y][z].value < EMPTY || editing)
      drawPiece(current_view, &board[x][y][z]);

  scroll_screen((current_view == left_view) ? 0 : SCREEN_WIDTH, 0);
  show_mouse(current_view);
}

Piece moves[144];
int n_moves, n_pairs_remaining;
int selectPiece(Piece *p);

void undoMove()
{
  if(n_moves)
  {
    Piece *p1 = &moves[--n_moves];
    Piece *p2 = &moves[--n_moves];
    board[p1->x][p1->y][p1->z].value = p1->value;
    board[p2->x][p2->y][p2->z].value = p2->value;
    selectPiece(NULL);
    updatePiece(p1);
    updatePiece(p2);
    n_pairs_remaining++;
  }
}

void giveHelp(int always);
void makeMove(Piece *p1, Piece *p2)
{
  p1->mode = NORMAL; moves[n_moves++] = *p1; p1->value = BLANK;
  p2->mode = NORMAL; moves[n_moves++] = *p2; p2->value = BLANK;
  updatePiece(p1);
  updatePiece(p2);
  n_pairs_remaining--;
  giveHelp(0);
}

int selectPiece(Piece *p)
{
  static Piece *s1 = NULL, *s2 = NULL;

  if(!p) // reset
  {
    if(s1) { s1->mode = NORMAL; updatePiece(s1); s1 = NULL; }
    if(s2) { s2->mode = NORMAL; updatePiece(s2); s2 = NULL; }
    return 0;
  }

  if(p->value >= EMPTY
     || (p->x > 0 && p->x < 15
	  && board[p->x-1][p->y][p->z].value < EMPTY
	  && board[p->x+1][p->y][p->z].value < EMPTY)
     || (p->z < 2
	  && board[p->x][p->y][p->z+1].value < EMPTY))
    return 0;

  if(!s1)
  {
    s1 = p;
    s1->mode = SELECTED;
    updatePiece(s1);
  }
  else if(p == s1)
  {
    s1->mode = NORMAL;
    if(s2)
    {
      s2->mode = SELECTED;
      updatePiece(s2);
    }
    updatePiece(s1);
    s1 = s2;
    s2 = NULL;
  }
  else if(pieceMatch(*p, *s1))
  {
    if(s2)
    {
      s2->mode = SELECTED;
      updatePiece(s2);
    }
    makeMove(p, s1);
    s1 = s2;
    s2 = NULL;
  }
  else
  {
     p->mode = SELECTED;
    s1->mode = OLD_SELECTED;
    if(s2 && p != s2)
    {
      s2->mode = NORMAL;
      updatePiece(s2);
    }
    updatePiece(p);
    updatePiece(s1);
    s2 = s1;
    s1 = p;
  }

  return 1;
}

void shuffle()
{
  int x, y, z, i, j, k, pairs[72];
  Piece *edges[144]; int n_edges;

top:
  n_edges = 0;

  // clean the board.
  for(x = 16; x--;)
  for(y =  9; y--;)
  for(z =  3; z--;)
    if(board[x][y][z].value != EMPTY)
      board[x][y][z].value = BLANK;

  // find the edges of the board
  for(y = 9; y--;)
  {
    for(z = 3; z--;) if(board[ 0][y][z].value == BLANK)
      { (edges[n_edges++] = &board[ 0][y][z])->value = EDGE; break; }
    for(z = 3; z--;) if(board[15][y][z].value == BLANK)
      { (edges[n_edges++] = &board[15][y][z])->value = EDGE; break; }
  }

  for(x = 15; --x;)
  for(y =  9; y--;)
  for(z =  3; z--;)
    if(board[x][y][z].value == BLANK)
    {
      if(board[x-1][y][z].value == EMPTY || board[x+1][y][z].value == EMPTY)
	(edges[n_edges++] = &board[x][y][z])->value = EDGE;
      break;
    }

  // shuffle the tiles in pairs
  for(i = 72; i--;)
    pairs[i] = i << 1;
  for(i = 72; i--;)
  {
    int temp = pairs[i];
    pairs[i] = pairs[j = rand() % 72];
    pairs[j] = temp;
  }

  // place the tiles
  for(i = 72; i--;)
  {
    int n = n_edges;
    for(j =  2; j--;)
    {
      Piece *edge;

      if(!n)
	goto top;

      edge     = edges[k = rand() % n];
      edges[k] = edges[--n];

      edge->value = pairs[i]++;

      x = edge->x;
      y = edge->y;
      z = edge->z;

      if(x > 0
	  && board[x-1][y][z].value == BLANK
	  && (z == 2 || board[x-1][y][z+1].value < BLANK))
	(edges[n++] = &board[x-1][y][z])->value = EDGE;

      if(x < 15
	  && board[x+1][y][z].value == BLANK
	  && (z == 2 || board[x+1][y][z+1].value < BLANK))
	(edges[n++] = &board[x+1][y][z])->value = EDGE;

      if(z > 0 && (x == 0 || x == 15
	  || board[x-1][y][z-1].value < BLANK
	  || board[x+1][y][z-1].value < BLANK))
      (edges[n++] = &board[x][y][z-1])->value = EDGE;
    }
    n_edges = n;
  }

  n_moves = 0;
  n_pairs_remaining = 72;
  selectPiece(NULL);
  update();
}

void giveHelp(int always)
{
  int x, y, z, kind_selectable[36], n_possible_moves = 0;

  for(x=36; x--;)
    kind_selectable[x] = 0;

  for(y=9; y--;)
  {
    Piece *p1 = (board[ 0][y][2].value < EMPTY) ? &board[ 0][y][2] :
		(board[ 0][y][1].value < EMPTY) ? &board[ 0][y][1] :
		(board[ 0][y][0].value < EMPTY) ? &board[ 0][y][0] : NULL;
    Piece *p2 = (board[15][y][2].value < EMPTY) ? &board[15][y][2] :
		(board[15][y][1].value < EMPTY) ? &board[15][y][1] :
		(board[15][y][0].value < EMPTY) ? &board[15][y][0] : NULL;

    if(p1 && kind_selectable[pieceKind(*p1)]++) n_possible_moves++;
    if(p2 && kind_selectable[pieceKind(*p2)]++) n_possible_moves++;
  }

  for(x = 1; x < 15; x++)
    for(y=9; y--;)
    for(z=3; z--;)
      if(board[x][y][z].value < EMPTY)
      {
	if((board[x-1][y][z].value >= EMPTY || board[x+1][y][z].value >= EMPTY)
	    && kind_selectable[pieceKind(board[x][y][z])]++)
	  n_possible_moves++;
	break;
      }

  if(always || (!n_possible_moves && n_pairs_remaining))
  {
    char msg[29];

    if(n_possible_moves)
      sprintf(msg, "There %s %i possible move%s.",
	  n_possible_moves > 1 ? "are" : "is",
	  n_possible_moves,
	  n_possible_moves > 1 ? "s" : "");
    else
      sprintf(msg, "There are no possible moves.");

    if(current_view != left_view)
      update();

    alert(msg, NULL, NULL, "Okay", NULL, 0, 0);
  }
}

// ***********************************************************************

volatile int click_ready = 0, click_x, click_y, click_x2, click_y2;

void mouse_handler(int flags)
{
  if(flags & (MOUSE_FLAG_LEFT_DOWN | MOUSE_FLAG_RIGHT_DOWN) && !click_ready)
  {
    click_x = mouse_x;
    click_y = mouse_y;
    click_ready = (flags & MOUSE_FLAG_LEFT_DOWN) ? 1 : 2;
  }
}

void mouse_handler_for_editing(int flags)
{
  if(flags & (MOUSE_FLAG_LEFT_DOWN | MOUSE_FLAG_RIGHT_DOWN) && !click_ready)
  {
    click_x = mouse_x;
    click_y = mouse_y;
    click_ready = (flags & MOUSE_FLAG_LEFT_DOWN) ? 1 : 2;
  }
  else if(flags & MOUSE_FLAG_MOVE && !click_ready && mouse_b & 3)
  {
    int x = mouse_x;
    int y = mouse_y;

    if	   (x > click_x && x - click_x >= FACE_WIDTH ) click_x += FACE_WIDTH;
    else if(x < click_x && click_x - x >= FACE_WIDTH ) click_x -= FACE_WIDTH;
    else if(y > click_y && y - click_y >= FACE_HEIGHT) click_y += FACE_HEIGHT;
    else if(y < click_y && click_y - y >= FACE_HEIGHT) click_y -= FACE_HEIGHT;
    else return;

    click_ready = (mouse_b & 1) ? 1 : 2;
  }
}

#define N_LAYOUTS (8)
Layout *layouts[N_LAYOUTS];

#define PATH_LENGTH (128)
char path[PATH_LENGTH] = "";

void defaultLayout();
void newGame();

int main(int argc, char *argv[])
{
  {
    int i;

    if((argc - 1) % 2)
      goto help;

    for(i = 1; i < argc - 1; i += 2)
      if(!editing && (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--edit")))
	editing = argv[i+1];
      else if(!layout_title[0] && (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--title")))
      {
	bzero(layout_title, 55);
	strncpy(layout_title, argv[i+1], 54);
      }
      else
      {
help:
	printf("usage: mahjongg [--edit <layout> [--title <layout title>]]\n");
	return 0;
      }
  }

  srand(time(NULL));
  allegro_init();

  {
    int x, y, z;
    for(x = 16; x--;)
    for(y =  9; y--;)
    for(z =  3; z--;)
      pieceInit(board[x][y][z], x, y, z);
  }

  set_color_depth(SCREEN_DEPTH);

  if(set_gfx_mode(GFX_AUTODETECT,
      SCREEN_WIDTH,   SCREEN_HEIGHT,
      SCREEN_WIDTH*2, SCREEN_HEIGHT) < 0)
  {
    fprintf(stderr, "fatal: %s\n", allegro_error);
    exit(1);
  }

#ifdef ALLEGRO_WINDOWS
  set_display_switch_callback(SWITCH_IN, update);
#endif

  left_view =
    create_sub_bitmap(screen, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  right_view =
    create_sub_bitmap(screen, SCREEN_WIDTH, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

  // init colors
    BACKGROUND_COLOR = makecol32(0x2F, 0x5F, 0x2F); // soft green
      SELECTED_COLOR = makecol32(0x00, 0xDF, 0x00); // green
  OLD_SELECTED_COLOR = makecol32(0x00, 0xBF, 0xBF); // cyan

  // load data
  {
    DATAFILE *data = load_datafile("#");
    new_game   = data[NEW_GAME_BMP].dat;
    undo       = data[UNDO_BMP].dat;
    help       = data[HELP_BMP].dat;
    quit       = data[QUIT_BMP].dat;
    tile       = data[TILE_BMP].dat;
    number[ 0] = data[BLACK1_BMP].dat;
    number[ 2] = data[BLACK2_BMP].dat;
    number[ 4] = data[BLACK3_BMP].dat;
    number[ 6] = data[BLACK4_BMP].dat;
    number[ 8] = data[BLACK5_BMP].dat;
    number[10] = data[BLACK6_BMP].dat;
    number[12] = data[BLACK7_BMP].dat;
    number[14] = data[BLACK8_BMP].dat;
    number[16] = data[BLACK9_BMP].dat;
    number[ 1] = data[RED1_BMP].dat;
    number[ 3] = data[RED2_BMP].dat;
    number[ 5] = data[RED3_BMP].dat;
    number[ 7] = data[RED4_BMP].dat;
    number[ 9] = data[RED5_BMP].dat;
    number[11] = data[RED6_BMP].dat;
    number[13] = data[RED7_BMP].dat;
    number[15] = data[RED8_BMP].dat;
    number[17] = data[RED9_BMP].dat;
    suit[0]    = data[SPADE_BMP].dat;
    suit[1]    = data[CLUB_BMP].dat;
    suit[2]    = data[DIAMOND_BMP].dat;
    suit[3]    = data[HEART_BMP].dat;
    layouts[0] = data[BLOCK_LYT].dat;
    layouts[1] = data[FLAT_LYT].dat;
    layouts[2] = data[FROGGER_LYT].dat;
    layouts[3] = data[PRECIOUS_LYT].dat;
    layouts[4] = data[PTRAD_LYT].dat;
    layouts[5] = data[PYRAMID_LYT].dat;
    layouts[6] = data[STEPS_LYT].dat;
    layouts[7] = data[THETA_LYT].dat;
  }

  scroll_screen(SCREEN_WIDTH, 0);
  current_view = right_view;

  install_timer();
  install_mouse();
  install_keyboard();
  show_mouse(current_view);

  text_mode(BACKGROUND_COLOR);

  if(!editing)
  {
    defaultLayout();

    if(alert("Our Own Version of Mahjongg Solitare, v0.1.4", NULL,
	     "Copyright (c) 2001 Eric Mulvaney, Michelle Bondy",
	     "Play", "Edit", 0, 0) == 2
	&& file_select_ex("Please select layout file to edit:", path, "lyt",
	    PATH_LENGTH-1, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
    {
      int x, y, z;

      editing = path;

      for(x = 16; x--;)
      for(y =  9; y--;)
      for(z =  3; z--;)
	board[x][y][z].value = EMPTY;
    }
  }

  mouse_callback = editing ? mouse_handler_for_editing : mouse_handler;

  if(editing)
  {
    Layout data;
    FILE *file = fopen(editing, "r");

    if(file)
    {
      if(fread(&data, sizeof(Layout), 1, file))
      {
	int x, y, z;

	if(!layout_title[0])
	  memcpy(layout_title, data.title, 55);

	for(x = 16; x--;)
	for(y =  9; y--;)
	for(z = (data.board[x][y] > 3) ? 3 : data.board[x][y]; z--;)
	{
	  board[x][y][z].value = BLANK;
	  if(!--n_pieces_left)
	    goto skip;
	}
      }
skip:
      fclose(file);
    }

    update();
  }

  click_ready = 0;
  while(1) // game loop
  {
    if(click_ready)
    {
      int x = click_x - (BOARD_XOFF - 2 * EDGE_WIDTH );
      int y = click_y - (BOARD_YOFF - 2 * EDGE_HEIGHT);
      int z;

      for(z = 3; x > 0 && y > 0 && z--; x -= EDGE_WIDTH, y -= EDGE_HEIGHT)
      {
	int i = x / FACE_WIDTH;
	int j = y / FACE_HEIGHT;

	if(i >= 16 || j >= 9)
	  continue;

	if(editing)
	{
	  if(click_ready == 1 && board[i][j][z].value == EMPTY)
	  {
	    if((z == 0 || board[i][j][z-1].value != EMPTY) && n_pieces_left)
	    {
	      n_pieces_left--;
	      board[i][j][z].value = BLANK;
	      updatePiece(&board[i][j][z]);
	      goto event_handled;
	    }
	  }
	  else if(click_ready == 2 && board[i][j][z].value != EMPTY)
	  {
	    if(z == 2 || board[i][j][z+1].value == EMPTY)
	    {
	      board[i][j][z].value = EMPTY;
	      n_pieces_left++;
	      updatePiece(&board[i][j][z]);
	      goto event_handled;
	    }
	  }
	}
	else if(selectPiece(&board[i][j][z]))
	{
	  if(!n_pairs_remaining)
	  {
	    if(current_view != left_view)
	      update();

	    if(alert("Congratulations!	You won!",
		     "Play another?",
		     NULL, "Yes", "No", 0, 0) == 1)
	      newGame();
	    else
	      return 0;
	  }

	  goto event_handled;
	}
      }

      if(click_y < BUTTON_YOFF + BUTTON_HEIGHT && click_y > BUTTON_YOFF)
      {
	if(editing)
	{
	  if(click_x > NEW_GAME_BUTTON_X1 && click_x < NEW_GAME_BUTTON_X2)
	  {
	    if(n_pieces_left == 144)
	      goto event_handled;

	    if(current_view != left_view)
	      update();

	    if(alert("Are you sure you want to clear the current Layout?",
		     NULL, NULL, "Yes", "No", 0, 0) == 1)
	    {
	      int x, y, z;
	      for(x = 16; x--;)
	      for(y =  9; y--;)
	      for(z =  3; z--;)
		board[x][y][z].value = EMPTY;
	      n_pieces_left = 144;
	      update();
	    }
	  }
	  else if(click_x > QUIT_BUTTON_X1 && click_x < QUIT_BUTTON_X2)
	  {
	    int ync;

	    if(current_view != left_view)
	      update();

	    if(n_pieces_left)
	      ync = alert3("WARNING: Layout is incomplete.",
			   NULL,
			   "Do you wish to save before exiting?",
			   "Yes", "No", "Cancel", 0, 0, 0);
	    else
	      ync = alert3("Do you wish to save before exiting?",
		    NULL, NULL, "Yes", "No", "Cancel", 0, 0, 0);

	    if(ync == 2)
	      return 0;
	    else if(ync == 1)
	    {
	      Layout data;
	      FILE *file;

	      memcpy(data.title, layout_title, 55);

	      data.complete = (n_pieces_left) ? 0 : 1;

	      if((file = fopen(editing, "w")))
	      {
		for(x = 16; x--;)
		for(y =  9; y--;)
		{
		  if	 (board[x][y][2].value == BLANK) data.board[x][y] = 3;
		  else if(board[x][y][1].value == BLANK) data.board[x][y] = 2;
		  else if(board[x][y][0].value == BLANK) data.board[x][y] = 1;
		  else					 data.board[x][y] = 0;
		}

		if(fwrite(&data, sizeof(Layout), 1, file))
		{
		  fclose(file);
		  return 0;
		}
		else
		  fclose(file);
	      }

	      if(alert("WARNING: Save failed!",
		       NULL,
		       "Do you still wish to exit?",
		       "Yes", "No", 0, 0) == 1)
		return 0;
	    }
	  }
	}
	else if(click_x > NEW_GAME_BUTTON_X1 && click_x < NEW_GAME_BUTTON_X2)
	  newGame();
	else if(click_x > UNDO_BUTTON_X1 && click_x < UNDO_BUTTON_X2)
	  undoMove();
	else if(click_x > HELP_BUTTON_X1 && click_x < HELP_BUTTON_X2)
	  giveHelp(1);
	else if(click_x > QUIT_BUTTON_X1 && click_x < QUIT_BUTTON_X2)
	{
	  if(current_view != left_view)
	    update();

	  if(alert("Are you sure you want to quit?",
	      NULL, NULL, "Yes", "No", 0, 0) == 1)
	    return 0;
	}
      }

event_handled:

      click_ready = 0;
    }
    else
      rest(100);
  }

  return 0;
}
END_OF_MAIN()

// ************************************************************************

void symmetryLayout()
{
  int x, y, z, i;

  for(i = 144/4; i;)
  {
    x = rand() % 16;
    y = rand() %  8;

    for(z = 0; z < 3; z++)
      if(board[x][y][z].value == EMPTY)
      {
	board[	 x][  y][z].value = BLANK;
	board[15-x][  y][z].value = BLANK;
	board[15-x][7-y][z].value = BLANK;
	board[	 x][7-y][z].value = BLANK;
	i--;
	break;
      }
  }
}

void tightSymmetryLayout()
{
  int x, y, z, i;

  for(i = 144/4; i;)
  {
    x = rand() % 16;
    y = rand() %  8;

    for(z = 0; z < 3; z++)
      if(board[x][y][z].value == EMPTY)
      {
	if((x < 7 && board[x+1][y][z].value == EMPTY) ||
	   (x > 8 && board[x-1][y][z].value == EMPTY) ||
	   (y < 3 && board[x][y+1][z].value == EMPTY) ||
	   (y > 4 && board[x][y-1][z].value == EMPTY))
	  break;

	board[	 x][  y][z].value = BLANK;
	board[15-x][  y][z].value = BLANK;
	board[15-x][7-y][z].value = BLANK;
	board[	 x][7-y][z].value = BLANK;
	i--;
	break;
      }
  }
}

void mirrorLayout()
{
  int x, y, z, i;

  for(i = 144/2; i;)
  {
    x = rand() % 16;
    y = rand() %  9;

    for(z = 0; z < 3; z++)
      if(board[x][y][z].value == EMPTY)
      {
	board[	 x][y][z].value = BLANK;
	board[15-x][y][z].value = BLANK;
	i--;
	break;
      }
  }
}

void tightMirrorLayout()
{
  int x, y, z, i;

  for(i = 144/2; i;)
  {
    x = rand() % 16;
    y = rand() %  9;

    for(z = 0; z < 3; z++)
      if(board[x][y][z].value == EMPTY)
      {
	if((x < 7 && board[x+1][y][z].value == EMPTY) ||
	   (x > 8 && board[x-1][y][z].value == EMPTY) ||
	   (y < 4 && board[x][y+1][z].value == EMPTY) ||
	   (y > 4 && board[x][y-1][z].value == EMPTY))
	  break;

	board[	 x][y][z].value = BLANK;
	board[15-x][y][z].value = BLANK;
	i--;
	break;
      }
  }
}

// ************************************************************************

#define N_GENERATORS (4)

struct {
  char *name;
  void (*generator)();
} generators[N_GENERATORS] = {
  { "Random Symmetry",	     symmetryLayout	 },
  { "Random Mirror",	     mirrorLayout	 },
  { "Random Tight Symmetry", tightSymmetryLayout },
  { "Random Tight Mirror",   tightMirrorLayout	 },
};

void loadLayout(int i)
{
  int x, y, z;

  for(x = 16; x--;)
  for(y =  9; y--;)
  for(z =  3; z--;)
    board[x][y][z].value = EMPTY;

  if(i < N_GENERATORS)
    (*generators[i].generator)();
  else
  {
    Layout *data = layouts[i - N_GENERATORS];

    for(x = 16; x--;)
    for(y =  9; y--;)
    for(z = data->board[x][y]; z--;)
      board[x][y][z].value = BLANK;
  }

  shuffle();
}

void defaultLayout()
{
  loadLayout(0);
}

void loadLayoutFromFile()
{
  if(file_select_ex("Please select layout file to play:", path, "lyt",
      PATH_LENGTH-1, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT))
  {
    FILE *file = fopen(path, "r");

    if(file)
    {
      Layout data;

      if(fread(&data, sizeof(Layout), 1, file))
      {
	int x, y, z;

	for(x = 0; x < 54; x++)
	  if(data.title[x] == 0)
	    break;
	  else if(data.title[x] < 0x20 || data.title[x] > 0x7e)
	    goto corrupt;
	for(; x < 55; x++)
	  if(data.title[x] != 0)
	    goto corrupt;

	z = 144;
	for(x = 16; x--;)
	for(y =  9; y--;)
	  if(data.board[x][y] > 3)
	    goto corrupt;
	  else
	    z--;

	if(data.complete != 1)
	{
	  alert("Layout file is incomplete:", NULL, path, "Okay", NULL, 0, 0);
	  goto close;
	}

	if(z)
	  goto corrupt;

	for(x = 16; x--;)
	for(y =  9; y--;)
	{
	  for(z = 3; z > data.board[x][y];)
	    board[x][y][--z].value = EMPTY;
	  while(z)
	    board[x][y][--z].value = BLANK;
	}

	shuffle();
      }
      else
      {
corrupt:
	alert("Layout file invalid or corrupt:", NULL, path, "Okay", NULL, 0, 0);
      }

close:
      fclose(file);
    }
    else
      alert("Could not open layout file:", NULL, path, "Okay", NULL, 0, 0);
  }
}

// ************************************************************************

char *layoutList(int index, int *list_size)
{
  if(index < 0)
  {
    *list_size = N_GENERATORS + N_LAYOUTS + 2;
    return NULL;
  }
  else if(index == 0)
    return "[ current layout ]";
  else if(index == 1)
    return "[ load layout from file... ]";
  else if(index < N_GENERATORS + 2)
    return generators[index - 2].name;
  else
    return layouts[index - N_GENERATORS - 2]->title;
}

#define BLACK (0x000000)
#define WHITE (0XFFFFFF)

//	     { proc, x, y, w, h, fg, bg, key, flags, d1, d2, dp, dp2, dp3 }
DIALOG dlg[] = {
  { d_box_proc,      0,   0, 400, 200, BLACK, WHITE, 0,      0, 0, 0, NULL, NULL, NULL },
  { d_text_proc,     5,   5, 390,  10, BLACK, WHITE, 0,      0, 0, 0, "New Game -- Please select a board layout:", NULL, NULL },
  { d_list_proc,     5,  17, 390, 158, BLACK, WHITE, 0, D_EXIT, 0, 0, layoutList, NULL, NULL },
  { d_button_proc, 270, 180,  60,  15, BLACK, WHITE, 0, D_EXIT, 0, 0, "Okay",	  NULL, NULL },
  { d_button_proc, 335, 180,  60,  15, BLACK, WHITE, 0, D_EXIT, 0, 0, "Cancel",   NULL, NULL },
  { NULL, 0, 0, 0, 0, BLACK, WHITE, 0, 0, 0, 0, NULL, NULL, NULL }
};

#define BOARD_SELECTED (dlg[2].d1)
#define CANCEL (4)

void newGame()
{
  static int configured = 0;

  if(!configured)
  {
    centre_dialog(dlg);
    configured = 1;
  }

  if(current_view != left_view)
    update();

  if(popup_dialog(dlg, -1) != CANCEL)
    switch(BOARD_SELECTED)
    {
    case 0:  shuffle(); break;
    case 1:  loadLayoutFromFile(); break;
    default: loadLayout(BOARD_SELECTED - 2);
    }
}
