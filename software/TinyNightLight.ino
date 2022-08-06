// ===================================================================================
// Project:   TinyNightLight - Night Light with NeoPixels based on ATtiny13A
// Version:   v1.0
// Year:      2021
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// Rotary encoder controlled and battery powered nightlight with NeoPixels.
//
// References:
// -----------
// The Neopixel implementation is based on NeoController.
// https://github.com/wagiminator/ATtiny13-NeoController
//
// Wiring:
// -------
//                              +-\/-+
//           --- RST ADC0 PB5  1|°   |8  Vcc
// ENCODER A ------- ADC3 PB3  2|    |7  PB2 ADC1 -------- ENCODER SW
// ENCODER B ------- ADC2 PB4  3|    |6  PB1 AIN1 OC0B --- MOSFET
//                        GND  4|    |5  PB0 AIN0 OC0A --- NEOPIXELS
//                              +----+
//
// Compilation Settings:
// ---------------------
// Controller:  ATtiny13A
// Core:        MicroCore (https://github.com/MCUdude/MicroCore)
// Clockspeed:  9.6 MHz internal
// BOD:         BOD disabled
// Timing:      Micros disabled
//
// Leave the rest on default settings. Don't forget to "Burn bootloader"!
// No Arduino core functions or libraries are used. Use the makefile if 
// you want to compile without Arduino IDE.
// Remove the battery before connecting the device to the programmer.
//
// Fuse settings: -U lfuse:w:0x3a:m -U hfuse:w:0xff:m
//
// Operating Instructions:
// -----------------------
// Place a 3.7V protected 16340 (LR123A) Li-Ion battery in the holder. The battery
// can be charged via the USB-C port. A red LED lights up during charging. It goes
// out when charging is complete.
// The device has four different states, which can be switched between by pressing
// the rotary encoder button.
// In state 0, all LEDs are off and the ATtiny is in sleep mode.
// In state 1, all LEDs light up white. The brightness can be changed by turning
// the rotary encoder. The brightness set by this is also retained for the
// following states.
// In state 2, the LEDs show a color animation with the previously set brightness.
// The speed of the animation can be adjusted using the rotary encoder.
// In state 3, all LEDs show the same color with the previously set brightness. The
// color can be changed by turning the rotary encoder.


// ===================================================================================
// Libraries and Definitions
// ===================================================================================

// Libraries
#include <avr/io.h>           // for GPIO
#include <avr/sleep.h>        // for sleep functions
#include <avr/interrupt.h>    // for interrupts
#include <util/delay.h>       // for delays

// Pin definitions
#define PIN_NEO       PB0     // pin connected to DIN of neopixels
#define PIN_ENABLE    PB1     // pin connected to MOSFET to switch on/off neopixels
#define PIN_ENC_SW    PB2     // pin connected to rotary encoder switch
#define PIN_ENC_A     PB3     // pin connected to rotary encoder A
#define PIN_ENC_B     PB4     // pin connected to rotary encoder B

// Pin manipulation macros
#define pinInput(x)   DDRB  &= ~(1<<(x))        // set pin to INPUT
#define pinOutput(x)  DDRB  |=  (1<<(x))        // set pin to OUTPUT
#define pinLow(x)     PORTB &= ~(1<<(x))        // set pin to LOW
#define pinHigh(x)    PORTB |=  (1<<(x))        // set pin to HIGH
#define pinPullup(x)  PORTB |=  (1<<(x))        // enable PULLUP resistor
#define pinIntEn(x)   PCMSK |=  (1<<(x))        // enable pin change interrupt
#define pinIntDis(x)  PCMSK &= ~(1<<(x))        // disable pin change interrupt
#define pinRead(x)    (PINB &   (1<<(x)))       // READ pin

// ===================================================================================
// Low-Level Neopixel Implementation for 9.6 MHz MCU Clock and 800 kHz Pixels
// ===================================================================================

