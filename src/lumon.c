#include "lumon.h"

#include <stdint.h>
#include <string.h>

#include "hardware/gpio.h"
#include "pico/multicore.h"

uint32_t kCore0Ready = 0xFEEDBAC0;
uint32_t kCore1Ready = 0xFEEDBAC1;
uint32_t kStartPlay = 0xC0DEABBA;

void init_flash(void) {
  gpio_init(25);
  gpio_set_dir(25, GPIO_OUT);
}

void flash(void) {
  gpio_put(25, 1);
  sleep_ms(100);
  gpio_put(25, 0);
  sleep_ms(100);
}

void core0_main(void) {
  while (1) {
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

int main(void) {
  init_flash();
  flash();
  flash();
  flash();
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
