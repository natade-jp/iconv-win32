# iconv-win32

Prebuilt **libiconv for Windows (x86 / Win32)**.

This repository provides the DLL, import library, and header file required to use libiconv in Visual Studio C/C++ projects without building it yourself.

---

## 📦 Contents

```

dist/
├ bin/
│   └ iconv-2.dll
├ include/
│   └ iconv.h
├ lib/
│   └ iconv.lib
└ LICENSES/
└ libiconv-LICENSE.txt

```

---

## 🚀 How to Use (Visual Studio)

### 1. Include path

Add:

```

dist/include

```

to:

```

Project
→ Properties
→ C/C++
→ Additional Include Directories

```

---

### 2. Library path

Add:

```

dist/lib

```

to:

```

Linker
→ General
→ Additional Library Directories

```

---

### 3. Link library

Add:

```

iconv.lib

```

to:

```

Linker
→ Input
→ Additional Dependencies

```

---

### 4. Runtime

Place:

```

dist/bin/iconv-2.dll

````

next to your executable  
(or in a directory included in `PATH`).

---

## 💻 Example

```c
#include <iconv.h>
````

---

## 🏗 Build It Yourself

If you want to build libiconv manually:

```
build_libiconv_x86.bat
make_package.bat
```

This will:

1. Download vcpkg
2. Build libiconv (x86 / DLL)
3. Create the distribution layout

Note: vcpkg itself is **not included** in this repository.

---

## 📋 Requirements (for building)

* Visual Studio 2022
* Desktop development with C++
* Git

---

## ⚖ License

libiconv is licensed under the **GNU Lesser General Public License (LGPL)**.

The license text is included in:

```
dist/LICENSES/libiconv-LICENSE.txt
```

Source:
[https://ftp.gnu.org/pub/gnu/libiconv/](https://ftp.gnu.org/pub/gnu/libiconv/)

---

## 📌 Notes

* This package is built for **Win32 (x86)**.
* For x64 builds, libiconv must be rebuilt.
* This repository contains **prebuilt binaries only**, not the libiconv source code.

---

## 🙏 Acknowledgements

* GNU libiconv
* vcpkg
