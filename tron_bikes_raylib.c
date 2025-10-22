#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define GRID_SIZE 100
#define CELL_SIZE 2
#define MAX_TRAIL_SEGMENTS 5000
#define MAX_OCCUPIED_CELLS 50000
#define INITIAL_SPEED 1.5f
#define BOOST_MULTIPLIER 2.5f
#define TURN_COOLDOWN 5
#define MAX_PICKUPS 10
#define PICKUP_SPEED_BOOST 1.4f
#define PICKUP_DURATION 300  // 5 seconds at 60fps

// Direction enum
typedef enum {
    DIR_NORTH = 0,
    DIR_SOUTH,
    DIR_EAST,
    DIR_WEST
} Direction;

// Trail segment
typedef struct {
    Vector3 start;
    Vector3 end;
    Color color;
    bool active;
} TrailSegment;

// Occupied cell for collision
typedef struct {
    int x;
    int z;
    bool occupied;
} OccupiedCell;

// Speed pickup
typedef struct {
    Vector3 position;
    bool active;
    float rotationAngle;
    Color color;
} Pickup;

// Bike structure
typedef struct {
    Vector3 position;
    Direction direction;
    float speed;
    bool alive;
    int turnCooldown;
    Color color;
    bool isPlayer;
    bool boosting;
    int boostTimer;          // Frames remaining for boost (3 seconds = 180 frames at 60fps)
    int boostCooldownTimer;  // Frames remaining for cooldown (3 seconds = 180 frames)
    int pickupSpeedTimer;    // Frames remaining for pickup speed boost

    TrailSegment trail[MAX_TRAIL_SEGMENTS];
    int trailCount;
    Vector3 currentTrailStart;

    OccupiedCell occupiedCells[MAX_OCCUPIED_CELLS];
    int occupiedCount;

    // AI specific
    int aiThinkTimer;
    int aiThinkInterval;
} Bike;

// Game mode enum
typedef enum {
    MENU,
    SINGLEPLAYER,
    SPLITSCREEN
} GameMode;

// Game state
typedef struct {
    Bike bikes[4];
    int bikeCount;
    bool gameOver;
    bool playerWon;
    Music bgm;
    Sound sndTurn;
    Sound sndBoost;
    Sound sndExplosion;
    Sound sndMenuSelect;
    Model bikeModel;
    GameMode mode;
    int menuSelection;
    Pickup pickups[MAX_PICKUPS];
    int pickupSpawnTimer;
} GameState;

// Function prototypes
void InitBike(Bike* bike, float x, float z, Color color, bool isPlayer, Direction dir);
void UpdateBike(Bike* bike, float deltaTime);
void DrawBike(Bike* bike);
void DrawTrail(Bike* bike);
void TurnBike(Bike* bike, Direction newDir, GameState* state);
void BoostBike(Bike* bike, bool active, GameState* state);
void AddOccupiedCell(Bike* bike, int cellX, int cellZ);
bool CheckCollision(Bike* bike, GameState* state);
Vector3 GetDirectionVector(Direction dir);
Direction GetOppositeDirection(Direction dir);
void UpdateAI(Bike* bike, GameState* state);
void InitGame(GameState* state, GameMode mode);
void DrawMenu(GameState* state, int screenWidth, int screenHeight);
void DrawTronGrid(void);
void DrawWalls(void);
void DrawMinimap(GameState* state, int x, int y, int size);
void SpawnPickup(GameState* state);
void UpdatePickups(GameState* state);
void DrawPickups(GameState* state);
void CheckPickupCollision(Bike* bike, GameState* state);
void DrawSpeedometer(Bike* bike, int x, int y, int size);

// Get direction vector from enum
Vector3 GetDirectionVector(Direction dir) {
    switch(dir) {
        case DIR_NORTH: return (Vector3){0, 0, -1};
        case DIR_SOUTH: return (Vector3){0, 0, 1};
        case DIR_EAST: return (Vector3){1, 0, 0};
        case DIR_WEST: return (Vector3){-1, 0, 0};
        default: return (Vector3){0, 0, 0};
    }
}

// Get opposite direction
Direction GetOppositeDirection(Direction dir) {
    switch(dir) {
        case DIR_NORTH: return DIR_SOUTH;
        case DIR_SOUTH: return DIR_NORTH;
        case DIR_EAST: return DIR_WEST;
        case DIR_WEST: return DIR_EAST;
        default: return dir;
    }
}

// Initialize bike
void InitBike(Bike* bike, float x, float z, Color color, bool isPlayer, Direction dir) {
    bike->position = (Vector3){x, 0, z};
    bike->direction = dir;
    bike->speed = INITIAL_SPEED;
    bike->alive = true;
    bike->turnCooldown = 0;
    bike->color = color;
    bike->isPlayer = isPlayer;
    bike->boosting = false;
    bike->boostTimer = 0;
    bike->boostCooldownTimer = 0;
    bike->pickupSpeedTimer = 0;
    bike->trailCount = 0;
    bike->currentTrailStart = bike->position;
    bike->occupiedCount = 0;

    // AI specific initialization
    bike->aiThinkTimer = GetRandomValue(0, 20); // Random offset so AIs don't all think at once
    bike->aiThinkInterval = 25 + GetRandomValue(0, 15); // 25-40 frames between decisions

    // Add initial occupied cell
    AddOccupiedCell(bike, (int)(x / CELL_SIZE), (int)(z / CELL_SIZE));
}

// Add occupied cell
void AddOccupiedCell(Bike* bike, int cellX, int cellZ) {
    if (bike->occupiedCount < MAX_OCCUPIED_CELLS) {
        bike->occupiedCells[bike->occupiedCount].x = cellX;
        bike->occupiedCells[bike->occupiedCount].z = cellZ;
        bike->occupiedCells[bike->occupiedCount].occupied = true;
        bike->occupiedCount++;
    }
}

// Turn bike
void TurnBike(Bike* bike, Direction newDir, GameState* state) {
    if (bike->turnCooldown > 0) return;
    if (newDir == GetOppositeDirection(bike->direction)) return; // No 180 turns
    if (newDir == bike->direction) return; // Already going this way
    
    // Create trail segment for the line we just drew
    Vector3 start = bike->currentTrailStart;
    Vector3 end = bike->position;
    float distance = Vector3Distance(start, end);
    
    if (distance > 0.1f && bike->trailCount < MAX_TRAIL_SEGMENTS) {
        bike->trail[bike->trailCount].start = start;
        bike->trail[bike->trailCount].end = end;
        bike->trail[bike->trailCount].color = bike->color;
        bike->trail[bike->trailCount].active = true;
        bike->trailCount++;
    }
    
    bike->direction = newDir;
    bike->currentTrailStart = bike->position;
    bike->turnCooldown = TURN_COOLDOWN;
    
    // Play turn sound if player bike
    if (bike->isPlayer && IsSoundValid(state->sndTurn)) {
        PlaySound(state->sndTurn);
    }
}

