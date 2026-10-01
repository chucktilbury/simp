/**
 * @file RuntimeStdlib.c
 * @brief Native implementations for the standard library packages.
 */
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include "simp/RuntimeGc.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

extern const SimpClassMeta simp_string_class_meta __attribute__((weak));
static const char stdlib_native_file[] = "<stdlib>";

static int process_argc = 0;
static char **process_argv = NULL;

void simp_runtime_init_args(int32_t argc, char **argv) {
    process_argc = argc < 0 ? 0 : argc;
    process_argv = argv;
}

static char *copy_string_bytes(void *object) {
    if (object == NULL) return NULL;
    const char *bytes;
    uint64_t length;
    simp_string_bytes(object, &bytes, &length);
    if (length > SIZE_MAX - 1) return NULL;
    char *result = (char *)malloc((size_t)length + 1);
    if (result == NULL) return NULL;
    if (length != 0) memcpy(result, bytes, (size_t)length);
    result[length] = '\0';
    return result;
}

static void *simple_string(const char *bytes, uint64_t length) {
    if (&simp_string_class_meta == NULL) abort();
    return simp_string_new(&simp_string_class_meta, bytes, length);
}

static void *simple_string_from_cstr(const char *text) {
    if (text == NULL) return simple_string(NULL, 0);
    return simple_string(text, (uint64_t)strlen(text));
}

static void *new_array(void) {
    return simp_gc_alloc_array(0);
}

static void append_array_object(SimpArray *array, void *object) {
    if (array == NULL || array->length >= INT32_MAX) abort();
    if (array->length == array->capacity) {
        uint64_t capacity = array->capacity == 0 ? 1 : array->capacity * 2;
        if (capacity > INT32_MAX) capacity = INT32_MAX;
        if (capacity > SIZE_MAX / sizeof(*array->values)) abort();
        SimpArrayValue *values = (SimpArrayValue *)realloc(
            array->values, (size_t)capacity * sizeof(*array->values));
        if (values == NULL) abort();
        array->values = values;
        array->capacity = capacity;
    }
    array->values[array->length++] =
        (SimpArrayValue){SIMP_ARRAY_OBJECT, 0, object, 0};
}

int32_t simp_system_argc(void *self) {
    (void)self;
    return process_argc;
}

void *simp_system_argv(void *self) {
    (void)self;
    SimpArray *array = (SimpArray *)new_array();
    void *root = array;
    SimpRootFrame frame;
    void *slots[] = {&root};
    simp_gc_push_or_abort(&frame, slots, 1);
    for (int32_t index = 0; index < process_argc; ++index) {
        const char *argument = process_argv == NULL || process_argv[index] == NULL
                                   ? ""
                                   : process_argv[index];
        void *value = simple_string_from_cstr(argument);
        append_array_object((SimpArray *)root, value);
    }
    simp_gc_pop_or_abort(&frame);
    return root;
}

void *simp_system_arg(void *self, int32_t index) {
    (void)self;
    if (index < 0 || index >= process_argc || process_argv == NULL ||
        process_argv[index] == NULL) {
        return simple_string(NULL, 0);
    }
    return simple_string_from_cstr(process_argv[index]);
}

void *simp_system_getenv(void *self, void *name) {
    (void)self;
    char *key = copy_string_bytes(name);
    if (key == NULL) return simple_string(NULL, 0);
    const char *value = getenv(key);
    void *result = simple_string_from_cstr(value);
    free(key);
    return result;
}

int32_t simp_system_setenv(void *self, void *name, void *value) {
    (void)self;
    char *key = copy_string_bytes(name);
    char *text = copy_string_bytes(value);
    if (key == NULL || text == NULL || key[0] == '\0') {
        free(key);
        free(text);
        return 0;
    }
    const int result = setenv(key, text, 1);
    free(key);
    free(text);
    return result == 0;
}

int32_t simp_fs_exists(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    struct stat info;
    const int result = stat(name, &info) == 0;
    free(name);
    return result;
}

int32_t simp_fs_is_file(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    struct stat info;
    const int result = stat(name, &info) == 0 && S_ISREG(info.st_mode);
    free(name);
    return result;
}

int32_t simp_fs_is_dir(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    struct stat info;
    const int result = stat(name, &info) == 0 && S_ISDIR(info.st_mode);
    free(name);
    return result;
}

