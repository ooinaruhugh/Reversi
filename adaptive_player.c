// #include "base.h"
#include "definitions.h"

// todo: Remove struct Position as it is not necessary
static inline Position make_position(int x, int y)
{
  Position p = {x, y};
  return p;
}

void print_position(Position p)
{
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
  Game *g = malloc(sizeof(*g));
  g->current_player = current_player;
  g->board[BLACK] = 0x810000000;  // Replace with actual magic bit pattern 0x810000000
  g->board[WHITE] = 0x1008000000; // For maximum beauty 0x1008000000
  g->legal_moves = possible_moves(g);

  return g;
}

void print_row(Game *g, int row)
{
  // Printing the head row
  if (row == 0)
    fprintf(stderr, " |A|B|C|D|E|F|G|H|\n");
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

uint_fast64_t reverse_dir(Game *g, uint_fast64_t move, uint_fast64_t edge, short direction)
{
  uint_fast64_t non_edge_set = g->board[!(g->current_player)] & edge;

  // This time, while we are sliding player's stone, we are effectively converting
  // every enemy stone that is encountered
  uint_fast64_t result = non_edge_set & (bitshift(move, direction));

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
void true_reverse(Game *g, uint_fast64_t move)
{
  g->board[g->current_player] |= move;

  uint_fast64_t result = 0;

  // We are gathering the changes as results of possible turns.
  result |= reverse_dir(g, move, BOTTOM, -DOWN);
  result |= reverse_dir(g, move, ERIGHT, -DRIGHT);
  result |= reverse_dir(g, move, BOTTOM_RIGHT, -DOWN_RIGHT);
  result |= reverse_dir(g, move, BOTTOM_LEFT, -DOWN_LEFT);
  result |= reverse_dir(g, move, UPPER, -UP);
  result |= reverse_dir(g, move, ELEFT, -DLEFT);
  result |= reverse_dir(g, move, UPPER_RIGHT, -UP_RIGHT);
  result |= reverse_dir(g, move, UPPER_LEFT, -UP_LEFT);

  // And then we commit them to the bitboards.
  g->board[g->current_player] |= result;
  g->board[!(g->current_player)] ^= result;
  g->legal_moves = possible_moves(g);
}

// todo: First remove struct Position, then this can go as well.
void reverse(Game *g, int x, int y)
{
  true_reverse(g, field_at(x, y));
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

// Count the number of cells of the given value.
int count_cells(Game *g, Players player)
{
  return popcount(g->board[player]);
}

int eval_board(Game *g, Players us)
{
  int value = count_cells(g, us) - count_cells(g, !us);

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

#define SCORE_BINS 15
const int score_bins = SCORE_BINS;
#define EARLY_GAME_DURATION 12
unsigned int early_game_duration = EARLY_GAME_DURATION;

uint_fast64_t *current_measure;

const uint_fast64_t early_game[SCORE_BINS] = {
    [SCORE_BINS - 1] = CORNERS,
    [9] = B_TIER | G_TIER,
    [7] = A_TIER,

    [4] = F_TIER,
    [1] = D_TIER,
    [2] = E_TIER,
    [0] = C_SPOTS | X_SPOTS,
};

const uint_fast64_t mid_game[SCORE_BINS] = {
    [SCORE_BINS - 1] = CORNERS,
    [9] = G_TIER,
    [7] = B_TIER | F_TIER,
    [4] = A_TIER,
    [1] = D_TIER,
    [2] = E_TIER,
    [0] = C_SPOTS | X_SPOTS,
};

const uint_fast64_t corner_taken[SCORE_BINS] = {
    // [0] = CORNERS,
    [SCORE_BINS - 2] = G_TIER,
    [SCORE_BINS - 4] = F_TIER,
    [5] = B_TIER,
    [4] = A_TIER,
    [1] = D_TIER,
    [2] = E_TIER,
    [SCORE_BINS - 3] = X_SPOTS,
    [SCORE_BINS - 1] = C_SPOTS | CORNERS,
};

const uint_fast64_t corner_has_fallen[SCORE_BINS] = {
    [0] = CORNERS | C_SPOTS | G_TIER | F_TIER | X_SPOTS,
    [7] = B_TIER,
    [6] = A_TIER,
    [2] = D_TIER | E_TIER,
};

//if taken corner by enemy, C TO F evil, other corner same
//if taken corner by us, C TO F good, X slightly better, other corner stays the same
//border gets high if taken 2 corners
//mid becomes crucial in case corner is taken

#define FIRST_QUARTER 0xF0F0F0F
#define SECOND_QUARTER 0xF0F0F0F0
#define THIRD_QUARTER 0xF0F0F0F00000000
#define FOURTH_QUARTER 0xF0F0F0F000000000

void update_heuristic(uint_fast64_t move, bool is_enemy)
{
  early_game_duration--;
  if (early_game_duration <= 0)
  {
    memcpy(current_measure, early_game, score_bins * sizeof(*early_game));
    fprintf(stderr, "Updating heuristic.\n");
  }

  if (move & CORNERS)
  {
    fprintf(stderr, "Updating heuristic.\n");
    if (is_enemy)
    {
      if (move & field_at(8, 8))
      {
        for (int i = 0; i < score_bins; i++)
        {
          current_measure[i] &= ~FOURTH_QUARTER;
          current_measure[i] |= (corner_has_fallen[i] & FOURTH_QUARTER);
        }
      }
      if (move & field_at(1, 8))
      {
        for (int i = 0; i < score_bins; i++)
        {
          current_measure[i] &= ~THIRD_QUARTER;
          current_measure[i] |= (corner_has_fallen[i] & THIRD_QUARTER);
        }
      }
      if (move & field_at(8, 1))
      {
        for (int i = 0; i < score_bins; i++)
        {
          current_measure[i] &= ~SECOND_QUARTER;
          current_measure[i] |= (corner_has_fallen[i] & SECOND_QUARTER);
        }
      }
      else
      {
        for (int i = 0; i < score_bins; i++)
        {
          current_measure[i] &= ~FIRST_QUARTER;
          current_measure[i] |= (corner_has_fallen[i] & FIRST_QUARTER);
        }
      }
    }
    else
    {
      if (move & field_at(8, 8))
      {
        for (int i = 0; i < score_bins; i++)
        {
          current_measure[i] &= ~FOURTH_QUARTER;
          current_measure[i] |= (corner_taken[i] & FOURTH_QUARTER);
        }
      }
      if (move & field_at(1, 8))
      {
        for (int i = 0; i < score_bins; i++)
        {
          current_measure[i] &= ~THIRD_QUARTER;
          current_measure[i] |= (corner_taken[i] & THIRD_QUARTER);
        }
      }
      if (move & field_at(8, 1))
      {
        for (int i = 0; i < score_bins; i++)
        {
          current_measure[i] &= ~SECOND_QUARTER;
          current_measure[i] |= (corner_taken[i] & SECOND_QUARTER);
        }
      }
      else
      {
        for (int i = 0; i < score_bins; i++)
        {
          current_measure[i] &= ~FIRST_QUARTER;
          current_measure[i] |= (corner_taken[i] & FIRST_QUARTER);
        }
      }
    }
  }
}

static inline int heuristic(uint_fast64_t pos)
{
  for (int i = 0; i < score_bins; i++)
  {
    if (current_measure[i] & pos)
    {
      fprintf(stderr, "Returning from the new heuristic.\n");
      return i + 1;
    }
  }

  fprintf(stderr, "Returning from the old heuristic.\n");
  return CORNERS & pos ? 10 : C_SPOTS & pos ? 1 : X_SPOTS & pos ? 1 : A_TIER & pos ? 1 : B_TIER & pos ? 4 : D_TIER & pos ? 7 : E_TIER & pos ? 5 : F_TIER & pos ? 4 : G_TIER & pos ? 6 : 0;
}

uint_fast64_t some_move(uint_fast64_t possible)
{
  int count = rand() % (64 - clzll(possible));
  count += ctzll(possible >> count);
  return ONE << count & possible;
}

static inline void execute_move(Game *g, uint_fast64_t move)
{
  true_reverse(g, move);
  switch_stones(g);
}

uint_fast64_t most_promising_move(Game *g, uint_fast64_t possible)
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
  uint_fast64_t some_move = most_promising_move(g, g->legal_moves);

  Position some_pos = {-1, -1};
  if (g->legal_moves)
    some_pos = make_position(ctzll(some_move) % BOARD_WIDTH, ctzll(some_move) / BOARD_HEIGHT);

  return some_pos;
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
  // return human_move(g);
}

Position opponent_move(Game *g, uint64_t move_string, char *input_buffer)
{
  Position pos;
  pos.x = toupper(move_string & 0xFF) - 'A';
  pos.y = 0xFF & (move_string >> __CHAR_BIT__) - '1';

  if (legal(g, pos.x, pos.y))
  {
    return pos;
  }

  fprintf(stderr, "Invalid position!");
  // return opponent_move(g);
}
///////////////////////////////////////////////////////////////////////////////

void play(void)
{
  srand(time(NULL));
  Game *g = NULL;
  Players us;

  while (true)
  {
    char *input_buffer = calloc(1, 100);
    uint64_t *gyoutou = input_buffer; // Alternatively BOL (jap. gyoutou)

    fgets(input_buffer, 99, stdin);

    if (!strcmp(input_buffer, "exit\n"))
    {
      free(input_buffer);
      if (g)
        free(g);
      exit(EXIT_SUCCESS);
    }

    // todo: In the future, this should be handled with command line args

    else if ((*gyoutou & 0xFFFFFFFFFFFF) == INIT_DPS_MAGIC)
    {
      if (g)
        free(g);

      // We only want the first seven bytes
      // or rather not that line break from fgets
      *gyoutou &= 0xFFFFFFFFFFFFFF;
      uint64_t my_stone = *gyoutou - INIT_DPS_MAGIC;

      // We only want to shift my_stone by whole bytes.
      // That's why we divide by 8 and multiply again afterwards.
      char c = 0xFF & (my_stone >> (ctzll(my_stone) / 8 * 8));
      // Theoretically, we could hard code that as:
      // char c = 0xFF & (my_stone >> __CHAR_BIT__ * 6);

      // "a == b" == "!(a ^ b)"
      if (*gyoutou ^ INIT_DPS_X_MAGIC && *gyoutou ^ INIT_DPS_O_MAGIC)
      {
        free(input_buffer);
        if (g)
          free(g);
        exit(0);
      }

      us = which_stone(c);
      g = init_game(us);
      memcpy(current_measure, early_game, score_bins * sizeof(*early_game));
      early_game_duration = EARLY_GAME_DURATION;
    }

    // todo: like really, which sensible person wouldn't do this with args
    else if ((*gyoutou & 0xFFFFFFFFFFFFFF) == SRAND_DPS_MAGIC)
    {
      // We blank out the "srand: " and replace it with whitespace.
      *gyoutou -= SRAND_DPS_MAGIC - 0x20202020202020;

      // Because atoi can handle that
      unsigned int seed = atoi(input_buffer);
      srand(seed);
    }

    else if ((*gyoutou & 0xFFFFFFFF) == NONE_MAGIC)
    {
      Position pos = this_players_turn(g);
      if (pos.x >= 0)
      {
        reverse(g, pos.x, pos.y);
        update_heuristic(field_at(pos.x, pos.y), g->current_player ^ us);
        // print_board(g);
        printf("%c%d\n", pos.x + 'a', pos.y + 1);
      }
      else
      {
        printf("none\n"); // no valid move found
      }
    }

    // Only the first 16 bits and the following two bits for line feed should be set
    // todo: Maybe needs to consider Windows and Unix stuff (CR LF)
    else if (!(0xFFFFFFFFFFF50000 & *gyoutou))
    {
      *gyoutou &= 0xFFFF;

      // regular move of opponent
      Position pos;
      pos.x = toupper(*gyoutou & 0xFF) - 'A';
      pos.y = 0xFF & (*gyoutou >> __CHAR_BIT__) - '1';

      if (pos.x < 0 || pos.x >= N || pos.y < 0 || pos.y >= N)
      {
        free(input_buffer);
        if (g)
          free(g);

        exit(0);
      }

      switch_stones(g);         // switch to opponent
      reverse(g, pos.x, pos.y); // make opponent move
      update_heuristic(field_at(pos.x, pos.y), g->current_player ^ us);
      // print_board(g); // DEBUG
      switch_stones(g);           // switch back to this player
      pos = this_players_turn(g); // compute our move
      if (pos.x >= 0)
      {
        reverse(g, pos.x, pos.y); // make our move
        update_heuristic(field_at(pos.x, pos.y), g->current_player ^ us);

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
      fprintf(stderr, "Unknown command: %s\n", input_buffer);
      free(input_buffer);
      if (g)
        free(g);
      exit(0);
    }
    fflush(stdout); // need to push the data out of the door

    free(input_buffer);
  }
}

int main(void)
{
  current_measure = calloc(score_bins, sizeof(*current_measure));
  play();
  return EXIT_SUCCESS;
}
