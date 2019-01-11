#include "definitions.h"

#ifndef BOARD_H
#define BOARD_H

Game *init_game(Players current_player);
uint_fast64_t possible_moves(const Game *g);
bool legal(const Game *g, uint_fast64_t move);
void true_reverse(Game *g, uint_fast64_t move);
void reverse(Game *g, int x, int y);

void switch_stones(Game *g);


#endif // BOARD_H
