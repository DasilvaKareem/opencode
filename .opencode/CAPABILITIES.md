# Agent Capabilities System

## Overview

Agents now have a capabilities manifest system that allows them to know their capabilities from the start, without needing to search through code.

## How It Works

1. **Capabilities Manifest**: Each agent can have a `.capabilities.json` file that defines what it can do
2. **Auto-Loading**: When an agent starts, its capabilities are automatically loaded and added to the system prompt
3. **Self-Awareness**: The agent immediately knows what it can/can't do without code exploration

## Creating a Capabilities Manifest

Create a file named `{agent-name}.capabilities.json` in `.opencode/agent/`:

```json
{
  "agent": "asset",
  "version": "1.0.0",
  "capabilities": {
    "assetTypes": [
      {
        "type": "video",
        "description": "Video content including anime-style animations",
        "provider": "fal",
        "parameters": ["aspectRatio", "resolution", "seed"]
      }
    ],
    "features": [
      "Feature 1",
      "Feature 2"
    ],
    "limitations": [
      "Limitation 1"
    ]
  },
  "examples": [
    {
      "task": "Generate anime video",
      "capability": "video",
      "supported": true
    }
  ]
}
```

## Benefits

- **No Discovery Phase**: Agent doesn't waste time/tokens searching code
- **Faster Responses**: Immediate answers about capabilities
- **Better UX**: No uncertainty about what the agent can do
- **Maintainable**: Capabilities defined in declarative JSON format

## Example: Asset Agent

Before: Had to read `types.ts`, `worker-pool.ts`, etc. to discover it could generate videos

After: Knows immediately from `asset.capabilities.json` that it supports:
- sprites
- textures
- 3d-model
- sfx
- music
- voice
- **video** (including anime)