// Boost bike - now with timer and cooldown
void BoostBike(Bike* bike, bool wantsBoost, GameState* state) {
    // If player wants to boost and not on cooldown, activate boost
    if (wantsBoost && !bike->boosting && bike->boostCooldownTimer == 0) {
        bike->boosting = true;
        bike->boostTimer = 180; // 3 seconds at 60fps
        bike->speed = INITIAL_SPEED * BOOST_MULTIPLIER;

        // Play boost sound
        if (bike->isPlayer && IsSoundValid(state->sndBoost)) {
            PlaySound(state->sndBoost);
        }
    }
}

// Update bike
void UpdateBike(Bike* bike, float deltaTime) {
    if (!bike->alive) return;

    if (bike->turnCooldown > 0) bike->turnCooldown--;

    // Handle boost timer and cooldown
    if (bike->boosting) {
        bike->boostTimer--;
        if (bike->boostTimer <= 0) {
            // Boost expired, deactivate and start cooldown
            bike->boosting = false;
            bike->boostTimer = 0;
            bike->boostCooldownTimer = 180; // 3 seconds cooldown
        }
    } else if (bike->boostCooldownTimer > 0) {
        bike->boostCooldownTimer--;
    }

    // Handle pickup speed boost timer
    if (bike->pickupSpeedTimer > 0) {
        bike->pickupSpeedTimer--;
    }

    // Calculate current speed (base + modifiers)
    float currentSpeed = INITIAL_SPEED;
    if (bike->boosting) {
        currentSpeed *= BOOST_MULTIPLIER;
    }
    if (bike->pickupSpeedTimer > 0) {
        currentSpeed *= PICKUP_SPEED_BOOST;
    }
    bike->speed = currentSpeed;

    // TIME-BASED MOVEMENT: Multiply by 60 * deltaTime to make it framerate independent
    Vector3 dirVec = GetDirectionVector(bike->direction);
    dirVec = Vector3Scale(dirVec, bike->speed * 12.0f * deltaTime);  // Slower speed
    bike->position = Vector3Add(bike->position, dirVec);
    
    // Add occupied cell every frame
    int cellX = (int)(bike->position.x / CELL_SIZE);
    int cellZ = (int)(bike->position.z / CELL_SIZE);
    AddOccupiedCell(bike, cellX, cellZ);
    
    // Check bounds - with larger buffer so walls are visible when you die
    float halfSize = (GRID_SIZE * CELL_SIZE) / 2.0f - 5.0f; // 5 unit buffer from edge
    if (fabs(bike->position.x) > halfSize || fabs(bike->position.z) > halfSize) {
        TraceLog(LOG_INFO, "WALL COLLISION: Bike hit boundary at (%.1f, %.1f)", bike->position.x, bike->position.z);
        bike->alive = false;
    }
}

// Check collision - OPTIMIZED
bool CheckCollision(Bike* bike, GameState* state) {
    if (!bike->alive) return false;

    int cellX = (int)(bike->position.x / CELL_SIZE);
    int cellZ = (int)(bike->position.z / CELL_SIZE);

    // Check against all other bikes' occupied cells
    for (int i = 0; i < state->bikeCount; i++) {
        Bike* other = &state->bikes[i];

        // MAJOR OPTIMIZATION: Only check cells in a local area around the bike
        int startCheck = 0;
        int endCheck;

        if (other == bike) {
            // For your own trail: skip last 300 cells to prevent hitting your immediate trail
            endCheck = (other->occupiedCount > 300) ? (other->occupiedCount - 300) : 0;
            // Only check the last 2000 cells instead of all cells
            if (endCheck > 2000) startCheck = endCheck - 2000;
        } else {
            // For other bikes: skip last 30 cells to avoid false positives at their head position
            endCheck = (other->occupiedCount > 30) ? (other->occupiedCount - 30) : 0;
            // Only check the last 3000 cells instead of all cells
            if (endCheck > 3000) startCheck = endCheck - 3000;
        }

        // Check only relevant cells with distance culling
        for (int j = startCheck; j < endCheck; j++) {
            if (!other->occupiedCells[j].occupied) continue;

            // Quick distance check before exact comparison
            int dx = abs(other->occupiedCells[j].x - cellX);
            int dz = abs(other->occupiedCells[j].z - cellZ);
            if (dx > 2 || dz > 2) continue; // Skip cells too far away

            if (other->occupiedCells[j].x == cellX &&
                other->occupiedCells[j].z == cellZ) {
                TraceLog(LOG_INFO, "COLLISION: Bike %p hit trail at cell (%d, %d)", (void*)bike, cellX, cellZ);
                bike->alive = false;
                return true;
            }
        }
    }

    return false;
}

// Check if a path is clear (for AI)
bool IsPathClear(Vector3 pos, Direction dir, float distance, GameState* state) {
    Vector3 dirVec = GetDirectionVector(dir);
    float halfSize = (GRID_SIZE * CELL_SIZE) / 2.0f - 5.0f;

    // Check multiple points along the path
    for (float d = 2.0f; d < distance; d += 2.0f) {
        Vector3 testPos = Vector3Add(pos, Vector3Scale(dirVec, d));

        // Check bounds
        if (fabs(testPos.x) > halfSize || fabs(testPos.z) > halfSize) {
            return false;
        }

        // Check for trails
        int cellX = (int)(testPos.x / CELL_SIZE);
        int cellZ = (int)(testPos.z / CELL_SIZE);

        for (int i = 0; i < state->bikeCount; i++) {
            Bike* other = &state->bikes[i];
            int checkLimit = (other->occupiedCount > 50) ? (other->occupiedCount - 50) : 0;

            for (int j = 0; j < checkLimit; j++) {
                if (other->occupiedCells[j].occupied &&
                    other->occupiedCells[j].x == cellX &&
                    other->occupiedCells[j].z == cellZ) {
                    return false;
                }
            }
        }
    }
    return true;
}

// Update AI - Improved intelligence
void UpdateAI(Bike* bike, GameState* state) {
    if (!bike->alive) return;

    bike->aiThinkTimer++;
    if (bike->aiThinkTimer < bike->aiThinkInterval) return;
    bike->aiThinkTimer = 0;

    Direction directions[] = {DIR_NORTH, DIR_SOUTH, DIR_EAST, DIR_WEST};
    int scores[4] = {0, 0, 0, 0};
    int bestScore = -1;
    Direction bestDir = bike->direction;

    // Score each direction
    for (int i = 0; i < 4; i++) {
        Direction dir = directions[i];

        // Can't go backwards
        if (dir == GetOppositeDirection(bike->direction)) {
            scores[i] = -1000;
            continue;
        }

        // Check immediate path (30 units)
        if (!IsPathClear(bike->position, dir, 30.0f, state)) {
            scores[i] = -100;
            continue;
        }

        // Bonus for continuing straight (reduces erratic turning)
        if (dir == bike->direction) {
            scores[i] += 20;
        }

        // Check longer path (50 units) - more score for more space
        if (IsPathClear(bike->position, dir, 50.0f, state)) {
            scores[i] += 30;
        }

        // Check even longer path (70 units)
        if (IsPathClear(bike->position, dir, 70.0f, state)) {
            scores[i] += 20;
        }

        // Small random factor to avoid predictability
        scores[i] += GetRandomValue(0, 10);

        // Track best direction
        if (scores[i] > bestScore) {
            bestScore = scores[i];
            bestDir = dir;
        }
    }

    // Make a turn if best direction is different and has positive score
    if (bestDir != bike->direction && bestScore > 0) {
        TurnBike(bike, bestDir, state);
    } else if (bestScore < 0) {
        // Emergency: all paths blocked, try any valid direction
        for (int i = 0; i < 4; i++) {
            if (directions[i] != GetOppositeDirection(bike->direction)) {
                TurnBike(bike, directions[i], state);
                break;
            }
        }
    }
}

