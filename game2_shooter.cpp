// SpaceShooter.cpp - Classic Fixed-Shooter Style with 2-Bullet Limit
// Compile with: g++ spaceShooter.cpp -o spaceShooterClassic2Shot -lGL -lGLU -lglut -lm

#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector> // Ensure vector is included

namespace shooter{
// --- Game Constants ---
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float PLAYER_SIZE = 30.0f;
const float PLAYER_BULLET_WIDTH = 3.0f;
const float PLAYER_BULLET_HEIGHT = 10.0f;
const float ENEMY_BULLET_SIZE = 6.0f;
const float ENEMY_SIZE = 25.0f;
const float PLAYER_SPEED = 7.0f;
const float PLAYER_BULLET_SPEED = 11.0f;
const int MAX_STARS = 200;
const int MAX_PARTICLES = 200;
const float PARTICLE_LIFESPAN = 0.5f;
const int MAX_ACTIVE_PLAYER_BULLETS = 2; // Limit player bullets on screen

// --- Formation & Wave Settings ---
const int ENEMY_GRID_ROWS = 4;
const int ENEMY_GRID_COLS = 8;
const int MAX_ENEMIES_PER_WAVE = ENEMY_GRID_ROWS * ENEMY_GRID_COLS;
const float ENEMY_GRID_SPACING_X = ENEMY_SIZE * 1.8f;
const float ENEMY_GRID_SPACING_Y = ENEMY_SIZE * 1.5f;
const float FORMATION_START_Y = WINDOW_HEIGHT - 80.0f;
const float FORMATION_START_X = 80.0f;
const float FORMATION_STEP_DOWN = ENEMY_SIZE * 0.6f;
const float FORMATION_EDGE_PADDING = 20.0f;
const float GAME_OVER_LINE_Y = PLAYER_SIZE + 30.0f;

// --- Level and Difficulty Settings ---
const int MAX_LEVELS = 7;
int gameLevel = 1;
int currentWave = 1;

// Difficulty parameters per level (Index corresponds to level number)
const float formationMoveIntervalPerLevel[MAX_LEVELS + 1] =
    {0.0f, 1.0f, 0.85f, 0.70f, 0.60f, 0.50f, 0.40f, 0.30f};
const float bulletSpeedPerLevel[MAX_LEVELS + 1] =
    {0.0f, 4.0f, 4.5f, 5.0f, 5.5f, 6.0f, 6.5f, 7.0f};
const int enemyShootProbPerLevel[MAX_LEVELS + 1] =
    {0, 10, 15, 20, 25, 30, 35, 40};

// --- Game State Variables ---

// Base structure for game objects
struct GameObject {
    float x, y;
    float dx, dy;
    float r, g, b;
    bool active;
    float shootCooldown;
    int gridRow, gridCol; // Store original position in grid

    GameObject() :
        x(0), y(0), dx(0), dy(0), r(1), g(1), b(1),
        active(false), shootCooldown(0.0f), gridRow(-1), gridCol(-1) {}

    GameObject(float startX, float startY) :
        x(startX), y(startY), dx(0), dy(0), r(1), g(1), b(1),
        active(true), shootCooldown(1.0f + (rand() % 100) / 100.0f), gridRow(-1), gridCol(-1) {}
};

// Player specific structure
struct Player : GameObject {
    int score;
    int lives;
    // No ammo field needed anymore

    Player() : GameObject(), score(0), lives(3) {}

    Player(float startX, float startY) : GameObject(startX, startY), score(0), lives(3) {
        r = 0.0f;
        g = 1.0f;
        b = 0.0f;
    }
};

// Particle structure for effects
struct Particle : GameObject {
    float life;

