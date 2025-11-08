# Material Baker

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
*   **Flexible Output:** Save baked textures as **Texture Assets**, **PNG**, **JPEG**, or **TGA** files. Note that JPEG files will always be saved as 8-bit.
*   **Bit Depth Selection:** Choose between **8-bit** and **16-bit** output to fit your project's needs.
*   **Bake Queue:** Add multiple materials to a queue for batch baking.
*   **Update in Queue:** Select items in the queue to update their settings.
*   **Automatic Naming and Path:** Automatically suggests a texture name and output path based on the selected material.
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

---

# マテリアルベイカー

> [!NOTE]
> このバージョンはプレリリース版であり、現在開発中です。

プロシージャルマテリアルをテクスチャにベイクするための、無料で使いやすいUnreal Engineプラグインです。個人開発者や小規模チームでの利用に最適化されており、シンプルで効率的なワークフローを提供します。

## 機能

*   **多様なプロパティのベイク:** 幅広いマテリアルプロパティをベイクできます:
    *   Final Color (最終的な色)
    *   Base Color (ベースカラー)
    *   Normal (法線)
    *   Roughness (ラフネス)
    *   Metallic (メタリック)
    *   Specular (スペキュラ)
    *   Opacity (オパシティ)
    *   Emissive Color (エミッシブカラー)
*   **柔軟な出力形式:** ベイクしたテクスチャを **Texture Asset**, **PNG**, **JPEG**, **TGA** ファイルとして保存できます。なお、JPEGファイルは常に8-bitで保存されます。
*   **ビット深度の選択:** プロジェクトのニーズに合わせて **8-bit** と **16-bit** の出力形式を選択できます。
*   **ベイクキュー:** 複数のマテリアルをキューに追加し、一括でベイク処理できます。
*   **キューの更新:** キュー内のアイテムを選択して、設定を更新できます。
*   **自動命名とパス提案:** 選択したマテリアルに基づいて、テクスチャ名と出力パスを自動的に提案します。
*   **自動サフィックス:** 選択したプロパティに応じて、テクスチャ名に関連するサフィックス（例: Normalの場合は `_N`、Base Colorの場合は `_BC`）を自動的に追加し、重複したサフィックスの追加を防ぎます。この機能は無効化できます。

| プロパティ | サフィックス |
| --- | --- |
| Base Color | `_BC` |
| Normal | `_N` |
| Roughness | `_R` |
| Metallic | `_M` |
| Emissive Color | `_E` |
| Opacity | `_O` |
*   **カスタムテクスチャサイズ:** 出力するテクスチャの幅と高さを自由に設定できます（最大8192x8192）。
*   **圧縮設定:** テクスチャアセットに適した圧縮形式を選択できます。
*   **sRGB切り替え:** 色の正確性を保つためにsRGBの有効/無効を切り替えられます。
*   **詳細な進捗表示:** ベイク処理中に詳細な進捗状況を表示します。

## 使い方

### 1. ツールの起動

1.  Unreal Engineエディタのメインツールバーに移動します。
2.  **「Tools > Material Baker」** をクリックして、プラグインウィンドウを開きます。

### 2. ベイク設定

**「Bake Settings」** タブで、各ベイクジョブのパラメータを定義します。

1.  **Property to Bake (ベイクするプロパティ):** エクスポートしたいマテリアルチャンネル（例: Base Color, Normal, Roughness）を選択します。
2.  **Bit Depth (ビット深度):** 標準的なテクスチャには **8-bit** を、より高い色/データ精度を持つ高品質なテクスチャには **16-bit** を選択します。
3.  **Target Material (対象マテリアル):** ドロップダウンからベイクしたいマテリアルを選択します。選択すると、出力名とパスが自動的に提案されます。
4.  **Baked Texture Name (ベイク後のテクスチャ名):** 出力テクスチャの名前を割り当てます。
5.  **Bake Texture Size (ベイクテクスチャのサイズ):** ベイクするテクスチャの解像度（幅と高さ）を設定します。
6.  **Compression Setting (圧縮設定):** (テクスチャアセット用) 圧縮形式を選択します。`TC_Default` はほとんどのカラーテクスチャに適しており、`TC_Normalmap` は法線マップに最適です。
7.  **sRGB:** カラーテクスチャ（Base Color, Final Color）では有効にします。法線、ラフネス、メタリックなどのリニアデータマップでは、正確な結果を得るために無効にします。
8.  **Output Type (出力タイプ):**
    *   **Texture Asset:** プロジェクトのコンテンツフォルダ内に `UTexture` アセットを作成します。これが最も一般的な選択肢です。
    *   **PNG, JPEG, TGA:** テクスチャを画像ファイルとして、PC上の指定した場所にエクスポートします。
9.  **Output Path (出力パス):**
    *   **Texture Asset** の場合、プロジェクトの `/Game/` ディレクトリ内のパス（例: `/Game/Textures/MyBakes`）を指定します。
    *   画像ファイルの場合、システム上の絶対パス（例: `D:/MyProject/Exports`）を指定します。

