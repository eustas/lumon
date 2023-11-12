#include "render.h"

// Board
#include "hardware/gpio.h"

// Project
#include "bt.h"
#include "io.h"

// Data persisted between "frames".
typedef struct Cookie {
#define PACE (NUM_LED / 6)
  int pos;
  uint32_t brightness;
  uint32_t slope[PACE];
  int parser_state;
} Cookie;

static Cookie cookie;

void set_brightness(int brightness) {
  cookie.brightness = brightness;
  int rounding = PACE / 2;
  for (int i = 0; i < PACE; i++) {
    cookie.slope[i] = (brightness * i + rounding) / PACE;
  }
}

// Run-once for business-logic.
void init_render(void) {
  cookie.pos = 0;
  cookie.parser_state = 0;
  set_brightness(LOW_BRIGHTNESS);
}

void parse_commands(void) {
  while (bt_data_start < bt_data_end) {
    uint8_t b = bt_data_buf[bt_data_start++ & (BT_DATA_WRAP - 1)];
    switch (cookie.parser_state) {
      case 0: {
        if (b == 'b') {
          cookie.parser_state = 'b';
        } else {
          cookie.parser_state = -1;
        }
        break;
      }
      case 'b': {
        set_brightness(b);
        cookie.parser_state = 0;
        break;
      }
      default: {
        // Parser is in broken state. Just skip input.
        // TODO(eustas): add reset / recovery message.
        break;
      }
    }
  }
}

// Put your amazing code here.
void render(uint32_t* led) {
  parse_commands();
  uint32_t* slope = cookie.slope;
  uint32_t t = cookie.brightness;

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
