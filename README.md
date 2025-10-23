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
*   **Flexible Output:** Save baked textures as **Texture Assets**, **PNG**, **JPEG**, or **TGA** files.
*   **Bit Depth Selection:** Choose between **8-bit** and **16-bit** output to fit your project's needs.
*   **Bake Queue:** Add multiple materials to a queue for batch baking.
*   **Update in Queue:** Select items in the queue to update their settings.
*   **Automatic Naming and Path:** Automatically suggests a texture name and output path based on the selected material.
*   **Custom Texture Size:** Set the width and height of the output texture (up to 8192x8192).
*   **Compression Settings:** Choose the desired compression format for your texture assets.
*   **sRGB Toggle:** Enable or disable sRGB for color accuracy.
*   **Detailed Progress Display:** Shows detailed progress during the baking process.

## How to Use

1.  Find the **Material Baker** button in the Unreal Engine toolbar (**Tools > Material Baker**) and click it to open the plugin window.
2.  Select the material you want to bake and configure the settings, such as texture size, name, and output format.
3.  Click the **Add to Queue** button to add the configured material to the bake queue.
4.  To modify an item in the queue, select it from the list. The settings panel will be populated with its data. After making changes, click the **Update Selected** button to apply them.
5.  To remove an item, select it and click the **Remove Selected** button.
6.  Repeat the process to add more materials to the queue.
7.  Once you have added all the materials you want to bake, click the **Bake All** button.
8.  The plugin will process all materials in the queue. Baked textures will be saved automatically to the specified path. **Texture Assets** are saved within the project's content folder, while **PNG**, **JPEG**, and **TGA** files are saved to the absolute path designated in the settings.

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

## Bug Reports & Feature Requests

If you encounter any bugs or have a feature request, please [open an issue](https://github.com/embarrassingmoment/MatBaker/issues) on our GitHub repository.

## Development

This plugin was developed with the assistance of the AI software engineer, Jules. This disclosure is intended to promote transparency in the use of AI in software development.

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
    *   Emissive Color (自己発光色)
*   **柔軟な出力形式:** ベイクしたテクスチャを **Texture Asset**, **PNG**, **JPEG**, **TGA** ファイルとして保存できます。
*   **ビット深度の選択:** プロジェクトのニーズに合わせて **8-bit** と **16-bit** の出力形式を選択できます。
*   **ベイクキュー:** 複数のマテリアルをキューに追加し、一括でベイク処理できます。
*   **キューの更新:** キュー内のアイテムを選択して、設定を更新できます。
*   **自動命名とパス提案:** 選択したマテリアルに基づいて、テクスチャ名と出力パスを自動的に提案します。
*   **カスタムテクスチャサイズ:** 出力するテクスチャの幅と高さを自由に設定できます（最大8192x8192）。
*   **圧縮設定:** テクスチャアセットに適した圧縮形式を選択できます。
*   **sRGB切り替え:** 色の正確性を保つためにsRGBの有効/無効を切り替えられます。
*   **詳細な進捗表示:** ベイク処理中に詳細な進捗状況を表示します。

## 使い方

1.  Unreal Engineのツールバーにある「Material Baker」ボタン（**Tools > Material Baker**）をクリックして、プラグインウィンドウを開きます。
2.  ベイクしたいマテリアルを選択し、テクスチャサイズ、名前、出力形式などの設定を行います。
3.  **「Add to Queue」** ボタンをクリックして、設定したマテリアルをベイクキューに追加します。
4.  キュー内のアイテムを編集するには、リストからそのアイテムを選択します。設定パネルにそのデータが読み込まれます。変更を加えた後、 **「Update Selected」** ボタンをクリックして変更を適用します。
5.  アイテムを削除するには、それを選択して **「Remove Selected」** ボタンをクリックします。
6.  このプロセスを繰り返して、さらにマテリアルをキューに追加します。
7.  ベイクしたいすべてのマテリアルを追加したら、 **「Bake All」** ボタンをクリックします。
8.  プラグインがキュー内のすべてのマテリアルを処理します。ベイクされたテクスチャは指定されたパスに自動的に保存されます。**Texture Asset** はプロジェクトのコンテンツフォルダ内に、**PNG**、**JPEG**、**TGA** ファイルは設定で指定された絶対パスに保存されます。

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

## バグ報告・機能要望

バグを発見した場合や機能の要望がある場合は、GitHubリポジトリの[イシュー](https://github.com/embarrassingmoment/MatBaker/issues)からご報告ください。

## 開発

このプラグインは、AIソフトウェアエンジニア「Jules」の支援を受けて開発されました。これは、ソフトウェア開発におけるAI利用の透明性を推進するための開示です。

## ライセンス

このプラグインは [MITライセンス](LICENSE) の下で提供されています。
