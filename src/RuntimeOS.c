/**
 * @file RuntimeOS.c
 * @brief POSIX standard I/O, paths, clocks, terminal, and random facilities.
 */
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include "simp/RuntimeGc.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/random.h>
#endif

extern const SimpClassMeta simp_string_class_meta __attribute__((weak));

static _Thread_local char simp_last_error[256];

void simp_runtime_set_error(int error_number) {
    if (error_number == 0) error_number = EIO;
    /* strerror() returns a pointer to shared/static storage and is not
     * guaranteed reentrant or thread-safe across platforms. Use the
     * reentrant POSIX strerror_r() and always fall back to a non-empty
     * message so lastError() can never observe an empty string after an
     * error was set, regardless of libc/platform differences. */
    char buffer[sizeof(simp_last_error)];
    const int saved_errno = errno;
    const int result = strerror_r(error_number, buffer, sizeof(buffer));
    errno = saved_errno;
    if (result == 0 && buffer[0] != '\0') {
        (void)snprintf(simp_last_error, sizeof(simp_last_error), "%s", buffer);
    } else {
        (void)snprintf(simp_last_error, sizeof(simp_last_error),
                       "operating system error %d", error_number);
    }
}

void simp_runtime_clear_error(void) {
    simp_last_error[0] = '\0';
}

void *simp_system_last_error(void *self) {
    (void)self;
    if (&simp_string_class_meta == NULL) abort();
    return simp_string_new(&simp_string_class_meta, simp_last_error,
                           (uint64_t)strlen(simp_last_error));
}

static char *native_string(void *object) {
    if (object == NULL) {
        simp_runtime_set_error(EINVAL);
        return NULL;
    }
    const char *bytes;
    uint64_t length;
    simp_string_bytes(object, &bytes, &length);
    if (length > SIZE_MAX - 1 || memchr(bytes, '\0', (size_t)length) != NULL) {
        simp_runtime_set_error(EINVAL);
        return NULL;
    }
    char *copy = (char *)malloc((size_t)length + 1);
    if (copy == NULL) {
        simp_runtime_set_error(ENOMEM);
        return NULL;
    }
    memcpy(copy, bytes, (size_t)length);
    copy[length] = '\0';
    return copy;
}

static void *make_string(const char *text, size_t length) {
    if (&simp_string_class_meta == NULL) abort();
    return simp_string_new(&simp_string_class_meta, text, length);
}

static int64_t write_stream(FILE *stream, const char *bytes, size_t length) {
    if (length == 0) {
        simp_runtime_clear_error();
        return 0;
    }
    const size_t written = fwrite(bytes, 1, length, stream);
    if (written != length) simp_runtime_set_error(errno == 0 ? EIO : errno);
    else simp_runtime_clear_error();
    if (written > (size_t)INT64_MAX) {
        simp_runtime_set_error(EOVERFLOW);
        return INT64_MAX;
    }
    return (int64_t)written;
}

void *simp_stdio_read(void *self, int64_t size) {
    (void)self;
    if (size < 0) {
        simp_runtime_set_error(EINVAL);
        return make_string("", 0);
    }
    if ((uint64_t)size > SIZE_MAX) {
        simp_runtime_set_error(EOVERFLOW);
        return make_string("", 0);
    }
    char *bytes = size == 0 ? NULL : (char *)malloc((size_t)size);
    if (size != 0 && bytes == NULL) {
        simp_runtime_set_error(ENOMEM);
        return make_string("", 0);
    }
    simp_runtime_gil_release();
    const size_t count = size == 0 ? 0 : fread(bytes, 1, (size_t)size, stdin);
    const int read_error = ferror(stdin) ? (errno == 0 ? EIO : errno) : 0;
    simp_runtime_gil_acquire();
    if (read_error != 0) simp_runtime_set_error(read_error);
    else simp_runtime_clear_error();
    void *result = make_string(bytes == NULL ? "" : bytes, count);
    free(bytes);
    return result;
}

