#include "base.h"
#include "definitions.h"

Position make_position(int x, int y) {
    Position p = { x, y };
    return p;
}

void print_position(Position p) {
    fprintf(stderr, "%c%d\n", p.x + 'a', p.y + 1);
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
uint_fast64_t possible_moves(Game *g)
{
  uint_fast64_t moves = 0;

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
  g->board[BLACK] = 0x810000000; // Replace with actual magic bit pattern 0x810000000
  g->board[WHITE] = 0x1008000000; // For maximum beauty 0x1008000000
  g->legal_moves = possible_moves(g);            

  return g;
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
bool out_of_bounds(int x, int y) {
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

static inline void switch_stones(Game *g)
{
  g->current_player = !g->current_player;
  g->legal_moves = possible_moves(g);
}

// Check whether (x,y) is a legal position to place a stone. A position is legal
// if it is empty ('_'), is on the board, and has at least one legal direction.

// static inline bool legal(Game *g, int x, int y)
// {
//   return is_set(g->legal_moves, x, y);
// }
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
  g->legal_moves = possible_moves(g);
}

// is this even necessary?
// static inline int eval_cell(Game *g, int x, int y, int v)
// {
//   return is_set(g->board[g->current_player], x, y) ? v : is_set(g->board[!(g->current_player)], x, y) ? -v : 0;
// }

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
int eval_board(Game* g) {
    int value = 0;

    return value;
}

typedef struct {
    Position pos;
    int score;
} Move;

Move make_move(int x, int y, int score) {
    Move m = { make_position(x, y), score };
    return m;
}

// Count the number of cells of the given value.
int count_cells(Game *g, char c)
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

static inline int heuristic(uint_fast64_t pos)
{
  return CORNERS & pos ? 10 :
   C_SPOTS & pos ? 1 :
    X_SPOTS & pos ? 3 :
     ONE_SPOTS & pos ? 1 :
      TWO_SPOTS & pos ? 2 :
       FOUR_SPOTS & pos ? 4 :
        SIX_SPOTS & pos ? 6 : 0;
}

uint_fast64_t some_move(uint_fast64_t possible) {
  int count = rand() % (64 - __builtin_clzll(possible));
  count += ctzll(possible>>count);
  return ONE << count & possible;
}

uint_fast64_t most_promising_move(uint_fast64_t possible)
{
  int count = 0; //where y = count / height +1 and x = count % width + 1;
  uint_fast64_t best_move = 0;
  int best_score = 0;

  while (possible)
  {
    count += ctzll(possible);
    possible >>= ctzll(possible) + 1;

    int current_score = heuristic(ONE << count);
    if (current_score == best_score)
      best_move |= ONE << count;
    if (current_score > best_score)
    {
      best_move = ONE << count;
      best_score = current_score;
    }
    
    count++;
  }


  return some_move(best_move);
}

// Tests all positions and chooses a random one.
Position this_players_turn(Game *g)
{
  // clock_t t = clock();          // get current timestamp
  

  uint_fast64_t some_move = most_promising_move(g->legal_moves);

  // t = clock() - t; // compute elapsed time
  // double duration = t * 1000.0 / CLOCKS_PER_SEC;
  // fprintf(stderr, "duration: %g ms\n", duration);

  // fprintf(stderr, "best score = %d, size = %d\n", s.values[ibest].score, best_moves.length);

  Position some_pos = {-1,-1}; 
  if(g->legal_moves)
    some_pos = make_position(ctzll(some_move) % BOARD_WIDTH, ctzll(some_move) / BOARD_HEIGHT);
  
  return some_pos;
}
///////////////////////////////////////////////////////////////////////////////

void play(void)
{
  srand(time(NULL));
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
      fprintf(stderr, "my stone is: %c\n", my_stone(g)); // DEBUG
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
        // print_board(g);
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
    fprintf(stderr, "Possible moves:0x%"PRIxFAST64"\n", g->legal_moves);
    
  }
}


int main(void) {
    play();
    return EXIT_SUCCESS;
}
