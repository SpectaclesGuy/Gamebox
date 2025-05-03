#include <GL/glut.h>
#include <stdlib.h>
#include <time.h>
#include <cstdio>
#include <vector>

namespace tetris{
// Game Constants
const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;
const int BLOCK_SIZE = 30;
const float GAME_SPEED_INIT = 500.0f; // milliseconds
const float GAME_SPEED_DECREASE = 0.9f; // speed multiplier per level

// Game variables
int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
int score = 0;
int level = 1;
int linesCleared = 0;
float gameSpeed = GAME_SPEED_INIT;
bool gameOver = false;

// Current piece variables
int currentPiece[4][4] = {0};
int currentPieceX = 0;
int currentPieceY = 0;
int currentPieceType = 0;
int currentRotation = 0;

// Timer variables
int lastFrameTime = 0;
int dropTime = 0;

// Define the tetrimino shapes
const int tetriminos[7][4][4] = {
    // I-piece
    {
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    // J-piece
    {
        {2, 0, 0, 0},
        {2, 2, 2, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    // L-piece
    {
        {0, 0, 3, 0},
        {3, 3, 3, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    // O-piece
    {
        {0, 4, 4, 0},
        {0, 4, 4, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    // S-piece
    {
        {0, 5, 5, 0},
        {5, 5, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    // T-piece
    {
        {0, 6, 0, 0},
        {6, 6, 6, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    // Z-piece
    {
        {7, 7, 0, 0},
        {0, 7, 7, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    }
};

// Colors for the pieces (R,G,B)
const float colors[8][3] = {
    {0.0f, 0.0f, 0.0f},    // Empty (Black)
    {0.0f, 1.0f, 1.0f},    // I-piece (Cyan)
    {0.0f, 0.0f, 1.0f},    // J-piece (Blue)
    {1.0f, 0.65f, 0.0f},   // L-piece (Orange)
    {1.0f, 1.0f, 0.0f},    // O-piece (Yellow)
    {0.0f, 1.0f, 0.0f},    // S-piece (Green)
    {0.8f, 0.0f, 0.8f},    // T-piece (Purple)
    {1.0f, 0.0f, 0.0f}     // Z-piece (Red)
};

// Function prototypes
void init();
void display();
void reshape(int width, int height);
void timer(int value);
void keyboard(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);
void createNewPiece();
void rotatePiece();
void drawBlock(int x, int y, int colorIndex);
void drawBoard();
void drawCurrentPiece();
void drawUI();
bool isValidMove(int pieceX, int pieceY, int pieceMatrix[4][4]);
void placePiece();
void checkLines();
void gameUpdate();

void launchTetris() {
    // Initialize random seed
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(600, 640);
    glutCreateWindow("Tetris");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(10, timer, 0);

    init();
}

void init() {
    // Set background color
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // Initialize game variables
    score = 0;
    level = 1;
    linesCleared = 0;
    gameSpeed = GAME_SPEED_INIT;
    gameOver = false;

    // Clear the board
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            board[i][j] = 0;
        }
    }

    // Create the first piece
    createNewPiece();

    // Initialize timing
    lastFrameTime = glutGet(GLUT_ELAPSED_TIME);
    dropTime = 0;
}

void createNewPiece() {
    // Choose a random tetrimino
    currentPieceType = rand() % 7;
    currentRotation = 0;

    // Reset position
    currentPieceX = BOARD_WIDTH / 2 - 2;
    currentPieceY = 0;

    // Copy the tetrimino into the current piece
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            currentPiece[i][j] = tetriminos[currentPieceType][i][j];
        }
    }

    // Check if the new piece overlaps with any existing blocks
    if (!isValidMove(currentPieceX, currentPieceY, currentPiece)) {
        gameOver = true;
    }
}

void rotatePiece() {
    int tempPiece[4][4] = {0};

    // Copy and rotate
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            tempPiece[j][3-i] = currentPiece[i][j];
        }
    }

    // Check if the rotated piece is valid
    if (isValidMove(currentPieceX, currentPieceY, tempPiece)) {
        // Apply rotation
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                currentPiece[i][j] = tempPiece[i][j];
            }
        }
        currentRotation = (currentRotation + 1) % 4;
    }
}

bool isValidMove(int pieceX, int pieceY, int pieceMatrix[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (pieceMatrix[i][j] != 0) {
                int boardX = pieceX + j;
                int boardY = pieceY + i;

                // Check boundaries
                if (boardX < 0 || boardX >= BOARD_WIDTH || boardY < 0 || boardY >= BOARD_HEIGHT) {
                    return false;
                }

                // Check collision with placed blocks
                if (board[boardY][boardX] != 0) {
                    return false;
                }
            }
        }
    }
    return true;
}

void placePiece() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (currentPiece[i][j] != 0) {
                int boardX = currentPieceX + j;
                int boardY = currentPieceY + i;

                // Place the piece on the board
                board[boardY][boardX] = currentPiece[i][j];
            }
        }
    }

    // Check for completed lines
    checkLines();

    // Create a new piece
    createNewPiece();
}

void checkLines() {
    int linesCompleted = 0;

    for (int i = 0; i < BOARD_HEIGHT; i++) {
        bool lineComplete = true;

        // Check if the line is complete
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j] == 0) {
                lineComplete = false;
                break;
            }
        }

        // If the line is complete, remove it and shift everything down
        if (lineComplete) {
            linesCompleted++;

            // Shift all lines above down by one
            for (int k = i; k > 0; k--) {
                for (int j = 0; j < BOARD_WIDTH; j++) {
                    board[k][j] = board[k-1][j];
                }
            }

            // Clear the top line
            for (int j = 0; j < BOARD_WIDTH; j++) {
                board[0][j] = 0;
            }
        }
    }

    // Update score and level
    if (linesCompleted > 0) {
        // Score formula: 100 * 2^(linesCompleted-1) * level
        score += 100 * (1 << (linesCompleted - 1)) * level;
        linesCleared += linesCompleted;

        // Level up every 10 lines
        int newLevel = 1 + (linesCleared / 10);
        if (newLevel > level) {
            level = newLevel;
            gameSpeed *= GAME_SPEED_DECREASE; // Make the game faster
        }
    }
}

