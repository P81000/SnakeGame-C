#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <termio.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>

#define HEIGHT 16
#define WIDTH 32

typedef enum { UP, DOWN, LEFT, RIGHT } Direction;

typedef struct {
    int headX, headY;
    int length;
    Direction dir;
    int body[WIDTH * HEIGHT][2];
} Snake;

void initializeSnake(Snake *snake) {
    snake->headX = WIDTH / 2;
    snake->headY = HEIGHT / 2;
    snake->length = 3;
    snake->dir = RIGHT;

    for (int i = 0; i < snake->length; i++) {
        snake->body[i][0] = snake->headX - i;
        snake->body[i][1] = snake->headY;
    }
}

void drawBoard(int rowOffset, int colOffset, int appleX, int appleY, Snake *snake) {
    printf("\033[H\033[J");
    for(int i = 0; i < rowOffset; i++) {
        printf("\n");
    }

    for(int i = 0; i < HEIGHT; i++) {
        for(int j = 0; j < colOffset; j++) {
            printf(" ");
        }

        for(int j = 0; j < WIDTH; j++) {
            if (i == appleY && j == appleX) {
                printf("*");
            } else if (i == snake->headY && j == snake->headX) {
                switch (snake->dir) {
                    case RIGHT: printf("]"); break;
                    case LEFT: printf("["); break;
                    case DOWN: printf("_"); break;
                    case UP: printf("-"); break;
                }
            } else {
                int isBodyPart = 0;
                for (int k = 1; k < snake->length; k++) {
                    if (i == snake->body[k][1] && j == snake->body[k][0]) {
                        printf(" ");
                        isBodyPart = 1;
                        break;
                    }
                }
                if (!isBodyPart) printf("#");
            }
        }
        printf("\n");
    }

    for(int i = 0; i < rowOffset; i++) printf("\n");
}

struct winsize getTermSize() {
    struct winsize ws;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1) {
        return ws;
    } else {
        perror("Unable to get terminal size");
        exit(EXIT_FAILURE);
    }
}

void enableRawMode(struct termios *origTermios) {
    struct termios raw = *origTermios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDOUT_FILENO, TCSAFLUSH, &raw);
}

void disableRawMode(struct termios *origTermios) {
    tcsetattr(STDOUT_FILENO, TCSAFLUSH, origTermios);
}

void updateDirection(Snake *snake, char input) {
    switch (input) {
        case 'W': if (snake->dir != DOWN) snake->dir = UP; break;
        case 'S': if (snake->dir != UP) snake->dir = DOWN; break;
        case 'A': if (snake->dir != RIGHT) snake->dir = LEFT; break;
        case 'D': if (snake->dir != LEFT) snake->dir = RIGHT; break;
    }
}

void growSnake(Snake *snake) {
    snake->body[snake->length][0] = snake->body[snake->length - 1][0];
    snake->body[snake->length][1] = snake->body[snake->length - 1][1];
    snake->length++;
}

int moveSnake(Snake *snake, int *appleX, int *appleY) {
    switch (snake->dir) {
        case UP: snake->headY--; break;
        case DOWN: snake->headY++; break;
        case LEFT: snake->headX--; break;
        case RIGHT: snake->headX++; break;
    }

    if (snake->headX < 0) snake->headX = WIDTH - 1;
    else if (snake->headX >= WIDTH) snake->headX = 0;

    if (snake->headY < 0) snake->headY = HEIGHT - 1;
    else if (snake->headY >= HEIGHT) snake->headY = 0;

    for (int i = snake->length - 1; i > 0; i--) {
        snake->body[i][0] = snake->body[i - 1][0];
        snake->body[i][1] = snake->body[i - 1][1];
    }

    snake->body[0][0] = snake->headX;
    snake->body[0][1] = snake->headY;

    if (snake->headX == *appleX && snake->headY == *appleY) { 
        growSnake(snake);
        *appleX = rand() % WIDTH;
        *appleY = rand() % HEIGHT;
    }

    for (int i = 1; i < snake->length; i++) {
        if (snake->headX == snake->body[i][0] && snake->headY == snake->body[i][1])  return 1;
    }
    
    return 0;
}

void drawGameOver(int rowOffset, int colOffset) {
    printf("\033[H\033[J");
    for(int i = 0; i < rowOffset; i++) {
        printf("\n");
    }

    const char *msg = "GAME OVER";
    int msgLength = strlen(msg);
    int msgRow = HEIGHT / 2;
    int msgCol = (WIDTH - msgLength) / 2;

    for(int i = 0; i < HEIGHT; i++) {
        for(int j = 0; j < colOffset; j++) {
            printf(" ");
        }

        for (int j = 0; j < WIDTH; j++) {
            if (i == msgRow && j == msgCol) {
                printf("%s", msg);
                j += 8;
            } else {
                printf("#");
            }
        }
        printf("\n");
    }

    for(int i = 0; i < rowOffset; i++) printf("\n");
}


int main(void) {
    struct winsize ws = getTermSize();
    srand(time(NULL));

    int appleX = rand() % WIDTH;
    int appleY = rand() % HEIGHT;

    Snake snake;
    initializeSnake(&snake);

    int rowOffset = (ws.ws_row - HEIGHT) / 2;
    int colOffset = (ws.ws_col - WIDTH) / 2;

    printf("\033[32m");

    struct termios origTermios;
    tcgetattr(STDOUT_FILENO, &origTermios);
    enableRawMode(&origTermios);

    int origFlags = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, origFlags | O_NONBLOCK);

    char input;
    int startInput = 0;
    int gameOver = 0;
    while (1) {
        drawBoard(rowOffset, colOffset, appleX, appleY, &snake);

        if (read(STDOUT_FILENO, &input, 1) == 1) {
            input = toupper(input);

            if (input == 'Q') break;

            if (input == 'W' || input == 'A' || input == 'S' || input == 'D') {
                startInput = 1;
                updateDirection(&snake, input);
            }
        }

        if (startInput) gameOver = moveSnake(&snake, &appleX, &appleY);

        if (gameOver) { drawGameOver(rowOffset, colOffset); break; }

        usleep(200000);
    }

    printf("\033[0m");

    fcntl(STDIN_FILENO, F_SETFL, origFlags);
    disableRawMode(&origTermios);

    return 0;
}
