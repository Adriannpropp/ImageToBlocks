# Manual Build Guide

If the automated scripts aren't working, follow these steps manually:

## Step 1: Open the Right Terminal

You MUST use the correct Visual Studio terminal:

1. Press Windows key
2. Search for "x64 Native Tools Command Prompt for VS 2022"
3. Run as Administrator
4. Navigate to your mod folder:
   ```cmd
   cd C:\path\to\your\mod
   ```

## Step 2: Set Environment Variables

```cmd
set GEODE_SDK=C:\GeodeSDK
set PATH=C:\geode-sdk;%PATH%
```

## Step 3: Verify Everything

```cmd
REM Check Geode
C:\geode-sdk\geode.exe --version

REM Check CMake
cmake --version

REM Check compiler
cl
```

All three should work. If any fail, fix them before continuing.

## Step 4: Generate Build Files

```cmd
mkdir build
cd build
cmake .. -A win32 -T host=x64 -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

**Common issues:**
- "CMake Error: GEODE_SDK not set" → Go back to Step 2
- "Cannot find vcvarsall.bat" → Use the x64 Native Tools Command Prompt
- "Generator mismatch" → Delete the build folder and try again

## Step 5: Build the Mod

```cmd
cmake --build . --config RelWithDebInfo
```

This will take a minute. Watch for errors.

## Step 6: Find Your Mod

If successful, your `.geode` file will be at:
```
build\RelWithDebInfo\your.username.imageimporter.geode
```

## Step 7: Install

### Method A: Manual Copy
```cmd
copy /Y build\RelWithDebInfo\*.geode "%LocalAppData%\GeometryDash\geode\mods\"
```

### Method B: Geode CLI
```cmd
C:\geode-sdk\geode.exe mod install build\RelWithDebInfo\your.username.imageimporter.geode
```

## Step 8: Test in Game

1. Launch Geometry Dash
2. Check that Geode loads (you should see the Geode menu)
3. Go to Geode mods list and verify your mod is there
4. Open the editor and press ESC
5. Look for the "Import Image" button

## Troubleshooting Specific Errors

### "fatal error LNK1181: cannot open input file 'geode.lib'"

This means CMake didn't find the Geode SDK properly.

**Fix:**
```cmd
cd ..
rmdir /s /q build
set GEODE_SDK=C:\GeodeSDK
mkdir build
cd build
cmake .. -A win32 -T host=x64
```

### "LINK : fatal error LNK1104: cannot open file 'LIBCMT.lib'"

You're missing Windows SDK components.

**Fix:**
1. Open Visual Studio Installer
2. Modify your installation
3. Check "Windows 10 SDK" under Individual Components
4. Install and restart

### The build succeeds but no .geode file appears

Check if you're building for the wrong configuration:

```cmd
cmake --build . --config RelWithDebInfo
dir /s *.geode
```

The file should be in `build\RelWithDebInfo\`

### "Error: Cannot find module 'geode'"

Your mod.json might be wrong. Check:
```json
{
    "geode": "3.0.0",
    "gd": {
        "win": "2.206"
    }
}
```

### Mod loads but button doesn't appear

The menu hook might be failing. Check logs:
```cmd
type "%LocalAppData%\GeometryDash\geode\logs\latest.log" | findstr imageimporter
```

## Alternative: Use Geode CLI Build

If CMake is giving you trouble, try the Geode CLI directly:

```cmd
C:\geode-sdk\geode.exe build
```

This should handle everything automatically.

## Last Resort: Build from Scratch

1. Delete everything except `main.cpp`
2. Run:
   ```cmd
   C:\geode-sdk\geode.exe new
   ```
3. Follow the prompts
4. Replace the generated `main.cpp` with yours
5. Run:
   ```cmd
   C:\geode-sdk\geode.exe build
   ```

## Need More Help?

- Check Geode docs: https://docs.geode-sdk.org/
- Join Geode Discord: https://discord.gg/geode-sdk
- Look at example mods: https://github.com/geode-sdk/examples
- Check your paths are EXACTLY correct (case-sensitive in some cases)
