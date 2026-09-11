#include <raylib.h>
#include <iostream>
#include <cstdlib>
using namespace std;

// 2D Vector structure for position and size
struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float xx, float yy) : x(xx), y(yy) {}
};

// Enemy spacecraft entity
class Enemy {
public:
    Vec2 position;
    Vec2 originalPosition;
    bool active;

    Enemy() : position(0, 0), originalPosition(0, 0), active(false) {}
    Enemy(Vec2 pos) : position(pos), originalPosition(pos), active(true) {}

    void ResetPosition() {
        position = originalPosition;
    }

    void Move(float xOffset, float yOffset) {
        position.x += xOffset;
        position.y += yOffset;
    }

    Rectangle GetBounds(Vec2 size) const {
        return { position.x, position.y, size.x, size.y };
    }
};

// Player spacecraft entity
class Spacecraft {
public:
    Vec2 position;
    Vec2 dimensions;
    int lives;
    Texture2D texture;
    int speed;

    Spacecraft(Vec2 pos, Texture2D tex, int spd, int lifeCount)
        : position(pos), texture(tex), speed(spd), lives(lifeCount) {
        dimensions = { (float)tex.width, (float)tex.height };
    }

    void MoveLeft(float deltaTime) {
        position.x -= speed * deltaTime;
    }

    void MoveRight(float deltaTime) {
        position.x += speed * deltaTime;
    }

    Rectangle GetBounds() const {
        return { position.x, position.y, dimensions.x, dimensions.y };
    }

    void Draw() const {
        DrawTexture(texture, (int)position.x, (int)position.y, WHITE);
    }
};

// Projectile entity for player and enemy fire
class Projectile {
public:
    Vec2 position;

    Projectile(Vec2 pos) : position(pos) {}

    void Update(float speed, float deltaTime, bool upward = true) {
        position.y += (upward ? -1 : 1) * speed * deltaTime;
    }

    Rectangle GetBounds(Vec2 size) const {
        return { position.x, position.y, size.x, size.y };
    }
};

// Check collision between two rectangles
bool CheckCollision(const Rectangle& a, const Rectangle& b) {
    return CheckCollisionRecs(a, b);
}

// Draw GAME OVER text with outline effect
void DrawGameOverText(int screenWidth, int screenHeight) {
    const char* text = "GAME OVER";
    int fontSize = 80;
    int textWidth = MeasureText(text, fontSize);
    Vec2 position = { (screenWidth - textWidth) / 2.0f, screenHeight / 2.0f - 60 };
    Color white = WHITE;
    Color red = RED;

    int outlineOffsets[8][2] = {
        {-3, -3}, {-3, 3}, {3, -3}, {3, 3},
        {0, -4}, {0, 4}, {-4, 0}, {4, 0}
    };

    for (int i = 0; i < 8; i++)
        DrawText(text, (int)position.x + outlineOffsets[i][0], (int)position.y + outlineOffsets[i][1], fontSize, white);

    DrawText(text, (int)position.x, (int)position.y, fontSize, red);
}