    Particle() : GameObject(), life(0.0f) {}
};

// Star structure for background
struct Star {
    float x, y, z;
    float brightness;
};

// Global game state instances
Player player;
std::vector<GameObject> playerBullets;
std::vector<GameObject> enemies; // Will hold the current wave's enemies
std::vector<GameObject> enemyBullets;
std::vector<Star> stars;
std::vector<Particle> particles;

// Wave & Formation State
bool waveActive = false;
int enemiesRemainingInWave = 0;
float formationBaseX = FORMATION_START_X;
float formationBaseY = FORMATION_START_Y;
float formationDirection = 1.0f; // 1.0 for right, -1.0 for left
float formationMoveTimer = 0.0f;
bool moveDownNext = false; // Flag to move down on the next sideways move

// Current Difficulty Settings (based on gameLevel)
float currentFormationMoveInterval = formationMoveIntervalPerLevel[1];
float currentEnemyBulletSpeed = bulletSpeedPerLevel[1];
int currentEnemyShootProb = enemyShootProbPerLevel[1];

// Other global state
int frameCount = 0;
bool gameOver = false;
std::string gameOverReason = "";
float lastFrameTime = 0.0f;

// --- Function Prototypes ---
void init();
void display();
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);
void specialKeysUp(int key, int x, int y);
void update();
void updateFormation(float dt);
void spawnWave(int waveNum);
void drawPlayer();
void drawPlayerBullet(const GameObject& bullet);
void drawEnemyBullet(const GameObject& bullet);
void drawEnemy(const GameObject& enemy);
void drawStars();
void drawParticles();
void drawText(float x, float y, const std::string& text, void* font = GLUT_BITMAP_HELVETICA_18);
void firePlayerBullet();
void fireEnemyBullet(const GameObject& enemy);
void spawnExplosion(float x, float y, float r, float g, float b);
void updateDifficulty();
bool checkCollision(const GameObject& obj1, float size1_x, float size1_y, const GameObject& obj2, float size2_x, float size2_y);
float getDeltaTime();
void resetFormationPosition();


// --- Main Function ---
void launchShooter() {
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);  // Use your defined window size
    glutCreateWindow("Space Shooter");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);         // If used
    glutSpecialUpFunc(specialKeysUp);     // If used
    glutIdleFunc(update);                 // If you use animation

    init();
    lastFrameTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
}

// --- Initialization ---
void init() {
    glClearColor(0.0f, 0.0f, 0.05f, 1.0f); // Dark background

    player = Player(WINDOW_WIDTH / 2.0f, PLAYER_SIZE * 1.5f); // Create/reset player

    // Initialize stars
    stars.resize(MAX_STARS);
    for (auto& star : stars) {
        star.x = static_cast<float>(rand() % WINDOW_WIDTH);
        star.y = static_cast<float>(rand() % WINDOW_HEIGHT);
        star.z = static_cast<float>(rand() % 100) / 100.0f;
        star.brightness = 0.5f + static_cast<float>(rand() % 50) / 100.0f;
    }

    // Reserve memory for vectors to avoid frequent reallocations
    playerBullets.reserve(100);
    enemies.resize(MAX_ENEMIES_PER_WAVE); // Resize to max wave size
    for(auto& e : enemies) {
        e.active = false; // Ensure all slots are initially inactive
    }
    enemyBullets.reserve(150);
    particles.reserve(MAX_PARTICLES);

    // Clear dynamic elements from previous game (if any)
    playerBullets.clear();
    enemyBullets.clear();
    particles.clear();

    // --- Reset Game State ---
    gameOver = false;
    gameOverReason = "";
    currentWave = 1; // Start at wave 1
    gameLevel = 1;   // Reset game level (controls difficulty)
    waveActive = false; // Start with no active wave (will be spawned in update)
    enemiesRemainingInWave = 0;
    resetFormationPosition(); // Reset formation position and direction

    updateDifficulty(); // Apply level 1 settings

    frameCount = 0;
    // Player score and lives are reset when 'player' object is recreated above

    std::cout << "Game Initialized" << std::endl;
}

// Resets the formation's starting position and state
void resetFormationPosition() {
    formationBaseX = FORMATION_START_X;
    formationBaseY = FORMATION_START_Y;
    formationDirection = 1.0f; // Start moving right
    formationMoveTimer = 0.0f;
    moveDownNext = false;
}

// --- Delta Time Calculation ---
// Calculates time elapsed since the last frame for smooth, frame-rate independent movement
float getDeltaTime() {
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // Time in seconds
    float dt = currentTime - lastFrameTime;
    lastFrameTime = currentTime;
    // Clamp delta time to avoid unusually large jumps if frame rate stalls
    return std::min(dt, 0.1f);
}

