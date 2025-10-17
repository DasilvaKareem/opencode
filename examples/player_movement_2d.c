#include "raylib.h"

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "raylib - 2D Player Movement");

    Vector2 playerPos = { screenWidth / 2.0f, screenHeight / 2.0f };
    float playerSpeed = 5.0f;
    float playerRadius = 20.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        if (IsKeyDown(KEY_RIGHT)) playerPos.x += playerSpeed;
        if (IsKeyDown(KEY_LEFT)) playerPos.x -= playerSpeed;
        if (IsKeyDown(KEY_UP)) playerPos.y -= playerSpeed;
        if (IsKeyDown(KEY_DOWN)) playerPos.y += playerSpeed;

        if (playerPos.x < playerRadius) playerPos.x = playerRadius;
        if (playerPos.x > screenWidth - playerRadius) playerPos.x = screenWidth - playerRadius;
        if (playerPos.y < playerRadius) playerPos.y = playerRadius;
        if (playerPos.y > screenHeight - playerRadius) playerPos.y = screenHeight - playerRadius;

        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawCircleV(playerPos, playerRadius, RED);

            DrawText("Move the ball with arrow keys", 10, 10, 20, DARKGRAY);
            DrawFPS(10, 40);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