void *simp_stdio_read_line(void *self) {
    (void)self;
    size_t length = 0;
    size_t capacity = 128;
    char *bytes = (char *)malloc(capacity);
    if (bytes == NULL) {
        simp_runtime_set_error(ENOMEM);
        return make_string("", 0);
    }
    int allocation_error = 0;
    simp_runtime_gil_release();
    int character;
    while ((character = fgetc(stdin)) != EOF) {
        if (character == '\n') break;
        if (length == (size_t)INT64_MAX) {
            /* Leave the loop so the GIL is reacquired before reporting the
             * error or allocating the managed result string. */
            allocation_error = EOVERFLOW;
            break;
        }
        if (length == capacity) {
            const size_t grown = capacity > (size_t)INT64_MAX / 2
                                     ? (size_t)INT64_MAX
                                     : capacity * 2;
            char *next = (char *)realloc(bytes, grown);
            if (next == NULL) {
                allocation_error = ENOMEM;
                break;
            }
            bytes = next;
            capacity = grown;
        }
        bytes[length++] = (char)character;
    }
    const int read_error = ferror(stdin) ? (errno == 0 ? EIO : errno) : 0;
    simp_runtime_gil_acquire();
    if (allocation_error != 0) {
        simp_runtime_set_error(allocation_error);
        free(bytes);
        return make_string("", 0);
    }
    if (read_error != 0) simp_runtime_set_error(read_error);
    else simp_runtime_clear_error();
    if (length != 0 && bytes[length - 1] == '\r') --length;
    void *result = make_string(bytes, length);
    free(bytes);
    return result;
}

int64_t simp_stdio_write(void *self, void *text) {
    (void)self;
    if (text == NULL) {
        simp_runtime_set_error(EINVAL);
        return -1;
    }
    const char *bytes;
    uint64_t length;
    simp_string_bytes(text, &bytes, &length);
    return write_stream(stdout, bytes, (size_t)length);
}

static int64_t write_line(FILE *stream, void *text) {
    if (text == NULL) {
        simp_runtime_set_error(EINVAL);
        return -1;
    }
    const char *bytes;
    uint64_t length;
    simp_string_bytes(text, &bytes, &length);
    const size_t written = length == 0 ? 0 : fwrite(bytes, 1, (size_t)length, stream);
    if (written != length || fputc('\n', stream) == EOF) {
        simp_runtime_set_error(errno == 0 ? EIO : errno);
        return written > (size_t)INT64_MAX ? INT64_MAX : (int64_t)written;
    }
    if (length >= (uint64_t)INT64_MAX) {
        simp_runtime_set_error(EOVERFLOW);
        return INT64_MAX;
    }
    simp_runtime_clear_error();
    return (int64_t)length + 1;
}

int64_t simp_stdio_write_line(void *self, void *text) {
    (void)self;
    return write_line(stdout, text);
}

int64_t simp_stdio_write_bytes(void *self, void *object) {
    (void)self;
    if (object == NULL) {
        simp_runtime_set_error(EINVAL);
        return -1;
    }
    const SimpBuffer *buffer = (const SimpBuffer *)object;
    return write_stream(stdout, (const char *)buffer->data,
                        (size_t)buffer->length);
}

int64_t simp_stdio_write_error(void *self, void *text) {
    (void)self;
    if (text == NULL) {
        simp_runtime_set_error(EINVAL);
        return -1;
    }
    const char *bytes;
    uint64_t length;
    simp_string_bytes(text, &bytes, &length);
    return write_stream(stderr, bytes, (size_t)length);
}

int64_t simp_stdio_write_error_line(void *self, void *text) {
    (void)self;
    return write_line(stderr, text);
}

int64_t simp_stdio_write_error_bytes(void *self, void *object) {
    (void)self;
    if (object == NULL) {
        simp_runtime_set_error(EINVAL);
        return -1;
    }
    const SimpBuffer *buffer = (const SimpBuffer *)object;
    return write_stream(stderr, (const char *)buffer->data,
                        (size_t)buffer->length);
}

void simp_stdio_flush(void *self) {
    (void)self;
    if (fflush(stdout) != 0) simp_runtime_set_error(errno);
    else simp_runtime_clear_error();
}

