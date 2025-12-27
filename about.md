# About ImageToBlocks

This mod was built because manually placing thousands of pixels for level art is a waste of a human life. 

### How it works:
When you pick an image,the mod looks at the pixels and places them where they need to. Instead of just placing one block per pixel, it checks the ones next to eachother. If they are the same color, it stretches one block to cover the whole area using Cocos2d-x scaling(so cpu no die). This "merging" is the difference between a level that runs at 144fps and a level that won't even open lol

It uses **stb_image** for the heavy lifting and Geode's UI wrappers for the settings menu. It’s built to be fast, safe, and actually usable.

### How to use 
- Import image, if the image looks weird do the following steps
1. click **EDIT** in the editor, and select a block.
2. click **HSV** the button, and set the 3rd row to black (move to bottom)
3. if it somehow doesnt work, message mc_adriannn on discord
