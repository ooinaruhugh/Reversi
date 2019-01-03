/*
Random player with evaluation function. Picks move with highest score. Only regards currently possible moves, no game tree.
make eval_player

This player accepts commands from standard input. It is designed to be used with 
the ReversiController. Usage of the ReversiController:

java -jar ReversiController.jar <player1.exe> <player2.exe> [options]
Options:
repeat=<n>    how many games to play
show=0|1      whether to show each state
timeout=-1|return|<ms>    -1 for infinite wait, return for continue after key press, <ms> for timeout in ms
Examples:
java -jar ReversiController.jar ./random_player ./eval_player . repeat=1 show=1 timeout=-1 stderr=0
java -jar ReversiController.jar random_player.exe eval_player.exe . repeat=1 show=1 timeout=-1 stderr=0
java -jar ReversiController.jar ./random_player ./eval_player . repeat=100 show=0 timeout=-1 stderr=0
java -jar ReversiController.jar random_player.exe eval_player.exe . repeat=100 show=0 timeout=-1 stderr=0

Commands (cp = controller to player, pc = player to controller, | means or):

Initialization (tells the player, which stones it has):
cp: init: X | O

Seeding the random number generator:
cp: srand: <number>

Stopping the player:
cp: exit

Receiving the opponent move (or none) and responding with our own move (or none):
cp: d6 | none (opponent move)
pc: e6 | none (own move)
*/

///////////////////////////////////////////////////////////////////////////////

#include "base.h"
#include <stdint.h>
#include "definitions.h"


Position make_position(int x, int y)
{
    Position p = {x, y};
    return p;
}

void print_position(Position p)
{
    fprintf(stderr, "%c%d\n", p.x + 'a', p.y + 1);
}

// #define N 8

/* typedef struct
{
    char board[N][N]; // the NxN playing board
    char my_stone;
} Game;
 */
// Initialize the board such that it looks like this if printed:
//  |A|B|C|D|E|F|G|H|
// 1|_|_|_|_|_|_|_|_|
// 2|_|_|_|_|_|_|_|_|
// 3|_|_|_|_|_|_|_|_|
// 4|_|_|_|O|X|_|_|_|
// 5|_|_|_|X|O|_|_|_|
// 6|_|_|_|_|_|_|_|_|
// 7|_|_|_|_|_|_|_|_|
// 8|_|_|_|_|_|_|_|_|
uint_fast64_t possible_moves(Game *g);
Game *init_game(Players current_player)
{
  // todo: implement
  Game *g = malloc(sizeof(*g));
  g->current_player = current_player;
  g->board[BLACK] = field_at(4, 3) | field_at(3, 4); // Replace with actual magic bit pattern 0x810000000
  g->board[WHITE] = field_at(3, 3) | field_at(4, 4); // For maximum beauty 0x1008000000
  g->legal_moves = possible_moves(g);            

  return g;
}

static inline Game *copy_state(Game *g)
{
  return memcpy(malloc(sizeof(*g)), g, sizeof(*g));
}

// This makes "curling" (or everything involving coordinates) more beautiful
// Bitshifting with negative shift values is scary because undefined, so we need two cases.
uint_fast64_t bitshift(uint_fast64_t left_value, short shift_count)
{
  if (shift_count < 0)
    return left_value >> -shift_count;
  else
    return left_value <<= shift_count;
}

// We slide all our stones into a certain direction. They only slide as far as rows of enemy stones carry them.
// Stones that are not protected by enemy stones will just vanish, as will those next to a specific edge.
uint_fast64_t curling(Game *g, uint_fast64_t edge, short direction)
{
  // We blank out the fields for the edge of whatever direction we are going to
  // since we can't set stones beyond the edge.
  uint_fast64_t non_edge_set = g->board[!(g->current_player)] & edge;

  // Now we shift the current player's stones in one direction
  // If we encounter an enemy stone, there might be a possible turn.
  // Otherwise said stone will vanish.
  uint_fast64_t possible = non_edge_set & (bitshift(g->board[g->current_player], direction));

  // We're a doing that eight times (since it is a 8x8 board, eh?)
  for (int i = 0; i < 6; i++)
  {
    // If we continue to encounter enemy stones we move forward
    // Otherwise we stay put wherever that stone is.
    // (Think Pokémon ice-floor maze)
    possible |= non_edge_set & bitshift(possible, direction);
  }

  // Now we check whether behind an enemy row there is a free spot
  // If so, we slide our stones there and, et voila, we have possible moves.
  // Otherwise, we discard that move candidate.
  possible = (empty(g) & bitshift(possible, direction));

  return possible;
}

