/*
Random player with evaluation function. Picks move with highest score. Only regards currently possible moves, no game tree.
make eval_player

This player accepts commands from standard input. It is designed to be used with 
the ReversiController. Usage of the ReversiController:

java -jar ReversiController.jar <player1.exe> <player2.exe> [options]
Options:
repeat=<n>    how many games to play
show=0|1      whether to show each state
timeout=-1|return|<ms>    -1 for infinite wait, return for continue after key press, <ms> for timeout in ms
Examples:
java -jar ReversiController.jar ./random_player ./eval_player . repeat=1 show=1 timeout=-1 stderr=0
java -jar ReversiController.jar random_player.exe eval_player.exe . repeat=1 show=1 timeout=-1 stderr=0
java -jar ReversiController.jar ./random_player ./eval_player . repeat=100 show=0 timeout=-1 stderr=0
java -jar ReversiController.jar random_player.exe eval_player.exe . repeat=100 show=0 timeout=-1 stderr=0

Commands (cp = controller to player, pc = player to controller, | means or):

Initialization (tells the player, which stones it has):
cp: init: X | O

Seeding the random number generator:
cp: srand: <number>

Stopping the player:
cp: exit

Receiving the opponent move (or none) and responding with our own move (or none):
cp: d6 | none (opponent move)
pc: e6 | none (own move)
*/

///////////////////////////////////////////////////////////////////////////////

#include "base.h"

typedef struct {
    int x;
    int y;
} Position;

Position make_position(int x, int y) {
    Position p = { x, y };
    return p;
}

void print_position(Position p) {
    fprintf(stderr, "%c%d\n", p.x + 'a', p.y + 1);
}

#define N 8

typedef struct {
    char board[N][N]; // the NxN playing board
    char my_stone;
} Game;

// Initialize the board such that it looks like this if printed:
//  |A|B|C|D|E|F|G|H|
// 1|_|_|_|_|_|_|_|_|
// 2|_|_|_|_|_|_|_|_|
// 3|_|_|_|_|_|_|_|_|
// 4|_|_|_|O|X|_|_|_|
// 5|_|_|_|X|O|_|_|_|
// 6|_|_|_|_|_|_|_|_|
// 7|_|_|_|_|_|_|_|_|
// 8|_|_|_|_|_|_|_|_|
Game init_game(char my_stone) {
    Game g;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            g.board [i][j] = '_';
        }
    }
    int a = N / 2;
    int b = a - 1;
    g.board[b][b] = 'O';
    g.board[a][b] = 'X';
    g.board[b][a] = 'X';
    g.board[a][a] = 'O';
    g.my_stone = my_stone;
    return g;
}

// Print the board. The initial board should look like shown above.
void print_board(Game* g) {
    fprintf(stderr, " |A|B|C|D|E|F|G|H|\n");
    for (int i = 0; i < N; i++) {
        fprintf(stderr, "%d", i + 1);
        for (int j = 0; j < N; j++) {
            fprintf(stderr, "|%c", g->board[j][i]);
        }
        fprintf(stderr, "|\n");
    }
    fflush(stderr);
}

// Check whether position (x,y) is on the board.
bool out_of_bounds(int x, int y) {
    return x < 0 || x > N - 1 || y < 0 || y > N - 1;
}

// If it is X's turn, then "my stone" is 'X', otherwise it is 'O'.
char my_stone(Game* g) {
    return g->my_stone;
}

// If it is X's turn, then "your stone" is 'O', otherwise it is 'X'.
char your_stone(Game* g) {
    return (g->my_stone == 'X') ? 'O' : 'X';
}

void switch_stones(Game *g) {
    g->my_stone = (g->my_stone == 'X') ? 'O' : 'X';
}

// Check whether starting at (x,y) direction (dx,dy) is a legal direction
// to reverse stones. A direction is legal if (assuming my stone is 'X')
// the pattern in that direction is: O+X.* (one or more 'O', followed by a 'X', 
// followed by zero or more arbitrary characters).
bool legal_dir(Game* g, int x, int y, int dx, int dy) {
    x += dx; y += dy;
    while (!out_of_bounds(x, y) && (g->board[x][y] == your_stone(g))) {
        x += dx; y += dy;
        if (!out_of_bounds(x, y) && (g->board[x][y] == my_stone(g))) {
            return true;
        }
    }
    return false;
}

