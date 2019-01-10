#include "definitions.h"

#if !defined(BOARD_H)
#define BOARD_H

// typedef enum Players
// {
//   BLACK = 0, // X
//   WHITE = 1  // O
// } Players;

// typedef struct Game
// {
//   // This way, we can easily set black or white stones. Seeing whether a stone is set requires a macro.
//   uint_fast64_t board[2]; // Bitboards representing "X"/"black" and "O"/"white"
//   uint_fast64_t legal_moves;
//   Players current_player; // 'X' is false and 'O' is true
// } Game;


Game *init_game(Players current_player);
uint_fast64_t possible_moves(const Game *g);
static inline bool legal(const Game *g, uint_fast64_t move);
void reverse(Game *g, uint_fast64_t move);

static inline bool which_stone(char c);
static inline char my_stone(Game *g);
static inline void switch_stones(Game *g);


#endif // BOARD_H
