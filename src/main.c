/*
 * main.c — Bare-Metal Morse Code Decoder + (Future) Encoder
 * Target : STM32F401RE Nucleo
 *
 * Current Tasks Implemented:
 *   Task 1 - LD2 (PA5) blinks "I LOVE YOU" in Morse code              [DONE]
 *   Task 3 - LCD (I2C1, PB8=SCL / PB9=SDA) decodes Morse in real-time [DONE]
 *
 * Future Tasks (stubs in place, NOT yet active):
 *   Task 2 - External LED on a second GPIO                             [STUB]
 *   Task 4 - B1 (PC13) = dit, B2 (PB4) = dah  Morse encoder input     [STUB]
 *
 * Hardware:
 *   LCD backpack : PCF8574 I/O expander, I2C address 0x27
 *   Pinout       : D15 = PB8 (SCL), D14 = PB9 (SDA)  [Arduino headers]
 *
 * No HAL, no CMSIS, no stdlib.
 */

#include <stdint.h>

/* ============================================================
   1.  REGISTER MAP
   ============================================================ */

#define PERIPH_BASE     0x40000000UL
#define AHB1_BASE       (PERIPH_BASE + 0x00020000UL)
#define APB1_BASE       (PERIPH_BASE + 0x00000000UL)

/* RCC */
#define RCC_BASE        (AHB1_BASE + 0x3800UL)
#define RCC_AHB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x40))

/* GPIOA  (LD2 = PA5) */
#define GPIOA_BASE      (AHB1_BASE + 0x0000UL)
#define GPIOA_MODER     (*(volatile uint32_t*)(GPIOA_BASE + 0x00))
#define GPIOA_ODR       (*(volatile uint32_t*)(GPIOA_BASE + 0x14))

/* GPIOB  (PB8=SCL, PB9=SDA, PB4=B2-future) */
#define GPIOB_BASE      (AHB1_BASE + 0x0400UL)
#define GPIOB_MODER     (*(volatile uint32_t*)(GPIOB_BASE + 0x00))
#define GPIOB_OTYPER    (*(volatile uint32_t*)(GPIOB_BASE + 0x04))
#define GPIOB_OSPEEDR   (*(volatile uint32_t*)(GPIOB_BASE + 0x08))
#define GPIOB_PUPDR     (*(volatile uint32_t*)(GPIOB_BASE + 0x0C))
#define GPIOB_IDR       (*(volatile uint32_t*)(GPIOB_BASE + 0x10))
#define GPIOB_AFRH      (*(volatile uint32_t*)(GPIOB_BASE + 0x24))

/* GPIOC  (PC13 = B1, onboard user button) */
#define GPIOC_BASE      (AHB1_BASE + 0x0800UL)
#define GPIOC_MODER     (*(volatile uint32_t*)(GPIOC_BASE + 0x00))
#define GPIOC_PUPDR     (*(volatile uint32_t*)(GPIOC_BASE + 0x0C))
#define GPIOC_IDR       (*(volatile uint32_t*)(GPIOC_BASE + 0x10))

/* I2C1  (APB1) */
#define I2C1_BASE       (APB1_BASE + 0x5400UL)
#define I2C1_CR1        (*(volatile uint32_t*)(I2C1_BASE + 0x00))
#define I2C1_CR2        (*(volatile uint32_t*)(I2C1_BASE + 0x04))
#define I2C1_CCR        (*(volatile uint32_t*)(I2C1_BASE + 0x1C))
#define I2C1_TRISE      (*(volatile uint32_t*)(I2C1_BASE + 0x20))
#define I2C1_SR1        (*(volatile uint32_t*)(I2C1_BASE + 0x14))
#define I2C1_SR2        (*(volatile uint32_t*)(I2C1_BASE + 0x18))
#define I2C1_DR         (*(volatile uint32_t*)(I2C1_BASE + 0x10))

/* ============================================================
   2.  MORSE TIMING
   ============================================================ */

#define UNIT            200000UL
#define DOT             (1UL * UNIT)
#define DASH            (3UL * UNIT)
#define GAP_SYMBOL      (1UL * UNIT)
#define GAP_LETTER      (3UL * UNIT)
#define GAP_WORD        (7UL * UNIT)

/* ============================================================
   3.  PCF8574 / LCD CONFIGURATION
   ============================================================
 *
 * PCF8574 bit layout for a standard HD44780 backpack:
 *   P7 P6 P5 P4  P3  P2  P1  P0
 *   D7 D6 D5 D4  BL  EN  RW  RS
 */
