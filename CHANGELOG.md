# Changelog

All notable changes to this project will be documented in this file.

## Unreleased

### Fixed

*   **Capture Framing:** Property bakes (Base Color, Normal, Emissive Color) now fit the capture to the actual plane mesh bounds instead of a hard-coded width, so the material fills the whole texture. Non-square texture sizes stretch the material to fill the output, matching the Final Color path.
*   **TGA Export:** TGA output now always exports 8-bit data and the UI locks the bit depth accordingly. The format has no 16-bit-per-channel mode, so 16-bit TGA exports previously failed.
*   **Texture Asset Lifetime:** Baked texture assets are no longer added to the root set, so they can be garbage collected and deleted normally without restarting the editor.
*   **Output Path Validation:** Texture Asset output now requires a valid content path (e.g. `/Game/Textures`). The Browse button only converts folders under a mounted content root to a package path; other folders are kept as file system paths for image export.

### Changed

*   **Opacity Hidden:** The `Opacity` property is hidden from the UI because its capture path does not produce the material's opacity.

## v1.0.0-pre (Pre-release)

### Initial Release

*   **Bake Various Properties:** Added support for baking multiple material properties including Final Color, Base Color, Normal, Emissive Color, and Opacity.
*   **Flexible Output Formats:** Supported exporting as Texture Assets (`.uasset`), PNG images (`.png`), and EXR images (`.exr`).
*   **Bit Depth Selection:** Implemented options for 8-bit and 16-bit output depths to suit different precision needs.
*   **Bake Queue:** Introduced a queue system to batch process multiple materials efficiently.
*   **Smart Naming:** Added automatic suggestion of output names and paths, including handling of `M_`/`MI_` to `T_` prefix conversion.
*   **Automatic Suffixes:** Implemented automatic appending of suffixes (e.g., `_BC`, `_N`) based on the selected property to prevent naming conflicts.
*   **Custom Resolution:** Enabled setting custom output texture resolutions up to 8192x8192.
*   **Compression Settings:** Added support for selecting compression settings (e.g., Default, Normalmap) for Texture Assets.

### Fixed

*   **UI Bug Fixes:** Fixed issues where UI state (e.g., Bit Depth, sRGB) did not update correctly when changing Output Type, and Bake Queue selection did not visually update dropdowns. Added missing JPEG output option.
