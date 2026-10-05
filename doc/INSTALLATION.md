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
   the nearest ancestor of the first Simple source input containing
   `simpkg.toml` or a `modules` directory (starting at the current directory
   when no source is supplied). Without a marker it is the source parent.
   A missing default module directory
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

Selecting a locked project's `modules` directory, implicitly or explicitly,
uses the adjacent `simpkg.lock`. A different explicit `-M` or environment
directory selects that directory's policy instead; it does not combine it with
the source project's lock. `-M` takes precedence over `SIMP_MODULE_DIR`.

### Project module version selection

New projects use `simpkg.toml` for direct dependencies and generated
`simpkg.lock` for the complete exact graph. The compiler checks the lock schema,
manifest fingerprint, dependency pins, and hashes of packages used by the
compilation. It never accesses the network. Missing, stale, or invalid locks
are errors, not invitations to fall back to an installed version. Both
`simpkg.toml` and `modules/modules.toml` in one project are an ambiguous
configuration and are rejected. See [the package schema](PACKAGES.md).

For legacy projects without `simpkg.toml`, place an optional `modules.toml` in the canonical project module root: the
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

When both the new manifest and `modules.toml` are absent, existing package and deprecated registry
resolution behavior is retained for compatibility with current projects.
`simp --print-paths` shows the policy file location, and verbose compilation
reports the selected module versions.

## Project package manager (`simpkg`)

`simpkg` is a small project-environment and GitHub package installer. The
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
`String` prelude/runtime is built in and available by default; it is not an
ordinary imported module and is not listed in the allowlist.

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
Evaluate the output from the project root as shown above. It reads optional
user preferences from `$XDG_CONFIG_HOME/simp/preferences.toml`, or
`~/.config/simp/preferences.toml` when `XDG_CONFIG_HOME` is unset. Create the
file yourself if desired; only `[environment]` string values are supported:

```toml
[environment]
CC = "clang"
MY_BUILD_SETTING = "debug"
```

Then `eval "$(simpkg env)"` applies those values in the current shell and sets
`SIMP_MODULE_DIR` to the current project's `modules` directory, regardless of
any `SIMP_MODULE_DIR` inherited from elsewhere. An alternate existing file can
be selected with `simpkg env --preferences FILE`; `simpkg` never creates or
edits preference files. Environment names must be valid shell variable names,
and `SIMP_MODULE_DIR` is reserved for the activated project.

`simpkg env` remains an optional way to apply user preferences or deliberately
activate a module-directory override. It is not required for normal compilation,
including sources nested beneath the project root. An inherited override still
takes precedence; unset `SIMP_MODULE_DIR` to return to source-based discovery.