int32_t simp_fs_file_size(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return -1;
    struct stat info;
    const int result = stat(name, &info) == 0 && S_ISREG(info.st_mode)
                           ? (info.st_size > INT32_MAX ? INT32_MAX
                                                       : (int32_t)info.st_size)
                           : -1;
    free(name);
    return result;
}

int32_t simp_fs_remove(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    const int result = remove(name) == 0;
    free(name);
    return result;
}

int32_t simp_fs_rename(void *self, void *old_path, void *new_path) {
    (void)self;
    char *old_name = copy_string_bytes(old_path);
    char *new_name = copy_string_bytes(new_path);
    if (old_name == NULL || new_name == NULL) {
        free(old_name);
        free(new_name);
        return 0;
    }
    const int result = rename(old_name, new_name) == 0;
    free(old_name);
    free(new_name);
    return result;
}

int32_t simp_fs_copy(void *self, void *source, void *destination) {
    (void)self;
    char *source_name = copy_string_bytes(source);
    char *destination_name = copy_string_bytes(destination);
    if (source_name == NULL || destination_name == NULL) {
        free(source_name);
        free(destination_name);
        return 0;
    }
    FILE *input = fopen(source_name, "rb");
    FILE *output = input == NULL ? NULL : fopen(destination_name, "wb");
    int success = input != NULL && output != NULL;
    char buffer[16384];
    while (success) {
        const size_t count = fread(buffer, 1, sizeof(buffer), input);
        if (count != 0 && fwrite(buffer, 1, count, output) != count) success = 0;
        if (count < sizeof(buffer)) {
            if (ferror(input)) success = 0;
            break;
        }
    }
    if (input != NULL && fclose(input) != 0) success = 0;
    if (output != NULL && fclose(output) != 0) success = 0;
    free(source_name);
    free(destination_name);
    return success;
}

int32_t simp_fs_mkdir(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    const int result = mkdir(name, 0777) == 0;
    free(name);
    return result;
}

int32_t simp_fs_rmdir(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    const int result = rmdir(name) == 0;
    free(name);
    return result;
}

