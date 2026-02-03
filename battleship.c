#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Forward declarations
void initialization();
void teardown();
bool acceptInput(char *row, int *col);
bool updateState(char row, int col, char *result);
void displayWorld(const char *result);

int main() {
    initialization();

    bool running = true;
    char row;
    int col;
    char result[20];

    while (running) {
        if (!acceptInput(&row, &col)) {
            running = false;  // to exit 
            continue;
        }

        if (updateState(row, col, result)) {
            displayWorld(result);
        }
    }

    teardown();
    return 0;
}

// Setup game environment (empty for now)
void initialization() {
    printf("=== Welcome to Battleship ===\n");
    printf("Please enter moves in the format Letter (A-J) + Number (0-9).\n");
    printf("To quit, type Q.\n\n");
}

// Cleanup game environment (empty for now)
void teardown() {
    printf("\nExiting Battleship... Bye bye!\n");
}

// To get user input
bool acceptInput(char *row, int *col) {
    char buffer[100];
    printf("Enter your move (e.g., A5): ");
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return false;
    }

    // To remove new line
    buffer[strcspn(buffer, "\n")] = 0;

    // Command to exit
    if (strcasecmp(buffer, "Q") == 0 || strcasecmp(buffer, "QUIT") == 0) {
        return false;
    }

    // Validate input length
    if (strlen(buffer) < 2) {
        printf("Invalid input. Please enter a letter A-J followed by a number 0-9.\n");
        return acceptInput(row, col);
    }

    char letter = toupper(buffer[0]);
    int number = buffer[1] - '0';

    // Validate range
    if (letter < 'A' || letter > 'J' || number < 0 || number > 9) {
        printf("Invalid input. Please enter a letter A-J and a number 0-9.\n");
        return acceptInput(row, col);
    }

    *row = letter;
    *col = number;
    return true;
}

// Update game state: even = hit, odd = miss
bool updateState(char row, int col, char *result) {
    if (col % 2 == 0) {
        strcpy(result, "Hit!");
    } else {
        strcpy(result, "Miss!");
    }
    return true;
}

// Display the results
void displayWorld(const char *result) {
    printf("Result: %s\n\n", result);
}
