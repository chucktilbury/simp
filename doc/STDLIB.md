# Standard library reference

Standard packages are imported by package name. The alias itself names the
package namespace; use the exported classes directly beneath it. For example,
`import system as Sys` makes `Sys.Process` and `Sys.System` the class names
(not `Sys.System.Process`):

```simp
import system as Sys
import math as MathLib

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

## `system`

Import with `import system as Sys`; the alias names the package namespace, so
the exported classes are available directly as `Sys.Process`, `Sys.File`,
`Sys.FileSystem`, `Sys.StandardIO`, and `Sys.System`.

`Sys.Process` provides command-line and environment access:

```text
int argc()
array argv()
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
array readLines()
int write(String data)
int writeLine(String line)
int seek(int offset, int whence)
int tell()
void flush()
void close()
```

`readLine` removes the line ending (including a preceding carriage return);
`readLines` returns lines as an array. End-of-file reads return empty
string/array results. Invalid handles and I/O failures also return empty
string/array or the operation's failure value and set `lastError()`.
`write`/`writeLine` return the number of bytes written (including the newline
for `writeLine` when successful); failed writes may return a partial count.
`seek` returns zero on success and `-1` on failure; `tell` returns the
position, capped at the `int` range, or `-1`. `File` records whether the
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
array listDir(String path)
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
listing returns an empty array, and path-producing operations return an empty
string on failure. These failures set `lastError()`; successful filesystem
calls clear it. `absolutePath` resolves an existing path with `realpath`.
`normalize` is lexical and does not resolve symbolic links. `tempFile` creates
a file with mode `0600` and closes its descriptor before returning the path;
`tempDir` creates a temporary directory.

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

start {
    T.Clock clock = T.Clock()
    unsigned before = clock.monotonicMilliseconds()
    print(clock.epochSeconds() > 0u)
    print(clock.monotonicMilliseconds() >= before)
}
```

## `process`

Import with `import process as P`; the alias names the package namespace, so
the exported class is available directly as `P.Process`.

```text
Process(String executable, array arguments)
bool started()
bool wait()
int exitCode()
String stdout()
String stderr()
void close()
```

The argument array contains arguments after argv[0]; the executable is
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

start {
    Sync.Mutex lock = Sync.Mutex()
    print(lock.lock())
    print(lock.unlock())
    print(lock.close())
}
```
