#ifndef PARTICLES_H
#define PARTICLES_H

#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>

#define MAX_PARTICLES 500

// Particle structure
typedef struct {
    Vector3 position;
    Vector3 velocity;
    Color color;
    float lifetime;      // Seconds remaining
    float maxLifetime;   // Original lifetime
    float size;
    bool active;
} Particle;

// Particle system
typedef struct {
    Particle particles[MAX_PARTICLES];
    int particleCount;
} ParticleSystem;

// Initialize particle system
void InitParticleSystem(ParticleSystem* ps) {
    ps->particleCount = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        ps->particles[i].active = false;
    }
}

// Emit particles from a position
void EmitParticles(ParticleSystem* ps, Vector3 position, Color color, int count) {
    for (int i = 0; i < count; i++) {
        // Find inactive particle slot
        int index = -1;
        for (int j = 0; j < MAX_PARTICLES; j++) {
            if (!ps->particles[j].active) {
                index = j;
                break;
            }
        }

        if (index == -1) continue; // No free slots

        Particle* p = &ps->particles[index];
        p->position = position;

        // Random velocity spread
        float spreadX = ((float)GetRandomValue(-50, 50) / 100.0f) * 0.5f;
        float spreadZ = ((float)GetRandomValue(-50, 50) / 100.0f) * 0.5f;
        float spreadY = ((float)GetRandomValue(0, 100) / 100.0f) * 0.3f;

        p->velocity = (Vector3){spreadX, spreadY, spreadZ};
        p->color = color;
        p->lifetime = 0.5f + ((float)GetRandomValue(0, 50) / 100.0f); // 0.5-1.0 seconds
        p->maxLifetime = p->lifetime;
        p->size = 0.3f + ((float)GetRandomValue(0, 30) / 100.0f); // 0.3-0.6
        p->active = true;

        if (index >= ps->particleCount) {
            ps->particleCount = index + 1;
        }
    }
}

// Emit trail particles (continuous effect)
void EmitTrailParticles(ParticleSystem* ps, Vector3 position, Vector3 velocity, Color color) {
    // Emit 0-1 particles per frame for trail effect
    int count = GetRandomValue(0, 1);

    for (int i = 0; i < count; i++) {
        // Find inactive particle slot
        int index = -1;
        for (int j = 0; j < MAX_PARTICLES; j++) {
            if (!ps->particles[j].active) {
                index = j;
                break;
            }
        }

        if (index == -1) continue;

        Particle* p = &ps->particles[index];

        // Slightly offset position for variety
        p->position.x = position.x + ((float)GetRandomValue(-10, 10) / 20.0f);
        p->position.y = position.y + ((float)GetRandomValue(0, 20) / 20.0f);
        p->position.z = position.z + ((float)GetRandomValue(-10, 10) / 20.0f);

        // Particles drift slowly perpendicular to movement
        Vector3 perpendicular = {-velocity.z, 0, velocity.x};
        float drift = ((float)GetRandomValue(-30, 30) / 100.0f);
        p->velocity = Vector3Scale(perpendicular, drift);
        p->velocity.y = ((float)GetRandomValue(-5, 15) / 100.0f); // Slight upward drift

        p->color = color;
        p->lifetime = 0.5f + ((float)GetRandomValue(0, 30) / 100.0f); // 0.5-0.8 seconds
        p->maxLifetime = p->lifetime;
        p->size = 0.4f + ((float)GetRandomValue(0, 20) / 100.0f);
        p->active = true;

        if (index >= ps->particleCount) {
            ps->particleCount = index + 1;
        }
    }
}

// Update particle system
void UpdateParticleSystem(ParticleSystem* ps, float deltaTime) {
    for (int i = 0; i < ps->particleCount; i++) {
        Particle* p = &ps->particles[i];

        if (!p->active) continue;

        // Update lifetime
        p->lifetime -= deltaTime;

        if (p->lifetime <= 0.0f) {
            p->active = false;
            continue;
        }

        // Update position
        p->position = Vector3Add(p->position, Vector3Scale(p->velocity, deltaTime));

        // Apply gravity/drag
        p->velocity.y -= 0.5f * deltaTime;
        p->velocity = Vector3Scale(p->velocity, 0.98f); // Drag
    }
}

// Draw particle system with glow
void DrawParticleSystem(ParticleSystem* ps) {
    for (int i = 0; i < ps->particleCount; i++) {
        Particle* p = &ps->particles[i];

        if (!p->active) continue;

        // Calculate fade based on lifetime (fade out at end)
        float lifetimeRatio = p->lifetime / p->maxLifetime;
        float alpha = lifetimeRatio * 255.0f;

        Color particleColor = p->color;
        particleColor.a = (unsigned char)alpha;

        // Draw particle core only (removed glow for performance)
        DrawSphere(p->position, p->size, particleColor);
    }
}

// Draw boost particles (more intense)
void EmitBoostParticles(ParticleSystem* ps, Vector3 position, Vector3 direction, Color color) {
    int count = GetRandomValue(1, 2); // Optimized particle count for boost

    for (int i = 0; i < count; i++) {
        int index = -1;
        for (int j = 0; j < MAX_PARTICLES; j++) {
            if (!ps->particles[j].active) {
                index = j;
                break;
            }
        }

        if (index == -1) continue;

        Particle* p = &ps->particles[index];

        // Position behind the bike
        p->position = Vector3Subtract(position, Vector3Scale(direction, 3.0f));
        p->position.x += ((float)GetRandomValue(-15, 15) / 20.0f);
        p->position.y += ((float)GetRandomValue(0, 10) / 20.0f);
        p->position.z += ((float)GetRandomValue(-15, 15) / 20.0f);

        // Particles shoot backward
        p->velocity = Vector3Scale(direction, -2.0f);
        p->velocity.x += ((float)GetRandomValue(-30, 30) / 50.0f);
        p->velocity.y += ((float)GetRandomValue(-10, 30) / 50.0f);
        p->velocity.z += ((float)GetRandomValue(-30, 30) / 50.0f);

        p->color = color;
        p->lifetime = 0.4f + ((float)GetRandomValue(0, 30) / 100.0f);
        p->maxLifetime = p->lifetime;
        p->size = 0.5f + ((float)GetRandomValue(0, 30) / 100.0f);
        p->active = true;

        if (index >= ps->particleCount) {
            ps->particleCount = index + 1;
        }
    }
}

#endif // PARTICLES_H
