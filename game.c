#include "definitions.h"
#include "board.h"
#include "scoring.h"
// #include "debug.h"

Position make_position(int x, int y)
{
  Position p = {x, y};
  return p;
}

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


void play(void)
{
  srand(time(NULL));
  Game *g = NULL;
  Players us;
#if MEASURE_TIME
  double avg_time = 0;
  int count_time = 0;
#endif

  while (true)
  {
#if MEASURE_TIME
    // clock_t t = clock(); // get current timestamp
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    // fprintf(stderr, "t: %llu\n", t);

#endif

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
#if DEBUG
        fprintf(stderr, "Illegal stone: %c\n", c);
#endif
        free(input_buffer);
        if (g)
          free(g);
        exit(0);
      }

      us = which_stone(c);
      g = init_game(us);

#if DEBUG
      print_board(g);                                    // DEBUG
      fprintf(stderr, "my stone is: %c\n", my_stone(g)); // DEBUG
#endif
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
#if DEBUG
      fprintf(stderr, "opponent made no move\n"); // DEBUG
#endif
      Position pos = get_turn(g);
      if (pos.x >= 0)
      {
        reverse(g, pos.x, pos.y);
        // print_board(g);
        printf("%c%d\n", pos.x + 'a', pos.y + 1);
      }
      else
      {
        printf("none\n"); // no valid move found
#if MEASURE_TIME
        if (count_time)
          fprintf(stderr, "Average time: %f\n", avg_time / count_time);
        avg_time = 0;
        count_time = 0;
#endif
      }
    }

    // Only the first 16 bits and the following two bits for line feed should be set
    // todo: Maybe needs to consider Windows and Unix stuff (CR LF)
    else if (!(0xFFFFFFFFFFF50000 & *gyoutou))
    {
      *gyoutou &= 0xFFFF;
#if DEBUG
      fprintf(stderr, "some_move: 0x%llx\n", *gyoutou);
#endif
      // regular move of opponent
      Position pos;
      pos.x = toupper(*gyoutou & 0xFF) - 'A';
      pos.y = 0xFF & (*gyoutou >> __CHAR_BIT__) - '1';
#if DEBUG
      fprintf(stderr, "opponent set: %c%d\n", pos.x + 'a', pos.y + 1); // DEBUG
#endif
      if (out_of_bounds(pos.x, pos.y))
      {
#if DEBUG
        fprintf(stderr, "Opponent move out of bounds: (%d, %d)\n", pos.x, pos.y);
#endif
        free(input_buffer);
        if (g)
          free(g);

        exit(0);
      }

      switch_stones(g);           // switch to opponent
      reverse(g, pos.x, pos.y);   // make opponent move
                                  // print_board(g); // DEBUG
      switch_stones(g);           // switch back to this player
      pos = get_turn(g); // compute our move
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
      fprintf(stderr, "Unknown command: %s\n", input_buffer);
      free(input_buffer);
      if (g)
        free(g);
      exit(0);
    }
#if DEBUG
    print_board(g); // DEBUG
#endif
    //        sleep(2); // seconds
    fflush(stdout); // need to push the data out of the door
#if DEBUG
    fprintf(stderr, "Possible moves:0x%" PRIxFAST64 "\n", g->legal_moves);
#endif
    free(input_buffer);

#if MEASURE_TIME
    // t = clock() - t; // compute elapsed time
    // fprintf(stderr, "t: %llu\n", t);
    clock_gettime(CLOCK_REALTIME, &end);
    double time_spent = (end.tv_sec - start.tv_sec) * 1000 +
                        (end.tv_nsec - start.tv_nsec) / MILLION;

    // double duration = t * 1000.0 / CLOCKS_PER_SEC;
    fprintf(stderr, "duration: %g ms\n", time_spent);
    avg_time += time_spent;
    count_time++;
#endif
  }
}

int main(void)
{
  play();
  return EXIT_SUCCESS;
}
