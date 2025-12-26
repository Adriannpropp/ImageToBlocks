# Image to Blocks - Geode Mod

Import images as colored blocks in the Geometry Dash editor.

## Setup

### Prerequisites
- Geode CLI installed at `C:\geode-sdk\geode.exe`
- Geode SDK at `C:\GeodeSDK`
- CMake (comes with Geode CLI)
- Visual Studio 2022 with C++ desktop development tools

### Environment Setup

1. **Set the GEODE_SDK environment variable:**
   - Open Command Prompt as Administrator
   - Run: `setx GEODE_SDK "C:\GeodeSDK" /M`
   - Restart your command prompt

2. **Verify your setup:**
   ```cmd
   C:\geode-sdk\geode.exe --version
   ```

## Building

### Option 1: Using the build script (easiest)
```cmd
build.bat
```

### Option 2: Manual build
```cmd
set GEODE_SDK=C:\GeodeSDK
mkdir build
cd build
cmake .. -A win32 -T host=x64
cmake --build . --config RelWithDebInfo
```

### Option 3: Using Geode CLI
```cmd
C:\geode-sdk\geode.exe build
```

## Installation

After building, your mod will be at:
- `build/RelWithDebInfo/your.username.imageimporter.geode`

Copy this `.geode` file to:
- `%LocalAppData%\GeometryDash\geode\mods\`

Or install via Geode CLI:
```cmd
C:\geode-sdk\geode.exe mod install build/RelWithDebInfo/your.username.imageimporter.geode
```

## Usage

1. Open the Geometry Dash editor
2. Pause the editor (ESC)
3. Click the "Import Image" button
4. Select an image file (PNG, JPG, BMP)
5. Wait for the import to complete

**Note:** Large images (>200x200 pixels) will create many objects and may cause lag!

## Troubleshooting

### CMake can't find Geode SDK
- Make sure `GEODE_SDK` environment variable is set correctly
- Restart your terminal after setting environment variables

### Build fails with "cannot find vcvars"
- Install Visual Studio 2022 with "Desktop development with C++"
- Make sure you're using an x64 Native Tools Command Prompt

### Mod doesn't load in game
- Check Geode logs in `%LocalAppData%\GeometryDash\geode\logs\`
- Make sure you're running the correct GD version (2.206)

### Objects don't have color
- This is a known limitation - GD's color system requires custom color channels
- Current version creates geometry only

## Common Errors Fixed

- ✅ Fixed memory leaks with CCImage
- ✅ Added proper error handling for file loading
- ✅ Fixed Y-axis flipping for correct image orientation
- ✅ Added safety checks for large images
- ✅ Improved object creation with null checks
- ✅ Added proper state management to prevent multiple simultaneous imports

## To-Do / Improvements

- [ ] Add color channel support for proper block coloring
- [ ] Add image downsampling for large images
- [ ] Add progress bar for large imports
- [ ] Add option to customize block size/type
- [ ] Add undo/redo support
- [ ] Optimize batch object creation
"# ImageToBlocks" 
