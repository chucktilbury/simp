/**
 * @file RuntimeProcess.c
 * @brief argv-based POSIX child process spawning and captured output.
 */
#define _POSIX_C_SOURCE 200809L

#include "simp/RuntimeGc.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
extern const SimpClassMeta simp_string_class_meta __attribute__((weak));
extern void simp_runtime_set_error(int error_number);
extern void simp_runtime_clear_error(void);

typedef struct SimpProcess {
    pid_t pid;
    int stdout_fd;
    int stderr_fd;
    int waited;
    int status;
    int output_error;
    int stdout_error;
    int stderr_error;
    char *stdout_bytes;
    size_t stdout_length;
    size_t stdout_capacity;
    char *stderr_bytes;
    size_t stderr_length;
    size_t stderr_capacity;
} SimpProcess;

static char *copy_string(void *object) {
    if (object == NULL) {
        errno = EINVAL;
        return NULL;
    }
    const char *bytes;
    uint64_t length;
    simp_string_bytes(object, &bytes, &length);
    if (length > SIZE_MAX - 1 || memchr(bytes, '\0', (size_t)length) != NULL) {
        errno = EINVAL;
        return NULL;
    }
    char *copy = (char *)malloc((size_t)length + 1);
    if (copy == NULL) return NULL;
    memcpy(copy, bytes, (size_t)length);
    copy[length] = '\0';
    return copy;
}

static void close_fd(int *descriptor) {
    if (*descriptor >= 0) {
        (void)close(*descriptor);
        *descriptor = -1;
    }
}

static int make_pipe(int descriptors[2]) {
    if (pipe(descriptors) != 0) return 0;
    const int read_flags = fcntl(descriptors[0], F_GETFL);
    const int close_read = fcntl(descriptors[0], F_GETFD);
    const int close_write = fcntl(descriptors[1], F_GETFD);
    if (read_flags < 0 || close_read < 0 || close_write < 0 ||
        fcntl(descriptors[0], F_SETFL, read_flags | O_NONBLOCK) != 0 ||
        fcntl(descriptors[0], F_SETFD, close_read | FD_CLOEXEC) != 0 ||
        fcntl(descriptors[1], F_SETFD, close_write | FD_CLOEXEC) != 0) {
        const int saved = errno;
        (void)close(descriptors[0]);
        (void)close(descriptors[1]);
        errno = saved;
        return 0;
    }
    return 1;
}

static void free_arguments(char **arguments, size_t count) {
    if (arguments == NULL) return;
    for (size_t index = 0; index < count; ++index) free(arguments[index]);
    free(arguments);
}

