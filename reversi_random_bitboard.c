/*
Compile: make reversi
Run: ./reversi
make reversi && ./reversi

Reversi Rules
- X moves first
- Pieces are reversed if surrounded (on two sides) by opponent pieces
- A valid move is one in which at least one piece is reversed
- If one player cannot make a move, play passes back to the other player
- When neither player can move, the game ends, the player with the most pieces wins

Details: https://en.wikipedia.org/wiki/Reversi
*/

#include "base.h"
#include <stdint.h>

#define ONE (uint_fast64_t)1          // Typecasting prevents ugly overflow stuff
#define BOARD_MASK 0xFFFFFFFFFFFFFFFF // 16 * 4 bits
#define BOARD_WIDTH 8
#define BOARD_HEIGHT 8
// #define BLACK 0
// #define WHITE 1

#define occupied(state) ((state->board[0] | state->board[1]))
#define empty(state) ~(occupied(state))
#define shift_xy(x, y) (x + BOARD_WIDTH * y)
#define field_at(x, y) (ONE << shift_xy(x, y))
#define is_set(b, x, y) b &field_at(x, y)

// todo: something with enums?
// const uint_fast64_t edge[] = {
//     0x00FFFFFFFFFFFFFF, // a: bottom edge
//     0x7F7F7F7F7F7F7F7F, // b: right edge
//     0x007F7F7F7F7F7F7F, // c: bottom and right edge (redundant)
//     0x00FEFEFEFEFEFEFE, // d: bottom and left edge (reduntdant)
//     0xFFFFFFFFFFFFFF00, // e: upper edge
//     0xFEFEFEFEFEFEFEFE, // f: left edge
//     0x7F7F7F7F7F7F7F00, // g: upper and right edge (redundant)
//     0xFEFEFEFEFEFEFE00  // h: upper and left edge (redundant)
// };

typedef enum Edges
{
  BOTTOM = 0x00FFFFFFFFFFFFFF,
  ERIGHT = 0x7F7F7F7F7F7F7F7F,       // b: right edge
  BOTTOM_RIGHT = 0x007F7F7F7F7F7F7F, // c: bottom and right edge (redundant)
  BOTTOM_LEFT = 0x00FEFEFEFEFEFEFE,  // d: bottom and left edge (reduntdant)
  UPPER = 0xFFFFFFFFFFFFFF00,        // e: upper edge
  ELEFT = 0xFEFEFEFEFEFEFEFE,        // f: left edge
  UPPER_RIGHT = 0x7F7F7F7F7F7F7F00,  // g: upper and right edge (redundant)
  UPPER_LEFT = 0xFEFEFEFEFEFEFE00    // h: upper and left edge (redundant)
} Edges;

/* // Bitshift to the left means moving to the right or down. Yes, it does.
// todo: seriously consider enums
const short delta[] = {
    BOARD_WIDTH,        // a: vertical downwards
    1,                  // b: horizontal to the right
    BOARD_WIDTH + 1,    // c: top-left to bottom-right
    -(BOARD_WIDTH - 1), // d: top-right to bottom-left
    -BOARD_WIDTH,       // e: vertical upwards
    -1,                 // f: horizontal to the left
    BOARD_WIDTH - 1,    // g: bottom-left to top-right
    -(BOARD_WIDTH + 1), // h: bottom-right to top-left
}; */

typedef enum Delta
{
  DOWN = BOARD_WIDTH,             // a: vertical downwards
  DRIGHT = 1,                     // b: horizontal to the right
  DOWN_RIGHT = BOARD_WIDTH + 1,   // c: top-left to bottom-right
  DOWN_LEFT = -(BOARD_WIDTH - 1), // d: top-right to bottom-left
  UP = -BOARD_WIDTH,              // e: vertical upwards
  DLEFT = -1,                     // f: horizontal to the left
  UP_RIGHT = BOARD_WIDTH - 1,     // g: bottom-left to top-right
  UP_LEFT = -(BOARD_WIDTH + 1),   // h: bottom-right to top-left
} DELTA;

typedef struct
{
  int x;
  int y;
} Position;

static inline Position make_position(int x, int y)
{
  Position p = {x, y};
  return p;
}

static inline void print_position(Position p)
{
  printf("%c%d\n", p.x + 'A', p.y + 1);
}

#define N 8

typedef enum Players
{
  BLACK = 0, // X
  WHITE = 1  // O
} Players;

