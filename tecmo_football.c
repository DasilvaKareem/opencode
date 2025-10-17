#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SIZE 32
#define BALL_SIZE 16
#define MAX_PLAYERS 11

typedef enum {
    PLAYER_OFFENSE,
    PLAYER_DEFENSE
} PlayerType;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    PlayerType type;
    Color color;
    bool hasball;
    float speed;
} Player;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool inair;
    int carrier_idx;
} Ball;

typedef struct {
    int score_home;
    int score_away;
    int down;
    int yards_to_go;
    bool play_active;
} GameState;

Player players[MAX_PLAYERS * 2];
Ball ball;
GameState game;
int controlled_idx;
Sound whistle_sound;
Sound tackle_sound;
Sound crowd_sound;

void GenerateWhistleSound(void) {
    Wave wave = {0};
    wave.frameCount = 8000;
    wave.sampleRate = 44100;
    wave.sampleSize = 16;
    wave.channels = 1;
    
    short* data = (short*)malloc(wave.frameCount * sizeof(short));
    
    for (unsigned int i = 0; i < wave.frameCount; i++) {
        float t = (float)i / wave.sampleRate;
        float freq = 2000.0f + 500.0f * sinf(t * 20.0f);
        data[i] = (short)(32000.0f * sinf(2.0f * PI * freq * t));
    }
    
    wave.data = data;
    whistle_sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
}

void GenerateTackleSound(void) {
    Wave wave = {0};
    wave.frameCount = 6000;
    wave.sampleRate = 44100;
    wave.sampleSize = 16;
    wave.channels = 1;
    
    short* data = (short*)malloc(wave.frameCount * sizeof(short));
    
    for (unsigned int i = 0; i < wave.frameCount; i++) {
        float t = (float)i / wave.sampleRate;
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float env = expf(-t * 8.0f);
        data[i] = (short)(20000.0f * noise * env);
    }
    
    wave.data = data;
    tackle_sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
}

void GenerateCrowdSound(void) {
    Wave wave = {0};
    wave.frameCount = 88200;
    wave.sampleRate = 44100;
    wave.sampleSize = 16;
    wave.channels = 1;
    
    short* data = (short*)malloc(wave.frameCount * sizeof(short));
    
    for (unsigned int i = 0; i < wave.frameCount; i++) {
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        data[i] = (short)(8000.0f * noise);
    }
    
    wave.data = data;
    crowd_sound = LoadSoundFromWave(wave);
    SetSoundVolume(crowd_sound, 0.3f);
    UnloadWave(wave);
}

void InitGame(void) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].position.x = 200 + (i % 4) * 50;
        players[i].position.y = 300 + (i / 4) * 40;
        players[i].velocity.x = 0;
        players[i].velocity.y = 0;
        players[i].type = PLAYER_OFFENSE;
        players[i].color = BLUE;
        players[i].hasball = (i == 0);
        players[i].speed = 3.0f;
    }
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        int idx = MAX_PLAYERS + i;
        players[idx].position.x = 500 + (i % 4) * 50;
        players[idx].position.y = 250 + (i / 4) * 40;
        players[idx].velocity.x = 0;
        players[idx].velocity.y = 0;
        players[idx].type = PLAYER_DEFENSE;
        players[idx].color = RED;
        players[idx].hasball = false;
        players[idx].speed = 3.0f;
    }
    
    controlled_idx = 0;
    ball.carrier_idx = 0;
    ball.position = players[0].position;
    ball.velocity.x = 0;
    ball.velocity.y = 0;
    ball.inair = false;
    
    game.score_home = 0;
    game.score_away = 0;
    game.down = 1;
    game.yards_to_go = 10;
    game.play_active = false;
}

