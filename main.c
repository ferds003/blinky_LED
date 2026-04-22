#include <stdint.h>

#define PERIPH_BASE     0x40000000
#define AHB1_OFFSET     0x00020000
#define AHB1_BASE       (PERIPH_BASE + AHB1_OFFSET)

#define GPIOA_OFFSET    0x0000
#define GPIOA_BASE      (AHB1_BASE + GPIOA_OFFSET)

#define RCC_OFFSET      0x3800
#define RCC_BASE        (AHB1_BASE + RCC_OFFSET)

// Registers
#define RCC_AHB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define GPIOA_MODER     (*(volatile uint32_t*)(GPIOA_BASE + 0x00))
#define GPIOA_ODR       (*(volatile uint32_t*)(GPIOA_BASE + 0x14))

void delay(volatile uint32_t count)
{
    while(count--);
}

int main(void)
{
    // 1. Enable clock for GPIOA
    RCC_AHB1ENR |= (1 << 0);

    // 2. Set PA5 as output
    GPIOA_MODER &= ~(3 << (5 * 2));  // clear bits
    GPIOA_MODER |=  (1 << (5 * 2));  // set as output (01)

    while (1)
    {
    // 3. Toggle PA5
        GPIOA_ODR ^= (1 << 5);
        delay(500000);
    }
}