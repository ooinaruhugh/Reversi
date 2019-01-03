#include "base.h"
#include "definitions.h"

#define occupied(state) ((state->board[0] | state->board[1]))
#define empty(state) ~(occupied(state))
#define shift_xy(x, y) (x + BOARD_WIDTH * y)
#define field_at(x, y) (ONE << shift_xy(x, y))
#define is_set(b, x, y) b &field_at(x, y)

#define CORNERS 0x8100000000000081
#define C_SPOTS 0x4281000000008142
#define X_SPOTS 0x42000000004200
#define ONE_SPOTS 0x182424180000
#define TWO_SPOTS 0x240000240000
#define FOUR_SPOTS 0x3C424242423C00
#define SIX_SPOTS 0x3C0081818181003C

#define POSITION_STACK_SIZE 64
typedef struct {
    int length;
    Position values[POSITION_STACK_SIZE];
} PositionStack;

PositionStack make_position_stack() {
    PositionStack ps;
    ps.length = 0;
    return ps;
}

void push(PositionStack *ps, Position p) {
    if (ps->length >= POSITION_STACK_SIZE) {
        fprintf(stderr, "Stack overflow!\n");
        exit(0);
    }
    ps->values[ps->length++] = p;
}

Position pop(PositionStack *ps) { 
    if (ps->length <= 0) {
        fprintf(stderr, "Stack empty!\n");
        exit(0);
    }
    return ps->values[--ps->length];
}

Position random_position(PositionStack *ps) {
    if (ps->length <= 0) {
        fprintf(stderr, "Stack empty!\n");
        exit(0);
    }
    // not available on Windows :-(   int i = arc4random_uniform(ps->length);
    int i =rand() %ps->length;
    return ps->values[i];
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

uint_fast64_t random_move(uint_fast64_t possible) {
  int count = rand() % (64 - __builtin_clzll(possible));
  printf("%llx >> %i = %llx\n", possible, count, possible >> count);
  count += __builtin_ctzll(possible>>count);
  return ONE << count & possible;
}

uint_fast64_t most_promising_move(uint_fast64_t possible)
{
  int count = 1; //where y = count / height +1 and x = count % width + 1;
  uint_fast64_t best_move = 0;
  int best_score = 0;

  while (possible)
  {
    printf("%llx\n", possible);
    count += __builtin_ctzll(possible);
    possible >>= __builtin_ctzll(possible)+1;

    // if (possible & 1)
    int current_score = heuristic(ONE << count);
    printf("Current score: %i\n", current_score);
    if (current_score == best_score)
      best_move |= ONE << count;
    if (current_score > best_score)
    {
      best_move = ONE << count;
      best_score = current_score;
    }
    // best_move = current_score > best_score ? ONE << count : best_move;
    // best_score = current_score > best_score ? ONE << current_score : best_score;
  }

  return random_move(best_move);
}

int main(int argc, char const *argv[])
{
  // srand(time(NULL));
  uint_fast64_t test = 0x102004080000;
  uint_fast64_t most_promising = most_promising_move(test);
  // uint_fast64_t most_promising = ONE;
  printf("Most promising move: %c%i\n", 
          (__builtin_ctzll(most_promising) % BOARD_WIDTH) + 'A',
          (__builtin_ctzll(most_promising) / BOARD_HEIGHT)+1);
  return 0;
}
