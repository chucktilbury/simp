# Standard library source layout

This directory contains the source packages shipped with the Simple compiler.
For the package APIs, see the [standard library reference](../doc/STDLIB.md).

## Layout

Each package is stored as `<package>/<semver>/simp-package.toml` plus its
Simple source files (and any optional native assets):

```text
stdlib/
└── <package-name>/
    └── <semver>/
        ├── simp-package.toml
        └── <sources>.simp
```

The package directory name is its valid Simple package identifier. The version
directory is a SemVer version and must match the manifest's `version` value.

## Manifest format

Every version directory contains `simp-package.toml` with a `[package]`
section. For example:

```toml
[package]
name = "example"
version = "0.1.0"
source = "example.simp"
export = "namespace:example"    # or class:ClassName
```

`name` must match the package directory; `version` must match the version
directory; `source` is relative to that version directory; and `export` names
the exported class or namespace.

Optional manifest sections include `[dependencies]` with exact version pins
(for example, `pkg = "=0.1.0"`), `[sources]` mapping those names to GitHub
`OWNER/REPO`, and native-library configuration. Omitted dependency sources
must be mapped explicitly by the consuming project (bundled dependencies
resolve locally). See [the complete package and lock schema](../doc/PACKAGES.md).
An unaliased import binds the declared export name, not the package name.

## Staging and installation

CMake stages and installs package contents automatically:

- During a build, contents are staged into
  `<prefix>/share/simp/modules/` during the build.
- During installation, contents go to
  `${CMAKE_INSTALL_DATADIR}/simp/modules/`.

The `gtk/0.1.0` source tree is optional: it is staged/installed only with
`SIMP_GTK=ON`, together with its package-native archive. Its native directory
is package implementation detail. See [the GTK foundation guide](../doc/GTK.md).

The `sourceview/0.1.0` package is enabled by
`SIMP_GTK_SOURCEVIEW=ON` and depends on the optional GTK package and GtkSourceView
5. It provides editor-oriented source buffers/views, search and replace,
undo/redo, cursor information, and shortcut binding. The Tweed language syntax
definition lives with that package. The Simple-written first-iteration editor
is in `examples/editor/`; see [its guide](../doc/TWEED-EDITOR.md). Neither
optional package is linked into the compiler or programs that do not import it.
Fresh configurations default these options to available development dependencies;
see [build and installation](../doc/INSTALLATION.md) for compiler-only settings.

## Adding a package

Create `stdlib/<package>/<semver>/`, add the manifest and source files, and
declare the package's root source and exported class or namespace in the
manifest. New package directories are picked up by CMake staging and
installation without separate CMake edits. Keep the API reference in
[`doc/STDLIB.md`](../doc/STDLIB.md), rather than duplicating it here.
