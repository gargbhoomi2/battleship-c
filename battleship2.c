#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

// ENUMS 
typedef enum {
    NO_SHIP,
    CARRIER,    // size 5
    BATTLESHIP, // size 4
    CRUISER,    // size 3
    SUBMARINE,  // size 2
    DESTROYER   // size 1
} ShipType;

typedef enum {
    UNTRIED,
    MISS,
    HIT
} ShotResult;

// Information of ship
typedef struct {
    ShipType type;
    char *name;
    char *code;
    int size;
} ShipInfo;

ShipInfo ships[] = {
    {CARRIER, "Carrier", "CV", 5},
    {BATTLESHIP, "Battleship", "BB", 4},
    {CRUISER, "Cruiser", "CA", 3},
    {SUBMARINE, "Submarine", "SS", 2},
    {DESTROYER, "Destroyer", "DD", 1}
};

// Global grid size
#define ROWS 10
#define COLS 10

// Declaration of functions 
ShipType **allocateShipGrid(void);
ShotResult **allocateShotGrid(void);
void freeShipGrid(ShipType **grid);
void freeShotGrid(ShotResult **grid);
void printShipGrid(ShipType **grid);
bool placeShip(ShipType **grid, ShipInfo ship);

// grid allocation
ShipType **allocateShipGrid(void) {
    ShipType **grid = malloc(ROWS * sizeof(ShipType *));
    if (!grid) { perror("malloc failed"); exit(EXIT_FAILURE); }
    for (int i = 0; i < ROWS; i++) {
        grid[i] = malloc(COLS * sizeof(ShipType));
        if (!grid[i]) { perror("malloc failed"); exit(EXIT_FAILURE); }
        for (int j = 0; j < COLS; j++) {
            grid[i][j] = NO_SHIP;
        }
    }
    return grid;
}

ShotResult **allocateShotGrid(void) {
    ShotResult **grid = malloc(ROWS * sizeof(ShotResult *));
    if (!grid) { perror("malloc failed"); exit(EXIT_FAILURE); }
    for (int i = 0; i < ROWS; i++) {
        grid[i] = malloc(COLS * sizeof(ShotResult));
        if (!grid[i]) { perror("malloc failed"); exit(EXIT_FAILURE); }
        for (int j = 0; j < COLS; j++) {
            grid[i][j] = UNTRIED;
        }
    }
    return grid;
}

// free grids
void freeShipGrid(ShipType **grid) {
    for (int i = 0; i < ROWS; i++) free(grid[i]);
    free(grid);
}
void freeShotGrid(ShotResult **grid) {
    for (int i = 0; i < ROWS; i++) free(grid[i]);
    free(grid);
}

void printShipGrid(ShipType **grid) {
    printf("     ");
    for (int c = 0; c < COLS; c++) printf("%3d ", c);
    printf("\n");

    for (int r = 0; r < ROWS; r++) {
        printf("%c  ", 'A' + r);
        for (int c = 0; c < COLS; c++) {
            switch (grid[r][c]) {
                case NO_SHIP: printf("    "); break;
                case CARRIER: printf(" CV "); break;
                case BATTLESHIP: printf(" BB "); break;
                case CRUISER: printf(" CA "); break;
                case SUBMARINE: printf(" SS "); break;
                case DESTROYER: printf(" DD "); break;
            }
            printf("|");
        }
        printf("\n");
    }
}

// place a ship
bool placeShip(ShipType **grid, ShipInfo ship) {
    char buffer[20];
    printf("Please enter a location for a ship of %d squares (%s): ",
           ship.size, ship.name);

    if (!fgets(buffer, sizeof(buffer), stdin)) return false;
    buffer[strcspn(buffer, "\n")] = '\0';

    char rowChar;
    int col;
    char orient = 'H'; // default

    if (sscanf(buffer, "%c%d%c", &rowChar, &col, &orient) < 2) {
        printf("wrong input. Try again.\n");
        return false;
    }

    int row = toupper(rowChar) - 'A';
    orient = toupper(orient);

    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) {
        printf("Out of bounds. Try again.\n");
        return false;
    }

    if (orient == 'H') {
        if (col + ship.size > COLS) {
            printf("Ship does not fit horizontally.\n");
            return false;
        }
        for (int j = 0; j < ship.size; j++) {
            if (grid[row][col+j] != NO_SHIP) {
                printf("Overlap detected.\n");
                return false;
            }
        }
        for (int j = 0; j < ship.size; j++) {
            grid[row][col+j] = ship.type;
        }
    } else if (orient == 'V') {
        if (row + ship.size > ROWS) {
            printf("Ship does not fit vertically.\n");
            return false;
        }
        for (int j = 0; j < ship.size; j++) {
            if (grid[row+j][col] != NO_SHIP) {
                printf("Overlap detected.\n");
                return false;
            }
        }
        for (int j = 0; j < ship.size; j++) {
            grid[row+j][col] = ship.type;
        }
    } else {
        printf("Orientation must be H or V.\n");
        return false;
    }

    return true;
}

// main
int main(void) {
    ShipType **shipGrid = allocateShipGrid();
    ShotResult **shotGrid = allocateShotGrid();

    printf("Place your ships. Format examples: A3H (horizontal), C3V (vertical).\n");
    printShipGrid(shipGrid);

    // loop through each ship
    for (int i = 0; i < sizeof(ships)/sizeof(ships[0]); i++) {
        bool placed = false;
        while (!placed) {
            placed = placeShip(shipGrid, ships[i]);
            printShipGrid(shipGrid);
        }
    }

    printf("Ship placement complete.\n");

    freeShipGrid(shipGrid);
    freeShotGrid(shotGrid);
    return 0;
}
