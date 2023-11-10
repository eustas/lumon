#ifndef LUMON_SETUP_H
#define LUMON_SETUP_H

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

#endif  // LUMON_SETUP_H