void simp_stdio_flush_error(void *self) {
    (void)self;
    if (fflush(stderr) != 0) simp_runtime_set_error(errno);
    else simp_runtime_clear_error();
}

uint64_t simp_time_epoch_seconds(void *self) {
    (void)self;
    const time_t now = time(NULL);
    if (now == (time_t)-1) {
        simp_runtime_set_error(errno);
        return 0;
    }
    simp_runtime_clear_error();
    return (uint64_t)now;
}

static uint64_t clock_milliseconds(clockid_t clock_id) {
    struct timespec now;
    if (clock_gettime(clock_id, &now) != 0) {
        simp_runtime_set_error(errno);
        return 0;
    }
    simp_runtime_clear_error();
    return (uint64_t)now.tv_sec * UINT64_C(1000) +
           (uint64_t)now.tv_nsec / UINT64_C(1000000);
}

uint64_t simp_time_epoch_milliseconds(void *self) {
    (void)self;
    return clock_milliseconds(CLOCK_REALTIME);
}

uint64_t simp_time_monotonic_milliseconds(void *self) {
    (void)self;
    return clock_milliseconds(CLOCK_MONOTONIC);
}

int32_t simp_time_sleep_milliseconds(void *self, uint64_t milliseconds) {
    (void)self;
    if (milliseconds / UINT64_C(1000) > (uint64_t)INT64_MAX) {
        simp_runtime_set_error(EOVERFLOW);
        return 0;
    }
    const uint64_t seconds = milliseconds / UINT64_C(1000);
    const time_t native_seconds = (time_t)seconds;
    if ((uint64_t)native_seconds != seconds) {
        simp_runtime_set_error(EOVERFLOW);
        return 0;
    }
    struct timespec remaining = {
        native_seconds,
        (long)((milliseconds % UINT64_C(1000)) * UINT64_C(1000000))
    };
    simp_runtime_gil_release();
    while (nanosleep(&remaining, &remaining) != 0) {
        if (errno == EINTR) continue;
        const int saved_error = errno;
        simp_runtime_gil_acquire();
        simp_runtime_set_error(saved_error);
        return 0;
    }
    simp_runtime_gil_acquire();
    simp_runtime_clear_error();
    return 1;
}

int32_t simp_terminal_stdin_interactive(void *self) {
    (void)self;
    return isatty(STDIN_FILENO) != 0;
}

int32_t simp_terminal_stdout_interactive(void *self) {
    (void)self;
    return isatty(STDOUT_FILENO) != 0;
}

static int64_t terminal_dimension(int column) {
    struct winsize dimensions;
    if (isatty(STDOUT_FILENO) == 0 ||
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &dimensions) != 0) {
        return 0;
    }
    return column ? dimensions.ws_col : dimensions.ws_row;
}

int64_t simp_terminal_columns(void *self) {
    (void)self;
    return terminal_dimension(1);
}

int64_t simp_terminal_rows(void *self) {
    (void)self;
    return terminal_dimension(0);
}

int32_t simp_terminal_supports_color(void *self) {
    (void)self;
    const char *term = getenv("TERM");
    return isatty(STDOUT_FILENO) != 0 && getenv("NO_COLOR") == NULL &&
           term != NULL && term[0] != '\0' && strcmp(term, "dumb") != 0;
}

static int secure_random_fill(uint8_t *bytes, size_t length) {
    size_t offset = 0;
#if defined(__linux__)
    while (offset < length) {
        const ssize_t count = getrandom(bytes + offset, length - offset, 0);
        if (count > 0) {
            offset += (size_t)count;
            continue;
        }
        if (count < 0 && errno == EINTR) continue;
        if (count < 0 && errno != ENOSYS && errno != EPERM) return 0;
        break;
    }
    if (offset == length) return 1;
#endif
    const int descriptor = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (descriptor < 0) return 0;
    while (offset < length) {
        const ssize_t count = read(descriptor, bytes + offset, length - offset);
        if (count > 0) offset += (size_t)count;
        else if (count < 0 && errno == EINTR) continue;
        else {
            const int saved = count == 0 ? EIO : errno;
            (void)close(descriptor);
            errno = saved;
            return 0;
        }
    }
    if (close(descriptor) != 0) return 0;
    return 1;
}

