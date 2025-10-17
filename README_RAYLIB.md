# OpenCode - Raylib Game Development Agent (C & Python)

This OpenCode instance is configured as a **raylib game development agent** powered by **Amazon Bedrock Sonnet 4.5**.

## 🎮 What This Does

OpenCode is now specialized for creating 2D and 3D games using raylib in **C** and **Python**. The AI agent understands:
- raylib API v5.5 (window, drawing, input, textures, models, audio)
- C programming with proper memory management
- Python programming with raylib (pyray bindings)
- Game development patterns
- 2D graphics (sprites, animations, particles)
- 3D graphics (models, cameras, shaders, lighting)
- Physics and collision detection
- Audio integration

## 🚀 Quick Start

### 1. Set up AWS Bedrock credentials with .env file:

Edit the `.env` file and add your credentials:
```bash
AWS_ACCESS_KEY_ID=your_access_key_here
AWS_SECRET_ACCESS_KEY=your_secret_key_here
AWS_REGION=us-east-1
```

Or use AWS CLI profile in `.env`:
```bash
AWS_PROFILE=your_profile_name
```

### 2. Install Python dependencies:
```bash
./setup.sh
```

This will:
- Create a Python virtual environment
- Install raylib Python bindings (pyray)
- Install AWS SDK (boto3)
- Install python-dotenv for .env support

### 3. Install raylib for C development (optional):
```bash
# macOS
brew install raylib

# Ubuntu/Debian
sudo apt install libraylib-dev

# Or build from source: https://github.com/raysan5/raylib
```

### 3. Run OpenCode:
```bash
bun dev
```

### 4. Use raylib-specific commands:

**Create a 2D game:**
```
/2d-game name=platformer description="A simple platformer game"
```

**Create a 3D game:**
```
/3d-game name=fps description="First person shooter" camera_type="first-person"
```

**Create a shader example:**
```
/shader-example effect_type="wave distortion" target="2D texture"
```

## 📁 Project Structure

```
.
├── .env                 # AWS Bedrock credentials (create from .env.example)
├── .env.example         # Template for environment variables
├── setup.sh             # Python setup script
├── requirements.txt     # Python dependencies
├── .opencode/
│   └── command/         # Custom raylib game templates
│       ├── 2d-game.md
│       ├── 3d-game.md
│       └── shader-example.md
├── examples/            # Example raylib programs
│   ├── basic_window.c
│   ├── basic_window.py
│   ├── player_movement_2d.c
│   ├── player_movement_2d.py
│   ├── basic_3d.c
│   ├── basic_3d.py
│   └── particle_system.py
├── Makefile            # Build configuration for C examples
└── opencode.json       # Agent configuration
```

## 🎨 Example Games Included

### Python Examples (Run after `./setup.sh`):
```bash
# Activate virtual environment
source venv/bin/activate

# Run examples
python examples/basic_window.py
python examples/player_movement_2d.py
python examples/basic_3d.py
python examples/particle_system.py
```

### C Examples:
```bash
make examples/basic_window
./examples/basic_window

make examples/player_movement_2d
./examples/player_movement_2d

make examples/basic_3d
./examples/basic_3d
```

## 💡 Example Prompts

- "Create a simple pong game with raylib in Python"
- "Make a 3D fps camera controller in C"
- "Add particle effects to my game"
- "Create a shader that makes water ripple effect"
- "Build a 2D platformer with jumping and gravity in Python"
- "Make a 3D model viewer with orbit camera in C"
- "Create a space invaders clone"
- "Build a racing game with 3D perspective"

## 🛠️ Build Commands

### Python:
```bash
# Setup environment
./setup.sh

# Run any Python game
python your_game.py
```

### C:
The agent knows these build commands for different platforms:

**Linux:**
```bash
gcc game.c -o game -lraylib -lm -lpthread -ldl -lrt -lX11
```

**macOS:**
```bash
gcc game.c -o game -lraylib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
```

**Windows (MinGW):**
```bash
gcc game.c -o game.exe -lraylib -lopengl32 -lgdi32 -lwinmm
```

## 📚 Resources

- [raylib Cheatsheet](https://www.raylib.com/cheatsheet/cheatsheet.html)
- [raylib Examples](https://www.raylib.com/examples.html)
- [raylib Documentation](https://www.raylib.com/)
- [Amazon Bedrock](https://aws.amazon.com/bedrock/)

## ⚙️ Configuration

The agent configuration is in `opencode.json`:
- **Model**: `anthropic.claude-sonnet-4-5-20250514-v1:0` (Bedrock)
- **Small Model**: `anthropic.claude-3-5-haiku-20241022-v1:0` (Bedrock)
- **Provider**: Amazon Bedrock (us-east-1)
- **Agent**: raylib (primary mode)

## 🎯 Agent Capabilities

The raylib agent has these tools enabled:
- `bash` - Compile and run games
- `read` - Read source files
- `write` - Create new game files
- `edit` - Modify existing code
- `glob` - Find files
- `grep` - Search code

## 🔧 Troubleshooting

**Bedrock not working?**
- Verify AWS credentials are set
- Check your AWS region supports Sonnet 4.5
- Ensure you have Bedrock access enabled

**raylib not found?**
- Install raylib: `brew install raylib` (macOS)
- Or build from source: https://github.com/raysan5/raylib

**Compilation errors?**
- Check raylib is in your library path
- Verify you're using the correct platform flags

---

Happy game development with raylib! 🎮✨
