#include "lumon.h"

#include <stdint.h>
#include <string.h>

#include "hardware/gpio.h"
#include "pico/multicore.h"
#include "ws2812.pio.h"

uint32_t kCore0Ready = 0xFEEDBAC0;
uint32_t kCore1Ready = 0xFEEDBAC1;
uint32_t kStartPlay = 0xC0DEABBA;

#define FLASH_PIN 28
#define OUT_PIN0 16
#define NUM_OUT_PINS 1

// Native CPU frequency (CLK) is 125MHz
// 1 bit on wire is 125us. It consists of 25 ticks, 5us each.
// 5us (TICK) corresponds to 200KHz.
// TICKS_PER_CLK is a CPU to PIO frequency divider
#define TICKS_PER_CLK 625

void init_flash(void) {
  gpio_init(FLASH_PIN);
  gpio_set_dir(FLASH_PIN, GPIO_OUT);
}

void flash(void) {
  gpio_put(FLASH_PIN, 1);
  sleep_ms(100);
  gpio_put(FLASH_PIN, 0);
  sleep_ms(100);
}

void core0_main(void) {
  pio_set_sm_mask_enabled(pio0,
      /* mask */ (1 << NUM_OUT_PINS) - 1, /* enabled */ true);
  while (1) {
    flash();
    pio0->txf[0] = 0xFF0000;
    pio0->txf[0] = 0x00FF00;
    pio0->txf[0] = 0x0000FF;
  }
}

void core1_main(void) {
  while (1) {
  }
}

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

void init_pio(void) {
  pio_clear_instruction_memory(pio0);
  pio_clear_instruction_memory(pio1);
  pio_add_program_at_offset(pio0, &ws2812_program, 0);
}

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
  sm_config_set_out_shift(&c, /* shift_right */ true, /* autopull */ false,
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

int main(void) {
  init_flash();
  flash();
  init_pio();
  flash();
  prepare_pio();
  flash();

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
