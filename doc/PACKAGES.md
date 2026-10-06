# Package manifests and reproducible projects

`simpkg` manages packages; `simp` compiles installed source without accessing
the network. See [installation](INSTALLATION.md#project-package-manager-simpkg)
for the common `init`, `add`, and `install` workflow.

## Project manifest: `simpkg.toml`

Schema 1 is human-readable TOML with exact SemVer pins. Only direct
dependencies belong in `[dependencies.NAME]`; dependency order is irrelevant.
The GitHub repository identity is explicit, not inferred from a package name.

```toml
schema = 1

[dependencies.geometry]
repo = "your-org/geometry"
version = "=1.2.3"

[sources]
vectors = "your-org/vectors"
```

The repository names above are illustrative, not a public package catalog.
`version` supports only `=VERSION`, including explicit prereleases; ranges
are not supported. `simpkg add OWNER/REPO [VERSION] --yes` reads the package
name from the downloaded manifest and records a direct exact pin. Omitting
VERSION selects the highest stable SemVer tag at that time; subsequent installs
use the lock's commit, not a fresh version selection.

`[sources]` is an optional explicit mapping for transitive names whose package
manifest lacks a source. A package may supply its own mapping; inconsistent
identities for the same name are errors, not priority-based guesses.
The shipped standard packages are locked locally during `init`; they are not
network dependencies and need no GitHub source mapping. Each bundled package
is pinned to its highest stable installed version; a prerelease-only bundled
package or an incompatible exact dependency on an older bundled version fails
explicitly rather than producing fallback arrays. `String` is the implicit
compiler prelude, not a package entry.

Commit `simpkg.toml` and `simpkg.lock`. To change a direct pin, use `add`, or edit
the manifest, remove the stale lock, and resolve with `simpkg install --yes`.
`add` rejects an already stale lock before contacting any repository. The compiler
always reads the lock from the discovered source project; `-M` and
`SIMP_MODULE_DIR` only add higher-priority package storage roots. A root can
satisfy a locked package only with the exact locked version, dependency edges,
and content hash. The lock remains the import allowlist. Legacy
`modules/modules.toml`, registry, and compatibility path workflows are removed;
create a manifest/lock project and declare dependencies with `simpkg`.

## Published package: `simp-package.toml`

Package directories have the shape `modules/NAME/VERSION/`:

```toml
[package]
name = "geometry"
version = "1.2.3"
source = "src/geometry.simp"
export = "namespace:Geometry"

[dependencies]
vectors = "=0.4.0"

[sources]
vectors = "your-org/vectors"

[link]
libraries = ["geometry_native"]
library-paths = ["lib"]
```

The package name and export name must be non-keyword Simple identifiers.
`name` and `version` must match the installed directory names; a fetched tag
must match `version`. `source` must name a file within the package and cannot
be absolute or contain `..`. `export` is either `namespace:Name` or
`class:Name`, and that top-level definition must exist in the source.

Dependencies are exact pins under `[dependencies]`. Every imported package in
package source must also be declared here. Optional `[sources]` entries must
name declared dependencies and use GitHub `OWNER/REPO`. An omitted repository
requires an explicit consuming-project mapping or an exact locally bundled
dependency; the installer never guesses where to download code.
The compiler accepts this same source-mapping table for installed and legacy
packages. Cycles, inconsistent pins, and inconsistent source identities fail.

Optional `[link]` entries provide arrays of native library names (without a
`-l` prefix) and package-relative library directories. Native assets must
already be present; the installer never executes package build scripts.
Package installation rejects symbolic links and special files.

```simp
import geometry
import geometry as G

start {
    // Both bindings denote the declared namespace, not an extra package layer.
    Geometry.Point point = Geometry.Point()
    G.Point another = G.Point()
}
```

The example assumes `namespace Geometry` exports a class `Point`.
Explicit aliases preserve the existing semantics. Without `as`, the binding
is the manifest's declared export (`Geometry`), not the package identifier
(`geometry`). Colliding local/import bindings and conflicting package namespaces
are diagnosed. An exported class similarly binds its own name by default.

## Generated graph: `simpkg.lock`

Schema 1 locks every resolved node, including the bundled standard packages.
The following excerpt illustrates a node with one dependency; a real lock
also contains that dependency's module entry and package table:

```toml
schema = 1
manifest-sha256 = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"

[modules]
geometry = ["1.2.3"]
vectors = ["0.4.0"]

[packages.geometry]
repo = "your-org/geometry"
version = "1.2.3"
ref = "refs/tags/1.2.3"
commit = "0123456789abcdef0123456789abcdef01234567"
sha256 = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
dependencies = ["vectors=0.4.0"]
```

`[modules]` contains exactly one version per node, not ordered fallback arrays.
Package tables store repository, tag ref, exact Git commit, content SHA256, and
exact graph edges. Local standard-package nodes use `builtin` for `repo`,
`ref`, and `commit`, with the same content checks. Tables and dependency lists
are generated deterministically. `manifest-sha256` hashes the raw manifest
bytes, so even a manual formatting edit makes an existing lock stale.

Content hashing sorts regular files by relative POSIX path and hashes each
UTF-8 path, NUL byte, file contents, and NUL byte in that order. The `.git`
directory is excluded; directories, timestamps, and file permissions are not
hashed. Symlinks and special files are rejected. Hashes detect stale/tampered
installed contents; they are not publisher signatures or proof that code is
safe. Review repositories before authorizing download or compiling their
native bindings.

`simpkg install --yes` verifies manifest freshness, schema, exact versions,
commit identities, edges, and content before restoring missing packages. It
does not overwrite mismatching existing directories. The compiler independently
checks lock structure/freshness and the hashes and declared edges of packages
used by the compilation. A changed bundled package therefore requires an
intentional lock regeneration, not silent acceptance.

## Network consent and plans

No network operation starts without an explicit interactive confirmation or
`--yes`. Noninteractive calls without consent fail. The initial plan shows
known repositories before resolution, each declared transitive repository is
shown before contact, and a complete resolved plan precedes installation.
`--yes` explicitly opts into the graph's declared repositories, not arbitrary
URL guessing. An unmapped dependency fails without contacting a guessed remote.

`--dry-run` displays known work without network or project writes.
`--dry-run --yes` permits temporary metadata downloads to discover the complete
graph, but does not install or change manifest/lock files. Resolution requires
reading package manifests, so a complete remote graph cannot be known offline
before the first authorized download.

File updates are atomic individually and installation failures attempt rollback,
reporting rollback failures explicitly. The manifest, lock, and package tree
are not a crash-transactional database; do not run concurrent package mutations
or replace project directories while installation is running.
