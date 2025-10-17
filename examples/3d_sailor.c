#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>

#define WATER_SIZE 200.0f
#define GRAVITY 9.81f
#define WATER_DENSITY 1000.0f
#define BOAT_MASS 500.0f
#define MAX_SPEED 15.0f

typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 rotation;
    Vector3 angularVelocity;
    float sailAngle;
    float rudderAngle;
} Boat;

typedef struct {
    Vector3 direction;
    float strength;
} Wind;



float GetWaterHeight(float x, float z, float time) {
    float wave1 = sinf(x * 0.1f + time * 2.0f) * 0.3f;
    float wave2 = cosf(z * 0.15f + time * 1.5f) * 0.25f;
    float wave3 = sinf((x + z) * 0.08f + time * 1.8f) * 0.2f;
    return wave1 + wave2 + wave3;
}

void UpdateBoatPhysics(Boat *boat, Wind wind, float dt) {
    Vector3 right = {cosf(boat->rotation.y), 0, -sinf(boat->rotation.y)};
    
    float waterHeight = GetWaterHeight(boat->position.x, boat->position.z, GetTime());
    
    Vector3 buoyancy = {0, 0, 0};
    if (boat->position.y < waterHeight) {
        float submersion = waterHeight - boat->position.y;
        buoyancy.y = submersion * WATER_DENSITY * 0.5f;
    }
    
    Vector3 gravity = {0, -GRAVITY * BOAT_MASS, 0};
    
    float effectiveSailAngle = boat->sailAngle + boat->rotation.y;
    Vector3 sailNormal = {sinf(effectiveSailAngle), 0, cosf(effectiveSailAngle)};
    
    Vector3 apparentWind = Vector3Subtract(wind.direction, Vector3Scale(boat->velocity, 0.5f));
    float windOnSail = sailNormal.x * apparentWind.x + sailNormal.y * apparentWind.y + sailNormal.z * apparentWind.z;
    
    Vector3 sailForce = Vector3Scale(sailNormal, windOnSail * wind.strength * 50.0f);
    
    if (windOnSail < 0) sailForce = Vector3Scale(sailForce, 0.3f);
    
    float speed = Vector3Length(boat->velocity);
    Vector3 dragDirection = Vector3Normalize(boat->velocity);
    Vector3 drag = Vector3Scale(dragDirection, -speed * speed * 2.0f);
    
    Vector3 rudderForce = Vector3Scale(right, boat->rudderAngle * speed * 30.0f);
    
    Vector3 totalForce = Vector3Add(Vector3Add(Vector3Add(buoyancy, gravity), sailForce), drag);
    totalForce = Vector3Add(totalForce, rudderForce);
    
    Vector3 acceleration = Vector3Scale(totalForce, 1.0f / BOAT_MASS);
    boat->velocity = Vector3Add(boat->velocity, Vector3Scale(acceleration, dt));
    
    float maxSpeed = MAX_SPEED;
    if (Vector3Length(boat->velocity) > maxSpeed) {
        boat->velocity = Vector3Scale(Vector3Normalize(boat->velocity), maxSpeed);
    }
    
    boat->position = Vector3Add(boat->position, Vector3Scale(boat->velocity, dt));
    
    float turnRate = boat->rudderAngle * speed * 0.3f;
    boat->rotation.y += turnRate * dt;
    
    boat->position.y = waterHeight + 0.5f;
    
    if (fabsf(boat->position.x) > WATER_SIZE / 2) boat->position.x = boat->position.x > 0 ? WATER_SIZE / 2 : -WATER_SIZE / 2;
    if (fabsf(boat->position.z) > WATER_SIZE / 2) boat->position.z = boat->position.z > 0 ? WATER_SIZE / 2 : -WATER_SIZE / 2;
}

