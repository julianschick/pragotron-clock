#ifndef SYNC_H_
#define SYNC_H_

#include <Arduino.h>
#include "defines.h"

#define TIM1_SECOND     5000000
#define TIM1_HALFSECOND 2500000
#define TIM1_DIVIDER    TIM_DIV16

#define ADAPTION_THRS 100000
#define UNSYNCED_THRS 900000
#define MAX_UNSYNCED_COUNTER 15

class Sync {

    private:
        inline static Sync* instance = NULL;

        uint32_t up_millis = 0;
        volatile boolean up_millis_pending = false;
        uint32_t down_millis = 0;
        volatile boolean down_millis_pending = false;
        uint8_t unsynced_counter = 255;

        uint32_t timer1_at_flank_up = 0;
        boolean flank_up_pending = false;

        volatile uint32_t timer_read = 0;
        uint32_t last_timer_max = TIM1_SECOND;
        #ifdef DEBUG_FINE
        volatile bool sec_pending = false;
        #endif

        int next_second = -1;
        int clock_seconds = -1;

        void IRAM_ATTR inputLevelChangedISR();
        void IRAM_ATTR timerFiredISR();

    public:
        Sync() {
            if (instance == NULL) {
                instance = this;
            }
        }

        static void IRAM_ATTR inputLevelChangedISR_() {
            if (instance != NULL) {
                instance->inputLevelChangedISR();
            }

        }
        static void IRAM_ATTR timerFiredISR_() {
            if (instance != NULL) {
                instance->timerFiredISR();
            }
        }

        void set_next_second(const int val) {
            next_second = val;
        }
        
        int get_clock_seconds() {
            return clock_seconds;
        }

        #ifdef DEBUG_FINEST
        static int get_timer1_at_flank_up() {
            return timer1_at_flank_up;
        }
        #endif

        std::optional<std::pair<uint32_t, uint32_t>> signal_pending();

        #ifdef DEBUG_FINE
        bool is_second_pending() {
            bool result = sec_pending;
            sec_pending = false;
            return result;
        }
        #endif
};

#endif //SYNC_H_