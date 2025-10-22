#include "raylib.h"

int main(void) {
    InitWindow(800, 600, "Sound Test");
    InitAudioDevice();
    
    Sound sndTurn = LoadSound("bike_turn.mp3");
    Sound sndBoost = LoadSound("bike_boost.mp3");
    Sound sndExplosion = LoadSound("bike_explosion.mp3");
    Sound sndMenu = LoadSound("menu_select.mp3");
    
    SetTargetFPS(60);
    
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ONE)) PlaySound(sndTurn);
        if (IsKeyPressed(KEY_TWO)) PlaySound(sndBoost);
        if (IsKeyPressed(KEY_THREE)) PlaySound(sndExplosion);
        if (IsKeyPressed(KEY_FOUR)) PlaySound(sndMenu);
        
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("Sound Effects Test", 250, 100, 30, SKYBLUE);
        DrawText("Press 1: Turn", 300, 200, 20, WHITE);
        DrawText("Press 2: Boost", 300, 240, 20, WHITE);
        DrawText("Press 3: Explosion", 300, 280, 20, WHITE);
        DrawText("Press 4: Menu Select", 300, 320, 20, WHITE);
        EndDrawing();
    }
    
    UnloadSound(sndTurn);
    UnloadSound(sndBoost);
    UnloadSound(sndExplosion);
    UnloadSound(sndMenu);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
