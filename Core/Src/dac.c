#include "stdbool.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_gpio_ex.h"
#include <stddef.h>
#include <stdint.h>

typedef enum { ALTERNATIVE = 2 } GpioMode;

struct interrupt_handler_functions {
	void (*on_uif)();
	void (*on_ccr1)();
};

// different wave select functions will alter this global struct
static struct interrupt_handler_functions callbacks;

//--------------------------------------------------
// other globals
static const int CLOCK_FREQUENCY = 4000000;

static const float DC_BIAS = 1.5f;
static const float V_PEAK_TO_PEAK = 3.0f;

static const float V_MIN = DC_BIAS - V_PEAK_TO_PEAK / 2;
static const float V_MAX = DC_BIAS + V_PEAK_TO_PEAK / 2;

//--------------------------------------------------

// forward-declarations
void TIM2_start(uint32_t prescaler, uint32_t auto_reload_register, uint32_t capture_compare_register);
void SPI_send_data(uint16_t value);
uint16_t SPI_volt_convert(float voltOut);
void dac_send(float voltOut);

//--------------------------------------------------
// interface

void dac_start(uint16_t frequency_hz, uint8_t percent_duty)
{
	// keep the update frequency at its smallest granularity
	uint32_t prescaler = 0;

	// how many clock ticks until the desired period is reached? subtract 1 due to 0-indexing of CNT
	uint32_t auto_reload_register = CLOCK_FREQUENCY / frequency_hz - 1;

	uint32_t capture_compare_register = (auto_reload_register + 1) * percent_duty / 100;

	TIM2_start(prescaler, auto_reload_register, capture_compare_register);

	/// TODO: mode
}

//--------------------------------------------------
// primary helpers

void sawtooth_wave_on_uif()
{
	dac_send(V_MIN);
}

void sawtooth_wave_on_ccr1()
{
	
}

void dac_send(float voltOut)
{
	uint16_t spi_out = SPI_volt_convert(voltOut);
	SPI_send_data(spi_out);
}

//--------------------------------------------------
// low-level helpers

// PA5 and PA7 alternative mode
// PA5 = SCK
// PA7 = MOSI = PICO
// PB6 = CS
void SPI_init()
{
	// GPIO clocks
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

	// SPI clock
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

	GPIOA->MODER &= ~(GPIO_MODER_MODE5_Msk | GPIO_MODER_MODE7_Msk);
	GPIOA->MODER |= 0b10 << GPIO_MODER_MODE5_Pos;
	GPIOA->MODER |= 0b10 << GPIO_MODER_MODE7_Pos;

	// AF=5=SPI for PA5 and PA7
	GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL5_Msk | GPIO_AFRL_AFSEL7_Msk);
	GPIOA->AFR[0] |= GPIO_AF5_SPI1 << GPIO_AFRL_AFSEL5_Pos;
	GPIOA->AFR[0] |= GPIO_AF5_SPI1 << GPIO_AFRL_AFSEL7_Pos;

	GPIOB->MODER &= ~(GPIO_MODER_MODE6_Msk);
	GPIOB->MODER |= (0b01 << GPIO_MODER_MODE6_Pos);

	// Enable GPIOB clock (if not already enabled)
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

	// PB6: General-purpose output (01)
	GPIOB->MODER &= ~(GPIO_MODER_MODE6_Msk);
	GPIOB->MODER |= (0b01 << GPIO_MODER_MODE6_Pos);

	// PB6: Push-pull output (0)
	GPIOB->OTYPER &= ~(1U << 6);

	// PB6: No pull-up or pull-down (00)
	GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD6_Msk);

	// PB6: Low-speed output (00)
	GPIOB->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED6_Msk); // ?

	// Initialize chip select as high (idle)
	GPIOB->BSRR = GPIO_BSRR_BS6;

	// disable SPI while configuring
	SPI1->CR1 &= ~SPI_CR1_SPE_Msk;

	// ----
	// CR1 config
	SPI1->CR1 |= SPI_CR1_MSTR;

	// baud rate calculation
	SPI1->CR1 &= ~(SPI_CR1_BR_Msk);
	SPI1->CR1 |= 1 << SPI_CR1_BR_Pos;

	// CPOL = 0 = idle low
	SPI1->CR1 &= ~(SPI_CR1_CPOL);

	// CPHA = 0 = rising edge sample
	SPI1->CR1 &= ~(SPI_CR1_CPHA);

	// LSBFIRST = 0 => Most Significant Bit first
	SPI1->CR1 &= ~(SPI_CR1_LSBFIRST);

	// SSM = 1 = NSS is managed by software through SSI instead of hardware
	SPI1->CR1 |= SPI_CR1_SSM;

	// SSI = 1 = internal NSS kept high in controller mode
	SPI1->CR1 |= SPI_CR1_SSI;

	// 16 bit frame
	SPI1->CR2 &= ~SPI_CR2_DS_Msk;
	SPI1->CR2 |= (0b1111 << SPI_CR2_DS_Pos);

	// -----------
	// re-enable SPI
	SPI1->CR1 |= SPI_CR1_SPE;
}