// --- Display Function ---
// Clears the screen and redraws all game elements
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawStars(); // Draw background first

    if (!gameOver) {
        // Draw active game elements
        drawPlayer();
        for (const auto& bullet : playerBullets) {
            if (bullet.active) {
                drawPlayerBullet(bullet);
            }
        }
        for (const auto& bullet : enemyBullets) {
            if (bullet.active) {
                drawEnemyBullet(bullet);
            }
        }
        for (const auto& enemy : enemies) {
            if (enemy.active) {
                drawEnemy(enemy);
            }
        }
        drawParticles(); // Draw explosions/effects

        // Draw UI Text
        drawText(10, WINDOW_HEIGHT - 30, "Score: " + std::to_string(player.score));
        drawText(WINDOW_WIDTH - 150, WINDOW_HEIGHT - 30, "Lives: " + std::to_string(player.lives));
        drawText(WINDOW_WIDTH / 2.0f - 50, WINDOW_HEIGHT - 30, "Wave: " + std::to_string(currentWave));
        // No ammo display

    } else {
        // Game Over Screen
        drawText(WINDOW_WIDTH / 2.0f - 120, WINDOW_HEIGHT / 2.0f + 60, "G A M E   O V E R", GLUT_BITMAP_TIMES_ROMAN_24);
        drawText(WINDOW_WIDTH / 2.0f - 120, WINDOW_HEIGHT / 2.0f + 20, gameOverReason);
        drawText(WINDOW_WIDTH / 2.0f - 100, WINDOW_HEIGHT / 2.0f - 20, "Final Score: " + std::to_string(player.score));
        drawText(WINDOW_WIDTH / 2.0f - 100, WINDOW_HEIGHT / 2.0f - 50, "Final Wave: " + std::to_string(currentWave));
        drawText(WINDOW_WIDTH / 2.0f - 110, WINDOW_HEIGHT / 2.0f - 90, "Press 'R' to Restart");
        drawParticles(); // Draw lingering particles
    }

    glutSwapBuffers(); // Display the drawn frame
}

// --- Reshape Function ---
// Handles window resizing
void reshape(int w, int h) {
    if (h == 0) { // Prevent division by zero
        h = 1;
    }
    glViewport(0, 0, w, h); // Set the viewport to the new window size
    glMatrixMode(GL_PROJECTION); // Set the projection matrix
    glLoadIdentity();            // Reset the projection matrix
    gluOrtho2D(0, w, 0, h);      // Set orthographic projection matching window dimensions
    glMatrixMode(GL_MODELVIEW);  // Set the modelview matrix
    glLoadIdentity();            // Reset the modelview matrix
}

// --- Input Handling ---

// Handles standard keyboard key presses
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: // ESC key
            exit(0);
            break;
        case ' ': // Space bar
            if (!gameOver) {
                firePlayerBullet();
            }
            break;
        case 'r': // R key
        case 'R': // Shift + R
            if (gameOver) {
                init(); // Restart game if over
            }
            break;
    }
}

// Handles special key presses (arrow keys, function keys, etc.)
void specialKeys(int key, int x, int y) {
    if (gameOver) {
        return;
    }
    switch (key) {
        case GLUT_KEY_LEFT:
            player.dx = -PLAYER_SPEED;
            break;
        case GLUT_KEY_RIGHT:
            player.dx = PLAYER_SPEED;
            break;
        case GLUT_KEY_UP:
            player.dy = PLAYER_SPEED; // Enable up movement
            break;
        case GLUT_KEY_DOWN:
            player.dy = -PLAYER_SPEED; // Enable down movement
            break;
    }
}

// Handles special key releases
void specialKeysUp(int key, int x, int y) {
    if (gameOver) {
        return;
    }
    switch (key) {
        case GLUT_KEY_LEFT:
            if (player.dx < 0) {
                player.dx = 0; // Stop moving left only if currently moving left
            }
            break;
        case GLUT_KEY_RIGHT:
            if (player.dx > 0) {
                player.dx = 0; // Stop moving right only if currently moving right
            }
            break;
        case GLUT_KEY_UP:
            if (player.dy > 0) {
                player.dy = 0; // Stop moving up only if currently moving up
            }
            break;
        case GLUT_KEY_DOWN:
            if (player.dy < 0) {
                player.dy = 0; // Stop moving down only if currently moving down
            }
            break;
    }
}

