#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SIZE 24
#define BALL_SIZE 12
#define FIELD_LENGTH 1000
#define YARD_LINE_SPACING 10

typedef enum {
    TEAM_OFFENSE,
    TEAM_DEFENSE
} Team;

typedef enum {
    GAME_KICKOFF,
    GAME_PLAYING,
    GAME_TACKLE,
    GAME_TOUCHDOWN,
    GAME_TURNOVER,
    GAME_OVER
} GamePhase;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Team team;
    Color color;
    bool has_ball;
    int number;
    float radius;
} Player;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool in_air;
    int carrier_idx;
    float air_time;
} Ball;

typedef struct {
    int home_score;
    int away_score;
    int down;
    int yards_to_go;
    int yard_line;
    int line_of_scrimmage;
    bool home_has_ball;
    GamePhase phase;
    float phase_timer;
    int quarter;
    float game_clock;
    char message[64];
    int start_of_play_yard;
} GameState;

Player players[22];
Ball ball;
GameState game;
int controlled_idx;
float camera_y;
Sound whistle;
Sound tackle_sound;
Sound crowd;

void GenerateWhistle(void) {
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
    whistle = LoadSoundFromWave(wave);
    UnloadWave(wave);
}

void GenerateTackle(void) {
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

void GenerateCrowd(void) {
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
    crowd = LoadSoundFromWave(wave);
    SetSoundVolume(crowd, 0.3f);
    UnloadWave(wave);
}

void SetupPlay(bool kickoff) {
    int start_yard = game.yard_line;
    
    if (kickoff) {
        start_yard = game.home_has_ball ? 35 : 65;
        game.yard_line = start_yard;
        game.line_of_scrimmage = start_yard;
    }
    
    game.start_of_play_yard = game.line_of_scrimmage;
    
    int offense_y = game.line_of_scrimmage * YARD_LINE_SPACING;
    int defense_y = offense_y + 50;
    
    if (game.home_has_ball) {
        for (int i = 0; i < 11; i++) {
            players[i].position.x = 400 + (i - 5) * 40;
            players[i].position.y = offense_y;
            players[i].velocity.x = 0;
            players[i].velocity.y = 0;
            players[i].team = TEAM_OFFENSE;
            players[i].color = BLUE;
            players[i].has_ball = (i == 5);
        }
        
        for (int i = 0; i < 11; i++) {
            int idx = 11 + i;
            players[idx].position.x = 400 + (i - 5) * 40;
            players[idx].position.y = defense_y;
            players[idx].velocity.x = 0;
            players[idx].velocity.y = 0;
            players[idx].team = TEAM_DEFENSE;
            players[idx].color = RED;
            players[idx].has_ball = false;
        }
        controlled_idx = 5;
    } else {
        for (int i = 0; i < 11; i++) {
            players[i].position.x = 400 + (i - 5) * 40;
            players[i].position.y = defense_y;
            players[i].velocity.x = 0;
            players[i].velocity.y = 0;
            players[i].team = TEAM_DEFENSE;
            players[i].color = BLUE;
            players[i].has_ball = false;
        }
        
        for (int i = 0; i < 11; i++) {
            int idx = 11 + i;
            players[idx].position.x = 400 + (i - 5) * 40;
            players[idx].position.y = offense_y;
            players[idx].velocity.x = 0;
            players[idx].velocity.y = 0;
            players[idx].team = TEAM_OFFENSE;
            players[idx].has_ball = (i == 5);
        }
        controlled_idx = 16;
    }
    
    ball.carrier_idx = controlled_idx;
    ball.position = players[controlled_idx].position;
    ball.in_air = false;
    ball.air_time = 0;
    camera_y = players[controlled_idx].position.y;
}

void InitGame(void) {
    for (int i = 0; i < 22; i++) {
        players[i].number = (i % 11) + 1;
        players[i].radius = PLAYER_SIZE / 2.0f;
    }
    
    ball.velocity.x = 0;
    ball.velocity.y = 0;
    ball.in_air = false;
    ball.air_time = 0;
    
    game.home_score = 0;
    game.away_score = 0;
    game.down = 1;
    game.yards_to_go = 10;
    game.yard_line = 20;
    game.line_of_scrimmage = 20;
    game.home_has_ball = true;
    game.phase = GAME_KICKOFF;
    game.phase_timer = 0;
    game.quarter = 1;
    game.game_clock = 900;
    strcpy(game.message, "");
    
    SetupPlay(true);
}

void ResolveCollision(Player* p1, Player* p2) {
    float dx = p2->position.x - p1->position.x;
    float dy = p2->position.y - p1->position.y;
    float dist = sqrtf(dx * dx + dy * dy);
    
    if (dist < p1->radius + p2->radius && dist > 0) {
        float overlap = (p1->radius + p2->radius) - dist;
        
        float nx = dx / dist;
        float ny = dy / dist;
        
        p1->position.x -= nx * overlap * 0.5f;
        p1->position.y -= ny * overlap * 0.5f;
        p2->position.x += nx * overlap * 0.5f;
        p2->position.y += ny * overlap * 0.5f;
        
        float rel_vx = p2->velocity.x - p1->velocity.x;
        float rel_vy = p2->velocity.y - p1->velocity.y;
        float dot = rel_vx * nx + rel_vy * ny;
        
        if (dot < 0) {
            float bounce = 0.5f;
            p1->velocity.x += nx * dot * bounce;
            p1->velocity.y += ny * dot * bounce;
            p2->velocity.x -= nx * dot * bounce;
            p2->velocity.y -= ny * dot * bounce;
        }
    }
}

void UpdatePlayer(Player* p) {
    p->position.x += p->velocity.x;
    p->position.y += p->velocity.y;
    
    if (p->position.x < 50 + p->radius) p->position.x = 50 + p->radius;
    if (p->position.x > SCREEN_WIDTH - 50 - p->radius) p->position.x = SCREEN_WIDTH - 50 - p->radius;
    if (p->position.y < 50 + p->radius) p->position.y = 50 + p->radius;
    if (p->position.y > FIELD_LENGTH - 50 - p->radius) p->position.y = FIELD_LENGTH - 50 - p->radius;
    
    p->velocity.x *= 0.85f;
    p->velocity.y *= 0.85f;
}

void UpdateAI(void) {
    if (game.phase != GAME_PLAYING) return;
    
    for (int i = 0; i < 22; i++) {
        if (players[i].team == TEAM_DEFENSE) {
            Player* defender = &players[i];
            Vector2 target;
            
            if (ball.carrier_idx >= 0) {
                target = players[ball.carrier_idx].position;
            } else if (ball.in_air) {
                target = ball.position;
            } else {
                continue;
            }
            
            float dx = target.x - defender->position.x;
            float dy = target.y - defender->position.y;
            float len = sqrtf(dx * dx + dy * dy);
            
            if (len > 0 && len > 30) {
                defender->velocity.x = (dx / len) * 2.5f;
                defender->velocity.y = (dy / len) * 2.5f;
            }
        }
    }
}

void EndPlay(const char* msg) {
    PlaySound(tackle_sound);
    PlaySound(whistle);
    
    if (ball.carrier_idx >= 0) {
        players[ball.carrier_idx].has_ball = false;
    }
    
    int tackle_yard = game.yard_line;
    if (ball.carrier_idx >= 0) {
        tackle_yard = (int)(players[ball.carrier_idx].position.y / YARD_LINE_SPACING);
    }
    
    int yards_gained = tackle_yard - game.start_of_play_yard;
    
    ball.carrier_idx = -1;
    ball.in_air = false;
    ball.air_time = 0;
    
    game.phase = GAME_TACKLE;
    game.phase_timer = 2.0f;
    game.yard_line = tackle_yard;
    game.yards_to_go -= yards_gained;
    
    if (strlen(msg) > 0) {
        strcpy(game.message, msg);
    } else {
        sprintf(game.message, "Gain of %d yards", yards_gained);
    }
    
    if (game.yards_to_go <= 0) {
        game.down = 1;
        game.yards_to_go = 10;
        game.line_of_scrimmage = game.yard_line;
        strcpy(game.message, "FIRST DOWN!");
    } else {
        game.down++;
        if (game.down > 4) {
            game.home_has_ball = !game.home_has_ball;
            game.down = 1;
            game.yards_to_go = 10;
            game.line_of_scrimmage = game.yard_line;
            game.phase = GAME_TURNOVER;
            strcpy(game.message, "TURNOVER ON DOWNS!");
        }
    }
}

void CheckTackle(void) {
    if (ball.carrier_idx < 0 || game.phase != GAME_PLAYING) return;
    
    for (int i = 0; i < 22; i++) {
        if (i == ball.carrier_idx) continue;
        
        float dx = players[i].position.x - players[ball.carrier_idx].position.x;
        float dy = players[i].position.y - players[ball.carrier_idx].position.y;
        float dist = sqrtf(dx * dx + dy * dy);
        
        if (dist < players[i].radius + players[ball.carrier_idx].radius && 
            players[i].team != players[ball.carrier_idx].team) {
            EndPlay("");
            return;
        }
    }
}

void CheckTouchdown(void) {
    if (ball.carrier_idx < 0) return;
    
    bool scored = false;
    
    if (game.home_has_ball && players[ball.carrier_idx].position.y < 50) {
        game.home_score += 6;
        scored = true;
    } else if (!game.home_has_ball && players[ball.carrier_idx].position.y > FIELD_LENGTH - 50) {
        game.away_score += 6;
        scored = true;
    }
    
    if (scored) {
        game.phase = GAME_TOUCHDOWN;
        game.phase_timer = 3.0f;
        PlaySound(whistle);
        strcpy(game.message, "TOUCHDOWN!");
        players[ball.carrier_idx].has_ball = false;
        ball.carrier_idx = -1;
    }
}

void CheckInterception(void) {
    if (!ball.in_air || game.phase != GAME_PLAYING) return;
    
    for (int i = 0; i < 22; i++) {
        float dx = players[i].position.x - ball.position.x;
        float dy = players[i].position.y - ball.position.y;
        float dist = sqrtf(dx * dx + dy * dy);
        
        if (dist < players[i].radius) {
            ball.in_air = false;
            ball.air_time = 0;
            ball.carrier_idx = i;
            players[i].has_ball = true;
            
            if (players[i].team == TEAM_DEFENSE) {
                controlled_idx = i;
                game.home_has_ball = !game.home_has_ball;
                game.down = 1;
                game.yards_to_go = 10;
                game.line_of_scrimmage = (int)(players[i].position.y / YARD_LINE_SPACING);
                game.yard_line = game.line_of_scrimmage;
                game.phase = GAME_TURNOVER;
                game.phase_timer = 2.0f;
                strcpy(game.message, "INTERCEPTION!");
                PlaySound(whistle);
            } else {
                controlled_idx = i;
            }
            return;
        }
    }
}

void UpdateBall(float dt) {
    if (ball.in_air) {
        ball.air_time += dt;
        ball.position.x += ball.velocity.x;
        ball.position.y += ball.velocity.y;
        
        ball.velocity.x *= 0.98f;
        ball.velocity.y *= 0.98f;
        
        CheckInterception();
        
        if (ball.air_time > 3.0f || fabsf(ball.velocity.x) < 0.5f && fabsf(ball.velocity.y) < 0.5f) {
            ball.in_air = false;
            ball.air_time = 0;
            EndPlay("Incomplete pass");
        }
    } else if (ball.carrier_idx >= 0) {
        ball.position = players[ball.carrier_idx].position;
    }
}

void DrawField(void) {
    DrawRectangle(0, (int)camera_y - 300, SCREEN_WIDTH, 600, DARKGREEN);
    
    for (int i = 0; i <= 100; i += 10) {
        int y = i * YARD_LINE_SPACING;
        int screen_y = (int)(y - camera_y) + 300;
        
        if (screen_y > -50 && screen_y < SCREEN_HEIGHT + 50) {
            DrawLine(50, screen_y, SCREEN_WIDTH - 50, screen_y, WHITE);
            DrawText(TextFormat("%d", i), 10, screen_y - 10, 20, WHITE);
        }
    }
    
    DrawRectangle(0, (int)(0 - camera_y) + 300, SCREEN_WIDTH, 50, (Color){0, 0, 139, 100});
    DrawRectangle(0, (int)(FIELD_LENGTH - camera_y) + 250, SCREEN_WIDTH, 50, (Color){139, 0, 0, 100});
    
    int los_y = (int)(game.line_of_scrimmage * YARD_LINE_SPACING - camera_y) + 300;
    DrawLine(50, los_y, SCREEN_WIDTH - 50, los_y, YELLOW);
    
    int first_down_y = (int)((game.line_of_scrimmage + game.yards_to_go) * YARD_LINE_SPACING - camera_y) + 300;
    DrawLine(50, first_down_y, SCREEN_WIDTH - 50, first_down_y, ORANGE);
}

void DrawUI(void) {
    DrawRectangle(10, 10, 380, 120, (Color){0, 0, 0, 200});
    DrawText(TextFormat("HOME: %d  AWAY: %d", game.home_score, game.away_score), 20, 20, 20, WHITE);
    DrawText(TextFormat("Q%d  %d:%02d", game.quarter, (int)game.game_clock/60, (int)game.game_clock%60), 20, 45, 20, WHITE);
    DrawText(TextFormat("Down: %d  To Go: %d", game.down, game.yards_to_go), 20, 70, 18, YELLOW);
    DrawText(TextFormat("Yard Line: %d  %s Ball", game.yard_line, game.home_has_ball ? "HOME" : "AWAY"), 20, 95, 16, YELLOW);
    
    DrawText("ARROWS: Move  SPACE: Pass  ENTER: Hike  R: Reset", 10, SCREEN_HEIGHT - 30, 16, WHITE);
    
    if (strlen(game.message) > 0) {
        int msg_width = MeasureText(game.message, 24);
        DrawRectangle(SCREEN_WIDTH/2 - msg_width/2 - 20, SCREEN_HEIGHT/2 - 30, msg_width + 40, 60, (Color){0, 0, 0, 180});
        DrawText(game.message, SCREEN_WIDTH/2 - msg_width/2, SCREEN_HEIGHT/2 - 12, 24, GREEN);
    }
    
    if (game.phase == GAME_KICKOFF || game.phase == GAME_TACKLE) {
        DrawText("PRESS ENTER TO HIKE", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 + 50, 24, WHITE);
    }
    
    if (game.phase == GAME_TOUCHDOWN) {
        DrawText("TOUCHDOWN!", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 80, 40, GOLD);
    }
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tecmo Bowl Football");
    InitAudioDevice();
    SetTargetFPS(60);
    
    GenerateWhistle();
    GenerateTackle();
    GenerateCrowd();
    
    InitGame();
    PlaySound(crowd);
    
    while (!WindowShouldClose()) {
        if (!IsSoundPlaying(crowd)) {
            PlaySound(crowd);
        }
        
        float dt = GetFrameTime();
        
        if (game.phase == GAME_PLAYING) {
            game.game_clock -= dt;
            if (game.game_clock <= 0) {
                game.quarter++;
                game.game_clock = 900;
                if (game.quarter > 4) {
                    game.phase = GAME_OVER;
                }
            }
        }
        
        if (game.phase == GAME_KICKOFF || game.phase == GAME_TACKLE) {
            if (IsKeyPressed(KEY_ENTER)) {
                game.phase = GAME_PLAYING;
                PlaySound(whistle);
                strcpy(game.message, "");
            }
        }
        
        if (game.phase == GAME_TURNOVER) {
            game.phase_timer -= dt;
            if (game.phase_timer <= 0) {
                game.phase = GAME_KICKOFF;
                SetupPlay(false);
            }
        }
        
        if (game.phase == GAME_TOUCHDOWN) {
            game.phase_timer -= dt;
            if (game.phase_timer <= 0) {
                game.phase = GAME_KICKOFF;
                game.down = 1;
                game.yards_to_go = 10;
                game.yard_line = 20;
                game.line_of_scrimmage = 20;
                SetupPlay(true);
                strcpy(game.message, "");
            }
        }
        
        if (game.phase == GAME_PLAYING && controlled_idx >= 0 && ball.carrier_idx >= 0) {
            players[controlled_idx].velocity.x = 0;
            players[controlled_idx].velocity.y = 0;
            
            if (IsKeyDown(KEY_RIGHT)) players[controlled_idx].velocity.x = 4.0f;
            if (IsKeyDown(KEY_LEFT)) players[controlled_idx].velocity.x = -4.0f;
            if (IsKeyDown(KEY_DOWN)) players[controlled_idx].velocity.y = 4.0f;
            if (IsKeyDown(KEY_UP)) players[controlled_idx].velocity.y = -4.0f;
            
            if (IsKeyPressed(KEY_SPACE) && players[controlled_idx].has_ball && controlled_idx == ball.carrier_idx) {
                ball.in_air = true;
                ball.air_time = 0;
                ball.velocity.x = 0;
                ball.velocity.y = game.home_has_ball ? -15.0f : 15.0f;
                players[controlled_idx].has_ball = false;
                ball.carrier_idx = -1;
                controlled_idx = -1;
            }
        }
        
        if (IsKeyPressed(KEY_R)) {
            InitGame();
        }
        
        if (game.phase == GAME_PLAYING) {
            UpdateAI();
            
            for (int i = 0; i < 22; i++) {
                UpdatePlayer(&players[i]);
            }
            
            for (int i = 0; i < 22; i++) {
                for (int j = i + 1; j < 22; j++) {
                    ResolveCollision(&players[i], &players[j]);
                }
            }
            
            UpdateBall(dt);
            CheckTackle();
            CheckTouchdown();
        }
        
        if (ball.carrier_idx >= 0) {
            float target_y = players[ball.carrier_idx].position.y;
            camera_y += (target_y - camera_y) * 0.1f;
        }
        
        BeginDrawing();
        ClearBackground(GREEN);
        
        DrawField();
        
        for (int i = 0; i < 22; i++) {
            int screen_x = (int)players[i].position.x;
            int screen_y = (int)(players[i].position.y - camera_y) + 300;
            
            DrawCircle(screen_x, screen_y, players[i].radius, players[i].color);
            DrawText(TextFormat("%d", players[i].number), screen_x - 5, screen_y - 8, 16, WHITE);
            
            if (i == controlled_idx) {
                DrawCircleLines(screen_x, screen_y, players[i].radius + 3, YELLOW);
            }
            
            if (i == ball.carrier_idx) {
                DrawCircleLines(screen_x, screen_y, players[i].radius + 5, ORANGE);
            }
        }
        
        if (ball.in_air) {
            int ball_screen_y = (int)(ball.position.y - camera_y) + 300;
            DrawCircle((int)ball.position.x, ball_screen_y, BALL_SIZE / 2, ORANGE);
        }
        
        DrawUI();
        
        EndDrawing();
    }
    
    UnloadSound(whistle);
    UnloadSound(tackle_sound);
    UnloadSound(crowd);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
