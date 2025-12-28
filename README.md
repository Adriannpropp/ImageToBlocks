# ImageToBlocks

An optimized image importer for Geometry Dash that utilizes greedy meshing to minimize object count and maintain high performance.

## Features
* **Greedy Meshing**: Automatically merges adjacent pixels of similar colors into single, larger blocks using Cocos2d-x scaling to keep object counts low.
* **HSVA Overrides**: Uses absolute color data for 1:1 visual accuracy in the editor.
* **Asynchronous Processing**: Image decoding and block generation run on a background thread to prevent UI exploding
* **Smart Safety**: Automatically scales down high  resolution images to prevent ur editor exploding

## Installation
1. Install the [Geode Loader](https://geode-sdk.org/).
2. Search for **Image to Blocks** in the Geode in-game index.
3. Once installed, access the importer via the new **+** button in the editor's Undo/Redo menu.

## Quick Start Guide
To ensure the best visual results, follow the built-in tutorial or these steps:
1. Select the block type you wish to use for pixels.
2. In the color settings, set the **Base Color to BLACK** (or Neutral) to avoid unwanted tinting.
3. Adjust the **Tolerance** slider to control how strictly colors are merged.

## Support
For bug reports or technical help, please open an issue on the GitHub repository or contact `mc_adriannn` on Discord.
