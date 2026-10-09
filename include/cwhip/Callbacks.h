#ifndef CWHIP_CALLBACKS_H
#define CWHIP_CALLBACKS_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct CwhipCallbackContext CwhipCallbackContext;
typedef struct CwhipCallbackTransfer CwhipCallbackTransfer;
typedef void (*CwhipCallbackAdapter)(void);
typedef union CwhipCallbackArgument {
    int64_t integer;
    uint64_t unsigned_integer;
    double floating;
    uint8_t boolean;
    void *pointer;
} CwhipCallbackArgument;
typedef void (*CwhipCallbackInvoker)(void *, const CwhipCallbackArgument *,
                                    CwhipCallbackArgument *);

/* Adapter's exact signature is R(CwhipCallbackContext *, P...).
 * Read doc/CALLBACKS.md before using this API. */
CwhipCallbackContext *cwhip_callback_acquire(void *callback, const char *signature);
CwhipCallbackAdapter cwhip_callback_adapter(CwhipCallbackContext *context);
const char *cwhip_callback_signature(CwhipCallbackContext *context);
void cwhip_callback_release(CwhipCallbackContext *context);
void cwhip_callback_dispose(CwhipCallbackContext *context);
void cwhip_callback_context_invoke(CwhipCallbackContext *context, const char *signature,
                                  const CwhipCallbackArgument *arguments,
                                  CwhipCallbackArgument *result);

/* One-shot rooted transport, not an invocable registration. All operations
 * require a registered thread holding the managed lock. Accept acquires a
 * fresh registration owned by the accepting thread. Accept/cancel consume
 * the transfer; the package must synchronize publication and consumption. */
CwhipCallbackTransfer *cwhip_callback_transfer_prepare(void *callback, const char *signature);
CwhipCallbackContext *cwhip_callback_transfer_accept(CwhipCallbackTransfer *transfer);
void cwhip_callback_transfer_cancel(CwhipCallbackTransfer *transfer);

/* Compiler-generated bridges; not Cwhip method symbols. */
void *cwhip_callback_new(void *receiver, void *code, CwhipCallbackAdapter adapter,
                        CwhipCallbackInvoker invoker, const char *signature,
                        uint64_t argument_count, const uint8_t *managed_arguments);
void *cwhip_callback_receiver(void *callback);
void *cwhip_callback_code(void *callback);

#ifdef __cplusplus
}
#endif
#endif
