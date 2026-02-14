# iconv-win

Prebuilt **libiconv for Windows (x86 / x64)**.

This repository provides ready-to-use DLLs, import libraries, and headers for Visual Studio C/C++ projects — no manual build required.

---

## 📦 Contents

```

dist/
├─ x86/
│  ├─ bin/
│  │  └─ iconv-2.dll
│  └─ lib/
│     └─ iconv.lib
├─ x64/
│  ├─ bin/
│  │  └─ iconv-2.dll
│  └─ lib/
│     └─ iconv.lib
├─ include/
│  └─ iconv.h
└─ LICENSES/
└─ libiconv-LICENSE.txt

```

---

## 🚀 How to Use (Visual Studio)

Choose the architecture that matches your project.

### 1. Include path

```

dist/include

```

Project  
→ Properties  
→ C/C++  
→ Additional Include Directories

---

### 2. Library path

#### Win32 (x86)

```

dist/x86/lib

```

#### x64

```

dist/x64/lib

```

Linker  
→ General  
→ Additional Library Directories

---

### 3. Link library

```

iconv.lib

```

Linker  
→ Input  
→ Additional Dependencies

---

### 4. Runtime

Place the appropriate DLL next to your executable:

#### Win32 (x86)

```

dist/x86/bin/iconv-2.dll

```

#### x64

```

dist/x64/bin/iconv-2.dll

````

(or add the directory to your `PATH`)

---

## 💻 Example

```c
#include <iconv.h>
````

---

## 🏗 Build It Yourself

```
build_libiconv.bat
make_package.bat
```

This will:

1. Download vcpkg
2. Build libiconv (x86 / x64 DLL)
3. Create the distribution layout

Note: **vcpkg itself is not included in this repository.**

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

* Both **Win32 (x86)** and **x64** binaries are provided
* This repository contains **prebuilt binaries only**, not the libiconv source code
* The binaries are built using **vcpkg**

---

## 🙏 Acknowledgements

* GNU libiconv
* vcpkg
