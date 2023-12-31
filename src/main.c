// System
#include <stdint.h>

// Board
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "pico/stdio.h"

// Project
#include "bt.h"
#include "io.h"
#include "render.h"
#include "ws2812.pio.h"  // Generated from ws2812.pio

// Initial dual-core handshake.
uint32_t kCore0Ready = 0xFEEDBAC0;
uint32_t kCore1Ready = 0xFEEDBAC1;
// We report that passed pixel data is not used anymore.
uint32_t kCore1Done  = 0xFEEDBAC2;

// Buffers for RGB data;
static uint32_t bankA[NUM_LED];
static uint32_t bankB[NUM_LED];

// Core 0 is used for system IO (WiFi, USB-UART, etc.). We also do calculations
// here, since those are tolerant to interrupts.
void core0_main(void) {
  init_render();
  int bank = 0;
  while (1) {
    uint32_t* led = bank ? bankA : bankB;
    bank ^= 1;
    render(led);
    // Wait with renderer.
    (void)multicore_fifo_pop_blocking();  // we expect kCore1Done here.
    multicore_fifo_push_blocking((uint32_t)led);
  }
}

// Core 1 is used to push data to LED strip. Normally, no interrupts should
// happen.
// TODO(eustas): we have a load of ~800 CLK busy-waiting periods plus a 280us
//               sleep; could we do something useful?
void core1_main(void) {
  // Run all PIO modules.
  pio_set_sm_mask_enabled(pio0,
      /* mask */ (1 << NUM_OUT_PINS) - 1, /* enabled */ true);

  // Notify, that we are ready to render.
  multicore_fifo_push_blocking(kCore1Done);

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
    multicore_fifo_push_blocking(kCore1Done);
    sleep_us(280);
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
  stdio_init_all();

  init_flash();
  flash();
  init_usr();
  flash();
  init_pio();
  flash();
  prepare_pio();
  flash();

  init_bt();

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
