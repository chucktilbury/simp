#include "cwhip/Stdlib.h"
#include "cwhip/Stdlib.h"

#include <stddef.h>

int main(void) {
    return cwhip_math_sqrt(NULL, 9.0) == 3.0 &&
           cwhip_time_epoch_seconds(NULL) > 0 &&
           cwhip_process_async_state(NULL, NULL) == 4 &&
           cwhip_process_text_incomplete_tail(NULL, NULL) == 0 ? 0 : 1;
}
