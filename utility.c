#include "utility.h"

// // This makes "curling" (or everything involving coordinates) more beautiful
// // Bitshifting with negative shift values is scary because undefined, so we need two cases.
// uint_fast64_t bitshift(uint_fast64_t left_value, short shift_count)
// {
//   if (shift_count < 0)
//     return left_value >> -shift_count;
//   else
//     return left_value <<= shift_count;
// }

// Check whether position (x,y) is on the board.
bool out_of_bounds(int x, int y)
{
  return x < 0 || x > BOARD_WIDTH - 1 || y < 0 || y > BOARD_WIDTH - 1;
}

bool which_stone(char c)
{
  return c == 'O';
}

// If it is X's turn, then "my stone" is 'X', otherwise it is 'O'.
char my_stone(Game *g)
{
  return g->current_player ? 'O' : 'X';
}
