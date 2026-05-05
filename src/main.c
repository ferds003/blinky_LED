#include <stdint.h>

/* ---- Register map ---- */

#define PERIPH_BASE   0x40000000UL
#define AHB1_BASE     (PERIPH_BASE + 0x00020000UL)
#define APB1_BASE     (PERIPH_BASE + 0x00000000UL)

#define RCC_BASE      (AHB1_BASE + 0x3800UL)
#define RCC_AHB1ENR   (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB1ENR   (*(volatile uint32_t*)(RCC_BASE + 0x40))

#define GPIOA_BASE    (AHB1_BASE + 0x0000UL)
#define GPIOA_MODER   (*(volatile uint32_t*)(GPIOA_BASE + 0x00))
#define GPIOA_ODR     (*(volatile uint32_t*)(GPIOA_BASE + 0x14))

#define GPIOB_BASE    (AHB1_BASE + 0x0400UL)
#define GPIOB_MODER   (*(volatile uint32_t*)(GPIOB_BASE + 0x00))
#define GPIOB_OTYPER  (*(volatile uint32_t*)(GPIOB_BASE + 0x04))
#define GPIOB_OSPEEDR (*(volatile uint32_t*)(GPIOB_BASE + 0x08))
#define GPIOB_PUPDR   (*(volatile uint32_t*)(GPIOB_BASE + 0x0C))
#define GPIOB_IDR     (*(volatile uint32_t*)(GPIOB_BASE + 0x10))
#define GPIOB_AFRH    (*(volatile uint32_t*)(GPIOB_BASE + 0x24))

#define GPIOC_BASE    (AHB1_BASE + 0x0800UL)
#define GPIOC_MODER   (*(volatile uint32_t*)(GPIOC_BASE + 0x00))
#define GPIOC_PUPDR   (*(volatile uint32_t*)(GPIOC_BASE + 0x0C))
#define GPIOC_IDR     (*(volatile uint32_t*)(GPIOC_BASE + 0x10))

#define I2C1_BASE     (APB1_BASE + 0x5400UL)
#define I2C1_CR1      (*(volatile uint32_t*)(I2C1_BASE + 0x00))
#define I2C1_CR2      (*(volatile uint32_t*)(I2C1_BASE + 0x04))
#define I2C1_CCR      (*(volatile uint32_t*)(I2C1_BASE + 0x1C))
#define I2C1_TRISE    (*(volatile uint32_t*)(I2C1_BASE + 0x20))
#define I2C1_SR1      (*(volatile uint32_t*)(I2C1_BASE + 0x14))
#define I2C1_SR2      (*(volatile uint32_t*)(I2C1_BASE + 0x18))
#define I2C1_DR       (*(volatile uint32_t*)(I2C1_BASE + 0x10))

/* ---- Morse timing (all multiples of UNIT) ---- */

#define UNIT        200000UL
#define DOT         (1UL * UNIT)
#define DASH        (3UL * UNIT)
#define GAP_SYMBOL  (1UL * UNIT)
#define GAP_LETTER  (3UL * UNIT)
#define GAP_WORD    (7UL * UNIT)

/* ---- PCF8574 backpack bit layout: D7 D6 D5 D4 BL EN RW RS ---- */

#define LCD_ADDR  0x27   /* try 0x3F if display stays blank */
#define LCD_BL    0x08
#define LCD_EN    0x04
#define LCD_RS    0x01
#define LCD_COLS  16

/* ---- Encoder pins (Task 4, not yet active) ---- */
/* B1 = PC13 onboard button (active LOW)           */
/* B2 = PB4  external button (active LOW)          */
/* NRST (black button) is hardware reset, not GPIO */

#define ENC_B1_IDR  GPIOC_IDR
#define ENC_B1_PIN  13
#define ENC_B2_IDR  GPIOB_IDR
#define ENC_B2_PIN  4

/* ---- Morse decode table ---- */
/* code: MSB-first bit string, 0=dot 1=dash        */

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

/* ---- Utility ---- */

static void delay(volatile uint32_t n) { while (n--); }

static void delay_us(volatile uint32_t us)
{
    while (us--) { volatile uint32_t n = 4; while (n--); }
}

/* ---- I2C1 ---- */