#define LCD_I2C_ADDR    0x27   /* try 0x3F if display is blank */
#define LCD_BL          0x08
#define LCD_EN          0x04
#define LCD_RS          0x01
#define LCD_COLS        16

/* ============================================================
   4.  FUTURE ENCODER PINS  (Task 4 -- NOT YET ACTIVE)
   ============================================================
 *
 * B1 = PC13  onboard blue button, active LOW
 * B2 = PB4   external button,     active LOW
 *
 * Task 4 plan:
 *   Short press B1 -> dot,  long press B1 -> dash
 *   OR use B2 as a dedicated dash (iambic paddle style)
 */
#define ENCODER_B1_PORT_IDR  GPIOC_IDR
#define ENCODER_B1_PIN       13
#define ENCODER_B2_PORT_IDR  GPIOB_IDR
#define ENCODER_B2_PIN       4

/* ============================================================
   5.  MORSE DECODE TABLE
   ============================================================ */

typedef struct { char ch; uint8_t len; uint8_t code; } MorseEntry;

static const MorseEntry morse_table[] = {
    {'A',2,0x40},{'B',4,0x80},{'C',4,0xA0},{'D',3,0x80},
    {'E',1,0x00},{'F',4,0x20},{'G',3,0xC0},{'H',4,0x00},
    {'I',2,0x00},{'J',4,0x70},{'K',3,0xA0},{'L',4,0x40},
    {'M',2,0xC0},{'N',2,0x80},{'O',3,0xE0},{'P',4,0x60},
    {'Q',4,0xD0},{'R',3,0x40},{'S',3,0x00},{'T',1,0x80},
    {'U',3,0x20},{'V',4,0x10},{'W',3,0x60},{'X',4,0x90},
    {'Y',4,0xB0},{'Z',4,0xC0},
    {'1',5,0x78},{'2',5,0x38},{'3',5,0x18},{'4',5,0x08},
    {'5',5,0x00},{'6',5,0x80},{'7',5,0xC0},{'8',5,0xE0},
    {'9',5,0xF0},{'0',5,0xF8},
};
#define MORSE_TABLE_LEN (sizeof(morse_table)/sizeof(morse_table[0]))

static char morse_decode(const uint8_t *s, uint8_t len)
{
    if (!len) return 0;
    uint8_t code = 0;
    for (uint8_t i = 0; i < len && i < 8; i++)
        if (s[i]) code |= (uint8_t)(0x80u >> i);
    for (uint32_t i = 0; i < MORSE_TABLE_LEN; i++)
        if (morse_table[i].len == len && morse_table[i].code == code)
            return morse_table[i].ch;
    return '?';
}

/* ============================================================
   6.  UTILITY
   ============================================================ */

static void delay(volatile uint32_t n) { while (n--); }

static void delay_us(volatile uint32_t us)
{
    while (us--) { volatile uint32_t n = 4; while (n--); }
}

/* ============================================================
   7.  I2C1 DRIVER
   ============================================================ */

static void i2c1_init(void)
{
    RCC_AHB1ENR |= (1u<<1);    /* GPIOBEN */
    RCC_APB1ENR |= (1u<<21);   /* I2C1EN  */

    /* PB8=SCL, PB9=SDA: AF4, open-drain, high-speed, no pull */
    GPIOB_MODER  &= ~((3u<<(8*2))|(3u<<(9*2)));
    GPIOB_MODER  |=  ((2u<<(8*2))|(2u<<(9*2)));
    GPIOB_OTYPER |=  (1u<<8)|(1u<<9);
    GPIOB_OSPEEDR|=  (3u<<(8*2))|(3u<<(9*2));
    GPIOB_PUPDR  &= ~((3u<<(8*2))|(3u<<(9*2)));
    GPIOB_AFRH   &= ~(0xFFu);
    GPIOB_AFRH   |=  (4u<<0)|(4u<<4);   /* AF4 for pins 8,9 */

    I2C1_CR1 |=  (1u<<15);  /* SWRST */
    I2C1_CR1 &= ~(1u<<15);

    /* 16 MHz APB1, standard mode 100 kHz */
    I2C1_CR2   = 16;
    I2C1_CCR   = 80;
    I2C1_TRISE = 17;
    I2C1_CR1  |= (1u<<0);   /* PE */
}

static void i2c_write_byte(uint8_t addr, uint8_t data)
{
    I2C1_CR1 |= (1u<<8);                    /* START    */
    while (!(I2C1_SR1 & (1u<<0)));          /* wait SB  */
    I2C1_DR = (uint8_t)(addr<<1);
    while (!(I2C1_SR1 & (1u<<1)));          /* wait ADDR*/
    (void)I2C1_SR2;
    I2C1_DR = data;
    while (!(I2C1_SR1 & (1u<<7)));          /* wait TXE */
    while (!(I2C1_SR1 & (1u<<2)));          /* wait BTF */
    I2C1_CR1 |= (1u<<9);                    /* STOP     */
    while (I2C1_CR1 & (1u<<9));
}