void UpdatePlayer(Player* p) {
    p->position.x += p->velocity.x;
    p->position.y += p->velocity.y;
    
    if (p->position.x < 0) p->position.x = 0;
    if (p->position.x > SCREEN_WIDTH - PLAYER_SIZE) p->position.x = SCREEN_WIDTH - PLAYER_SIZE;
    if (p->position.y < 0) p->position.y = 0;
    if (p->position.y > SCREEN_HEIGHT - PLAYER_SIZE) p->position.y = SCREEN_HEIGHT - PLAYER_SIZE;
    
    p->velocity.x *= 0.85f;
    p->velocity.y *= 0.85f;
}

void UpdateAI(void) {
    if (!game.play_active) return;
    
    for (int i = MAX_PLAYERS; i < MAX_PLAYERS * 2; i++) {
        Player* defender = &players[i];
        Vector2 target;
        
        if (ball.carrier_idx >= 0) {
            target = players[ball.carrier_idx].position;
        } else {
            target = ball.position;
        }
        
        float dx = target.x - defender->position.x;
        float dy = target.y - defender->position.y;
        float len = sqrtf(dx * dx + dy * dy);
        
        if (len > 0) {
            defender->velocity.x = (dx / len) * defender->speed * 0.5f;
            defender->velocity.y = (dy / len) * defender->speed * 0.5f;
        }
    }
}

void CheckCollisions(void) {
    if (ball.carrier_idx < 0) return;
    
    for (int i = 0; i < MAX_PLAYERS * 2; i++) {
        if (i == ball.carrier_idx) continue;
        
        float dx = players[i].position.x - players[ball.carrier_idx].position.x;
        float dy = players[i].position.y - players[ball.carrier_idx].position.y;
        float dist = sqrtf(dx * dx + dy * dy);
        
        if (dist < PLAYER_SIZE && players[i].type != players[ball.carrier_idx].type) {
            PlaySound(tackle_sound);
            
            players[ball.carrier_idx].hasball = false;
            players[ball.carrier_idx].velocity.x = dx * 0.5f;
            players[ball.carrier_idx].velocity.y = dy * 0.5f;
            ball.carrier_idx = -1;
            ball.inair = false;
            ball.velocity.x = 0;
            ball.velocity.y = 0;
            game.play_active = false;
            
            PlaySound(whistle_sound);
            
            game.down++;
            if (game.down > 4) {
                game.down = 1;
            }
            break;
        }
    }
}

void UpdateBall(void) {
    if (ball.inair) {
        ball.position.x += ball.velocity.x;
        ball.position.y += ball.velocity.y;
        
        ball.velocity.x *= 0.98f;
        ball.velocity.y *= 0.98f;
        
        for (int i = 0; i < MAX_PLAYERS; i++) {
            float dx = players[i].position.x - ball.position.x;
            float dy = players[i].position.y - ball.position.y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            if (dist < PLAYER_SIZE && players[i].type == PLAYER_OFFENSE) {
                ball.inair = false;
                ball.carrier_idx = i;
                players[i].hasball = true;
                controlled_idx = i;
                break;
            }
        }
        
        if (fabsf(ball.velocity.x) < 0.5f && fabsf(ball.velocity.y) < 0.5f) {
            ball.inair = false;
            ball.carrier_idx = -1;
            game.play_active = false;
            PlaySound(whistle_sound);
            game.down++;
        }
    } else if (ball.carrier_idx >= 0) {
        ball.position = players[ball.carrier_idx].position;
    }
}

void DrawField(void) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, DARKGREEN);
    
    for (int i = 0; i < 11; i++) {
        int y = i * (SCREEN_HEIGHT / 10);
        DrawLine(0, y, SCREEN_WIDTH, y, WHITE);
        DrawText(TextFormat("%d", i * 10), 10, y + 5, 20, WHITE);
    }
    
    DrawRectangle(0, 0, SCREEN_WIDTH, 60, (Color){ 0, 0, 139, 100 });
    DrawRectangle(0, SCREEN_HEIGHT - 60, SCREEN_WIDTH, 60, (Color){ 139, 0, 0, 100 });
}