typedef struct Game
{
  // todo: members for taken turn and value of said turn
  // This way, we can easily set black or white stones. Seeing whether a stone is set requires a macro.
  uint_fast64_t board[2]; // Bitboards representing "X"/"black" and "O"/"white"
  uint_fast64_t legal_moves;
  Players current_player; // 'X' is false and 'O' is true
} Game;
/* 
typedef struct Move
{
  Game state;             // How the board looks like
  Position pos;           // The move to reach state
  int score;              // Some score that state yields
  short next_moves_count; // Length of that array
  Move *next_moves[];     // An array of all possible moves
} Move; */

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
Game *init_game(Players current_player)
{
  // todo: implement
  Game *g = malloc(sizeof(*g));
  g->current_player = current_player;
  g->board[BLACK] = field_at(4, 3) | field_at(3, 4); // Replace with actual magic bit pattern 0x810000000
  g->board[WHITE] = field_at(3, 3) | field_at(4, 4); // For maximum beauty 0x1008000000
  g->legal_moves = 0x102004080000;                   // The first turn is ALWAYS the same.

  return g;
}

static inline Game *copy_state(Game *g)
{
  return memcpy(malloc(sizeof(*g)), g, sizeof(*g));
}
// todo: destructor?

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
  uint_fast64_t non_edge_set = g->board[!(g->current_player)] & edge;
  uint_fast64_t possible = non_edge_set & (bitshift(g->board[g->current_player], direction));

  for (int i = 0; i < 6; i++)
  {
    possible |= non_edge_set & bitshift(possible, direction);
  }

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
  if (row == -1)
  {
    for (int i = 0; i < 9; i++)
    {
      printf("%c|", i < 1 ? 32 : 64 + i);
    }
  }
  // Printing a "normal" row
  else
  {
    printf("%i|", row + 1);
    for (int i = 0; i < 8; i++)
    {
      printf("%c|", g->board[BLACK] & field_at(i, row) ? 'X' : g->board[WHITE] & field_at(i, row) ? 'O' : '_');
    }
  }
  printf("\n");
}