/* ============================================================
   8.  HD44780 VIA PCF8574
   ============================================================ */

static void lcd_pulse_en(uint8_t d)
{
    i2c_write_byte(LCD_I2C_ADDR, d | LCD_EN);
    delay_us(1);
    i2c_write_byte(LCD_I2C_ADDR, d & (uint8_t)~LCD_EN);
    delay_us(50);
}

static void lcd_write_nibble(uint8_t nib, uint8_t rs)
{
    lcd_pulse_en((nib & 0xF0u) | LCD_BL | rs);
}

static void lcd_send(uint8_t byte, uint8_t rs)
{
    lcd_write_nibble(byte & 0xF0u,          rs);
    lcd_write_nibble((uint8_t)(byte << 4),  rs);
}

static void lcd_cmd(uint8_t c)  { lcd_send(c, 0);      delay_us(2000); }
static void lcd_char(char c)    { lcd_send((uint8_t)c, LCD_RS); delay_us(50); }

static void lcd_set_cursor(uint8_t col, uint8_t row)
{
    const uint8_t offsets[] = {0x00, 0x40};
    lcd_cmd((uint8_t)(0x80 | (col + offsets[row])));
}

static void lcd_print(const char *s) { while (*s) lcd_char(*s++); }

static void lcd_clear(void) { lcd_cmd(0x01); delay_us(2000); }

static void lcd_init(void)
{
    delay_us(50000);
    lcd_write_nibble(0x30, 0); delay_us(5000);
    lcd_write_nibble(0x30, 0); delay_us(200);
    lcd_write_nibble(0x30, 0); delay_us(200);
    lcd_write_nibble(0x20, 0); delay_us(200);
    lcd_cmd(0x28);   /* 4-bit, 2 lines */
    lcd_cmd(0x08);   /* display off    */
    lcd_cmd(0x01);   /* clear          */
    delay_us(2000);
    lcd_cmd(0x06);   /* entry: inc      */
    lcd_cmd(0x0C);   /* display on      */
}

/* ============================================================
   9.  LCD DECODER STATE
   ============================================================
 * Row 0: decoded characters (scrolls when full)
 * Row 1: live dots/dashes of the letter being received
 */

static char    decoded_line[LCD_COLS + 1];
static uint8_t decoded_col;

static void lcd_append_char(char c)
{
    if (decoded_col >= LCD_COLS) {
        for (uint8_t i = 0; i < LCD_COLS - 1; i++)
            decoded_line[i] = decoded_line[i+1];
        decoded_line[LCD_COLS-1] = c;
        lcd_set_cursor(0,0);
        lcd_print(decoded_line);
    } else {
        decoded_line[decoded_col]   = c;
        decoded_line[decoded_col+1] = '\0';
        lcd_set_cursor(decoded_col, 0);
        lcd_char(c);
        decoded_col++;
    }
}

static void lcd_show_morse_row(const uint8_t *syms, uint8_t len)
{
    lcd_set_cursor(0, 1);
    for (uint8_t i = 0; i < LCD_COLS; i++)
        lcd_char(i < len ? (syms[i] ? '-' : '.') : ' ');
}

/* ============================================================
   10. LED  (LD2 = PA5)
   ============================================================ */

static void led_on(void)  { GPIOA_ODR |=  (1u<<5); }
static void led_off(void) { GPIOA_ODR &= ~(1u<<5); }

/* ============================================================
   11. MORSE PLAYBACK WITH LIVE DECODE
   ============================================================ */

static uint8_t cur_syms[5];
static uint8_t cur_sym_len;

static void dot(void)
{
    led_on(); delay(DOT); led_off(); delay(GAP_SYMBOL);
    if (cur_sym_len < 5) { cur_syms[cur_sym_len] = 0; cur_sym_len++; }
    lcd_show_morse_row(cur_syms, cur_sym_len);
}

static void dash(void)
{
    led_on(); delay(DASH); led_off(); delay(GAP_SYMBOL);
    if (cur_sym_len < 5) { cur_syms[cur_sym_len] = 1; cur_sym_len++; }
    lcd_show_morse_row(cur_syms, cur_sym_len);
}

static void commit_letter(void)
{
    char c = morse_decode(cur_syms, cur_sym_len);
    if (c) lcd_append_char(c);
    cur_sym_len = 0;
    lcd_show_morse_row(cur_syms, 0);
}

