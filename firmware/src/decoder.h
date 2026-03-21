#ifndef DECODER_H_
#define DECODER_H_

#include <cstddef>
#include "time.h"

#define ACCEPTED_SIGNAL_DURATION_DEVIATION_MS 45
#define ACCEPTED_MINUTE_SIGNAL_DURATION_DEVIATION_MS 200
#define MAX_SUSPICIOUS_TIME_FRAMES 9
#define SUSPICIOUS_SECOND_DIFF_THRS 5

class Decoder {
    public:
        void next(uint32_t rx_time, uint32_t signal_duration);

    private:
        int cursor = 0;
        bool invalid_bit = false;
        uint8_t buffer[8];
        unsigned long last_bit_rx_time;
        //
        int suspicious_time_counter = 0;
        Time* last_accepted_time = NULL;

        Time* decode_buffer();
        static bool parity_check(const uint8_t* buffer, const int begin, const int end);
};


#endif //DECODER_H_