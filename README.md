# 🖼️ Image to Blocks - Geode Mod

An advanced image importer for the **Geometry Dash 2.2** editor. Transform any PNG or JPG into high-fidelity pixel art using native game objects.

![Import Example](https://github.com/Adriannpropp/ImageToBlocks/raw/main/cat.png)

## 🚀 Features

* **Pixel-Perfect Accuracy**: Uses Object ID `1520` (2.2 Colorable Pixel), the most optimized object for high-detail art.
* **HSV Absolute Overrides**: Bypasses the limited Color Channel system. Colors are saved directly to the object using HSVA data, ensuring your art stays colored after saving and reloading.
* **In-Editor UI**: Adds a dedicated **Green Plus (+)** button to the Editor toolbar for seamless workflow.
* **Built-in Logger**: Real-time import reports showing image dimensions, pixel data, and placement counts.
* **Performance Controls**: Adjustable `Step` (pixel skipping) and `Scale` settings to manage object counts and prevent lag.

## 🛠️ Installation

1.  Download the latest `.geode` file from the [Releases](https://github.com/Adriannpropp/ImageToBlocks/releases) page.
2.  Drop it into your Geometry Dash `geode/mods` folder.
3.  Restart the game.

## 📖 How to Use

1.  Open the **Level Editor**.
2.  Locate the **Green Plus (+)** button on the top-right toolbar.
3.  Select your image file.
4.  **Configure Settings**:
    * **Step**: Skip pixels to save on object count (Recommended: 2 or 3).
    * **Scale**: Change the size of individual pixels (Recommended: 0.1).
5.  Click **Import**.
6.  Check the **Import Log** popup for details once the process completes.

## 🏗️ Technical Specifications

This mod is built using the **Geode SDK**. 
* **Decoding**: Powered by `stb_image` for fast, lightweight image processing.
* **Rendering**: Implements `m_hsv` absolute overrides on `GJSpriteColor` to force custom RGB values onto native objects.
* **UI**: Built with custom `Popup` and `TextInput` nodes for a native look and feel.

## 💻 Building from Source

```bash
git clone [https://github.com/Adriannpropp/ImageToBlocks.git](https://github.com/Adriannpropp/ImageToBlocks.git)
cd ImageToBlocks
cmake -B build -A win32
cmake --build build --config Release
Developed by Adriann. Made for creators who care about detail.
