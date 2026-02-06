#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

/* Socket headers */
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdarg.h>   /* needed for SendLine formatting */
#include <strings.h>  /* for strcasecmp() */

#define GRID_SIZE 10
#define NUM_SHIPS 5
#define LINE_BUF 128

/* Types for cells and ships */
typedef enum { EMPTY, SHIP, HIT, MISS } CellStatus;

typedef struct {
    int size;
    char name[20];
} Ship;

/* Game state: your ship grid and your shots */
typedef struct {
    CellStatus **playerShips;
    CellStatus **playerShots;
} GameState;

/* List of ships used in the game */
Ship ships[NUM_SHIPS] = {
    {5, "Carrier"},
    {4, "Battleship"},
    {3, "Cruiser"},
    {3, "Submarine"},
    {2, "Destroyer"}
};

/* Memory helpers */

/* Make a GRID_SIZE x GRID_SIZE grid filled with EMPTY */
CellStatus **AllocateGrid(void) {
    CellStatus **grid = malloc(GRID_SIZE * sizeof(CellStatus *));
    if (!grid) { perror("malloc"); exit(EXIT_FAILURE); }
    for (int r = 0; r < GRID_SIZE; ++r) {
        grid[r] = malloc(GRID_SIZE * sizeof(CellStatus));
        if (!grid[r]) { perror("malloc"); exit(EXIT_FAILURE); }
        for (int c = 0; c < GRID_SIZE; ++c) grid[r][c] = EMPTY;
    }
    return grid;
}

/* Free a grid made by AllocateGrid */
void FreeGrid(CellStatus **grid) {
    if (!grid) return;
    for (int r = 0; r < GRID_SIZE; ++r) free(grid[r]);
    free(grid);
}

/* Drawing the boards */

/* Print one grid; if hideShips is 1 we do not show S for ships */
void PrintGrid(CellStatus **grid, int hideShips) {
    printf("    ");
    for (int c = 0; c < GRID_SIZE; ++c) printf("%2d ", c);
    printf("\n");
    for (int r = 0; r < GRID_SIZE; ++r) {
        printf("%c  ", 'A' + r);
        for (int c = 0; c < GRID_SIZE; ++c) {
            char ch;
            switch (grid[r][c]) {
                case EMPTY: ch = '.'; break;
                case SHIP:  ch = hideShips ? '.' : 'S'; break;
                case HIT:   ch = 'X'; break;
                case MISS:  ch = 'o'; break;
                default:    ch = '?'; break;
            }
            printf(" %c ", ch);
        }
        printf("\n");
    }
}

/* Show your ship board and your shot board */
void DisplayWorld(GameState *game) {
    printf("\n=== Your Ships ===\n");
    PrintGrid(game->playerShips, 0);
    printf("\n=== Your Shots ===\n");
    PrintGrid(game->playerShots, 1);
}

/* Ship placement */

/* Put all ships on the grid in random spots without overlapping */
void RandomlyPlaceShips(CellStatus **grid) {
    for (int s = 0; s < NUM_SHIPS; ++s) {
        int placed = 0;
        int size = ships[s].size;
        while (!placed) {
            int r = rand() % GRID_SIZE;
            int c = rand() % GRID_SIZE;
            int vertical = rand() % 2;
            int fits = 1;
            if (vertical) {
                if (r + size > GRID_SIZE) { continue; }
                for (int i = 0; i < size; ++i) {
                    if (grid[r+i][c] != EMPTY) { fits = 0; break; }
                }
                if (fits) {
                    for (int i = 0; i < size; ++i) grid[r+i][c] = SHIP;
                    placed = 1;
                }
            } else {
                if (c + size > GRID_SIZE) { continue; }
                for (int i = 0; i < size; ++i) {
                    if (grid[r][c+i] != EMPTY) { fits = 0; break; }
                }
                if (fits) {
                    for (int i = 0; i < size; ++i) grid[r][c+i] = SHIP;
                    placed = 1;
                }
            }
        }
    }
}

/* Single-player setup and cleanup */

/* Make a single-player game: make grids and place ships for player */
GameState *SetupSinglePlayer(void) {
    GameState *game = malloc(sizeof(GameState));
    if (!game) { perror("malloc"); exit(EXIT_FAILURE); }
    game->playerShips = AllocateGrid();
    game->playerShots = AllocateGrid();

    /* For single-player we only store player grids here.
       The computer's grid is handled separately in main. */
    RandomlyPlaceShips(game->playerShips); /* put player ships randomly */

    return game;
}

/* Free memory for a single-player game */
void TeardownSinglePlayer(GameState *game) {
    if (!game) return;
    FreeGrid(game->playerShips);
    FreeGrid(game->playerShots);
    free(game);
}