void DrawUI(void) {
    DrawRectangle(10, 10, 300, 100, (Color){ 0, 0, 0, 200 });
    DrawText(TextFormat("HOME: %d", game.score_home), 20, 20, 20, WHITE);
    DrawText(TextFormat("AWAY: %d", game.score_away), 20, 45, 20, WHITE);
    DrawText(TextFormat("Down: %d  Yards: %d", game.down, game.yards_to_go), 20, 70, 16, YELLOW);
    
    if (!game.play_active) {
        DrawText("PRESS ENTER TO HIKE", 20, 95, 16, GREEN);
    }
    
    DrawText("ARROWS: Move  SPACE: Pass  ENTER: Hike  R: Reset", 10, SCREEN_HEIGHT - 30, 18, WHITE);
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tecmo Bowl Football");
    InitAudioDevice();
    SetTargetFPS(60);
    
    GenerateWhistleSound();
    GenerateTackleSound();
    GenerateCrowdSound();
    
    InitGame();
    PlaySound(crowd_sound);
    
    while (!WindowShouldClose()) {
        if (!IsSoundPlaying(crowd_sound)) {
            PlaySound(crowd_sound);
        }
        
        if (!game.play_active) {
            if (IsKeyPressed(KEY_ENTER)) {
                game.play_active = true;
                PlaySound(whistle_sound);
            }
        }
        
        if (game.play_active && controlled_idx >= 0 && controlled_idx < MAX_PLAYERS * 2) {
            players[controlled_idx].velocity.x = 0;
            players[controlled_idx].velocity.y = 0;
            
            if (IsKeyDown(KEY_RIGHT)) players[controlled_idx].velocity.x = players[controlled_idx].speed;
            if (IsKeyDown(KEY_LEFT)) players[controlled_idx].velocity.x = -players[controlled_idx].speed;
            if (IsKeyDown(KEY_DOWN)) players[controlled_idx].velocity.y = players[controlled_idx].speed;
            if (IsKeyDown(KEY_UP)) players[controlled_idx].velocity.y = -players[controlled_idx].speed;
            
            if (IsKeyPressed(KEY_SPACE) && players[controlled_idx].hasball) {
                ball.inair = true;
                ball.velocity.x = 10.0f;
                ball.velocity.y = 0.0f;
                players[controlled_idx].hasball = false;
                ball.carrier_idx = -1;
                controlled_idx = -1;
            }
        }
        
        if (IsKeyPressed(KEY_R)) {
            InitGame();
        }
        
        if (game.play_active) {
            UpdateAI();
            
            for (int i = 0; i < MAX_PLAYERS * 2; i++) {
                UpdatePlayer(&players[i]);
            }
            
            UpdateBall();
            CheckCollisions();
            
            if (ball.carrier_idx >= 0 && players[ball.carrier_idx].position.y < 60) {
                game.score_home += 6;
                game.play_active = false;
                PlaySound(whistle_sound);
                game.down = 1;
            }
            if (ball.carrier_idx >= 0 && players[ball.carrier_idx].position.y > SCREEN_HEIGHT - 60) {
                game.score_away += 6;
                game.play_active = false;
                PlaySound(whistle_sound);
                game.down = 1;
            }
        }
        
        BeginDrawing();
        ClearBackground(GREEN);
        
        DrawField();
        
        for (int i = 0; i < MAX_PLAYERS * 2; i++) {
            DrawRectangle((int)players[i].position.x, (int)players[i].position.y, 
                         PLAYER_SIZE, PLAYER_SIZE, players[i].color);
            
            if (i == controlled_idx) {
                DrawRectangleLines((int)players[i].position.x - 2, (int)players[i].position.y - 2, 
                                  PLAYER_SIZE + 4, PLAYER_SIZE + 4, YELLOW);
            }
        }
        
        if (ball.inair || ball.carrier_idx < 0) {
            DrawCircle((int)ball.position.x, (int)ball.position.y, BALL_SIZE / 2, ORANGE);
        }
        
        DrawUI();
        
        EndDrawing();
    }
    
    UnloadSound(whistle_sound);
    UnloadSound(tackle_sound);
    UnloadSound(crowd_sound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