### 3. ベイクキューの管理

**「Bake Queue」** タブで、処理するジョブのリストを管理します。

1.  **Add to Queue (キューに追加):** **「Bake Settings」** タブでジョブを設定した後、**「Add to Queue」** をクリックします。ジョブがベイクキューのリストに表示されます。
2.  **Update Selected (選択項目を更新):** キューに既にあるジョブを修正するには、リストからそのジョブを選択します。**「Bake Settings」** タブにそのデータが読み込まれるので、変更を加えてから **「Update Selected」** をクリックして保存します。
3.  **Remove Selected (選択項目を削除):** キューからジョブを削除するには、そのジョブを選択して **「Remove Selected」** をクリックします。

### 4. ベイクの実行

1.  キューの準備ができたら、**「Bake All」** ボタンをクリックします。
2.  プラグインが各ジョブを処理する間、プログレスバーが表示されます。
3.  完了すると、ベイクされたテクスチャは各ジョブで指定した出力パスで利用可能になります。

## Tips

### SDF (Signed Distance Field) のベイク

SDF (Signed Distance Field) は、形状の内側を負の値、外側を正の値で表現することがよくあります。このデータを正しくベイクし、負の値を失わない（0にクランプされない）ようにするためには、高精度かつリニアなフォーマットを使用する必要があります。

`Emissive Color` プロパティは、トーンマッピングを適用せずにHDR（ハイダイナミックレンジ）の値を処理するように設計されているため、この目的に最適です。

**推奨設定:**

*   **マテリアル設定:** マテリアル内で、SDFの計算結果を **Emissive Color** の出力ピンに接続します。
*   **Property:** `Emissive Color`
*   **Bit Depth:** `16-bit`
*   **Output Type:** `Texture Asset` （浮動小数点フォーマットでデータが保存されることを保証します）。
*   **sRGB:** `無効`（チェックを外す）。SDFデータはリニアであり、ガンマ補正を適用するべきではありません。

## 要件

*   Unreal Engine 5.3 以降

## インストール

1.  [リリースページ](https://github.com/embarrassingmoment/MatBaker/releases)から最新のリリースをダウンロードします。
2.  お使いのUnreal Engineプロジェクトのルートディレクトリに移動します。
3.  `Plugins` フォルダが存在しない場合は作成します。
4.  ダウンロードしたアーカイブを `Plugins` フォルダに展開します。
5.  Unreal Engineエディタを再起動します。プラグインが **[Tools] > [Material Baker]** から利用できるようになっているはずです。

## 仕組み

Material Bakerは、ベイク対象のプロパティに応じて2つの異なる方式を使い分けています。

*   **直接描画 (`Final Color`):** `Final Color`プロパティは、`DrawMaterialToRenderTarget`という非常に効率的な方法を使い、マテリアルの最終的な出力を直接テクスチャに描き込みます。これにより、ライティングやポストプロセスエフェクトを含んだ、見た目通りの結果をキャプチャします。
*   **シーンキャプチャ (その他のプロパティ):** `Base Color`や`Normal`など、`Final Color`以外のすべてのプロパティでは、シーンキャプチャ方式が使用されます。この方式では、シーン内に一時的に平らな平面メッシュを配置し、そこに対象のマテリアルを適用した上で、`SceneCapture2D`コンポーネントを用いて特定のG-Bufferやマテリアルプロパティを撮影します。これにより、特定のデータチャンネルを分離して抽出できますが、標準化された形状の上でベイクされるという制限が伴います。

## 制限事項

*   **平面メッシュ上でのベイク:** `Final Color`を除くすべてのプロパティでは、ベイク処理は標準の平らな平面メッシュ上で行われます。これは、特定のメッシュのUVレイアウトやジオメトリに依存する複雑なマテリアルエフェクトが、期待通りにキャプチャされない可能性があることを意味します。このツールは、プロシージャルまたはタイリング可能なマテリアル定義をベイクするために設計されており、あるメッシュから別のメッシュへテクスチャを転写する（テクスチャリプロジェクションツールのような）目的には適していません。
*   **Opacityの出力:** `Opacity`プロパティをベイクすると、生成されるテクスチャは、不透明度の値がR, G, B, Alphaの各チャンネルにコピーされたグレースケール画像になります。

## バグ報告・機能要望

バグを発見した場合や機能の要望がある場合は、GitHubリポジトリの[イシュー](https://github.com/embarrassingmoment/MatBaker/issues)からご報告ください。

## 開発

このプラグインは、AIソフトウェアエンジニア「Jules」の支援を受けて開発されました。これは、ソフトウェア開発におけるAI利用の透明性を推進するための開示です。

### AIエージェント向けの注記

このリポジトリのルートには、AIソフトウェアエンジニア向けの開発ガイドラインやプロジェクト固有の指示を記載した `AGENTS.md` ファイルが含まれています。何らかの変更を加える前には、このファイルを確認してください。現在、このファイルは日本語のみで提供されています。

## ライセンス

このプラグインは [MITライセンス](LICENSE) の下で提供されています。
