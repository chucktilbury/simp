# Standard library reference

Standard packages are imported by package name. Without an alias, the manifest's
declared namespace is exposed: `import system` provides `System.Process` and
`System.System`. An optional alias itself names the
package namespace; use the exported classes directly beneath it. For example,
`import system as Sys` makes `Sys.Process` and `Sys.System` the class names
(not `Sys.System.Process`):

```simp
import system as Sys
import math as MathLib
// test: {"stdout": "true"}

start {
    MathLib.Math math = MathLib.Math()
    print(math.sqrt(9.0) == 3.0)
    Sys.Process().exit(0)
}
```

The shipped package sources live in `stdlib/<package>/<version>/`; this
reference describes the current packages at version `0.1.0`. Error-message
examples below refer to `System.lastError()`, which returns the current thread's most recent native
error as a string (or an empty string when no error is recorded). It is not a
numeric error-code API. `lastError()` is an instance method; with the `Sys`
alias, call it as `Sys.System().lastError()`.

Simple `int` parameters and results in these APIs are signed 64-bit values;
`unsigned` values are unsigned 64-bit. The native-binding documentation in
the [language reference](LANGUAGE-REFERENCE.md#native-bindings-for-library-authors)
describes their C/LLVM ABI representations.

For native APIs accepting a function pointer plus user context, the installed
`<simp/Callbacks.h>` facade provides retained registrations and generated typed
adapters. See [CALLBACKS.md](CALLBACKS.md) for exact signatures, disconnection,
rooting, owner-thread restrictions, and the fail-fast exception boundary.

## `system`

Import with `import system` to use `System`, or `import system as Sys`;
the alias names the package namespace, so
the exported classes are available directly as `Sys.Process`, `Sys.File`,
`Sys.FileSystem`, `Sys.Glob`, `Sys.StandardIO`, and `Sys.System`.

`Sys.Process` provides command-line and environment access:

```text
int argc()
list argv()
String arg(int index)
void exit(int code)
void abort()
String getEnv(String name)
bool setEnv(String name, String value)
```

`System` forwards `argc`, `argv`, `arg`, `exit`, `abort`, and `getEnv`, and
also provides `StandardIO io()`, `String lastError()`, `File open(String path,
String mode)`, and `bool exists(String path)`. An invalid `arg` index and an
unset environment variable produce an empty string. `setEnv` reports success
as a boolean and records native failures in `lastError()`.

`Sys.File` can be constructed with `File(String path, String mode)` or
obtained from `Sys.System().open`. Its methods are:

```text
String path()
String mode()
bool isOpen()
bool eof()
String read(int size)
String readAll()
String readLine()
list readLines()
int write(String data)
int writeLine(String line)
int seek(int offset, int whence)
int tell()
void flush()
void close()
```

`readLine` removes the line ending (including a preceding carriage return);
`readLines` returns lines as a list. End-of-file reads return empty
string/list results. Invalid handles and I/O failures also return empty
string/list or the operation's failure value and set `lastError()`.
`write`/`writeLine` return the number of bytes written (including the newline
for `writeLine` when successful); failed writes may return a partial count.
`seek` returns zero on success and `-1` on failure; `tell` returns the
position, capped at the signed 64-bit `int` range, or `-1`. `File` records whether the
initial open succeeded; close it explicitly.

`Sys.FileSystem` provides:

```text
bool exists(String path)
bool isFile(String path)
bool isDir(String path)
int fileSize(String path)
bool remove(String path)
bool rename(String oldPath, String newPath)
bool copy(String src, String dest)
bool mkdir(String path)
bool rmdir(String path)
list listDir(String path)
String getCwd()
bool chDir(String path)
String absolutePath(String path)
String join(String left, String right)
String normalize(String path)
String basename(String path)
String dirname(String path)
String extension(String path)
String tempFile()
String tempDir()
```

Status operations return `false` on failure; `fileSize` returns `-1`, directory
listing returns an empty list, and path-producing operations return an empty
string on failure. These failures set `lastError()`; successful filesystem
calls clear it. `absolutePath` resolves an existing path with `realpath`.
`normalize` is lexical and does not resolve symbolic links. `tempFile` creates
a file with mode `0600` and closes its descriptor before returning the path;
`tempDir` creates a temporary directory.

`Sys.Glob` expands POSIX filename patterns without changing the current
directory:

```text
list glob(String pattern)
```

The supported pattern syntax includes `*`, `?`, and bracket expressions
(`[...]`). Results are matching paths in POSIX glob's default sorted order;
relative patterns produce relative paths. A pattern with no matches returns an
empty list and clears `lastError()`. Invalid string arguments and filesystem
errors reported during globbing return an empty list and set `lastError()`.
Recursive `**` and brace expansion are not provided.

`Sys.StandardIO`, obtained with `Sys.System().io()`, has these methods:

```text
String read(int size)
String readLine()
int write(String text)
int writeLine(String text)
int writeBytes(buffer data)
int writeError(String text)
int writeErrorLine(String text)
int writeErrorBytes(buffer data)
void flush()
void flushError()
```

Reads consume stdin; `write*` methods target stdout and `writeError*` methods
target stderr. Byte writes accept buffers. Write methods return bytes written
(including a newline for line writes on success); failures set `lastError()`
and can return partial counts or `-1`. Failed reads return an empty string and
set `lastError()`.

```simp
import system as Sys
// test: {"stdout": "hello\n"}

start {
    Sys.StandardIO io = Sys.System().io()
    io.writeLine("hello")
}
```

## `math`

Import with `import math as M`; the alias names the package namespace and its
exported class is available directly as `M.Math`.

```text
float pi()
float e()
float tau()
float abs(float x)
int absInt(int x)
float min(float a, float b)
float max(float a, float b)
int minInt(int a, int b)
int maxInt(int a, int b)
float clamp(float x, float minVal, float maxVal)
float floor(float x)
float ceil(float x)
float round(float x)
float trunc(float x)
float sqrt(float x)
float cbrt(float x)
float pow(float base, float exp)
float exp(float x)
float log(float x)
float log10(float x)
float log2(float x)
float sin(float x)
float cos(float x)
float tan(float x)
float asin(float x)
float acos(float x)
float atan(float x)
float atan2(float y, float x)
float degrees(float radians)
float radians(float degrees)
float sinh(float x)
float cosh(float x)
float tanh(float x)
```

The angle conversions convert between radians and degrees; trigonometric
functions use radians. The constants are returned by `pi()`, `e()`, and
`tau()`. Numeric results follow the native C math-library behavior; this
package does not report math-domain conditions through `System.lastError()`.

```simp
import math as M
// test: {"stdout": "true"}

start {
    M.Math math = M.Math()
    float root = math.sqrt(9.0)
    print(root == 3.0)
}
```

## `networking`

Import with `import networking as Net`; the alias names the package namespace,
so its classes are available directly as `Net.Socket`, `Net.ServerSocket`, and
`Net.Url`.

`Net.Socket` methods:

```text
Socket()
Socket(int fd)
bool connect(String host, int port)
int send(buffer data)
int sendString(String data)
buffer recv(int maxBytes)
String recvString(int maxBytes)
void close()
bool isConnected()
void setTimeout(int milliseconds)
int getPort()
String getHost()
```

Sockets are IPv4 TCP sockets. `send` and `sendString` return the number of
bytes sent or `-1`; receive methods return the received bytes/string, with a
zero-length result for peer closure and a null/empty result on native receive
failure. A negative `maxBytes` likewise returns null/empty. `connect` returns
`false` on failure. `setTimeout` sets the receive timeout (and has no return
value); negative timeout values are ignored. `close` releases the descriptor.

`Net.ServerSocket` methods:

```text
ServerSocket()
bool bind(int port)
bool bindAddress(String host, int port)
bool listen(int backlog)
Socket accept()
void close()
bool isBound()
int getPort()
```

Binding also creates a listening socket (with the requested address and
backlog); `listen` can set the backlog again. `accept` returns a socket whose
connected state is false if accepting failed. `getPort` returns zero when the
native lookup fails. Socket and DNS failures use the methods' return sentinels;
`System.lastError()` is not a reliable networking error channel.

`Net.Url` parses a string in its constructor and exposes:

```text
Url(String urlString)
String scheme()
String host()
int port()
String path()
String query()
String fragment()
String toString()
```

It retains the original string for `toString`; absent path defaults to `/`,
and `http`/`https` URLs without an explicit port default to 80/443. This is a
small parser, not a URL validator.

```simp
import networking as Net
// test: {"stdout": "true"}

start {
    Net.Url address = Net.Url("https://example.test/path")
    print(address.scheme().equals("https"))
}
```

## `time`

Import with `import time as T`; the alias names the package namespace, making
the exported class directly available as `T.Clock`.

```text
unsigned epochSeconds()
unsigned epochMilliseconds()
unsigned monotonicMilliseconds()
bool sleepMilliseconds(unsigned milliseconds)
```

Epoch values are Unix wall-clock time; monotonic milliseconds are for elapsed
time comparisons, not calendar dates. Sleep retries interrupted system calls
and releases the runtime lock while waiting. Clock failures return zero;
sleep returns `false` on overflow or system failure. These failures are
reported through `System.lastError()`.

```simp
import time as T
// test: {"stdout": "truetrue"}

start {
    T.Clock clock = T.Clock()
    unsigned before = clock.monotonicMilliseconds()
    print(clock.epochSeconds() > 0u)
    print(clock.monotonicMilliseconds() >= before)
}
```

## `sql` and `sqlite`

`import sql as SQL` provides the backend-neutral tagged `SQL.Value` type.
It has exactly five variants: SQL NULL (the zero-argument `Value()`),
signed 64-bit integer (`Value(int)`), 64-bit real (`Value(float)`), text
(`Value(String)`), and blob (`Value(buffer)`). NULL is an explicit value; it
is never represented by an absent value, zero, or empty string. A null
`String` or `buffer` cannot be used to construct the text/blob variants.
`kind()` returns 0 through 4 for NULL, integer, real, text, and blob
respectively; `isNull()` tests NULL. The `asInteger()`, `asReal()`, `asText()`,
and `asBlob()` accessors are exact-type checked and raise `Exception` on a
mismatch; they do not coerce or provide implicit conversions.

`SQL.Connection`, `SQL.Statement`, and `SQL.Transaction` define the shared
database API. A concrete backend subclasses these classes and overrides their
methods; assigning the backend object to the SQL base type retains runtime
dispatch. The methods on the base types raise an exception if a backend does
not override them. Simple does not enforce abstract methods at compile time.
This API does not make SQL syntax or backend behavior portable.

The shared class signatures are:

```text
Connection: bool isOpen(), String error(), bool execute(String sql),
            Statement prepare(String sql), Transaction beginTransaction(),
            bool close()
Statement:  String error(), bool bind(int oneBasedIndex, Value value),
            bool step(), bool hasRow(), bool isDone(), bool failed(),
            Value value(int zeroBasedColumnIndex), bool close()
Transaction: bool isActive(), bool commit(), bool rollback(), bool close()
```

`import sqlite as SQLite` provides:

```text
Connection(String path)
bool isOpen()
String Statement.error()
bool execute(String sql)
Statement prepare(String sql)
bool setBusyTimeout(int milliseconds)
Transaction beginTransaction()
bool close()

String error()
bool bind(int oneBasedIndex, SQL.Value value)
bool step()
bool hasRow()
bool isDone()
bool failed()
SQL.Value value(int zeroBasedColumnIndex)
bool close()

Transaction.isActive()
bool commit()
bool rollback()
bool close()
```

The SQLite package uses the system SQLite library. Its native shim is built and
staged by CMake and linked with `sqlite3`; a SQLite development header/library
must be available when configuring and building Simp, even if a particular
application does not import SQLite. This is a compiler/package build-time
dependency, distinct from application linkage: `sqlite3` and `simp_sqlite`
are added to an application's link only when its resolved import graph uses
the SQLite package. Applications that do not import SQLite do not acquire a
SQLite runtime dependency. `Connection` opens or creates a read/write database.
SQLite's empty path opens a temporary database that
SQLite deletes when the connection closes. Check `isOpen()` after construction
and inspect `error()` on failures. `execute` runs raw SQL (including PRAGMAs) and returns `false`
with the connection error available on failure. Use `prepare` and `bind` for
values originating outside trusted SQL text; binding is parameterized and
does not interpolate values. `prepare` accepts one statement, with trailing
whitespace only. A preparation failure returns null and sets `Connection.error()`.

Bind indexes start at 1; result column indexes start at 0. Binding uses the
same five `SQL.Value` variants, including explicit NULL. `step()` returns true
for a row and false for either completion or error; check `isDone()` and
`failed()` to distinguish them, and inspect `Statement.error()` on failure.
After completion or error, additional `step()` calls return false without
calling SQLite again; in particular, a completed DML statement is not rerun.
`value()` is valid only while `hasRow()` is true. SQLite values are tagged
from each result's runtime storage class (`sqlite3_column_type`), not the
column's declared affinity. Text and blob bytes are copied before returning.
Statements must be explicitly closed/finalized; close each connection as well.
Closing a connection with live statements or an active transaction returns
false, preserves the connection, and reports the SQLite error; finalize
statements and finish transactions before retrying. The native backend retains
connection ownership for each live statement and transaction, so a transaction
cannot retain a dangling connection pointer even if raw SQL ends it with
`ROLLBACK`; the transaction then reports inactive and releases its reference.
A transaction begins with `BEGIN IMMEDIATE`; explicitly commit or roll it
back. `Transaction.close()` rolls back an active transaction. Connection,
statement, and transaction wrappers also release their native ownership in
their language destructor as a GC fallback; explicit close remains the
deterministic lifecycle.

Busy timeout and PRAGMA execution are SQLite-specific. Do not infer cross-
database SQL syntax portability from the shared value API.

```simp
// test: {"stdout": "{\"roundTrip\":true}"}
import sqlite as SQLite
import sql as SQL

start {
    SQL.Connection database = SQLite.Connection(":memory:")
    try {
        if (!database.isOpen()) {
            raise(Exception(database.error()))
        }
        if (!database.execute("CREATE TABLE sample (value)")) {
            raise(Exception(database.error()))
        }
        SQL.Statement insert = database.prepare("INSERT INTO sample VALUES (?)")
        if (insert == null) {
            raise(Exception(database.error()))
        }
        try {
            if (!insert.bind(1, SQL.Value("parameterized text"))) {
                raise(Exception(insert.error()))
            }
            insert.step()
            if (insert.failed()) {
                raise(Exception(insert.error()))
            }
            if (!insert.isDone()) {
                raise(Exception("insert did not complete"))
            }
        } finally {
            if (!insert.close()) {
                raise(Exception(insert.error()))
            }
        }

        SQL.Statement query = database.prepare("SELECT value FROM sample")
        if (query == null) {
            raise(Exception(database.error()))
        }
        try {
            if (!query.step()) {
                if (query.failed()) { raise(Exception(query.error())) }
                raise(Exception("roundtrip row missing"))
            }
            if (!query.value(0).asText().equals("parameterized text")) {
                raise(Exception("roundtrip mismatch"))
            }
            if (query.step()) {
                raise(Exception("unexpected extra row"))
            }
            if (query.failed()) { raise(Exception(query.error())) }
            if (!query.isDone()) {
                raise(Exception("query did not complete"))
            }
        } finally {
            if (!query.close()) {
                raise(Exception(query.error()))
            }
        }
    } finally {
        if (!database.close()) {
            raise(Exception(database.error()))
        }
    }
    print("{\"roundTrip\":true}")
}
```

This example uses an isolated in-memory database and verifies a prepared
insert/query roundtrip. Each statement is finalized before the connection
closes, including on exceptions. It deliberately uses the backend-neutral
`SQL.Connection` and `SQL.Statement` types for ordinary operations; only
SQLite-specific settings such as `setBusyTimeout` require the concrete
`SQLite.Connection` type.

## `process`

Import with `import process as P`; the alias names the package namespace, so
the exported class is available directly as `P.Process`.

```text
Process(String executable, list arguments)
bool started()
bool wait()
int exitCode()
String stdout()
String stderr()
void close()
```

The argument list contains arguments after argv[0]; the executable is
inserted as argv[0] and launched directly with `posix_spawnp`, not through a
shell. The child inherits stdin and the caller's environment. `wait()` waits
for termination while collecting stdout and stderr separately; read those
streams after waiting. A failed spawn gives `started() == false`.
`exitCode()` is `-1` before a successful wait, the process exit status after
normal termination, or the negative signal number after signal termination.
Captured output is limited to `INT32_MAX` bytes per stream. `wait()` reports
wait errors via `false` and `System.lastError()`; output-capture failures set
`System.lastError()` even when the child itself was waited successfully.
Output access before waiting returns an empty string and sets an error.
Explicitly call `close()` for each successful start; it waits/reaps an
unwaited child and releases captured data and descriptors.

```simp
import process as P
// test: {"stdout": "truetruetrue"}

start {
    P.Process child = P.Process("/usr/bin/printf", ["hello"])
    print(child.started())
    print(child.wait())
    print(child.stdout().equals("hello"))
    child.close()
}
```

## `terminal`

Import with `import terminal as Term`; the alias names the package namespace,
so the exported class is available directly as `Term.Terminal`.

```text
bool isInteractive()
bool isOutputInteractive()
int columns()
int rows()
bool supportsColor()
```

The interactive checks test stdin and stdout respectively. Dimensions come
from stdout and return zero when stdout is not a terminal or its size cannot
be read. Color support requires stdout to be a terminal, `TERM` to be set and
not `dumb`, and `NO_COLOR` to be unset. These probes do not change terminal
mode and do not report errors through `System.lastError()`.

```simp
import terminal as Term
// test: {"stdout": "true"}

start {
    Term.Terminal terminal = Term.Terminal()
    print(terminal.isInteractive() == false)
}
```

## `random`

Import with `import random as R`; the alias names the package namespace, so the
exported class is available directly as `R.SecureRandom`.

```text
buffer bytes(int size)
bool fill(buffer target)
```

The implementation uses the operating system's cryptographic random source
(`getrandom` on Linux, with `/dev/urandom` fallback; `/dev/urandom` elsewhere),
not a deterministic pseudorandom generator. `bytes` returns a filled buffer
or null for a negative size or allocation/source failure; `fill` returns
false for an invalid buffer or source failure. Failures set
`System.lastError()`; success clears it.

```simp
import random as R
// test: {"stdout": "truetrue"}

start {
    R.SecureRandom random = R.SecureRandom()
    buffer nonce = random.bytes(16)
    print(nonce != null)
    print(nonce.length == 16)
}
```

## `synchronization`

Import with `import synchronization as Sync`; the alias names the package
namespace, so its classes are available directly as `Sync.Mutex`,
`Sync.Condition`, and `Sync.Semaphore`.

```text
Mutex()
handle nativeHandle()
bool lock()
bool unlock()
bool close()

Condition()
bool wait(Mutex mutex)
bool signal()
bool broadcast()
bool close()

Semaphore(int initialCount)
void wait()
void signal()
void close()
```

These wrappers use native pthread synchronization and require explicit
lifetime management. The `Mutex` is non-recursive and tracks its owning thread. `lock()` blocks
until available; a blocking lock releases the runtime lock while waiting, so
the owner can run and unlock. Relocking by its owner returns false and sets
`System.lastError()` to the platform message for `EDEADLK`. `unlock()` returns
false and sets the message for `EPERM` if called by a non-owner or when
unlocked. Successful operations clear the error; closing a locked or
contended mutex returns false and sets the message for `EBUSY`.

`Condition.wait(mutex)` requires the caller to own `mutex`; otherwise it
returns false and sets the message for `EPERM`. It releases the mutex and
runtime lock while waiting, then reacquires both before returning true.
`signal()` wakes one waiter and
`broadcast()` wakes all; both return true on success. Use a predicate loop
around waits, as condition notifications do not themselves establish that a
condition remains true. Closing a condition with waiters returns false and sets the message for
`EBUSY`.

`Semaphore(initialCount)` blocks in `wait()` until its count is positive,
then consumes one count. `signal()` increments the count; neither has a
result value. A blocked wait releases the runtime lock so other Simple threads
can make progress. `close()` releases its native resources and must only be
called after no thread can wait on or signal it. Semaphore operations have no
error-return channel; invalid negative initial counts abort in the native
runtime.

The mutex and condition operations above expose native error messages through
`System.lastError()`. Their `close()` methods return false and leave the
object usable when they report `EBUSY`; close each primitive only after all
users have stopped.

```simp
import synchronization as Sync
// test: {"stdout": "truetruetrue"}

start {
    Sync.Mutex lock = Sync.Mutex()
    print(lock.lock())
    print(lock.unlock())
    print(lock.close())
}
```

## Inline C API

The installed header **`simp/Stdlib.h`** is the supported application-facing
C facade. Generated inline shims include it automatically and link the shipped
runtime archive; no runtime implementation source or private object structures
are needed. Simple package imports and class wrappers continue to work normally.
The C facade reuses all 123 existing standard-package native bindings, plus
`simp_string_cstr` and `simp_string_bytes` (125 exported functions total).
Its declarations, not incidental declarations in `Runtime*.h`, define the
supported C surface.

| Package / Simple API | Public C functions |
|---|---|
| `system`: Process, System | `simp_system_argc`, `argv`, `arg`, `exit`, `abort`, `getenv`, `setenv`, `last_error` (all with the `simp_system_` prefix) |
| `system`: FileSystem | All `simp_fs_*` and `simp_path_*` declarations in the header: existence/type/size, remove/rename/copy, directories, cwd, absolute path, temporary files/directories, join/normalize/basename/dirname/extension |
| `system`: Glob, File, StandardIO | `simp_glob_glob`, all `simp_file_*` and `simp_stdio_*` declarations: the file and stream operations listed above |
| `math`: Math | All `simp_math_*` declarations: the numeric operations above except the Simple-only `pi`, `e`, and `tau` constants |
| `networking`: Socket, ServerSocket | All `simp_net_socket_*` and `simp_net_server_*` declarations: native descriptor operations (not `Url` parsing or wrapper state getters) |
| `time`: Clock | `simp_time_epoch_seconds`, `simp_time_epoch_milliseconds`, `simp_time_monotonic_milliseconds`, `simp_time_sleep_milliseconds` |
| `terminal`: Terminal | `simp_terminal_stdin_interactive`, `simp_terminal_stdout_interactive`, `simp_terminal_columns`, `simp_terminal_rows`, `simp_terminal_supports_color` |
| `random`: SecureRandom | `simp_random_bytes`, `simp_random_fill` |
| `process`: Process | `simp_process_spawn`, `wait`, `exit_code`, `stdout`, `stderr`, `close` (all with the `simp_process_` prefix) |
| `synchronization`: Mutex, Condition, Semaphore | All `simp_mutex_*`, `simp_condition_*`, and `simp_semaphore_*` declarations; `release` is the C counterpart of wrapper `close` |
| String conversion for C | `simp_string_cstr` accepts a captured String slot; `simp_string_bytes` accepts a managed String object and returns borrowed bytes/length |

All package functions take a reserved first receiver argument: **pass `NULL`**.
The native implementations do not inspect it. This is not an arbitrary Simple
method-call ABI. Pure Simple wrapper methods, constructors, `Url`, and wrapper
state management have no C bridge in this MVP; use them from Simple, or use
the listed native primitives and manage their resource state explicitly.
General object construction, GC control, reflection/layout metadata, and
private runtime functions are not part of this facade.

```simp
start {
    // test: {"stdout": "<WORK_DIR>\n3"}
    float root = 0.0
    strg directory
    inline (float root, strg directory) {
        *root = simp_math_sqrt(NULL, 9.0);
        *directory = simp_fs_get_cwd(NULL);
        printf("%s\n", simp_string_cstr(directory));
    }
    print(root) // 3
}
```

### Values, ownership, and errors

- Simple `int`/`unsigned` map to `int64_t`/`uint64_t`, `float` to `double`.
  Native boolean results are `int32_t` (zero/one); a captured Simple boolean
  is `_Bool *`. String/list/buffer API arguments and results are opaque
  managed object pointers, **not C strings or private structs**. Pass `*text`,
  `*values`, or `*bytes` from the respective capture. Captured references are
  mutable slots (`void **`, or `SimpBuffer **` for buffers).
- A managed return value must be stored **directly in a captured Simple slot
  before any further allocating call**. C automatic variables are not GC
  roots. Keep all managed inputs in captured/rooted slots for the entire call;
  the receiver roots fields, including inherited/base-qualified fields. The
  collector is non-moving, but an unrooted result may still be reclaimed.
  Do not `free` managed objects or store `malloc`/libc pointers in their slots.
  To inspect or manipulate a returned collection or object, return to Simple
  and use its public methods rather than accessing its layout.
- `simp_string_cstr(slot)` copies into a per-inline-block temporary arena;
  the NUL-terminated result expires when the shim returns. Do not free or
  retain it. `simp_string_bytes(object, &bytes, &length)` borrows a byte range
  that is not necessarily NUL-terminated, is invalidated by resizing, and
  must not outlive the rooted object. Embedded NUL bytes are preserved in
  managed strings; APIs requiring a C string may reject or truncate them.
- File/process/synchronization returns are **unmanaged opaque handles**;
  store them in `handle` captures, close/release explicitly, and clear the
  slot afterward. Networking uses integer file descriptors; close them with
  the matching socket function. Do not mix resource kinds or reuse closed
  handles. Wrapper objects do not automatically take ownership of raw C
  handles. Semaphore creation requires a nonnegative initial count; misuse
  follows the documented native abort behavior.
- Failure sentinels and `System.lastError()` behavior are the same as for
  the corresponding package operations documented above. In C use
  `simp_system_last_error(NULL)` and store its managed String result in a
  capture. Retrieve it promptly, before another operation changes the
  thread-local error. Network failures use return sentinels, not that error
  channel; math uses C math-library results. **Libc `errno` is separate** and
  is not automatically converted to `lastError()`.
- Use these APIs on the already-registered Simple thread executing the inline
  shim. Arbitrary external C threads/standalone managed allocations are not
  supported by this facade. Blocking package APIs use the existing runtime
  lock protocol; do not replace them with raw blocking calls when Simple
  thread progress is required. Raw C allocations and resources remain outside
  GC ownership. Do not use C `return`/`longjmp` to bypass shim cleanup.

### Headers and platforms

The shim also supplies `stdlib.h`, `stdio.h`, `string.h`, `errno.h`, `ctype.h`,
`stdint.h`, `limits.h`, and `unistd.h`, once before all inline bodies. These
are platform libc APIs, distinct from the Simple standard library. Developers
may use both surfaces without obtaining runtime source. Other private runtime
declarations still present for legacy capture compatibility are not a public
API contract, and the facade itself includes no private headers or structs.

`unistd.h` is **POSIX-only**. The compiler/runtime currently requires a POSIX
native host/target, C11/C++17, Clang, and pthreads. CMake rejects a missing
`unistd.h`; Clang emits a compilation diagnostic if a required target header
is unavailable. No required header is silently skipped, and non-POSIX/cross
targets are not promised. Optional POSIX functions may require explicit
feature-test macros in the toolchain configuration; this header list does not
promise every libc extension. Applications normally need only the installed
compiler, public headers, standard package artifacts, and runtime archive.
Install tests relocate those artifacts and compile/run both an inline
application and a standalone scalar C facade smoke test with no private
headers on its include path.

## Native implementation helpers

The package namespaces also expose `Runtime` classes used by the wrappers
above. Their methods are public in the shipped Simple sources, but are
low-level Simple implementation details; prefer the wrapper APIs. The deliberate
C surface above is supported independently; other implementation classes,
private layouts, and bindings are not covered by that contract.

`Sys.Runtime` file operations:

```text
handle fileOpen(String path, String mode)
String fileRead(handle fp, int size)
String fileReadAll(handle fp)
String fileReadLine(handle fp)
list fileReadLines(handle fp)
int fileWrite(handle fp, String data)
int fileWriteLine(handle fp, String line)
int fileSeek(handle fp, int offset, int whence)
int fileTell(handle fp)
void fileFlush(handle fp)
void fileClose(handle fp)
bool fileEof(handle fp)
```

`Net.Runtime` socket and server operations:

```text
int socketCreate()
bool socketConnect(int fd, String host, int port)
int socketSend(int fd, buffer data)
int socketSendString(int fd, String data)
buffer socketRecv(int fd, int maxBytes)
String socketRecvString(int fd, int maxBytes)
void socketClose(int fd)
void socketSetTimeout(int fd, int milliseconds)
int serverBind(String host, int port, int backlog)
int serverAccept(int fd)
int serverPort(int fd)
bool serverListen(int fd, int backlog)
```

`P.Runtime` process operations:

```text
handle spawn(String executable, list arguments)
bool wait(handle process)
int exitCode(handle process)
String stdout(handle process)
String stderr(handle process)
void close(handle process)
```

`Sync.Runtime` synchronization operations:

```text
handle mutexCreate()
bool mutexLock(handle mutex)
bool mutexUnlock(handle mutex)
bool mutexRelease(handle mutex)
handle conditionCreate()
bool conditionWait(handle condition, handle mutex)
bool conditionSignal(handle condition)
bool conditionBroadcast(handle condition)
bool conditionRelease(handle condition)
handle semaphoreCreate(int initialCount)
void semaphoreWait(handle semaphore)
void semaphoreSignal(handle semaphore)
void semaphoreRelease(handle semaphore)
```
