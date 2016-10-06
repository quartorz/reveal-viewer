# reveal-viewer

## ビルド方法

1. Boost のパスを設定する

   - ヘッダファイルはおそらく CMake が自動で探してくれる
   - 変数 `BOOST_LIBRARYDIR` にライブラリのパスを設定する

1. [CEF](https://bitbucket.org/chromiumembedded/cef) のパスを設定する

   - `CEF_INCLUDE_PATH`、`CEF_LIBRARY_PATH`、`CEF_BINARY_PATH` の3つの変数を設定した場合

     - `${CEF_INCLUDE_PATH}` に CEF のヘッダファイルを配置する
     - `${CEF_LIBRARY_PATH}/Debug` に CEF のデバッグビルド用の `.lib` ファイルを配置する
     - `${CEF_LIBRARY_PATH}/Release` に CEF のリリース用の `.lib` ファイルを配置する
     - `${CEF_BINARY_PATH}/Debug` に CEF の Standard Distribution
        を展開すると得られる `Debug` フォルダと `Resource` フォルダの中身を配置する
     - `${CEF_BINARY_PATH}/Release` に CEF の Standard Distribution
        を展開すると得られる `Release` フォルダと `Resource` フォルダの中身を配置する

   - 変数 `CEF_PATH` にパスを設定した場合

     - `${CEF_PATH}/include` に上の `CEF_INCLUDE_PATH` と同じものを配置する
     - `${CEF_PATH}/lib` に上の `CEF_LIBRARY_PATH` と同じものを配置する
     - `${CEF_PATH}/bin` に上の `CEF_BINARY_PATH` と同じものを配置する

1. [quote](https://github.com/quartorz) のパスを設定する

   - 変数 `QUOTE_PATH` に [quote](https://github.com/quartorz) の `include` ディレクトリを設定する

1. CMake でプロジェクトを生成する

1. ビルドする