int32_t simp_random_fill(void *self, void *object) {
    (void)self;
    if (object == NULL) {
        simp_runtime_set_error(EINVAL);
        return 0;
    }
    SimpBuffer *buffer = (SimpBuffer *)object;
    if (buffer->length > buffer->capacity) {
        simp_runtime_set_error(EINVAL);
        return 0;
    }
    errno = 0;
    if (!secure_random_fill(buffer->data, (size_t)buffer->length)) {
        if (errno == 0) errno = EIO;
        simp_runtime_set_error(errno);
        return 0;
    }
    simp_runtime_clear_error();
    return 1;
}

void *simp_random_bytes(void *self, int64_t size) {
    (void)self;
    if (size < 0) {
        simp_runtime_set_error(EINVAL);
        return NULL;
    }
    if ((uint64_t)size > SIZE_MAX) {
        simp_runtime_set_error(EOVERFLOW);
        return NULL;
    }
    void *buffer = simp_buffer_new(size, "<stdlib>", 8, 0, 0);
    if (buffer == NULL) {
        simp_runtime_set_error(ENOMEM);
        return NULL;
    }
    if (!simp_random_fill(self, buffer)) return NULL;
    return buffer;
}

static void *path_result(char *value) {
    if (value == NULL) {
        simp_runtime_set_error(ENOMEM);
        return make_string("", 0);
    }
    void *result = make_string(value, strlen(value));
    free(value);
    simp_runtime_clear_error();
    return result;
}

static char *normalize_path(const char *input) {
    const size_t length = strlen(input);
    if (length > SIZE_MAX / sizeof(size_t) - 2) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    char *result = (char *)malloc(length + 2);
    size_t *marks = (size_t *)malloc((length + 2) * sizeof(*marks));
    if (result == NULL || marks == NULL) {
        free(result);
        free(marks);
        errno = ENOMEM;
        return NULL;
    }
    const int absolute = input[0] == '/';
    size_t output_length = absolute ? 1 : 0;
    size_t component_count = 0;
    if (absolute) result[0] = '/';
    size_t index = 0;
    while (index < length) {
        while (input[index] == '/') ++index;
        const size_t start = index;
        while (index < length && input[index] != '/') ++index;
        const size_t component_length = index - start;
        if (component_length == 0 ||
            (component_length == 1 && input[start] == '.')) continue;
        if (component_length == 2 && input[start] == '.' &&
            input[start + 1] == '.') {
            if (component_count != 0) {
                const size_t previous_start = marks[component_count - 1];
                const size_t previous_component =
                    previous_start < output_length && result[previous_start] == '/'
                        ? previous_start + 1
                        : previous_start;
                const size_t previous_length =
                    output_length - previous_component;
                if (!(previous_length == 2 &&
                      result[previous_component] == '.' &&
                      result[previous_component + 1] == '.')) {
                    output_length = previous_start;
                    --component_count;
                    continue;
                }
            }
            if (absolute) continue;
        }
        marks[component_count++] = output_length;
        if (output_length != 0 && result[output_length - 1] != '/') {
            result[output_length++] = '/';
        }
        memcpy(result + output_length, input + start, component_length);
        output_length += component_length;
    }
    free(marks);
    if (output_length == 0) result[output_length++] = '.';
    result[output_length] = '\0';
    return result;
}

void *simp_path_normalize(void *self, void *path) {
    (void)self;
    char *input = native_string(path);
    if (input == NULL) return make_string("", 0);
    char *normalized = normalize_path(input);
    free(input);
    if (normalized == NULL) {
        simp_runtime_set_error(errno);
        return make_string("", 0);
    }
    return path_result(normalized);
}

