# ライセンスについて

本リポジトリは、Windows 向けにビルド済みの  
libiconv および nkf のバイナリを配布しています。

本リポジトリ自体は独自のソフトウェアを提供するものではなく、
各ソフトウェアのビルド済みバイナリのみを含みます。

---

## libiconv

- ライセンス: GNU Lesser General Public License (LGPL)
- 著作権: Free Software Foundation, Inc.

ライセンス全文は以下に含まれています。

```
dist/iconv/LICENSES/libiconv-LICENSE.txt
```

公式サイト:
https://ftp.gnu.org/pub/gnu/libiconv/

### LGPL について

libiconv は LGPL に基づいて配布されています。

本リポジトリでは以下を満たしています。

- LGPL ライセンス全文を同梱
- libiconv が LGPL であることを明示
- ユーザーが再ビルド可能であることを明確化

libiconv をリンクして再配布する場合は、
利用者自身の責任において LGPL の条件を満たす必要があります。

## nkf

- ライセンス: zlib License

ライセンス全文は以下に含まれています。

```
dist/nkf/LICENSES/nkf-LICENSE.txt
```

zlib ライセンスは、比較的緩やかな条件のオープンソースライセンスです。

## 本リポジトリについて

本リポジトリは：

- ビルド済みバイナリのみを提供します
- libiconv および nkf のソースコードは含みません
- Windows (x86 / x64) 向けにビルドされています
- libiconv は vcpkg を使用してビルドしています

再ビルドが必要な場合は、提供しているビルドスクリプトをご利用ください。

## 利用者の責任について

本リポジトリを利用してアプリケーションを配布する場合、
各ライセンス条件を理解し、遵守する責任は利用者にあります。

特に libiconv（LGPL）を動的リンクまたは再配布する場合は、
LGPL の条件を確認してください。