// --- Game Update Logic ---
// Called continuously by glutIdleFunc
void update() {
    float dt = getDeltaTime(); // Time since last update

    // If game is over, only update particles for lingering effects
    if (gameOver) {
        for (auto it = particles.begin(); it != particles.end(); ++it) {
            if (it->active) {
                it->x += it->dx * dt * 60.0f;
                it->y += it->dy * dt * 60.0f;
                it->life -= dt;
                if (it->life <= 0) {
                    it->active = false;
                }
            }
        }
        glutPostRedisplay(); // Keep redrawing game over screen
        return;              // Skip rest of update logic
    }

    frameCount++;

    // --- Wave Management ---
    // Check if a new wave needs to be started
    if (!waveActive) {
        if (enemiesRemainingInWave <= 0) { // Ensure previous wave is fully cleared
            std::cout << "Starting Wave " << currentWave << std::endl;
            updateDifficulty(); // Set difficulty for the new wave
            spawnWave(currentWave); // Create the enemies
            waveActive = true;
        }
    }

    // --- Player Update ---
    player.x += player.dx * dt * 60.0f; // Scale movement by time
    player.y += player.dy * dt * 60.0f;
    // Clamp player position within screen bounds (X) and lower half (Y)
    player.x = std::max(PLAYER_SIZE / 2.0f, std::min(player.x, WINDOW_WIDTH - PLAYER_SIZE / 2.0f));
    player.y = std::max(PLAYER_SIZE / 2.0f, std::min(player.y, WINDOW_HEIGHT / 2.0f - PLAYER_SIZE / 2.0f));

    // --- Player Bullet Update ---
    // Move bullets and deactivate if they go off-screen
    for (auto& bullet : playerBullets) {
        if (bullet.active) {
            bullet.y += bullet.dy * dt * 60.0f; // dy is positive (PLAYER_BULLET_SPEED)
            if (bullet.y > WINDOW_HEIGHT + PLAYER_BULLET_HEIGHT) {
                bullet.active = false;
            }
        }
    }

    // --- Enemy Bullet Update & Player Collision ---
    // Move enemy bullets, check off-screen, check collision with player
    for (auto& bullet : enemyBullets) {
        if (bullet.active) {
            bullet.y += bullet.dy * dt * 60.0f; // dy is negative (ENEMY_BULLET_SPEED)
            if (bullet.y < -ENEMY_BULLET_SIZE) { // Check bottom edge
                bullet.active = false;
            }

            // Check collision with player
            if (checkCollision(player, PLAYER_SIZE, PLAYER_SIZE, bullet, ENEMY_BULLET_SIZE, ENEMY_BULLET_SIZE)) {
                bullet.active = false; // Deactivate bullet
                spawnExplosion(player.x, player.y, 1.0f, 1.0f, 0.0f); // Player hit explosion
                player.lives--;
                if (player.lives <= 0) {
                    gameOver = true; // Set game over state
                    gameOverReason = "Your ship was destroyed!";
                    // Exit loop immediately if game over
                    break;
                }
            }
        }
    }
    // Check if game ended due to player hit
    if (gameOver) {
        glutPostRedisplay();
        return;
    }

    // --- Update Formation ---
    // Move the enemy formation if a wave is active
    if (waveActive) {
        updateFormation(dt);
        // Check if formation descended too low
        if (formationBaseY - (ENEMY_GRID_ROWS - 1) * ENEMY_GRID_SPACING_Y < GAME_OVER_LINE_Y) {
            gameOver = true;
            gameOverReason = "The invaders reached the defence line!";
        }
    }
    // Check if game ended due to formation reaching bottom
    if (gameOver) {
        glutPostRedisplay();
        return;
    }


    // --- Enemy Logic (Shooting, Collision with Player Bullets) ---
    int currentEnemiesRemaining = 0; // Count active enemies this frame
    for (auto& enemy : enemies) {
        if (enemy.active) {
            currentEnemiesRemaining++;

            // --- Enemy Shooting ---
            enemy.shootCooldown -= dt;
            if (enemy.shootCooldown <= 0) {
                // Check probability based on current difficulty
                if ((rand() % 1000) < currentEnemyShootProb) {
                    fireEnemyBullet(enemy);
                    // Reset cooldown (potentially varies with level or remaining enemies)
                    float baseCooldown = (gameLevel < 4) ? 1.2f : 0.9f;
                    enemy.shootCooldown = baseCooldown + static_cast<float>(rand() % 50) / 100.0f / fmax(1.0f, (float)currentEnemiesRemaining / 5.0f);
                } else {
                    // Small delay even if not shooting, prevents checking every single frame
                    enemy.shootCooldown = 0.1f;
                }
            }

            // --- Player Bullet-Enemy Collision ---
            for (auto& playerBullet : playerBullets) {
                if (playerBullet.active && checkCollision(playerBullet, PLAYER_BULLET_WIDTH, PLAYER_BULLET_HEIGHT, enemy, ENEMY_SIZE, ENEMY_SIZE)) {
                    playerBullet.active = false; // Deactivate player bullet
                    enemy.active = false;       // Deactivate enemy
                    spawnExplosion(enemy.x, enemy.y, enemy.r, enemy.g, enemy.b); // Enemy explosion
                    player.score += 10 * gameLevel; // Award score based on level
                    currentEnemiesRemaining--;    // Decrement frame's remaining count
                    break; // Bullet hits only one enemy
                }
            }
        }
    } // End enemy processing loop

    // Update the global count after checking all enemies
    enemiesRemainingInWave = currentEnemiesRemaining;

    // --- Check for Wave End ---
    if (waveActive && enemiesRemainingInWave <= 0) {
        std::cout << "Wave " << currentWave << " Cleared!" << std::endl;
        currentWave++;
        // Simple level progression: Level increases with wave number, capped at MAX_LEVELS
        gameLevel = std::min(MAX_LEVELS, currentWave);
        waveActive = false; // Mark wave inactive so next one can start
        playerBullets.clear(); // Clear leftover bullets
        enemyBullets.clear();
        resetFormationPosition(); // Reset formation for next wave
        // No ammo replenishment needed
    }


    // --- Particle Update ---
    // Update position and lifespan of explosion particles
    for (auto it = particles.begin(); it != particles.end(); ++it) {
        if (it->active) {
            it->x += it->dx * dt * 60.0f;
            it->y += it->dy * dt * 60.0f;
            it->life -= dt;
            if (it->life <= 0) {
                it->active = false;
            }
        }
    }

    glutPostRedisplay(); // Request redraw
}