/* Single-player helper functions */

/* Return 1 if no SHIP cells are left on this grid, else 0 */
int GridAllShipsDestroyed(CellStatus **grid) {
    for (int r = 0; r < GRID_SIZE; ++r)
        for (int c = 0; c < GRID_SIZE; ++c)
            if (grid[r][c] == SHIP) return 0;
    return 1;
}

/* Mark a shot on the grid and say if it was a hit (1) or miss (0) */
int ApplyShotToGrid(CellStatus **grid, int row, int col) {
    if (grid[row][col] == SHIP) {
        grid[row][col] = HIT;
        return 1;
    } else {
        if (grid[row][col] == EMPTY) grid[row][col] = MISS;
        return 0;
    }
}

/* Networking helper functions */

/* Send the whole buffer over the socket */
int SendAll(int sockfd, const char *buffer, size_t length) {
    size_t total_sent = 0;
    while (total_sent < length) {
        ssize_t sent = send(sockfd, buffer + total_sent, length - total_sent, 0);
        if (sent <= 0) return -1;
        total_sent += (size_t)sent;
    }
    return 0;
}

/* Read one line (ending with '\n') from the socket.
   outbuf has space maxlen. Returns number of bytes or -1. */
ssize_t ReceiveLine(int sockfd, char *outbuf, size_t maxlen) {
    size_t idx = 0;
    while (idx + 1 < maxlen) {
        char ch;
        ssize_t n = recv(sockfd, &ch, 1, 0);
        if (n <= 0) return -1; /* no data: closed or error */
        outbuf[idx++] = ch;
        if (ch == '\n') break;
    }
    outbuf[idx] = '\0';
    return (ssize_t)idx;
}

/* Send formatted text plus a newline to the socket */
int SendLine(int sockfd, const char *fmt, ...) {
    char buffer[LINE_BUF];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    if (n < 0) return -1;
    if ((size_t)n >= sizeof(buffer) - 1) return -1;
    /* add newline if missing */
    if (buffer[n-1] != '\n') {
        if (n + 1 >= (int)sizeof(buffer)) return -1;
        buffer[n] = '\n';
        buffer[n+1] = '\0';
        n++;
    }
    return SendAll(sockfd, buffer, (size_t)n);
}

/* Two-player: shots and replies */

/*
 * Simple text protocol:
 * - To shoot:   "SHOT r c\n"
 * - Reply:      "RESULT HIT\n" or "RESULT MISS\n"
 * - To quit:    "QUIT\n"
 *
 * Server takes the first shot.
 */

/* Answer a shot from the other player and tell them hit or miss */
int HandleIncomingShotAndRespond(GameState *localGame, int row, int col, int sockfd) {
    int hit = ApplyShotToGrid(localGame->playerShips, row, col);
    if (hit) SendLine(sockfd, "RESULT HIT");
    else     SendLine(sockfd, "RESULT MISS");
    return hit;
}

/* Shoot at the other player and update your shot grid */
int FireShotAtOpponent(GameState *localGame, int row, int col, int sockfd) {
    if (localGame->playerShots[row][col] != EMPTY) {
        printf("You already fired at %c%d. Choose a different target.\n", 'A'+row, col);
        return -2;
    }

    if (SendLine(sockfd, "SHOT %d %d", row, col) < 0) { perror("send"); return -1; }

    char line[LINE_BUF];
    if (ReceiveLine(sockfd, line, sizeof(line)) < 0) {
        printf("Connection closed while waiting for result.\n");
        return -1;
    }

    if (strncmp(line, "RESULT", 6) == 0) {
        if (strstr(line, "HIT")) {
            localGame->playerShots[row][col] = HIT;
            printf("You hit opponent at %c%d!\n", 'A'+row, col);
            return 1;
        } else {
            localGame->playerShots[row][col] = MISS;
            printf("You missed at %c%d.\n", 'A'+row, col);
            return 0;
        }
    } else if (strncmp(line, "QUIT", 4) == 0) {
        printf("Opponent quit. You win by default.\n");
        return -1;
    } else {
        printf("Unexpected response: %s\n", line);
        return -1;
    }
}

/* Two-player main loop */

/* Play two-player game on this socket.
   If amServer is 1, this side shoots first. */
