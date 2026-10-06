#ifndef SIMP_CALLBACKS_H
#define SIMP_CALLBACKS_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct SimpCallbackContext SimpCallbackContext;
typedef void (*SimpCallbackAdapter)(void);
typedef union SimpCallbackArgument {
    int64_t integer;
    uint64_t unsigned_integer;
    double floating;
    uint8_t boolean;
    void *pointer;
} SimpCallbackArgument;
typedef void (*SimpCallbackInvoker)(void *, const SimpCallbackArgument *,
                                    SimpCallbackArgument *);

/* Adapter's exact signature is R(SimpCallbackContext *, P...).
 * Read doc/CALLBACKS.md before using this API. */
SimpCallbackContext *simp_callback_acquire(void *callback, const char *signature);
SimpCallbackAdapter simp_callback_adapter(SimpCallbackContext *context);
const char *simp_callback_signature(SimpCallbackContext *context);
void simp_callback_release(SimpCallbackContext *context);
void simp_callback_dispose(SimpCallbackContext *context);
void simp_callback_context_invoke(SimpCallbackContext *context, const char *signature,
                                  const SimpCallbackArgument *arguments,
                                  SimpCallbackArgument *result);

/* Compiler-generated bridges; not Simple method symbols. */
void *simp_callback_new(void *receiver, void *code, SimpCallbackAdapter adapter,
                        SimpCallbackInvoker invoker, const char *signature,
                        uint64_t argument_count, const uint8_t *managed_arguments);
void *simp_callback_receiver(void *callback);
void *simp_callback_code(void *callback);

#ifdef __cplusplus
}
#endif
#endif
