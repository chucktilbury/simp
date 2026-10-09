# Build and run examples

After [building the compiler](INSTALLATION.md), the runnable product examples
in `examples/` use the canonical `.cw` suffix. The focused commands below also
compile checked-in functional fixtures, run the resulting executables, and
show the expected output or behavior:

Formatting uses literal templates, not calls on string literals:

```simp
// test: {"stdout": "this is 420000002AASCII: Aliteral {}"}
start {
    strg message = format("this is {}", 42)
    print(message)
    print("{value:08X}", value=42)
    print("ASCII: {:c}", 65)
    print("literal {}")
}
```

This writes `this is 42`, `0000002A`, `ASCII: A`, and `literal {}` with no
implicit line breaks.
`c` accepts only ASCII 0..127. See the language reference for the supported
specifier subset.

```sh
./bin/cwhip tests/functional/positive/positive_integer_output.simp \
  -o bin/positive_integer_output
./bin/positive_integer_output
# Prints: 42

./bin/cwhip tests/functional/positive/positive_integer_control_flow.simp \
  -o bin/positive_integer_control_flow
./bin/positive_integer_control_flow
# Prints: 9

./bin/cwhip tests/functional/positive/positive_string_format.simp \
  -o bin/positive_string_format
./bin/positive_string_format
# Writes "café\nvalue: 42sum 21 21"; print adds no implicit line breaks.

./bin/cwhip tests/functional/positive/positive_class_counter.simp \
  -o bin/positive_class_counter
./bin/positive_class_counter
# Prints: 42, then 42

./bin/cwhip tests/functional/positive/positive_gc_object_graph.simp \
  -o bin/positive_gc_object_graph
./bin/positive_gc_object_graph
# Prints: 1, 64, and 77 after repeated collections.

./bin/cwhip tests/functional/positive/positive_multiple_inheritance.simp \
  -o bin/positive_multiple_inheritance
./bin/positive_multiple_inheritance
# Prints: 7, 7, 10, 20, and 3; the two Root subobjects hold separate Node references.

./bin/cwhip tests/functional/positive/positive_secondary_bases.simp \
  -o bin/positive_secondary_bases
./bin/positive_secondary_bases
# Exercises secondary-base construction, conversions, dispatch, GC tracing, and destruction.
```

To save the generated LLVM IR as well as building an executable:

```sh
./bin/cwhip tests/functional/positive/positive_integer_output.simp \
  --emit-llvm build/positive_integer_output.ll -o bin/positive_integer_output
```

These are executable regression fixtures, not standalone documentation
snippets. The grammar and language reference describe each feature; see
[`../tests/README.md`](../tests/README.md) for adding or running tests.

## Package workflow

After installing `simp` and `simpkg` on `PATH`, a project with nested sources
needs no environment activation:

```sh
mkdir -p hello-simp/src
cd hello-simp
simpkg init
# Copy examples/package_workflow.cw from this repository to src/main.cw.
cwhip src/main.cw -o hello
./hello
```

The example uses the default exported `System` namespace, a `math` alias,
and the implicit `String` prelude. `init` locks the bundled packages locally,
without network access. For an external dependency, review its repository and
run `simpkg add OWNER/REPO VERSION --yes`; the command installs the complete
declared graph and updates the manifest/lock. `simpkg install --yes` restores
that lock on another checkout. See [package schemas and plan/consent
behavior](PACKAGES.md) before adding untrusted dependencies.
