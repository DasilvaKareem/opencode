# Asset Creator Agent

**Purpose**: Generate game assets including sprites, textures, 3D models, sound effects, music, voice, and video using AI providers (Fal AI and ElevenLabs).

**When to use**:
- User requests asset generation for their game/project
- Need to create multiple assets in batch (sprites, models, audio, etc.)
- Want to generate placeholder assets during prototyping
- Building a complete asset library for a game

**Capabilities**:
- **2D Assets**: Generate sprites and textures using Fal AI Imagen4
- **3D Assets**: Create 3D models (GLB, USDZ, FBX, OBJ, STL) using Fal AI Rodin
- **Video**: Generate short video clips using Fal AI WAN
- **Sound Effects**: Create SFX using ElevenLabs
- **Music**: Generate background music with genre selection using ElevenLabs
- **Voice**: Text-to-speech generation using ElevenLabs

**Asset Types Supported**:
- `sprite`: 2D game sprites (PNG)
- `texture`: 2D textures for materials (PNG)
- `3d-model`: 3D models in various formats
- `video`: Short video clips (MP4)
- `sfx`: Sound effects (MP3)
- `music`: Background music (MP3)
- `voice`: Voice/narration (MP3)

**Input Format**:
```json
{
  "assets": [
    {
      "name": "player_idle",
      "type": "sprite",
      "description": "pixel art character standing idle, 32x32, blue shirt",
      "parameters": {
        "aspectRatio": "1:1",
        "resolution": "1K",
        "seed": 12345
      }
    },
    {
      "name": "jump_sound",
      "type": "sfx",
      "description": "short jumping sound effect, bouncy and light",
      "parameters": {
        "duration": 0.5
      }
    },
    {
      "name": "spaceship",
      "type": "3d-model",
      "description": "low poly sci-fi spaceship with glowing engines",
      "parameters": {
        "format": "glb",
        "quality": "high"
      }
    }
  ],
  "budget": 5.0,
  "maxTime": 300
}
```

**Output Format**:
```json
{
  "success": 25,
  "failed": 0,
  "totalCost": 3.45,
  "totalTime": 58.2,
  "assets": {
    "player_idle": {
      "filePath": "./assets/sprites/player_idle.png",
      "url": "https://...",
      "type": "sprite",
      "metadata": {
        "provider": "fal",
        "cost": 0.05,
        "generationTime": 8.2,
        "seed": 12345,
        "size": 45678
      }
    }
  },
  "failures": {},
  "manifest": {
    "created": "2025-10-16T...",
    "totalAssets": 25,
    "byType": {
      "sprite": 10,
      "sfx": 10,
      "3d-model": 3,
      "music": 2
    },
    "totalCost": 3.45,
    "totalTime": 58.2
  }
}
```

**Features**:
- **Parallel Processing**: Executes multiple assets concurrently (5 Fal AI + 3 ElevenLabs)
- **Smart Prioritization**: Fast assets (SFX, sprites) processed before slow ones (3D models, video)
- **Dependency Management**: Can wait for one asset to complete before generating dependent assets
- **Progress Tracking**: Real-time updates on generation progress
- **Cost Tracking**: Monitors API costs per asset and total batch
- **Retry Logic**: Automatically retries failed generations up to 3 times
- **Organized Output**: Saves assets to organized directories (sprites/, sfx/, models/, etc.)

**Rate Limits**:
- Fal AI: 5 concurrent requests
- ElevenLabs: 3 concurrent requests

**Estimated Costs** (USD):
- Sprite/Texture: $0.05
- 3D Model: $0.40
- Video: $0.30
- SFX: $0.01
- Music: $0.05
- Voice: $0.02

**Estimated Generation Times**:
- Sprite/Texture: ~8 seconds
- 3D Model: ~20 seconds
- Video: ~60 seconds
- SFX: ~5 seconds
- Music: ~30 seconds
- Voice: ~3 seconds

**Example Usage**:

When user says: "Generate 10 explosion sound effects and 5 enemy sprites for my game"

Main Agent should delegate to Asset Creator Agent with:
```json
{
  "assets": [
    {
      "name": "explosion_1",
      "type": "sfx",
      "description": "loud explosion sound effect"
    },
    // ... 9 more explosions
    {
      "name": "enemy_basic",
      "type": "sprite",
      "description": "pixelated zombie enemy sprite, green skin, 32x32"
    }
    // ... 4 more enemies
  ]
}
```

Asset Creator Agent will:
1. Queue all 15 jobs
2. Prioritize SFX (fast) before sprites
3. Execute in parallel (3 ElevenLabs + 5 Fal AI concurrent)
4. Provide real-time progress updates
5. Save all assets to ./assets/ directory
6. Return manifest with file paths and metadata

**Requirements**:
- FAL_KEY environment variable must be set
- ELEVENLABS_API_KEY environment variable must be set
- Both keys should be in .env file

**Error Handling**:
- Retries failed generations up to 3 times
- Reports failures in the result
- Continues processing other assets even if some fail
- Provides detailed error messages for debugging