static void gap_letter(void) { delay(GAP_LETTER - GAP_SYMBOL); commit_letter(); }

static void gap_word(void)
{
    delay(GAP_WORD - GAP_SYMBOL);
    commit_letter();
    lcd_append_char(' ');
}

/* ============================================================
   12. MORSE SEQUENCES  -- "I LOVE YOU"
   ============================================================ */

static void morse_I(void) { dot(); dot();                gap_letter(); }
static void morse_L(void) { dot(); dash(); dot(); dot(); gap_letter(); }
static void morse_O(void) { dash(); dash(); dash();      gap_letter(); }
static void morse_V(void) { dot(); dot(); dot(); dash(); gap_letter(); }
static void morse_E(void) { dot();                       gap_letter(); }
static void morse_Y(void) { dash(); dot(); dash(); dash(); gap_letter(); }
static void morse_U(void) { dot(); dot(); dash();        gap_letter(); }

/* ============================================================
   13. FUTURE: ENCODER STUB  (Task 4 -- NOT YET ACTIVE)
   ============================================================
 *
 * To activate Task 4, replace the while(1) body in main() with:
 *     encoder_loop();
 *
 * Implementation guide:
 *   1. Poll ENCODER_B1_PORT_IDR bit ENCODER_B1_PIN (active LOW).
 *   2. On press-start, start counting elapsed "ticks".
 *   3. On release:
 *        if ticks < ENCODER_DIT_MAX  -> call dot()
 *        else                         -> call dash()
 *   4. After press, count silence:
 *        if silence >= ENCODER_LETTER_GAP -> gap_letter()
 *        if silence >= ENCODER_WORD_GAP   -> gap_word()
 *   5. Optionally: ENCODER_B2 = dedicated dash button (iambic).
 *
 * Timing hints (add these when implementing):
 *   #define ENCODER_DIT_MAX    (2UL * UNIT)
 *   #define ENCODER_LETTER_GAP (3UL * UNIT)
 *   #define ENCODER_WORD_GAP   (7UL * UNIT)
 */
static void encoder_loop(void)
{
    /* TODO Task 4 */
    (void)ENCODER_B1_PORT_IDR;
    (void)ENCODER_B1_PIN;
    (void)ENCODER_B2_PORT_IDR;
    (void)ENCODER_B2_PIN;
}

/* ============================================================
   14. GPIO INIT
   ============================================================ */

static void gpio_init(void)
{
    RCC_AHB1ENR |= (1u<<0)|(1u<<1)|(1u<<2);  /* GPIOA, GPIOB, GPIOC */

    /* PA5 = output (LD2) */
    GPIOA_MODER &= ~(3u<<(5*2));
    GPIOA_MODER |=  (1u<<(5*2));

    /* PC13 = input, pull-up (B1, active LOW) */
    GPIOC_MODER &= ~(3u<<(13*2));
    GPIOC_PUPDR &= ~(3u<<(13*2));
    GPIOC_PUPDR |=  (1u<<(13*2));

    /* PB4 = input, pull-up (B2 placeholder for Task 4) */
    GPIOB_MODER &= ~(3u<<(4*2));
    GPIOB_PUPDR &= ~(3u<<(4*2));
    GPIOB_PUPDR |=  (1u<<(4*2));
}

/* ============================================================
   15. MAIN
   ============================================================ */

int main(void)
{
    gpio_init();
    i2c1_init();
    lcd_init();

    lcd_set_cursor(0,0); lcd_print("Morse Decoder");
    lcd_set_cursor(0,1); lcd_print("I LOVE YOU");
    delay(UNIT * 10);
    lcd_clear();

    for (uint8_t i = 0; i < LCD_COLS; i++) decoded_line[i] = ' ';
    decoded_line[LCD_COLS] = '\0';
    decoded_col = 0;

    (void)encoder_loop;   /* suppress unused warning until Task 4 */

    while (1)
    {
        /*
         * DECODER MODE (current behaviour):
         *   LED blinks, LCD decodes simultaneously.
         *
         * When Task 4 is ready, swap this block for:
         *   encoder_loop();
         */
        morse_I();

        morse_L(); morse_O(); morse_V(); morse_E();
        gap_word();

        morse_Y(); morse_O(); morse_U();

        /* Long pause, then reset display */
        delay(GAP_WORD * 4);
        lcd_clear();
        decoded_col = 0;
        for (uint8_t i = 0; i < LCD_COLS; i++) decoded_line[i] = ' ';
        decoded_line[LCD_COLS] = '\0';
    }
}