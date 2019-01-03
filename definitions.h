#include <stdint.h>

//DEFINITIONS


//DEFINITIONS, STRUCTS, ENUMS OF THE AI

//BITMASKS
#define ONE (uint_fast64_t)1                         // Typecasting prevents ugly overflow stuff
#define BOARD_MASK (uint_fast64_t)0xFFFFFFFFFFFFFFFF // 16 * 4 bits
#define BOARD_WIDTH 8
#define BOARD_HEIGHT 8

#define occupied(state) ((state->board[0] | state->board[1]))
#define empty(state) ~(occupied(state))
#define shift_xy(x, y) (x + BOARD_WIDTH * y)
#define field_at(x, y) (ONE << shift_xy(x, y))
#define is_set(b, x, y) b &field_at(x, y)

//#################BITMASK#################
//BITMASK TO CONSTRUCT BOARD
typedef enum Edges
{
    BOTTOM = 0x00FFFFFFFFFFFFFF,
    ERIGHT = 0x7F7F7F7F7F7F7F7F,
    BOTTOM_RIGHT = 0x007F7F7F7F7F7F7F,
    BOTTOM_LEFT = 0x00FEFEFEFEFEFEFE,
    UPPER = 0xFFFFFFFFFFFFFF00,
    ELEFT = 0xFEFEFEFEFEFEFEFE,
    UPPER_RIGHT = 0x7F7F7F7F7F7F7F00,
    UPPER_LEFT = 0xFEFEFEFEFEFEFE00
} Edges;

//###########DIRECTIONS########
//AVAILABLE DIRECTIONS WHEN PUTTING STONES
typedef enum Delta
{
    DOWN = BOARD_WIDTH,
    DRIGHT = 1,
    DOWN_RIGHT = BOARD_WIDTH + 1,
    DOWN_LEFT = -(BOARD_WIDTH - 1),
    UP = -BOARD_WIDTH,
    DLEFT = -1,
    UP_RIGHT = BOARD_WIDTH - 1,
    UP_LEFT = -(BOARD_WIDTH + 1),
} DELTA;

//COORDINATES OF STONE
typedef struct
{
    int x;
    int y;
} Position;

Position make_position(int x, int y);


//@@@@@@@@@@@@@@@ REVERSI RANDOM BITBOARD BELOW @@@@@@@@@@@@@@@@//

#define N 8 //breadth + height of board

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
