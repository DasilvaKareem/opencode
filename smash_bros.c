#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_PLAYERS 4
#define MAX_PLATFORMS 6
#define GRAVITY 0.6f
#define MAX_FALL_SPEED 15.0f
#define GROUND_FRICTION 0.85f
#define AIR_FRICTION 0.98f
#define STAGE_DEATH_BUFFER 100.0f
#define GAMEPAD_DEADZONE 0.2f

typedef struct {
    Vector2 position;
    Vector2 size;
    Color color;
} Platform;

typedef enum {
    ATTACK_NONE,
    ATTACK_NORMAL,
    ATTACK_UP_AIR,
    ATTACK_DOWN_AIR,
    ATTACK_SMASH
} AttackType;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Vector2 size;
    float percent;
    int stocks;
    bool isGrounded;
    bool isAlive;
    int playerNum;
    Color color;
    bool isAttacking;
    AttackType currentAttack;
    int attackCooldown;
    Rectangle attackBox;
    int hitstun;
    bool canDoubleJump;
    int gamepadId;
    bool facingRight;
    int respawnInvincibility;
    bool isBlocking;
    float shieldHealth;
    bool isChargingSmash;
    int smashChargeTime;
    float smashChargePower;
    bool smashIsDown; // Track if it's a down smash
} Player;

typedef struct {
    Platform platforms[MAX_PLATFORMS];
    int platformCount;
    Rectangle stageBounds;
} Stage;

void InitStage(Stage* stage, int screenWidth, int screenHeight) {
    stage->platformCount = 6;
    
    // Main platform
    stage->platforms[0] = (Platform){
        .position = {(float)screenWidth/2.0f - 300, (float)screenHeight - 150},
        .size = {600, 30},
        .color = DARKBROWN
    };
    
    // Left platform
    stage->platforms[1] = (Platform){
        .position = {150, (float)screenHeight - 300},
        .size = {150, 20},
        .color = BROWN
    };
    
    // Right platform
    stage->platforms[2] = (Platform){
        .position = {(float)screenWidth - 300, (float)screenHeight - 300},
        .size = {150, 20},
        .color = BROWN
    };
    
    // Top left platform
    stage->platforms[3] = (Platform){
        .position = {(float)screenWidth/2.0f - 250, (float)screenHeight - 450},
        .size = {120, 20},
        .color = BROWN
    };
    
    // Top right platform
    stage->platforms[4] = (Platform){
        .position = {(float)screenWidth/2.0f + 130, (float)screenHeight - 450},
        .size = {120, 20},
        .color = BROWN
    };
    
    // Top center platform
    stage->platforms[5] = (Platform){
        .position = {(float)screenWidth/2.0f - 75, (float)screenHeight - 550},
        .size = {150, 20},
        .color = BROWN
    };
    
    // Stage death bounds (larger than screen)
    stage->stageBounds = (Rectangle){
        -STAGE_DEATH_BUFFER,
        -STAGE_DEATH_BUFFER,
        screenWidth + STAGE_DEATH_BUFFER * 2,
        screenHeight + STAGE_DEATH_BUFFER * 2
    };
}

void InitPlayer(Player* player, int playerNum, Vector2 startPos, Color color, int gamepadId) {
    player->position = startPos;
    player->velocity = (Vector2){0, 0};
    player->size = (Vector2){40, 50};
    player->percent = 0;
    player->stocks = 3;
    player->isGrounded = false;
    player->isAlive = true;
    player->playerNum = playerNum;
    player->color = color;
    player->isAttacking = false;
    player->currentAttack = ATTACK_NONE;
    player->attackCooldown = 0;
    player->hitstun = 0;
    player->canDoubleJump = true;
    player->gamepadId = gamepadId;
    player->facingRight = (playerNum == 1);
    player->respawnInvincibility = 0;
    player->isBlocking = false;
    player->shieldHealth = 100.0f;
    player->isChargingSmash = false;
    player->smashChargeTime = 0;
    player->smashChargePower = 1.0f;
    player->smashIsDown = false;
}

