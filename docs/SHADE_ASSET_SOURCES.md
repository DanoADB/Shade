# Asset sources for Shade

The openblack engine is GPL-3.0; its engine license does not supply the original Black & White game data. See https://github.com/openblack/openblack#extracting-asset-files-from-original-game.

Inspected sources:
- This checkout: engine shaders and application icons; no standalone creature or village art.
- https://github.com/openblack/openblack-assets: GPL-3.0 LICENSE, construct.lnd landscape and construct.txt playground script. The script references original game model identifiers. These are not drop-in browser models.

Original disk files can be inspected locally after the user provides a folder. Keep raw copies outside web/dist and outside Git. The static server exposes everything in web/dist. No proprietary files have been added to the served bundle.

Next import work: inventory the provided files, distinguish installation archives from extracted Data/Audio/Scripts, identify one model plus its textures, then implement or adapt a format loader for a local browser preview. The current canvas prototype and C++ procedural probe do not load original formats.