// Check whether (x,y) is a legal position to place a stone. A position is legal
// if it is empty ('_'), is on the board, and has at least one legal direction.
bool legal(Game* g, int x, int y) {
    return !out_of_bounds(x, y) && (g->board[x][y] == '_') &&
       (legal_dir(g, x, y, -1, -1) || legal_dir(g, x, y, 0, -1) ||
        legal_dir(g, x, y, 1, -1) || legal_dir(g, x, y, -1, 0) ||
        legal_dir(g, x, y, 1, 0) || legal_dir(g, x, y, -1, 1) ||
        legal_dir(g, x, y, 0, 1) || legal_dir(g, x, y, 1, 1));
}

// Reverse stones starting at (x,y) in direction (dx,dy), but only if the 
// direction is legal. May modify the state of the game.
void reverse_dir(Game* g, int x, int y, int dx, int dy) {
    if (!legal_dir(g, x, y, dx, dy)) return;
    do {
        g->board[x][y] = my_stone(g);
        x += dx; y += dy;
    } while (!out_of_bounds(x, y) && (g->board[x][y] == your_stone(g)));
}

// Reverse the stones in all legal directions starting at (x,y).
// May modify the state of the game.
void reverse(Game* g, int x, int y) {
    reverse_dir(g, x, y, -1, -1); reverse_dir(g, x, y, 0, -1);
    reverse_dir(g, x, y, 1, -1); reverse_dir(g, x, y, -1, 0);
    reverse_dir(g, x, y, 1, 0); reverse_dir(g, x, y, -1, 1);
    reverse_dir(g, x, y, 0, 1); reverse_dir(g, x, y, 1, 1);
}

// Evaluate the cell at position (x,y). If the cell at (x,y) is empty, its value
// is 0. If it is my stone then its value is v. If it is my opponents stone its
// value is -v.
int eval_cell(Game* g, int x, int y, int v) {
    char c = g->board[x][y];
    if (c == my_stone(g)) return v;
    else if (c == your_stone(g)) return -v;
    return 0;
}

// Evaluate the value of the stones on the board for the player 
// whose current turn it is.
//  |A|B|C|D|E|F|G|H|
// 1|_|_|_|_|_|_|_|_|
// 2|_|_|_|_|_|_|_|_|
// 3|_|_|_|_|_|_|_|_|
// 4|_|_|_|O|X|_|_|_|
// 5|_|_|_|X|O|_|_|_|
// 6|_|_|_|_|_|_|_|_|
// 7|_|_|_|_|_|_|_|_|
// 8|_|_|_|_|_|_|_|_|
int eval_board(Game* g) {
    int value = 0;

    int v = 1;
    // all cells have a value of 1
    for (int y = 0; y <= 7; y++) {
        for (int x = 0; x <= 7; x++) {
            value += eval_cell(g, x, y, v);
        }
    }

    // middle area has a higher weight: add 9 for each middle cell one
    v = 9;
    for (int y = 2; y <= 5; y++) {
        for (int x = 2; x < 5; x++) {
            value += eval_cell(g, x, y, v);
        }
    }

    return value;
}

typedef struct {
    Position pos;
    int score;
} Move;

Move make_move(int x, int y, int score) {
    Move m = { make_position(x, y), score };
    return m;
}

#define MOVE_STACK_SIZE 64
typedef struct {
    int length;
    Move values[MOVE_STACK_SIZE];
} MoveStack;

MoveStack make_position_stack() {
    MoveStack s;
    s.length = 0;
    return s;
}

void push(MoveStack *s, Move m) {
    if (s->length >= MOVE_STACK_SIZE) {
        fprintf(stderr, "Stack overflow!\n");
        exit(0);
    }
    s->values[s->length++] = m;
}

Move pop(MoveStack *s) {
    if (s->length <= 0) {
        fprintf(stderr, "Stack empty!\n");
        exit(0);
    }
    return s->values[--s->length];
}

Move random_move(MoveStack *s) {
    if (s->length <= 0) {
        fprintf(stderr, "Stack empty!\n");
        exit(0);
    }
    // not available on Windows :-(   int i = arc4random_uniform(s->length);
    int i = i_rnd(s->length);
    return s->values[i];
}