void PlayTwoPlayer(GameState *localGame, int sockfd, int amServer) {
    char input[LINE_BUF];

    printf("Two-player game started. Type 'quit' to leave and send QUIT.\n");
    if (amServer) printf("You are server: you shoot first.\n");
    else          printf("You are client: opponent shoots first.\n");

    while (1) {
        if (amServer) {
            /* Your turn to shoot */
            DisplayWorld(localGame);
            printf("\nYour turn (format A5). Type 'quit' to quit: ");
            if (!fgets(input, sizeof(input), stdin)) {
                printf("stdin closed.\n");
                break;
            }
            input[strcspn(input, "\n")] = '\0';
            if (strcasecmp(input, "quit") == 0) {
                SendLine(sockfd, "QUIT");
                printf("You quit. Closing connection.\n");
                break;
            }
            if (strlen(input) < 2) {
                printf("Invalid input.\n");
                continue;
            }
            int row = toupper((unsigned char)input[0]) - 'A';
            int col = input[1] - '0';
            if (row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE) {
                printf("Coordinates out of range.\n");
                continue;
            }

            int res = FireShotAtOpponent(localGame, row, col, sockfd);
            if (res == -1) break;    /* connection error or opponent quit */
            if (res == -2) continue; /* already fired there */

            /* Now wait for their shot */
            char line[LINE_BUF];
            if (ReceiveLine(sockfd, line, sizeof(line)) <= 0) {
                printf("Connection closed by opponent.\n");
                break;
            }
            if (strncmp(line, "SHOT", 4) == 0) {
                int r, c;
                if (sscanf(line, "SHOT %d %d", &r, &c) != 2) {
                    printf("Malformed SHOT received.\n");
                    break;
                }
                int hit = ApplyShotToGrid(localGame->playerShips, r, c);
                if (hit) SendLine(sockfd, "RESULT HIT");
                else     SendLine(sockfd, "RESULT MISS");
                if (hit) printf("Opponent hit you at %c%d.\n", 'A'+r, c);
                else     printf("Opponent missed at %c%d.\n", 'A'+r, c);
                if (GridAllShipsDestroyed(localGame->playerShips)) {
                    printf("All your ships destroyed. You lose.\n");
                    break;
                }
            } else if (strncmp(line, "QUIT", 4) == 0) {
                printf("Opponent quit. You win.\n");
                break;
            } else {
                printf("Unexpected message from opponent: %s\n", line);
                break;
            }
        } else {
            /* Client: wait for server to shoot first */
            char line[LINE_BUF];
            if (ReceiveLine(sockfd, line, sizeof(line)) <= 0) {
                printf("Connection closed by opponent.\n");
                break;
            }
            if (strncmp(line, "SHOT", 4) == 0) {
                int r, c;
                if (sscanf(line, "SHOT %d %d", &r, &c) != 2) {
                    printf("Malformed SHOT received.\n");
                    break;
                }
                int hit = ApplyShotToGrid(localGame->playerShips, r, c);
                if (hit) SendLine(sockfd, "RESULT HIT");
                else     SendLine(sockfd, "RESULT MISS");
                if (hit) printf("Opponent hit you at %c%d.\n", 'A'+r, c);
                else     printf("Opponent missed at %c%d.\n", 'A'+r, c);
                if (GridAllShipsDestroyed(localGame->playerShips)) {
                    printf("All your ships destroyed. You lose.\n");
                    break;
                }
            } else if (strncmp(line, "QUIT", 4) == 0) {
                printf("Opponent quit. You win.\n");
                break;
            } else {
                printf("Unexpected message from opponent: %s\n", line);
                break;
            }

            /* Now you shoot */
            DisplayWorld(localGame);
            printf("\nYour turn (format A5). Type 'quit' to quit: ");
            if (!fgets(input, sizeof(input), stdin)) {
                printf("stdin closed.\n");
                break;
            }
            input[strcspn(input, "\n")] = '\0';
            if (strcasecmp(input, "quit") == 0) {
                SendLine(sockfd, "QUIT");
                printf("You quit. Closing connection.\n");
                break;
            }
            if (strlen(input) < 2) {
                printf("Invalid input.\n");
                continue;
            }
            int row = toupper((unsigned char)input[0]) - 'A';
            int col = input[1] - '0';
            if (row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE) {
                printf("Coordinates out of range.\n");
                continue;
            }

            int res = FireShotAtOpponent(localGame, row, col, sockfd);
            if (res == -1) break;
            if (res == -2) continue;
        }
    }

    close(sockfd);
    printf("Two-player session ended.\n");
}

/* Server and client setup */

/* Run as server: open port, accept one player, then start game */
int RunServerMode(int port) {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) { perror("socket"); return -1; }

    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listenfd);
        return -1;
    }

    if (listen(listenfd, 1) < 0) {
        perror("listen");
        close(listenfd);
        return -1;
    }

    printf("Server listening on port %d. Waiting for a client...\n", port);
    int clientfd = accept(listenfd, NULL, NULL);
    if (clientfd < 0) {
        perror("accept");
        close(listenfd);
        return -1;
    }
    close(listenfd);
    printf("Client connected.\n");

    GameState *localGame = malloc(sizeof(GameState));
    if (!localGame) {
        perror("malloc");
        close(clientfd);
        return -1;
    }
    localGame->playerShips = AllocateGrid();
    localGame->playerShots = AllocateGrid();
    RandomlyPlaceShips(localGame->playerShips);

    PlayTwoPlayer(localGame, clientfd, 1); /* server shoots first */

    FreeGrid(localGame->playerShips);
    FreeGrid(localGame->playerShots);
    free(localGame);
    return 0;
}