// NeoPixel parameter and macros
#define NEO_init()    pinOutput(PIN_NEO)        // set pixel DATA pin as output
#define NEO_on()      pinOutput(PIN_ENABLE)     // switch on neopixels
#define NEO_off()     pinInput(PIN_ENABLE)      // switch off neopixels
#define NEO_latch()   _delay_us(281)            // delay until bytes are latched

// Send a byte to the pixels string
void NEO_sendByte(uint8_t byte) {               // CLK  comment
  for(uint8_t bit=8; bit; bit--) asm volatile(  //  3   8 bits, MSB first
    "sbi  %[port], %[pin]   \n\t"               //  2   DATA HIGH
    "sbrs %[byte], 7        \n\t"               // 1-2  if "1"-bit skip next instruction
    "cbi  %[port], %[pin]   \n\t"               //  2   "0"-bit: DATA LOW after 3 cycles
    "rjmp .+0               \n\t"               //  2   delay 2 cycles
    "add  %[byte], %[byte]  \n\t"               //  1   byte <<= 1
    "cbi  %[port], %[pin]   \n\t"               //  2   "1"-bit: DATA LOW after 7 cycles
    ::
    [port]  "I"   (_SFR_IO_ADDR(PORTB)),
    [pin]   "I"   (PIN_NEO),
    [byte]  "r"   (byte)
  );
}

// Write color to a single pixel
void NEO_writeColor(uint8_t r, uint8_t g, uint8_t b) {
  cli();
  NEO_sendByte(g); NEO_sendByte(r); NEO_sendByte(b);
  sei();
}

// Square function
uint8_t sqr(uint8_t x) {
  uint8_t result = 0;
  for(uint8_t i=x; i; i--) result += x;
  return result;
}

// Write hue value (0..47) and brightness (0..15) to a single pixel
void NEO_writeHue(uint8_t hue, uint8_t bright) {
  uint8_t phase = hue >> 4;
  hue          &= 15;
  bright        = (15 - bright) >> 1;
  uint8_t step  = (sqr(hue) >> bright) + 1;
  uint8_t nstep = (sqr(15 - hue) >> bright) + 1;
  switch(phase) {
    case 0:   NEO_writeColor(nstep,  step,     0); break;
    case 1:   NEO_writeColor(    0, nstep,  step); break;
    case 2:   NEO_writeColor( step,     0, nstep); break;
    default:  break;
  }
}

// Set a single pixel to white with brightness (0..15);
void NEO_writeWhite(uint8_t bright) {
  bright = sqr(bright);                         // simple gamma correction
  NEO_writeColor(bright, bright, bright);       // write to pixel
}

// ===================================================================================
// Rotary Encoder Implementation using Pin Change Interrupt
// ===================================================================================

// Global variables
volatile uint8_t  ENC_a0, ENC_b0, ENC_ab0, ENC_loop;
volatile int16_t  ENC_count, ENC_countMin, ENC_countMax, ENC_countStep;

// Init rotary encoder
void ENC_init(void) {
  pinPullup(PIN_ENC_A);                         // enable pullup on encoder pins ...
  pinPullup(PIN_ENC_B);
  pinPullup(PIN_ENC_SW);
  pinIntEn(PIN_ENC_A);                          // enable pin change interrupt on ENC A
  ENC_a0  = !pinRead(PIN_ENC_A);                // set initial values ...
  ENC_b0  = !pinRead(PIN_ENC_B);
  ENC_ab0 = (ENC_a0 == ENC_b0);
  GIMSK  |= (1<<PCIE);                          // enable pin change interrupts
}

// Set parameters for rotary encoder
void ENC_set(int16_t rmin, int16_t rmax, int16_t rstep, int16_t rvalue, uint8_t rloop) {
  ENC_countMin  = rmin << 1;                    // min value
  ENC_countMax  = rmax << 1;                    // max value
  ENC_countStep = rstep;                        // count steps (negative if CCW)
  ENC_count     = rvalue << 1;                  // actual count value
  ENC_loop      = rloop;                        // loop if true
}

// reads current rotary encoder value
int ENC_get(void) {
  return(ENC_count >> 1);
}

