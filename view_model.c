#include "raylib.h"
#include "raymath.h"

int main(void) {
    InitWindow(1280, 800, "Tron Bike Model Viewer");
    
    Camera3D camera = {0};
    camera.position = (Vector3){10.0f, 10.0f, 10.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    Model model = LoadModel("tron_light_cycle.glb");
    
    SetTargetFPS(60);
    
    float rotation = 0.0f;
    float scale = 1.0f;
    
    while (!WindowShouldClose()) {
        // Controls
        if (IsKeyDown(KEY_UP)) scale += 0.01f;
        if (IsKeyDown(KEY_DOWN)) scale -= 0.01f;
        if (IsKeyDown(KEY_LEFT)) rotation -= 2.0f;
        if (IsKeyDown(KEY_RIGHT)) rotation += 2.0f;
        
        UpdateCamera(&camera, CAMERA_ORBITAL);
        
        BeginDrawing();
        ClearBackground(BLACK);
        
        BeginMode3D(camera);
        
        DrawGrid(20, 1.0f);
        
        // Draw model
        DrawModelEx(model, (Vector3){0, 0, 0}, (Vector3){0, 1, 0}, rotation,
                    (Vector3){scale, scale, scale}, SKYBLUE);
        
        // Draw axes for reference
        DrawLine3D((Vector3){0,0,0}, (Vector3){5,0,0}, RED);
        DrawLine3D((Vector3){0,0,0}, (Vector3){0,5,0}, GREEN);
        DrawLine3D((Vector3){0,0,0}, (Vector3){0,0,5}, BLUE);
        
        EndMode3D();
        
        DrawText(TextFormat("Scale: %.2f (UP/DOWN to adjust)", scale), 10, 10, 20, WHITE);
        DrawText(TextFormat("Rotation: %.0f (LEFT/RIGHT to rotate)", rotation), 10, 40, 20, WHITE);
        DrawText("Mouse to orbit camera", 10, 70, 20, WHITE);
        
        EndDrawing();
    }
    
    UnloadModel(model);
    CloseWindow();
    return 0;
}
