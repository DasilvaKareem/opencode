#include "raylib.h"

#define MAX_TARGETS 10

typedef struct {
    Vector3 position;
    bool active;
} Target;

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Raylib FPS Shooter");

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 2.0f, 4.0f };
    camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Target targets[MAX_TARGETS];
    for (int i = 0; i < MAX_TARGETS; i++) {
        targets[i].position = (Vector3){ GetRandomValue(-10, 10), 1.0f, GetRandomValue(-10, 10) };
        targets[i].active = true;
    }

    int score = 0;
    float playerSpeed = 0.1f;

    DisableCursor();
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Update camera
        UpdateCamera(&camera, CAMERA_FIRST_PERSON);

        // Movement
        if (IsKeyDown(KEY_W)) camera.position.z -= playerSpeed;
        if (IsKeyDown(KEY_S)) camera.position.z += playerSpeed;
        if (IsKeyDown(KEY_A)) camera.position.x -= playerSpeed;
        if (IsKeyDown(KEY_D)) camera.position.x += playerSpeed;

        // Shooting
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Ray ray = GetMouseRay(GetMousePosition(), camera);
            for (int i = 0; i < MAX_TARGETS; i++) {
                if (targets[i].active) {
                    BoundingBox box = {
                        (Vector3){ targets[i].position.x - 0.5f, targets[i].position.y - 0.5f, targets[i].position.z - 0.5f },
                        (Vector3){ targets[i].position.x + 0.5f, targets[i].position.y + 0.5f, targets[i].position.z + 0.5f }
                    };
                    if (GetRayCollisionBox(ray, box).hit) {
                        targets[i].active = false;
                        score++;
                    }
                }
            }
        }

        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                // Draw ground
                DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 32.0f, 32.0f }, LIGHTGRAY);

                // Draw targets
                for (int i = 0; i < MAX_TARGETS; i++) {
                    if (targets[i].active) {
                        DrawCube(targets[i].position, 1.0f, 1.0f, 1.0f, RED);
                        DrawCubeWires(targets[i].position, 1.0f, 1.0f, 1.0f, DARKGRAY);
                    }
                }

            EndMode3D();

            DrawText(TextFormat("Score: %d", score), 10, 10, 20, DARKGRAY);
            DrawText("WASD to move, Mouse to look, Left click to shoot", 10, 40, 20, DARKGRAY);
            DrawFPS(10, 70);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}