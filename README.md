# Material Baker

> [!NOTE]
> This is a pre-release version and is still under development.

A free, easy-to-use Unreal Engine plugin for baking procedural materials into textures. It's designed to be simple and efficient, making it perfect for solo developers and small teams.

## Features

*   **Select any Material:** Choose any Material from your project to bake.
*   **Bake Queue:** Add multiple materials to a queue for batch baking.
*   **Custom Texture Size:** Set the width and height of the output texture.
*   **Compression Settings:** Choose the desired compression format for your texture.
*   **Custom Naming:** Give your baked texture a unique name.
*   **sRGB Toggle:** Enable or disable sRGB based on your needs.
*   **Custom Output Path:** Specify where to save the baked texture.
*   **Output Format Selection:** Choose the output file format (e.g., PNG, JPEG).

## How to Use

1.  Find the **Material Baker** button in the Unreal Engine toolbar (**Tools > Material Baker**) and click it to open the plugin window.
2.  Select the material you want to bake and configure the settings, such as texture size, name, and output format.
3.  Click the **Add to Queue** button to add the configured material to the bake queue.
4.  You can select an item in the queue to update its settings or click **Remove Selected** to remove it.
5.  Repeat the process to add more materials to the queue.
6.  Once you have added all the materials you want to bake, click the **Bake All** button.
7.  The plugin will process all materials in the queue. If the output type is set to **Texture Asset**, new textures will be created in the specified project path. If it's set to **PNG** or **JPEG**, you will be prompted to save each file on your computer.

## License

This plugin is free to use.

---

# マテリアルベイカー

> [!NOTE]
> このバージョンはプレリリース版であり、現在開発中です。

プロシージャルマテリアルをテクスチャにベイクするための、無料で使いやすいUnreal Engineプラグインです。個人開発者や小規模チームでの利用に最適化されており、シンプルで効率的なワークフローを提供します。

## 機能

*   **任意のマテリアル選択:** プロジェクト内の任意のマテリアルを選択してベイクできます。
*   **ベイクキュー:** 複数のマテリアルをキューに追加し、一括でベイク処理できます。
*   **カスタムテクスチャサイズ:** 出力するテクスチャの幅と高さを自由に設定できます。
*   **圧縮設定:** テクスチャに適した圧縮形式を選択できます。
*   **カスタム命名:** ベイクしたテクスチャに固有の名前を付けることができます。
*   **sRGB切り替え:** 必要に応じてsRGBの有効/無効を切り替えられます。
*   **カスタム出力パス:** ベイクしたテクスチャの保存先を指定できます。
*   **出力形式の選択:** 出力するファイル形式（例：PNG、JPEG）を選択できます。

## 使い方

1.  Unreal Engineのツールバーにある「Material Baker」ボタン（**Tools > Material Baker**）をクリックして、プラグインウィンドウを開きます。
2.  ベイクしたいマテリアルを選択し、テクスチャサイズ、名前、出力形式などの設定を行います。
3.  **「Add to Queue」** ボタンをクリックして、設定したマテリアルをベイクキューに追加します。
4.  キュー内のアイテムを選択して設定を更新したり、 **「Remove Selected」** をクリックして削除したりできます。
5.  このプロセスを繰り返して、さらにマテリアルをキューに追加します。
6.  ベイクしたいすべてのマテリアルを追加したら、 **「Bake All」** ボタンをクリックします。
7.  プラグインがキュー内のすべてのマテリアルを処理します。出力タイプが **「Texture Asset」** に設定されている場合、指定したプロジェクトパスに新しいテクスチャが作成されます。 **「PNG」** または **「JPEG」** に設定されている場合は、各ファイルをコンピューターに保存するよう求められます。

## ライセンス

このプラグインは無料で使用できます。