// Draw bike with 3D model
void DrawBikeModel(Bike* bike, Model* bikeModel) {
    if (!bike->alive) return;
    
    Vector3 modelPos = bike->position;
    modelPos.y = 0.0f;
    
    // Calculate rotation based on direction
    // Adjust these angles based on model's default facing direction
    float rotation = 0.0f;
    switch(bike->direction) {
        case DIR_NORTH: rotation = 180.0f; break;   // Facing -Z
        case DIR_SOUTH: rotation = 0.0f; break;      // Facing +Z
        case DIR_EAST: rotation = 90.0f; break;      // Facing +X
        case DIR_WEST: rotation = -90.0f; break;     // Facing -X
    }
    
    // Set model color by modifying materials
    Color tintColor = bike->color;
    if (!bike->alive) {
        tintColor.a = 80;
    }
    
    // Draw model with rotation and color
    DrawModelEx(*bikeModel, modelPos, (Vector3){0, 1, 0}, rotation,
                (Vector3){5.0f, 5.0f, 5.0f}, tintColor);
}

// Draw bike (fallback if model not loaded)
void DrawBike(Bike* bike) {
    if (!bike->alive) return;
    
    // Main body
    Vector3 bodyPos = bike->position;
    bodyPos.y = 1.0f;
    
    Color bodyColor = bike->color;
    if (!bike->alive) {
        bodyColor.a = 80;
    }
    
    DrawCube(bodyPos, 2.0f, 1.5f, 4.0f, bodyColor);
    DrawCubeWires(bodyPos, 2.0f, 1.5f, 4.0f, WHITE);

    // Front wedge
    Vector3 dirVec = GetDirectionVector(bike->direction);
    Vector3 wedgePos = Vector3Add(bodyPos, Vector3Scale(dirVec, 2.5f));
    wedgePos.y = 1.0f;

    DrawCube(wedgePos, 1.5f, 1.0f, 1.5f, bodyColor);
}

// Draw trail - continuous stream - OPTIMIZED
void DrawTrail(Bike* bike) {
    Color trailColor = bike->color;
    if (!bike->alive) {
        trailColor.a = 80;
    }

    // Draw all completed segments (REDUCED glow layers from 3 to 2 for performance)
    for (int i = 0; i < bike->trailCount; i++) {
        if (!bike->trail[i].active) continue;

        Vector3 start = bike->trail[i].start;
        Vector3 end = bike->trail[i].end;
        start.y = 0;
        end.y = 0;

        float distance = Vector3Distance(start, end);
        if (distance < 0.01f) continue;

        Vector3 midpoint = Vector3Scale(Vector3Add(start, end), 0.5f);
        Vector3 direction = Vector3Subtract(end, start);

        // Draw vertical light wall
        Vector3 wallPos = midpoint;
        wallPos.y = 3.0f; // Center at height 3

        // Determine if this is X-axis or Z-axis aligned
        bool isXAligned = fabs(direction.x) > fabs(direction.z);

        if (isXAligned) {
            // Trail goes along X axis, wall is perpendicular (thin in Z)
            DrawCube(wallPos, distance, 6.0f, 0.5f, trailColor);
            DrawCube(wallPos, distance + 0.8f, 6.8f, 1.2f, Fade(trailColor, 0.25f));
        } else {
            // Trail goes along Z axis, wall is perpendicular (thin in X)
            DrawCube(wallPos, 0.5f, 6.0f, distance, trailColor);
            DrawCube(wallPos, 1.2f, 6.8f, distance + 0.8f, Fade(trailColor, 0.25f));
        }
    }

    // Draw current active segment (from last turn to current position)
    if (bike->alive) {
        Vector3 start = bike->currentTrailStart;
        Vector3 end = bike->position;
        start.y = 0;
        end.y = 0;

        float distance = Vector3Distance(start, end);
        if (distance > 0.01f) {
            Vector3 midpoint = Vector3Scale(Vector3Add(start, end), 0.5f);

            // Draw vertical light wall for current segment
            Vector3 wallPos = midpoint;
            wallPos.y = 3.0f;

            Vector3 dir = Vector3Subtract(end, start);
            bool isXAligned = fabs(dir.x) > fabs(dir.z);

            if (isXAligned) {
                DrawCube(wallPos, distance, 6.0f, 0.5f, trailColor);
                DrawCube(wallPos, distance + 0.8f, 6.8f, 1.2f, Fade(trailColor, 0.25f));
            } else {
                DrawCube(wallPos, 0.5f, 6.0f, distance, trailColor);
                DrawCube(wallPos, 1.2f, 6.8f, distance + 0.8f, Fade(trailColor, 0.25f));
            }
        }
    }
}

// Draw grid - OPTIMIZED (fewer lines)
void DrawTronGrid(void) {
    float halfSize = (GRID_SIZE * CELL_SIZE) / 2.0f;
    int gridLines = GRID_SIZE / 4;  // REDUCED from /2 to /4 for better performance
    float spacing = (GRID_SIZE * CELL_SIZE) / gridLines;

    Color gridColor = (Color){0, 136, 255, 100};

    for (int i = 0; i <= gridLines; i++) {
        float pos = -halfSize + (i * spacing);

        // Lines along X axis
        DrawLine3D((Vector3){-halfSize, 0, pos}, (Vector3){halfSize, 0, pos}, gridColor);

        // Lines along Z axis
        DrawLine3D((Vector3){pos, 0, -halfSize}, (Vector3){pos, 0, halfSize}, gridColor);
    }

    // Draw floor plane
    DrawPlane((Vector3){0, -0.1f, 0}, (Vector2){GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE},
              (Color){0, 26, 51, 150});
}

// Draw walls
void DrawWalls(void) {
    float halfSize = (GRID_SIZE * CELL_SIZE) / 2.0f;
    float wallHeight = 8.0f;
    Color wallColor = (Color){0, 255, 255, 150};
    
    // North wall
    DrawCubeV((Vector3){0, wallHeight/2, -halfSize}, 
              (Vector3){GRID_SIZE * CELL_SIZE + 2, wallHeight, 2}, wallColor);
    
    // South wall
    DrawCubeV((Vector3){0, wallHeight/2, halfSize}, 
              (Vector3){GRID_SIZE * CELL_SIZE + 2, wallHeight, 2}, wallColor);
    
    // West wall
    DrawCubeV((Vector3){-halfSize, wallHeight/2, 0}, 
              (Vector3){2, wallHeight, GRID_SIZE * CELL_SIZE}, wallColor);
    
    // East wall
    DrawCubeV((Vector3){halfSize, wallHeight/2, 0},
              (Vector3){2, wallHeight, GRID_SIZE * CELL_SIZE}, wallColor);
}