/* Run as client: connect to given ip:port and start game */
int RunClientMode(const char *ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return -1; }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, &servaddr.sin_addr, ip) <= 0) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        close(sockfd);
        return -1;
    }

    printf("Connecting to %s:%d ...\n", ip, port);
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    printf("Connected to server.\n");

    GameState *localGame = malloc(sizeof(GameState));
    if (!localGame) {
        perror("malloc");
        close(sockfd);
        return -1;
    }
    localGame->playerShips = AllocateGrid();
    localGame->playerShots = AllocateGrid();
    RandomlyPlaceShips(localGame->playerShips);

    PlayTwoPlayer(localGame, sockfd, 0); /* client waits first, then shoots */

    FreeGrid(localGame->playerShips);
    FreeGrid(localGame->playerShots);
    free(localGame);
    return 0;
}

/* Main: choose single-player, server, or client */
int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));

    if (argc == 1) {
        /* No arguments: single-player (you vs computer) */
        GameState *game = malloc(sizeof(GameState));
        if (!game) { perror("malloc"); return 1; }
        CellStatus **computerShips = AllocateGrid();

        game->playerShips = AllocateGrid();
        game->playerShots = AllocateGrid();

        /* Place ships for you and for computer */
        RandomlyPlaceShips(game->playerShips);
        RandomlyPlaceShips(computerShips);

        printf("Welcome to Battleship (single-player).\nType 'quit' at any prompt to exit.\n");

        while (1) {
            DisplayWorld(game);
            char input[LINE_BUF];
            printf("\nEnter your shot (e.g. A5): ");
            if (!fgets(input, sizeof(input), stdin)) break;
            input[strcspn(input, "\n")] = '\0';
            if (strcasecmp(input, "quit") == 0) {
                printf("Quitting.\n");
                break;
            }
            if (strlen(input) < 2) {
                printf("Invalid input.\n");
                continue;
            }
            int row = toupper((unsigned char)input[0]) - 'A';
            int col = input[1] - '0';
            if (row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE) {
                printf("Coordinates out of range.\n");
                continue;
            }
            if (game->playerShots[row][col] != EMPTY) {
                printf("You already shot there.\n");
                continue;
            }

            int hit = ApplyShotToGrid(computerShips, row, col);
            game->playerShots[row][col] = hit ? HIT : MISS;
            if (hit) printf("You hit a ship at %c%d!\n", 'A'+row, col);
            else     printf("You missed at %c%d.\n", 'A'+row, col);

            if (GridAllShipsDestroyed(computerShips)) {
                printf("You won! All opponent ships destroyed.\n");
                break;
            }

            /* Computer picks a random new spot to shoot */
            int crow, ccol;
            do {
                crow = rand() % GRID_SIZE;
                ccol = rand() % GRID_SIZE;
            } while (game->playerShips[crow][ccol] == HIT ||
                     game->playerShips[crow][ccol] == MISS);

            int chit = ApplyShotToGrid(game->playerShips, crow, ccol);
            if (chit) printf("Computer hit you at %c%d!\n", 'A'+crow, ccol);
            else      printf("Computer missed at %c%d.\n", 'A'+crow, ccol);

            if (GridAllShipsDestroyed(game->playerShips)) {
                printf("Computer won! Your ships are destroyed.\n");
                break;
            }
        }

        FreeGrid(game->playerShips);
        FreeGrid(game->playerShots);
        FreeGrid(computerShips);
        free(game);
        return 0;
    } else if (argc == 2) {
        /* One argument: server mode, argument is port */
        int port = atoi(argv[1]);
        if (port <= 0) {
            fprintf(stderr, "Invalid port: %s\n", argv[1]);
            return 1;
        }
        return RunServerMode(port);
    } else if (argc == 3) {
        /* Two arguments: client mode, ip and port */
        const char *ip = argv[1];
        int port = atoi(argv[2]);
        if (port <= 0) {
            fprintf(stderr, "Invalid port: %s\n", argv[2]);
            return 1;
        }
        return RunClientMode(ip, port);
    } else {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s              (single-player)\n", argv[0]);
        fprintf(stderr, "  %s <port>       (server)\n", argv[0]);
        fprintf(stderr, "  %s <ip> <port>  (client)\n", argv[0]);
        return 1;
    }
}