// Computes all possible moves on the board for eight directions
// todo: please use enums, even if loop gets impossible to use
// todo: call-by-reference
uint_fast64_t possible_moves(Game *g)
{
  uint_fast64_t moves = 0;

  /* for (size_t i = 0; i < 8; i++)
  {
    moves |= curling(g, edge[i], delta[i]);
  } */

  // curling only works for one direction
  // So we do that eight times
  moves |= curling(g, BOTTOM, DOWN);
  moves |= curling(g, ERIGHT, DRIGHT);
  moves |= curling(g, BOTTOM_RIGHT, DOWN_RIGHT);
  moves |= curling(g, BOTTOM_LEFT, DOWN_LEFT);
  moves |= curling(g, UPPER, UP);
  moves |= curling(g, ELEFT, DLEFT);
  moves |= curling(g, UPPER_RIGHT, UP_RIGHT);
  moves |= curling(g, UPPER_LEFT, UP_LEFT);

  return moves;
}

void print_row(Game *g, int row)
{
  // Printing the head row
  if (row == 0)fprintf(stderr, " |A|B|C|D|E|F|G|H|\n");
  // Printing a "normal" row

  fprintf(stderr, "%i|", row + 1);
  for (int i = 0; i < 8; i++)
  {
    fprintf(stderr,
            "%c|",
            g->board[BLACK] & field_at(i, row) ? 'X' : g->board[WHITE] & field_at(i, row) ? 'O' : '_');
  }

  fprintf(stderr, "\n");
}

// Print the board. The initial board should look like shown above.
void print_board(Game *g)
{
  for (int i = 0; i < 8; i++)
  {
    print_row(g, i);
  }
  fflush(stderr);
}

// Check whether position (x,y) is on the board.
bool out_of_bounds(int x, int y)
{
  return x < 0 || x > N - 1 || y < 0 || y > N - 1;
}

static inline bool which_stone(char c)
{
  return c == 'O';
}

// If it is X's turn, then "my stone" is 'X', otherwise it is 'O'.
static inline char my_stone(Game *g)
{
  return g->current_player ? 'O' : 'X';
}

// If it is X's turn, then "your stone" is 'O', otherwise it is 'X'.
static inline char your_stone(Game *g)
{
  return g->current_player ? 'X' : 'O';
}

static inline void switch_stones(Game *g)
{
  g->current_player = !g->current_player;
  g->legal_moves = possible_moves(g);
}

// Check whether (x,y) is a legal position to place a stone. A position is legal
// if it is empty ('_'), is on the board, and has at least one legal direction.
static inline bool legal(Game *g, int x, int y)
{
  return is_set(g->legal_moves, x, y);
}

uint_fast64_t reverse_dir(Game *g, int x, int y, uint_fast64_t edge, short direction)
{
  uint_fast64_t non_edge_set = g->board[!(g->current_player)] & edge;

  // This time, while we are sliding player's stone, we are effectively converting
  // every enemy stone that is encountered
  uint_fast64_t result = non_edge_set & (bitshift(field_at(x, y), direction));

  for (int i = 0; i < 6; i++)
  {
    result |= non_edge_set & bitshift(result, direction);
  }

  // If with the last step we arrive at one of current player's stones
  // Then we can commit our changes to the board. Otherwise, zero, niet, nada, nichts da.
  return (g->board[g->current_player] & bitshift(result, direction)) ? result : 0;
}

