# Changelog

All notable changes to this project will be documented in this file.

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