static void i2c1_init(void)
{
    RCC_AHB1ENR |= (1u<<1);   /* GPIOBEN */
    RCC_APB1ENR |= (1u<<21);  /* I2C1EN  */

    /* PB8=SCL PB9=SDA: AF4, open-drain, fast, no pull */
    GPIOB_MODER   &= ~((3u<<(8*2))|(3u<<(9*2)));
    GPIOB_MODER   |=  ((2u<<(8*2))|(2u<<(9*2)));
    GPIOB_OTYPER  |=  (1u<<8)|(1u<<9);
    GPIOB_OSPEEDR |=  (3u<<(8*2))|(3u<<(9*2));
    GPIOB_PUPDR   &= ~((3u<<(8*2))|(3u<<(9*2)));
    GPIOB_AFRH    &= ~(0xFFu);
    GPIOB_AFRH    |=  (4u<<0)|(4u<<4);

    I2C1_CR1 |=  (1u<<15);  /* SWRST */
    I2C1_CR1 &= ~(1u<<15);

    /* 16 MHz APB1, standard 100 kHz: CCR=80, TRISE=17 */
    I2C1_CR2   = 16;
    I2C1_CCR   = 80;
    I2C1_TRISE = 17;
    I2C1_CR1  |= (1u<<0);   /* PE */
}

static void i2c_write(uint8_t addr, uint8_t data)
{
    I2C1_CR1 |= (1u<<8);                  /* START */
    while (!(I2C1_SR1 & (1u<<0)));
    I2C1_DR = (uint8_t)(addr<<1);
    while (!(I2C1_SR1 & (1u<<1)));        /* ADDR  */
    (void)I2C1_SR2;
    I2C1_DR = data;
    while (!(I2C1_SR1 & (1u<<7)));        /* TXE   */
    while (!(I2C1_SR1 & (1u<<2)));        /* BTF   */
    I2C1_CR1 |= (1u<<9);                  /* STOP  */
    while (I2C1_CR1 & (1u<<9));
}

/* ---- HD44780 via PCF8574 ---- */

static void lcd_pulse(uint8_t d)
{
    i2c_write(LCD_ADDR, d | LCD_EN);
    delay_us(1);
    i2c_write(LCD_ADDR, d & (uint8_t)~LCD_EN);
    delay_us(50);
}

static void lcd_nibble(uint8_t nib, uint8_t rs)
{
    lcd_pulse((nib & 0xF0u) | LCD_BL | rs);
}

static void lcd_send(uint8_t b, uint8_t rs)
{
    lcd_nibble(b & 0xF0u,         rs);
    lcd_nibble((uint8_t)(b << 4), rs);
}

static void lcd_cmd(uint8_t c)  { lcd_send(c, 0);      delay_us(2000); }
static void lcd_char(char c)    { lcd_send((uint8_t)c, LCD_RS); delay_us(50); }

static void lcd_pos(uint8_t col, uint8_t row)
{
    const uint8_t off[] = {0x00, 0x40};
    lcd_cmd((uint8_t)(0x80 | (col + off[row])));
}

static void lcd_str(const char *s) { while (*s) lcd_char(*s++); }

static void lcd_clear(void) { lcd_cmd(0x01); delay_us(2000); }

static void lcd_init(void)
{
    delay_us(50000);
    lcd_nibble(0x30, 0); delay_us(5000);
    lcd_nibble(0x30, 0); delay_us(200);
    lcd_nibble(0x30, 0); delay_us(200);
    lcd_nibble(0x20, 0); delay_us(200);
    lcd_cmd(0x28);  /* 4-bit, 2 lines */
    lcd_cmd(0x08);  /* display off    */
    lcd_cmd(0x01);  /* clear          */
    delay_us(2000);
    lcd_cmd(0x06);  /* entry: increment */
    lcd_cmd(0x0C);  /* display on       */
}

/* ---- LCD decoder state ---- */
/* Row 0: decoded text (scrolls left when full) */
/* Row 1: live dot/dash of letter in progress   */

static char    decoded[LCD_COLS + 1];
static uint8_t dcol;

static void lcd_push_char(char c)
{
    if (dcol >= LCD_COLS) {
        for (uint8_t i = 0; i < LCD_COLS - 1; i++)
            decoded[i] = decoded[i + 1];

        decoded[LCD_COLS - 1] = c;
        decoded[LCD_COLS] = '\0';   /* keep it a valid C string */

        lcd_pos(0, 0);
        lcd_str(decoded);
    } else {
        decoded[dcol]   = c;
        decoded[dcol+1] = '\0';

        lcd_pos(dcol, 0);
        lcd_char(c);
        dcol++;
    }
}

static void lcd_reset_line0(void)
{
    for (uint8_t i = 0; i < LCD_COLS; i++)
        decoded[i] = ' ';

    decoded[LCD_COLS] = '\0';
    dcol = 0;

    lcd_pos(0, 0);
    lcd_str(decoded);
}

