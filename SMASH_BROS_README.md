# Super Smash Bros Platform Fighter 🎮

A polished raylib-based platform fighter inspired by Super Smash Bros featuring percentage-based knockback, stock lives, and **full PS5 DualSense controller support**!

## ✨ New Features

### 🎮 PS5 Controller Support

- **Full DualSense Integration**: Automatic controller detection
- **Analog Stick Movement**: Smooth character control with deadzone
- **Button Mapping**:
  - X Button / D-Pad Up = Jump
  - Square / R1 / R2 = Attack
  - Left Stick / D-Pad = Movement
  - START Button = Restart game
- **Multi-Controller**: Supports 2 players with separate controllers
- **Visual Indicator**: 🎮 icon shows when controller is connected
- **Keyboard Fallback**: Seamless switch between controller and keyboard

### 🛡️ Polish & Bug Fixes

- **Respawn Invincibility**: 2 seconds of invincibility after respawning (flashing effect)
- **Facing Direction**: Characters face the direction they're moving with eye indicator
- **Improved Knockback**: Fixed velocity override for more consistent launches
- **Attack Hit Detection**: Prevents multi-hitting from single attack
- **Platform Collision**: More reliable platform detection
- **Visual Polish**:
  - Platform outlines and highlights
  - Transparent HUD backgrounds
  - Percentage damage display with background
  - Smoother animations

## Core Mechanics

### Percentage System

- **Damage Accumulation**: 0% to 999%
- **Knockback Scaling**: Higher percentage = further launch distance
- **Visual Feedback**: White flash on hit + percentage display

### Stock Lives System

- Each player starts with **3 stocks**
- Fall off stage = lose 1 stock
- Last player standing wins!

### Combat

- **Attack Cooldown**: Prevents spam (30 frames)
- **Hitstun**: Can't act while stunned, scales with damage
- **Knockback Formula**: `Base (8.0) + (Percentage × 0.08)`
- **Launch Direction**: Based on attacker's position

### Movement

- **Double Jump**: Jump once in air for recovery
- **Air Control**: Momentum-based movement
- **Platform Pass-Through**: Jump through platforms from below

## 🕹️ Controls

### Player 1 (Blue) 🔵

**Keyboard:**

- **A / D**: Move left/right
- **W**: Jump (double jump in air)
- **SPACE**: Attack

**PS5 Controller (Gamepad 0):**

- **Left Stick / D-Pad**: Move
- **X / D-Pad Up**: Jump
- **Square / R1 / R2**: Attack

### Player 2 (Red) 🔴

**Keyboard:**

- **Arrow Keys**: Move left/right/jump
- **UP Arrow**: Jump (double jump in air)
- **RIGHT CTRL**: Attack

**PS5 Controller (Gamepad 1):**

- **Left Stick / D-Pad**: Move
- **X / D-Pad Up**: Jump
- **Square / R1 / R2**: Attack

### General

- **R / START**: Restart game (when game over)
- **ESC**: Close game

## 🎯 How to Play

1. **Connect Controllers**: Plug in PS5 controllers before or during gameplay
2. **Movement**: Use stick/keys to navigate platforms
3. **Attacking**: Hit opponents to increase their damage percentage
4. **Launching**: At high percentages, knock them off-stage!
5. **Recovery**: Use double jump to get back to stage
6. **Victory**: Eliminate all opponent stocks to win!

## 🏗️ Stage Layout

```
        [Top Center Platform]

  [Top Left]     [Top Right]

  [Left Side]    [Right Side]

    ========================
         [Main Platform]
```

- **6 Platforms**: Multiple levels for strategic play
- **Fall-off Edges**: All sides lead to death
- **Sky Blue Background**: Classic Smash aesthetic

## 🚀 Compilation & Running

### Quick Start

```bash
make -f smash_bros_makefile run
```

### Manual Compilation (macOS)

```bash
gcc smash_bros.c -o smash_bros \
  -I/opt/homebrew/Cellar/raylib/5.5/include \
  -L/opt/homebrew/Cellar/raylib/5.5/lib \
  -lraylib -framework CoreVideo -framework IOKit \
  -framework Cocoa -framework GLUT -framework OpenGL

./smash_bros
```

### Linux

```bash
gcc smash_bros.c -o smash_bros -lraylib -lm -lpthread -ldl -lrt -lX11
./smash_bros
```

## 📊 Game Mechanics Explained

### Knockback System

The knockback formula creates Smash-like gameplay:

| Damage % | Launch Power | Result          |
| -------- | ------------ | --------------- |
| 0%       | 8.0          | Barely moves    |
| 50%      | 12.0         | Moderate launch |
| 100%     | 16.0         | Strong launch   |
| 200%     | 24.0         | Extreme launch! |

### Hitstun Duration

```
Hitstun Frames = 15 + (Percentage × 0.2)
```

- At 0%: 15 frames (0.25 seconds)
- At 100%: 35 frames (0.58 seconds)
- At 200%: 55 frames (0.92 seconds)

### Frame Data