// Print the board. The initial board should look like shown above.
void print_board(Game *g)
{
  for (int i = -1; i < 8; i++)
  {
    print_row(g, i);
  }
}
// Check whether position (x,y) is on the board.
bool out_of_bounds(int x, int y)
{
  return ((x > 7 || x < 0)) || ((y > 7) || (y < 0));
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

// Switch the my stones and your stones ('X' <--> 'O')
static inline void switch_stones(Game *g)
{
  g->current_player = !g->current_player;
  g->legal_moves = possible_moves(g);
}

// Check whether, if starting at (x,y), direction (dx,dy) is a legal direction
// to reverse stones. A direction is legal if (assuming my stone is 'X')
// the pattern in that direction is: O+X.* (one or more 'O's, followed by an 'X',
// followed by zero or more arbitrary characters).
// (dx,dy) is element of { (a,b) | a, b in {-1, 0, 1} and (a,b) != (0,0) }
bool legal_dir(Game *g, int x, int y, int dx, int dy)
{
  /*  x += dx;
  y += dy;
  while (!out_of_bounds(x, y) && (g->board[x][y] == your_stone(g)))
  {
    x += dx;
    y += dy;
    if (!out_of_bounds(x, y) && (g->board[x][y] == my_stone(g)))
    {
      return true;
    }
  } */
  return false;
}

// Check whether (x,y) is a legal position to place a stone. A position is legal
// if it is empty ('_'), is on the board, and has at least one legal direction.
static inline bool legal(Game *g, int x, int y)
{
  return is_set(g->legal_moves, x, y);
}

// Reverse stones starting at (x,y) in direction (dx,dy), but only if the
// direction is legal. May modify the state of the game.
// (dx,dy) is element of { (a,b) | a, b in {-1, 0, 1} and (a,b) != (0,0) }
uint_fast64_t reverse_dir(Game *g, int x, int y, uint_fast64_t edge, short direction)
{
  uint_fast64_t non_edge_set = g->board[!(g->current_player)] & edge;
  uint_fast64_t result = non_edge_set & (bitshift(field_at(x, y), direction));

  for (int i = 0; i < 6; i++)
  {
    result |= non_edge_set & bitshift(result, direction);
  }

  return (bitshift(result, direction) & g->board[g->current_player]) ? result : 0;
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

  result |= reverse_dir(g, x, y, BOTTOM, -DOWN);
  result |= reverse_dir(g, x, y, ERIGHT, -DRIGHT);
  result |= reverse_dir(g, x, y, BOTTOM_RIGHT, -DOWN_RIGHT);
  result |= reverse_dir(g, x, y, BOTTOM_LEFT, -DOWN_LEFT);
  result |= reverse_dir(g, x, y, UPPER, -UP);
  result |= reverse_dir(g, x, y, ELEFT, -DLEFT);
  result |= reverse_dir(g, x, y, UPPER_RIGHT, -UP_RIGHT);
  result |= reverse_dir(g, x, y, UPPER_LEFT, -UP_LEFT);

  g->board[g->current_player] |= result;
  g->board[!(g->current_player)] ^= result;
}

// Due to recursion we need a function prototype for human_move
Position human_move(Game *g);

Position mark_possible_moves(Game *g)
{

  uint_fast64_t possible = possible_moves(g);

  printf(" |A|B|C|D|E|F|G|H|\n");
  for (int i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++)
  {
    if (!(i % 8))
      printf("%i|", 1 + i / BOARD_WIDTH);
    if ((occupied(g) | possible) & ONE << i)
    {
      printf("%c|", g->board[BLACK] & (ONE << i) ? 'X' : g->board[WHITE] & (ONE << i) ? 'O' : possible & (ONE << i) ? '*' : '_');
    }
    else
      printf("_|");
    if (!((i + 1) % BOARD_WIDTH))
      printf("\n");
  }

  printf("%c's move: ", my_stone(g));
  return human_move(g);
}

// Input a position of the form D6 or d6, i.e., giving the column first and
// then the row. A1 corresponds to position (0,0). B1 corresponds to (1,0).
Position human_move(Game *g)
{
  String s = s_input(10);
  if (s_length(s) >= 1 && s[0] == 'q')
    exit(0);
  if (s_length(s) >= 1 && s[0] == '?')
    return mark_possible_moves(g);
  // todo: modify to temporarily mark valid moves
  Position pos = {-1, -1};
  if (s_length(s) >= 2)
  {
    pos.x = (int)tolower(s[0]) - 'a';
    pos.y = (int)s[1] - '1';
  }
  if (legal(g, pos.x, pos.y))
  {
    return pos;
  }
  else
  {
    printsln("Invalid position!");
    return human_move(g);
  }
}

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

#define POSITION_STACK_SIZE 64
typedef struct
{
  int length;
  Position values[POSITION_STACK_SIZE];
} PositionStack;

// Initializes a position stack.
PositionStack make_position_stack()
{
  PositionStack ps;
  ps.length = 0;
  return ps;
}

// Pushes a new position on top of the stack.
void push(PositionStack *ps, Position p)
{
  if (ps->length < POSITION_STACK_SIZE)
  {
    ps->length++;
    ps->values[ps->length] = p;
  }
  else
  {
    exit(EXIT_FAILURE);
  }
}

// Pops the topmost position from the stack.
Position pop(PositionStack *ps)
{
  // todo: implement
  if (ps->length > 0)
  {
    Position p = ps->values[ps->length];
    ps->length--;
    return p;
  }
  else
  {
    exit(EXIT_FAILURE);
  }
}

// Returns a random position from the stack.
Position random_position(PositionStack *ps)
{
  int rand_index = rand() % ps->length + 1;
  return ps->values[rand_index];
}

// Tests all positions and chooses a random valid move.
Position computer_move(Game *g)
{
  PositionStack ps = make_position_stack();

  for (int row = 0; row < 8; row++)
  {
    for (int column = 0; column < 8; column++)
    {
      if (legal(g, column, row))
        push(&ps, make_position(column, row));
    }
  }

  return ps.length ? random_position(&ps) : make_position(-1, -1);
}

int main(void)
{
  srand(time(NULL));

  Game *g = init_game(BLACK);
  print_board(g);
  while (true)
  {
    printf("%c's move: ", my_stone(g));
    // Position pos = !(g->current_player) ? human_move(g) : computer_move(g);
    Position pos = human_move(g);

    if (my_stone(g) == 'O')
      print_position(pos);

    reverse(g, pos.x, pos.y);
    print_board(g);
    int score = count_stones(g, my_stone(g)) - count_stones(g, your_stone(g));
    printf("Score for %c: %d\n", my_stone(g), score);
    switch_stones(g);

    printf("%c's move: ", my_stone(g));
    pos = computer_move(g);
    print_position(pos);
    reverse(g, pos.x, pos.y);
    print_board(g);
    score = count_stones(g, my_stone(g)) - count_stones(g, your_stone(g));
    printf("Score for %c: %d\n", my_stone(g), score);
    switch_stones(g);
  }
  return 0;
}
