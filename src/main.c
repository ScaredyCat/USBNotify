#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "tusb.h"

#define WS2812_PIN 16
volatile uint8_t r_val=0, g_val=0, b_val=0, mode=0, user_bright=255;

static inline void put_pixel_scaled(uint8_t r, uint8_t g, uint8_t b, float intensity) {
    float master = (float)user_bright / 255.0f;
    float final = intensity * master;
    
    // Waveshare GRB Order
    uint32_t rgb = ((uint32_t)(r * final) << 16) | 
                   ((uint32_t)(g * final) << 8)  | 
                    (uint32_t)(b * final);
    pio_sm_put_blocking(pio0, 0, rgb << 8u);
}

int main() {
    tusb_init();
    uint offset = pio_add_program(pio0, &ws2812_program);
    ws2812_program_init(pio0, 0, offset, WS2812_PIN, 800000, false);

    uint32_t step = 0;
    while (1) {
        tud_task();

        if (mode == 1) { // FLASH
            (step++ % 40 < 20) ? put_pixel_scaled(r_val, g_val, b_val, 1.0f) : put_pixel_scaled(0,0,0,0);
            sleep_ms(20);
        } 
        else if (mode == 2) { // PULSE
            float intensity = (sinf(step++ * 0.05f) + 1.0f) / 2.0f;
            put_pixel_scaled(r_val, g_val, b_val, intensity);
            sleep_ms(30);
        } 
	else if (mode == 3) { // Quick flash
	    (step++ % 20 < 10) ? put_pixel_scaled(r_val, g_val, b_val, 1.0f) : put_pixel_scaled(0,0,0,0);
            sleep_ms(10);
			      
	} 
	else if (mode == 4) {
		// Use step to create three phases 120 degrees apart
    		float angle = step++ * 0.05f; 
    
    		// Calculate RGB values based on sine waves
    		uint8_t r = (uint8_t)((sinf(angle) + 1.0f) * 127.5f);
    		uint8_t g = (uint8_t)((sinf(angle + 2.0944f) + 1.0f) * 127.5f); // +2pi/3
    		uint8_t b = (uint8_t)((sinf(angle + 4.1888f) + 1.0f) * 127.5f); // +4pi/3

    		put_pixel_scaled(r, g, b, 1.0f);
    		sleep_ms(20); // Adjust for speed

	}
        else { // SOLID
            put_pixel_scaled(r_val, g_val, b_val, 1.0f);
            sleep_ms(10);
        }
    }
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    if (report_type == HID_REPORT_TYPE_OUTPUT && bufsize >= 5) {
        r_val       = buffer[0];
        g_val       = buffer[1];
        b_val       = buffer[2];
        mode        = buffer[3];
        user_bright = buffer[4];
    }
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) { return 0; }

