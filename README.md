# Material Baker

> [!NOTE]
> A Japanese version of the README is available in [`README.ja.md`](README.ja.md).
>
> 日本語版のREADMEは[`README.ja.md`](README.ja.md)にあります。

> [!NOTE]
> This is a pre-release version and is still under development.

A free, easy-to-use Unreal Engine plugin for baking procedural materials into textures. It's designed to be simple and efficient, making it perfect for solo developers and small teams.

## Features

*   **Bake Various Properties:** Bake a wide range of material properties:
    *   Final Color
    *   Base Color
    *   Normal
    *   Roughness
    *   Metallic
    *   Specular
    *   Opacity
    *   Emissive Color
*   **Flexible Output:** Save baked textures as **Texture Assets**, **PNG**, **JPEG**, **TGA**, or **EXR** files. Note that JPEG files will always be saved as 8-bit.
*   **Bit Depth Selection:** Choose between **8-bit** and **16-bit** output to fit your project's needs.
*   **Bake Queue:** Add multiple materials to a queue for batch baking.
*   **Update in Queue:** Select items in the queue to update their settings.
*   **Automatic Naming and Path:** Automatically suggests a texture name and output path based on the selected material. It also follows common naming conventions, such as automatically changing a material's `M_` or `MI_` prefix to `T_` for the texture.
*   **Automatic Suffix:** Automatically appends a relevant suffix to the texture name based on the selected property (e.g., `_N` for Normal, `_BC` for Base Color), and prevents duplicate suffixes. This feature can be disabled.

| Property | Suffix |
| --- | --- |
| Base Color | `_BC` |
| Normal | `_N` |
| Roughness | `_R` |
| Metallic | `_M` |
| Emissive Color | `_E` |
| Opacity | `_O` |
*   **Custom Texture Size:** Set the width and height of the output texture (up to 8192x8192).
*   **Compression Settings:** Choose the desired compression format for your texture assets.
*   **sRGB Toggle:** Enable or disable sRGB for color accuracy.
*   **Detailed Progress Display:** Shows detailed progress during the baking process.

## Target Audience

*   **Individuals and small developers** who find Substance Designer too complex or costly.
*   **Artists who want to quickly experiment** with creating procedural materials and textures directly within Unreal Engine 5.
*   **Long-time Unreal Engine users** (like me!) who have created numerous procedural materials over the years and need an easy way to bake them all.

## How to Use

### 1. Opening the Tool

1.  In the Unreal Engine editor, navigate to the main toolbar.
2.  Click on **Tools > Material Baker** to open the plugin window.

### 2. Configuring a Bake Job

The **Bake Settings** tab is where you define the parameters for each bake.

1.  **Property to Bake:** Select the material channel you want to export (e.g., Base Color, Normal, Roughness).
2.  **Bit Depth:** Choose between **8-bit** for standard textures and **16-bit** for high-quality textures with more color/data precision.
3.  **Target Material:** Use the dropdown to select the material you want to bake. The plugin will automatically suggest an output name and path.
4.  **Baked Texture Name:** Assign a name to your output texture.
5.  **Bake Texture Size:** Set the resolution (width and height) for the baked texture.
6.  **Compression Setting:** (For Texture Assets) Choose the compression method. `TC_Default` is suitable for most color textures, while `TC_Normalmap` is best for normal maps.
7.  **sRGB:** Enable this for color textures (Base Color, Final Color). Disable it for linear data maps (Normal, Roughness, Metallic, etc.) to ensure correct results.
8.  **Output Type:**
    *   **Texture Asset:** Creates a `UTexture` asset inside your project's content folder. This is the most common choice.
    *   **PNG, JPEG, TGA:** Exports the texture as an image file to a specified location on your computer.
9.  **Output Path:**
    *   For **Texture Assets**, this is a path within your project's `/Game/` directory (e.g., `/Game/Textures/MyBakes`).
    *   For image files, this is an absolute path on your system (e.g., `D:/MyProject/Exports`).