void gameUpdate() {
    if (gameOver) return;

    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    int deltaTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;

    // Update drop timer
    dropTime += deltaTime;

    // Check if it's time to drop the piece
    if (dropTime >= gameSpeed) {
        dropTime = 0;

        // Try to move the piece down
        if (isValidMove(currentPieceX, currentPieceY + 1, currentPiece)) {
            currentPieceY++;
        } else {
            // If the piece can't move down, place it
            placePiece();
        }
    }
}

void drawBlock(int x, int y, int colorIndex) {
    float r = colors[colorIndex][0];
    float g = colors[colorIndex][1];
    float b = colors[colorIndex][2];

    // Fill
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2i(x * BLOCK_SIZE, y * BLOCK_SIZE);
    glVertex2i((x+1) * BLOCK_SIZE, y * BLOCK_SIZE);
    glVertex2i((x+1) * BLOCK_SIZE, (y+1) * BLOCK_SIZE);
    glVertex2i(x * BLOCK_SIZE, (y+1) * BLOCK_SIZE);
    glEnd();

    // Border
    glColor3f(r * 0.5f, g * 0.5f, b * 0.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(x * BLOCK_SIZE, y * BLOCK_SIZE);
    glVertex2i((x+1) * BLOCK_SIZE, y * BLOCK_SIZE);
    glVertex2i((x+1) * BLOCK_SIZE, (y+1) * BLOCK_SIZE);
    glVertex2i(x * BLOCK_SIZE, (y+1) * BLOCK_SIZE);
    glEnd();
}

void drawBoard() {
    // Draw the board background
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(BOARD_WIDTH * BLOCK_SIZE, 0);
    glVertex2i(BOARD_WIDTH * BLOCK_SIZE, BOARD_HEIGHT * BLOCK_SIZE);
    glVertex2i(0, BOARD_HEIGHT * BLOCK_SIZE);
    glEnd();

    // Draw the grid lines
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_LINES);
    for (int i = 0; i <= BOARD_WIDTH; i++) {
        glVertex2i(i * BLOCK_SIZE, 0);
        glVertex2i(i * BLOCK_SIZE, BOARD_HEIGHT * BLOCK_SIZE);
    }
    for (int i = 0; i <= BOARD_HEIGHT; i++) {
        glVertex2i(0, i * BLOCK_SIZE);
        glVertex2i(BOARD_WIDTH * BLOCK_SIZE, i * BLOCK_SIZE);
    }
    glEnd();

    // Draw the placed blocks
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j] != 0) {
                drawBlock(j, i, board[i][j]);
            }
        }
    }
}

void drawCurrentPiece() {
    if (gameOver) return;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (currentPiece[i][j] != 0) {
                drawBlock(currentPieceX + j, currentPieceY + i, currentPiece[i][j]);
            }
        }
    }
}

