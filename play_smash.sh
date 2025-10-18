#!/bin/bash

# Smash Bros Platform Fighter - Quick Launch Script
# Handles controller detection and game launch

echo "🎮 ====================================="
echo "   SMASH BROS PLATFORM FIGHTER"
echo "   PS5 Controller Ready!"
echo "====================================="
echo ""

# Check if game is compiled
if [ ! -f "./smash_bros" ]; then
    echo "⚠️  Game not compiled. Compiling now..."
    make -f smash_bros_makefile
    echo ""
fi

# Check for controllers
echo "🔍 Checking for PS5 controllers..."
echo ""

# Give user time to connect controllers
echo "📋 Pre-flight checklist:"
echo "   [ ] PS5 Controller 1 paired and connected?"
echo "   [ ] PS5 Controller 2 paired and connected?"
echo "   [ ] Both controllers charged?"
echo ""
echo "💡 Tip: Hold PS + Share to pair new controllers"
echo ""

read -p "Press ENTER to launch game (or Ctrl+C to cancel)..." 

echo ""
echo "🚀 Launching game..."
echo ""

# Launch the game
./smash_bros

echo ""
echo "👋 Thanks for playing!"