void SPI_send_data(uint16_t value)
{
	// header may be different depending on the target

	// A/B = 0 = DAC A
	// BUF = 0 = unbuffered VREF
	// GA = 1 = 1x gain
	// SHDN = 1 enable output

	// TXE = 0: transmit buffer is not empty
	// TXE = 1: transmit buffer is empty

	// == 0b0011

	uint16_t command = 0x3 << 12 | (value & 0x0FFF);

	// CS low = begin transmission
	// PB6 is CS in STM32
	// BSRR modifies ODR
	GPIOB->BSRR = GPIO_BSRR_BR6;

	// wait until transmit buffer is empty
	while (!(SPI1->SR & SPI_SR_TXE))
		;

	SPI1->DR = command;

	// wait until the transmit buffer is empty.
	// if we don't do this, we might check the BSY flag before
	// any data actually entered SPI, which would be result in a
	// premature end of transmission
	while (!(SPI1->SR & SPI_SR_TXE))
		;

	// after the data has exited the buffer, check that SPI is done
	// processing it.
	while (SPI1->SR & SPI_SR_BSY)
		;

	// B6 high = CS high = end transmission
	GPIOB->BSRR = GPIO_BSRR_BS6;
}

uint16_t SPI_volt_convert(float voltOut)
{
	//Calculate the output voltage
	float gain = 1.0;
	float DAC_value = (voltOut * 4096.0f) / (3.3f * gain);

	//Clamp to valid range
	if (DAC_value < 0.0) {
		DAC_value = 0.0;
	}

	if (DAC_value > 4095.0) {
		DAC_value = 4095.0;
	}

	//Cast the data to valid type and return
	uint16_t ans = (uint16_t)(DAC_value);
	return ans;
}

void TIM2_start(uint32_t prescaler, uint32_t auto_reload_register, uint32_t capture_compare_register)
{
	// disable
	TIM2->CR1 &= ~(TIM_CR1_CEN_Msk);

	// configure
	TIM2->CNT = 0;
	TIM2->PSC = prescaler;
	TIM2->ARR = auto_reload_register;
	TIM2->CCR1 = capture_compare_register;

	// enable
	TIM2->CR1 |= TIM_CR1_CEN_Msk;
}

void TIM2_init()
{
	// APB1 = advanced peripheral bus
	// enable the CPU to TIM2 connection for configuring.
	// also connects a peripheral clock to TIM2
	RCC->APB1ENR1 |= (RCC_APB1ENR1_TIM2EN_Msk);

	// disable timer while configuring
	TIM2->CR1 &= ~(TIM_CR1_CEN_Msk);

	// CR1.DIR is up count by default
	// this is what we want

	// start at a known val
	TIM2->CNT = 0;

	// enable update interrupt
	// goes off whenever CNT == ARR
	TIM2->DIER |= TIM_DIER_UIE_Msk;

	// enable CCR1
	// goes off whenever CNT = CCR1
	TIM2->DIER |= TIM_DIER_CC1IE;

	// initialize the update interrupt to zero
	TIM2->SR &= ~(TIM_SR_UIF_Msk);

	// enable TIM2 in the NVIC
	// TIM2 is ISER[0] within NVIC.
	NVIC->ISER[0] = 1 << TIM2_IRQn;

	// can also do this
	// NVIC_EnableIRQ(TIM2_IRQn);

	// terminology:
	// DIER enable timer interrupt requests?
	// EGR generate timer events by software?
	// CR counter enable
	// UIE - update interrupt enable
	// CEN - counter enable

	// do not enable until PSC, ARR, CCR1 are set
}

void TIM2_IRQHandler()
{
	if (TIM2->SR & TIM_SR_UIF) {
		callbacks.on_uif();

		// SPI_send_data(0xFFF);

		// always clear flag
		TIM2->SR &= ~(TIM_SR_UIF);
		return;
	}

	if (TIM2->SR & TIM_SR_CC1IF) {
		callbacks.on_ccr1();

		// clear flag
		TIM2->SR &= ~(TIM_SR_CC1IF);
		return;
	}
}