void DrawWater(float time) {
    for (int x = -20; x < 20; x++) {
        for (int z = -20; z < 20; z++) {
            float x1 = x * 10.0f, z1 = z * 10.0f;
            float x2 = (x + 1) * 10.0f, z2 = (z + 1) * 10.0f;
            
            float h1 = GetWaterHeight(x1, z1, time);
            float h2 = GetWaterHeight(x2, z1, time);
            float h3 = GetWaterHeight(x2, z2, time);
            float h4 = GetWaterHeight(x1, z2, time);
            
            DrawTriangle3D(
                (Vector3){x1, h1, z1},
                (Vector3){x2, h2, z1},
                (Vector3){x2, h3, z2},
                (Color){30, 144, 255, 180}
            );
            DrawTriangle3D(
                (Vector3){x1, h1, z1},
                (Vector3){x2, h3, z2},
                (Vector3){x1, h4, z2},
                (Color){30, 144, 255, 180}
            );
        }
    }
}

void DrawBoat(Boat boat) {
    Vector3 forward = {sinf(boat.rotation.y), 0, cosf(boat.rotation.y)};
    Vector3 right = {cosf(boat.rotation.y), 0, -sinf(boat.rotation.y)};
    
    Vector3 hullPoints[8];
    hullPoints[0] = Vector3Add(boat.position, Vector3Add(Vector3Scale(forward, 2.5f), (Vector3){0, 0, 0}));
    hullPoints[1] = Vector3Add(boat.position, Vector3Add(Vector3Scale(forward, -2.5f), Vector3Scale(right, 1.0f)));
    hullPoints[2] = Vector3Add(boat.position, Vector3Add(Vector3Scale(forward, -2.5f), Vector3Scale(right, -1.0f)));
    
    DrawTriangle3D(hullPoints[0], hullPoints[1], hullPoints[2], BROWN);
    
    Vector3 hullBottom = Vector3Add(boat.position, (Vector3){0, -0.5f, 0});
    DrawTriangle3D(hullPoints[1], hullBottom, hullPoints[0], DARKBROWN);
    DrawTriangle3D(hullPoints[2], hullBottom, hullPoints[0], DARKBROWN);
    DrawTriangle3D(hullPoints[1], hullBottom, hullPoints[2], DARKBROWN);
    
    Vector3 mastBase = boat.position;
    Vector3 mastTop = Vector3Add(mastBase, (Vector3){0, 4.0f, 0});
    DrawCylinderEx(mastBase, mastTop, 0.1f, 0.1f, 8, DARKGRAY);
    
    float sailAngle = boat.rotation.y + boat.sailAngle;
    Vector3 sailDir = {sinf(sailAngle), 0, cosf(sailAngle)};
    
    Vector3 sailPoints[4];
    sailPoints[0] = Vector3Add(mastBase, (Vector3){0, 0.5f, 0});
    sailPoints[1] = Vector3Add(sailPoints[0], Vector3Scale(sailDir, 2.5f));
    sailPoints[2] = Vector3Add(mastTop, Vector3Scale(sailDir, 2.0f));
    sailPoints[3] = Vector3Add(mastTop, (Vector3){0, -0.5f, 0});
    
    DrawTriangle3D(sailPoints[0], sailPoints[1], sailPoints[2], WHITE);
    DrawTriangle3D(sailPoints[0], sailPoints[2], sailPoints[3], WHITE);
    
    Vector3 rudderBase = Vector3Add(boat.position, Vector3Scale(forward, -2.5f));
    rudderBase.y -= 0.3f;
    Vector3 rudderEnd = Vector3Add(rudderBase, (Vector3){0, -0.8f, 0});
    DrawCylinderEx(rudderBase, rudderEnd, 0.08f, 0.08f, 6, DARKBROWN);
}

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "3D Sailor - Sailing Simulation");

    Camera camera = {0};
    camera.position = (Vector3){0.0f, 15.0f, -20.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Boat boat = {0};
    boat.position = (Vector3){0, 1, 0};
    boat.velocity = (Vector3){0, 0, 0};
    boat.rotation = (Vector3){0, 0, 0};
    boat.sailAngle = 0.5f;
    boat.rudderAngle = 0.0f;

    Wind wind = {0};
    wind.direction = (Vector3){1.0f, 0, 1.0f};
    wind.direction = Vector3Normalize(wind.direction);
    wind.strength = 5.0f;

    float cameraAngle = 0.0f;
    float cameraDistance = 20.0f;
    float cameraHeight = 12.0f;

    DisableCursor();
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        Vector2 mouseDelta = GetMouseDelta();
        cameraAngle -= mouseDelta.x * 0.003f;
        cameraHeight += mouseDelta.y * 0.1f;
        cameraHeight = fmaxf(2.0f, fminf(30.0f, cameraHeight));
        
        float mouseWheel = GetMouseWheelMove();
        cameraDistance -= mouseWheel * 2.0f;
        cameraDistance = fmaxf(5.0f, fminf(50.0f, cameraDistance));
        
        if (IsKeyDown(KEY_LEFT)) boat.rudderAngle = 1.0f;
        else if (IsKeyDown(KEY_RIGHT)) boat.rudderAngle = -1.0f;
        else boat.rudderAngle = 0.0f;
        
        if (IsKeyDown(KEY_A)) boat.sailAngle -= 1.0f * dt;
        if (IsKeyDown(KEY_D)) boat.sailAngle += 1.0f * dt;
        boat.sailAngle = fmaxf(-1.5f, fminf(1.5f, boat.sailAngle));
        
        if (IsKeyDown(KEY_W)) wind.strength += 1.0f * dt;
        if (IsKeyDown(KEY_S)) wind.strength -= 1.0f * dt;
        wind.strength = fmaxf(0.0f, fminf(15.0f, wind.strength));
        
        UpdateBoatPhysics(&boat, wind, dt);
        
        camera.position.x = boat.position.x + cosf(cameraAngle) * cameraDistance;
        camera.position.y = boat.position.y + cameraHeight;
        camera.position.z = boat.position.z + sinf(cameraAngle) * cameraDistance;
        camera.target = boat.position;

        BeginDrawing();
            ClearBackground(SKYBLUE);

            BeginMode3D(camera);
                DrawWater(GetTime());
                DrawBoat(boat);
                DrawGrid(20, 10.0f);
                
                Vector3 windStart = Vector3Add(boat.position, (Vector3){5, 8, 5});
                Vector3 windEnd = Vector3Add(windStart, Vector3Scale(wind.direction, wind.strength * 0.5f));
                DrawLine3D(windStart, windEnd, RED);
                DrawSphere(windEnd, 0.3f, RED);
            EndMode3D();

            DrawText("3D SAILOR", 10, 10, 30, DARKBLUE);
            DrawText("CONTROLS:", 10, 50, 20, BLACK);
            DrawText("MOUSE - Rotate Camera", 10, 75, 16, DARKGRAY);
            DrawText("WHEEL - Zoom In/Out", 10, 95, 16, DARKGRAY);
            DrawText("LEFT/RIGHT - Rudder", 10, 115, 16, DARKGRAY);
            DrawText("A/D - Adjust Sail", 10, 135, 16, DARKGRAY);
            DrawText("W/S - Wind Strength", 10, 155, 16, DARKGRAY);
            
            float speed = Vector3Length(boat.velocity);
            DrawText(TextFormat("Speed: %.1f knots", speed), 10, 185, 16, DARKGREEN);
            DrawText(TextFormat("Wind: %.1f", wind.strength), 10, 205, 16, DARKGREEN);
            DrawText(TextFormat("Sail Angle: %.1f", boat.sailAngle * 57.3f), 10, 225, 16, DARKGREEN);
            DrawText(TextFormat("Rudder: %.1f", boat.rudderAngle), 10, 245, 16, DARKGREEN);
            
            DrawFPS(screenWidth - 100, 10);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
