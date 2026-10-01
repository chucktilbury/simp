# Simple Standard Library

This directory is the source tree for standard library packages shipped with
the Simple compiler.

It is named `stdlib/` rather than `modules/` so that compiling a Simple source
file from the repository root does not implicitly treat the standard library as
the project module root (`<project-root>/modules`).

## Directory Layout

The layout inside `stdlib/` mirrors the package module layout:

```
stdlib/
├── README.md
└── <package-name>/
    └── <semver-version>/
        ├── simp-package.toml
        └── <source-files>.simp
```

- Each top-level directory corresponds to a package name (a valid Simple
  identifier).
- Each subdirectory corresponds to a SemVer version string (e.g., `0.1.0`,
  `1.0.0`). The directory name must match the `version` field in its manifest.
- Each version directory contains a package manifest (`simp-package.toml`) and
  its Simple source files (and optional native library assets).

## Package Manifest (`simp-package.toml`)

Manifests must provide the `[package]` section:

```toml
[package]
name = "example"
version = "0.1.0"
source = "example.simp"
export = "namespace:example"    # or class:ClassName
```

- `name`: Must be a valid Simple identifier matching the directory name.
- `version`: A valid SemVer string matching the version directory name.
- `source`: Relative path to the root Simple source file within the package.
- `export`: Export specification, either `class:<Name>` or `namespace:<Name>`.

Optional sections include `[dependencies]` with exact pins (e.g. `pkg = "=0.1.0"`)
and native library configurations.

## Staging and Installation

CMake stages and installs the contents of `stdlib/` automatically:

- **Build staging:** The directory contents are staged into
  `<prefix>/share/simp/modules/` during the build.
- **Installation:** The directory contents are installed into
  `${CMAKE_INSTALL_DATADIR}/simp/modules/`.

New standard library packages added here are automatically staged and installed
without requiring changes to CMake configuration files.

The shipped interfaces are documented in the repository `README.md` and
`SIMPLE-LANGUAGE-NOTES.md`. They currently include `system`, `math`,
`networking`, `time`, `process`, `terminal`, `random`, and
`synchronization`.
