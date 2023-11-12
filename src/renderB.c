#include "render.h"

// Board
#include "hardware/gpio.h"

// Project
#include "io.h"

// Data persisted between "frames".
typedef struct Cookie {
#define PACE (NUM_LED / 2)
  int pos[3];
  int velocity[3];
  int flop;
  uint32_t slopeLo[PACE];
  uint32_t slopeHi[PACE];
} Cookie;

static Cookie cookie;

// Run-once for business-logic.
void init_render(void) {
  cookie.pos[0] = 0;
  cookie.velocity[0] = 1;
  cookie.pos[1] = 0;
  cookie.velocity[1] = 2;
  cookie.pos[2] = 0;
  cookie.velocity[2] = -3;
  cookie.flop = 0;
  int half = PACE / 2;
  int rounding = half / 2;
  for (int i = 0; i < PACE; i++) {
    int k = i < half ? 0 : (i - half);
    cookie.slopeLo[i] = (LOW_BRIGHTNESS * k + rounding) / half;
    cookie.slopeHi[i] = (HIGH_BRIGHTNESS * k + rounding) / half;
  }
}

// Put your amazing code here.
void render(uint32_t* led) {
  int usr1 = gpio_get(USR1_PIN);
  uint32_t* slope = usr1 ? cookie.slopeLo : cookie.slopeHi;
  uint32_t t = usr1 ? LOW_BRIGHTNESS : HIGH_BRIGHTNESS;

  int pos[3];
  int dir[3];
  int lim[3];
  for (int i = 0; i < 3; ++i) {
    if (cookie.flop) {
      pos[i] = (cookie.pos[i] + NUM_LED + cookie.velocity[i]) % NUM_LED;
    } else {
      pos[i] = cookie.pos[i];
    }
    cookie.pos[i] = pos[i];
    dir[i] = 1;
    lim[i] = PACE;
    if (pos[i] >= lim[i]) {
      dir[i] = -dir[i];
      pos[i] = 2 * PACE - 1 - pos[i];
      lim[i] = PACE - 1 - lim[i];
    }
  }
  cookie.flop = ~cookie.flop;
  for (int i = 0; i < NUM_LED; i++) {
    int rgb[3];
    for (int j = 0; j < 3; ++j) {
      rgb[j] = slope[pos[j]];
      pos[j] = pos[j] + dir[j];
      if (pos[j] == lim[j]) {
        dir[j] = -dir[j];
        pos[j] += dir[j];
        lim[j] = PACE - 1 - lim[j];
      }
    }
    led[i] = (rgb[0] << 24) | (rgb[1] << 16) | (rgb[2] << 8);
  }
}
