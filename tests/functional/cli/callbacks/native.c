#include <simp/Callbacks.h>
#include <simp/RuntimeGc.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

typedef int64_t (*IntCallback)(SimpCallbackContext *, int64_t);
typedef struct Registration {
    SimpCallbackContext *context;
    IntCallback function;
} Registration;

void *fixture_callback_register(void *self, void *callback) {
    (void)self;
    Registration *registration = malloc(sizeof(*registration));
    if (!registration) abort();
    registration->context = simp_callback_acquire(callback, "callback<int(int)>");
    registration->function = (IntCallback)simp_callback_adapter(registration->context);
    return registration;
}

int64_t fixture_callback_invoke(void *self, void *handle, int64_t value) {
    (void)self;
    Registration *registration = handle;
    return registration->function(registration->context, value);
}

void fixture_callback_release(void *self, void *handle) {
    (void)self;
    Registration *registration = handle;
    simp_callback_release(registration->context);
}

void fixture_callback_dispose(void *self, void *handle) {
    (void)self;
    Registration *registration = handle;
    simp_callback_dispose(registration->context);
    free(registration);
}

void fixture_callback_collect(void *self) {
    (void)self;
    simp_gc_collect();
}

static void *foreign_invoke(void *handle) {
    fixture_callback_invoke(NULL, handle, 1);
    return NULL;
}

void fixture_callback_foreign(void *self, void *handle) {
    (void)self;
    pthread_t thread;
    if (pthread_create(&thread, NULL, foreign_invoke, handle) != 0) abort();
    pthread_join(thread, NULL);
}

static void *registered_invoke(void *handle) {
    simp_runtime_thread_enter();
    fixture_callback_invoke(NULL, handle, 1);
    simp_runtime_thread_exit();
    return NULL;
}

void fixture_callback_registered_foreign(void *self, void *handle) {
    (void)self;
    pthread_t thread;
    simp_runtime_gil_release();
    if (pthread_create(&thread, NULL, registered_invoke, handle) != 0) abort();
    pthread_join(thread, NULL);
    simp_runtime_gil_acquire();
}

void fixture_callback_unlocked(void *self, void *handle) {
    (void)self;
    simp_runtime_gil_release();
    fixture_callback_invoke(NULL, handle, 1);
    simp_runtime_gil_acquire();
}

void *fixture_callback_transfer(void *self, void *callback) {
    (void)self;
    return simp_callback_transfer_prepare(callback, "callback<int(int)>");
}

void *fixture_callback_accept(void *self, void *transfer) {
    (void)self;
    Registration *registration = malloc(sizeof(*registration));
    if (!registration) abort();
    registration->context = simp_callback_transfer_accept(transfer);
    registration->function = (IntCallback)simp_callback_adapter(registration->context);
    return registration;
}

void fixture_callback_cancel(void *self, void *transfer) {
    (void)self;
    simp_callback_transfer_cancel(transfer);
}

static void *accept_on_worker(void *transfer) {
    simp_runtime_thread_enter();
    void *registration = fixture_callback_accept(NULL, transfer);
    if (fixture_callback_invoke(NULL, registration, 40) != 42) abort();
    fixture_callback_collect(NULL);
    fixture_callback_release(NULL, registration);
    fixture_callback_dispose(NULL, registration);
    simp_runtime_thread_exit();
    return NULL;
}

void fixture_callback_transfer_worker(void *self, void *transfer) {
    (void)self;
    pthread_t thread;
    simp_runtime_gil_release();
    if (pthread_create(&thread, NULL, accept_on_worker, transfer)) abort();
    pthread_join(thread, NULL);
    simp_runtime_gil_acquire();
}

static void *unregistered_enter(void *unused) {
    (void)unused;
    simp_runtime_managed_enter();
    return NULL;
}

void fixture_callback_unregistered_enter(void *self) {
    (void)self;
    pthread_t thread;
    if (pthread_create(&thread, NULL, unregistered_enter, NULL)) abort();
    pthread_join(thread, NULL);
}
