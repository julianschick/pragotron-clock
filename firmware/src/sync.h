#ifndef SYNC_H_
#define SYNC_H_

#include <Arduino.h>
#include "defines.h"

#define TIM1_SECOND     5000000
#define TIM1_HALFSECOND 2500000
#define TIM1_DIVIDER TIM_DIV16

#define ADAPTION_THRS 100
#define UNSYNCED_THRS 100000
#define MAX_UNSYNCED_COUNTER 15

class Sync {

    private:
        inline static uint32_t up_millis = 0;
        inline static volatile boolean up_millis_pending = false;
        inline static uint32_t down_millis = 0;
        inline static volatile boolean down_millis_pending = false;
        inline static uint8_t unsynced_counter = 255;
        inline static uint32_t timer1_at_flank_up = 0;

        inline static volatile uint32_t timer_read = 0;
        inline static uint32_t last_timer_max = TIM1_SECOND;
        inline static volatile bool sec_pending = false;

        inline static int next_second = -1;
        inline static int clock_seconds = -1;

    public:
        static void IRAM_ATTR inputLevelChangedISR();
        static void IRAM_ATTR timerFiredISR();

        static void set_next_second(const int val) {
            next_second = val;
        }
        
        static int get_clock_seconds() {
            return clock_seconds;
        }

        #ifdef DEBUG_FINEST
        static int get_timer1_at_flank_up() {
            return timer1_at_flank_up;
        }
        #endif

        static std::optional<std::pair<uint32_t, uint32_t>> signal_pending() {
            if (up_millis_pending && down_millis_pending) {
                up_millis_pending = false;
                down_millis_pending = false;
                return std::pair(up_millis, down_millis - up_millis);
            } else {
                return {};
            }
        }

        static bool is_second_pending() {
            bool result = sec_pending;
            sec_pending = false;
            return result;
        }
};

#endif //SYNC_H_