# Quick Setup Guide - Raylib Game Development with Python

## 🚀 5-Minute Setup

### Step 1: Configure AWS Bedrock Credentials

Edit the `.env` file:
```bash
# Open .env file
nano .env

# Or with your preferred editor
code .env
```

Add your AWS credentials:
```
AWS_ACCESS_KEY_ID=your_actual_access_key
AWS_SECRET_ACCESS_KEY=your_actual_secret_key
AWS_REGION=us-east-1
```

**Alternative:** Use AWS Profile
```
AWS_PROFILE=your_profile_name
```

### Step 2: Install Python Dependencies

Run the setup script:
```bash
./setup.sh
```

This installs:
- ✅ Python virtual environment
- ✅ raylib Python bindings (pyray)
- ✅ AWS Bedrock SDK (boto3)
- ✅ python-dotenv for .env support

### Step 3: Activate Virtual Environment

```bash
source venv/bin/activate
```

### Step 4: Test Python Examples

```bash
# Basic window
python examples/basic_window.py

# 2D player movement
python examples/player_movement_2d.py

# 3D scene
python examples/basic_3d.py

# Particle system
python examples/particle_system.py
```

### Step 5: Run OpenCode

```bash
# Make sure you're in the project root
bun dev
```

## 🎮 Start Creating Games!

Once OpenCode is running, try:
- "Create a pong game in Python"
- "Make a space shooter with raylib"
- "Build a 3D maze game"

## 📦 What's Installed

### Python Packages:
- `raylib==5.0.0.3` - Python bindings for raylib
- `boto3>=1.34.0` - AWS SDK for Bedrock
- `python-dotenv>=1.0.0` - Load .env variables
- `awscli>=1.32.0` - AWS command line tools

### Directory Structure:
```
venv/                    # Python virtual environment
examples/                # Game examples (Python & C)
.env                     # Your AWS credentials (git ignored)
.opencode/command/       # Game templates
```

## 🔧 Troubleshooting

### AWS Credentials Not Working?
```bash
# Test AWS credentials
python -c "import boto3; print(boto3.client('bedrock-runtime', region_name='us-east-1'))"
```

### Python Module Not Found?
```bash
# Make sure virtual environment is activated
source venv/bin/activate

# Reinstall dependencies
pip install -r requirements.txt
```

### Raylib Not Working?
```bash
# Test raylib installation
python -c "from pyray import *; print('raylib working!')"
```

## 📝 Environment Variables Reference

### Required:
- `AWS_ACCESS_KEY_ID` - Your AWS access key
- `AWS_SECRET_ACCESS_KEY` - Your AWS secret key
- `AWS_REGION` - AWS region (default: us-east-1)

### Optional:
- `AWS_PROFILE` - Use AWS CLI profile instead
- `AWS_SESSION_TOKEN` - For temporary credentials

## 🎯 Next Steps

1. ✅ Set up AWS credentials in `.env`
2. ✅ Run `./setup.sh`
3. ✅ Activate virtual environment
4. ✅ Test examples
5. ✅ Run `bun dev`
6. 🎮 Start making games!

---

**Need help?** See `README_RAYLIB.md` for full documentation.
