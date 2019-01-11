#include "definitions.h"


#ifndef UTILITY_H
#define UTILITY_H

// uint_fast64_t bitshift(uint_fast64_t left_value, short shift_count);
bool out_of_bounds(int x, int y);

bool which_stone(char c);
char my_stone(Game *g);


#endif // UTILITY_H