// Count the number of cells of the given value.
int count_cells(Game *g, char c) {
    int result = 0;
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            if (g->board[x][y] == c) {
                result++;
            }
        }
    }
    return result;
}

// Tests all positions and chooses a random one.
Position this_players_turn(Game *g) {
    clock_t t = clock(); // get current timestamp
    Game gLocal = *g; // copy game state
    MoveStack s = make_position_stack();
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            if (legal(&gLocal, x, y)) {
                reverse(&gLocal, x, y);
                int score = eval_board(&gLocal);
                push(&s, make_move(x, y, score));
                gLocal = *g; // restore state
            }
        }
    }
    if (s.length <= 0) {
        return make_position(-1, -1); // could not find valid move
    }
    // fprintf(stderr, "%d\n", s.length);
    int ibest = 0;
    for (int i = 1; i < s.length; i++) {
        if (s.values[i].score > s.values[ibest].score) ibest = i;
    }
    
    // bests
    MoveStack best_moves = make_position_stack();
    for (int i = 0; i < s.length; i++) {
        if (s.values[i].score >= s.values[ibest].score) push(&best_moves, s.values[i]);
    }

    t = clock() - t; // compute elapsed time
    double duration = t * 1000.0 / CLOCKS_PER_SEC;
    // fprintf(stderr, "duration: %g ms\n", duration);

    // fprintf(stderr, "best score = %d, size = %d\n", s.values[ibest].score, best_moves.length);
    return random_move(&best_moves).pos;
}

///////////////////////////////////////////////////////////////////////////////

void play(void) {
    Game g;
    while (true) {
        String s = s_input(100);
        int n = s_length(s);
        if (s_equals(s, "exit")) {
            exit(0);
        } else if (s_starts_with(s, "init: ") && n == 7) { // initialization tells use what stone we have
            int i = s_index(s, " ");
            char c = s[i + 1];
            if (c != 'X' && c != 'O') {
                fprintf(stderr, "Illegal stone: %c\n", c);
                exit(0);
            }
            g = init_game(c);
            // print_board(&g);
            // fprintf(stderr, "my stone is: %c\n", my_stone(&g));
        } else if (s_starts_with(s, "srand: ") && n >= 8) { // seed random number generator
            int i = s_index(s, " ");
            int seed = atoi(s + i + 1);
            // fprintf(stderr, "random seed: %d\n", seed);
            i = i_rnd(2); // trigger seed
            srand(seed + i); // call seed again
        } else if (s_equals(s, "none")) { // opponent made no move
            // fprintf(stderr, "opponent made no move\n");
            Position pos = this_players_turn(&g);
            if (pos.x >= 0) {
                reverse(&g, pos.x, pos.y);
                // print_board(&g);
                printf("%c%d\n", pos.x + 'a', pos.y + 1);
            } else {
                printf("none\n"); // no valid move found
            }
        } else if (n == 2) { // regular move of opponent
            Position pos;
            pos.x = (int)tolower(s[0]) - 'a';
            pos.y = (int)s[1] - '1';
            // fprintf(stderr, "opponent set: %c%d\n", pos.x + 'a', pos.y + 1);
            if (pos.x < 0 || pos.x >= N || pos.y < 0 || pos.y >= N) {
                fprintf(stderr, "Opponent move out of bounds: (%d, %d)\n", pos.x, pos.y);
                exit(0);
            }
            switch_stones(&g); // switch to opponent
            reverse(&g, pos.x, pos.y); // make opponent move
//            print_board(&g);
            switch_stones(&g); // switch back to this player
            pos = this_players_turn(&g); // compute our move
            if (pos.x >= 0) {
                reverse(&g, pos.x, pos.y); // make our move
//                print_board(&g);
                printf("%c%d\n", pos.x + 'a', pos.y + 1);
            } else {
                printf("none\n"); // no valid move found
            }
        } else {
            fprintf(stderr, "Unknown command: %s\n", s);
            exit(0);
        }
//        print_board(&g);
//        sleep(2); // seconds
        fflush(stdout); // need to push the data out of the door
    }
}

int main(void) {
    play();
    return 0;
}
