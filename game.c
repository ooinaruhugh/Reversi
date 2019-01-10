#include "definitions.h"
#include "board.h"
#include "scoring.h"
// #include "debug.h"

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
bool out_of_bounds(ThreeChars position)
{
  return (position && 0xFF) < 0x41 || (position && 0xFF) > 0x48 || (position && 0xFF00) < 0x31 || (position && 0xFF00) > 0x38;
  // return (position & 0xC0B0);// || position > ;
}

static inline Bitboard make_move(int x, int y)
{
  return ONE << x + BOARD_WIDTH * y;
}

ThreeChars move_to_string(Bitboard move)
{
  return ('A' + ctzll(move) % BOARD_WIDTH) + ('1' + ctzll(move) / BOARD_WIDTH) << __CHAR_BIT__;
}

void play(void)
{
  srand(time(NULL));
  Game *g = NULL;
  Players us;

  while (true)
  {
#if MEASURE_TIME
    arm_timer();
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

      if (g->legal_moves)
      {
        Bitboard player_move = this_players_turn(g);
        reverse(g, player_move); // make our move
                                 // print_board(g); // DEBUG
        // print_board(g);
        ThreeChars buffer = move_to_string(player_move);
        printf("%s\n", (char *)&buffer);
      }
      else
      {
        printf("none\n"); // no valid move found
#if MEASURE_TIME
        average_time();
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

      if (out_of_bounds((ThreeChars)*gyoutou))
      {
#if DEBUG
        fprintf(stderr, "Opponent move out of bounds: (0x%x, 0x%x)\n", *gyoutou & 0xFF, *gyoutou >> __CHAR_BIT__);
#endif
        free(input_buffer);
        if (g)
          free(g);

        exit(0);
      }

      // regular move of opponent
      Bitboard enemy_move = make_move(toupper(*gyoutou & 0xFF) - 'A', 0xFF & (*gyoutou >> __CHAR_BIT__) - '1');
#if DEBUG
      ThreeChars buffer = move_to_string(enemy_move);
      fprintf(stderr, "opponent set: %s\n", (char *)buffer); // DEBUG
#endif

      switch_stones(g);       // switch to opponent
      reverse(g, enemy_move); // make opponent move
                              // print_board(g); // DEBUG
      switch_stones(g);       // switch back to this player
      if (g->legal_moves)
      {
        Bitboard player_move = this_players_turn(g);
        reverse(g, player_move); // make our move
                                 // print_board(g); // DEBUG
        ThreeChars buffer = move_to_string(enemy_move);
        printf("%s\n", (char *)&buffer);
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
    update_stats();
#endif
  }
}

int main(void)
{
  play();
  return EXIT_SUCCESS;
}
