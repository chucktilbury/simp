/**
 * @file Stdlib.h
 * @brief Supported opaque C facade for the shipped Cwhip standard library.
 *
 * See doc/STDLIB.md for ownership, rooting, errors, and the package mapping.
 * The receiver argument is reserved; pass NULL. Managed results must be stored
 * in a captured Cwhip reference slot before another allocating call.
 */
#ifndef CWHIP_STDLIB_H
#define CWHIP_STDLIB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *cwhip_string_cstr(void *const *text);
void cwhip_string_bytes(void *object, const char **data, uint64_t *length);

int64_t cwhip_system_argc(void *self);
void *cwhip_system_argv(void *self);
void *cwhip_system_arg(void *self, int64_t index);
void cwhip_system_exit(void *self, int64_t code);
void cwhip_system_abort(void *self);
void *cwhip_system_getenv(void *self, void *name);
int32_t cwhip_system_setenv(void *self, void *name, void *value);
void *cwhip_system_last_error(void *self);

int32_t cwhip_fs_exists(void *self, void *path);
int32_t cwhip_fs_is_file(void *self, void *path);
int32_t cwhip_fs_is_dir(void *self, void *path);
int64_t cwhip_fs_file_size(void *self, void *path);
int32_t cwhip_fs_remove(void *self, void *path);
int32_t cwhip_fs_rename(void *self, void *old_path, void *new_path);
int32_t cwhip_fs_copy(void *self, void *source, void *destination);
int32_t cwhip_fs_mkdir(void *self, void *path);
int32_t cwhip_fs_rmdir(void *self, void *path);
void *cwhip_fs_list_dir(void *self, void *path);
void *cwhip_fs_get_cwd(void *self);
int32_t cwhip_fs_ch_dir(void *self, void *path);
void *cwhip_fs_absolute_path(void *self, void *path);
void *cwhip_path_join(void *self, void *left, void *right);
void *cwhip_path_normalize(void *self, void *path);
void *cwhip_path_basename(void *self, void *path);
void *cwhip_path_dirname(void *self, void *path);
void *cwhip_path_extension(void *self, void *path);
void *cwhip_fs_temp_file(void *self);
void *cwhip_fs_temp_dir(void *self);
void *cwhip_glob_glob(void *self, void *pattern);

void *cwhip_file_open(void *self, void *path, void *mode);
void *cwhip_file_read(void *self, void *handle, int64_t size);
void *cwhip_file_read_all(void *self, void *handle);
void *cwhip_file_read_line(void *self, void *handle);
void *cwhip_file_read_lines(void *self, void *handle);
int64_t cwhip_file_write(void *self, void *handle, void *data);
int64_t cwhip_file_write_line(void *self, void *handle, void *line);
int64_t cwhip_file_seek(void *self, void *handle, int64_t offset, int64_t whence);
int64_t cwhip_file_tell(void *self, void *handle);
void cwhip_file_flush(void *self, void *handle);
void cwhip_file_close(void *self, void *handle);
int32_t cwhip_file_eof(void *self, void *handle);

void *cwhip_stdio_read(void *self, int64_t size);
void *cwhip_stdio_read_line(void *self);
int64_t cwhip_stdio_write(void *self, void *text);
int64_t cwhip_stdio_write_line(void *self, void *text);
int64_t cwhip_stdio_write_bytes(void *self, void *buffer);
int64_t cwhip_stdio_write_error(void *self, void *text);
int64_t cwhip_stdio_write_error_line(void *self, void *text);
int64_t cwhip_stdio_write_error_bytes(void *self, void *buffer);
void cwhip_stdio_flush(void *self);
void cwhip_stdio_flush_error(void *self);

double cwhip_math_abs(void *self, double x);
int64_t cwhip_math_abs_int(void *self, int64_t x);
double cwhip_math_min(void *self, double a, double b);
double cwhip_math_max(void *self, double a, double b);
int64_t cwhip_math_min_int(void *self, int64_t a, int64_t b);
int64_t cwhip_math_max_int(void *self, int64_t a, int64_t b);
double cwhip_math_clamp(void *self, double x, double min_value, double max_value);
double cwhip_math_floor(void *self, double x);
double cwhip_math_ceil(void *self, double x);
double cwhip_math_round(void *self, double x);
double cwhip_math_trunc(void *self, double x);
double cwhip_math_sqrt(void *self, double x);
double cwhip_math_cbrt(void *self, double x);
double cwhip_math_pow(void *self, double base, double exponent);
double cwhip_math_exp(void *self, double x);
double cwhip_math_log(void *self, double x);
double cwhip_math_log10(void *self, double x);
double cwhip_math_log2(void *self, double x);
double cwhip_math_sin(void *self, double x);
double cwhip_math_cos(void *self, double x);
double cwhip_math_tan(void *self, double x);
double cwhip_math_asin(void *self, double x);
double cwhip_math_acos(void *self, double x);
double cwhip_math_atan(void *self, double x);
double cwhip_math_atan2(void *self, double y, double x);
double cwhip_math_degrees(void *self, double radians);
double cwhip_math_radians(void *self, double degrees);
double cwhip_math_sinh(void *self, double x);
double cwhip_math_cosh(void *self, double x);
double cwhip_math_tanh(void *self, double x);

