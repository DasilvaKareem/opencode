# 🎮 PS5 Controller Setup Guide - Smash Bros Platform Fighter

## ✅ What's Working

The game now has **full 2-player PS5 DualSense controller support** with automatic detection!

### Confirmed Working Features:

- ✅ Automatic controller detection at startup
- ✅ Hot-plug support (connect controllers during gameplay)
- ✅ Individual controller assignment (Player 1 & Player 2)
- ✅ Visual indicators showing controller status (🎮0, 🎮1)
- ✅ Analog stick movement with deadzone
- ✅ D-Pad support
- ✅ Multiple button mappings for attacks
- ✅ START button for restart
- ✅ Keyboard fallback for any player without controller

## 🎮 How to Connect 2 PS5 Controllers

### Method 1: Connect Before Starting Game (Recommended)

1. **First Controller:**
   - Hold **PS + Share** buttons until light bar flashes blue
   - Open **System Settings** → **Bluetooth**
   - Select "DualSense Wireless Controller"
   - Wait for it to connect (solid blue/white light)

2. **Second Controller:**
   - Hold **PS + Share** buttons until light bar flashes blue
   - It should appear as another "DualSense Wireless Controller"
   - Connect it (it will have a different light color)

3. **Launch Game:**

   ```bash
   cd /Users/kareemdasilva/code/opencode
   ./smash_bros
   ```

4. **Check Console Output:**

   ```
   INFO: 🎮 Scanning for PS5 controllers...
   INFO: ✅ Player 1: DualSense Wireless Controller (Gamepad 0)
   INFO: ✅ Player 2: DualSense Wireless Controller (Gamepad 1)
   ```

5. **Check In-Game HUD:**
   - Player 1 HUD (top-left): Should show **🎮0**
   - Player 2 HUD (top-right): Should show **🎮1**

### Method 2: Connect During Gameplay (Hot-Plug)

1. **Start game** with keyboard or 1 controller
2. **Hold PS + Share** on second controller
3. **Connect via Bluetooth settings**
4. Game will **auto-detect** and assign to available player
5. Check HUD for **🎮** indicator

## 🎮 Controller Layout

### PS5 DualSense Mapping:

```
        △ Triangle
    ▢ Square    ○ Circle
        ✕ X (Cross)

Left Stick = Move character
D-Pad = Alternative movement
✕ (X) = Jump (double jump in air)
D-Pad Up = Alternative jump
▢ (Square) = Attack
R1 = Attack
R2 = Attack
START (Options) = Restart game
```

## 🕹️ Full Control Scheme

### Player 1:

**PS5 Controller (Gamepad 0):**

- Left Stick / D-Pad: Move
- X / D-Pad Up: Jump
- Square / R1 / R2: Attack
- START: Restart

**Keyboard Fallback:**

- A/D: Move
- W: Jump
- Space: Attack

### Player 2:

**PS5 Controller (Gamepad 1):**

- Left Stick / D-Pad: Move
- X / D-Pad Up: Jump
- Square / R1 / R2: Attack
- START: Restart

**Keyboard Fallback:**

- Arrow Keys: Move
- Up Arrow: Jump
- Right Ctrl: Attack

## 🔧 Testing Your Controllers

### Quick Test:

```bash
cd /Users/kareemdasilva/code/opencode
./controller_test
```

This will show:

- Connected controllers (Gamepad 0, 1, 2, 3)
- Controller names
- Button press detection
- Analog stick movement visualization

### In-Game Status Icons:

- **🎮0** = Controller 0 connected (Player 1)
- **🎮1** = Controller 1 connected (Player 2)
- **⌨️** = Keyboard mode (no controller)
- **❌** = Controller disconnected

## 🐛 Troubleshooting

### Problem: Only 1 controller detected

**Solution 1: Check Bluetooth pairing**

```bash
# Open System Preferences → Bluetooth
# You should see TWO "DualSense Wireless Controller" entries
# Both should say "Connected"
```