// Pin change interrupt service routine for rotary encoder
ISR(PCINT0_vect) {
  uint8_t a = !pinRead(PIN_ENC_A);
  uint8_t b = !pinRead(PIN_ENC_B);
  if(a != ENC_a0) {                             // A changed?
    ENC_a0 = a;
    if(b != ENC_b0) {                           // B changed?
      ENC_b0 = b;
      ENC_count += (a == b) ? -ENC_countStep : ENC_countStep;
      if((a == b) != ENC_ab0) ENC_count += (a == b) ? -ENC_countStep : ENC_countStep;
      if(ENC_count < ENC_countMin) ENC_count = (ENC_loop) ? ENC_countMax : ENC_countMin;
      if(ENC_count > ENC_countMax) ENC_count = (ENC_loop) ? ENC_countMin : ENC_countMax;
      ENC_ab0 = (a == b);
    }
  }
}

// ===================================================================================
// Main Function
// ===================================================================================

int main(void) {
  // Local variables
  uint8_t state   = 0;                          // aninmation state
  uint8_t hue     = 0;                          // hue
  uint8_t bright  = 5;                          // brightness
  uint8_t anim    = 0;                          // animation state
  uint8_t counter = 8;                          // animation delay counter
  uint8_t huey;                                 // temp

  // Disable unused peripherals and prepare sleep mode to save power
  ACSR   =  (1<<ACD);                           // disable analog comparator
  PRR    =  (1<<PRADC)|(1<<PRTIM0);             // shut down ADC and timer0
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);          // set sleep mode to power down

  // Setup
  NEO_init();                                   // init neopixels
  ENC_init();                                   // init rotary encoder
  pinIntEn(PIN_ENC_SW);                         // enable pin change interrupt on ENC SW
  sei();                                        // enable interrupts

  // Loop
  while(1) {
    // State machine - remain state until encoder switch is pressed
    while(pinRead(PIN_ENC_SW)) {
      switch(state) {
        // State 0: device is in sleep mode
        case 0:   sleep_mode();                 // sleep until rotary button pressed
                  break;

        // State 1: device shows white LEDs; brightness can be adjusted via encoder
        case 1:   bright = ENC_get();           // get brightness from encoder
                  for(uint8_t i=8; i; i--)      // set all 8 pixels ...
                    NEO_writeWhite(bright);     // ... to white at specified brightness
                  NEO_latch();                  // latch the pixels
                  break;

        // State 2: device shows color animation; speed can be adjusted
        case 2:   if(!--counter) {                // time for next animation step?
                    huey = anim;                  // get hue for first pixel
                    for(uint8_t i=8; i; i--) {    // set all 8 pixels ...
                      NEO_writeHue(huey, bright); // ... to a different color
                      huey += 6;                  // color step 6 (48/8)
                      if(huey > 47) huey -= 48;   // make sure it's a color ring
                    }
                    if(++anim > 47) anim = 0;     // shift starting color for next step
                    counter = ENC_get();          // get animation speed from encoder
                  }
                  _delay_ms(21);                  // a little delay; also latch
                  break;

        // State 3: device shows one color that can be adjusted
        case 3:   hue = ENC_get();              // get hue from encoder
                  for(uint8_t i=8; i; i--)      // set all 8 pixels ...
                    NEO_writeHue(hue, bright);  // ... to the specified hue
                  NEO_latch();                  // latch the pixels
                  break;
        default:  break;
      }
    }

    // Encoder switch pressed - wait until released
    _delay_ms(10);                              // debounce
    while(!pinRead(PIN_ENC_SW));                // wait for button to be released
    _delay_ms(10);                              // debounce

    // Change state after encoder switch was pressed
    if(++state > 3) state = 0;                  // next state
    switch(state) {
      case 0:     NEO_off();                    // switch off neopixels
                  break;
      case 1:     NEO_on();                     // switch on neopixels
                  ENC_set(2, 15, 1, bright, 0); // set rotary encoder
                  break;
      case 3:     ENC_set(0, 48, 1, hue, 1);    // set rotary encoder
                  break;
      default:    break;
    }
  }
}