// Draw minimap - top-down view of arena with trails and bike positions
void DrawMinimap(GameState* state, int x, int y, int size) {
    float worldSize = GRID_SIZE * CELL_SIZE;
    float scale = size / worldSize;

    int centerX = x + size / 2;
    int centerY = y + size / 2;

    // Draw background
    DrawRectangle(x - 5, y - 5, size + 10, size + 10, Fade(BLACK, 0.8f));
    DrawRectangleLines(x - 5, y - 5, size + 10, size + 10, SKYBLUE);

    // Draw grid outline
    DrawRectangleLines(x, y, size, size, Fade(SKYBLUE, 0.5f));

    // Draw grid lines
    int gridLines = 10;
    float gridSpacing = size / (float)gridLines;
    for (int i = 0; i <= gridLines; i++) {
        float offset = i * gridSpacing;
        // Vertical lines
        DrawLine(x + offset, y, x + offset, y + size, Fade(SKYBLUE, 0.2f));
        // Horizontal lines
        DrawLine(x, y + offset, x + size, y + offset, Fade(SKYBLUE, 0.2f));
    }

    // Draw all bike trails - OPTIMIZED (draw every other segment for performance)
    for (int i = 0; i < state->bikeCount; i++) {
        Bike* bike = &state->bikes[i];

        Color trailColor = bike->color;
        if (!bike->alive) {
            trailColor = Fade(trailColor, 0.3f);
        }

        // Draw completed trail segments (every other one for performance)
        for (int j = 0; j < bike->trailCount; j += 2) {  // SKIP every other segment
            if (!bike->trail[j].active) continue;

            Vector3 start = bike->trail[j].start;
            Vector3 end = bike->trail[j].end;

            // Convert world coordinates to minimap coordinates
            int startX = centerX + (int)(start.x * scale);
            int startY = centerY + (int)(start.z * scale);
            int endX = centerX + (int)(end.x * scale);
            int endY = centerY + (int)(end.z * scale);

            DrawLineEx((Vector2){startX, startY}, (Vector2){endX, endY}, 2.0f, trailColor);
        }

        // Draw current active segment
        if (bike->alive) {
            Vector3 start = bike->currentTrailStart;
            Vector3 end = bike->position;

            int startX = centerX + (int)(start.x * scale);
            int startY = centerY + (int)(start.z * scale);
            int endX = centerX + (int)(end.x * scale);
            int endY = centerY + (int)(end.z * scale);

            DrawLineEx((Vector2){startX, startY}, (Vector2){endX, endY}, 2.0f, trailColor);
        }
    }

    // Draw bike positions (on top of trails)
    for (int i = 0; i < state->bikeCount; i++) {
        Bike* bike = &state->bikes[i];

        if (!bike->alive) continue;

        int bikeX = centerX + (int)(bike->position.x * scale);
        int bikeY = centerY + (int)(bike->position.z * scale);

        // Draw bike as a glowing dot
        Color bikeColor = bike->color;

        if (bike->boosting) {
            // Larger, brighter dot when boosting
            DrawCircle(bikeX, bikeY, 5, bikeColor);
            DrawCircle(bikeX, bikeY, 7, Fade(bikeColor, 0.5f));
        } else {
            DrawCircle(bikeX, bikeY, 4, bikeColor);
            DrawCircle(bikeX, bikeY, 6, Fade(bikeColor, 0.4f));
        }

        // Draw direction indicator (small line showing where bike is heading)
        Vector3 dir = GetDirectionVector(bike->direction);
        int dirEndX = bikeX + (int)(dir.x * 8);
        int dirEndY = bikeY + (int)(dir.z * 8);
        DrawLineEx((Vector2){bikeX, bikeY}, (Vector2){dirEndX, dirEndY}, 2.0f, WHITE);
    }

    // Label
    DrawText("MINIMAP", x, y - 20, 12, SKYBLUE);

    // Draw pickups on minimap
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!state->pickups[i].active) continue;

        int pickupX = centerX + (int)(state->pickups[i].position.x * scale);
        int pickupY = centerY + (int)(state->pickups[i].position.z * scale);

        DrawCircle(pickupX, pickupY, 3, GOLD);
        DrawCircle(pickupX, pickupY, 5, Fade(GOLD, 0.3f));
    }
}

// Spawn a new speed pickup at a random safe location
void SpawnPickup(GameState* state) {
    // Find inactive pickup slot
    int slot = -1;
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!state->pickups[i].active) {
            slot = i;
            break;
        }
    }

    if (slot == -1) return; // No free slots

    // Generate random position (avoid edges)
    float halfSize = (GRID_SIZE * CELL_SIZE) / 2.0f - 20.0f;
    float x = ((float)GetRandomValue(-100, 100) / 100.0f) * halfSize;
    float z = ((float)GetRandomValue(-100, 100) / 100.0f) * halfSize;

    state->pickups[slot].position = (Vector3){x, 2.0f, z};
    state->pickups[slot].active = true;
    state->pickups[slot].rotationAngle = 0.0f;
    state->pickups[slot].color = GOLD;
}

// Update pickups (rotation animation)
void UpdatePickups(GameState* state) {
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!state->pickups[i].active) continue;
        state->pickups[i].rotationAngle += 2.0f;
        if (state->pickups[i].rotationAngle >= 360.0f) {
            state->pickups[i].rotationAngle -= 360.0f;
        }
    }

    // Spawn new pickups periodically
    state->pickupSpawnTimer--;
    if (state->pickupSpawnTimer <= 0) {
        SpawnPickup(state);
        state->pickupSpawnTimer = 180 + GetRandomValue(0, 120); // 3-5 seconds
    }
}

// Draw pickups in 3D
void DrawPickups(GameState* state) {
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!state->pickups[i].active) continue;

        Pickup* pickup = &state->pickups[i];

        // Draw rotating cube
        DrawCubeV(pickup->position, (Vector3){1.5f, 1.5f, 1.5f}, pickup->color);

        // Draw wireframe layers
        DrawCubeWiresV(pickup->position, (Vector3){2.0f, 2.0f, 2.0f}, pickup->color);
        DrawCubeWiresV(pickup->position, (Vector3){2.5f, 2.5f, 2.5f}, Fade(pickup->color, 0.5f));
    }
}

// Check if bike collides with any pickup
void CheckPickupCollision(Bike* bike, GameState* state) {
    if (!bike->alive) return;

    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!state->pickups[i].active) continue;

        float distance = Vector3Distance(bike->position, state->pickups[i].position);

        if (distance < 3.0f) {
            // Collected!
            state->pickups[i].active = false;
            bike->pickupSpeedTimer = PICKUP_DURATION;

            TraceLog(LOG_INFO, "Pickup collected! Speed boost activated");
        }
    }
}