void *simp_fs_list_dir(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return new_array();
    DIR *directory = opendir(name);
    free(name);
    SimpArray *array = (SimpArray *)new_array();
    void *root = array;
    SimpRootFrame frame;
    void *slots[] = {&root};
    simp_gc_push_or_abort(&frame, slots, 1);
    if (directory != NULL) {
        struct dirent *entry;
        while ((entry = readdir(directory)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            void *value = simple_string_from_cstr(entry->d_name);
            append_array_object((SimpArray *)root, value);
        }
        closedir(directory);
    }
    simp_gc_pop_or_abort(&frame);
    return root;
}

void *simp_fs_get_cwd(void *self) {
    (void)self;
    size_t capacity = 256;
    for (;;) {
        char *path = (char *)malloc(capacity);
        if (path == NULL) return simple_string(NULL, 0);
        if (getcwd(path, capacity) != NULL) {
            void *result = simple_string_from_cstr(path);
            free(path);
            return result;
        }
        free(path);
        if (errno != ERANGE || capacity > SIZE_MAX / 2) return simple_string(NULL, 0);
        capacity *= 2;
    }
}

int32_t simp_fs_ch_dir(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    const int result = chdir(name) == 0;
    free(name);
    return result;
}

void *simp_fs_absolute_path(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return simple_string(NULL, 0);
    char *resolved = realpath(name, NULL);
    free(name);
    if (resolved == NULL) return simple_string(NULL, 0);
    void *result = simple_string_from_cstr(resolved);
    free(resolved);
    return result;
}

void *simp_file_open(void *self, void *path, void *mode) {
    (void)self;
    char *name = copy_string_bytes(path);
    char *open_mode = copy_string_bytes(mode);
    if (name == NULL || open_mode == NULL) {
        free(name);
        free(open_mode);
        return NULL;
    }
    FILE *file = fopen(name, open_mode);
    free(name);
    free(open_mode);
    return file;
}

void *simp_file_read(void *self, void *handle, int32_t size) {
    (void)self;
    if (handle == NULL || size <= 0) return simple_string(NULL, 0);
    char *bytes = (char *)malloc((size_t)size);
    if (bytes == NULL) return simple_string(NULL, 0);
    const size_t count = fread(bytes, 1, (size_t)size, (FILE *)handle);
    void *result = simple_string(bytes, count);
    free(bytes);
    return result;
}

static void *read_all(FILE *file) {
    char *bytes = NULL;
    size_t length = 0;
    size_t capacity = 0;
    char chunk[16384];
    for (;;) {
        const size_t count = fread(chunk, 1, sizeof(chunk), file);
        if (count != 0) {
            if (count > (size_t)INT32_MAX - length) {
                free(bytes);
                return simple_string(NULL, 0);
            }
            const size_t required = length + count;
            if (required > capacity) {
                size_t grown = capacity == 0 ? sizeof(chunk) : capacity;
                while (grown < required) grown = grown > (size_t)INT32_MAX / 2
                                                     ? (size_t)INT32_MAX
                                                     : grown * 2;
                char *grown_bytes = (char *)realloc(bytes, grown);
                if (grown_bytes == NULL) {
                    free(bytes);
                    return simple_string(NULL, 0);
                }
                bytes = grown_bytes;
                capacity = grown;
            }
            memcpy(bytes + length, chunk, count);
            length += count;
        }
        if (count < sizeof(chunk)) break;
    }
    void *result = simple_string(bytes, length);
    free(bytes);
    return result;
}

void *simp_file_read_all(void *self, void *handle) {
    (void)self;
    return handle == NULL ? simple_string(NULL, 0) : read_all((FILE *)handle);
}

static int read_line_bytes(FILE *file, char **line, size_t *length) {
    size_t capacity = 128;
    size_t used = 0;
    char *bytes = (char *)malloc(capacity);
    if (bytes == NULL) return 0;
    int character;
    while ((character = fgetc(file)) != EOF) {
        if (character == '\n') break;
        if (used == (size_t)INT32_MAX) break;
        if (used == capacity) {
            const size_t grown = capacity > (size_t)INT32_MAX / 2
                                     ? (size_t)INT32_MAX
                                     : capacity * 2;
            char *grown_bytes = (char *)realloc(bytes, grown);
            if (grown_bytes == NULL) {
                free(bytes);
                return 0;
            }
            bytes = grown_bytes;
            capacity = grown;
        }
        bytes[used++] = (char)character;
    }
    if (used > 0 && bytes[used - 1] == '\r') --used;
    if (character == EOF && used == 0) {
        free(bytes);
        return 0;
    }
    *line = bytes;
    *length = used;
    return 1;
}

void *simp_file_read_line(void *self, void *handle) {
    (void)self;
    if (handle == NULL) return simple_string(NULL, 0);
    char *line;
    size_t length;
    if (!read_line_bytes((FILE *)handle, &line, &length)) return simple_string(NULL, 0);
    void *result = simple_string(line, length);
    free(line);
    return result;
}

void *simp_file_read_lines(void *self, void *handle) {
    (void)self;
    SimpArray *array = (SimpArray *)new_array();
    void *root = array;
    SimpRootFrame frame;
    void *slots[] = {&root};
    simp_gc_push_or_abort(&frame, slots, 1);
    if (handle != NULL) {
        char *line;
        size_t length;
        while (read_line_bytes((FILE *)handle, &line, &length)) {
            void *value = simple_string(line, length);
            free(line);
            append_array_object((SimpArray *)root, value);
        }
    }
    simp_gc_pop_or_abort(&frame);
    return root;
}

int32_t simp_file_write(void *self, void *handle, void *data) {
    (void)self;
    if (handle == NULL || data == NULL) return 0;
    const char *bytes;
    uint64_t length;
    simp_string_bytes(data, &bytes, &length);
    const size_t written = fwrite(bytes, 1, (size_t)length, (FILE *)handle);
    return written > INT32_MAX ? INT32_MAX : (int32_t)written;
}

int32_t simp_file_write_line(void *self, void *handle, void *line) {
    (void)self;
    const int32_t written = simp_file_write(self, handle, line);
    if (handle == NULL || line == NULL) return written;
    const int newline = fputc('\n', (FILE *)handle);
    return newline == EOF || written == INT32_MAX ? written : written + 1;
}

int32_t simp_file_seek(void *self, void *handle, int32_t offset, int32_t whence) {
    (void)self;
    return handle == NULL ? -1 : fseek((FILE *)handle, (long)offset, whence);
}

int32_t simp_file_tell(void *self, void *handle) {
    (void)self;
    if (handle == NULL) return -1;
    const long position = ftell((FILE *)handle);
    return position < 0 ? -1 : position > INT32_MAX ? INT32_MAX : (int32_t)position;
}

void simp_file_flush(void *self, void *handle) {
    (void)self;
    if (handle != NULL) (void)fflush((FILE *)handle);
}

void simp_file_close(void *self, void *handle) {
    (void)self;
    if (handle != NULL) (void)fclose((FILE *)handle);
}

int32_t simp_file_eof(void *self, void *handle) {
    (void)self;
    return handle != NULL && feof((FILE *)handle);
}

double simp_math_abs(void *self, double x) { (void)self; return fabs(x); }
int32_t simp_math_abs_int(void *self, int32_t x) {
    (void)self;
    return x == INT32_MIN ? INT32_MAX : (x < 0 ? -x : x);
}
double simp_math_min(void *self, double a, double b) { (void)self; return fmin(a, b); }
double simp_math_max(void *self, double a, double b) { (void)self; return fmax(a, b); }
int32_t simp_math_min_int(void *self, int32_t a, int32_t b) { (void)self; return a < b ? a : b; }
int32_t simp_math_max_int(void *self, int32_t a, int32_t b) { (void)self; return a > b ? a : b; }
double simp_math_clamp(void *self, double x, double min_value, double max_value) {
    (void)self;
    if (min_value > max_value) {
        const double temporary = min_value;
        min_value = max_value;
        max_value = temporary;
    }
    return fmin(fmax(x, min_value), max_value);
}
double simp_math_floor(void *self, double x) { (void)self; return floor(x); }
double simp_math_ceil(void *self, double x) { (void)self; return ceil(x); }
double simp_math_round(void *self, double x) { (void)self; return round(x); }
double simp_math_trunc(void *self, double x) { (void)self; return trunc(x); }
double simp_math_sqrt(void *self, double x) { (void)self; return sqrt(x); }
double simp_math_cbrt(void *self, double x) { (void)self; return cbrt(x); }
double simp_math_pow(void *self, double base, double exponent) { (void)self; return pow(base, exponent); }
double simp_math_exp(void *self, double x) { (void)self; return exp(x); }
double simp_math_log(void *self, double x) { (void)self; return log(x); }
double simp_math_log10(void *self, double x) { (void)self; return log10(x); }
double simp_math_log2(void *self, double x) { (void)self; return log2(x); }
double simp_math_sin(void *self, double x) { (void)self; return sin(x); }
double simp_math_cos(void *self, double x) { (void)self; return cos(x); }
double simp_math_tan(void *self, double x) { (void)self; return tan(x); }
double simp_math_asin(void *self, double x) { (void)self; return asin(x); }
double simp_math_acos(void *self, double x) { (void)self; return acos(x); }
double simp_math_atan(void *self, double x) { (void)self; return atan(x); }
double simp_math_atan2(void *self, double y, double x) { (void)self; return atan2(y, x); }
double simp_math_degrees(void *self, double radians) {
    (void)self;
    return radians * (180.0 / 3.14159265358979323846);
}
double simp_math_radians(void *self, double degrees) {
    (void)self;
    return degrees * (3.14159265358979323846 / 180.0);
}
double simp_math_sinh(void *self, double x) { (void)self; return sinh(x); }
double simp_math_cosh(void *self, double x) { (void)self; return cosh(x); }
double simp_math_tanh(void *self, double x) { (void)self; return tanh(x); }

static int socket_fd(int32_t fd) { return fd; }

static int32_t native_send(int fd, const void *bytes, size_t length) {
#ifdef MSG_NOSIGNAL
    const ssize_t result = send(fd, bytes, length, MSG_NOSIGNAL);
#else
    const ssize_t result = send(fd, bytes, length, 0);
#endif
    return result < 0 ? -1 : result > INT32_MAX ? INT32_MAX : (int32_t)result;
}

int32_t simp_net_socket_create(void *self) {
    (void)self;
    return socket(AF_INET, SOCK_STREAM, 0);
}

int32_t simp_net_socket_connect(void *self, int32_t fd, void *host, int32_t port) {
    (void)self;
    char *hostname = copy_string_bytes(host);
    if (hostname == NULL || port < 0 || port > 65535) {
        free(hostname);
        return 0;
    }
    char service[6];
    (void)snprintf(service, sizeof(service), "%d", port);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *addresses = NULL;
    const int lookup = getaddrinfo(hostname, service, &hints, &addresses);
    free(hostname);
    if (lookup != 0) return 0;
    int connected = 0;
    for (struct addrinfo *address = addresses; address != NULL; address = address->ai_next) {
        if (connect(socket_fd(fd), address->ai_addr, address->ai_addrlen) == 0) {
            connected = 1;
            break;
        }
    }
    freeaddrinfo(addresses);
    return connected;
}

int32_t simp_net_socket_send(void *self, int32_t fd, void *buffer) {
    (void)self;
    if (buffer == NULL) return -1;
    const SimpBuffer *data = (const SimpBuffer *)buffer;
    return native_send(socket_fd(fd), data->data, (size_t)data->length);
}

int32_t simp_net_socket_send_string(void *self, int32_t fd, void *text) {
    (void)self;
    if (text == NULL) return -1;
    const char *bytes;
    uint64_t length;
    simp_string_bytes(text, &bytes, &length);
    return native_send(socket_fd(fd), bytes, (size_t)length);
}

void *simp_net_socket_recv(void *self, int32_t fd, int32_t max_bytes) {
    (void)self;
    if (max_bytes < 0 || max_bytes > INT32_MAX) return NULL;
    char *bytes = max_bytes == 0 ? NULL : (char *)malloc((size_t)max_bytes);
    if (max_bytes != 0 && bytes == NULL) return NULL;
    const ssize_t received = recv(socket_fd(fd), bytes, (size_t)max_bytes, 0);
    if (received < 0) {
        free(bytes);
        return NULL;
    }
    void *result = simp_buffer_new((int32_t)received, stdlib_native_file,
                                   sizeof(stdlib_native_file) - 1, 0, 0);
    SimpBuffer *buffer = (SimpBuffer *)result;
    if (received != 0) memcpy(buffer->data, bytes, (size_t)received);
    free(bytes);
    return result;
}

void *simp_net_socket_recv_string(void *self, int32_t fd, int32_t max_bytes) {
    (void)self;
    if (max_bytes < 0) return simple_string(NULL, 0);
    char *bytes = max_bytes == 0 ? NULL : (char *)malloc((size_t)max_bytes);
    if (max_bytes != 0 && bytes == NULL) return simple_string(NULL, 0);
    const ssize_t received = recv(socket_fd(fd), bytes, (size_t)max_bytes, 0);
    if (received < 0) {
        free(bytes);
        return simple_string(NULL, 0);
    }
    void *result = simple_string(bytes, (uint64_t)received);
    free(bytes);
    return result;
}

void simp_net_socket_close(void *self, int32_t fd) {
    (void)self;
    if (fd >= 0) (void)close(socket_fd(fd));
}

void simp_net_socket_set_timeout(void *self, int32_t fd, int32_t milliseconds) {
    (void)self;
    if (fd < 0 || milliseconds < 0) return;
    struct timeval timeout;
    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000;
    (void)setsockopt(socket_fd(fd), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

int32_t simp_net_server_bind(void *self, void *host, int32_t port, int32_t backlog) {
    (void)self;
    char *hostname = copy_string_bytes(host);
    if (hostname == NULL || port < 0 || port > 65535 || backlog < 0) {
        free(hostname);
        return -1;
    }
    char service[6];
    (void)snprintf(service, sizeof(service), "%d", port);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *addresses = NULL;
    const int lookup = getaddrinfo(hostname[0] == '\0' ? NULL : hostname,
                                   service, &hints, &addresses);
    free(hostname);
    if (lookup != 0) return -1;
    int server_fd = -1;
    for (struct addrinfo *address = addresses; address != NULL; address = address->ai_next) {
        const int candidate = socket(address->ai_family, address->ai_socktype,
                                     address->ai_protocol);
        if (candidate < 0) continue;
        int reuse = 1;
        (void)setsockopt(candidate, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        if (bind(candidate, address->ai_addr, address->ai_addrlen) == 0 &&
            listen(candidate, backlog > 0 ? backlog : 16) == 0) {
            server_fd = candidate;
            break;
        }
        (void)close(candidate);
    }
    freeaddrinfo(addresses);
    return server_fd;
}

int32_t simp_net_server_accept(void *self, int32_t server_fd) {
    (void)self;
    return accept(socket_fd(server_fd), NULL, NULL);
}

int32_t simp_net_server_listen(void *self, int32_t server_fd, int32_t backlog) {
    (void)self;
    if (server_fd < 0 || backlog < 0) return 0;
    return listen(socket_fd(server_fd), backlog > 0 ? backlog : 16) == 0;
}

int32_t simp_net_server_port(void *self, int32_t server_fd) {
    (void)self;
    struct sockaddr_in address;
    socklen_t length = sizeof(address);
    if (server_fd < 0 ||
        getsockname(socket_fd(server_fd), (struct sockaddr *)&address, &length) != 0) {
        return 0;
    }
    return ntohs(address.sin_port);
}