bool CheckPlatformCollision(Player* player, Platform* platform) {
    // Only collide from above and moving downward
    if (player->velocity.y >= 0 &&
        player->position.x + player->size.x > platform->position.x &&
        player->position.x < platform->position.x + platform->size.x &&
        player->position.y + player->size.y >= platform->position.y &&
        player->position.y + player->size.y <= platform->position.y + platform->size.y + 10) {
        
        player->position.y = platform->position.y - player->size.y;
        player->velocity.y = 0;
        return true;
    }
    
    return false;
}

void ApplyKnockback(Player* target, Vector2 knockbackDir, float damageMultiplier) {
    // Check if blocking
    if (target->isBlocking && target->shieldHealth > 0) {
        // Shield absorbs attack
        float shieldDamage = 12.0f * damageMultiplier;
        target->shieldHealth -= shieldDamage;
        
        if (target->shieldHealth <= 0) {
            target->shieldHealth = 0;
            target->isBlocking = false;
            // Shield break stun
            target->hitstun = 120; // 2 seconds of stun
        } else {
            // Small pushback when blocking
            target->velocity.x = knockbackDir.x * 2.0f;
            target->velocity.y = knockbackDir.y * 0.5f;
        }
        return;
    }
    
    float baseKnockback = 8.0f;
    float knockbackGrowth = 0.08f;
    
    // Calculate knockback based on percentage and damage multiplier
    float totalKnockback = (baseKnockback + (target->percent * knockbackGrowth)) * damageMultiplier;
    
    // Apply knockback
    target->velocity.x = knockbackDir.x * totalKnockback;
    target->velocity.y = knockbackDir.y * totalKnockback;
    
    // Apply hitstun (more hitstun at higher percent)
    target->hitstun = (int)((15 + target->percent * 0.2f) * damageMultiplier);
    
    // Add damage
    target->percent += 12.0f * damageMultiplier;
    if (target->percent > 999.0f) target->percent = 999.0f;
    
    // Reset grounded state
    target->isGrounded = false;
}

void HandleAttack(Player* player, Player* opponents, int opponentCount) {
    if (!player->isAttacking) return;
    
    // Create attack hitbox based on attack type
    if (player->currentAttack == ATTACK_UP_AIR) {
        // Up air - hitbox above player
        player->attackBox = (Rectangle){
            player->position.x - 10,
            player->position.y - 60,
            player->size.x + 20,
            65
        };
    } else if (player->currentAttack == ATTACK_DOWN_AIR) {
        // Down air - hitbox below player
        player->attackBox = (Rectangle){
            player->position.x - 10,
            player->position.y + player->size.y,
            player->size.x + 20,
            65
        };
    } else {
        // Normal attack - in front of player
        float hitboxOffset = player->facingRight ? player->size.x : -60;
        player->attackBox = (Rectangle){
            player->position.x + hitboxOffset,
            player->position.y,
            60,
            player->size.y
        };
    }
    
    // Check for hits (only during first frames of attack)
    if (player->attackCooldown > 20) {
        for (int i = 0; i < opponentCount; i++) {
            if (!opponents[i].isAlive || opponents[i].hitstun > 0 || opponents[i].respawnInvincibility > 0) continue;
            
            Rectangle opponentRect = {
                opponents[i].position.x,
                opponents[i].position.y,
                opponents[i].size.x,
                opponents[i].size.y
            };
            
            if (CheckCollisionRecs(player->attackBox, opponentRect)) {
                // Calculate knockback direction and damage multiplier based on attack type
                Vector2 knockbackDir;
                float damageMultiplier = 1.0f;
                
                if (player->currentAttack == ATTACK_UP_AIR) {
                    // Up air launches straight up
                    knockbackDir = (Vector2){0.0f, -1.0f};
                } else if (player->currentAttack == ATTACK_DOWN_AIR) {
                    // Down air spikes downward (meteor smash)
                    knockbackDir = (Vector2){0.0f, 1.2f};
                } else if (player->currentAttack == ATTACK_SMASH) {
                    // Smash attack - uses charge power
                    damageMultiplier = player->smashChargePower;
                    
                    // Down smash launches UP (like real Smash Bros)
                    if (player->smashIsDown) {
                        knockbackDir = (Vector2){0.0f, -1.2f}; // Strong vertical launch
                    } else {
                        // Side/up smash launches diagonally up
                        knockbackDir = (Vector2){
                            opponents[i].position.x + opponents[i].size.x/2.0f - (player->position.x + player->size.x/2.0f),
                            -0.8f
                        };
                    }
                } else {
                    // Normal attack launches diagonally
                    knockbackDir = (Vector2){
                        opponents[i].position.x + opponents[i].size.x/2.0f - (player->position.x + player->size.x/2.0f),
                        -0.7f
                    };
                }
                
                knockbackDir = Vector2Normalize(knockbackDir);
                ApplyKnockback(&opponents[i], knockbackDir, damageMultiplier);
                
                // End attack on hit to prevent multi-hits
                player->attackCooldown = 10;
            }
        }
    }
}