// --- Update Formation Movement ---
// Handles the synchronized movement of the enemy grid
void updateFormation(float dt) {
    formationMoveTimer -= dt; // Countdown to next move

    if (formationMoveTimer <= 0.0f) {
        formationMoveTimer = currentFormationMoveInterval; // Reset timer based on difficulty

        // Calculate formation bounds and step size
        float formationWidth = (ENEMY_GRID_COLS - 1) * ENEMY_GRID_SPACING_X;
        float stepSize = ENEMY_SIZE * 0.25f; // Adjust for desired horizontal speed feel
        float leftmostX = formationBaseX;
        float rightmostX = formationBaseX + formationWidth;

        // Check if the formation hits the screen edges
        bool hitEdge = false;
        if (formationDirection > 0 && (rightmostX + stepSize) > (WINDOW_WIDTH - FORMATION_EDGE_PADDING)) {
            // Hit right edge
            formationDirection = -1.0f; // Reverse direction
            hitEdge = true;
            moveDownNext = true;        // Flag to move down
        } else if (formationDirection < 0 && (leftmostX - stepSize) < FORMATION_EDGE_PADDING) {
            // Hit left edge
            formationDirection = 1.0f; // Reverse direction
            hitEdge = true;
            moveDownNext = true;        // Flag to move down
        }

        // Perform movement: down if flagged, otherwise sideways
        if (moveDownNext) {
            formationBaseY -= FORMATION_STEP_DOWN;
            moveDownNext = false; // Reset flag after moving down
        } else {
            formationBaseX += stepSize * formationDirection;
        }

        // Update the actual position of each active enemy based on the new formation base
        for (auto& enemy : enemies) {
            if (enemy.active) {
                enemy.x = formationBaseX + enemy.gridCol * ENEMY_GRID_SPACING_X;
                enemy.y = formationBaseY - enemy.gridRow * ENEMY_GRID_SPACING_Y;
            }
        }
    }
}


