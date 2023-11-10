#include "io.h"

// Board
#include "pico/time.h"
#include "hardware/gpio.h"

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

void sos(void) {
  while (1) {
    for (int i = 0; i < 9; i++) {
      gpio_put(FLASH_PIN, 1);
      sleep_ms(200 + 200 * (i >= 3 && i < 6));
      gpio_put(FLASH_PIN, 0);
      sleep_ms(200);
    }
    sleep_ms(800);
  }
}

void init_usr(void) {
  gpio_init(USR0_PIN);
  gpio_pull_up(USR0_PIN);
  gpio_set_dir(USR0_PIN, GPIO_IN);

  gpio_init(USR1_PIN);
  gpio_pull_up(USR1_PIN);
  gpio_set_dir(USR1_PIN, GPIO_IN);
}