### 3. Managing the Bake Queue

The **Bake Queue** tab manages the list of jobs to be processed.

1.  **Add to Queue:** After configuring a job in the **Bake Settings** tab, click **Add to Queue**. The job will appear in the bake queue list.
2.  **Update Selected:** To modify a job that's already in the queue, select it from the list. The **Bake Settings** tab will populate with its data. Make your changes and click **Update Selected** to save them.
3.  **Remove Selected:** To delete a job from the queue, select it and click **Remove Selected**.

### 4. Baking

1.  Once your queue is ready, click the **Bake All** button.
2.  A progress bar will appear as the plugin processes each job.
3.  When complete, the baked textures will be available in the output path you specified for each job.

## Tips

### Baking Signed Distance Fields (SDF)

Signed Distance Fields (SDFs) often use negative values to represent the area inside a shape and positive values for the area outside. To correctly bake this data without losing the negative values (i.e., clamping them to 0), you need to use a high-precision, linear format.

The `Emissive Color` property is the best choice for this purpose, as its baking pipeline is set up to handle HDR (High Dynamic Range) values without applying tonemapping.

**Recommended Settings:**

*   **Material Setup:** Connect your SDF calculation result to the **Emissive Color** output pin in your material.
*   **Property:** `Emissive Color`
*   **Bit Depth:** `16-bit`
*   **Output Type:** `Texture Asset` (This ensures the data is saved in a floating-point format).
*   **sRGB:** `Disabled` (Unchecked). SDF data is linear and should not be gamma-corrected.

## Requirements

*   Unreal Engine 5.3 or later.

## Installation

1.  Download the latest release from the [Releases](https://github.com/embarrassingmoment/MatBaker/releases) page.
2.  Navigate to your Unreal Engine project's root directory.
3.  Create a `Plugins` folder if it doesn't already exist.
4.  Extract the downloaded archive into the `Plugins` folder.
5.  Restart your Unreal Engine editor. The plugin should now be available under **Tools > Material Baker**.

## How It Works

Material Baker uses two different methods to bake materials depending on the selected property:

*   **Direct Drawing (`Final Color`):** For the `Final Color` property, the plugin uses a highly efficient method (`DrawMaterialToRenderTarget`) that directly draws the material's final output to a texture. This captures the material exactly as it appears, including lighting and post-processing effects.
*   **Scene Capture (Other Properties):** For all other properties (e.g., `Base Color`, `Normal`, `Roughness`), the plugin uses a scene capture method. It temporarily spawns a flat plane mesh in the scene, applies the target material to it, and uses a `SceneCapture2D` component to capture the specific G-Buffer or material property. This allows for isolating specific data channels but comes with the limitation of being baked on a standardized shape.

## Limitations

*   **Baking on a Plane Mesh:** For all properties except `Final Color`, the baking process is performed on a standard, flat plane mesh. This means that complex material effects that depend on a specific mesh's UV layout or geometry may not be captured as expected. The tool is designed for baking procedural or tileable material definitions, not for transferring textures from one mesh to another (which is the purpose of texture re-projection tools).
*   **Opacity Output:** When baking the `Opacity` property, the resulting texture will be a grayscale image where the opacity value is copied into the R, G, B, and Alpha channels.

## Bug Reports & Feature Requests

If you encounter any bugs or have a feature request, please [open an issue](https://github.com/embarrassingmoment/MatBaker/issues) on our GitHub repository.

## Development

This plugin was developed with the assistance of the AI software engineer, Jules. This disclosure is intended to promote transparency in the use of AI in software development.

### Note for AI Agents

This repository contains an `AGENTS.md` file at the root, which provides development guidelines and project-specific instructions for AI software engineers. Please review it before making any changes. Currently, this file is available only in Japanese.

## License

This plugin is licensed under the [MIT License](LICENSE).
