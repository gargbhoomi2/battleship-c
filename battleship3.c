#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define GRID_SIZE 10
#define NUM_SHIPS 5

typedef enum { EMPTY, SHIP, HIT, MISS, DESTROYED } CellStatus;

typedef struct {
    int size;
    char name[20];
} Ship;

typedef struct {
    CellStatus **playerShips;
    CellStatus **playerShots;
    CellStatus **computerShips;
    CellStatus **computerShots;
} GameState;

// Ship list: size and name
Ship ships[NUM_SHIPS] = {
    {5, "Carrier"},
    {4, "Battleship"},
    {3, "Cruiser"},
    {3, "Submarine"},
    {2, "Destroyer"}
};

// Allocate a new grid
CellStatus **AllocateGrid() {
    CellStatus **grid = malloc(GRID_SIZE * sizeof(CellStatus *));
    if (!grid) {
        printf("Memory allocation failed!\n");
        exit(1);
    }
    for (int i = 0; i < GRID_SIZE; i++) {
        grid[i] = malloc(GRID_SIZE * sizeof(CellStatus));
        if (!grid[i]) {
            printf("Memory allocation failed!\n");
            exit(1);
        }
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = EMPTY;
        }
    }
    return grid;
}

// Free a grid
void FreeGrid(CellStatus **grid) {
    for (int i = 0; i < GRID_SIZE; i++) {
        free(grid[i]);
    }
    free(grid);
}

// Print a grid
void PrintGrid(CellStatus **grid, int hideShips) {
    printf("    ");
    for (int j = 0; j < GRID_SIZE; j++) printf("%2d ", j);
    printf("\n");
    for (int i = 0; i < GRID_SIZE; i++) {
        printf("%c ", 'A' + i);
        for (int j = 0; j < GRID_SIZE; j++) {
            char symbol = ' ';
            switch (grid[i][j]) {
                case EMPTY: symbol = '.'; break;
                case SHIP: symbol = hideShips ? '.' : 'S'; break;
                case HIT: symbol = 'X'; break;
                case MISS: symbol = 'o'; break;
                default: break;
            }
            printf(" %c ", symbol);
        }
        printf("\n");
    }
}

// Randomly place ships on a grid
void RandomlyPlaceShips(CellStatus **grid) {
    for (int s = 0; s < NUM_SHIPS; s++) {
        int placed = 0;
        while (!placed) {
            int row = rand() % GRID_SIZE;
            int col = rand() % GRID_SIZE;
            int vertical = rand() % 2;
            int size = ships[s].size;
            int fits = 1;

            if (vertical) {
                if (row + size > GRID_SIZE) continue;
                for (int i = 0; i < size; i++)
                    if (grid[row + i][col] != EMPTY) fits = 0;
                if (fits) {
                    for (int i = 0; i < size; i++)
                        grid[row + i][col] = SHIP;
                    placed = 1;
                }
            } else {
                if (col + size > GRID_SIZE) continue;
                for (int i = 0; i < size; i++)
                    if (grid[row][col + i] != EMPTY) fits = 0;
                if (fits) {
                    for (int i = 0; i < size; i++)
                        grid[row][col + i] = SHIP;
                    placed = 1;
                }
            }
        }
    }
}

// Setup single-player game
GameState *SetupSinglePlayer() {
    GameState *game = malloc(sizeof(GameState));
    if (!game) {
        printf("Memory allocation failed!\n");
        exit(1);
    }

    game->playerShips = AllocateGrid();
    game->playerShots = AllocateGrid();
    game->computerShips = AllocateGrid();
    game->computerShots = AllocateGrid();

    RandomlyPlaceShips(game->playerShips);
    RandomlyPlaceShips(game->computerShips);

    return game;
}

// Teardown: free all memory
void TeardownSinglePlayer(GameState *game) {
    FreeGrid(game->playerShips);
    FreeGrid(game->playerShots);
    FreeGrid(game->computerShips);
    FreeGrid(game->computerShots);
    free(game);
}

// Check if all ships are destroyed
int SinglePlayerDidWin(CellStatus **grid) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == SHIP) return 0;
        }
    }
    return 1;
}

// Player takes a shot
int MakeSinglePlayerShot(GameState *game, int row, int col) {
    if (game->computerShips[row][col] == SHIP) {
        game->computerShips[row][col] = HIT;
        game->playerShots[row][col] = HIT;
        return 1;
    } else {
        game->computerShips[row][col] = MISS;
        game->playerShots[row][col] = MISS;
        return 0;
    }
}

// Computer takes a random shot
void GetSinglePlayerShot(GameState *game) {
    int row, col;
    do {
        row = rand() % GRID_SIZE;
        col = rand() % GRID_SIZE;
    } while (game->computerShots[row][col] != EMPTY);

    if (game->playerShips[row][col] == SHIP) {
        game->playerShips[row][col] = HIT;
        game->computerShots[row][col] = HIT;
        printf("Computer hit your ship at %c%d!\n", 'A' + row, col);
    } else {
        game->playerShips[row][col] = MISS;
        game->computerShots[row][col] = MISS;
        printf("Computer missed at %c%d.\n", 'A' + row, col);
    }
}

// Display both boards
void DisplayWorld(GameState *game) {
    printf("\n=== Your Ships ===\n");
    PrintGrid(game->playerShips, 0);
    printf("\n=== YourShots ===\n");
	
    PrintGrid(game->playerShots, 1);
 }

// Main game loop
int main() {
    srand(time(NULL));
    GameState *game = SetupSinglePlayer();
    int gameOver = 0;

    printf("Welcome to Battleship 3: Single Player Mode!\n");
    printf("Type 'quit' at any time to end the game.\n");

    while (!gameOver) {
        DisplayWorld(game);
        char input[10];
        printf("\nEnter your shot (e.g. A5): ");
        scanf("%9s", input);

         // if player wants to quit
        if (strcmp(input, "quit") == 0 || strcmp(input, "QUIT") == 0) {
            printf("You chose to quit the game. Goodbye!\n");
            break;
        }

        int row = input[0] - 'A';
        int col = input[1] - '0';

        if (row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE) {
            printf("Invalid input.\n");
            continue;
        }

        int hit = MakeSinglePlayerShot(game, row, col);
        if (hit)
            printf("You hit a ship!\n");
        else
            printf("You missed.\n");

        if (SinglePlayerDidWin(game->computerShips)) {
            printf("You won! All enemy ships destroyed.\n");
            break;
        }

        GetSinglePlayerShot(game);

        if (SinglePlayerDidWin(game->playerShips)) {
            printf("Computer won! Your ships are all destroyed.\n");
            break;
        }
    }

    TeardownSinglePlayer(game);
    return 0;
}
