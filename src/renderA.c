#include "render.h"

// Board
#include "hardware/gpio.h"

// Project
#include "io.h"

// Data persisted between "frames".
typedef struct Cookie {
#define PACE (NUM_LED / 6)
  int pos;
  uint32_t slopeLo[PACE];
  uint32_t slopeHi[PACE];
} Cookie;

static Cookie cookie;

// Run-once for business-logic.
void init_render(void) {
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