void RespawnPlayer(Player* player, float screenWidth) {
    player->position = (Vector2){screenWidth/2.0f - 20, 100};
    player->velocity = (Vector2){0, 0};
    player->percent = 0;
    player->hitstun = 0;
    player->isAlive = true;
    player->canDoubleJump = true;
    player->respawnInvincibility = 120; // 2 seconds of invincibility
    player->shieldHealth = 100.0f; // Reset shield
    player->isBlocking = false;
    player->isChargingSmash = false;
    player->smashChargeTime = 0;
    player->smashChargePower = 1.0f;
}

void UpdatePlayer(Player* player, Stage* stage, int screenWidth, int screenHeight) {
    if (!player->isAlive) return;
    
    // Update cooldowns
    if (player->attackCooldown > 0) player->attackCooldown--;
    if (player->hitstun > 0) {
        player->hitstun--;
        player->isAttacking = false;
        player->isChargingSmash = false;
    }
    if (player->respawnInvincibility > 0) player->respawnInvincibility--;
    
    // Shield regeneration (when not blocking)
    if (!player->isBlocking && player->shieldHealth < 100.0f) {
        player->shieldHealth += 0.5f; // Regenerate 0.5 per frame (30 health per second)
        if (player->shieldHealth > 100.0f) player->shieldHealth = 100.0f;
    }
    
    // Shield depletion while blocking
    if (player->isBlocking) {
        player->shieldHealth -= 0.3f; // Slowly depletes while holding
        if (player->shieldHealth <= 0) {
            player->shieldHealth = 0;
            player->isBlocking = false;
            player->hitstun = 120; // Shield break stun
        }
    }
    
    // Update smash charge
    if (player->isChargingSmash) {
        player->smashChargeTime++;
        // Max charge at 120 frames (2 seconds)
        if (player->smashChargeTime > 120) player->smashChargeTime = 120;
        // Charge power from 1.0x to 2.5x
        player->smashChargePower = 1.0f + (player->smashChargeTime / 120.0f) * 1.5f;
    }
    
    // Check if dead (fell off stage)
    if (player->position.x < -STAGE_DEATH_BUFFER ||
        player->position.x > screenWidth + STAGE_DEATH_BUFFER ||
        player->position.y < -STAGE_DEATH_BUFFER ||
        player->position.y > screenHeight + STAGE_DEATH_BUFFER) {
        
        player->stocks--;
        if (player->stocks <= 0) {
            player->isAlive = false;
        } else {
            RespawnPlayer(player, (float)screenWidth);
        }
        return;
    }
    
    // Apply gravity
    player->velocity.y += GRAVITY;
    if (player->velocity.y > MAX_FALL_SPEED) {
        player->velocity.y = MAX_FALL_SPEED;
    }
    
    // Apply friction
    if (player->isGrounded) {
        player->velocity.x *= GROUND_FRICTION;
    } else {
        player->velocity.x *= AIR_FRICTION;
    }
    
    // Update position
    player->position.x += player->velocity.x;
    player->position.y += player->velocity.y;
    
    // Check platform collisions
    player->isGrounded = false;
    for (int i = 0; i < stage->platformCount; i++) {
        if (CheckPlatformCollision(player, &stage->platforms[i])) {
            player->isGrounded = true;
            player->canDoubleJump = true;
        }
    }
    
    // Update facing direction based on velocity
    if (player->velocity.x > 0.5f) player->facingRight = true;
    else if (player->velocity.x < -0.5f) player->facingRight = false;
}