- **Attack Cooldown**: 30 frames (0.5 seconds)
- **Attack Active**: 10 frames
- **Jump Power**: -13 units
- **Move Speed**: 5 units/frame
- **Gravity**: 0.6 units/frame²
- **Max Fall Speed**: 15 units/frame

## 🎮 PS5 Controller Setup

### macOS

1. **Pair Controller**:
   - Hold PS + Share button until light bar flashes
   - Open Bluetooth settings and connect
2. **Launch Game**: Controller auto-detected!

### Linux

1. **Install DS4 Driver** (if needed):
   ```bash
   sudo apt-get install ds4drv
   ```
2. **Pair via Bluetooth**: Same as macOS
3. **Run Game**: Should work automatically

### Windows

- Use Steam Input or DS4Windows for compatibility
- Controllers should auto-map through raylib

## 🎯 Strategy Guide

### Beginner Tips

1. **Learn to Recover**: Master the double jump timing
2. **Center Stage**: Control the main platform
3. **Combo at Low %**: Chain attacks when opponent is fresh
4. **Kill at High %**: Go for stage KO at 100%+

### Advanced Techniques

1. **Edge Guarding**: Attack opponents during recovery
2. **Platform Movement**: Use upper platforms to escape
3. **Bait & Punish**: Let opponent attack, then counter
4. **Momentum Control**: Use air movement for survival

### Character Properties

- **Size**: 40×50 units
- **Weight**: Affects knockback resistance (currently equal)
- **Speed**: 5 units ground, 0.98× air friction

## 🐛 Bug Fixes (v2.0)

✅ **Fixed velocity override bug**: Knockback now sets velocity instead of adding
✅ **Fixed multi-hit attacks**: Attacks end on successful hit
✅ **Fixed platform collisions**: Improved detection reliability  
✅ **Fixed respawn camping**: Added invincibility frames
✅ **Fixed facing direction**: Characters track movement direction
✅ **Fixed attack direction**: Hitbox follows facing direction
✅ **Fixed gamepad detection**: Hot-plug support for controllers

## 📈 Technical Details

- **Engine**: raylib 5.5
- **Language**: C (C11 standard)
- **FPS**: 60 (locked)
- **Resolution**: 1280×720
- **Players**: 2 (local multiplayer)
- **Controller Support**: Up to 4 gamepads (2 currently used)
- **Input Latency**: < 1 frame
- **Physics**: Custom implementation

## 🔮 Future Enhancements

### Planned Features

- [ ] 4-player support
- [ ] Special moves (side-B, up-B, down-B, neutral-B)
- [ ] Charged smash attacks
- [ ] Shield and dodge mechanics
- [ ] Ledge grabbing and recovery
- [ ] Multiple characters with unique stats
- [ ] Multiple stages with different layouts
- [ ] Items and power-ups
- [ ] Game modes (Time, Stock, Training)
- [ ] Sound effects and music
- [ ] Particle effects
- [ ] Victory animations
- [ ] Character select screen
- [ ] Replay system

### Potential Characters

- **Brawler**: High damage, slow movement
- **Speedster**: Fast attacks, low damage
- **Tank**: Heavy weight, strong knockback resistance
- **Aerial**: Better air control, weak ground game

## 🎨 Visual Features

- **Sky Blue Background**: Classic Smash aesthetic
- **Color-Coded Players**: Easy identification (Blue vs Red)
- **Damage Indicators**: Real-time percentage display
- **Stock Icons**: Visual lives counter
- **Hitstun Flash**: White flash on hit
- **Respawn Flash**: Invincibility indicator
- **Attack Hitboxes**: Red transparent boxes (debug visual)
- **Platform Highlights**: 3D-style shading
- **Facing Indicator**: Eye shows direction
- **HUD Backgrounds**: Semi-transparent panels

## 📝 Code Structure

```c
// Main Components
- Player System: Position, velocity, state management
- Stage System: Platform collision detection
- Combat System: Attack hitboxes, knockback calculation
- Input System: Keyboard + gamepad with deadzone
- Rendering: Layered drawing with effects
- Game Loop: 60 FPS fixed timestep
```

## 🏆 Credits

**Built with**: [raylib](https://www.raylib.com/) - A simple and easy-to-use library to enjoy videogames programming

**Inspired by**: Super Smash Bros series by Nintendo / HAL Laboratory

**Game Design**: Platform fighter mechanics from Smash Bros Melee/Ultimate

---

## 🎮 Quick Reference Card

### Essential Controls

| Action     | P1 Keyboard | P2 Keyboard | PS5 Controller   |
| ---------- | ----------- | ----------- | ---------------- |
| Move Left  | A           | ←           | Left Stick/D-Pad |
| Move Right | D           | →           | Left Stick/D-Pad |
| Jump       | W           | ↑           | X Button         |
| Attack     | Space       | Right Ctrl  | Square/R1/R2     |
| Restart    | R           | R           | START            |

### Game Rules

- **3 Stocks** per player
- **Fall off = -1 Stock**
- **High % = Far Launch**
- **Last Standing Wins**

---

**Have fun smashing with your PS5 controllers!** 🎮✨🔥