int main() {
    const int screenWidth = 1080;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Space Invaders");
    InitAudioDevice();
    SetTargetFPS(60);

    // Load textures
    Texture2D playerTexture = LoadTexture("player_ship.png");
    Texture2D enemyTexture = LoadTexture("enemy_invader.png");
    Texture2D backgroundTexture = LoadTexture("background.png");
    Texture2D enemyProjectileTexture = LoadTexture("enemy_bomb.png");
    Texture2D powerUpDoubleShotTexture = LoadTexture("powerup_double_shot.png");
    Texture2D powerUpShowBillaTexture = LoadTexture("powerup_bonus.png");

    // Load sounds
    Sound shootSound = LoadSound("shoot.wav");
    Sound gameOverSound = LoadSound("game_over.wav");
    bool gameOverSoundPlayed = false;

    // Initialize player
    Spacecraft player(
        { screenWidth / 2.0f, (float)(screenHeight - playerTexture.height) },
        playerTexture, 1000, 3
    );

    // Projectile configuration
    Vec2 projectileSize = { 10, 20 };
    const int maxProjectiles = 50;
    Projectile* playerProjectiles[maxProjectiles] = {};
    int playerProjectileCount = 0;

    Projectile* enemyProjectiles[maxProjectiles] = {};
    int enemyProjectileCount = 0;

    // Enemy configuration
    const int enemyCount = 50;
    const int enemiesPerLine = 10;
    Vec2 enemySize = { (float)enemyTexture.width, (float)enemyTexture.height };
    Vec2 enemyGap = { enemySize.x / 2.0f, enemySize.y / 4.0f };

    Enemy enemies[enemyCount];
    for (int i = 0; i < enemyCount; ++i) {
        Vec2 position = {
            enemyGap.x + (enemySize.x + enemyGap.x) * (i % enemiesPerLine),
            (enemySize.y + enemyGap.y) * (i / enemiesPerLine)
        };
        enemies[i] = Enemy(position);
    }

    // Game state variables
    int enemiesAlive = enemyCount;
    float nextPlayerShotTime = 0;
    float shootingRate = 2.0f;
    float nextEnemyShotTime = 0;
    float enemyShootingRate = 3.0f;
    int enemySpeed = 125;
    int movementDirection = 1;
    bool shouldDescend = false;
    int enemiesDestroyed = 0;

    // Power-up states
    bool doubleShotActive = false;
    Vec2 doubleShotPosition;
    int doubleShotSpeed = 400;
    bool showBillaActive = false;
    float billaTimer = 0;
    const float billaShowDuration = 1.0f;
    Vec2 billaPosition;

    // Score and game status
    int score = 0;
    bool gameOver = false;
    double doubleShotEndTime = 0;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        if (!gameOver) {
            // Player movement
            if (IsKeyDown(KEY_LEFT) && player.position.x > 0)
                player.MoveLeft(deltaTime);
            if (IsKeyDown(KEY_RIGHT) && player.position.x < screenWidth - player.dimensions.x)
                player.MoveRight(deltaTime);

            // Enemy formation movement
            float maxX = 0, minX = screenWidth;
            for (int i = 0; i < enemyCount; i++) {
                if (enemies[i].active) {
                    if (enemies[i].position.x < minX) minX = enemies[i].position.x;
                    if (enemies[i].position.x + enemySize.x > maxX) maxX = enemies[i].position.x + enemySize.x;
                }
            }

            // Change direction at screen edges
            if (maxX >= screenWidth - enemyGap.x || minX <= enemyGap.x) {
                movementDirection *= -1;
                shouldDescend = true;
            }

            // Move all active enemies
            for (int i = 0; i < enemyCount; ++i) {
                if (enemies[i].active)
                    enemies[i].Move(enemySpeed * movementDirection * deltaTime, shouldDescend ? enemyGap.y : 0.0f);
            }
            shouldDescend = false;

            // Player shooting
            if (IsKeyDown(KEY_SPACE) && GetTime() >= nextPlayerShotTime && playerProjectileCount < maxProjectiles) {
                PlaySound(shootSound);
                if (GetTime() < doubleShotEndTime && playerProjectileCount + 2 <= maxProjectiles) {
                    // Double shot with slight spread
                    playerProjectiles[playerProjectileCount++] = new Projectile(
                        { player.position.x + player.dimensions.x / 2 - projectileSize.x / 2 - 5, player.position.y }
                    );
                    playerProjectiles[playerProjectileCount++] = new Projectile(
                        { player.position.x + player.dimensions.x / 2 - projectileSize.x / 2 + 5, player.position.y }
                    );
                } else {
                    // Single shot
                    playerProjectiles[playerProjectileCount++] = new Projectile(
                        { player.position.x + player.dimensions.x / 2 - projectileSize.x / 2, player.position.y }
                    );
                }
                nextPlayerShotTime = GetTime() + 1.0f / shootingRate;
            }

            // Update player projectiles and check enemy collisions
            for (int i = 0; i < playerProjectileCount;) {
                playerProjectiles[i]->Update(700, deltaTime);
                bool hit = false;
                for (int j = 0; j < enemyCount; ++j) {
                    if (enemies[j].active && CheckCollision(
                        playerProjectiles[i]->GetBounds(projectileSize),
                        enemies[j].GetBounds(enemySize)
                    )) {
                        enemies[j].active = false;
                        enemiesDestroyed++;
                        enemiesAlive--;
                        score += 10;
                        hit = true;

                        // Spawn double shot power-up every 4 kills
                        if (enemiesDestroyed % 4 == 0) {
                            doubleShotActive = true;
                            doubleShotPosition = { enemies[j].position.x, 0 };
                        }

                        break;
                    }
                }
                if (playerProjectiles[i]->position.y < 0 || hit) {
                    delete playerProjectiles[i];
                    playerProjectiles[i] = playerProjectiles[--playerProjectileCount];
                } else {
                    ++i;
                }
            }

            // Update double shot power-up
            if (doubleShotActive) {
                doubleShotPosition.y += doubleShotSpeed * deltaTime;
                Rectangle doubleShotRect = {
                    doubleShotPosition.x, doubleShotPosition.y,
                    (float)powerUpDoubleShotTexture.width, (float)powerUpDoubleShotTexture.height
                };
                if (CheckCollision(doubleShotRect, player.GetBounds())) {
                    score += 50;
                    doubleShotActive = false;
                    doubleShotEndTime = GetTime() + 7.0;
                } else if (doubleShotPosition.y > screenHeight) {
                    doubleShotActive = false;
                }
            }

            // Enemy shooting
            if (GetTime() >= nextEnemyShotTime && enemyProjectileCount < maxProjectiles && enemiesAlive > 0) {
                int shooterIndex = rand() % enemyCount;
                while (!enemies[shooterIndex].active) {
                    shooterIndex = rand() % enemyCount;
                }
                enemyProjectiles[enemyProjectileCount++] = new Projectile(
                    { enemies[shooterIndex].position.x + enemySize.x / 2, enemies[shooterIndex].position.y + enemySize.y }
                );
                nextEnemyShotTime = GetTime() + 1.0f / enemyShootingRate;
            }

            // Update enemy projectiles and check player collision
            for (int i = 0; i < enemyProjectileCount;) {
                enemyProjectiles[i]->Update(700, deltaTime, false);
                if (CheckCollision(enemyProjectiles[i]->GetBounds(projectileSize), player.GetBounds())) {
                    player.lives--;
                    delete enemyProjectiles[i];
                    enemyProjectiles[i] = enemyProjectiles[--enemyProjectileCount];
                    if (player.lives <= 0) {
                        gameOver = true;
                        if (!gameOverSoundPlayed) {
                            PlaySound(gameOverSound);
                            gameOverSoundPlayed = true;
                        }
                    }
                } else if (enemyProjectiles[i]->position.y > screenHeight) {
                    // Show Billa when enemy missile passes
                    showBillaActive = true;
                    billaTimer = billaShowDuration;
                    billaPosition = { enemyProjectiles[i]->position.x, (float)(screenHeight - powerUpShowBillaTexture.height - 10) };
                    delete enemyProjectiles[i];
                    enemyProjectiles[i] = enemyProjectiles[--enemyProjectileCount];
                } else {
                    ++i;
                }
            }

            // Handle Billa timer
            if (showBillaActive) {
                billaTimer -= deltaTime;
                if (billaTimer <= 0) showBillaActive = false;
            }

            // Check win condition
            if (enemiesAlive <= 0) {
                gameOver = true;
                if (!gameOverSoundPlayed) {
                    PlaySound(gameOverSound);
                    gameOverSoundPlayed = true;
                }
            }
        } else if (IsKeyPressed(KEY_ENTER)) {
            // Restart game
            player.lives = 3;
            score = 0;
            playerProjectileCount = enemyProjectileCount = enemiesDestroyed = 0;
            doubleShotActive = showBillaActive = false;
            enemiesAlive = enemyCount;
            doubleShotEndTime = 0;
            gameOverSoundPlayed = false;
            for (int i = 0; i < enemyCount; ++i) {
                enemies[i].active = true;
                enemies[i].ResetPosition();
            }
            gameOver = false;
        }

        // Rendering
        BeginDrawing();
        DrawTexture(backgroundTexture, 0, 0, WHITE);
        player.Draw();

        // Draw active enemies
        for (int i = 0; i < enemyCount; ++i)
            if (enemies[i].active)
                DrawTexture(enemyTexture, (int)enemies[i].position.x, (int)enemies[i].position.y, WHITE);

        // Draw player projectiles
        for (int i = 0; i < playerProjectileCount; ++i)
            DrawRectangle((int)playerProjectiles[i]->position.x, (int)playerProjectiles[i]->position.y,
                          (int)projectileSize.x, (int)projectileSize.y, GREEN);

        // Draw enemy projectiles
        for (int i = 0; i < enemyProjectileCount; ++i)
            DrawTexture(enemyProjectileTexture, (int)enemyProjectiles[i]->position.x,
                        (int)enemyProjectiles[i]->position.y, WHITE);

        // Draw power-ups
        if (doubleShotActive)
            DrawTexture(powerUpDoubleShotTexture, (int)doubleShotPosition.x, (int)doubleShotPosition.y, WHITE);

        if (showBillaActive)
            DrawTexture(powerUpShowBillaTexture, (int)billaPosition.x, (int)billaPosition.y, WHITE);

        // Draw UI
        DrawText(TextFormat("Score: %d", score), 10, 10, 30, WHITE);
        DrawText(TextFormat("Lives: %d", player.lives), screenWidth - 150, 10, 30, WHITE);

        if (gameOver) {
            DrawGameOverText(screenWidth, screenHeight);
            DrawText("Press ENTER to Restart", screenWidth / 2 - 150, screenHeight / 2 + 70, 30, WHITE);
        }

        EndDrawing();
    }

    // Cleanup
    for (int i = 0; i < playerProjectileCount; ++i) delete playerProjectiles[i];
    for (int i = 0; i < enemyProjectileCount; ++i) delete enemyProjectiles[i];

    UnloadTexture(playerTexture);
    UnloadTexture(enemyTexture);
    UnloadTexture(backgroundTexture);
    UnloadTexture(enemyProjectileTexture);
    UnloadTexture(powerUpDoubleShotTexture);
    UnloadTexture(powerUpShowBillaTexture);
    UnloadSound(shootSound);
    UnloadSound(gameOverSound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