// Draw speedometer - circular gauge showing current speed
void DrawSpeedometer(Bike* bike, int x, int y, int size) {
    // Calculate speed as percentage of max possible speed
    float maxSpeed = INITIAL_SPEED * BOOST_MULTIPLIER * PICKUP_SPEED_BOOST;
    float speedPercent = bike->speed / maxSpeed;

    int centerX = x + size / 2;
    int centerY = y + size / 2;
    int radius = size / 2 - 5;

    // Draw background circle
    DrawCircle(centerX, centerY, radius + 5, Fade(BLACK, 0.8f));
    DrawCircleLines(centerX, centerY, radius + 5, SKYBLUE);

    // Draw speed arc (gauge)
    // Arc goes from 135 degrees to 45 degrees (270 degree range)
    float startAngle = 135.0f;
    float endAngle = 45.0f;
    float angleRange = 270.0f;

    // Current speed angle
    float currentAngle = startAngle + (speedPercent * angleRange);

    // Draw background arc (dark)
    DrawCircleSector((Vector2){centerX, centerY}, radius, startAngle, startAngle + angleRange, 32, Fade(DARKGRAY, 0.5f));

    // Draw speed arc with color based on speed
    Color speedColor = SKYBLUE;
    if (bike->boosting) {
        speedColor = ORANGE;
    } else if (bike->pickupSpeedTimer > 0) {
        speedColor = GOLD;
    }

    if (speedPercent > 0.01f) {
        DrawCircleSector((Vector2){centerX, centerY}, radius, startAngle, currentAngle, 32, Fade(speedColor, 0.7f));
    }

    // Draw tick marks
    for (int i = 0; i <= 10; i++) {
        float tickAngle = (startAngle + (i / 10.0f) * angleRange) * DEG2RAD;
        float innerRadius = radius - 8;
        float outerRadius = radius - 2;

        int x1 = centerX + cosf(tickAngle) * innerRadius;
        int y1 = centerY - sinf(tickAngle) * innerRadius;
        int x2 = centerX + cosf(tickAngle) * outerRadius;
        int y2 = centerY - sinf(tickAngle) * outerRadius;

        DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 2, LIGHTGRAY);
    }

    // Draw needle
    float needleAngle = (startAngle + (speedPercent * angleRange)) * DEG2RAD;
    int needleLength = radius - 10;
    int needleX = centerX + cosf(needleAngle) * needleLength;
    int needleY = centerY - sinf(needleAngle) * needleLength;

    DrawLineEx((Vector2){centerX, centerY}, (Vector2){needleX, needleY}, 3, WHITE);
    DrawCircle(centerX, centerY, 5, WHITE);

    // Draw speed multiplier text
    float speedMultiplier = bike->speed / INITIAL_SPEED;
    const char* speedText = TextFormat("%.1fx", speedMultiplier);
    int textWidth = MeasureText(speedText, 16);
    DrawText(speedText, centerX - textWidth/2, centerY + 15, 16, WHITE);

    // Status indicators
    if (bike->boosting) {
        DrawText("BOOST", centerX - 20, centerY - 10, 12, ORANGE);
    } else if (bike->pickupSpeedTimer > 0) {
        DrawText("PICKUP", centerX - 22, centerY - 10, 12, GOLD);
    }

    // Label
    DrawText("SPEED", x + size/2 - 20, y - 18, 12, SKYBLUE);
}

// Draw menu screen
void DrawMenu(GameState* state, int screenWidth, int screenHeight) {
    const char* title = "TRON LIGHT BIKES";
    int titleWidth = MeasureText(title, 60);
    DrawText(title, screenWidth/2 - titleWidth/2, 100, 60, SKYBLUE);
    
    const char* option1 = "1. SINGLE PLAYER (vs AI)";
    const char* option2 = "2. SPLIT SCREEN (2 Players)";
    
    Color color1 = (state->menuSelection == 0) ? YELLOW : WHITE;
    Color color2 = (state->menuSelection == 1) ? YELLOW : WHITE;
    
    int opt1Width = MeasureText(option1, 30);
    int opt2Width = MeasureText(option2, 30);
    
    DrawText(option1, screenWidth/2 - opt1Width/2, 300, 30, color1);
    DrawText(option2, screenWidth/2 - opt2Width/2, 350, 30, color2);
    
    DrawText("Use UP/DOWN or Left Stick to select", screenWidth/2 - 200, 450, 20, GRAY);
    DrawText("Press ENTER or X to start", screenWidth/2 - 150, 480, 20, GRAY);
}