int64_t cwhip_net_socket_create(void *self);
int32_t cwhip_net_socket_connect(void *self, int64_t fd, void *host, int64_t port);
int64_t cwhip_net_socket_send(void *self, int64_t fd, void *buffer);
int64_t cwhip_net_socket_send_string(void *self, int64_t fd, void *text);
void *cwhip_net_socket_recv(void *self, int64_t fd, int64_t max_bytes);
void *cwhip_net_socket_recv_string(void *self, int64_t fd, int64_t max_bytes);
void cwhip_net_socket_close(void *self, int64_t fd);
void cwhip_net_socket_set_timeout(void *self, int64_t fd, int64_t milliseconds);
int64_t cwhip_net_server_bind(void *self, void *host, int64_t port, int64_t backlog);
int64_t cwhip_net_server_accept(void *self, int64_t server_fd);
int64_t cwhip_net_server_port(void *self, int64_t server_fd);
int32_t cwhip_net_server_listen(void *self, int64_t server_fd, int64_t backlog);

uint64_t cwhip_time_epoch_seconds(void *self);
uint64_t cwhip_time_epoch_milliseconds(void *self);
uint64_t cwhip_time_monotonic_milliseconds(void *self);
int32_t cwhip_time_sleep_milliseconds(void *self, uint64_t milliseconds);

int32_t cwhip_terminal_stdin_interactive(void *self);
int32_t cwhip_terminal_stdout_interactive(void *self);
int64_t cwhip_terminal_columns(void *self);
int64_t cwhip_terminal_rows(void *self);
int32_t cwhip_terminal_supports_color(void *self);

int32_t cwhip_random_fill(void *self, void *buffer);
void *cwhip_random_bytes(void *self, int64_t size);

void *cwhip_process_spawn(void *self, void *executable, void *arguments);
int32_t cwhip_process_wait(void *self, void *process);
int64_t cwhip_process_exit_code(void *self, void *process);
void *cwhip_process_stdout(void *self, void *process);
void *cwhip_process_stderr(void *self, void *process);
void cwhip_process_close(void *self, void *process);

/* Asynchronous processes (POSIX). See doc/STDLIB.md "process". A handle must
 * be closed exactly once; events returned by next must be released. */
void *cwhip_process_async_start(void *self, void *executable, void *arguments,
                               void *working_directory);
void *cwhip_process_async_next(void *self, void *process, int64_t timeout_milliseconds);
int64_t cwhip_process_async_event_kind(void *self, void *event);
void *cwhip_process_async_event_data(void *self, void *event);
void cwhip_process_async_event_release(void *self, void *event);
int32_t cwhip_process_async_wait(void *self, void *process, int64_t timeout_milliseconds);
int32_t cwhip_process_async_cancel(void *self, void *process, int64_t grace_milliseconds);
int64_t cwhip_process_async_state(void *self, void *process);
int64_t cwhip_process_async_exit_code(void *self, void *process);
int64_t cwhip_process_async_signal(void *self, void *process);
void *cwhip_process_async_error(void *self, void *process);
void cwhip_process_async_shutdown(void *self, void *process);
void cwhip_process_async_close(void *self, void *process);
void *cwhip_process_text_decode(void *self, void *bytes);
int64_t cwhip_process_text_incomplete_tail(void *self, void *bytes);

void *cwhip_mutex_create(void *receiver);
int32_t cwhip_mutex_lock(void *receiver, void *mutex);
int32_t cwhip_mutex_unlock(void *receiver, void *mutex);
int32_t cwhip_mutex_release(void *receiver, void *mutex);
void *cwhip_condition_create(void *receiver);
int32_t cwhip_condition_wait(void *receiver, void *condition, void *mutex);
int32_t cwhip_condition_signal(void *receiver, void *condition);
int32_t cwhip_condition_broadcast(void *receiver, void *condition);
int32_t cwhip_condition_release(void *receiver, void *condition);
void *cwhip_semaphore_create(void *receiver, int64_t initial_count);
void cwhip_semaphore_wait(void *receiver, void *semaphore);
void cwhip_semaphore_signal(void *receiver, void *semaphore);
void cwhip_semaphore_release(void *receiver, void *semaphore);

#ifdef __cplusplus
}
#endif
#endif
