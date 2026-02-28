#include "clock.h"
#include <stdio.h>

Clock::Clock(uint32_t freq) : freq(freq) {
    bit_counter = -1;
    last_start_ms = 0;
}

Clock::~Clock() {

}

void Clock::put_telegram(uint32_t start, uint32_t len) {
    uint32_t start_ms = start / (freq / 1000);
    uint32_t len_ms = len / (freq / 1000);

    if (last_start_ms == 0) {
        last_start_ms = start_ms;
        return;
    }

    printf("start_ms = %d\n", start_ms);
    printf("last_start_ms = %d\n", last_start_ms);
    printf("diff = %d\n", start_ms - last_start_ms);

    if (last_start_ms > 0 && start_ms - last_start_ms > 1800) {
        printf("SYNC59\n");
        // 1-minute-datagram complete
        if (bit_counter == 58) {
            printf("minute datagram complete\n");
            for (int i = 0; i < 59; i++) {
                last_complete_datagram[i] = bits[i];
            }
        }

        bit_counter = 0;
    } else if (bit_counter >= 0) {
        bit_counter++;
    }

    if (bit_counter >= 60) {
        bit_counter = -1;
    } else if (bit_counter >= 0) {
        bool bit = len_ms > 150;
        printf("clock got %d bit[%d] = %d\n", len_ms, bit_counter, bit);
        bits[bit_counter] = bit;
    }

    last_start_ms = start_ms;
}