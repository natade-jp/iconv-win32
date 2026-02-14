# windows-encoding-binaries

Windows 向けの文字コード変換ライブラリ／ツールの
ビルド済みバイナリ配布リポジトリです。

本リポジトリでは以下を提供します：

- **libiconv（DLL + import library + header）**
- **nkf（DLL + import library + header）**

Visual Studio C/C++ プロジェクトで
すぐに利用できる事前ビルド済みバイナリを提供します。

## 📦 内容

```
dist/
├─ iconv/
│  ├─ x86/
│  │  ├─ bin/iconv-2.dll
│  │  └─ lib/iconv.lib
│  ├─ x64/
│  │  ├─ bin/iconv-2.dll
│  │  └─ lib/iconv.lib
│  ├─ include/iconv.h
│  └─ LICENSES/libiconv-LICENSE.txt
│
└─ nkf/
   ├─ x86/
   │  ├─ bin/nkf32.dll
   │  └─ lib/nkf32.lib
   ├─ x64/
   │  ├─ bin/nkf32.dll
   │  └─ lib/nkf32.lib
   ├─ include/
   │  ├─ nkf32.h
   │  └─ nkf.h
   └─ LICENSES/
      └─ zlib-LICENSE.txt
```

## 🎯 目的

Windows 環境で

- 文字コード変換処理を組み込みたい
- Visual Studio で簡単に利用したい
- 自分でビルドする手間を省きたい

といった用途向けのバイナリ配布です。

※ ソースコードは含みません。

## 🚀 使用方法（概要）

### libiconv

1. `dist/iconv/include` をインクルードパスに追加
2. 対応アーキテクチャ（x86 / x64）の `lib` をライブラリパスに追加
3. `iconv.lib` をリンク設定に追加
4. 実行時に `iconv-2.dll` を配置

### nkf

1. `dist/nkf/include` をインクルードパスに追加
2. 必要に応じて `nkf32.lib` をリンク
3. 実行時に `nkf32.dll` を配置

## 🏗 ビルド方法

以下のスクリプトを用意しています。

```
build_iconv.bat
make_dist_iconv.bat

build_nkf.bat
make_dist_nkf.bat
```

これにより：

1. ソース取得
2. x86 / x64 ビルド
3. dist レイアウト生成

を行います。

※ vcpkg 本体はリポジトリに含みません。

## 📋 ビルド要件

- Visual Studio 2022
- Desktop development with C++
- Git

## 📌 注意事項

- 本リポジトリはビルド済みバイナリのみを含みます
- Windows 専用です
- x86 / x64 両対応です
- 各ライセンス条件に従ってご利用ください
