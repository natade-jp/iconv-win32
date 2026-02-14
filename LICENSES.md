# Licenses

This repository distributes **prebuilt libiconv binaries for Windows (x86 / x64)**.

---

## libiconv

libiconv is licensed under the **GNU Lesser General Public License (LGPL)**.

Copyright (C) Free Software Foundation, Inc.

The full license text is included in:

```

dist/LICENSES/libiconv-LICENSE.txt

```

Source code:  
https://ftp.gnu.org/pub/gnu/libiconv/

---

## About This Repository

This repository:

- provides **prebuilt binaries only**
- does **not** include the libiconv source code
- builds the binaries using **vcpkg**

If you need to rebuild libiconv yourself, use the provided build scripts.

---

## License Compliance

This distribution follows the requirements of the LGPL by:

- providing the original license text
- clearly stating that libiconv is LGPL-licensed
- allowing users to rebuild the library from source

Users of this package must ensure their own compliance with the LGPL when redistributing applications that link to libiconv.
