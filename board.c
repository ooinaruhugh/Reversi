#include "board.h"

Game *init_game(Players current_player)
{
  Game *g = malloc(sizeof(*g));
  g->current_player = current_player;
  g->board[BLACK] = 0x810000000;
  g->board[WHITE] = 0x1008000000;
  g->legal_moves = possible_moves(g);

  return g;
}

// This makes "curling" (or everything involving coordinates) more beautiful
// Bitshifting with negative shift values is scary because undefined, so we need two cases.
uint_fast64_t bitshift(uint_fast64_t left_value, short shift_count)
{
  if (shift_count < 0)
    return left_value >> -shift_count;
  else
    return left_value << shift_count;
}

// Detecting groups of enemy stones with an adjacent player stone.
uint_fast64_t curling(const Game *g, uint_fast64_t edge, short direction)
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
uint_fast64_t possible_moves(const Game *g)
{
  uint_fast64_t moves = 0;

  // curling() only works for one direction
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

bool legal(const Game *g, uint_fast64_t move)
{
  return g->legal_moves & move;
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

void reverse(Game *g, int x, int y)
{
  true_reverse(g, field_at(x, y));
}

void switch_stones(Game *g)
{
  g->current_player = !g->current_player;
  g->legal_moves = possible_moves(g);
}