// Reverse the stones in all legal directions starting at (x,y).
// May modify the state of the game.
// todo: use the curling algorithm
// Probably let our one stone slide over the board without blanking the occupied slots
void reverse(Game *g, int x, int y)
{
  g->board[g->current_player] |= field_at(x, y);

  uint_fast64_t result = 0;

  /* for (size_t i = 0; i < 8; i++)
  {
    result |= reverse_dir(g, x, y, edge[i], -delta[i]);
  } */

  // We are gathering the changes as results of possible turns.
  result |= reverse_dir(g, x, y, BOTTOM, -DOWN);
  result |= reverse_dir(g, x, y, ERIGHT, -DRIGHT);
  result |= reverse_dir(g, x, y, BOTTOM_RIGHT, -DOWN_RIGHT);
  result |= reverse_dir(g, x, y, BOTTOM_LEFT, -DOWN_LEFT);
  result |= reverse_dir(g, x, y, UPPER, -UP);
  result |= reverse_dir(g, x, y, ELEFT, -DLEFT);
  result |= reverse_dir(g, x, y, UPPER_RIGHT, -UP_RIGHT);
  result |= reverse_dir(g, x, y, UPPER_LEFT, -UP_LEFT);

  // And then we commit them to the bitboards.
  g->board[g->current_player] |= result;
  g->board[!(g->current_player)] ^= result;
}

// Evaluate the cell at position (x,y). If the cell at (x,y) is empty, its value
// is 0. If it is my stone then its value is v. If it is my opponents stone its
// value is -v.

// is this even necessary?
static inline int eval_cell(Game *g, int x, int y, int v)
{
  return is_set(g->board[g->current_player], x, y) ? v : is_set(g->board[!(g->current_player)], x, y) ? -v : 0;
}

// Evaluate the value of the stones on the board for the player
// whose current turn it is.
//  |A|B|C|D|E|F|G|H|
// 1|_|_|_|_|_|_|_|_|
// 2|_|_|_|_|_|_|_|_|
// 3|_|_|_|_|_|_|_|_|
// 4|_|_|_|O|X|_|_|_|
// 5|_|_|_|X|O|_|_|_|
// 6|_|_|_|_|_|_|_|_|
// 7|_|_|_|_|_|_|_|_|
// 8|_|_|_|_|_|_|_|_|
int eval_board(Game *g)
{
  int value = 0;

  int v = 1;
  // all cells have a value of 1
  for (int y = 0; y <= 7; y++)
  {
    for (int x = 0; x <= 7; x++)
    {
      value += eval_cell(g, x, y, v);
    }
  }

  // middle area has a higher weight: add 9 for each middle cell one
  v = 9;
  for (int y = 2; y <= 5; y++)
  {
    for (int x = 2; x < 5; x++)
    {
      value += eval_cell(g, x, y, v);
    }
  }

  return value;
}

typedef struct
{
  Position pos;
  int score;
} Move;

Move make_move(int x, int y, int score)
{
  Move m = {make_position(x, y), score};
  return m;
}

#define MOVE_STACK_SIZE 64
typedef struct
{
  int length;
  Move values[MOVE_STACK_SIZE];
} MoveStack;

MoveStack make_position_stack()
{
  MoveStack s;
  s.length = 0;
  return s;
}

void push(MoveStack *s, Move m)
{
  if (s->length >= MOVE_STACK_SIZE)
  {
    fprintf(stderr, "Stack overflow!\n");
    exit(0);
  }
  s->values[s->length++] = m;
}

Move pop(MoveStack *s)
{
  if (s->length <= 0)
  {
    fprintf(stderr, "Stack empty!\n");
    exit(0);
  }
  return s->values[--s->length];
}

Move random_move(MoveStack *s)
{
  if (s->length <= 0)
  {
    fprintf(stderr, "Stack empty!\n");
    exit(0);
  }
  // not available on Windows :-(   int i = arc4random_uniform(s->length);
  int i = i_rnd(s->length);
  return s->values[i];
}

// Count the number of cells of the given value.
int count_stones(Game *g, char c)
{
  short count = 0;
  uint_fast64_t current_player = g->board[which_stone(c)];
  while (current_player)
  {
    count += (current_player & 1);
    current_player >>= 1;
  }
  return count;
}