// --- Spawn a Wave of Enemies ---
// Populates the 'enemies' vector with a new formation
void spawnWave(int waveNum) {
    resetFormationPosition(); // Start formation at the top-left designated spot
    int currentEnemyCount = 0; // Count enemies spawned in this wave
    int enemyIndex = 0;       // Index into the global 'enemies' vector

    // Iterate through grid positions
    for (int r = 0; r < ENEMY_GRID_ROWS; ++r) {
        for (int c = 0; c < ENEMY_GRID_COLS; ++c) {
            // Safety check against vector bounds
            if (enemyIndex >= enemies.size()) {
                break;
            }

            GameObject& enemy = enemies[enemyIndex]; // Get reference to enemy slot

            // Activate and configure the enemy
            enemy.active = true;
            enemy.gridRow = r;
            enemy.gridCol = c;
            // Calculate initial position based on formation base and grid coords
            enemy.x = formationBaseX + c * ENEMY_GRID_SPACING_X;
            enemy.y = formationBaseY - r * ENEMY_GRID_SPACING_Y;
            enemy.dx = 0; // Not used for standard movement
            enemy.dy = 0;
            enemy.shootCooldown = 0.5f + static_cast<float>(rand() % 150) / 100.0f; // Stagger initial shots

            // Assign color based on row or wave (example variation)
            float hue = fmod(r * 60.0f + waveNum * 20.0f, 360.0f);
            // Basic Hue to RGB (simplified, can be improved)
            if (hue < 120) { enemy.r = 1.0f - hue / 120.f; enemy.g = hue / 120.f; enemy.b = 0.0f; }
            else if (hue < 240) { enemy.r = 0.0f; enemy.g = 1.0f - (hue - 120.f) / 120.f; enemy.b = (hue - 120.f) / 120.f; }
            else { enemy.r = (hue - 240.f) / 120.f; enemy.g = 0.0f; enemy.b = 1.0f - (hue - 240.f) / 120.f; }

            currentEnemyCount++;
            enemyIndex++;
        }
        if (enemyIndex >= enemies.size()) break; // Exit outer loop if vector full
    }

    // Update the global count for the wave logic
    enemiesRemainingInWave = currentEnemyCount;

    // Ensure any remaining slots in the pre-resized vector are marked inactive
    for (int i = enemyIndex; i < enemies.size(); ++i) {
        enemies[i].active = false;
    }

    waveActive = true; // Mark wave as officially started
}


// --- Update Difficulty ---
// Sets current difficulty parameters based on gameLevel (derived from wave number)
void updateDifficulty() {
    // Ensure gameLevel stays within the bounds of the defined arrays
    gameLevel = std::min(MAX_LEVELS, currentWave);

    // Apply settings from the per-level arrays
    currentFormationMoveInterval = formationMoveIntervalPerLevel[gameLevel];
    currentEnemyBulletSpeed = bulletSpeedPerLevel[gameLevel];
    currentEnemyShootProb = enemyShootProbPerLevel[gameLevel];

    // Log the change (optional)
    std::cout << "Setting Difficulty for Level " << gameLevel << " (Wave " << currentWave << ")" << std::endl;
    std::cout << "  MoveInterval=" << currentFormationMoveInterval
              << ", BulletSpeed=" << currentEnemyBulletSpeed
              << ", ShootProb=" << currentEnemyShootProb << std::endl;
}

// --- Drawing Functions ---

// Draws the player ship
void drawPlayer() {
    glPushMatrix();
    glTranslatef(player.x, player.y, 0.0f);
    glColor3f(player.r, player.g, player.b); // Player color (green)
    // Ship shape polygon
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, PLAYER_SIZE / 2.0f); // Nose
    glVertex2f(-PLAYER_SIZE / 3.0f, 0.0f);
    glVertex2f(-PLAYER_SIZE / 2.0f, -PLAYER_SIZE / 3.0f);
    glVertex2f(-PLAYER_SIZE / 4.0f, -PLAYER_SIZE / 2.0f); // Engine L
    glVertex2f(PLAYER_SIZE / 4.0f, -PLAYER_SIZE / 2.0f);  // Engine R
    glVertex2f(PLAYER_SIZE / 2.0f, -PLAYER_SIZE / 3.0f);
    glVertex2f(PLAYER_SIZE / 3.0f, 0.0f);
    glEnd();
    // Cockpit triangle
    glColor3f(0.8f, 0.8f, 1.0f); // Light blue
    glBegin(GL_TRIANGLES);
    glVertex2f(0.0f, PLAYER_SIZE / 3.0f);
    glVertex2f(-PLAYER_SIZE / 6.0f, PLAYER_SIZE / 6.0f);
    glVertex2f(PLAYER_SIZE / 6.0f, PLAYER_SIZE / 6.0f);
    glEnd();
    glPopMatrix();
}

// Draws a player bullet
void drawPlayerBullet(const GameObject& bullet) {
    glPushMatrix();
    glTranslatef(bullet.x, bullet.y, 0.0f);
    glColor3f(1.0f, 1.0f, 0.0f); // Yellow
    // Simple quad shape
    glBegin(GL_QUADS);
    glVertex2f(-PLAYER_BULLET_WIDTH / 2.0f, PLAYER_BULLET_HEIGHT / 2.0f);
    glVertex2f(PLAYER_BULLET_WIDTH / 2.0f, PLAYER_BULLET_HEIGHT / 2.0f);
    glVertex2f(PLAYER_BULLET_WIDTH / 2.0f, -PLAYER_BULLET_HEIGHT / 2.0f);
    glVertex2f(-PLAYER_BULLET_WIDTH / 2.0f, -PLAYER_BULLET_HEIGHT / 2.0f);
    glEnd();
    glPopMatrix();
}

