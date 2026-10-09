#ifndef CWHIP_GTK_PRIVATE_H
#define CWHIP_GTK_PRIVATE_H

#include <gtk/gtk.h>
#include <stdint.h>
#include <stdbool.h>

/* Package-local signal contract, not installed as a
 * runtime header. Objects are borrowed; connections own only callback roots. */
void cwhip_gtk_require_owner(void);
int64_t cwhip_gtk_connect_clicked(GtkButton *button, void *callback);
int64_t cwhip_gtk_connect_changed(GtkEditable *editable, void *callback);
int64_t cwhip_gtk_connect_close_request(GtkWindow *window, void *callback);
bool cwhip_gtk_disconnect(int64_t token);

void cwhip_gtk_initialize(void *self);
void cwhip_gtk_run(void *self);
void cwhip_gtk_quit(void *self);
void cwhip_gtk_shutdown(void *self);
int64_t cwhip_gtk_post(void *self, void *callback);
bool cwhip_gtk_cancel(void *self, int64_t token);

#endif
