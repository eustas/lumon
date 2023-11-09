#include "lumon.h"

#include <stdint.h>
#include <string.h>

#include "hardware/gpio.h"
#include "pico/multicore.h"
#include "ws2812.pio.h"

// Initial dual-core handshake.
uint32_t kCore0Ready = 0xFEEDBAC0;
uint32_t kCore1Ready = 0xFEEDBAC1;
// We report that passed pixel data is not used anymore.
uint32_t kCore0Done  = 0xFEEDBAC2;

// Pin configuration
#define FLASH_PIN 15
#define OUT_PIN0 16
// It is possible to drive up to 8 LED strips.
#define NUM_OUT_PINS 1
#define USR0_PIN 14
#define USR1_PIN 0

// We do not throttle PIO; every bit is transfered in 101 clk that maps to 808ns
// at default CPU frequency (125MHz).
#define TICKS_PER_CLK 1

// Number of LEDs in a line
#define NUM_LED 300

// Define max brightness levels.
#define LOW_BRIGHTNESS 63
#define HIGH_BRIGHTNESS 255

// We use a separate LED for debugging.
void init_flash(void) {
  gpio_init(FLASH_PIN);
  gpio_set_dir(FLASH_PIN, GPIO_OUT);
}

// "Blocking" debug flash. Takes 200ms.
void flash(void) {
  gpio_put(FLASH_PIN, 1);
  sleep_ms(100);
  gpio_put(FLASH_PIN, 0);
  sleep_ms(100);
}

// Those pins are used as inputs. By default those are "pulled-up" and read 1
// when not connected; connect pin to GND to make it read 0.
void init_usr(void) {
  gpio_init(USR0_PIN);
  gpio_pull_up(USR0_PIN);
  gpio_set_dir(USR0_PIN, GPIO_IN);

  gpio_init(USR1_PIN);
  gpio_pull_up(USR1_PIN);
  gpio_set_dir(USR1_PIN, GPIO_IN);
}

// Core 0 is used for system IO. It pushes updates to LED strip, and does other
// IO (WiFi, USB-UART, etc.)
void core0_main(void) {
  // Run all PIO modules.
  pio_set_sm_mask_enabled(pio0,
      /* mask */ (1 << NUM_OUT_PINS) - 1, /* enabled */ true);

  // Notify, that we are ready to render.
  multicore_fifo_push_blocking(kCore0Done);

  while (1) {
    // The other core gives us the new data to render.
    // Likely non-blocking, i.e. data is ready at this moment.
    uint32_t* led = (uint32_t*)multicore_fifo_pop_blocking();
    for (int t = 0; t < NUM_LED;) {
      while (pio_sm_get_tx_fifo_level(pio0, /* sm */ 0) >= 4) {}
      pio0->txf[0] = led[t++];
      pio0->txf[0] = led[t++];
      pio0->txf[0] = led[t++];
      pio0->txf[0] = led[t++];
    }
    // Actually non-blocking - our synchronization is pull-driven.
    multicore_fifo_push_blocking(kCore0Done);
    sleep_us(280);
  }
}

// Buffers for RGB data;
static uint32_t bankA[NUM_LED];
static uint32_t bankB[NUM_LED];

// Data persisted between "frames".
typedef struct Cookie {
#define PACE (NUM_LED / 6)
  int pos;
  uint32_t slopeLo[PACE];
  uint32_t slopeHi[PACE];
} Cookie;

static Cookie cookie;

// Run-once for business-logic.
void init_cookie(void) {
  cookie.pos = 0;
  for (int i = 0; i < PACE; i++) {
    cookie.slopeLo[i] = (LOW_BRIGHTNESS * i) / PACE;
    cookie.slopeHi[i] = (HIGH_BRIGHTNESS * i) / PACE;
  }
}

