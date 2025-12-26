# Common Errors & Solutions

## Build Errors

### Error: "GEODE_SDK environment variable not set"
**Solution:**
```cmd
setx GEODE_SDK "C:\GeodeSDK" /M
```
Then restart your command prompt.

### Error: "CMake not found"
**Solution:**
The Geode CLI includes CMake. Add it to your PATH:
```cmd
set PATH=C:\geode-sdk;%PATH%
```

### Error: "Cannot find vcvarsall.bat"
**Solution:**
- Install Visual Studio 2022 with C++ desktop development
- Use "x64 Native Tools Command Prompt for VS 2022"
- Or manually run: `"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"`

### Error: "Geode not found"
**Solution:**
Check that your `CMakeLists.txt` has the correct SDK path:
```cmake
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
```

### Error: "LNK1181: cannot open input file"
**Solution:**
- Make sure you're building for Win32 (x86), not x64
- Use: `cmake .. -A win32 -T host=x64`

### Error: "undefined reference to geode::Mod::get()"
**Solution:**
- This means Geode isn't linking properly
- Ensure `setup_geode_mod(${PROJECT_NAME})` is in your CMakeLists.txt
- Try cleaning your build folder: `rmdir /s /q build`

## Runtime Errors

### Mod doesn't appear in game
**Solution:**
1. Check the mod is in: `%LocalAppData%\GeometryDash\geode\mods\`
2. Check Geode logs: `%LocalAppData%\GeometryDash\geode\logs\`
3. Ensure mod.json has correct GD version (2.206)

### Button doesn't appear in editor
**Solution:**
- The menu ID might be wrong for your GD version
- Try removing the menu->setID line and creating a standalone menu

### Game crashes on import
**Solution:**
- Image might be too large (try <100x100 pixels first)
- Image format might not be supported (stick to PNG)
- Check logs for specific error messages

### Objects don't appear
**Solution:**
- The objects might be created outside the visible area
- Try placing them at `{0, 0}` first for testing
- Check that `editor->m_objectLayer` exists

### Colors are wrong/missing
**Solution:**
- GD's color system requires custom color channels
- Current implementation has limited color support
- This is a known limitation that requires more advanced color channel management

## Code-Specific Errors

### "CCDirector::get() returns null"
**Fix:**
```cpp
// Old:
CCDirector::get()

// New:
CCDirector::sharedDirector()
```

### "m_objects is null"
**Fix:**
Add null check:
```cpp
if (editor->m_objects) {
    editor->m_objects->addObject(obj);
}
```

### "Memory leak detected"
**Fix:**
Always delete CCImage after use:
```cpp
CCImage* img = new CCImage();
// ... use img ...
delete img; // Don't forget this!
```

### "Access violation reading location"
**Fix:**
Add bounds checking:
```cpp
if (data && pixelIndex + 3 < width * height * 4) {
    unsigned char r = data[pixelIndex];
    // ...
}
```

## Tips for Debugging

1. **Enable verbose logging:**
```cpp
log::info("Debug checkpoint 1");
log::error("Error: {}", errorMessage);
```

2. **Check Geode logs:**
```cmd
type "%LocalAppData%\GeometryDash\geode\logs\latest.log"
```

3. **Use MessageBox for quick debugging:**
```cpp
FLAlertLayer::create("Debug", 
    fmt::format("Value: {}", myVariable), 
    "OK")->show();
```

4. **Test with small images first:**
- Start with 10x10 pixel images
- Gradually increase size once working

5. **Use try-catch for Geode operations:**
```cpp
try {
    auto result = file::pickFile(/*...*/);
    if (result.isErr()) {
        // handle error
    }
} catch (...) {
    log::error("Exception caught!");
}
```

## Still Having Issues?

1. Check the Geode Discord for help
2. Look at other mods' source code on GitHub
3. Read Geode documentation: https://docs.geode-sdk.org/
4. Check that all paths are correct (SDK, CLI, mod folder)
5. Try the example mods in the Geode SDK to verify your setup works