void *simp_process_spawn(void *self, void *executable, void *arguments) {
    (void)self;
    char *program = copy_string(executable);
    if (program == NULL || program[0] == '\0' || arguments == NULL) {
        const int saved = errno == 0 ? EINVAL : errno;
        free(program);
        simp_runtime_set_error(saved);
        return NULL;
    }
    const SimpArray *array = (const SimpArray *)arguments;
    if (array->length > SIZE_MAX / sizeof(char *) - 2) {
        free(program);
        simp_runtime_set_error(E2BIG);
        return NULL;
    }
    const size_t argument_count = (size_t)array->length;
    char **argv = (char **)calloc(argument_count + 2, sizeof(*argv));
    if (argv == NULL) {
        free(program);
        simp_runtime_set_error(ENOMEM);
        return NULL;
    }
    argv[0] = program;
    for (size_t index = 0; index < argument_count; ++index) {
        const SimpArrayValue *value = &array->values[index];
        if ((value->tag != SIMP_ARRAY_STRING && value->tag != SIMP_ARRAY_OBJECT) ||
            value->pointer == NULL) {
            free_arguments(argv, index + 1);
            simp_runtime_set_error(EINVAL);
            return NULL;
        }
        argv[index + 1] = copy_string(value->pointer);
        if (argv[index + 1] == NULL) {
            const int saved = errno == 0 ? ENOMEM : errno;
            free_arguments(argv, index + 1);
            simp_runtime_set_error(saved);
            return NULL;
        }
    }

    int output_pipe[2] = {-1, -1};
    int error_pipe[2] = {-1, -1};
    if (!make_pipe(output_pipe) || !make_pipe(error_pipe)) {
        const int saved = errno;
        close_fd(&output_pipe[0]);
        close_fd(&output_pipe[1]);
        close_fd(&error_pipe[0]);
        close_fd(&error_pipe[1]);
        free_arguments(argv, argument_count + 1);
        simp_runtime_set_error(saved);
        return NULL;
    }

    posix_spawn_file_actions_t actions;
    int action_status = posix_spawn_file_actions_init(&actions);
    const int actions_initialized = action_status == 0;
    if (action_status == 0) {
        action_status = posix_spawn_file_actions_adddup2(
            &actions, output_pipe[1], STDOUT_FILENO);
    }
    if (action_status == 0) {
        action_status = posix_spawn_file_actions_adddup2(
            &actions, error_pipe[1], STDERR_FILENO);
    }
    if (action_status == 0) {
        action_status = posix_spawn_file_actions_addclose(&actions, output_pipe[0]);
    }
    if (action_status == 0) {
        action_status = posix_spawn_file_actions_addclose(&actions, error_pipe[0]);
    }
    if (action_status == 0) {
        action_status = posix_spawn_file_actions_addclose(&actions, output_pipe[1]);
    }
    if (action_status == 0) {
        action_status = posix_spawn_file_actions_addclose(&actions, error_pipe[1]);
    }
    pid_t pid = -1;
    if (action_status == 0) {
        action_status = posix_spawnp(&pid, program, &actions, NULL, argv, environ);
    }
    if (actions_initialized) (void)posix_spawn_file_actions_destroy(&actions);
    free_arguments(argv, argument_count + 1);
    close_fd(&output_pipe[1]);
    close_fd(&error_pipe[1]);
    if (action_status != 0) {
        close_fd(&output_pipe[0]);
        close_fd(&error_pipe[0]);
        simp_runtime_set_error(action_status);
        return NULL;
    }

    SimpProcess *process = (SimpProcess *)calloc(1, sizeof(*process));
    if (process == NULL) {
        (void)kill(pid, SIGTERM);
        while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) {
        }
        close_fd(&output_pipe[0]);
        close_fd(&error_pipe[0]);
        simp_runtime_set_error(ENOMEM);
        return NULL;
    }
    process->pid = pid;
    process->stdout_fd = output_pipe[0];
    process->stderr_fd = error_pipe[0];
    simp_runtime_clear_error();
    return process;
}

static int append_output(char **data, size_t *length, size_t *capacity,
                         const char *bytes, size_t count) {
    if (*length > (size_t)INT64_MAX || count > (size_t)INT64_MAX - *length) {
        errno = EOVERFLOW;
        return 0;
    }
    const size_t required = *length + count;
    if (required > *capacity) {
        size_t grown = *capacity == 0 ? 4096 : *capacity;
        while (grown < required) {
            if (grown > (size_t)INT64_MAX / 2) {
                grown = (size_t)INT64_MAX;
                break;
            }
            grown *= 2;
        }
        char *next = (char *)realloc(*data, grown);
        if (next == NULL) {
            errno = ENOMEM;
            return 0;
        }
        *data = next;
        *capacity = grown;
    }
    memcpy(*data + *length, bytes, count);
    *length = required;
    return 1;
}

