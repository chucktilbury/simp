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
#include <glob.h>
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
extern void simp_runtime_set_error(int error_number);
extern void simp_runtime_clear_error(void);
static const char stdlib_native_file[] = "<stdlib>";

static int process_argc = 0;
static char **process_argv = NULL;

void simp_runtime_init_args(int32_t argc, char **argv) {
    process_argc = argc < 0 ? 0 : argc;
    process_argv = argv;
}

static char *copy_string_bytes(void *object) {
    if (object == NULL) {
        simp_runtime_set_error(EINVAL);
        return NULL;
    }
    const char *bytes;
    uint64_t length;
    simp_string_bytes(object, &bytes, &length);
    if (length > SIZE_MAX - 1) {
        simp_runtime_set_error(EOVERFLOW);
        return NULL;
    }
    if (memchr(bytes, '\0', (size_t)length) != NULL) {
        simp_runtime_set_error(EINVAL);
        return NULL;
    }
    char *result = (char *)malloc((size_t)length + 1);
    if (result == NULL) {
        simp_runtime_set_error(ENOMEM);
        return NULL;
    }
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
    if (array == NULL || array->length >= (uint64_t)INT64_MAX) abort();
    if (array->length == array->capacity) {
        uint64_t capacity = array->capacity == 0 ? 1 :
            array->capacity > (uint64_t)INT64_MAX / 2
                ? (uint64_t)INT64_MAX : array->capacity * 2;
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

static _Thread_local int glob_callback_error_number;

static int glob_error(const char *path, int error_number) {
    (void)path;
    glob_callback_error_number = error_number != 0 ? error_number : EIO;
    return 1;
}

void *simp_glob_glob(void *self, void *pattern) {
    (void)self;
    char *name = copy_string_bytes(pattern);
    SimpArray *array = (SimpArray *)new_array();
    void *root = array;
    SimpRootFrame frame;
    void *slots[] = {&root};
    simp_gc_push_or_abort(&frame, slots, 1);
    if (name == NULL) {
        simp_gc_pop_or_abort(&frame);
        return root;
    }

    glob_t matches = {0};
    glob_callback_error_number = 0;
    errno = 0;
    const int result = glob(name, GLOB_ERR, glob_error, &matches);
    const int saved_error = errno;
    free(name);
    if (result == 0) {
        for (size_t index = 0; index < matches.gl_pathc; ++index) {
            void *value = simple_string_from_cstr(matches.gl_pathv[index]);
            append_array_object((SimpArray *)root, value);
        }
        simp_runtime_clear_error();
    } else if (result == GLOB_NOMATCH) {
        simp_runtime_clear_error();
    } else if (result == GLOB_NOSPACE) {
        simp_runtime_set_error(ENOMEM);
    } else if (result == GLOB_ABORTED) {
        const int error_number = glob_callback_error_number != 0
                                     ? glob_callback_error_number
                                     : saved_error;
        simp_runtime_set_error(error_number == 0 ? EIO : error_number);
    } else {
        simp_runtime_set_error(EIO);
    }
    globfree(&matches);
    simp_gc_pop_or_abort(&frame);
    return root;
}

int64_t simp_system_argc(void *self) {
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
    for (int64_t index = 0; index < process_argc; ++index) {
        const char *argument = process_argv == NULL || process_argv[index] == NULL
                                   ? ""
                                   : process_argv[index];
        void *value = simple_string_from_cstr(argument);
        append_array_object((SimpArray *)root, value);
    }
    simp_gc_pop_or_abort(&frame);
    return root;
}

void *simp_system_arg(void *self, int64_t index) {
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
    simp_runtime_clear_error();
    return result;
}

int32_t simp_system_setenv(void *self, void *name, void *value) {
    (void)self;
    char *key = copy_string_bytes(name);
    char *text = copy_string_bytes(value);
    if (key == NULL || text == NULL || key[0] == '\0') {
        if (key != NULL && key[0] == '\0') simp_runtime_set_error(EINVAL);
        free(key);
        free(text);
        return 0;
    }
    const int result = setenv(key, text, 1);
    const int saved_error = errno;
    free(key);
    free(text);
    if (result == 0) simp_runtime_clear_error();
    else simp_runtime_set_error(saved_error);
    return result == 0;
}

int32_t simp_fs_exists(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    struct stat info;
    const int status = stat(name, &info);
    const int saved_error = errno;
    free(name);
    if (status == 0) simp_runtime_clear_error();
    else simp_runtime_set_error(saved_error);
    return status == 0;
}

int32_t simp_fs_is_file(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    struct stat info;
    const int status = stat(name, &info);
    const int result = status == 0 && S_ISREG(info.st_mode);
    if (result) simp_runtime_clear_error();
    else simp_runtime_set_error(status != 0 ? errno : EINVAL);
    free(name);
    return result;
}

int32_t simp_fs_is_dir(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    struct stat info;
    const int status = stat(name, &info);
    const int result = status == 0 && S_ISDIR(info.st_mode);
    if (result) simp_runtime_clear_error();
    else simp_runtime_set_error(status != 0 ? errno : ENOTDIR);
    free(name);
    return result;
}

int64_t simp_fs_file_size(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return -1;
    struct stat info;
    const int status = stat(name, &info);
    const uintmax_t native_size = status == 0 && S_ISREG(info.st_mode) &&
                                  info.st_size >= 0
                                      ? (uintmax_t)info.st_size : 0;
    const int64_t result = status == 0 && S_ISREG(info.st_mode) &&
                           info.st_size >= 0 &&
                           native_size <= (uintmax_t)INT64_MAX
                               ? (int64_t)native_size : -1;
    if (status == 0 && S_ISREG(info.st_mode) && info.st_size >= 0 &&
        native_size > (uintmax_t)INT64_MAX)
        simp_runtime_set_error(EOVERFLOW);
    else if (result >= 0) simp_runtime_clear_error();
    else simp_runtime_set_error(status != 0 ? errno : EINVAL);
    free(name);
    return result;
}

int32_t simp_fs_remove(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    const int result = remove(name) == 0;
    const int saved_error = errno;
    free(name);
    if (result) simp_runtime_clear_error();
    else simp_runtime_set_error(saved_error);
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
    const int saved_error = errno;
    free(old_name);
    free(new_name);
    if (result) simp_runtime_clear_error();
    else simp_runtime_set_error(saved_error);
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
    struct stat source_info;
    struct stat destination_info;
    int same_file = 0;
    if (input != NULL && fstat(fileno(input), &source_info) == 0 &&
        stat(destination_name, &destination_info) == 0 &&
        source_info.st_dev == destination_info.st_dev &&
        source_info.st_ino == destination_info.st_ino) {
        errno = EINVAL;
        same_file = 1;
    }
    FILE *output = input == NULL || same_file
                       ? NULL
                       : fopen(destination_name, "wb");
    int success = input != NULL && output != NULL && !same_file;
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
    const int saved_error = errno;
    free(source_name);
    free(destination_name);
    if (success) simp_runtime_clear_error();
    else simp_runtime_set_error(saved_error == 0 ? EIO : saved_error);
    return success;
}

int32_t simp_fs_mkdir(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    const int result = mkdir(name, 0777) == 0;
    const int saved_error = errno;
    free(name);
    if (result) simp_runtime_clear_error();
    else simp_runtime_set_error(saved_error);
    return result;
}

int32_t simp_fs_rmdir(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    const int result = rmdir(name) == 0;
    const int saved_error = errno;
    free(name);
    if (result) simp_runtime_clear_error();
    else simp_runtime_set_error(saved_error);
    return result;
}

void *simp_fs_list_dir(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return new_array();
    DIR *directory = opendir(name);
    const int open_error = errno;
    free(name);
    SimpArray *array = (SimpArray *)new_array();
    void *root = array;
    SimpRootFrame frame;
    void *slots[] = {&root};
    simp_gc_push_or_abort(&frame, slots, 1);
    if (directory != NULL) {
        errno = 0;
        struct dirent *entry;
        while ((entry = readdir(directory)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            void *value = simple_string_from_cstr(entry->d_name);
            append_array_object((SimpArray *)root, value);
        }
        const int read_error = errno;
        closedir(directory);
        if (read_error != 0) simp_runtime_set_error(read_error);
        else simp_runtime_clear_error();
    } else {
        simp_runtime_set_error(open_error);
    }
    simp_gc_pop_or_abort(&frame);
    return root;
}

void *simp_fs_get_cwd(void *self) {
    (void)self;
    size_t capacity = 256;
    for (;;) {
        char *path = (char *)malloc(capacity);
        if (path == NULL) {
            simp_runtime_set_error(ENOMEM);
            return simple_string(NULL, 0);
        }
        if (getcwd(path, capacity) != NULL) {
            void *result = simple_string_from_cstr(path);
            free(path);
            simp_runtime_clear_error();
            return result;
        }
        free(path);
        if (errno != ERANGE || capacity > SIZE_MAX / 2) {
            simp_runtime_set_error(errno);
            return simple_string(NULL, 0);
        }
        capacity *= 2;
    }
}

int32_t simp_fs_ch_dir(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return 0;
    const int result = chdir(name) == 0;
    const int saved_error = errno;
    free(name);
    if (result) simp_runtime_clear_error();
    else simp_runtime_set_error(saved_error);
    return result;
}

void *simp_fs_absolute_path(void *self, void *path) {
    (void)self;
    char *name = copy_string_bytes(path);
    if (name == NULL) return simple_string(NULL, 0);
    char *resolved = realpath(name, NULL);
    const int saved_error = errno;
    free(name);
    if (resolved == NULL) {
        simp_runtime_set_error(saved_error);
        return simple_string(NULL, 0);
    }
    void *result = simple_string_from_cstr(resolved);
    free(resolved);
    simp_runtime_clear_error();
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
    const int saved_error = errno;
    free(name);
    free(open_mode);
    if (file == NULL) simp_runtime_set_error(saved_error);
    else simp_runtime_clear_error();
    return file;
}

void *simp_file_read(void *self, void *handle, int64_t size) {
    (void)self;
    if (handle == NULL || size < 0) {
        simp_runtime_set_error(handle == NULL ? EBADF : EINVAL);
        return simple_string(NULL, 0);
    }
    if (size == 0) {
        simp_runtime_clear_error();
        return simple_string(NULL, 0);
    }
    if ((uint64_t)size > SIZE_MAX) {
        simp_runtime_set_error(EOVERFLOW);
        return simple_string(NULL, 0);
    }
    char *bytes = (char *)malloc((size_t)size);
    if (bytes == NULL) {
        simp_runtime_set_error(ENOMEM);
        return simple_string(NULL, 0);
    }
    const size_t count = fread(bytes, 1, (size_t)size, (FILE *)handle);
    if (ferror((FILE *)handle)) simp_runtime_set_error(errno == 0 ? EIO : errno);
    else simp_runtime_clear_error();
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
            if (length > (size_t)INT64_MAX ||
                count > (size_t)INT64_MAX - length) {
                free(bytes);
                simp_runtime_set_error(EOVERFLOW);
                return simple_string(NULL, 0);
            }
            const size_t required = length + count;
            if (required > capacity) {
                size_t grown = capacity == 0 ? sizeof(chunk) : capacity;
                while (grown < required) grown = grown > (size_t)INT64_MAX / 2
                                                     ? (size_t)INT64_MAX
                                                     : grown * 2;
                char *grown_bytes = (char *)realloc(bytes, grown);
                if (grown_bytes == NULL) {
                    free(bytes);
                    simp_runtime_set_error(ENOMEM);
                    return simple_string(NULL, 0);
                }
                bytes = grown_bytes;
                capacity = grown;
            }
            memcpy(bytes + length, chunk, count);
            length += count;
        }
        if (count < sizeof(chunk)) {
            if (ferror(file)) simp_runtime_set_error(errno == 0 ? EIO : errno);
            else simp_runtime_clear_error();
            break;
        }
    }
    void *result = simple_string(bytes, length);
    free(bytes);
    return result;
}

void *simp_file_read_all(void *self, void *handle) {
    (void)self;
    if (handle == NULL) {
        simp_runtime_set_error(EBADF);
        return simple_string(NULL, 0);
    }
    return read_all((FILE *)handle);
}

static int read_line_bytes(FILE *file, char **line, size_t *length) {
    errno = 0;
    size_t capacity = 128;
    size_t used = 0;
    char *bytes = (char *)malloc(capacity);
    if (bytes == NULL) {
        errno = ENOMEM;
        return 0;
    }
    int character;
    while ((character = fgetc(file)) != EOF) {
        if (character == '\n') break;
        if (used == (size_t)INT64_MAX) {
            errno = EOVERFLOW;
            free(bytes);
            return 0;
        }
        if (used == capacity) {
            const size_t grown = capacity > (size_t)INT64_MAX / 2
                                     ? (size_t)INT64_MAX
                                     : capacity * 2;
            char *grown_bytes = (char *)realloc(bytes, grown);
            if (grown_bytes == NULL) {
                errno = ENOMEM;
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
    if (handle == NULL) {
        simp_runtime_set_error(EBADF);
        return simple_string(NULL, 0);
    }
    char *line;
    size_t length;
    if (!read_line_bytes((FILE *)handle, &line, &length)) {
        if (ferror((FILE *)handle) || errno == ENOMEM || errno == EOVERFLOW)
            simp_runtime_set_error(errno == 0 ? EIO : errno);
        else simp_runtime_clear_error();
        return simple_string(NULL, 0);
    }
    simp_runtime_clear_error();
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
        if (ferror((FILE *)handle) || errno == ENOMEM || errno == EOVERFLOW)
            simp_runtime_set_error(errno == 0 ? EIO : errno);
        else simp_runtime_clear_error();
    } else {
        simp_runtime_set_error(EBADF);
    }
    simp_gc_pop_or_abort(&frame);
    return root;
}

int64_t simp_file_write(void *self, void *handle, void *data) {
    (void)self;
    if (handle == NULL || data == NULL) {
        simp_runtime_set_error(EINVAL);
        return 0;
    }
    const char *bytes;
    uint64_t length;
    simp_string_bytes(data, &bytes, &length);
    const size_t written = fwrite(bytes, 1, (size_t)length, (FILE *)handle);
    if (written != length) simp_runtime_set_error(errno == 0 ? EIO : errno);
    else simp_runtime_clear_error();
    return written > (size_t)INT64_MAX ? INT64_MAX : (int64_t)written;
}

int64_t simp_file_write_line(void *self, void *handle, void *line) {
    (void)self;
    if (handle == NULL || line == NULL) {
        simp_runtime_set_error(EINVAL);
        return 0;
    }
    const char *bytes;
    uint64_t length;
    simp_string_bytes(line, &bytes, &length);
    const size_t written = fwrite(bytes, 1, (size_t)length, (FILE *)handle);
    if (written != length) {
        simp_runtime_set_error(errno == 0 ? EIO : errno);
        return written > (size_t)INT64_MAX ? INT64_MAX : (int64_t)written;
    }
    const int newline = fputc('\n', (FILE *)handle);
    if (newline == EOF) {
        simp_runtime_set_error(errno == 0 ? EIO : errno);
        return written > (size_t)INT64_MAX ? INT64_MAX : (int64_t)written;
    }
    if (written >= (size_t)INT64_MAX) {
        simp_runtime_set_error(EOVERFLOW);
        return INT64_MAX;
    }
    simp_runtime_clear_error();
    return (int64_t)written + 1;
}

int64_t simp_file_seek(void *self, void *handle, int64_t offset, int64_t whence) {
    (void)self;
    if (handle == NULL) {
        simp_runtime_set_error(EBADF);
        return -1;
    }
    if (offset < (int64_t)LONG_MIN || offset > (int64_t)LONG_MAX ||
        whence < INT_MIN || whence > INT_MAX) {
        simp_runtime_set_error(EOVERFLOW);
        return -1;
    }
    const int result = fseek((FILE *)handle, (long)offset, (int)whence);
    if (result != 0) simp_runtime_set_error(errno);
    else simp_runtime_clear_error();
    return result;
}

int64_t simp_file_tell(void *self, void *handle) {
    (void)self;
    if (handle == NULL) {
        simp_runtime_set_error(EBADF);
        return -1;
    }
    const long position = ftell((FILE *)handle);
    if (position < 0) simp_runtime_set_error(errno);
    else simp_runtime_clear_error();
    return position;
}

void simp_file_flush(void *self, void *handle) {
    (void)self;
    if (handle == NULL) simp_runtime_set_error(EBADF);
    else if (fflush((FILE *)handle) != 0) simp_runtime_set_error(errno);
    else simp_runtime_clear_error();
}

void simp_file_close(void *self, void *handle) {
    (void)self;
    if (handle == NULL) simp_runtime_set_error(EBADF);
    else if (fclose((FILE *)handle) != 0) simp_runtime_set_error(errno);
    else simp_runtime_clear_error();
}

int32_t simp_file_eof(void *self, void *handle) {
    (void)self;
    if (handle == NULL) {
        simp_runtime_set_error(EBADF);
        return 0;
    }
    simp_runtime_clear_error();
    return feof((FILE *)handle);
}

double simp_math_abs(void *self, double x) { (void)self; return fabs(x); }
int64_t simp_math_abs_int(void *self, int64_t x) {
    (void)self;
    return x == INT64_MIN ? INT64_MAX : (x < 0 ? -x : x);
}
double simp_math_min(void *self, double a, double b) { (void)self; return fmin(a, b); }
double simp_math_max(void *self, double a, double b) { (void)self; return fmax(a, b); }
int64_t simp_math_min_int(void *self, int64_t a, int64_t b) { (void)self; return a < b ? a : b; }
int64_t simp_math_max_int(void *self, int64_t a, int64_t b) { (void)self; return a > b ? a : b; }
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

static int socket_fd(int64_t fd) {
    if (fd < 0) {
        errno = EBADF;
        simp_runtime_set_error(errno);
        return -1;
    }
    if (fd > INT_MAX) {
        errno = EOVERFLOW;
        simp_runtime_set_error(errno);
        return -1;
    }
    return (int)fd;
}

static int64_t native_send(int fd, const void *bytes, size_t length) {
    if (length > (size_t)SSIZE_MAX || (uintmax_t)length > (uintmax_t)INT64_MAX) {
        errno = EOVERFLOW;
        simp_runtime_set_error(errno);
        return -1;
    }
#ifdef MSG_NOSIGNAL
    const ssize_t result = send(fd, bytes, length, MSG_NOSIGNAL);
#else
    const ssize_t result = send(fd, bytes, length, 0);
#endif
    if (result < 0) simp_runtime_set_error(errno);
    else simp_runtime_clear_error();
    return (int64_t)result;
}

int64_t simp_net_socket_create(void *self) {
    (void)self;
    const int descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (descriptor < 0) simp_runtime_set_error(errno);
    else simp_runtime_clear_error();
    return descriptor;
}

int32_t simp_net_socket_connect(void *self, int64_t fd, void *host, int64_t port) {
    (void)self;
    const int descriptor = socket_fd(fd);
    if (descriptor < 0) return 0;
    char *hostname = copy_string_bytes(host);
    if (hostname == NULL || port < 0 || port > 65535) {
        if (hostname != NULL) simp_runtime_set_error(EINVAL);
        free(hostname);
        return 0;
    }
    char service[6];
    (void)snprintf(service, sizeof(service), "%d", (int)port);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *addresses = NULL;
    const int lookup = getaddrinfo(hostname, service, &hints, &addresses);
    free(hostname);
    if (lookup != 0) {
        simp_runtime_set_error(EINVAL);
        return 0;
    }
    int connected = 0;
    for (struct addrinfo *address = addresses; address != NULL; address = address->ai_next) {
        if (connect(descriptor, address->ai_addr, address->ai_addrlen) == 0) {
            connected = 1;
            break;
        }
    }
    const int saved_error = errno;
    freeaddrinfo(addresses);
    if (connected) simp_runtime_clear_error();
    else simp_runtime_set_error(saved_error == 0 ? EIO : saved_error);
    return connected;
}

int64_t simp_net_socket_send(void *self, int64_t fd, void *buffer) {
    (void)self;
    const int descriptor = socket_fd(fd);
    if (descriptor < 0) return -1;
    if (buffer == NULL) {
        simp_runtime_set_error(EINVAL);
        return -1;
    }
    const SimpBuffer *data = (const SimpBuffer *)buffer;
    if (data->length > SIZE_MAX) {
        simp_runtime_set_error(EOVERFLOW);
        return -1;
    }
    return native_send(descriptor, data->data, (size_t)data->length);
}

int64_t simp_net_socket_send_string(void *self, int64_t fd, void *text) {
    (void)self;
    const int descriptor = socket_fd(fd);
    if (descriptor < 0) return -1;
    if (text == NULL) {
        simp_runtime_set_error(EINVAL);
        return -1;
    }
    const char *bytes;
    uint64_t length;
    simp_string_bytes(text, &bytes, &length);
    if (length > SIZE_MAX) {
        simp_runtime_set_error(EOVERFLOW);
        return -1;
    }
    return native_send(descriptor, bytes, (size_t)length);
}

void *simp_net_socket_recv(void *self, int64_t fd, int64_t max_bytes) {
    (void)self;
    const int descriptor = socket_fd(fd);
    if (descriptor < 0) return NULL;
    if (max_bytes < 0) {
        simp_runtime_set_error(EINVAL);
        return NULL;
    }
    if ((uint64_t)max_bytes > SIZE_MAX || max_bytes > SSIZE_MAX) {
        simp_runtime_set_error(EOVERFLOW);
        return NULL;
    }
    char *bytes = max_bytes == 0 ? NULL : (char *)malloc((size_t)max_bytes);
    if (max_bytes != 0 && bytes == NULL) {
        simp_runtime_set_error(ENOMEM);
        return NULL;
    }
    const ssize_t received = recv(descriptor, bytes, (size_t)max_bytes, 0);
    if (received < 0) {
        simp_runtime_set_error(errno);
        free(bytes);
        return NULL;
    }
    simp_runtime_clear_error();
    void *result = simp_buffer_new((int64_t)received, stdlib_native_file,
                                   sizeof(stdlib_native_file) - 1, 0, 0);
    SimpBuffer *buffer = (SimpBuffer *)result;
    if (received != 0) memcpy(buffer->data, bytes, (size_t)received);
    free(bytes);
    return result;
}

void *simp_net_socket_recv_string(void *self, int64_t fd, int64_t max_bytes) {
    (void)self;
    const int descriptor = socket_fd(fd);
    if (descriptor < 0) return simple_string(NULL, 0);
    if (max_bytes < 0) {
        simp_runtime_set_error(EINVAL);
        return simple_string(NULL, 0);
    }
    if ((uint64_t)max_bytes > SIZE_MAX || max_bytes > SSIZE_MAX) {
        simp_runtime_set_error(EOVERFLOW);
        return simple_string(NULL, 0);
    }
    char *bytes = max_bytes == 0 ? NULL : (char *)malloc((size_t)max_bytes);
    if (max_bytes != 0 && bytes == NULL) {
        simp_runtime_set_error(ENOMEM);
        return simple_string(NULL, 0);
    }
    const ssize_t received = recv(descriptor, bytes, (size_t)max_bytes, 0);
    if (received < 0) {
        simp_runtime_set_error(errno);
        free(bytes);
        return simple_string(NULL, 0);
    }
    simp_runtime_clear_error();
    void *result = simple_string(bytes, (uint64_t)received);
    free(bytes);
    return result;
}

void simp_net_socket_close(void *self, int64_t fd) {
    (void)self;
    const int descriptor = socket_fd(fd);
    if (descriptor >= 0) {
        if (close(descriptor) != 0) simp_runtime_set_error(errno);
        else simp_runtime_clear_error();
    }
}

void simp_net_socket_set_timeout(void *self, int64_t fd, int64_t milliseconds) {
    (void)self;
    const int descriptor = socket_fd(fd);
    if (descriptor < 0) return;
    if (milliseconds < 0) {
        simp_runtime_set_error(EINVAL);
        return;
    }
    struct timeval timeout;
    const int64_t seconds = milliseconds / 1000;
    timeout.tv_sec = (time_t)seconds;
    if ((int64_t)timeout.tv_sec != seconds) {
        simp_runtime_set_error(EOVERFLOW);
        return;
    }
    timeout.tv_usec = (suseconds_t)((milliseconds % 1000) * 1000);
    if (setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0)
        simp_runtime_set_error(errno);
    else simp_runtime_clear_error();
}

int64_t simp_net_server_bind(void *self, void *host, int64_t port, int64_t backlog) {
    (void)self;
    char *hostname = copy_string_bytes(host);
    if (hostname == NULL || port < 0 || port > 65535 || backlog < 0 ||
        backlog > INT_MAX) {
        if (hostname != NULL) simp_runtime_set_error(EINVAL);
        free(hostname);
        return -1;
    }
    char service[6];
    (void)snprintf(service, sizeof(service), "%d", (int)port);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *addresses = NULL;
    const int lookup = getaddrinfo(hostname[0] == '\0' ? NULL : hostname,
                                   service, &hints, &addresses);
    free(hostname);
    if (lookup != 0) {
        simp_runtime_set_error(EINVAL);
        return -1;
    }
    int server_fd = -1;
    for (struct addrinfo *address = addresses; address != NULL; address = address->ai_next) {
        const int candidate = socket(address->ai_family, address->ai_socktype,
                                     address->ai_protocol);
        if (candidate < 0) continue;
        int reuse = 1;
        (void)setsockopt(candidate, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        if (bind(candidate, address->ai_addr, address->ai_addrlen) == 0 &&
            listen(candidate, backlog > 0 ? (int)backlog : 16) == 0) {
            server_fd = candidate;
            break;
        }
        (void)close(candidate);
    }
    const int saved_error = errno;
    freeaddrinfo(addresses);
    if (server_fd < 0) simp_runtime_set_error(saved_error == 0 ? EIO : saved_error);
    else simp_runtime_clear_error();
    return server_fd;
}

int64_t simp_net_server_accept(void *self, int64_t server_fd) {
    (void)self;
    const int descriptor = socket_fd(server_fd);
    if (descriptor < 0) return -1;
    const int accepted = accept(descriptor, NULL, NULL);
    if (accepted < 0) simp_runtime_set_error(errno);
    else simp_runtime_clear_error();
    return accepted;
}

int32_t simp_net_server_listen(void *self, int64_t server_fd, int64_t backlog) {
    (void)self;
    const int descriptor = socket_fd(server_fd);
    if (descriptor < 0) return 0;
    if (backlog < 0 || backlog > INT_MAX) {
        simp_runtime_set_error(EINVAL);
        return 0;
    }
    const int result = listen(descriptor, backlog > 0 ? (int)backlog : 16) == 0;
    if (result) simp_runtime_clear_error();
    else simp_runtime_set_error(errno);
    return result;
}

int64_t simp_net_server_port(void *self, int64_t server_fd) {
    (void)self;
    const int descriptor = socket_fd(server_fd);
    if (descriptor < 0) return 0;
    struct sockaddr_in address;
    socklen_t length = sizeof(address);
    if (getsockname(descriptor, (struct sockaddr *)&address, &length) != 0) {
        simp_runtime_set_error(errno);
        return 0;
    }
    simp_runtime_clear_error();
    return ntohs(address.sin_port);
}
