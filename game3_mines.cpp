#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>

namespace mines{
// Game settings
#define GRID_WIDTH 10
#define GRID_HEIGHT 10
#define MINE_COUNT 15
#define CELL_SIZE 50
#define WINDOW_WIDTH (GRID_WIDTH * CELL_SIZE)
#define WINDOW_HEIGHT ((GRID_HEIGHT * CELL_SIZE) + 50) // Extra height for status bar

// Game state
typedef enum {
    PLAYING,
    WON,
    LOST
} GameState;

// Cell states
typedef struct {
    int hasMine;
    int revealed;
    int flagged;
    int neighborMines;
} Cell;

// Game variables
Cell grid[GRID_WIDTH][GRID_HEIGHT];
GameState gameState = PLAYING;
int remainingCells;
int flagsUsed = 0;
int gameTime = 0;
int lastUpdateTime = 0;

// Function prototypes
void initializeGame();
void drawGrid();
void drawStatusBar();
void drawCell(int x, int y);
void reveal(int x, int y);
int countNeighborMines(int x, int y);
void checkWinCondition();
void display();
void reshape(int width, int height);
void keyboard(unsigned char key, int x, int y);
void mouseClick(int button, int state, int x, int y);
void timer(int value);
void drawText(const char* text, float x, float y);

// Initialize game
void initializeGame() {
    int i, j, minesPlaced = 0;

    // Reset all cells
    for (i = 0; i < GRID_WIDTH; i++) {
        for (j = 0; j < GRID_HEIGHT; j++) {
            grid[i][j].hasMine = 0;
            grid[i][j].revealed = 0;
            grid[i][j].flagged = 0;
            grid[i][j].neighborMines = 0;
        }
    }

    // Place mines randomly
    srand(time(NULL));
    while (minesPlaced < MINE_COUNT) {
        int x = rand() % GRID_WIDTH;
        int y = rand() % GRID_HEIGHT;

        if (!grid[x][y].hasMine) {
            grid[x][y].hasMine = 1;
            minesPlaced++;
        }
    }

    // Calculate neighbor mines
    for (i = 0; i < GRID_WIDTH; i++) {
        for (j = 0; j < GRID_HEIGHT; j++) {
            grid[i][j].neighborMines = countNeighborMines(i, j);
        }
    }

    // Set initial game state
    gameState = PLAYING;
    remainingCells = GRID_WIDTH * GRID_HEIGHT - MINE_COUNT;
    flagsUsed = 0;
    gameTime = 0;
    lastUpdateTime = glutGet(GLUT_ELAPSED_TIME) / 1000;
}

// Count mines in the 8 neighboring cells
int countNeighborMines(int x, int y) {
    int count = 0;
    int i, j;

    for (i = -1; i <= 1; i++) {
        for (j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;

            int nx = x + i;
            int ny = y + j;

            if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                if (grid[nx][ny].hasMine) {
                    count++;
                }
            }
        }
    }

    return count;
}

// Reveal a cell and its neighbors if empty
void reveal(int x, int y) {
    // Check if valid coordinates and cell not already revealed or flagged
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT ||
        grid[x][y].revealed || grid[x][y].flagged) {
        return;
    }

    // Reveal cell
    grid[x][y].revealed = 1;

    // If it's a mine, game over
    if (grid[x][y].hasMine) {
        gameState = LOST;
        return;
    }

    // Decrease remaining cells count
    remainingCells--;

    // Auto-reveal neighbors if cell is empty
    if (grid[x][y].neighborMines == 0) {
        int i, j;
        for (i = -1; i <= 1; i++) {
            for (j = -1; j <= 1; j++) {
                reveal(x + i, y + j);
            }
        }
    }

    // Check if player has won
    checkWinCondition();
}

// Check if all non-mine cells have been revealed
void checkWinCondition() {
    if (remainingCells == 0) {
        gameState = WON;
    }
}

// Draw text on screen
void drawText(const char* text, float x, float y) {
    glRasterPos2f(x, y);
    int len = strlen(text);
    for (int i = 0; i < len; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, text[i]);
    }
}