static void drain_pipe(SimpProcess *process, int *descriptor, int is_error) {
    char chunk[8192];
    for (;;) {
        const ssize_t count = read(*descriptor, chunk, sizeof(chunk));
        if (count > 0) {
            int stored;
            if (is_error) {
                if (process->stderr_error != 0) continue;
                stored = append_output(&process->stderr_bytes, &process->stderr_length,
                                      &process->stderr_capacity, chunk, (size_t)count);
            } else {
                if (process->stdout_error != 0) continue;
                stored = append_output(&process->stdout_bytes, &process->stdout_length,
                                      &process->stdout_capacity, chunk, (size_t)count);
            }
            if (!stored) {
                if (is_error) process->stderr_error = errno;
                else process->stdout_error = errno;
                if (process->output_error == 0) process->output_error = errno;
            }
            continue;
        }
        if (count == 0) {
            close_fd(descriptor);
            return;
        }
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return;
        process->output_error = errno;
        close_fd(descriptor);
        return;
    }
}

int32_t simp_process_wait(void *self, void *object) {
    (void)self;
    SimpProcess *process = (SimpProcess *)object;
    if (process == NULL) {
        simp_runtime_set_error(EINVAL);
        return 0;
    }
    if (process->waited) return 1;
    int child_reaped = 0;
    int wait_status = 0;
    int wait_error = 0;
    simp_runtime_gil_release();
    while (!child_reaped || process->stdout_fd >= 0 || process->stderr_fd >= 0) {
        struct pollfd descriptors[2] = {
            {process->stdout_fd, POLLIN | POLLHUP, 0},
            {process->stderr_fd, POLLIN | POLLHUP, 0}
        };
        (void)poll(descriptors, 2, 20);
        if (process->stdout_fd >= 0) drain_pipe(process, &process->stdout_fd, 0);
        if (process->stderr_fd >= 0) drain_pipe(process, &process->stderr_fd, 1);
        if (!child_reaped) {
            const pid_t waited = waitpid(process->pid, &wait_status, WNOHANG);
            if (waited == process->pid) child_reaped = 1;
            else if (waited < 0 && errno != EINTR) {
                wait_error = errno;
                child_reaped = 1;
                close_fd(&process->stdout_fd);
                close_fd(&process->stderr_fd);
            }
        }
    }
    if (!child_reaped) {
        while (waitpid(process->pid, &wait_status, 0) < 0) {
            if (errno == EINTR) continue;
            wait_error = errno;
            break;
        }
    }
    simp_runtime_gil_acquire();
    if (wait_error != 0) {
        simp_runtime_set_error(wait_error);
        return 0;
    }
    process->status = WIFEXITED(wait_status)
                          ? WEXITSTATUS(wait_status)
                          : WIFSIGNALED(wait_status) ? -WTERMSIG(wait_status) : -1;
    process->waited = 1;
    if (process->output_error != 0) simp_runtime_set_error(process->output_error);
    else simp_runtime_clear_error();
    return 1;
}

int64_t simp_process_exit_code(void *self, void *object) {
    (void)self;
    SimpProcess *process = (SimpProcess *)object;
    return process == NULL || !process->waited ? -1 : process->status;
}

static void *captured_string(const char *bytes, size_t length) {
    if (&simp_string_class_meta == NULL) abort();
    return simp_string_new(&simp_string_class_meta, bytes, length);
}

void *simp_process_stdout(void *self, void *object) {
    (void)self;
    SimpProcess *process = (SimpProcess *)object;
    if (process == NULL || !process->waited) {
        simp_runtime_set_error(EINVAL);
        return captured_string("", 0);
    }
    return captured_string(process->stdout_bytes, process->stdout_length);
}

void *simp_process_stderr(void *self, void *object) {
    (void)self;
    SimpProcess *process = (SimpProcess *)object;
    if (process == NULL || !process->waited) {
        simp_runtime_set_error(EINVAL);
        return captured_string("", 0);
    }
    return captured_string(process->stderr_bytes, process->stderr_length);
}

void simp_process_close(void *self, void *object) {
    (void)self;
    SimpProcess *process = (SimpProcess *)object;
    if (process == NULL) return;
    if (!process->waited) (void)simp_process_wait(self, process);
    close_fd(&process->stdout_fd);
    close_fd(&process->stderr_fd);
    free(process->stdout_bytes);
    free(process->stderr_bytes);
    free(process);
}