**Solution 2: Re-pair controllers**

1. Remove all DualSense controllers from Bluetooth settings
2. Pair first controller (hold PS+Share)
3. Wait for it to fully connect
4. Pair second controller (hold PS+Share on different controller)
5. Both should now show as connected

**Solution 3: Check USB-C connection**

- Connect controllers via USB-C cable
- macOS should recognize them immediately
- Game will detect both

### Problem: Controllers not responding

**Check 1: Battery level**

- Low battery can cause input lag
- Hold PS button to see battery indicator
- Charge if needed

**Check 2: Deadzone**

- Analog sticks need > 20% movement to register
- Use controller_test tool to verify

**Check 3: Restart game**

- Press ESC to exit
- Re-launch: `./smash_bros`
- Controllers re-detected on startup

### Problem: Wrong player assigned

**Solution:**

- The first controller detected = Player 1 (Blue)
- The second controller detected = Player 2 (Red)
- To swap: Disconnect both, connect in desired order

### Problem: Controller disconnects mid-game

**The game handles this gracefully:**

- Player automatically switches to keyboard fallback
- HUD shows ⌨️ or ❌ indicator
- Reconnect controller (hot-plug supported)
- Game will reassign automatically

## 📊 Controller Detection Logic

The game uses this system:

1. **Startup Detection:**
   - Scans gamepad slots 0-3
   - Assigns first found to Player 1
   - Assigns second found to Player 2

2. **Hot-Plug Detection:**
   - Continuously scans every frame
   - Auto-assigns new controllers to players without one
   - Preserves existing assignments

3. **Priority System:**
   - Controllers preferred over keyboard
   - Gamepad ID stored per player
   - Survives game restarts (until app closes)

## 🎯 Advanced Tips

### Using Mix of Controller + Keyboard:

- Player 1 can use controller
- Player 2 can use keyboard (or vice versa)
- Each input method works independently

### Testing Individual Controllers:

```bash
# Run the test utility
./controller_test

# Move each controller's left stick
# Press buttons on each controller
# Verify both are detected separately
```

### Multiple Gaming Sessions:

- Controllers stay paired to your Mac
- No need to re-pair each time
- Just turn on controllers (press PS button)
- Launch game

## 📝 Technical Details

### Gamepad API:

- Uses raylib's cross-platform gamepad API
- Supports up to 4 controllers (extensible to 4-player)
- 20% deadzone on analog sticks
- Button mapping compatible with PS5/PS4/Xbox controllers

### Controller IDs:

- macOS assigns IDs 0, 1, 2, 3 based on connection order
- Game maps these to Player 1, 2 automatically
- HUD shows actual gamepad ID number

### Input Latency:

- < 1 frame (16.67ms at 60 FPS)
- Polled every frame
- Direct input (no buffering)

## ✅ Verification Checklist

Before playing with 2 controllers:

- [ ] Both controllers paired in Bluetooth settings
- [ ] Both controllers showing "Connected"
- [ ] Both controllers charged (> 20% battery)
- [ ] Game launched: `./smash_bros`
- [ ] Console shows: "Gamepad 0 assigned to Player 1"
- [ ] Console shows: "Gamepad 1 assigned to Player 2"
- [ ] Player 1 HUD shows: 🎮0
- [ ] Player 2 HUD shows: 🎮1
- [ ] Test movement on both controllers
- [ ] Test attacks on both controllers

## 🎮 Ready to Play!

If you see **🎮0** and **🎮1** in the HUD, you're all set!

**Now you can:**

- Move both characters independently
- Attack each other
- Build up damage percentage
- Launch opponents off-stage
- Have an epic 2-player Smash battle!

---

**Need Help?**

- Run `./controller_test` to diagnose issues
- Check console output for detection logs
- Verify Bluetooth pairing in System Settings

**Enjoy the game!** 🎮✨🔥
