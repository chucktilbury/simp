# Build and installation

The compiler uses CMake and requires Clang on `PATH` to compile and link its
generated LLVM IR. The project uses C11 and C++17. A repository build stages
the compiler resources beside `bin/simp`, so the executable can run directly
from the source tree:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

The root build is the recommended build. The `include/`, `src/`, and `tests/`
directories also have component `CMakeLists.txt` files; standalone
configuration is optional. To build only the compiler outside the repository's
in-tree build directory:

```sh
cmake -S src -B /tmp/simp-compiler-build
cmake --build /tmp/simp-compiler-build
```

`tests/functional/` contains fixtures, not a separate buildable component.
The root build places the compiler and test executables in `bin/` and the
front-end archive in `lib/libsimp_frontend.a`.

## Install layout and relocation

Configure the installation prefix, build, and install:

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

For a staged package or image, use `DESTDIR`:

```sh
DESTDIR=/tmp/stage cmake --install build
```

CMake's `GNUInstallDirs` control the destinations. The installed tree contains
the compiler in `${CMAKE_INSTALL_BINDIR}`, the runtime archive in
`${CMAKE_INSTALL_LIBDIR}/simp/`, runtime headers in
`${CMAKE_INSTALL_INCLUDEDIR}/simp/`, the String prelude in
`${CMAKE_INSTALL_DATADIR}/simp/prelude/`, standard modules in
`${CMAKE_INSTALL_DATADIR}/simp/modules/`, and documentation in
`${CMAKE_INSTALL_DOCDIR}` plus `${CMAKE_INSTALL_MANDIR}/man1/simp.1`.
The documentation install also preserves the repository-relative
`doc/`, `tests/`, and `stdlib/` indexes and references.
Installation directories must remain inside `CMAKE_INSTALL_PREFIX`.

The compiler derives resources relative to its executable and the installation
prefix; an installed tree can be moved as a unit. Resource lookup can be
overridden with `SIMP_RUNTIME_DIR`, `SIMP_INCLUDE_DIR`, `SIMP_PRELUDE_DIR`, or
`SIMP_STDLIB_MODULE_DIR` individually, or with `SIMP_HOME` for the prefix.
`CC` selects the compiler-driver executable in place of the configured Clang
driver. Use `simp --print-paths` to inspect the resolved executable, resources,
module roots, registry, and Clang executable.

## Project module search

Imports resolve packages through these module roots, in order:

1. The project root selected with `-M DIR`/`--module-dir DIR`, otherwise
   `SIMP_MODULE_DIR`, otherwise `<project-root>/modules`. The project root is
   the directory containing the first Simple source input (or the current
   directory when no source is supplied). A missing default module directory
   is skipped; a selected but missing directory is an error.
2. The standard modules shipped with the compiler, normally under
   `<prefix>/share/simp/modules`, configurable with `SIMP_STDLIB_MODULE_DIR`.
3. Deprecated compatibility roots from `--package-path` and then
   `SIMP_PACKAGE_PATH`.
4. The deprecated tab-separated module registry, selected by
   `SIMP_MODULE_REGISTRY` (default `./simp-modules.tsv`).

The first root containing a package shadows lower-priority roots, and version
selection occurs only among versions in that root. `-M` and
`SIMP_MODULE_DIR` set the project package root; `-p`/`--path` sets the
separate textual `include` search path. Package layout and APIs are described
in the [standard library docs](STDLIB.md) and
[`stdlib/README.md`](../stdlib/README.md); exact CLI behavior is in
[simp(1)](simp.1).

### Project module version selection

Place an optional `modules.toml` in the canonical project module root: the
directory selected by `-M DIR`/`--module-dir DIR`, `SIMP_MODULE_DIR`, or the
default `<project-root>/modules`. When present, it is a strict package
allowlist and ordered version preference for imports from every package root,
including the bundled standard modules:

```toml
[modules]
geometry = ["1.2.3", "1.1.0"]
system = ["0.1.0"]
```

Each value must be a nonempty, single-line array of distinct exact SemVer
strings; only the `[modules]` table is supported.
Versions are checked in order, and the first version installed in the
established highest-priority root wins. A later listed version is tried only
when an earlier version is absent; malformed installed packages and exact
dependency-pin conflicts are errors, not reasons to fall back. If no listed
version is installed, compilation reports the versions searched. Unlisted
direct or transitive packages are not found. Version ranges and automatic
newest-version selection are not supported while this file exists. Legacy
registry-only modules also cannot be resolved under this policy because they
do not provide the package manifests needed to enforce exact versions.

When `modules.toml` is absent, existing package and deprecated registry
resolution behavior is retained for compatibility with current projects.
`simp --print-paths` shows the policy file location, and verbose compilation
reports the selected module versions.
