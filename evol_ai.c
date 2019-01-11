#include "evol_ai.h"
// #include "definitions.h"

static inline int heuristic(uint_fast64_t pos)
{
  return CORNERS & pos ? 10 : C_SPOTS & pos ? 1 : X_SPOTS & pos ? 3 : ONE_SPOTS & pos ? 1 : TWO_SPOTS & pos ? 2 : FOUR_SPOTS & pos ? 4 : SIX_SPOTS & pos ? 6 : 0;
}

uint_fast64_t some_move(uint_fast64_t possible)
{
  int count = rand() % (64 - clzll(possible));
  count += ctzll(possible >> count);
  return ONE << count & possible;
}

uint_fast64_t most_promising_move(uint_fast64_t possible)
{
  int count = 0; // where y = count / height +1 and x = count % width + 1;
  uint_fast64_t best_move = 0;
  int best_score = 0;

  while (possible)
  {
    count += ctzll(possible);
    possible >>= ctzll(possible) + 1;

    Bitboard current_move = ONE << count;
    int current_score = heuristic(current_move);

    if (current_score == best_score)
      best_move |= current_move;
    if (current_score > best_score)
    {
      best_move = current_move;
      best_score = current_score;
    }

    count++;
  }

  return best_move;
}

// Tests all positions and chooses a good looking one.
Position get_turn(Game *g)
{
  uint_fast64_t some_move = most_promising_move(g->legal_moves);

  Position some_pos = {-1, -1};
  if (g->legal_moves)
    some_pos = make_position(ctzll(some_move) % BOARD_WIDTH, ctzll(some_move) / BOARD_HEIGHT);

  return some_pos;
}