void drawUI() {
    // Draw UI panel background
    glColor3f(0.15f, 0.15f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2i(BOARD_WIDTH * BLOCK_SIZE, 0);
    glVertex2i(BOARD_WIDTH * BLOCK_SIZE + 200, 0);
    glVertex2i(BOARD_WIDTH * BLOCK_SIZE + 200, BOARD_HEIGHT * BLOCK_SIZE);
    glVertex2i(BOARD_WIDTH * BLOCK_SIZE, BOARD_HEIGHT * BLOCK_SIZE);
    glEnd();

    // Draw UI border
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(BOARD_WIDTH * BLOCK_SIZE, 0);
    glVertex2i(BOARD_WIDTH * BLOCK_SIZE + 200, 0);
    glVertex2i(BOARD_WIDTH * BLOCK_SIZE + 200, BOARD_HEIGHT * BLOCK_SIZE);
    glVertex2i(BOARD_WIDTH * BLOCK_SIZE, BOARD_HEIGHT * BLOCK_SIZE);
    glEnd();

    // Draw UI text
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2i(BOARD_WIDTH * BLOCK_SIZE + 20, 50);

    // Display score
    char scoreText[50];
    sprintf(scoreText, "Score: %d", score);
    for (char* c = scoreText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Display level
    glRasterPos2i(BOARD_WIDTH * BLOCK_SIZE + 20, 80);
    char levelText[50];
    sprintf(levelText, "Level: %d", level);
    for (char* c = levelText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Display lines cleared
    glRasterPos2i(BOARD_WIDTH * BLOCK_SIZE + 20, 110);
    char linesText[50];
    sprintf(linesText, "Lines: %d", linesCleared);
    for (char* c = linesText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Display controls
    glRasterPos2i(BOARD_WIDTH * BLOCK_SIZE + 20, 160);
    const char* controlsText = "Controls:";
    for (const char* c = controlsText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    glRasterPos2i(BOARD_WIDTH * BLOCK_SIZE + 20, 180);
    const char* controlsText1 = "Left/Right - Move";
    for (const char* c = controlsText1; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    glRasterPos2i(BOARD_WIDTH * BLOCK_SIZE + 20, 200);
    const char* controlsText2 = "Up - Rotate";
    for (const char* c = controlsText2; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    glRasterPos2i(BOARD_WIDTH * BLOCK_SIZE + 20, 220);
    const char* controlsText3 = "Down - Soft Drop";
    for (const char* c = controlsText3; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    glRasterPos2i(BOARD_WIDTH * BLOCK_SIZE + 20, 240);
    const char* controlsText4 = "Space - Hard Drop";
    for (const char* c = controlsText4; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    glRasterPos2i(BOARD_WIDTH * BLOCK_SIZE + 20, 260);
    const char* controlsText5 = "R - Restart Game";
    for (const char* c = controlsText5; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // Display game over message if game is over
    if (gameOver) {
        glColor3f(1.0f, 0.0f, 0.0f);
        glRasterPos2i(BOARD_WIDTH * BLOCK_SIZE + 20, 320);
        const char* gameOverText = "GAME OVER";
        for (const char* c = gameOverText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
    }
}

void display() {
    // Update game state
    gameUpdate();

    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw game elements
    drawBoard();
    drawCurrentPiece();
    drawUI();

    // Swap buffers
    glutSwapBuffers();
}

void reshape(int width, int height) {
    // Set the viewport
    glViewport(0, 0, width, height);

    // Set the projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);

    // Set the modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void timer(int value) {
    // Redisplay and set the next timer
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // ~60 FPS
}

void keyboard(unsigned char key, int x, int y) {
    if (gameOver && key != 'r' && key != 'R') return;

    switch (key) {
        case ' ':
            // Hard drop - Move the piece all the way down
            while (isValidMove(currentPieceX, currentPieceY + 1, currentPiece)) {
                currentPieceY++;
            }
            placePiece();
            break;
        case 'r':
        case 'R':
            // Restart the game
            init();
            break;
        case 'q':
        case 'Q':
        case 27: // ESC
            // Quit the game
            exit(0);
            break;
    }

    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    if (gameOver) return;

    switch (key) {
        case GLUT_KEY_LEFT:
            // Move left
            if (isValidMove(currentPieceX - 1, currentPieceY, currentPiece)) {
                currentPieceX--;
            }
            break;
        case GLUT_KEY_RIGHT:
            // Move right
            if (isValidMove(currentPieceX + 1, currentPieceY, currentPiece)) {
                currentPieceX++;
            }
            break;
        case GLUT_KEY_UP:
            // Rotate
            rotatePiece();
            break;
        case GLUT_KEY_DOWN:
            // Soft drop - Move down faster
            if (isValidMove(currentPieceX, currentPieceY + 1, currentPiece)) {
                currentPieceY++;
            }
            break;
    }

    glutPostRedisplay();
}
}

void launchTetris() {
    tetris::launchTetris();
}