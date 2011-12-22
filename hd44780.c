/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * HD44780 LCD display driver
 *
 * Modified for a custom 8-bit mode (v2) by Urja Rannikko 2010.
 * Pinout of this 8-bit mode:
 * E = PD7, LED backlight = PD6, RW = PD1, RS = PD4, contrast = PD5. PORTB = data port.
 *
 * The LCD controller is used in 8-bit mode with a full bi-directional
 * interface (i.e. R/~W is connected) so the busy flag can be read.
 *
 */

#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>

#include "hd44780.h"

#define HD44780_BUSYFLAG 0x80

#define LCD_E _BV(7)
#define LCD_RW _BV(1)
#define LCD_RS _BV(4)


/*
 * Send one pulse to the E signal (enable).  Mind the timing
 * constraints.  If readback is set to true, read the HD44780 data
 * pins right before the falling edge of E, and return that value.
 */
static inline uint8_t
hd44780_pulse_e(bool readback) __attribute__((always_inline));

static inline uint8_t
hd44780_pulse_e(bool readback)
{
  uint8_t x;

  PORTD |= LCD_E;
  /*
   * Guarantee at least 500 ns of pulse width.  For high CPU
   * frequencies, a delay loop is used.  For lower frequencies, NOPs
   * are used, and at or below 1 MHz, the native pulse width will
   * already be 1 us or more so no additional delays are needed.
   */
#if F_CPU > 4000000UL
  _delay_us(0.5);
#else
  /*
   * When reading back, we need one additional NOP, as the value read
   * back from the input pin is sampled close to the beginning of a
   * CPU clock cycle, while the previous edge on the output pin is
   * generated towards the end of a CPU clock cycle.
   */
  if (readback)
    __asm__ volatile("nop");
#  if F_CPU > 1000000UL
  __asm__ volatile("nop");
#    if F_CPU > 2000000UL
  __asm__ volatile("nop");
  __asm__ volatile("nop");
#    endif /* F_CPU > 2000000UL */
#  endif /* F_CPU > 1000000UL */
#endif
  if (readback)
    x = PINB;
  else
    x = 0;
  PORTD &= ~LCD_E;
  return x;
}


/*
 * Send one byte to the LCD controller.
 */
void
hd44780_outbyte(uint8_t b, uint8_t rs)
{
  PORTD &= ~LCD_RW; // RW
  if (rs)
    PORTD |= LCD_RS; // RS
  else
    PORTD &= ~LCD_RS; // RS
  PORTB = b;
  (void)hd44780_pulse_e(false);
}



/*
 * Read one byte from the LCD controller.
 */
uint8_t
hd44780_inbyte(uint8_t rs)
{
  uint8_t x;

  PORTD |= LCD_RW; // RW
  DDRB = 0; // DATABUS IN
  if (rs)
    PORTD |= LCD_RS; // RS
  else
    PORTD &= ~LCD_RS; // RS
  x = hd44780_pulse_e(true);
  DDRB = 0xFF; // DATABUS OUT
  PORTD &= ~LCD_RW;
  return x;
}

/*
 * Wait until the busy flag is cleared.
 */
void
hd44780_wait_ready(void)
{
  uint16_t cnt=30000; // With 1us/read this is 30ms+, should be enough.
  while ((hd44780_incmd() & HD44780_BUSYFLAG)&&(cnt)) cnt--;
}

/*
 * Initialize the LCD controller.
 *
 * The initialization sequence has a mandatory timing so the
 * controller can safely recognize the type of interface desired.
 * This is the only area where timed waits are really needed as
 * the busy flag cannot be probed initially.
 */
void
hd44780_init(void)
{

  DDRB = 0xFF; // Data port out
  DDRD = 0xF2; // (comment was obsolete)
  _delay_ms(15);		/* 40 ms needed for Vcc = 2.7 V */
  hd44780_outcmd(HD44780_FNSET(1, 0, 0));
  _delay_ms(4.1);
  hd44780_outcmd(HD44780_FNSET(1, 0, 0));
  _delay_ms(0.1);
  hd44780_outcmd(HD44780_FNSET(1, 0, 0));

  hd44780_outcmd(HD44780_FNSET(1, 1, 0));
  hd44780_wait_ready();
  hd44780_outcmd(HD44780_FNSET(1, 1, 0));
  hd44780_wait_ready();
  hd44780_outcmd(HD44780_DISPCTL(0, 0, 0));
  hd44780_wait_ready();
}

