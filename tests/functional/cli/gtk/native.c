#include "gtk_private.h"
#include <simp/RuntimeGc.h>
#include <simp/RuntimeThreads.h>
#include <simp/Callbacks.h>
#include <pthread.h>
#include <stdlib.h>

void *fixture_gtk_button(void *self) {
    (void)self;
    simp_gtk_require_owner();
    return g_object_ref_sink(gtk_button_new());
}

void *fixture_gtk_entry(void *self) {
    (void)self;
    simp_gtk_require_owner();
    return g_object_ref_sink(gtk_entry_new());
}

void *fixture_gtk_window(void *self) {
    (void)self;
    simp_gtk_require_owner();
    return g_object_ref(gtk_window_new());
}

int64_t fixture_gtk_clicked(void *self, void *button, void *callback) {
    (void)self;
    return simp_gtk_connect_clicked(GTK_BUTTON(button), callback);
}

int64_t fixture_gtk_changed(void *self, void *entry, void *callback) {
    (void)self;
    return simp_gtk_connect_changed(GTK_EDITABLE(entry), callback);
}

int64_t fixture_gtk_close(void *self, void *window, void *callback) {
    (void)self;
    return simp_gtk_connect_close_request(GTK_WINDOW(window), callback);
}

bool fixture_gtk_disconnect(void *self, int64_t token) {
    (void)self;
    return simp_gtk_disconnect(token);
}

void fixture_gtk_emit(void *self, void *button) {
    (void)self;
    simp_gtk_require_owner();
    g_signal_emit_by_name(button, "clicked");
}

void fixture_gtk_text(void *self, void *entry) {
    (void)self;
    simp_gtk_require_owner();
    gtk_editable_set_text(GTK_EDITABLE(entry), "updated");
}

bool fixture_gtk_is_updated(void *self, void *entry) {
    (void)self;
    simp_gtk_require_owner();
    return g_str_equal(gtk_editable_get_text(GTK_EDITABLE(entry)), "updated");
}

bool fixture_gtk_request(void *self, void *window) {
    (void)self;
    simp_gtk_require_owner();
    gboolean result = FALSE;
    g_signal_emit_by_name(window, "close-request", &result);
    return result != FALSE;
}

void fixture_gtk_drop(void *self, void *widget) {
    (void)self;
    simp_gtk_require_owner();
    if (GTK_IS_WINDOW(widget)) gtk_window_destroy(GTK_WINDOW(widget));
    g_object_unref(widget);
}

void fixture_gtk_collect(void *self) {
    (void)self;
    simp_gc_collect();
}

void fixture_gtk_owner(void *self) {
    (void)self;
    simp_gtk_require_owner();
}

static int64_t destroyed;
void fixture_gtk_destroyed(void *self) { (void)self; ++destroyed; }
int64_t fixture_gtk_count(void *self) { (void)self; return destroyed; }

static gboolean unblock(gpointer semaphore) {
    simp_gtk_require_owner();
    int acquired = simp_runtime_managed_enter();
    simp_semaphore_signal(NULL, semaphore);
    simp_runtime_managed_leave(acquired);
    return G_SOURCE_REMOVE;
}

void fixture_gtk_unblock(void *self, void *semaphore) {
    (void)self;
    simp_gtk_require_owner();
    g_timeout_add(40, unblock, semaphore);
}

static void *foreign(void *unused) {
    (void)unused;
    simp_gtk_require_owner();
    return NULL;
}

void fixture_gtk_foreign(void *self) {
    (void)self;
    pthread_t thread;
    if (pthread_create(&thread, NULL, foreign, NULL)) abort();
    pthread_join(thread, NULL);
}

void fixture_gtk_unlocked_post(void *self, void *callback) {
    (void)self;
    simp_runtime_gil_release();
    simp_gtk_post(NULL, callback);
}