// Draws an enemy bullet
void drawEnemyBullet(const GameObject& bullet) {
    glPushMatrix();
    glTranslatef(bullet.x, bullet.y, 0.0f);
    glColor3f(1.0f, 0.0f, 1.0f); // Magenta
    // Simple quad shape
    glBegin(GL_QUADS);
    glVertex2f(-ENEMY_BULLET_SIZE / 2.0f, ENEMY_BULLET_SIZE / 2.0f);
    glVertex2f(ENEMY_BULLET_SIZE / 2.0f, ENEMY_BULLET_SIZE / 2.0f);
    glVertex2f(ENEMY_BULLET_SIZE / 2.0f, -ENEMY_BULLET_SIZE / 2.0f);
    glVertex2f(-ENEMY_BULLET_SIZE / 2.0f, -ENEMY_BULLET_SIZE / 2.0f);
    glEnd();
    glPopMatrix();
}

// Draws an enemy ship
void drawEnemy(const GameObject& enemy) {
    glPushMatrix();
    glTranslatef(enemy.x, enemy.y, 0.0f);
    glColor3f(enemy.r, enemy.g, enemy.b); // Use assigned enemy color
    // Outer square
    glBegin(GL_QUADS);
    glVertex2f(-ENEMY_SIZE / 2.0f, ENEMY_SIZE / 2.0f);
    glVertex2f(ENEMY_SIZE / 2.0f, ENEMY_SIZE / 2.0f);
    glVertex2f(ENEMY_SIZE / 2.0f, -ENEMY_SIZE / 2.0f);
    glVertex2f(-ENEMY_SIZE / 2.0f, -ENEMY_SIZE / 2.0f);
    glEnd();
    // Inner contrasting square
    glColor3f(fmax(0.0f, 1.0f - enemy.r * 1.2f), fmax(0.0f, 1.0f - enemy.g * 1.2f), fmax(0.0f, 1.0f - enemy.b * 1.2f));
    glBegin(GL_QUADS);
    glVertex2f(-ENEMY_SIZE / 4.0f, ENEMY_SIZE / 4.0f);
    glVertex2f(ENEMY_SIZE / 4.0f, ENEMY_SIZE / 4.0f);
    glVertex2f(ENEMY_SIZE / 4.0f, -ENEMY_SIZE / 4.0f);
    glVertex2f(-ENEMY_SIZE / 4.0f, -ENEMY_SIZE / 4.0f);
    glEnd();
    glPopMatrix();
}

// Draws the starfield background
void drawStars() {
    glPointSize(1.0f);
    glBegin(GL_POINTS);
    for (const auto& star : stars) {
        // Simulate parallax effect
        float parallaxOffsetY = (1.0f - star.z) * frameCount * 0.05f;
        float screenY = star.y - parallaxOffsetY;
        // Wrap stars vertically
        while (screenY < 0) {
            screenY += WINDOW_HEIGHT;
        }
        glColor3f(star.brightness, star.brightness, star.brightness);
        glVertex2f(star.x, fmod(screenY, WINDOW_HEIGHT)); // Use fmod for smooth wrap
    }
    glEnd();
}

// Draws active particles (explosions)
void drawParticles() {
    glEnable(GL_BLEND); // Enable blending for transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard alpha blending
    glPointSize(3.0f); // Particle size
    glBegin(GL_POINTS);
    for (const auto& p : particles) {
        if (p.active) {
            float alpha = fmax(0.0f, p.life / PARTICLE_LIFESPAN); // Calculate alpha based on life
            glColor4f(p.r, p.g, p.b, alpha); // Set color with alpha
            glVertex2f(p.x, p.y);
        }
    }
    glEnd();
    glDisable(GL_BLEND); // Disable blending afterwards
}

// --- Spawning Functions ---