static void lcd_show_symbols(const uint8_t *s, uint8_t len)
{
    lcd_pos(0, 1);
    for (uint8_t i = 0; i < LCD_COLS; i++)
        lcd_char(i < len ? (s[i] ? '-' : '.') : ' ');
}

/* ---- LED ---- */

static void led_on(void)  { GPIOA_ODR |=  (1u<<5); }
static void led_off(void) { GPIOA_ODR &= ~(1u<<5); }

/* ---- Morse playback with live decode ---- */

static uint8_t cur[5];
static uint8_t cur_len;

static void dot(void)
{
    led_on(); delay(DOT); led_off(); delay(GAP_SYMBOL);
    if (cur_len < 5) { cur[cur_len] = 0; cur_len++; }
    lcd_show_symbols(cur, cur_len);
}

static void dash(void)
{
    led_on(); delay(DASH); led_off(); delay(GAP_SYMBOL);
    if (cur_len < 5) { cur[cur_len] = 1; cur_len++; }
    lcd_show_symbols(cur, cur_len);
}

static void commit(void)
{
    char c = morse_decode(cur, cur_len);
    if (c) lcd_push_char(c);
    cur_len = 0;
    lcd_show_symbols(cur, 0);
}

static void gap_letter(void) { delay(GAP_LETTER - GAP_SYMBOL); commit(); }

static void gap_word(void)
{
    delay(GAP_WORD - GAP_SYMBOL);
    commit();
    lcd_push_char(' ');
}

/* ---- Morse sequences ---- */

static void morse_I(void) { dot(); dot();                gap_letter(); }
static void morse_L(void) { dot(); dash(); dot(); dot(); gap_letter(); }
static void morse_O(void) { dash(); dash(); dash();      gap_letter(); }
static void morse_V(void) { dot(); dot(); dot(); dash(); gap_letter(); }
static void morse_E(void) { dot();                       gap_letter(); }
static void morse_Y(void) { dash(); dot(); dash(); dash(); gap_letter(); }
static void morse_U(void) { dot(); dot(); dash();        gap_letter(); }

/* ---- Encoder stub (Task 4, not yet active) ---- */
/* When ready: replace the while(1) body with encoder_loop()            */
/* Use ENC_B1_IDR/ENC_B1_PIN for dit and ENC_B2_IDR/ENC_B2_PIN for dah */
/* Timing thresholds to add:                                             */
/*   ENCODER_DIT_MAX    (2UL * UNIT)                                     */
/*   ENCODER_LETTER_GAP (3UL * UNIT)                                     */
/*   ENCODER_WORD_GAP   (7UL * UNIT)                                     */

static void encoder_loop(void)
{
    /* TODO Task 4 */
    (void)ENC_B1_IDR; (void)ENC_B1_PIN;
    (void)ENC_B2_IDR; (void)ENC_B2_PIN;
}

/* ---- GPIO init ---- */

static void gpio_init(void)
{
    RCC_AHB1ENR |= (1u<<0)|(1u<<1)|(1u<<2);  /* GPIOA, B, C */

    GPIOA_MODER &= ~(3u<<(5*2));
    GPIOA_MODER |=  (1u<<(5*2));              /* PA5 output */

    GPIOC_MODER &= ~(3u<<(13*2));             /* PC13 input */
    GPIOC_PUPDR &= ~(3u<<(13*2));
    GPIOC_PUPDR |=  (1u<<(13*2));             /* pull-up    */

    GPIOB_MODER &= ~(3u<<(4*2));              /* PB4 input  */
    GPIOB_PUPDR &= ~(3u<<(4*2));
    GPIOB_PUPDR |=  (1u<<(4*2));              /* pull-up    */
}

/* ---- Main ---- */

int main(void)
{
    gpio_init();
    i2c1_init();
    lcd_init();

    lcd_pos(0,0); lcd_str("Morse Playback");
    lcd_pos(0,1); lcd_str("I LOVE YOU");
    delay(UNIT * 10);

    lcd_clear();
    lcd_reset_line0();

    (void)encoder_loop;

    while (1)
    {
        /* LD2 flashes the Morse code */
        morse_I();
        morse_L();
        morse_O();
        morse_V();
        morse_E();
        gap_word();
        morse_Y();
        morse_O();
        morse_U();

        /* pause before repeating */
        delay(GAP_WORD * 4);

        lcd_clear();
        lcd_reset_line0();
    }
}