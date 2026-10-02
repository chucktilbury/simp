# Build and run examples

After [building the compiler](INSTALLATION.md), the following examples
compile checked-in functional fixtures, run the resulting executables, and
show the expected output or behavior:

```sh
./bin/simp tests/functional/positive/positive_integer_output.simp \
  -o bin/positive_integer_output
./bin/positive_integer_output
# Prints: 42

./bin/simp tests/functional/positive/positive_integer_control_flow.simp \
  -o bin/positive_integer_control_flow
./bin/positive_integer_control_flow
# Prints: 9

./bin/simp tests/functional/positive/positive_string_format.simp \
  -o bin/positive_string_format
./bin/positive_string_format
# Prints café and a blank line, then "value: 42" and "sum 21 21".

./bin/simp tests/functional/positive/positive_class_counter.simp \
  -o bin/positive_class_counter
./bin/positive_class_counter
# Prints: 42, then 42

./bin/simp tests/functional/positive/positive_gc_object_graph.simp \
  -o bin/positive_gc_object_graph
./bin/positive_gc_object_graph
# Prints: 1, 64, and 77 after repeated collections.

./bin/simp tests/functional/positive/positive_multiple_inheritance.simp \
  -o bin/positive_multiple_inheritance
./bin/positive_multiple_inheritance
# Prints: 7, 7, 10, 20, and 3; the two Root subobjects hold separate Node references.

./bin/simp tests/functional/positive/positive_secondary_bases.simp \
  -o bin/positive_secondary_bases
./bin/positive_secondary_bases
# Exercises secondary-base construction, conversions, dispatch, GC tracing, and destruction.
```

To save the generated LLVM IR as well as building an executable:

```sh
./bin/simp tests/functional/positive/positive_integer_output.simp \
  --emit-llvm build/positive_integer_output.ll -o bin/positive_integer_output
```

These are executable regression fixtures, not standalone documentation
snippets. The grammar and language reference describe each feature; see
[`../tests/README.md`](../tests/README.md) for adding or running tests.
