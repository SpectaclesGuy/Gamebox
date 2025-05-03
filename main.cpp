#include <GL/glut.h>
#include <iostream>

// Function declarations from game files
void launchTetris();
void launchShooter();
void launchMines();


// Current screen state
enum ScreenState { MENU, GAME1, GAME2, GAME3 };
ScreenState currentScreen = MENU;

void displayMenu() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1, 1, 1);

    glRasterPos2f(100, 150);
    const char* title = "Select a Game:";
    for (const char* c = title; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    const char* options[] = {
        "1. Tetris",
        "2. Space Shooter",
        "3. Minesweeper",
        "ESC - Exit"
    };
    int y = 180;
    for (const char* line : options) {
        glRasterPos2f(100, y);
        for (const char* c = line; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        y += 30;
    }

    glutSwapBuffers();
}

void displayCallback() {
    if (currentScreen == MENU) displayMenu();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: exit(0); break; // ESC
        case '1': currentScreen = GAME1; launchTetris(); break;
        case '2': currentScreen = GAME2; launchShooter(); break;
        case '3': currentScreen = GAME3; launchMines(); break;
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(600, 400);
    glutCreateWindow("Game Emulator");

    glClearColor(0, 0, 0, 1);

    glutDisplayFunc(displayCallback);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);
    glutMainLoop();
    return 0;
}
