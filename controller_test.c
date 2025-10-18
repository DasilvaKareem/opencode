#include "raylib.h"
#include <stdio.h>

int main(void) {
    InitWindow(800, 600, "PS5 Controller Detector");
    SetTargetFPS(60);
    
    printf("\n🎮 PS5 Controller Detection Tool\n");
    printf("================================\n\n");
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(DARKGRAY);
        
        DrawText("PS5 CONTROLLER DETECTOR", 200, 20, 30, WHITE);
        DrawText("Press ESC to exit", 20, 560, 20, LIGHTGRAY);
        
        int yPos = 80;
        bool anyFound = false;
        
        // Check all 4 possible gamepad slots
        for (int i = 0; i < 4; i++) {
            if (IsGamepadAvailable(i)) {
                anyFound = true;
                
                // Draw gamepad info
                DrawText(TextFormat("GAMEPAD %d: CONNECTED", i), 50, yPos, 25, GREEN);
                DrawText(TextFormat("Name: %s", GetGamepadName(i)), 70, yPos + 30, 20, WHITE);
                DrawText(TextFormat("Axis Count: %d", GetGamepadAxisCount(i)), 70, yPos + 55, 18, LIGHTGRAY);
                
                // Test buttons
                DrawText("Testing buttons...", 70, yPos + 80, 18, YELLOW);
                
                if (IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                    DrawText("X BUTTON PRESSED!", 250, yPos + 80, 20, GREEN);
                }
                if (IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) {
                    DrawText("SQUARE PRESSED!", 250, yPos + 105, 20, GREEN);
                }
                if (IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) {
                    DrawText("R1 PRESSED!", 250, yPos + 130, 20, GREEN);
                }
                
                // Test analog stick
                float axisX = GetGamepadAxisMovement(i, GAMEPAD_AXIS_LEFT_X);
                float axisY = GetGamepadAxisMovement(i, GAMEPAD_AXIS_LEFT_Y);
                
                DrawText(TextFormat("Left Stick: X=%.2f Y=%.2f", axisX, axisY), 70, yPos + 105, 18, SKYBLUE);
                
                // Draw stick visualization
                DrawCircle(600, yPos + 60, 40, DARKBLUE);
                DrawCircle(600 + (int)(axisX * 35), yPos + 60 + (int)(axisY * 35), 10, BLUE);
                
                yPos += 180;
            }
        }
        
        if (!anyFound) {
            DrawText("NO CONTROLLERS DETECTED", 150, 250, 30, RED);
            DrawText("Please connect your PS5 controllers", 150, 290, 20, YELLOW);
            DrawText("Make sure they are paired via Bluetooth", 150, 320, 18, LIGHTGRAY);
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}
