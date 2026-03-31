#include "sync.h"
#include "decoder.h" // for MINIMAL_ONE_TIME

std::optional<std::pair<uint32_t, uint32_t>> Sync::signal_pending() {
    if (up_millis_pending && down_millis_pending) {
        up_millis_pending = false;
        down_millis_pending = false;
        if (down_millis - up_millis >= MINIMAL_ZERO_TIME) {
            return std::pair(up_millis, down_millis - up_millis);
        } else {
            return {};
        }
    } else {
        return {};
    }
}

void IRAM_ATTR Sync::inputLevelChangedISR() {
    int v = digitalRead(DCF_IN);
    if (v) {
        up_millis = millis();
        down_millis_pending = false;
        up_millis_pending = true;
        if (timer1_enabled()) {
            timer1_at_flank_up = timer1_read();
            flank_up_pending = true;
        }
    } else {
        down_millis = millis();
        down_millis_pending = true;
    }
}

void IRAM_ATTR Sync::timerFiredISR() {
    int8_t sign = timer1_at_flank_up < TIM1_HALFSECOND ? -1 : 1;
    uint32_t diff = sign == -1 ? timer1_at_flank_up : last_timer_max - timer1_at_flank_up;

    #ifdef DEBUG_SYNC
    Serial.printf("[sync] tim1 = %d, diff = %s%d\n", timer1_at_flank_up, sign > 0 ? "+" : "-", diff);
    #endif

    if (diff > UNSYNCED_THRS && unsynced_counter < 255) {
        unsynced_counter++;

        #ifdef DEBUG_SYNC
        Serial.printf("[sync] not in sync -> unsynced_counter = %d\n", unsynced_counter);
        #endif
    }

    uint32_t next_timer_max = TIM1_SECOND;

    if (flank_up_pending && diff > ADAPTION_THRS && (diff <= UNSYNCED_THRS || unsynced_counter >= MAX_UNSYNCED_COUNTER)) {
        
        #ifdef DEBUG_SYNC    
        Serial.print("[sync] adapting");
        if (unsynced_counter > 0) {
            Serial.print(", unsynced_counter zeroed.");
        }
        Serial.println("");
        #endif

        if (sign == -1) {
            // timer fired too late -> make next timer period shorter
            next_timer_max = TIM1_SECOND - (diff/2);
        } else {
            // timer fired too early -> make next timer period longer
            next_timer_max = TIM1_SECOND + (diff/2);
        }

        unsynced_counter = 0;
        timer1_write(next_timer_max);
    } else if (last_timer_max != TIM1_SECOND) {
        timer1_write(TIM1_SECOND);
    }

    flank_up_pending = false;
    last_timer_max = next_timer_max;

    if (clock_seconds != -1) {
        clock_seconds = (clock_seconds + 1) % OVERFLOW_SECS;
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
    
    #ifdef DEBUG_FINE 
    sec_pending = true; 
    #endif
}