// Draw a cell
void drawCell(int x, int y) {
    float cellX = x * CELL_SIZE;
    float cellY = y * CELL_SIZE;

    // Draw cell background
    if (grid[x][y].revealed) {
        if (grid[x][y].hasMine) {
            glColor3f(1.0f, 0.0f, 0.0f); // Red for mines
        } else {
            glColor3f(0.8f, 0.8f, 0.8f); // Light gray for revealed cells
        }
    } else {
        glColor3f(0.6f, 0.6f, 0.6f); // Dark gray for unrevealed cells
    }

    glBegin(GL_QUADS);
    glVertex2f(cellX, cellY);
    glVertex2f(cellX + CELL_SIZE, cellY);
    glVertex2f(cellX + CELL_SIZE, cellY + CELL_SIZE);
    glVertex2f(cellX, cellY + CELL_SIZE);
    glEnd();

    // Draw cell border
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cellX, cellY);
    glVertex2f(cellX + CELL_SIZE, cellY);
    glVertex2f(cellX + CELL_SIZE, cellY + CELL_SIZE);
    glVertex2f(cellX, cellY + CELL_SIZE);
    glEnd();

    // Draw cell content
    if (grid[x][y].revealed) {
        if (grid[x][y].hasMine) {
            // Draw mine
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_POLYGON);
            float cx = cellX + CELL_SIZE / 2;
            float cy = cellY + CELL_SIZE / 2;
            float radius = CELL_SIZE / 4;
            for (int i = 0; i < 360; i += 30) {
                float angle = i * 3.14159 / 180;
                glVertex2f(cx + radius * cos(angle), cy + radius * sin(angle));
            }
            glEnd();
        } else if (grid[x][y].neighborMines > 0) {
            // Draw number of neighboring mines
            switch (grid[x][y].neighborMines) {
                case 1: glColor3f(0.0f, 0.0f, 1.0f); break; // Blue
                case 2: glColor3f(0.0f, 0.5f, 0.0f); break; // Green
                case 3: glColor3f(1.0f, 0.0f, 0.0f); break; // Red
                case 4: glColor3f(0.0f, 0.0f, 0.5f); break; // Dark Blue
                case 5: glColor3f(0.5f, 0.0f, 0.0f); break; // Dark Red
                case 6: glColor3f(0.0f, 0.5f, 0.5f); break; // Cyan
                case 7: glColor3f(0.0f, 0.0f, 0.0f); break; // Black
                case 8: glColor3f(0.5f, 0.5f, 0.5f); break; // Gray
            }
            char number[2];
            sprintf(number, "%d", grid[x][y].neighborMines);
            drawText(number, cellX + CELL_SIZE/2 - 4, cellY + CELL_SIZE/2 + 5);
        }
    } else if (grid[x][y].flagged) {
        // Draw flag
        glColor3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_TRIANGLES);
        glVertex2f(cellX + CELL_SIZE / 2, cellY + CELL_SIZE / 4);
        glVertex2f(cellX + CELL_SIZE / 2, cellY + CELL_SIZE * 3 / 4);
        glVertex2f(cellX + CELL_SIZE * 3 / 4, cellY + CELL_SIZE / 2);
        glEnd();

        // Flag pole
        glColor3f(0.0f, 0.0f, 0.0f);
        glBegin(GL_LINES);
        glVertex2f(cellX + CELL_SIZE / 2, cellY + CELL_SIZE / 4);
        glVertex2f(cellX + CELL_SIZE / 2, cellY + CELL_SIZE * 3 / 4);
        glEnd();
    }
}

// Draw the game grid
void drawGrid() {
    int i, j;

    for (i = 0; i < GRID_WIDTH; i++) {
        for (j = 0; j < GRID_HEIGHT; j++) {
            drawCell(i, j);
        }
    }

    // In case of game over, reveal all mines
    if (gameState == LOST) {
        for (i = 0; i < GRID_WIDTH; i++) {
            for (j = 0; j < GRID_HEIGHT; j++) {
                if (grid[i][j].hasMine && !grid[i][j].revealed) {
                    grid[i][j].revealed = 1;
                    drawCell(i, j);
                }
            }
        }
    }
}

// Draw status bar
void drawStatusBar() {
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(0, GRID_HEIGHT * CELL_SIZE);
    glVertex2f(GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE);
    glVertex2f(GRID_WIDTH * CELL_SIZE, WINDOW_HEIGHT);
    glVertex2f(0, WINDOW_HEIGHT);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);

    // Display mines/flags info
    char flagText[50];
    sprintf(flagText, "Flags: %d/%d", flagsUsed, MINE_COUNT);
    drawText(flagText, 10, GRID_HEIGHT * CELL_SIZE + 30);

    // Display time
    char timeText[50];
    sprintf(timeText, "Time: %d sec", gameTime);
    drawText(timeText, WINDOW_WIDTH - 100, GRID_HEIGHT * CELL_SIZE + 30);

    // Display game status
    char statusText[50];
    if (gameState == PLAYING) {
        strcpy(statusText, "Game in progress");
    } else if (gameState == WON) {
        strcpy(statusText, "You won! Press R to restart");
    } else {
        strcpy(statusText, "Game over! Press R to restart");
    }

    drawText(statusText, (WINDOW_WIDTH / 2) - 70, GRID_HEIGHT * CELL_SIZE + 30);
}

// Display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawGrid();
    drawStatusBar();
    glutSwapBuffers();
}

// Reshape function
void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}

// Keyboard function
void keyboard(unsigned char key, int x, int y) {
    if (key == 'r' || key == 'R') {
        initializeGame();
        glutPostRedisplay();
    } else if (key == 27) { // ESC key
        exit(0);
    }
}

// Mouse function
void mouseClick(int button, int state, int x, int y) {
    if (state != GLUT_DOWN || gameState != PLAYING) return;

    // Convert mouse coordinates to grid coordinates
    int gridX = x / CELL_SIZE;
    int gridY = y / CELL_SIZE;

    // Make sure click is within grid
    if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
        if (button == GLUT_LEFT_BUTTON) {
            // Left click - reveal cell
            if (!grid[gridX][gridY].flagged) {
                reveal(gridX, gridY);
            }
        } else if (button == GLUT_RIGHT_BUTTON) {
            // Right click - toggle flag
            if (!grid[gridX][gridY].revealed) {
                if (grid[gridX][gridY].flagged) {
                    grid[gridX][gridY].flagged = 0;
                    flagsUsed--;
                } else if (flagsUsed < MINE_COUNT) {
                    grid[gridX][gridY].flagged = 1;
                    flagsUsed++;
                }
            }
        }

        glutPostRedisplay();
    }
}

// Timer function
void timer(int value) {
    if (gameState == PLAYING) {
        // Update game time
        int currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000;
        if (currentTime > lastUpdateTime) {
            gameTime = currentTime - lastUpdateTime;
            glutPostRedisplay();
        }
    }

    glutTimerFunc(1000, timer, 0);
}

// Main function
void launchMines() {
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);  // Use your defined window size
    glutCreateWindow("Minesweeper");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);  // Optional background color

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouseClick);
    // glutTimerFunc(1000, timer, 0);        // For timed logic if used

    initializeGame();  // or init(), based on your function
}
}

void launchMines() {
    mines::launchMines();
}
