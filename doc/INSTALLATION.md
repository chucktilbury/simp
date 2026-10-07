# Build and installation

The compiler uses CMake and requires Clang on `PATH` to compile and link its
generated LLVM IR. The project uses C11 and C++17. Building and running
`simpkg` also requires Python 3.11 or newer; installing packages requires
`git`. A repository build stages
the compiler resources beside `bin/simp`, so the executable can run directly
from the source tree:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

The current native compiler/runtime target is POSIX (including `unistd.h`
and pthreads); non-POSIX targets are not supported. CMake reports a missing
`unistd.h`, and generated inline-C compilation diagnoses any required target
header that Clang cannot find rather than silently omitting it.
Installed application resources include the public opaque C API
`include/simp/Stdlib.h` and the linked runtime archive; application developers
do not need runtime implementation source. See
[the inline C API](STDLIB.md#inline-c-api) for supported bindings.

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

## Optional GTK 4 interface

GTK is opt-in: `-DSIMP_GTK=ON` requires `pkg-config`, GTK 4 development files,
and Xvfb plus `dbus-daemon` for the configured real-GTK integration tests.
On Debian/Ubuntu these are `pkg-config libgtk-4-dev xvfb dbus-daemon`.
Configuration fails if a dependency is
missing; tests do not silently skip an unavailable display. Xvfb can be selected
with `-DSIMP_XVFB_EXECUTABLE=/absolute/path/to/Xvfb`.

The optional Tweed editor additionally uses GtkSourceView 5. Enable it with
`-DSIMP_GTK_SOURCEVIEW=ON` (which also enables GTK). Install
`libgtksourceview-5-dev` on Debian/Ubuntu. This option stages the separate
`sourceview/0.1.0` package and its native support; ordinary compiler builds and
applications that do not import these packages remain independent of GTK.
Build and run the editor with:

```sh
cmake -S . -B build-editor -DCMAKE_BUILD_TYPE=Debug \
  -DSIMP_GTK=ON -DSIMP_GTK_SOURCEVIEW=ON \
  -DSIMP_STAGE_PREFIX="$PWD/build-editor/stage"
cmake --build build-editor -j4
build-editor/stage/bin/simp examples/editor/tweed.simp \
  -o build-editor/tweed-editor
build-editor/tweed-editor file1.simp file2.simp
```

The editor documentation describes its current capabilities and limitations:
[Tweed editor first iteration](TWEED-EDITOR.md).

The build stages/installs the `gtk/0.1.0` package and its native archive together.
Its import metadata links GTK only into applications importing it, never the
compiler or non-import programs. With the option off, no GTK package is staged
or installed. Use a separate staging prefix for different configurations.
With sourceview enabled, the staged GTK manifest also carries the GtkSourceView
link dependency needed by the optional native editor package. See
[GTK.md](GTK.md) for the widget API, application activation, and ownership.
Optional GTK examples and documentation programs run under a fresh headless
display and isolated session bus when this build option is enabled.

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
`${CMAKE_INSTALL_INCLUDEDIR}/simp/`, the String builtin in
`${CMAKE_INSTALL_DATADIR}/simp/builtin/`, standard modules in
`${CMAKE_INSTALL_DATADIR}/simp/modules/`, and documentation in
`${CMAKE_INSTALL_DOCDIR}` plus `${CMAKE_INSTALL_MANDIR}/man1/simp.1`.
The documentation install also preserves the repository-relative
`doc/`, `tests/`, and `stdlib/` indexes and references.
Installation directories must remain inside `CMAKE_INSTALL_PREFIX`.

The compiler derives resources relative to its executable and the installation
prefix; an installed tree can be moved as a unit. Resource lookup can be
overridden with `SIMP_RUNTIME_DIR`, `SIMP_INCLUDE_DIR`, `SIMP_BUILTIN_DIR`, or
`SIMP_STDLIB_MODULE_DIR` individually, or with `SIMP_HOME` for the prefix.
`CC` selects the compiler-driver executable in place of the configured Clang
driver. Use `simp --print-paths` to inspect the resolved executable, resources,
every package root and its precedence, project lock location, and Clang executable.

The former `prelude/` source directory, `share/simp/prelude/` resource path,
and `SIMP_PRELUDE_DIR` override have been replaced by `builtin/`,
`share/simp/builtin/`, and `SIMP_BUILTIN_DIR`. No legacy path fallback or
environment alias is supported: rebuild and reinstall the compiler and its
resources together, and update any resource overrides. `SIMP_BUILTIN_DIR`
names the directory containing `String.simp`, not a project package root.

## Project module search

The compiler discovers project configuration separately from package storage.
The project root is the nearest ancestor of the first Simple source input
containing `simpkg.toml` or a `modules` directory; without a marker it is the
source parent. With no source input, discovery starts at the current directory.
This source-based discovery is the same for relative and absolute source paths.
The manifest and adjacent `simpkg.lock` remain at that root regardless of
package-root overrides.

Package imports use ordered first-match lookup through these roots:

1. `-M DIR`/`--module-dir DIR`, when provided.
2. `<project-root>/modules`.
3. `SIMP_MODULE_DIR`, when set.
4. The user package root `<config-home>/simp/modules`, normally
   `~/.config/simp/modules`. If `XDG_CONFIG_HOME` is set, it is used as
   `<XDG_CONFIG_HOME>/simp/modules` and must be absolute.
5. Standard modules shipped with the compiler, normally
   `<prefix>/share/simp/modules`, configurable with `SIMP_STDLIB_MODULE_DIR`.

The first root containing a package shadows lower-priority roots, and version
selection occurs only among versions in that root. Project and home roots are
optional fallbacks when missing; explicit `-M` and `SIMP_MODULE_DIR` roots must
exist for compilation. `-M` and `SIMP_MODULE_DIR` add storage roots; they never
move or replace the discovered project's manifest or lock. `-p`/`--path` sets
the separate textual `include` search path. Package layout and APIs are
described in the [standard library docs](STDLIB.md) and
[`stdlib/README.md`](../stdlib/README.md); exact CLI behavior is in
[simp(1)](simp.1).

### Locked project dependencies

New projects use `simpkg.toml` for direct dependencies and generated
`simpkg.lock` for the complete exact graph. The compiler checks the lock schema,
manifest fingerprint, dependency pins, and hashes of packages used by the
compilation. It never accesses the network. Missing, stale, or invalid locks
are errors, not invitations to fall back to another version or root. A storage
override can satisfy a lock only when the locked package name, exact version,
dependency edges, and content hash match; a conflicting or invalid higher
root is reported rather than bypassed. Imports not listed in the lock are
rejected. `-M` and `SIMP_MODULE_DIR` do not select another project's policy.
See [the package schema](PACKAGES.md).

The previous `--package-path`, `SIMP_PACKAGE_PATH`, `SIMP_MODULE_REGISTRY`,
`./simp-modules.tsv`, and `modules/modules.toml` lookup/policy mechanisms have
been removed. They have no compatibility aliases or fallback behavior.
Compiler diagnostics identify legacy inputs when found; migrate projects to
`simpkg.toml` and `simpkg.lock` with `simpkg init`, then declare and install
dependencies with `simpkg add`/`simpkg install`. `simp --print-paths` reports
the project lock and every applicable package root with its origin and order.

## Project package manager (`simpkg`)

`simpkg` is a small project manager and GitHub package installer. The
compiler remains responsible for compiling and building programs; `simpkg`
does not run programs or implement compiler build logic. It is installed beside
`simp` and uses the standard modules shipped with that same installation.

Start a project from its root:

```sh
mkdir hello-simp
cd hello-simp
simpkg init
simpkg add OWNER/REPO --yes
simp path/to/app.simp
```

`simpkg init [PROJECT_DIR]` creates `simpkg.toml`, `simpkg.lock`, and the
`modules` directory, locking the installed standard modules locally without
network access. It refuses to overwrite existing configuration. The compiler's
`String` builtin/runtime is available by default; it is not an ordinary
imported module and is not listed in the lockfile's module allowlist or packages.

`simpkg add OWNER/REPO [VERSION] --yes` obtains a package from that GitHub
repository, using Git's configured authentication for private repositories
that the user can access. With no version it picks the highest stable SemVer
tag; with a version it checks out that exact SemVer tag (including a
prerelease, if requested). It validates
`simp-package.toml` and the package source, resolves all exact transitive
dependencies, and installs into `modules/<package-name>/<version>/`. It saves
the direct dependency in `simpkg.toml` and generates the complete graph in
`simpkg.lock`, including repository, tag, commit, content digest, and edges.
Existing versions are never overwritten: different contents at an existing
destination are an integrity error. Run `simpkg list` to inspect selections.

GitHub `OWNER/REPO` is a direct repository shortcut, not a package catalog:
there is no central index or guessed repository lookup. A dependency's
repository must be declared in its package's `[sources]` table or explicitly
mapped in the project's `[sources]` table. Bundled packages are resolved
locally. Missing mappings and conflicting exact pins fail explicitly. Ranges,
branches, arbitrary URLs, install scripts, and automatic builds are not
supported.

Before network access, `simpkg` prints the requested plan and requires explicit
confirmation, or `--yes` for noninteractive use. Each declared repository is
shown before it is contacted; `--yes` authorizes the declared transitive
repositories too. Without consent, redirected/noninteractive input fails
without fetching. `--dry-run` shows the known plan without network access;
`--dry-run --yes` may fetch metadata and resolve the full graph, but does not
install or modify project files. A full resolved graph is printed before
installation. Review untrusted repositories before granting consent: fetched
source can contain native bindings used during later compilation.

`simpkg install --yes` restores a committed lockfile's exact commits and
verifies hashes, rather than trusting potentially moved tags. With no lock it
resolves the direct manifest. Commit both manifest and lock; do not hand-edit
the lock or maintain transitive version arrays. `add`, `install`, and `list`
locate the project from nested working directories. After `add` or `install`,
ordinary `simp src/nested/app.simp` is compile-ready with no environment eval.
Legacy projects without `simpkg.toml` retain their old policy and explicit
dependency installation contract.

`simpkg env` prints POSIX shell exports; it does not modify the parent shell.
It reads optional user preferences from
`$XDG_CONFIG_HOME/simp/preferences.toml`, or
`~/.config/simp/preferences.toml` when `XDG_CONFIG_HOME` is unset. Create the
file yourself if desired; only `[environment]` string values are supported:

```toml
[environment]
CC = "clang"
MY_BUILD_SETTING = "debug"
```

Then `eval "$(simpkg env)"` applies those values in the current shell. An
alternate existing file can be selected with `simpkg env --preferences FILE`;
`simpkg` never creates or edits preference files. Environment names must be
valid shell variable names. Package lookup needs no activation: source-based
project discovery works from nested directories and for absolute source paths.
User packages installed at `<config-home>/simp/modules` are searched after
project packages and before installation defaults.
