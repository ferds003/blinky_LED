#include <stdint.h>

#define PERIPH_BASE     0x40000000
#define AHB1_OFFSET     0x00020000
#define AHB1_BASE       (PERIPH_BASE + AHB1_OFFSET)

#define GPIOA_OFFSET    0x0000
#define GPIOA_BASE      (AHB1_BASE + GPIOA_OFFSET)

#define RCC_OFFSET      0x3800
#define RCC_BASE        (AHB1_BASE + RCC_OFFSET)

#define RCC_AHB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define GPIOA_MODER     (*(volatile uint32_t*)(GPIOA_BASE + 0x00))
#define GPIOA_ODR       (*(volatile uint32_t*)(GPIOA_BASE + 0x14))

/* -------------------------------------------------------
   Morse code timing (standard ITU rules):
   - 1 unit  = dot duration
   - 3 units = dash duration
   - 1 unit  = gap between dots/dashes within a letter
   - 3 units = gap between letters
   - 7 units = gap between words
------------------------------------------------------- */
#define UNIT            200000   /* 1 unit ≈ ~200ms at -O0, tune as needed */
#define DOT             (1  * UNIT)
#define DASH            (3  * UNIT)
#define GAP_SYMBOL      (1  * UNIT)   /* between dots/dashes inside a letter */
#define GAP_LETTER      (3  * UNIT)   /* between letters */
#define GAP_WORD        (7  * UNIT)   /* between words */

void delay(volatile uint32_t count)
{
    while(count--);
}

/* Turn LED on for 'duration', then off */
void led_on(void)  { GPIOA_ODR |=  (1 << 5); }
void led_off(void) { GPIOA_ODR &= ~(1 << 5); }

/* Flash a dot: on for DOT, off for GAP_SYMBOL */
void dot(void)
{
    led_on();
    delay(DOT);
    led_off();
    delay(GAP_SYMBOL);
}

/* Flash a dash: on for DASH, off for GAP_SYMBOL */
void dash(void)
{
    led_on();
    delay(DASH);
    led_off();
    delay(GAP_SYMBOL);
}

/* Call after finishing a letter to add the extra 2-unit gap
   (1 already added by last dot/dash = 3 units total) */
void gap_letter(void) { delay(GAP_LETTER - GAP_SYMBOL); }

/* Call after finishing a word (7 units total between words) */
void gap_word(void)   { delay(GAP_WORD - GAP_SYMBOL); }

/* -------------------------------------------------------
   Each function below spells one letter in Morse code
   I  = ..
   L  = .-..
   O  = ---
   V  = ...-
   E  = .
   Y  = -.--
   U  = ..-
------------------------------------------------------- */
void morse_I(void) { dot(); dot();               gap_letter(); }
void morse_L(void) { dot(); dash(); dot(); dot(); gap_letter(); }
void morse_O(void) { dash(); dash(); dash();     gap_letter(); }
void morse_V(void) { dot(); dot(); dot(); dash(); gap_letter(); }
void morse_E(void) { dot();                      gap_letter(); }
void morse_Y(void) { dash(); dot(); dash(); dash(); gap_letter(); }
void morse_U(void) { dot(); dot(); dash();       gap_letter(); }

int main(void)
{
    /* Enable GPIOA clock */
    RCC_AHB1ENR |= (1 << 0);

    /* Set PA5 as output */
    GPIOA_MODER &= ~(3 << (5 * 2));
    GPIOA_MODER |=  (1 << (5 * 2));

    while (1)
    {
        /* "I" */
        morse_I();

        /* "LOVE" */
        morse_L();
        morse_O();
        morse_V();
        morse_E();

        gap_word();   /* space between "LOVE" and "YOU" */

        /* "YOU" */
        morse_Y();
        morse_O();
        morse_U();

        /* Long pause before repeating */
        delay(GAP_WORD * 4);
    }
}