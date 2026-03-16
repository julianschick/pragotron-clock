#include "sync.h"

void IRAM_ATTR Sync::inputLevelChangedISR() {
    int v = digitalRead(DCF_IN);
    if (v) {
        up_millis = millis();
        down_millis_pending = false;
        up_millis_pending = true;
        if (timer1_enabled()) {
            timer1_at_flank_up = timer1_read();
        }
    } else {
        down_millis = millis();
        down_millis_pending = true;
    }
}

void IRAM_ATTR Sync::timerFiredISR() {
    int8_t sign = timer1_at_flank_up < TIM1_HALFSECOND ? -1 : 1;
    uint32_t diff = sign == -1 ? 
        timer1_at_flank_up : last_timer_max - timer1_at_flank_up;

    if (diff > UNSYNCED_THRS && unsynced_counter < 255) {
        unsynced_counter++;
    }

    if (diff > ADAPTION_THRS && (diff <= UNSYNCED_THRS || unsynced_counter >= MAX_UNSYNCED_COUNTER)) {
        if (sign == -1) {
            // timer fired too late -> make next timer period shorter
            last_timer_max = TIM1_SECOND - diff;
        } else {
            // timer fired too early -> make next timer period longer
            last_timer_max = TIM1_SECOND + diff;
        }
        unsynced_counter = 0;
    } else {
        last_timer_max = TIM1_SECOND;
    }
    timer1_write(last_timer_max);

    if (clock_seconds != -1) {
        clock_seconds = (clock_seconds + 1) % 86400;
    }
    
    if (next_second != -1) {
        #ifdef DEBUG
        if (clock_seconds != -1 && clock_seconds != next_second) {
            Serial.println("### clock_seconds != next_second");
        }
        #endif
        clock_seconds = next_second;
        next_second = -1;
    }
    
    sec_pending = true;

    #ifdef DEBUG_FINEST
    Serial.printf("@up = %d, D = %d*%d, [[%d]]\n", 
        timer1_at_flank_up,
        sign, diff,
        unsynced_counter
    );
    #endif
}