// Tests all positions and chooses a random one.
Position this_players_turn(Game *g)
{
  clock_t t = clock();          // get current timestamp
  Game *gLocal = copy_state(g); // copy game state
  MoveStack s = make_position_stack();
  for (int y = 0; y < N; y++)
  {
    for (int x = 0; x < N; x++)
    {
      if (legal(gLocal, x, y))
      {
        reverse(gLocal, x, y);
        int score = eval_board(gLocal);
        push(&s, make_move(x, y, score));

        free(gLocal);
        gLocal = copy_state(g); // restore state
      }
    }
  }
  if (s.length <= 0)
  {
    return make_position(-1, -1); // could not find valid move
  }
  // fprintf(stderr, "%d\n", s.length);
  int ibest = 0;
  for (int i = 1; i < s.length; i++)
  {
    if (s.values[i].score > s.values[ibest].score)
      ibest = i;
  }

  // bests
  MoveStack best_moves = make_position_stack();
  for (int i = 0; i < s.length; i++)
  {
    if (s.values[i].score >= s.values[ibest].score)
      push(&best_moves, s.values[i]);
  }

  t = clock() - t; // compute elapsed time
  double duration = t * 1000.0 / CLOCKS_PER_SEC;
  // fprintf(stderr, "duration: %g ms\n", duration);

  // fprintf(stderr, "best score = %d, size = %d\n", s.values[ibest].score, best_moves.length);
  return random_move(&best_moves).pos;
}

///////////////////////////////////////////////////////////////////////////////

void play(void)
{
  Game *g;
  while (true)
  {
    String s = s_input(100);
    int n = s_length(s);
    if (s_equals(s, "exit"))
    {
      exit(0);
    }
    else if (s_starts_with(s, "init: ") && n == 7)
    { // initialization tells use what stone we have
      int i = s_index(s, " ");
      char c = s[i + 1];
      if (c != 'X' && c != 'O')
      {
        fprintf(stderr, "Illegal stone: %c\n", c);
        exit(0);
      }
      g = init_game(which_stone(c));
      // print_board(g); // DEBUG
      // fprintf(stderr, "my stone is: %c\n", my_stone(g)); // DEBUG
    }
    else if (s_starts_with(s, "srand: ") && n >= 8)
    { // seed random number generator
      int i = s_index(s, " ");
      int seed = atoi(s + i + 1);
      // fprintf(stderr, "random seed: %d\n", seed); // DEBUG
      i = i_rnd(2);    // trigger seed
      srand(seed + i); // call seed again
    }
    else if (s_equals(s, "none"))
    { // opponent made no move
      // fprintf(stderr, "opponent made no move\n"); // DEBUG
      Position pos = this_players_turn(g);
      if (pos.x >= 0)
      {
        reverse(g, pos.x, pos.y);
        // print_board(&g);
        printf("%c%d\n", pos.x + 'a', pos.y + 1);
      }
      else
      {
        printf("none\n"); // no valid move found
      }
    }
    else if (n == 2)
    { // regular move of opponent
      Position pos;
      pos.x = (int)tolower(s[0]) - 'a';
      pos.y = (int)s[1] - '1';
      // fprintf(stderr, "opponent set: %c%d\n", pos.x + 'a', pos.y + 1); // DEBUG
      if (pos.x < 0 || pos.x >= N || pos.y < 0 || pos.y >= N)
      {
        fprintf(stderr, "Opponent move out of bounds: (%d, %d)\n", pos.x, pos.y);
        exit(0);
      }
      switch_stones(g);           // switch to opponent
      reverse(g, pos.x, pos.y);   // make opponent move
                                              // print_board(g); // DEBUG
      switch_stones(g);           // switch back to this player
      pos = this_players_turn(g); // compute our move
      if (pos.x >= 0)
      {
        reverse(g, pos.x, pos.y); // make our move
                                                  // print_board(g); // DEBUG
        printf("%c%d\n", pos.x + 'a', pos.y + 1);
      }
      else
      {
        printf("none\n"); // no valid move found
      }
    }
    else
    {
      fprintf(stderr, "Unknown command: %s\n", s);
      exit(0);
    }
          //  print_board(g); // DEBUG
    //        sleep(2); // seconds
    fflush(stdout); // need to push the data out of the door
  }
}

int main(void)
{
  play();
  return 0;
}
