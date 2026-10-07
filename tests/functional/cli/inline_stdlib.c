#include "simp/Stdlib.h"
#include "simp/Stdlib.h"

#include <stddef.h>

int main(void) {
    return simp_math_sqrt(NULL, 9.0) == 3.0 &&
           simp_time_epoch_seconds(NULL) > 0 &&
           simp_process_async_state(NULL, NULL) == 4 &&
           simp_process_text_incomplete_tail(NULL, NULL) == 0 ? 0 : 1;
}
