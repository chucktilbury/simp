/**
 * @file RuntimeProcessAsync.c
 * @brief Asynchronous argv-based POSIX child processes with streamed output.
 *
 * Each started child is owned by one unregistered native "pump" thread. The
 * pump never touches managed memory or the runtime lock: it drains stdout and
 * stderr into an unmanaged, internally locked event queue, reaps the child,
 * escalates cancellation, and finally appends exactly one completion event.
 * Simple threads consume that queue with explicit pull operations. Blocking
 * consumers release the runtime lock before waiting on the native mutex, and
 * never acquire the runtime lock while holding it.
 */
#define _GNU_SOURCE

#include "simp/RuntimeGc.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char **environ;
extern const SimpClassMeta simp_string_class_meta __attribute__((weak));
extern void simp_runtime_set_error(int error_number);
extern void simp_runtime_clear_error(void);

enum {
    EVENT_STDOUT = 1,
    EVENT_STDERR = 2,
    EVENT_COMPLETED = 3
};

enum {
    STATE_RUNNING = 0,
    STATE_EXITED = 1,
    STATE_SIGNALED = 2,
    STATE_CANCELLED = 3,
    STATE_LAUNCH_FAILED = 4,
    STATE_WAIT_FAILED = 5
};

enum {
    PUMP_POLL_MILLISECONDS = 50,
    CANCEL_DRAIN_MILLISECONDS = 250,
    READ_CHUNK_BYTES = 65536,
    READS_PER_STREAM_PASS = 16
};

typedef struct SimpProcessEvent {
    struct SimpProcessEvent *next;
    int kind;
    size_t length;
    unsigned char data[];
} SimpProcessEvent;

typedef struct SimpAsyncProcess {
    pthread_mutex_t lock;
    pthread_cond_t changed;
    pthread_t pump;
    int pump_started;
    int pump_joining;
    int pump_joined;

    pid_t pid;
    /* Owned exclusively by the pump thread after start. */
    int stdout_fd;
    int stderr_fd;

    SimpProcessEvent *head;
    SimpProcessEvent *tail;

    int reaped;
    int completed;
    int completion_taken;
    int closing;
    int references;

    int cancel_requested;
    int cancel_effective;
    int killed;
    int64_t kill_deadline;
    int64_t cancel_time;
    int64_t reap_time;

    int state;
    int exit_code;
    int signal_number;
    char *error;
} SimpAsyncProcess;