void *simp_path_join(void *self, void *left, void *right) {
    (void)self;
    char *first = native_string(left);
    char *second = native_string(right);
    if (first == NULL || second == NULL) {
        free(first);
        free(second);
        return make_string("", 0);
    }
    if (second[0] == '/') {
        free(first);
        first = second;
    } else {
        const size_t first_length = strlen(first);
        const size_t second_length = strlen(second);
        if (first_length > SIZE_MAX - second_length - 2) {
            free(first);
            free(second);
            simp_runtime_set_error(ENAMETOOLONG);
            return make_string("", 0);
        }
        char *joined = (char *)malloc(first_length + second_length + 2);
        if (joined == NULL) {
            free(first);
            free(second);
            simp_runtime_set_error(ENOMEM);
            return make_string("", 0);
        }
        memcpy(joined, first, first_length);
        size_t at = first_length;
        if (at != 0 && joined[at - 1] != '/') joined[at++] = '/';
        memcpy(joined + at, second, second_length + 1);
        free(first);
        free(second);
        first = joined;
    }
    char *normalized = normalize_path(first);
    free(first);
    if (normalized == NULL) {
        simp_runtime_set_error(errno);
        return make_string("", 0);
    }
    return path_result(normalized);
}

static char *trim_trailing_slashes(char *path) {
    size_t length = strlen(path);
    while (length > 1 && path[length - 1] == '/') path[--length] = '\0';
    return path;
}

void *simp_path_basename(void *self, void *path) {
    (void)self;
    char *value = native_string(path);
    if (value == NULL) return make_string("", 0);
    trim_trailing_slashes(value);
    if (strcmp(value, "/") == 0) return path_result(value);
    char *base = strrchr(value, '/');
    base = base == NULL ? value : base + 1;
    void *result = make_string(base, strlen(base));
    free(value);
    simp_runtime_clear_error();
    return result;
}

void *simp_path_dirname(void *self, void *path) {
    (void)self;
    char *value = native_string(path);
    if (value == NULL) return make_string("", 0);
    trim_trailing_slashes(value);
    char *slash = strrchr(value, '/');
    if (slash == NULL) return path_result(strdup("."));
    if (slash == value) return path_result(strdup("/"));
    *slash = '\0';
    return path_result(value);
}

void *simp_path_extension(void *self, void *path) {
    (void)self;
    char *value = native_string(path);
    if (value == NULL) return make_string("", 0);
    char *base = strrchr(value, '/');
    base = base == NULL ? value : base + 1;
    char *dot = strrchr(base, '.');
    void *result = dot == NULL || dot == base
                       ? make_string("", 0)
                       : make_string(dot + 1, strlen(dot + 1));
    free(value);
    simp_runtime_clear_error();
    return result;
}

static char *temp_directory(void) {
    const char *configured = getenv("TMPDIR");
    if (configured == NULL || configured[0] != '/') configured = "/tmp";
    const size_t length = strlen(configured);
    const char suffix[] = "/simp-XXXXXX";
    char *path = (char *)malloc(length + sizeof(suffix));
    if (path == NULL) return NULL;
    memcpy(path, configured, length);
    if (length != 0 && path[length - 1] == '/') {
        memcpy(path + length, suffix + 1, sizeof(suffix) - 1);
    } else {
        memcpy(path + length, suffix, sizeof(suffix));
    }
    return path;
}

void *simp_fs_temp_file(void *self) {
    (void)self;
    char *path = temp_directory();
    if (path == NULL) {
        simp_runtime_set_error(ENOMEM);
        return make_string("", 0);
    }
    const int descriptor = mkstemp(path);
    if (descriptor < 0) {
        simp_runtime_set_error(errno);
        free(path);
        return make_string("", 0);
    }
    if (close(descriptor) != 0) {
        const int saved = errno;
        (void)unlink(path);
        simp_runtime_set_error(saved);
        free(path);
        return make_string("", 0);
    }
    return path_result(path);
}

void *simp_fs_temp_dir(void *self) {
    (void)self;
    char *path = temp_directory();
    if (path == NULL) {
        simp_runtime_set_error(ENOMEM);
        return make_string("", 0);
    }
    if (mkdtemp(path) == NULL) {
        simp_runtime_set_error(errno);
        free(path);
        return make_string("", 0);
    }
    return path_result(path);
}