void HandlePlayerInput(Player* player) {
    if (player->hitstun > 0) return; // Can't move during hitstun
    
    int playerNum = player->playerNum;
    float moveSpeed = 5.0f;
    float jumpPower = -13.0f;
    
    bool moveLeft = false;
    bool moveRight = false;
    bool moveUp = false;
    bool moveDown = false;
    bool jumpPressed = false;
    bool attackPressed = false;
    bool blockHeld = false;
    bool isSmashing = false; // Direction + Attack = Smash
    
    // Check gamepad input first (PS5 controller)
    if (IsGamepadAvailable(player->gamepadId)) {
        float axisX = GetGamepadAxisMovement(player->gamepadId, GAMEPAD_AXIS_LEFT_X);
        float axisY = GetGamepadAxisMovement(player->gamepadId, GAMEPAD_AXIS_LEFT_Y);
        
        // Handle analog stick movement with deadzone
        if (axisX < -GAMEPAD_DEADZONE) moveLeft = true;
        if (axisX > GAMEPAD_DEADZONE) moveRight = true;
        if (axisY < -GAMEPAD_DEADZONE) moveUp = true;
        if (axisY > GAMEPAD_DEADZONE) moveDown = true;
        
        // D-pad support
        if (IsGamepadButtonDown(player->gamepadId, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) moveLeft = true;
        if (IsGamepadButtonDown(player->gamepadId, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) moveRight = true;
        if (IsGamepadButtonDown(player->gamepadId, GAMEPAD_BUTTON_LEFT_FACE_UP)) moveUp = true;
        if (IsGamepadButtonDown(player->gamepadId, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) moveDown = true;
        
        // Jump buttons (X button)
        if (IsGamepadButtonPressed(player->gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
            jumpPressed = true;
        }
        
        // Attack button (Square) - THIS IS THE MAIN ATTACK BUTTON
        if (IsGamepadButtonPressed(player->gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) {
            attackPressed = true;
        }
        
        // Block button (L1, L2, R1 - shoulder/trigger buttons)
        if (GetGamepadAxisMovement(player->gamepadId, GAMEPAD_AXIS_LEFT_TRIGGER) > 0.1f) {
            blockHeld = true;
        }
        
        if (IsGamepadButtonDown(player->gamepadId, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) {
            blockHeld = true;
        }
        
        if (IsGamepadButtonDown(player->gamepadId, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) {
            blockHeld = true;
        }
        
        // Smash attack = Direction held + Attack (like real Smash Bros)
        if ((moveLeft || moveRight || moveUp || moveDown) && 
            IsGamepadButtonPressed(player->gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) {
            isSmashing = true;
        }
    }
    
    // Fallback to keyboard controls
    if (playerNum == 1) {
        if (IsKeyDown(KEY_A)) moveLeft = true;
        if (IsKeyDown(KEY_D)) moveRight = true;
        if (IsKeyDown(KEY_W)) moveUp = true;
        if (IsKeyDown(KEY_S)) moveDown = true;
        if (IsKeyPressed(KEY_W) && !attackPressed) jumpPressed = true; // W = jump if not attacking
        if (IsKeyPressed(KEY_SPACE)) attackPressed = true;
        if (IsKeyDown(KEY_LEFT_SHIFT)) blockHeld = true;
        
        // Smash = Direction + Space
        if ((moveLeft || moveRight || moveUp || moveDown) && IsKeyPressed(KEY_SPACE)) {
            isSmashing = true;
        }
    } else if (playerNum == 2) {
        if (IsKeyDown(KEY_LEFT)) moveLeft = true;
        if (IsKeyDown(KEY_RIGHT)) moveRight = true;
        if (IsKeyDown(KEY_UP)) moveUp = true;
        if (IsKeyDown(KEY_DOWN)) moveDown = true;
        if (IsKeyPressed(KEY_UP) && !attackPressed) jumpPressed = true; // Up = jump if not attacking
        if (IsKeyPressed(KEY_RIGHT_CONTROL)) attackPressed = true;
        if (IsKeyDown(KEY_RIGHT_SHIFT)) blockHeld = true;
        
        // Smash = Direction + RCtrl
        if ((moveLeft || moveRight || moveUp || moveDown) && IsKeyPressed(KEY_RIGHT_CONTROL)) {
            isSmashing = true;
        }
    }
    
    // Handle blocking
    if (blockHeld && !player->isChargingSmash && !player->isAttacking) {
        player->isBlocking = true;
        player->velocity.x *= 0.5f; // Slow movement while blocking
    } else {
        player->isBlocking = false;
    }
    
    // Apply movement (can't move while charging smash)
    if (!player->isChargingSmash && !player->isBlocking) {
        if (moveLeft) player->velocity.x = -moveSpeed;
        if (moveRight) player->velocity.x = moveSpeed;
    }
    
    // Handle jumping (can't jump while charging)
    if (jumpPressed && !player->isChargingSmash) {
        if (player->isGrounded) {
            player->velocity.y = jumpPower;
            player->isGrounded = false;
        } else if (player->canDoubleJump) {
            player->velocity.y = jumpPower;
            player->canDoubleJump = false;
        }
    }
    
    // Handle attacks - ONE BUTTON, DIRECTION DETERMINES MOVE (like Smash Bros!)
    if (player->attackCooldown == 0 && !player->isChargingSmash && !player->isBlocking && attackPressed) {
        
        // PRIORITY 1: Aerial attacks (ONLY in air)
        if (!player->isGrounded) {
            // Up Air (Up + Attack in air)
            if (moveUp) {
                player->isAttacking = true;
                player->currentAttack = ATTACK_UP_AIR;
                player->attackCooldown = 30;
            }
            // Down Air (Down + Attack in air)
            else if (moveDown) {
                player->isAttacking = true;
                player->currentAttack = ATTACK_DOWN_AIR;
                player->attackCooldown = 30;
            }
            // Normal Air Attack (no direction in air)
            else {
                player->isAttacking = true;
                player->currentAttack = ATTACK_NORMAL;
                player->attackCooldown = 30;
            }
        }
        // PRIORITY 2: Smash Attack (ONLY on ground with direction)
        else if (player->isGrounded && isSmashing) {
            player->isChargingSmash = true;
            player->smashChargeTime = 0;
            player->smashIsDown = moveDown; // Track if it's a down smash
        }
        // PRIORITY 3: Normal Attack (on ground, no direction or just tapped)
        else {
            player->isAttacking = true;
            player->currentAttack = ATTACK_NORMAL;
            player->attackCooldown = 30;
        }
    }
    
    // Auto-release smash after max charge
    if (player->isChargingSmash && player->smashChargeTime >= 120) {
        player->isAttacking = true;
        player->currentAttack = ATTACK_SMASH;
        player->attackCooldown = 40;
        player->isChargingSmash = false;
    }
    
    // Manual release smash when attack button released
    if (player->isChargingSmash && !attackPressed) {
        if (IsGamepadAvailable(player->gamepadId)) {
            if (IsGamepadButtonReleased(player->gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) {
                player->isAttacking = true;
                player->currentAttack = ATTACK_SMASH;
                player->attackCooldown = 40;
                player->isChargingSmash = false;
            }
        } else {
            // Keyboard release
            if ((playerNum == 1 && IsKeyReleased(KEY_SPACE)) ||
                (playerNum == 2 && IsKeyReleased(KEY_RIGHT_CONTROL))) {
                player->isAttacking = true;
                player->currentAttack = ATTACK_SMASH;
                player->attackCooldown = 40;
                player->isChargingSmash = false;
            }
        }
    }
    
    // Reset attack state
    if (player->attackCooldown == 0 && !player->isAttacking) {
        player->currentAttack = ATTACK_NONE;
    }
}

void DrawPlayer(Player* player) {
    if (!player->isAlive) return;
    
    // Flash during respawn invincibility
    if (player->respawnInvincibility > 0 && player->respawnInvincibility % 10 < 5) {
        return; // Don't draw to create flashing effect
    }
    
    // Draw shield bubble
    if (player->isBlocking) {
        float shieldAlpha = player->shieldHealth / 100.0f;
        Color shieldColor = {100, 200, 255, (unsigned char)(180 * shieldAlpha)};
        DrawCircle((int)(player->position.x + player->size.x/2), 
                   (int)(player->position.y + player->size.y/2), 
                   40, Fade(shieldColor, 0.6f));
        DrawCircleLines((int)(player->position.x + player->size.x/2), 
                        (int)(player->position.y + player->size.y/2), 
                        40, shieldColor);
    }
    
    // Flash white during hitstun
    Color drawColor = player->color;
    if (player->hitstun > 0 && player->hitstun % 4 < 2) {
        drawColor = WHITE;
    }
    
    // Draw character body
    DrawRectangleV(player->position, player->size, drawColor);
    
    // Draw charging smash effect
    if (player->isChargingSmash) {
        int pulseSize = (int)(5 + (player->smashChargeTime % 20) * 0.5f);
        Color chargeColor = ORANGE;
        if (player->smashChargeTime > 60) chargeColor = RED;
        if (player->smashChargeTime >= 120) chargeColor = MAROON;
        
        DrawRectangleLinesEx(
            (Rectangle){player->position.x - pulseSize, player->position.y - pulseSize,
                       player->size.x + pulseSize*2, player->size.y + pulseSize*2},
            3, chargeColor);
        
        // Charge particles
        for (int i = 0; i < 3; i++) {
            int offset = (player->smashChargeTime * (i + 1) * 3) % 360;
            float angle = offset * DEG2RAD;
            int px = (int)(player->position.x + player->size.x/2 + cosf(angle) * 30);
            int py = (int)(player->position.y + player->size.y/2 + sinf(angle) * 30);
            DrawCircle(px, py, 3, chargeColor);
        }
    }
    
    // Draw facing indicator (eye)
    float eyeX = player->facingRight ? player->position.x + 30 : player->position.x + 10;
    DrawCircle((int)eyeX, (int)(player->position.y + 15), 4, BLACK);
    
    // Draw attack hitbox with different colors
    if (player->isAttacking && player->attackCooldown > 20) {
        Color hitboxColor = RED;
        if (player->currentAttack == ATTACK_UP_AIR) {
            hitboxColor = YELLOW; // Up air is yellow
        } else if (player->currentAttack == ATTACK_DOWN_AIR) {
            hitboxColor = PURPLE; // Down air is purple
        } else if (player->currentAttack == ATTACK_SMASH) {
            hitboxColor = ORANGE; // Smash is orange
        }
        DrawRectangleRec(player->attackBox, Fade(hitboxColor, 0.5f));
    }
    
    // Draw smash charge indicator above player
    if (player->isChargingSmash) {
        char chargeText[20];
        sprintf(chargeText, "%.1fx", player->smashChargePower);
        DrawText(chargeText, (int)player->position.x, (int)player->position.y - 25, 18, ORANGE);
    }
}

void DrawStage(Stage* stage) {
    // Draw platforms with outlines
    for (int i = 0; i < stage->platformCount; i++) {
        Platform* p = &stage->platforms[i];
        
        // Draw platform
        DrawRectangleV(p->position, p->size, p->color);
        
        // Draw outline
        DrawRectangleLinesEx(
            (Rectangle){p->position.x, p->position.y, p->size.x, p->size.y},
            2,
            ColorBrightness(p->color, -0.3f)
        );
        
        // Draw top highlight
        DrawRectangle((int)p->position.x, (int)p->position.y, (int)p->size.x, 3, ColorBrightness(p->color, 0.3f));
    }
}

void DrawUI(Player* players, int playerCount, int screenWidth, int screenHeight) {
    // Draw clean HUD at top
    int centerX = screenWidth / 2;
    
    // Player 1 HUD (left side)
    if (players[0].isAlive) {
        int p1X = centerX - 300;
        int hudY = 20;
        
        // Background panel
        DrawRectangle(p1X - 10, hudY - 10, 250, 90, Fade(BLACK, 0.8f));
        DrawRectangleLines(p1X - 10, hudY - 10, 250, 90, players[0].color);
        
        // Player indicator
        DrawText("P1", p1X, hudY, 35, players[0].color);
        
        // Stock lives
        DrawText("LIVES:", p1X, hudY + 40, 18, GRAY);
        for (int j = 0; j < players[0].stocks; j++) {
            DrawCircle(p1X + 70 + (j * 25), hudY + 48, 10, players[0].color);
        }
        
        // Shield bar
        DrawText("SHIELD", p1X, hudY + 65, 16, GRAY);
        int shieldWidth = (int)(140 * (players[0].shieldHealth / 100.0f));
        Color shieldColor = (Color){100, 200, 255, 255};
        if (players[0].shieldHealth < 50) shieldColor = YELLOW;
        if (players[0].shieldHealth < 25) shieldColor = RED;
        DrawRectangle(p1X + 70, hudY + 68, 140, 10, DARKGRAY);
        DrawRectangle(p1X + 70, hudY + 68, shieldWidth, 10, shieldColor);
        DrawRectangleLines(p1X + 70, hudY + 68, 140, 10, WHITE);
        DrawText(TextFormat("%.0f", players[0].shieldHealth), p1X + 215, hudY + 66, 12, WHITE);
    }
    
    // Player 2 HUD (right side)
    if (players[1].isAlive) {
        int p2X = centerX + 50;
        int hudY = 20;
        
        // Background panel
        DrawRectangle(p2X - 10, hudY - 10, 250, 90, Fade(BLACK, 0.8f));
        DrawRectangleLines(p2X - 10, hudY - 10, 250, 90, players[1].color);
        
        // Player indicator
        DrawText("P2", p2X, hudY, 35, players[1].color);
        
        // Stock lives
        DrawText("LIVES:", p2X, hudY + 40, 18, GRAY);
        for (int j = 0; j < players[1].stocks; j++) {
            DrawCircle(p2X + 70 + (j * 25), hudY + 48, 10, players[1].color);
        }
        
        // Shield bar
        DrawText("SHIELD", p2X, hudY + 65, 16, GRAY);
        int shieldWidth = (int)(140 * (players[1].shieldHealth / 100.0f));
        Color shieldColor = (Color){100, 200, 255, 255};
        if (players[1].shieldHealth < 50) shieldColor = YELLOW;
        if (players[1].shieldHealth < 25) shieldColor = RED;
        DrawRectangle(p2X + 70, hudY + 68, 140, 10, DARKGRAY);
        DrawRectangle(p2X + 70, hudY + 68, shieldWidth, 10, shieldColor);
        DrawRectangleLines(p2X + 70, hudY + 68, 140, 10, WHITE);
        DrawText(TextFormat("%.0f", players[1].shieldHealth), p2X + 215, hudY + 66, 12, WHITE);
    }
    
    // VS indicator in center
    DrawText("VS", centerX - 25, 35, 40, WHITE);
    
    // PERCENTAGE AT BOTTOM (BIG!)
    if (players[0].isAlive) {
        DrawRectangle(50, screenHeight - 80, 160, 60, Fade(BLACK, 0.8f));
        DrawRectangleLines(50, screenHeight - 80, 160, 60, players[0].color);
        DrawText("P1", 60, screenHeight - 75, 20, players[0].color);
        DrawText(TextFormat("%.0f%%", players[0].percent), 110, screenHeight - 70, 50, WHITE);
    }
    
    if (players[1].isAlive) {
        DrawRectangle(screenWidth - 210, screenHeight - 80, 160, 60, Fade(BLACK, 0.8f));
        DrawRectangleLines(screenWidth - 210, screenHeight - 80, 160, 60, players[1].color);
        DrawText("P2", screenWidth - 200, screenHeight - 75, 20, players[1].color);
        DrawText(TextFormat("%.0f%%", players[1].percent), screenWidth - 150, screenHeight - 70, 50, WHITE);
    }
}

void DetectGamepads(Player* players, int playerCount) {
    static bool gamepadAssigned[MAX_PLAYERS] = {false};
    static int assignedCount = 0;
    
    // Check all possible gamepads
    for (int gamepad = 0; gamepad < 4; gamepad++) {
        if (IsGamepadAvailable(gamepad)) {
            // Check if this gamepad is already assigned
            bool alreadyAssigned = false;
            for (int i = 0; i < playerCount; i++) {
                if (players[i].gamepadId == gamepad) {
                    alreadyAssigned = true;
                    break;
                }
            }
            
            // Assign to first available player
            if (!alreadyAssigned && assignedCount < playerCount) {
                for (int i = 0; i < playerCount; i++) {
                    if (!gamepadAssigned[i]) {
                        players[i].gamepadId = gamepad;
                        gamepadAssigned[i] = true;
                        assignedCount++;
                        TraceLog(LOG_INFO, TextFormat("🎮 Gamepad %d (%s) assigned to Player %d", 
                            gamepad, GetGamepadName(gamepad), i + 1));
                        break;
                    }
                }
            }
        }
    }
}

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    InitWindow(screenWidth, screenHeight, "Smash Bros - Platform Fighter [PS5 Ready]");
    SetTargetFPS(60);
    
    Stage stage;
    InitStage(&stage, screenWidth, screenHeight);
    
    Player players[2];
    InitPlayer(&players[0], 1, (Vector2){(float)screenWidth/2.0f - 100, 100}, BLUE, -1);
    InitPlayer(&players[1], 2, (Vector2){(float)screenWidth/2.0f + 100, 100}, RED, -1);
    
    // Initial gamepad detection
    TraceLog(LOG_INFO, "🎮 Scanning for PS5 controllers...");
    DetectGamepads(players, 2);
    
    // Show initial controller status
    for (int i = 0; i < 2; i++) {
        if (IsGamepadAvailable(players[i].gamepadId)) {
            TraceLog(LOG_INFO, TextFormat("✅ Player %d: %s (Gamepad %d)", 
                i + 1, GetGamepadName(players[i].gamepadId), players[i].gamepadId));
        } else {
            TraceLog(LOG_INFO, TextFormat("⌨️  Player %d: Keyboard only", i + 1));
        }
    }
    
    bool gameOver = false;
    int winner = -1;
    
    while (!WindowShouldClose()) {
        // Detect gamepad connections (hot-plug support)
        DetectGamepads(players, 2);
        
        // Update
        if (!gameOver) {
            for (int i = 0; i < 2; i++) {
                HandlePlayerInput(&players[i]);
                UpdatePlayer(&players[i], &stage, screenWidth, screenHeight);
            }
            
            // Handle attacks for both players
            for (int i = 0; i < 2; i++) {
                Player opponents[1];
                opponents[0] = players[1 - i];
                HandleAttack(&players[i], opponents, 1);
                players[1 - i] = opponents[0];
            }
            
            // Check for game over
            int alivePlayers = 0;
            for (int i = 0; i < 2; i++) {
                if (players[i].isAlive) {
                    alivePlayers++;
                    winner = i;
                }
            }
            if (alivePlayers <= 1) {
                gameOver = true;
            }
        }
        
        // Draw
        BeginDrawing();
        ClearBackground((Color){135, 206, 235, 255}); // Sky blue
        
        DrawStage(&stage);
        
        for (int i = 0; i < 2; i++) {
            DrawPlayer(&players[i]);
        }
        
        DrawUI(players, 2, screenWidth, screenHeight);
        
        if (gameOver) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
            const char* text = TextFormat("PLAYER %d WINS!", players[winner].playerNum);
            int textWidth = MeasureText(text, 60);
            DrawText(text, screenWidth/2 - textWidth/2, screenHeight/2 - 30, 60, players[winner].color);
            
            const char* restart = "Press R or START to Restart";
            int restartWidth = MeasureText(restart, 30);
            DrawText(restart, screenWidth/2 - restartWidth/2, screenHeight/2 + 50, 30, WHITE);
            
            // Restart with R key or START button
            bool restartPressed = IsKeyPressed(KEY_R);
            for (int i = 0; i < 2; i++) {
                if (IsGamepadAvailable(players[i].gamepadId)) {
                    if (IsGamepadButtonPressed(players[i].gamepadId, GAMEPAD_BUTTON_MIDDLE_RIGHT)) {
                        restartPressed = true;
                    }
                }
            }
            
            if (restartPressed) {
                // Preserve gamepad assignments
                int p1GamepadId = players[0].gamepadId;
                int p2GamepadId = players[1].gamepadId;
                InitPlayer(&players[0], 1, (Vector2){(float)screenWidth/2.0f - 100, 100}, BLUE, p1GamepadId);
                InitPlayer(&players[1], 2, (Vector2){(float)screenWidth/2.0f + 100, 100}, RED, p2GamepadId);
                gameOver = false;
                winner = -1;
            }
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}