static int64_t monotonic_milliseconds(void) {
    struct timespec now;
    (void)clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static void deadline_after(struct timespec *deadline, int64_t milliseconds) {
    (void)clock_gettime(CLOCK_MONOTONIC, deadline);
    deadline->tv_sec += (time_t)(milliseconds / 1000);
    deadline->tv_nsec += (long)(milliseconds % 1000) * 1000000L;
    if (deadline->tv_nsec >= 1000000000L) {
        deadline->tv_sec += 1;
        deadline->tv_nsec -= 1000000000L;
    }
}

static char *format_message(const char *prefix, const char *subject, int error_number) {
    char reason[256];
    const int saved = errno;
    if (strerror_r(error_number, reason, sizeof(reason)) != 0 || reason[0] == '\0') {
        (void)snprintf(reason, sizeof(reason), "operating system error %d", error_number);
    }
    errno = saved;
    const char *quoted = subject == NULL ? "" : subject;
    const size_t size = strlen(prefix) + strlen(quoted) + strlen(reason) + 8;
    char *message = (char *)malloc(size);
    if (message == NULL) return NULL;
    if (subject != NULL) (void)snprintf(message, size, "%s '%s': %s", prefix, quoted, reason);
    else (void)snprintf(message, size, "%s: %s", prefix, reason);
    return message;
}

/* Caller holds process->lock (or exclusively owns a not-yet-shared object). */
static void record_error_locked(SimpAsyncProcess *process, const char *prefix,
                                const char *subject, int error_number) {
    if (process->error != NULL) return;
    process->error = format_message(prefix, subject, error_number);
}

static int append_event_locked(SimpAsyncProcess *process, int kind, const unsigned char *data,
                               size_t length) {
    SimpProcessEvent *event = (SimpProcessEvent *)malloc(sizeof(*event) + length);
    if (event == NULL) return 0;
    event->next = NULL;
    event->kind = kind;
    event->length = length;
    if (length > 0) memcpy(event->data, data, length);
    if (process->tail != NULL) process->tail->next = event;
    else process->head = event;
    process->tail = event;
    (void)pthread_cond_broadcast(&process->changed);
    return 1;
}

static void close_fd(int *descriptor) {
    if (*descriptor >= 0) {
        (void)close(*descriptor);
        *descriptor = -1;
    }
}

/* Pipe descriptors stay above the standard streams so the child's dup2/open
 * actions can never overwrite them, even if the parent closed stdin/stdout. */
static int raise_descriptor(int *descriptor) {
    if (*descriptor > STDERR_FILENO) return 1;
    const int raised = fcntl(*descriptor, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
    if (raised < 0) return 0;
    (void)close(*descriptor);
    *descriptor = raised;
    return 1;
}

static int make_pipe(int descriptors[2]) {
    if (pipe2(descriptors, O_CLOEXEC) != 0) return 0;
    const int flags = fcntl(descriptors[0], F_GETFL);
    if (!raise_descriptor(&descriptors[0]) || !raise_descriptor(&descriptors[1]) ||
        flags < 0 || fcntl(descriptors[0], F_SETFL, flags | O_NONBLOCK) != 0) {
        const int saved = errno;
        close_fd(&descriptors[0]);
        close_fd(&descriptors[1]);
        errno = saved;
        return 0;
    }
    return 1;
}

static char *copy_string(void *object) {
    if (object == NULL) {
        errno = EINVAL;
        return NULL;
    }
    const char *bytes;
    uint64_t length;
    simp_string_bytes(object, &bytes, &length);
    if (length > SIZE_MAX - 1 || (length > 0 && memchr(bytes, '\0', (size_t)length) != NULL)) {
        errno = EINVAL;
        return NULL;
    }
    char *copy = (char *)malloc((size_t)length + 1);
    if (copy == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if (length > 0) memcpy(copy, bytes, (size_t)length);
    copy[length] = '\0';
    return copy;
}

static void free_arguments(char **arguments, size_t count) {
    if (arguments == NULL) return;
    for (size_t index = 0; index < count; ++index) free(arguments[index]);
    free(arguments);
}

/* Signals the child's process group (the child is its leader), falling back to
 * the child itself. Only called with the lock held while the child is not yet
 * reaped, so the identifiers cannot have been recycled. */
static void signal_child_locked(SimpAsyncProcess *process, int signal_number) {
    if (kill(-process->pid, signal_number) != 0) (void)kill(process->pid, signal_number);
}

static void reap_locked(SimpAsyncProcess *process) {
    if (process->reaped) return;
    int status = 0;
    pid_t waited;
    do {
        waited = waitpid(process->pid, &status, WNOHANG);
    } while (waited < 0 && errno == EINTR);
    if (waited == 0) return;
    process->reaped = 1;
    process->reap_time = monotonic_milliseconds();
    if (waited < 0) {
        process->state = STATE_WAIT_FAILED;
        record_error_locked(process, "wait failed", NULL, errno);
        return;
    }
    if (WIFEXITED(status)) {
        process->exit_code = WEXITSTATUS(status);
        process->state = STATE_EXITED;
    } else if (WIFSIGNALED(status)) {
        process->signal_number = WTERMSIG(status);
        process->state = STATE_SIGNALED;
    } else {
        process->state = STATE_WAIT_FAILED;
        record_error_locked(process, "wait failed", NULL, ECHILD);
    }
    if (process->cancel_effective && process->state != STATE_WAIT_FAILED) {
        process->state = STATE_CANCELLED;
    }
}

static void drain_stream(SimpAsyncProcess *process, int *descriptor, int kind) {
    unsigned char chunk[READ_CHUNK_BYTES];
    for (int pass = 0; pass < READS_PER_STREAM_PASS && *descriptor >= 0; ++pass) {
        const ssize_t count = read(*descriptor, chunk, sizeof(chunk));
        if (count > 0) {
            (void)pthread_mutex_lock(&process->lock);
            if (!append_event_locked(process, kind, chunk, (size_t)count)) {
                record_error_locked(process,
                                    kind == EVENT_STDOUT ? "stdout capture failed; output was discarded"
                                                         : "stderr capture failed; output was discarded",
                                    NULL, ENOMEM);
            }
            (void)pthread_mutex_unlock(&process->lock);
            continue;
        }
        if (count == 0) {
            close_fd(descriptor);
            return;
        }
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return;
        const int saved = errno;
        (void)pthread_mutex_lock(&process->lock);
        record_error_locked(process, kind == EVENT_STDOUT ? "stdout read failed" : "stderr read failed",
                            NULL, saved);
        (void)pthread_mutex_unlock(&process->lock);
        close_fd(descriptor);
        return;
    }
}

static void *pump_main(void *argument) {
    SimpAsyncProcess *process = (SimpAsyncProcess *)argument;
    for (;;) {
        struct pollfd descriptors[2];
        nfds_t count = 0;
        if (process->stdout_fd >= 0) {
            descriptors[count].fd = process->stdout_fd;
            descriptors[count].events = POLLIN;
            descriptors[count].revents = 0;
            ++count;
        }
        if (process->stderr_fd >= 0) {
            descriptors[count].fd = process->stderr_fd;
            descriptors[count].events = POLLIN;
            descriptors[count].revents = 0;
            ++count;
        }
        (void)poll(count == 0 ? NULL : descriptors, count, PUMP_POLL_MILLISECONDS);
        if (process->stdout_fd >= 0) drain_stream(process, &process->stdout_fd, EVENT_STDOUT);
        if (process->stderr_fd >= 0) drain_stream(process, &process->stderr_fd, EVENT_STDERR);

        (void)pthread_mutex_lock(&process->lock);
        const int64_t now = monotonic_milliseconds();
        reap_locked(process);
        if (!process->reaped && process->cancel_effective && !process->killed &&
            now >= process->kill_deadline) {
            signal_child_locked(process, SIGKILL);
            process->killed = 1;
        }
        int abandon_output = 0;
        if (process->reaped) {
            if (process->closing) abandon_output = 1;
            else if (process->cancel_requested) {
                const int64_t since = process->reap_time > process->cancel_time
                                          ? process->reap_time
                                          : process->cancel_time;
                abandon_output = now - since >= CANCEL_DRAIN_MILLISECONDS;
            }
        }
        (void)pthread_mutex_unlock(&process->lock);

        if (abandon_output) {
            close_fd(&process->stdout_fd);
            close_fd(&process->stderr_fd);
        }
        if (process->reaped && process->stdout_fd < 0 && process->stderr_fd < 0) {
            (void)pthread_mutex_lock(&process->lock);
            process->completed = 1;
            if (!append_event_locked(process, EVENT_COMPLETED, NULL, 0)) {
                record_error_locked(process, "completion event allocation failed", NULL, ENOMEM);
            }
            (void)pthread_cond_broadcast(&process->changed);
            (void)pthread_mutex_unlock(&process->lock);
            return NULL;
        }
    }
}

static SimpAsyncProcess *allocate_process(void) {
    SimpAsyncProcess *process = (SimpAsyncProcess *)calloc(1, sizeof(*process));
    if (process == NULL) return NULL;
    pthread_condattr_t attributes;
    if (pthread_condattr_init(&attributes) != 0) {
        free(process);
        return NULL;
    }
    (void)pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC);
    const int cond_status = pthread_cond_init(&process->changed, &attributes);
    (void)pthread_condattr_destroy(&attributes);
    if (cond_status != 0) {
        free(process);
        return NULL;
    }
    if (pthread_mutex_init(&process->lock, NULL) != 0) {
        (void)pthread_cond_destroy(&process->changed);
        free(process);
        return NULL;
    }
    process->pid = -1;
    process->stdout_fd = -1;
    process->stderr_fd = -1;
    process->references = 1;
    process->state = STATE_RUNNING;
    process->exit_code = -1;
    return process;
}

static void destroy_process(SimpAsyncProcess *process) {
    SimpProcessEvent *event = process->head;
    while (event != NULL) {
        SimpProcessEvent *next = event->next;
        free(event);
        event = next;
    }
    free(process->error);
    (void)pthread_mutex_destroy(&process->lock);
    (void)pthread_cond_destroy(&process->changed);
    free(process);
}

/* Launch failures still produce a handle in the completed LAUNCH_FAILED state
 * whose only event is the completion event. */
static SimpAsyncProcess *fail_launch(SimpAsyncProcess *process, const char *prefix,
                                     const char *subject, int error_number) {
    if (error_number == 0) error_number = EIO;
    process->state = STATE_LAUNCH_FAILED;
    process->reaped = 1;
    process->completed = 1;
    record_error_locked(process, prefix, subject, error_number);
    (void)append_event_locked(process, EVENT_COMPLETED, NULL, 0);
    simp_runtime_set_error(error_number);
    return process;
}

#if defined(__GLIBC__) || defined(__APPLE__)
#define SIMP_HAVE_SPAWN_CHDIR 1
#endif

void *simp_process_async_start(void *self, void *executable, void *arguments,
                               void *working_directory) {
    (void)self;
    SimpAsyncProcess *process = allocate_process();
    if (process == NULL) {
        simp_runtime_set_error(ENOMEM);
        return NULL;
    }
    char *program = copy_string(executable);
    if (program == NULL || program[0] == '\0') {
        const int saved = program == NULL ? errno : EINVAL;
        free(program);
        return fail_launch(process, "invalid executable name", NULL, saved);
    }
    char *directory = NULL;
    if (working_directory != NULL) {
        directory = copy_string(working_directory);
        if (directory == NULL || directory[0] == '\0') {
            const int saved = directory == NULL ? errno : EINVAL;
            free(directory);
            free(program);
            return fail_launch(process, "invalid working directory", NULL, saved);
        }
        struct stat information;
        int reason = 0;
        if (stat(directory, &information) != 0) reason = errno == 0 ? ENOENT : errno;
        else if (!S_ISDIR(information.st_mode)) reason = ENOTDIR;
        if (reason != 0) {
            SimpAsyncProcess *failed =
                fail_launch(process, "cannot use working directory", directory, reason);
            free(directory);
            free(program);
            return failed;
        }
#ifndef SIMP_HAVE_SPAWN_CHDIR
        free(directory);
        free(program);
        return fail_launch(process, "working directories are unsupported on this platform", NULL,
                           ENOSYS);
#endif
    }
    if (arguments == NULL) {
        free(directory);
        free(program);
        return fail_launch(process, "invalid argument list", NULL, EINVAL);
    }
    const SimpArray *array = (const SimpArray *)arguments;
    if (array->length > SIZE_MAX / sizeof(char *) - 2) {
        free(directory);
        free(program);
        return fail_launch(process, "invalid argument list", NULL, E2BIG);
    }
    const size_t argument_count = (size_t)array->length;
    char **argv = (char **)calloc(argument_count + 2, sizeof(*argv));
    if (argv == NULL) {
        free(directory);
        free(program);
        return fail_launch(process, "invalid argument list", NULL, ENOMEM);
    }
    argv[0] = program;
    for (size_t index = 0; index < argument_count; ++index) {
        const SimpArrayValue *value = &array->values[index];
        if ((value->tag != SIMP_ARRAY_STRING && value->tag != SIMP_ARRAY_OBJECT) ||
            value->pointer == NULL) {
            free_arguments(argv, index + 1);
            free(directory);
            return fail_launch(process, "invalid argument list (arguments must be Strings)", NULL,
                               EINVAL);
        }
        argv[index + 1] = copy_string(value->pointer);
        if (argv[index + 1] == NULL) {
            const int saved = errno == 0 ? ENOMEM : errno;
            free_arguments(argv, index + 1);
            free(directory);
            return fail_launch(process, "invalid argument list", NULL, saved);
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
        free(directory);
        return fail_launch(process, "cannot create output pipes", NULL, saved);
    }

    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attributes;
    int status = posix_spawn_file_actions_init(&actions);
    const int actions_initialized = status == 0;
    if (status == 0) status = posix_spawnattr_init(&attributes);
    const int attributes_initialized = actions_initialized && status == 0;
    if (status == 0) {
        sigset_t defaults;
        sigset_t empty;
        (void)sigemptyset(&empty);
        (void)sigemptyset(&defaults);
        const int reset[] = {SIGPIPE, SIGINT, SIGQUIT, SIGTERM, SIGHUP, SIGCHLD,
                             SIGTSTP, SIGTTIN, SIGTTOU, SIGUSR1, SIGUSR2, SIGALRM};
        for (size_t index = 0; index < sizeof(reset) / sizeof(reset[0]); ++index) {
            (void)sigaddset(&defaults, reset[index]);
        }
        status = posix_spawnattr_setflags(
            &attributes, POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF);
        if (status == 0) status = posix_spawnattr_setpgroup(&attributes, 0);
        if (status == 0) status = posix_spawnattr_setsigmask(&attributes, &empty);
        if (status == 0) status = posix_spawnattr_setsigdefault(&attributes, &defaults);
    }
    if (status == 0) {
        status = posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    }
    if (status == 0) {
        status = posix_spawn_file_actions_adddup2(&actions, output_pipe[1], STDOUT_FILENO);
    }
    if (status == 0) {
        status = posix_spawn_file_actions_adddup2(&actions, error_pipe[1], STDERR_FILENO);
    }
#ifdef SIMP_HAVE_SPAWN_CHDIR
    if (status == 0 && directory != NULL) {
        status = posix_spawn_file_actions_addchdir_np(&actions, directory);
    }
#endif
    pid_t pid = -1;
    const int setup_failed = status != 0;
    if (status == 0) status = posix_spawnp(&pid, program, &actions, &attributes, argv, environ);
    if (attributes_initialized) (void)posix_spawnattr_destroy(&attributes);
    if (actions_initialized) (void)posix_spawn_file_actions_destroy(&actions);
    close_fd(&output_pipe[1]);
    close_fd(&error_pipe[1]);
    if (status != 0) {
        close_fd(&output_pipe[0]);
        close_fd(&error_pipe[0]);
        SimpAsyncProcess *failed =
            setup_failed ? fail_launch(process, "cannot prepare launch of", program, status)
                         : fail_launch(process, "cannot start", program, status);
        free_arguments(argv, argument_count + 1);
        free(directory);
        return failed;
    }
    process->pid = pid;
    process->stdout_fd = output_pipe[0];
    process->stderr_fd = error_pipe[0];

    /* The pump inherits a fully blocked mask so process signals are delivered
     * to application threads, never to this private native thread. */
    sigset_t all;
    sigset_t previous;
    (void)sigfillset(&all);
    (void)pthread_sigmask(SIG_SETMASK, &all, &previous);
    status = pthread_create(&process->pump, NULL, pump_main, process);
    (void)pthread_sigmask(SIG_SETMASK, &previous, NULL);
    if (status != 0) {
        (void)kill(-pid, SIGKILL);
        (void)kill(pid, SIGKILL);
        while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) {
        }
        close_fd(&process->stdout_fd);
        close_fd(&process->stderr_fd);
        process->pid = -1;
        SimpAsyncProcess *failed = fail_launch(process, "cannot start output thread for", program,
                                               status);
        free_arguments(argv, argument_count + 1);
        free(directory);
        return failed;
    }
    process->pump_started = 1;
    free_arguments(argv, argument_count + 1);
    free(directory);
    simp_runtime_clear_error();
    return process;
}

/* Every operation pins the object while the caller still holds the runtime
 * lock, so a concurrent close cannot free it underneath a blocked call. */
static void retain(SimpAsyncProcess *process) {
    (void)pthread_mutex_lock(&process->lock);
    ++process->references;
    (void)pthread_mutex_unlock(&process->lock);
}

static void release(SimpAsyncProcess *process) {
    (void)pthread_mutex_lock(&process->lock);
    const int remaining = --process->references;
    (void)pthread_mutex_unlock(&process->lock);
    if (remaining == 0) destroy_process(process);
}

/* Callers re-check their predicate; the runtime lock is never held here. */
static void wait_changed_locked(SimpAsyncProcess *process, int64_t timeout,
                                const struct timespec *deadline) {
    if (timeout < 0) (void)pthread_cond_wait(&process->changed, &process->lock);
    else (void)pthread_cond_timedwait(&process->changed, &process->lock, deadline);
}

static int deadline_passed(int64_t timeout, int64_t start) {
    return timeout >= 0 && monotonic_milliseconds() - start >= timeout;
}

void *simp_process_async_next(void *self, void *object, int64_t timeout) {
    (void)self;
    SimpAsyncProcess *process = (SimpAsyncProcess *)object;
    if (process == NULL) {
        simp_runtime_set_error(EINVAL);
        return NULL;
    }
    retain(process);
    SimpProcessEvent *event = NULL;
    (void)pthread_mutex_lock(&process->lock);
    if (process->head == NULL && timeout != 0 && !process->completion_taken && !process->closing) {
        (void)pthread_mutex_unlock(&process->lock);
        simp_runtime_gil_release();
        struct timespec deadline;
        const int64_t start = monotonic_milliseconds();
        if (timeout > 0) deadline_after(&deadline, timeout);
        (void)pthread_mutex_lock(&process->lock);
        while (process->head == NULL && !process->completion_taken && !process->closing &&
               !deadline_passed(timeout, start)) {
            wait_changed_locked(process, timeout, &deadline);
        }
        (void)pthread_mutex_unlock(&process->lock);
        simp_runtime_gil_acquire();
        (void)pthread_mutex_lock(&process->lock);
    }
    if (process->head != NULL) {
        event = process->head;
        process->head = event->next;
        if (process->head == NULL) process->tail = NULL;
        event->next = NULL;
        if (event->kind == EVENT_COMPLETED) process->completion_taken = 1;
    }
    (void)pthread_mutex_unlock(&process->lock);
    release(process);
    return event;
}

int64_t simp_process_async_event_kind(void *self, void *object) {
    (void)self;
    const SimpProcessEvent *event = (const SimpProcessEvent *)object;
    return event == NULL ? 0 : event->kind;
}

void *simp_process_async_event_data(void *self, void *object) {
    (void)self;
    const SimpProcessEvent *event = (const SimpProcessEvent *)object;
    if (event == NULL || event->kind == EVENT_COMPLETED) return NULL;
    SimpBuffer *buffer =
        (SimpBuffer *)simp_buffer_new((int64_t)event->length, "<stdlib>", 8, 0, 0);
    if (event->length > 0) memcpy(buffer->data, event->data, event->length);
    return buffer;
}

void simp_process_async_event_release(void *self, void *object) {
    (void)self;
    free(object);
}

int32_t simp_process_async_wait(void *self, void *object, int64_t timeout) {
    (void)self;
    SimpAsyncProcess *process = (SimpAsyncProcess *)object;
    if (process == NULL) {
        simp_runtime_set_error(EINVAL);
        return 0;
    }
    retain(process);
    (void)pthread_mutex_lock(&process->lock);
    int completed = process->completed;
    (void)pthread_mutex_unlock(&process->lock);
    if (!completed && timeout != 0) {
        simp_runtime_gil_release();
        struct timespec deadline;
        const int64_t start = monotonic_milliseconds();
        if (timeout > 0) deadline_after(&deadline, timeout);
        (void)pthread_mutex_lock(&process->lock);
        while (!process->completed && !process->closing && !deadline_passed(timeout, start)) {
            wait_changed_locked(process, timeout, &deadline);
        }
        completed = process->completed;
        (void)pthread_mutex_unlock(&process->lock);
        simp_runtime_gil_acquire();
    }
    release(process);
    return completed;
}

int32_t simp_process_async_cancel(void *self, void *object, int64_t grace) {
    (void)self;
    SimpAsyncProcess *process = (SimpAsyncProcess *)object;
    if (process == NULL || grace < 0) {
        simp_runtime_set_error(EINVAL);
        return 0;
    }
    int accepted = 0;
    (void)pthread_mutex_lock(&process->lock);
    if (!process->completed && !process->closing) {
        const int64_t now = monotonic_milliseconds();
        if (!process->cancel_requested) process->cancel_time = now;
        process->cancel_requested = 1;
        accepted = 1;
        if (!process->reaped) {
            if (!process->cancel_effective) {
                process->cancel_effective = 1;
                process->kill_deadline = now + grace;
                signal_child_locked(process, grace == 0 ? SIGKILL : SIGTERM);
                if (grace == 0) process->killed = 1;
            } else if (!process->killed && now + grace < process->kill_deadline) {
                process->kill_deadline = now + grace;
            }
        }
    }
    (void)pthread_mutex_unlock(&process->lock);
    simp_runtime_clear_error();
    return accepted;
}

static int read_int_locked(SimpAsyncProcess *process, const int *field) {
    (void)pthread_mutex_lock(&process->lock);
    const int value = process->completed ? *field : -2;
    (void)pthread_mutex_unlock(&process->lock);
    return value;
}

int64_t simp_process_async_state(void *self, void *object) {
    (void)self;
    SimpAsyncProcess *process = (SimpAsyncProcess *)object;
    if (process == NULL) return STATE_LAUNCH_FAILED;
    const int value = read_int_locked(process, &process->state);
    return value == -2 ? STATE_RUNNING : value;
}

int64_t simp_process_async_exit_code(void *self, void *object) {
    (void)self;
    SimpAsyncProcess *process = (SimpAsyncProcess *)object;
    if (process == NULL) return -1;
    const int value = read_int_locked(process, &process->exit_code);
    return value == -2 ? -1 : value;
}

int64_t simp_process_async_signal(void *self, void *object) {
    (void)self;
    SimpAsyncProcess *process = (SimpAsyncProcess *)object;
    if (process == NULL) return 0;
    const int value = read_int_locked(process, &process->signal_number);
    return value == -2 ? 0 : value;
}

void *simp_process_async_error(void *self, void *object) {
    (void)self;
    if (&simp_string_class_meta == NULL) abort();
    SimpAsyncProcess *process = (SimpAsyncProcess *)object;
    if (process == NULL) {
        static const char message[] = "process could not be allocated";
        return simp_string_new(&simp_string_class_meta, message, sizeof(message) - 1);
    }
    char *copy = NULL;
    size_t length = 0;
    (void)pthread_mutex_lock(&process->lock);
    if (process->error != NULL) {
        length = strlen(process->error);
        copy = (char *)malloc(length + 1);
        if (copy != NULL) memcpy(copy, process->error, length + 1);
    }
    (void)pthread_mutex_unlock(&process->lock);
    void *result = simp_string_new(&simp_string_class_meta, copy == NULL ? "" : copy,
                                   copy == NULL ? 0 : length);
    free(copy);
    return result;
}

/* Kills a still-running child immediately, abandons any output still held
 * open by descendants, and joins the pump. Safe to call concurrently and
 * repeatedly; afterwards the process is completed. */
void simp_process_async_shutdown(void *self, void *object) {
    (void)self;
    SimpAsyncProcess *process = (SimpAsyncProcess *)object;
    if (process == NULL) return;
    retain(process);
    (void)pthread_mutex_lock(&process->lock);
    if (!process->closing) {
        process->closing = 1;
        if (!process->reaped && process->pid > 0) {
            process->cancel_requested = 1;
            process->cancel_effective = 1;
            if (!process->killed) {
                signal_child_locked(process, SIGKILL);
                process->killed = 1;
            }
        }
        (void)pthread_cond_broadcast(&process->changed);
    }
    int join = 0;
    if (process->pump_started && !process->pump_joined && !process->pump_joining) {
        process->pump_joining = 1;
        join = 1;
    }
    const int wait_for_joiner = !join && process->pump_joining && !process->pump_joined;
    (void)pthread_mutex_unlock(&process->lock);
    if (join || wait_for_joiner) {
        simp_runtime_gil_release();
        if (join) {
            (void)pthread_join(process->pump, NULL);
            (void)pthread_mutex_lock(&process->lock);
            process->pump_joined = 1;
            (void)pthread_cond_broadcast(&process->changed);
            (void)pthread_mutex_unlock(&process->lock);
        } else {
            (void)pthread_mutex_lock(&process->lock);
            while (!process->pump_joined) (void)pthread_cond_wait(&process->changed, &process->lock);
            (void)pthread_mutex_unlock(&process->lock);
        }
        simp_runtime_gil_acquire();
    }
    release(process);
}

void simp_process_async_close(void *self, void *object) {
    (void)self;
    SimpAsyncProcess *process = (SimpAsyncProcess *)object;
    if (process == NULL) return;
    simp_process_async_shutdown(self, process);
    release(process);
}

/*
 * Scans UTF-8 using the WHATWG "maximal subpart" policy. When `out` is
 * non-NULL, valid sequences are copied and each maximal invalid subpart becomes
 * U+FFFD. Returns the length of a trailing truncated-but-valid prefix, which is
 * also replaced when `out` is non-NULL (callers pass only complete input then).
 */
static size_t scan_utf8(const uint8_t *bytes, size_t length, uint8_t *out, size_t *out_length) {
    static const uint8_t replacement[3] = {0xEF, 0xBF, 0xBD};
    size_t index = 0;
    size_t written = 0;
    size_t tail = 0;
    while (index < length) {
        uint8_t lead = bytes[index];
        size_t need = 0;
        uint8_t low = 0x80;
        uint8_t high = 0xBF;
        if (lead < 0x80) {
            if (out != NULL) out[written] = lead;
            written++;
            index++;
            continue;
        } else if (lead >= 0xC2 && lead <= 0xDF) {
            need = 1;
        } else if (lead == 0xE0) {
            need = 2;
            low = 0xA0;
        } else if ((lead >= 0xE1 && lead <= 0xEC) || lead == 0xEE || lead == 0xEF) {
            need = 2;
        } else if (lead == 0xED) {
            need = 2;
            high = 0x9F;
        } else if (lead == 0xF0) {
            need = 3;
            low = 0x90;
        } else if (lead >= 0xF1 && lead <= 0xF3) {
            need = 3;
        } else if (lead == 0xF4) {
            need = 3;
            high = 0x8F;
        }
        size_t taken = 1;
        int valid = need > 0;
        int truncated = 0;
        for (size_t k = 1; valid && k <= need; k++) {
            if (index + k >= length) {
                truncated = 1;
                valid = 0;
                break;
            }
            uint8_t next = bytes[index + k];
            uint8_t min = k == 1 ? low : 0x80;
            uint8_t max = k == 1 ? high : 0xBF;
            if (next < min || next > max) {
                valid = 0;
                break;
            }
            taken = k + 1;
        }
        if (truncated) {
            tail = length - index;
            taken = tail;
        }
        if (valid) {
            if (out != NULL) memcpy(out + written, bytes + index, taken);
            written += taken;
        } else {
            if (out != NULL) memcpy(out + written, replacement, sizeof(replacement));
            written += sizeof(replacement);
        }
        index += taken;
    }
    if (out_length != NULL) *out_length = written;
    return tail;
}

void *simp_process_text_decode(void *self, void *bytes) {
    (void)self;
    if (&simp_string_class_meta == NULL) abort();
    const SimpBuffer *buffer = (const SimpBuffer *)bytes;
    if (buffer == NULL || buffer->length == 0) {
        return simp_string_new(&simp_string_class_meta, "", 0);
    }
    size_t length = (size_t)buffer->length;
    size_t decoded_length = 0;
    scan_utf8(buffer->data, length, NULL, &decoded_length);
    if (decoded_length == length) {
        return simp_string_new(&simp_string_class_meta, (const char *)buffer->data, length);
    }
    uint8_t *decoded = malloc(decoded_length);
    if (decoded == NULL) {
        static const char message[] = "out of memory decoding process output";
        simp_exception_raise(message, sizeof(message) - 1, "<native process>", 16, 0, 0);
        return NULL;
    }
    scan_utf8(buffer->data, length, decoded, &decoded_length);
    void *text = simp_string_new(&simp_string_class_meta, (const char *)decoded, decoded_length);
    free(decoded);
    return text;
}

int64_t simp_process_text_incomplete_tail(void *self, void *bytes) {
    (void)self;
    const SimpBuffer *buffer = (const SimpBuffer *)bytes;
    if (buffer == NULL || buffer->length == 0) return 0;
    return (int64_t)scan_utf8(buffer->data, (size_t)buffer->length, NULL, NULL);
}