// Put your amazing code here.
void render(uint32_t* led) {
  int usr1 = gpio_get(USR1_PIN);
  uint32_t* slope = usr1 ? cookie.slopeLo : cookie.slopeHi;
  uint32_t t = usr1 ? LOW_BRIGHTNESS : HIGH_BRIGHTNESS;

  int dir = gpio_get(USR0_PIN) ? 1 : -1;
  int pos = (cookie.pos + NUM_LED + dir) % NUM_LED;
  cookie.pos = pos;

  int part = pos / PACE;
  int phase = pos % PACE;
  for (int i = 0; i < NUM_LED; i++) {
    int u = slope[phase];
    int d = t - u;
    int r, g, b;
    switch (part) {
      case 0: r = t; g = u; b = 0; break;
      case 1: r = d; g = t; b = 0; break;
      case 2: r = 0; g = t; b = u; break;
      case 3: r = 0; g = d; b = t; break;
      case 4: r = u; g = 0; b = t; break;
      default: r = t; g = 0; b = d; break;
    }
    led[i] = (r << 24) | (g << 16) | (b << 8);

    phase++;
    if (phase == PACE) {
      phase = 0;
      part++;
      if (part == 6) {
        part = 0;
      }
    }
  }
}

// Core 1 is used for calculations.
void core1_main(void) {
  init_cookie();
  int bank = 0;
  while (1) {
    uint32_t* led = bank ? bankA : bankB;
    bank ^= 1;
    render(led);
    // Wait with renderer.
    (void)multicore_fifo_pop_blocking();  // we expect kCore0Done here.
    multicore_fifo_push_blocking((uint32_t)led);
  }
}

// Handshake with core 0 and start main loop.
void core1_start(void) {
  uint32_t test = multicore_fifo_pop_blocking();
  if (test == kCore0Ready) {
    multicore_fifo_push_blocking(kCore1Ready);
    core1_main();
  }
  while (1) {
    tight_loop_contents();
  }
}

// Reset PIO and load program.
void init_pio(void) {
  pio_clear_instruction_memory(pio0);
  pio_clear_instruction_memory(pio1);
  pio_add_program_at_offset(pio0, &ws2812_program, 0);
}

// Configure PIO modules.
void prepare_pio(void) {
  pio_sm_config c = pio_get_default_sm_config();
  // sm_config_set_in_pins(&c, in_base);
  // sm_config_set_sideset(&c, bit_count, optional, pindirs false);
  sm_config_set_clkdiv_int_frac(&c,
       /* div_int */ TICKS_PER_CLK, /* div_frac */ 0);
  sm_config_set_wrap(&c, ws2812_wrap_target, ws2812_wrap);
  // sm_config_set_jmp_pin(&c, pin);
  sm_config_set_in_shift(&c, /* shift_right */ true, /* autopush */ false,
                          /* push threshold */ 32);
  sm_config_set_out_shift(&c, /* shift_right */ false, /* autopull */ false,
                          /* pull_threshold */ 24);
  sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
  // sm_config_set_out_special(&c, sticky, has_enable_pin, enable_pin_index);
  // sm_config_set_mov_status(&c, status_sel, status_n);

  for (uint32_t sm = 0; sm < NUM_OUT_PINS; ++sm) {
    uint32_t out_pins = OUT_PIN0 + sm;
    sm_config_set_out_pins(&c, out_pins, /* out_count */ 1);
    // Clear pins.
    pio_sm_set_pins_with_mask(pio0, sm,
        /* values */0, /* mask */ 1 << out_pins);
    // Set direction
    pio_sm_set_consecutive_pindirs(pio0, sm,
        out_pins, /* pin_count */ 1, /* is_out */ true);

    pio_gpio_init(pio0, out_pins);
    // gpio_set_drive_strength(out_pins, GPIO_DRIVE_STRENGTH_12MA);

    pio_sm_init(pio0, sm, ws2812_offset_entry_point, &c);
  }
}

// Entry point.
int main(void) {
  init_flash();
  flash();
  init_usr();
  flash();
  init_pio();
  flash();
  prepare_pio();
  flash();

  // Initialize handshake. We are running on core 0.
  multicore_launch_core1(core1_start);
  multicore_fifo_push_blocking(kCore0Ready);
  uint32_t test = multicore_fifo_pop_blocking();
  if (test == kCore1Ready) {
    core0_main();
  }
  while (1) {
    tight_loop_contents();
  }
}
