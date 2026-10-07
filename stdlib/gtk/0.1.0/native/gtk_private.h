#ifndef SIMP_GTK_PRIVATE_H
#define SIMP_GTK_PRIVATE_H

#include <gtk/gtk.h>
#include <stdint.h>
#include <stdbool.h>

/* Package-local signal contract, not installed as a
 * runtime header. Objects are borrowed; connections own only callback roots. */
void simp_gtk_require_owner(void);
int64_t simp_gtk_connect_clicked(GtkButton *button, void *callback);
int64_t simp_gtk_connect_changed(GtkEditable *editable, void *callback);
int64_t simp_gtk_connect_close_request(GtkWindow *window, void *callback);
bool simp_gtk_disconnect(int64_t token);

void simp_gtk_initialize(void *self);
void simp_gtk_run(void *self);
void simp_gtk_quit(void *self);
void simp_gtk_shutdown(void *self);
int64_t simp_gtk_post(void *self, void *callback);
bool simp_gtk_cancel(void *self, int64_t token);

#endif
