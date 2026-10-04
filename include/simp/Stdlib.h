/**
 * @file Stdlib.h
 * @brief Supported opaque C facade for the shipped Simple standard library.
 *
 * See doc/STDLIB.md for ownership, rooting, errors, and the package mapping.
 * The receiver argument is reserved; pass NULL. Managed results must be stored
 * in a captured Simple reference slot before another allocating call.
 */
#ifndef SIMP_STDLIB_H
#define SIMP_STDLIB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *simp_string_cstr(void *const *text);
void simp_string_bytes(void *object, const char **data, uint64_t *length);

int64_t simp_system_argc(void *self);
void *simp_system_argv(void *self);
void *simp_system_arg(void *self, int64_t index);
void simp_system_exit(void *self, int64_t code);
void simp_system_abort(void *self);
void *simp_system_getenv(void *self, void *name);
int32_t simp_system_setenv(void *self, void *name, void *value);
void *simp_system_last_error(void *self);

int32_t simp_fs_exists(void *self, void *path);
int32_t simp_fs_is_file(void *self, void *path);
int32_t simp_fs_is_dir(void *self, void *path);
int64_t simp_fs_file_size(void *self, void *path);
int32_t simp_fs_remove(void *self, void *path);
int32_t simp_fs_rename(void *self, void *old_path, void *new_path);
int32_t simp_fs_copy(void *self, void *source, void *destination);
int32_t simp_fs_mkdir(void *self, void *path);
int32_t simp_fs_rmdir(void *self, void *path);
void *simp_fs_list_dir(void *self, void *path);
void *simp_fs_get_cwd(void *self);
int32_t simp_fs_ch_dir(void *self, void *path);
void *simp_fs_absolute_path(void *self, void *path);
void *simp_path_join(void *self, void *left, void *right);
void *simp_path_normalize(void *self, void *path);
void *simp_path_basename(void *self, void *path);
void *simp_path_dirname(void *self, void *path);
void *simp_path_extension(void *self, void *path);
void *simp_fs_temp_file(void *self);
void *simp_fs_temp_dir(void *self);
void *simp_glob_glob(void *self, void *pattern);

void *simp_file_open(void *self, void *path, void *mode);
void *simp_file_read(void *self, void *handle, int64_t size);
void *simp_file_read_all(void *self, void *handle);
void *simp_file_read_line(void *self, void *handle);
void *simp_file_read_lines(void *self, void *handle);
int64_t simp_file_write(void *self, void *handle, void *data);
int64_t simp_file_write_line(void *self, void *handle, void *line);
int64_t simp_file_seek(void *self, void *handle, int64_t offset, int64_t whence);
int64_t simp_file_tell(void *self, void *handle);
void simp_file_flush(void *self, void *handle);
void simp_file_close(void *self, void *handle);
int32_t simp_file_eof(void *self, void *handle);

void *simp_stdio_read(void *self, int64_t size);
void *simp_stdio_read_line(void *self);
int64_t simp_stdio_write(void *self, void *text);
int64_t simp_stdio_write_line(void *self, void *text);
int64_t simp_stdio_write_bytes(void *self, void *buffer);
int64_t simp_stdio_write_error(void *self, void *text);
int64_t simp_stdio_write_error_line(void *self, void *text);
int64_t simp_stdio_write_error_bytes(void *self, void *buffer);
void simp_stdio_flush(void *self);
void simp_stdio_flush_error(void *self);

double simp_math_abs(void *self, double x);
int64_t simp_math_abs_int(void *self, int64_t x);
double simp_math_min(void *self, double a, double b);
double simp_math_max(void *self, double a, double b);
int64_t simp_math_min_int(void *self, int64_t a, int64_t b);
int64_t simp_math_max_int(void *self, int64_t a, int64_t b);
double simp_math_clamp(void *self, double x, double min_value, double max_value);
double simp_math_floor(void *self, double x);
double simp_math_ceil(void *self, double x);
double simp_math_round(void *self, double x);
double simp_math_trunc(void *self, double x);
double simp_math_sqrt(void *self, double x);
double simp_math_cbrt(void *self, double x);
double simp_math_pow(void *self, double base, double exponent);
double simp_math_exp(void *self, double x);
double simp_math_log(void *self, double x);
double simp_math_log10(void *self, double x);
double simp_math_log2(void *self, double x);
double simp_math_sin(void *self, double x);
double simp_math_cos(void *self, double x);
double simp_math_tan(void *self, double x);
double simp_math_asin(void *self, double x);
double simp_math_acos(void *self, double x);
double simp_math_atan(void *self, double x);
double simp_math_atan2(void *self, double y, double x);
double simp_math_degrees(void *self, double radians);
double simp_math_radians(void *self, double degrees);
double simp_math_sinh(void *self, double x);
double simp_math_cosh(void *self, double x);
double simp_math_tanh(void *self, double x);

int64_t simp_net_socket_create(void *self);
int32_t simp_net_socket_connect(void *self, int64_t fd, void *host, int64_t port);
int64_t simp_net_socket_send(void *self, int64_t fd, void *buffer);
int64_t simp_net_socket_send_string(void *self, int64_t fd, void *text);
void *simp_net_socket_recv(void *self, int64_t fd, int64_t max_bytes);
void *simp_net_socket_recv_string(void *self, int64_t fd, int64_t max_bytes);
void simp_net_socket_close(void *self, int64_t fd);
void simp_net_socket_set_timeout(void *self, int64_t fd, int64_t milliseconds);
int64_t simp_net_server_bind(void *self, void *host, int64_t port, int64_t backlog);
int64_t simp_net_server_accept(void *self, int64_t server_fd);
int64_t simp_net_server_port(void *self, int64_t server_fd);
int32_t simp_net_server_listen(void *self, int64_t server_fd, int64_t backlog);

uint64_t simp_time_epoch_seconds(void *self);
uint64_t simp_time_epoch_milliseconds(void *self);
uint64_t simp_time_monotonic_milliseconds(void *self);
int32_t simp_time_sleep_milliseconds(void *self, uint64_t milliseconds);

int32_t simp_terminal_stdin_interactive(void *self);
int32_t simp_terminal_stdout_interactive(void *self);
int64_t simp_terminal_columns(void *self);
int64_t simp_terminal_rows(void *self);
int32_t simp_terminal_supports_color(void *self);

int32_t simp_random_fill(void *self, void *buffer);
void *simp_random_bytes(void *self, int64_t size);

void *simp_process_spawn(void *self, void *executable, void *arguments);
int32_t simp_process_wait(void *self, void *process);
int64_t simp_process_exit_code(void *self, void *process);
void *simp_process_stdout(void *self, void *process);
void *simp_process_stderr(void *self, void *process);
void simp_process_close(void *self, void *process);

void *simp_mutex_create(void *receiver);
int32_t simp_mutex_lock(void *receiver, void *mutex);
int32_t simp_mutex_unlock(void *receiver, void *mutex);
int32_t simp_mutex_release(void *receiver, void *mutex);
void *simp_condition_create(void *receiver);
int32_t simp_condition_wait(void *receiver, void *condition, void *mutex);
int32_t simp_condition_signal(void *receiver, void *condition);
int32_t simp_condition_broadcast(void *receiver, void *condition);
int32_t simp_condition_release(void *receiver, void *condition);
void *simp_semaphore_create(void *receiver, int64_t initial_count);
void simp_semaphore_wait(void *receiver, void *semaphore);
void simp_semaphore_signal(void *receiver, void *semaphore);
void simp_semaphore_release(void *receiver, void *semaphore);

#ifdef __cplusplus
}
#endif
#endif
