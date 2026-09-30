#ifndef LED_H_
#define LED_H_

#define BLINK_MS_QUICK 300
#define BLINK_MS_SLOW 1200

namespace Led {
    bool wlan_connected = false;
    bool ap_open = false;
    bool clock_sprinting = false;
    bool coil_active = false;
    bool time_known = false;

    // 0 = off
    // 1 = on
    // 2 = slow blink
    // 3 = quick blink
    int mode = 0;

    boolean state = false;
    uint32_t last_state_transition = 0;

    void process() {
        if (ap_open) {
            mode = 3;
        } else if (clock_sprinting) {
            mode = 2;
        } else {
            mode = coil_active ? 1 : 0;             
        }

        uint32_t now = millis();

        if (mode == 0) {
            state = false;
            last_state_transition = now;
        } else if (mode == 1) {
            state = true;
            last_state_transition = now;
        } else if (mode == 2 || mode == 3) {
            if (now - last_state_transition > (mode == 2 ? BLINK_MS_SLOW : BLINK_MS_QUICK)) {
                state = !state;
                last_state_transition = now;
            }
        }

        digitalWrite(LED_BUILTIN, state ? LOW : HIGH);
                
    }

}


#endif // LED_H_