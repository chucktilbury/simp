#include "simp/Stdlib.h"
#include "simp/Stdlib.h"

#include <stddef.h>

int main(void) {
    return simp_math_sqrt(NULL, 9.0) == 3.0 &&
           simp_time_epoch_seconds(NULL) > 0 ? 0 : 1;
}