// Creates a player bullet if the limit is not reached
void firePlayerBullet() {
    // --- Limit Active Bullets ---
    int activeBulletCount = 0;
    for (const auto& b : playerBullets) {
        if (b.active) {
            activeBulletCount++;
        }
    }

    // Only fire if count is below the limit
    if (activeBulletCount >= MAX_ACTIVE_PLAYER_BULLETS) {
        return; // Do nothing if limit is reached
    }
    // --- End Limit Check ---


    // Find an inactive bullet slot in the pool or add a new one
    GameObject* bullet = nullptr;
    for (auto& b : playerBullets) {
        if (!b.active) {
            bullet = &b;
            break;
        }
    }
    // If no inactive slot found, potentially add to the pool (with a limit)
    if (bullet == nullptr) {
        if (playerBullets.size() < 50) { // Optional safety limit for pool size
            playerBullets.push_back(GameObject());
            bullet = &playerBullets.back();
        } else {
            return; // Don't fire if pool is truly full
        }
    }

    // Initialize the bullet's properties
    bullet->x = player.x;
    bullet->y = player.y + PLAYER_SIZE / 2.0f; // Start from player's nose
    bullet->dx = 0;
    bullet->dy = PLAYER_BULLET_SPEED;
    bullet->active = true;
    // No ammo decrement needed
}

// Creates an enemy bullet originating from a specific enemy
void fireEnemyBullet(const GameObject& enemy) {
    // Find/create bullet slot
    GameObject* bullet = nullptr;
    for (auto& b : enemyBullets) {
        if (!b.active) {
            bullet = &b;
            break;
        }
    }
    if (bullet == nullptr) {
        if (enemyBullets.size() < 150) { // Pool size limit
            enemyBullets.push_back(GameObject());
            bullet = &enemyBullets.back();
        } else {
            return;
        }
    }

    // Initialize enemy bullet properties
    bullet->x = enemy.x;
    bullet->y = enemy.y - ENEMY_SIZE / 2.0f; // Start from enemy's front
    bullet->dx = 0;
    bullet->dy = -currentEnemyBulletSpeed; // Use current difficulty speed (negative for down)
    bullet->active = true;
    bullet->r = 1.0f; bullet->g = 0.2f; bullet->b = 0.2f; // Set enemy bullet color (e.g., reddish)
}

// Creates explosion particles at a given location and color
void spawnExplosion(float x, float y, float r, float g, float b) {
    int particlesToSpawn = 15 + rand() % 10; // Number of particles per explosion

    for (int i = 0; i < particlesToSpawn; ++i) {
        // Find/create particle slot
        Particle* p = nullptr;
        for (auto& existingParticle : particles) {
            if (!existingParticle.active) {
                p = &existingParticle;
                break;
            }
        }
        if (p == nullptr) {
            if (particles.size() < MAX_PARTICLES) { // Pool size limit
                particles.push_back(Particle());
                p = &particles.back();
            } else {
                continue; // Skip if particle pool is full
            }
        }

        // Initialize particle properties
        p->x = x;
        p->y = y;
        float angle = static_cast<float>(rand() % 360) * M_PI / 180.0f; // Random direction
        float speed = 50.0f + static_cast<float>(rand() % 50);        // Random speed
        p->dx = cos(angle) * speed;
        p->dy = sin(angle) * speed;
        p->r = r;
        p->g = g;
        p->b = b;
        p->life = PARTICLE_LIFESPAN * (0.7f + static_cast<float>(rand() % 31) / 100.0f); // Vary lifespan
        p->active = true;
    }
}

// --- Collision Detection ---
// Simple Axis-Aligned Bounding Box (AABB) collision check
bool checkCollision(const GameObject& obj1, float size1_x, float size1_y, const GameObject& obj2, float size2_x, float size2_y) {
    // Calculate boundaries for object 1
    float obj1Left = obj1.x - size1_x / 2.0f;
    float obj1Right = obj1.x + size1_x / 2.0f;
    float obj1Top = obj1.y + size1_y / 2.0f;
    float obj1Bottom = obj1.y - size1_y / 2.0f;

    // Calculate boundaries for object 2
    float obj2Left = obj2.x - size2_x / 2.0f;
    float obj2Right = obj2.x + size2_x / 2.0f;
    float obj2Top = obj2.y + size2_y / 2.0f;
    float obj2Bottom = obj2.y - size2_y / 2.0f;

    // Check for overlap on both X and Y axes
    bool overlapX = obj1Right > obj2Left && obj1Left < obj2Right;
    bool overlapY = obj1Top > obj2Bottom && obj1Bottom < obj2Top;

    return overlapX && overlapY; // Collision if overlap on both axes
}

// --- Text Drawing ---
// Renders text on screen using GLUT bitmap fonts
void drawText(float x, float y, const std::string& text, void* font) {
    glColor3f(1.0f, 1.0f, 1.0f); // Set text color (white)
    glRasterPos2f(x, y);          // Set position for text rendering
    // Iterate through characters and draw them
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}
}

void launchShooter() {
    shooter::launchShooter();
}