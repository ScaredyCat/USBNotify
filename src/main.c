#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/flash.h"
#include "hardware/pio.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "ws2812.pio.h"
#include "tusb.h"

#define WS2812_PIN 16

// Settings sector at 0xC8000 (~800KB in), safely above firmware (~21KB)
//#define SETTINGS_FLASH_OFFSET 0xC8000
#define SETTINGS_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define SETTINGS_MAGIC        0xDEADBEEF

#define SOLID_EFFECT 0
#define PULSE_EFFECT 1
#define BLINK_EFFECT 2
#define FASTBLINK_EFFECT 3
#define RAINBOW_EFFECT 4


typedef struct {
    uint32_t magic;
    uint8_t  r, g, b;
    uint8_t  mode;
    uint8_t  brightness;
    uint8_t  _pad[3];
} led_settings_t;

static const led_settings_t *flash_settings =
    (const led_settings_t *)(XIP_BASE + SETTINGS_FLASH_OFFSET);

// Passed to flash_safe_execute callback
static led_settings_t g_save_buf;

// This callback runs with interrupts disabled, XIP cache flushed, core 1 stalled —
// everything flash_safe_execute guarantees. Safe to call flash_range_* from here.
static void do_flash_write(void *param) {
    (void)param;
    flash_range_erase(SETTINGS_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(SETTINGS_FLASH_OFFSET, (const uint8_t *)&g_save_buf, FLASH_PAGE_SIZE);
}

static void save_settings(uint8_t r, uint8_t g, uint8_t b,
                          uint8_t mode, uint8_t brightness) {
    g_save_buf.magic      = SETTINGS_MAGIC;
    g_save_buf.r          = r;
    g_save_buf.g          = g;
    g_save_buf.b          = b;
    g_save_buf.mode       = mode;
    g_save_buf.brightness = brightness;
    g_save_buf._pad[0]    = 0;
    g_save_buf._pad[1]    = 0;
    g_save_buf._pad[2]    = 0;

    // flash_safe_execute handles disabling interrupts, flushing XIP cache,
    // and stalling core 1 if needed — the correct way to write flash on RP2040
    flash_safe_execute(do_flash_write, NULL, UINT32_MAX);
}

static bool load_settings(volatile uint8_t *r, volatile uint8_t *g, volatile uint8_t *b,
                           volatile uint8_t *mode, volatile uint8_t *brightness) {
    if (flash_settings->magic != SETTINGS_MAGIC) return false;
    *r          = flash_settings->r;
    *g          = flash_settings->g;
    *b          = flash_settings->b;
    *mode       = flash_settings->mode;
    *brightness = flash_settings->brightness;
    return true;
}

volatile uint8_t r_val=0, g_val=0, b_val=0, mode=0, user_bright=255;
volatile bool pending_save = false;

static inline void put_pixel_raw(uint8_t r, uint8_t g, uint8_t b) {
    while (pio_sm_is_tx_fifo_full(pio0, 0)) tight_loop_contents();
    uint32_t rgb = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    pio_sm_put_blocking(pio0, 0, rgb << 8u);
}

static inline void put_pixel_scaled(uint8_t r, uint8_t g, uint8_t b, float intensity) {
    float master = (float)user_bright / 255.0f;
    float final  = intensity * master;
    while (pio_sm_is_tx_fifo_full(pio0, 0)) tight_loop_contents();
    uint32_t rgb = ((uint32_t)(r * final) << 16) |
                   ((uint32_t)(g * final) << 8)  |
                    (uint32_t)(b * final);
    pio_sm_put_blocking(pio0, 0, rgb << 8u);
}

static void blink_raw(uint8_t r, uint8_t g, uint8_t b, int times) {
    for (int i = 0; i < times; i++) {
        put_pixel_raw(r, g, b);
        sleep_ms(250);
        put_pixel_raw(0, 0, 0);
        sleep_ms(250);
    }
    sleep_ms(400);
}

static void usb_spin_ms(uint32_t ms) {
    uint32_t t = time_us_32();
    while ((int32_t)(time_us_32() - t) < (int32_t)(ms * 1000)) {
        tud_task();
    }
}

int main() {
    tusb_init();
    uint offset = pio_add_program(pio0, &ws2812_program);
    ws2812_program_init(pio0, 0, offset, WS2812_PIN, 800000, false);

    blink_raw(0, 0, 40, 1);

    bool had_saved = load_settings(&r_val, &g_val, &b_val, &mode, &user_bright);
    if (had_saved) {
        blink_raw(0, 40, 0, 2);
    } else {
        blink_raw(40, 0, 0, 1);
    }

    uint32_t step = 0;
    uint32_t next_update_us = time_us_32();

    while (1) {
        tud_task();

        if (pending_save) {
            pending_save = false;
            uint8_t sr = r_val, sg = g_val, sb = b_val, sm = mode, sbr = user_bright;
            save_settings(sr, sg, sb, sm, sbr);

            for (int i = 0; i < 3; i++) {
                put_pixel_raw(40, 40, 40);
                usb_spin_ms(150);
                put_pixel_raw(0, 0, 0);
                usb_spin_ms(150);
            }

            next_update_us = time_us_32();
            continue;
        }

        uint32_t now = time_us_32();
        if ((int32_t)(now - next_update_us) < 0) continue;

        if (mode == PULSE_EFFECT) {
            (step++ % 40 < 20) ? put_pixel_scaled(r_val, g_val, b_val, 1.0f) : put_pixel_scaled(0,0,0,0);
            next_update_us += 20000;
        }
        else if (mode == BLINK_EFFECT) {
            float intensity = (sinf(step++ * 0.05f) + 1.0f) / 2.0f;
            put_pixel_scaled(r_val, g_val, b_val, intensity);
            next_update_us += 30000;
        }
        else if (mode == FASTBLINK_EFFECT) {
            (step++ % 20 < 10) ? put_pixel_scaled(r_val, g_val, b_val, 1.0f) : put_pixel_scaled(0,0,0,0);
            next_update_us += 10000;
        }
        else if (mode == RAINBOW_EFFECT) {
            float angle = step++ * 0.05f;
            uint8_t r = (uint8_t)((sinf(angle)           + 1.0f) * 127.5f);
            uint8_t g = (uint8_t)((sinf(angle + 2.0944f) + 1.0f) * 127.5f);
            uint8_t b = (uint8_t)((sinf(angle + 4.1888f) + 1.0f) * 127.5f);
            put_pixel_scaled(r, g, b, 1.0f);
            next_update_us += 20000;
        }
        else {
            // SOLID_EFFECT
            put_pixel_scaled(r_val, g_val, b_val, 1.0f);
            next_update_us += 10000;
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

        if (bufsize >= 6 && buffer[5] == 0x01) {
            pending_save = true;
        }
    }
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) { return 0; }