// Initialize game
void InitGame(GameState* state, GameMode mode) {
    state->mode = mode;
    state->gameOver = false;
    state->playerWon = false;

    // Initialize pickups
    for (int i = 0; i < MAX_PICKUPS; i++) {
        state->pickups[i].active = false;
    }
    state->pickupSpawnTimer = 180; // First pickup spawns after 3 seconds

    // Clear all bikes first
    for (int i = 0; i < 4; i++) {
        state->bikes[i].trailCount = 0;
        state->bikes[i].occupiedCount = 0;
    }
    
    if (mode == SPLITSCREEN) {
        state->bikeCount = 4;
        // Player 1 (cyan)
        InitBike(&state->bikes[0], -50, -50, SKYBLUE, true, DIR_EAST);
        // Player 2 (magenta)
        InitBike(&state->bikes[1], 50, 50, MAGENTA, true, DIR_WEST);
        // NPC 1 (red) - AI opponent
        InitBike(&state->bikes[2], -50, 50, RED, false, DIR_SOUTH);
        // NPC 2 (yellow) - AI opponent
        InitBike(&state->bikes[3], 50, -50, YELLOW, false, DIR_NORTH);
    } else {
        state->bikeCount = 4;
        // Player (cyan)
        InitBike(&state->bikes[0], -30, -30, SKYBLUE, true, DIR_SOUTH);
        // NPC 1 (red)
        InitBike(&state->bikes[1], 30, 30, RED, false, DIR_NORTH);
        // NPC 2 (green)
        InitBike(&state->bikes[2], -30, 30, GREEN, false, DIR_EAST);
        // NPC 3 (yellow)
        InitBike(&state->bikes[3], 30, -30, YELLOW, false, DIR_WEST);
    }
}

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 800;
    
    InitWindow(screenWidth, screenHeight, "Tron Light Bikes - Raylib");
    InitAudioDevice();
    
    SetTargetFPS(60);
    
    // Camera setup
    Camera3D camera = {0};
    camera.position = (Vector3){0.0f, 25.0f, 30.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 75.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    // Load music and sounds
    GameState state = {0};
    if (FileExists("tron_bgm.mp3")) {
        state.bgm = LoadMusicStream("tron_bgm.mp3");
        PlayMusicStream(state.bgm);
        SetMusicVolume(state.bgm, 0.6f);
    }
    
    // Load 3D bike model
    bool hasModel = false;
    if (FileExists("tron_light_cycle.glb")) {
        state.bikeModel = LoadModel("tron_light_cycle.glb");
        if (IsModelValid(state.bikeModel)) {
            hasModel = true;
            TraceLog(LOG_INFO, "3D bike model loaded successfully!");
        }
    }
    
    if (!hasModel) {
        TraceLog(LOG_WARNING, "3D model not loaded, using fallback cubes");
    }
    
    // Load sound effects
    if (FileExists("bike_turn.mp3")) {
        state.sndTurn = LoadSound("bike_turn.mp3");
        SetSoundVolume(state.sndTurn, 0.5f);
    }
    if (FileExists("bike_boost.mp3")) {
        state.sndBoost = LoadSound("bike_boost.mp3");
        SetSoundVolume(state.sndBoost, 0.7f);
    }
    if (FileExists("bike_explosion.mp3")) {
        state.sndExplosion = LoadSound("bike_explosion.mp3");
        SetSoundVolume(state.sndExplosion, 0.8f);
    }
    if (FileExists("menu_select.mp3")) {
        state.sndMenuSelect = LoadSound("menu_select.mp3");
        SetSoundVolume(state.sndMenuSelect, 0.6f);
    }
    
    // Start in menu
    state.mode = MENU;
    state.menuSelection = 0;
    
    // PS5 controller support
    int gamepadId = -1;
    int gamepadId2 = -1;

    // Split screen render textures
    RenderTexture screenPlayer1 = LoadRenderTexture(screenWidth/2, screenHeight);
    RenderTexture screenPlayer2 = LoadRenderTexture(screenWidth/2, screenHeight);
    Rectangle splitScreenRect = { 0.0f, 0.0f, (float)screenPlayer1.texture.width, (float)-screenPlayer1.texture.height };
    
    // Main game loop
    while (!WindowShouldClose()) {
        // Update music
        if (IsMusicValid(state.bgm)) {
            UpdateMusicStream(state.bgm);
        }
        
        // Check for connected gamepads
        if (gamepadId == -1 || gamepadId2 == -1) {
            for (int i = 0; i < 4; i++) {
                if (IsGamepadAvailable(i)) {
                    if (gamepadId == -1) {
                        gamepadId = i;
                        TraceLog(LOG_INFO, "Gamepad 1 detected: %s", GetGamepadName(i));
                    } else if (gamepadId2 == -1 && i != gamepadId) {
                        gamepadId2 = i;
                        TraceLog(LOG_INFO, "Gamepad 2 detected: %s", GetGamepadName(i));
                    }
                }
            }
        }
        
        // Menu controls
        if (state.mode == MENU) {
            if (IsKeyPressed(KEY_UP) || (gamepadId >= 0 && IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_LEFT_FACE_UP))) {
                state.menuSelection = 0;
            }
            if (IsKeyPressed(KEY_DOWN) || (gamepadId >= 0 && IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_LEFT_FACE_DOWN))) {
                state.menuSelection = 1;
            }
            if (IsKeyPressed(KEY_ENTER) || (gamepadId >= 0 && IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) {
                GameMode selectedMode = (state.menuSelection == 0) ? SINGLEPLAYER : SPLITSCREEN;
                InitGame(&state, selectedMode);
            }
        }
        
        // Restart game
        if (IsKeyPressed(KEY_R) || 
            (gamepadId >= 0 && IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_UP))) {
            if (state.mode == MENU) {
                // Already in menu
            } else {
                InitGame(&state, state.mode);
            }
        }
        
        // Back to menu
        if (IsKeyPressed(KEY_ESCAPE) || (gamepadId >= 0 && IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_MIDDLE_LEFT))) {
            state.mode = MENU;
        }
        
        if (!state.gameOver) {
            // Player input
            Bike* player = &state.bikes[0];
            if (player->alive) {
                // Keyboard input - RELATIVE TURNING (left/right from current direction)
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                    // Turn left relative to current direction
                    switch(player->direction) {
                        case DIR_NORTH: TurnBike(player, DIR_WEST, &state); break;
                        case DIR_SOUTH: TurnBike(player, DIR_EAST, &state); break;
                        case DIR_EAST: TurnBike(player, DIR_NORTH, &state); break;
                        case DIR_WEST: TurnBike(player, DIR_SOUTH, &state); break;
                    }
                }
                if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                    // Turn right relative to current direction
                    switch(player->direction) {
                        case DIR_NORTH: TurnBike(player, DIR_EAST, &state); break;
                        case DIR_SOUTH: TurnBike(player, DIR_WEST, &state); break;
                        case DIR_EAST: TurnBike(player, DIR_SOUTH, &state); break;
                        case DIR_WEST: TurnBike(player, DIR_NORTH, &state); break;
                    }
                }
                // Keep absolute controls for UP/DOWN if you want
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                    TurnBike(player, DIR_NORTH, &state);
                }
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                    TurnBike(player, DIR_SOUTH, &state);
                }
                
                // PS5 Controller input - RELATIVE TURNING
                if (gamepadId >= 0) {
                    // D-Pad Left/Right for relative turning
                    if (IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
                        // Turn left
                        switch(player->direction) {
                            case DIR_NORTH: TurnBike(player, DIR_WEST, &state); break;
                            case DIR_SOUTH: TurnBike(player, DIR_EAST, &state); break;
                            case DIR_EAST: TurnBike(player, DIR_NORTH, &state); break;
                            case DIR_WEST: TurnBike(player, DIR_SOUTH, &state); break;
                        }
                    }
                    if (IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
                        // Turn right
                        switch(player->direction) {
                            case DIR_NORTH: TurnBike(player, DIR_EAST, &state); break;
                            case DIR_SOUTH: TurnBike(player, DIR_WEST, &state); break;
                            case DIR_EAST: TurnBike(player, DIR_SOUTH, &state); break;
                            case DIR_WEST: TurnBike(player, DIR_NORTH, &state); break;
                        }
                    }
                    // D-Pad Up/Down for absolute
                    if (IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
                        TurnBike(player, DIR_NORTH, &state);
                    }
                    if (IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
                        TurnBike(player, DIR_SOUTH, &state);
                    }
                    
                    // Left analog stick - RELATIVE TURNING
                    float axisX = GetGamepadAxisMovement(gamepadId, GAMEPAD_AXIS_LEFT_X);
                    float axisY = GetGamepadAxisMovement(gamepadId, GAMEPAD_AXIS_LEFT_Y);
                    
                    static bool analogCooldown = false;
                    static int cooldownTimer = 0;
                    
                    if (cooldownTimer > 0) {
                        cooldownTimer--;
                    } else {
                        analogCooldown = false;
                    }
                    
                    if (!analogCooldown) {
                        if (fabs(axisX) > 0.5f) {
                            if (axisX < -0.5f) {
                                // Left stick left = turn left
                                switch(player->direction) {
                                    case DIR_NORTH: TurnBike(player, DIR_WEST, &state); break;
                                    case DIR_SOUTH: TurnBike(player, DIR_EAST, &state); break;
                                    case DIR_EAST: TurnBike(player, DIR_NORTH, &state); break;
                                    case DIR_WEST: TurnBike(player, DIR_SOUTH, &state); break;
                                }
                                analogCooldown = true;
                                cooldownTimer = 10;
                            } else if (axisX > 0.5f) {
                                // Left stick right = turn right
                                switch(player->direction) {
                                    case DIR_NORTH: TurnBike(player, DIR_EAST, &state); break;
                                    case DIR_SOUTH: TurnBike(player, DIR_WEST, &state); break;
                                    case DIR_EAST: TurnBike(player, DIR_SOUTH, &state); break;
                                    case DIR_WEST: TurnBike(player, DIR_NORTH, &state); break;
                                }
                                analogCooldown = true;
                                cooldownTimer = 10;
                            }
                        }
                    }
                }
                
                // Boost - Space or R2/RT trigger
                bool boost = IsKeyDown(KEY_SPACE);
                if (gamepadId >= 0) {
                    float rightTrigger = GetGamepadAxisMovement(gamepadId, GAMEPAD_AXIS_RIGHT_TRIGGER);
                    boost = boost || (rightTrigger > 0.1f) || 
                            IsGamepadButtonDown(gamepadId, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
                }
                BoostBike(player, boost, &state);
            }
            
            // Player 2 input (splitscreen mode only)
            if (state.mode == SPLITSCREEN && state.bikeCount > 1) {
                Bike* player2 = &state.bikes[1];
                if (player2->alive) {
                    // Arrow keys for player 2
                    if (IsKeyPressed(KEY_LEFT)) {
                        switch(player2->direction) {
                            case DIR_NORTH: TurnBike(player2, DIR_WEST, &state); break;
                            case DIR_SOUTH: TurnBike(player2, DIR_EAST, &state); break;
                            case DIR_EAST: TurnBike(player2, DIR_NORTH, &state); break;
                            case DIR_WEST: TurnBike(player2, DIR_SOUTH, &state); break;
                        }
                    }
                    if (IsKeyPressed(KEY_RIGHT)) {
                        switch(player2->direction) {
                            case DIR_NORTH: TurnBike(player2, DIR_EAST, &state); break;
                            case DIR_SOUTH: TurnBike(player2, DIR_WEST, &state); break;
                            case DIR_EAST: TurnBike(player2, DIR_SOUTH, &state); break;
                            case DIR_WEST: TurnBike(player2, DIR_NORTH, &state); break;
                        }
                    }

                    // PS5 Controller 2 input - RELATIVE TURNING
                    if (gamepadId2 >= 0) {
                        // D-Pad Left/Right for relative turning
                        if (IsGamepadButtonPressed(gamepadId2, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
                            // Turn left
                            switch(player2->direction) {
                                case DIR_NORTH: TurnBike(player2, DIR_WEST, &state); break;
                                case DIR_SOUTH: TurnBike(player2, DIR_EAST, &state); break;
                                case DIR_EAST: TurnBike(player2, DIR_NORTH, &state); break;
                                case DIR_WEST: TurnBike(player2, DIR_SOUTH, &state); break;
                            }
                        }
                        if (IsGamepadButtonPressed(gamepadId2, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
                            // Turn right
                            switch(player2->direction) {
                                case DIR_NORTH: TurnBike(player2, DIR_EAST, &state); break;
                                case DIR_SOUTH: TurnBike(player2, DIR_WEST, &state); break;
                                case DIR_EAST: TurnBike(player2, DIR_SOUTH, &state); break;
                                case DIR_WEST: TurnBike(player2, DIR_NORTH, &state); break;
                            }
                        }

                        // Left analog stick - RELATIVE TURNING
                        float axisX2 = GetGamepadAxisMovement(gamepadId2, GAMEPAD_AXIS_LEFT_X);

                        static bool analogCooldown2 = false;
                        static int cooldownTimer2 = 0;

                        if (cooldownTimer2 > 0) {
                            cooldownTimer2--;
                        } else {
                            analogCooldown2 = false;
                        }

                        if (!analogCooldown2) {
                            if (fabs(axisX2) > 0.5f) {
                                if (axisX2 < -0.5f) {
                                    // Left stick left = turn left
                                    switch(player2->direction) {
                                        case DIR_NORTH: TurnBike(player2, DIR_WEST, &state); break;
                                        case DIR_SOUTH: TurnBike(player2, DIR_EAST, &state); break;
                                        case DIR_EAST: TurnBike(player2, DIR_NORTH, &state); break;
                                        case DIR_WEST: TurnBike(player2, DIR_SOUTH, &state); break;
                                    }
                                    analogCooldown2 = true;
                                    cooldownTimer2 = 10;
                                } else if (axisX2 > 0.5f) {
                                    // Left stick right = turn right
                                    switch(player2->direction) {
                                        case DIR_NORTH: TurnBike(player2, DIR_EAST, &state); break;
                                        case DIR_SOUTH: TurnBike(player2, DIR_WEST, &state); break;
                                        case DIR_EAST: TurnBike(player2, DIR_SOUTH, &state); break;
                                        case DIR_WEST: TurnBike(player2, DIR_NORTH, &state); break;
                                    }
                                    analogCooldown2 = true;
                                    cooldownTimer2 = 10;
                                }
                            }
                        }
                    }

                    // Boost - Right Shift or R2/RT trigger
                    bool boost2 = IsKeyDown(KEY_RIGHT_SHIFT);
                    if (gamepadId2 >= 0) {
                        float rightTrigger2 = GetGamepadAxisMovement(gamepadId2, GAMEPAD_AXIS_RIGHT_TRIGGER);
                        boost2 = boost2 || (rightTrigger2 > 0.1f) ||
                                IsGamepadButtonDown(gamepadId2, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
                    }
                    BoostBike(player2, boost2, &state);
                }
            }
            
            // Update AI
            if (state.mode == SINGLEPLAYER) {
                // In singleplayer: update all NPCs (bikes 1-3)
                for (int i = 1; i < state.bikeCount; i++) {
                    UpdateAI(&state.bikes[i], &state);
                }
            } else if (state.mode == SPLITSCREEN) {
                // In splitscreen: update only AI bikes (bikes 2-3), not human players (bikes 0-1)
                for (int i = 2; i < state.bikeCount; i++) {
                    if (!state.bikes[i].isPlayer) {
                        UpdateAI(&state.bikes[i], &state);
                    }
                }
            }
            
            // Update all bikes (with delta time for framerate independence)
            float deltaTime = GetFrameTime();
            for (int i = 0; i < state.bikeCount; i++) {
                UpdateBike(&state.bikes[i], deltaTime);
                CheckCollision(&state.bikes[i], &state);
                CheckPickupCollision(&state.bikes[i], &state);
            }

            // Update pickups
            UpdatePickups(&state);

            // Update camera to follow player
            if (player->alive) {
                Vector3 dirVec = GetDirectionVector(player->direction);
                Vector3 targetPos = player->position;
                targetPos = Vector3Subtract(targetPos, Vector3Scale(dirVec, 5.0f));
                
                camera.target.x += (targetPos.x - camera.target.x) * 0.1f;
                camera.target.z += (targetPos.z - camera.target.z) * 0.1f;
                
                Vector3 camPos = Vector3Subtract(player->position, Vector3Scale(dirVec, 20.0f));
                camPos.y = 15.0f;
                
                camera.position.x += (camPos.x - camera.position.x) * 0.1f;
                camera.position.z += (camPos.z - camera.position.z) * 0.1f;
                camera.position.y += (camPos.y - camera.position.y) * 0.1f;
            }
            
            // Check game over
            int aliveCount = 0;
            for (int i = 0; i < state.bikeCount; i++) {
                if (state.bikes[i].alive) aliveCount++;
            }
            
            // Game over when only 0 or 1 bikes left
            if (aliveCount == 0 || (aliveCount == 1 && state.bikeCount > 1)) {
                state.gameOver = true;
                state.playerWon = player->alive;
            }
        }
        
        // Drawing
        BeginDrawing();
        ClearBackground(BLACK);
        
        if (state.mode == MENU) {
            // Draw menu
            DrawMenu(&state, screenWidth, screenHeight);
        } else if (state.mode == SPLITSCREEN) {
            // SPLIT SCREEN MODE - Draw two views side by side
            TraceLog(LOG_INFO, "Rendering split screen mode");
            Bike* player1 = &state.bikes[0];
            Bike* player2 = &state.bikes[1];
            
            // Setup camera for player 1
            Camera3D camera1 = camera;
            if (player1->alive) {
                Vector3 dirVec = GetDirectionVector(player1->direction);
                camera1.position = Vector3Subtract(player1->position, Vector3Scale(dirVec, 20.0f));
                camera1.position.y = 15.0f;
                camera1.target = player1->position;
            }
            
            // Setup camera for player 2
            Camera3D camera2 = camera;
            if (player2->alive) {
                Vector3 dirVec = GetDirectionVector(player2->direction);
                camera2.position = Vector3Subtract(player2->position, Vector3Scale(dirVec, 20.0f));
                camera2.position.y = 15.0f;
                camera2.target = player2->position;
            }
            
            // Draw Player 1 view to render texture
            BeginTextureMode(screenPlayer1);
            ClearBackground(BLACK);
            BeginMode3D(camera1);
            DrawTronGrid();
            DrawWalls();
            DrawPickups(&state);
            for (int i = 0; i < state.bikeCount; i++) {
                DrawTrail(&state.bikes[i]);
                if (IsModelValid(state.bikeModel)) DrawBikeModel(&state.bikes[i], &state.bikeModel); else DrawBike(&state.bikes[i]);
            }
            EndMode3D();
            DrawText("PLAYER 1", 10, 10, 20, SKYBLUE);
            // Minimap for player 1 view
            DrawMinimap(&state, screenWidth/2 - 130, 40, 120);
            // Speedometer for player 1
            DrawSpeedometer(&state.bikes[0], 10, screenHeight - 110, 100);
            EndTextureMode();

            // Draw Player 2 view to render texture
            BeginTextureMode(screenPlayer2);
            ClearBackground(BLACK);
            BeginMode3D(camera2);
            DrawTronGrid();
            DrawWalls();
            DrawPickups(&state);
            for (int i = 0; i < state.bikeCount; i++) {
                DrawTrail(&state.bikes[i]);
                if (IsModelValid(state.bikeModel)) DrawBikeModel(&state.bikes[i], &state.bikeModel); else DrawBike(&state.bikes[i]);
            }
            EndMode3D();
            DrawText("PLAYER 2", 10, 10, 20, MAGENTA);
            // Minimap for player 2 view
            DrawMinimap(&state, screenWidth/2 - 130, 40, 120);
            // Speedometer for player 2
            DrawSpeedometer(&state.bikes[1], 10, screenHeight - 110, 100);
            EndTextureMode();
            
            // Draw both screens side by side
            DrawTextureRec(screenPlayer1.texture, splitScreenRect, (Vector2){0, 0}, WHITE);
            DrawTextureRec(screenPlayer2.texture, splitScreenRect, (Vector2){screenWidth/2.0f, 0}, WHITE);
            DrawRectangle(screenWidth/2 - 2, 0, 4, screenHeight, LIGHTGRAY);
        } else {
            // SINGLE PLAYER MODE
            BeginMode3D(camera);
            
            // Draw grid and walls
            DrawTronGrid();
            DrawWalls();

            // Draw pickups (before trails so they're visible)
            DrawPickups(&state);

            // Draw all bikes and trails
            for (int i = 0; i < state.bikeCount; i++) {
                DrawTrail(&state.bikes[i]);
                if (IsModelValid(state.bikeModel)) DrawBikeModel(&state.bikes[i], &state.bikeModel); else DrawBike(&state.bikes[i]);
            }

            EndMode3D();
            
            // HUD for single player
            int aliveCount = 0;
        for (int i = 0; i < state.bikeCount; i++) {
            if (state.bikes[i].alive) aliveCount++;
        }
        
        DrawText(TextFormat("Players Alive: %d", aliveCount), 20, 20, 20, SKYBLUE);
        DrawText(TextFormat("Speed: %.1fx", state.bikes[0].boosting ? BOOST_MULTIPLIER : 1.0f), 
                 20, 50, 20, SKYBLUE);
        
        // Debug info
        Bike* player = &state.bikes[0];
        DrawText(TextFormat("Pos: (%.1f, %.1f)", player->position.x, player->position.z), 20, 80, 16, YELLOW);
        DrawText(TextFormat("Trail Count: %d", player->trailCount), 20, 100, 16, YELLOW);
        DrawText(TextFormat("Occupied: %d cells", player->occupiedCount), 20, 120, 16, YELLOW);
        
        // Death criteria
        DrawText("DIE IF: Hit wall | Hit any trail | Hit other bike", 20, 150, 14, ORANGE);
        
        // Controller status
        if (gamepadId >= 0) {
            DrawText(TextFormat("Controller: %s", GetGamepadName(gamepadId)), 20, 180, 16, GREEN);
        } else {
            DrawText("No Controller Connected", 20, 180, 16, GRAY);
        }
        
        // Instructions
        if (gamepadId >= 0) {
            DrawText("D-Pad/Left Stick: Turn | R2: Boost | Triangle: Restart",
                     screenWidth/2 - 280, screenHeight - 30, 16, SKYBLUE);
        } else {
            DrawText("Arrow Keys/WASD: Turn | Space: Boost | R: Restart",
                     screenWidth/2 - 250, screenHeight - 30, 16, SKYBLUE);
        }

        // Draw minimap in upper right corner
        DrawMinimap(&state, screenWidth - 220, 20, 200);

        // Draw speedometer below minimap
        DrawSpeedometer(player, screenWidth - 140, 240, 120);

            // Game over
            if (state.gameOver) {
                const char* text = state.playerWon ? "YOU WIN!" : "GAME OVER";
                Color textColor = state.playerWon ? SKYBLUE : RED;
                int textWidth = MeasureText(text, 60);
                DrawText(text, screenWidth/2 - textWidth/2, screenHeight/2 - 30, 60, textColor);
            }
        }
        
        EndDrawing();
    }
    
    // Cleanup
    if (IsMusicValid(state.bgm)) {
        UnloadMusicStream(state.bgm);
    }
    CloseAudioDevice();
    CloseWindow();
    
    return 0;
}
