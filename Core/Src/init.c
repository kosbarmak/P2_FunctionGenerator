//init_c
#include "init.h"
#include "stdbool.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include <stdint.h>

// Function prototypes
void keypad_gpio_init();
void bus_init();

// Init every pin for this project
void gpio_init()
{
	HAL_Init();
	bus_init();
	keypad_gpio_init();
}

// Start the 2 hearts
void bus_init()
{
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOCEN);
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN);
}

// Keypad will use pins GPIOA 5-8 and GPIOC 10-12
void keypad_gpio_init()
{
	// set rows as input
	// PC0-3
	GPIOA->MODER &= ~(GPIO_MODER_MODE5 | GPIO_MODER_MODE6 | GPIO_MODER_MODE7 | GPIO_MODER_MODE8);

	// set rows as pull-down
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD7 | GPIO_PUPDR_PUPD8);
	GPIOA->PUPDR |= (GPIO_PUPDR_PUPD5_1 | GPIO_PUPDR_PUPD6_1 | GPIO_PUPDR_PUPD7_1 | GPIO_PUPDR_PUPD8_1);

	// set cols as output
	// PC10-12
	GPIOC->MODER &= ~(GPIO_MODER_MODE10 | GPIO_MODER_MODE11 | GPIO_MODER_MODE12);
	GPIOC->MODER |= (GPIO_MODER_MODE10_0 | GPIO_MODER_MODE11_0 | GPIO_MODER_MODE12_0);
}

