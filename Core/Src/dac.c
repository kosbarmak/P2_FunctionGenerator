#include "dac.h"

#include <math.h>

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
// wave generation globals

static int n_steps = 10;
static int CCR1_increment;
static float voltage_increment;
static int current_step;

//--------------------------------------------------
// other globals
static const int CLOCK_FREQUENCY = 4000000;

// from the example: 1 step per 1ms = 1/0.001s = 1000/s = 1000 Hz
enum {
	STEPS_PER_SECOND = 5000,
	MAX_HX = 500,
	MAX_STEPS_PER_PERIOD = STEPS_PER_SECOND / MAX_HX
};

static const float DC_BIAS = 1.5f;
static const float V_PEAK_TO_PEAK = 3.0f;

static const float V_MIN = DC_BIAS - V_PEAK_TO_PEAK / 2;
static const float V_MAX = DC_BIAS + V_PEAK_TO_PEAK / 2;

//--------------------------------------------------
// Sine lookup table
float SINES[MAX_STEPS_PER_PERIOD];

//--------------------------------------------------

// forward-declarations
void TIM2_pause();
void TIM2_restart();
void TIM2_start(uint32_t prescaler, uint32_t auto_reload_register, uint32_t capture_compare_register);
void SPI_send_data(uint16_t value);
uint16_t SPI_volt_convert(float voltOut);
void dac_send(float voltOut);
void SPI_init();
void TIM2_init();
void TIM2_ccr1_increment();
void TIM2_CCR1_reset();
void sawtooth_wave_on_uif();
void sawtooth_wave_on_ccr1();
void square_wave_on_uif();
void square_wave_on_ccr1();
void triangle_wave_on_uif();
void triangle_wave_on_ccr1();

void sine_wave_populate();
void sine_wave_on_ccr1();
void sine_wave_on_uif();

//--------------------------------------------------
// interface

void dac_init()
{
	SPI_init();
	TIM2_init();
	sine_wave_populate();
}

void dac_start(WaveType type, uint16_t frequency_hz, uint8_t percent_duty)
{
	// keep the update frequency at its smallest granularity
	uint32_t PSC = 0;
	uint32_t ARR;
	uint32_t CCR1;

	// how many clock ticks until the desired period is reached? subtract 1 due to 0-indexing of CNT
	ARR = CLOCK_FREQUENCY / frequency_hz - 1;

	if (type == SQUARE) {
		CCR1 = (ARR + 1) * percent_duty / 100;
		dac_send(V_MAX);
	} else {
		n_steps = STEPS_PER_SECOND / frequency_hz;
		CCR1_increment = ARR / n_steps;
		voltage_increment = V_MAX / (n_steps - 1);

		// 0-indexed
		current_step = 0;
		CCR1 = CCR1_increment; // first CCR1 begins step 1
	}

	switch (type) {
	case SAWTOOTH:
		callbacks.on_ccr1 = sawtooth_wave_on_ccr1;
		callbacks.on_uif = sawtooth_wave_on_uif;
		break;
	case SQUARE:
		callbacks.on_ccr1 = square_wave_on_ccr1;
		callbacks.on_uif = square_wave_on_uif;
		break;
	case TRIANGLE:
		callbacks.on_ccr1 = triangle_wave_on_ccr1;
		callbacks.on_uif = triangle_wave_on_uif;
	case SINE:
		callbacks.on_ccr1 = sine_wave_on_ccr1;
		callbacks.on_uif = sine_wave_on_uif;
	}

	TIM2_start(PSC, ARR, CCR1);
}

//--------------------------------------------------
// wave generation

void sawtooth_wave_on_uif()
{
	current_step = 0;
	TIM2_CCR1_reset();
	dac_send(V_MIN);
}

void sawtooth_wave_on_ccr1()
{
	current_step += 1;
	TIM2_ccr1_increment();
	dac_send(current_step * voltage_increment);
}

void square_wave_on_uif()
{
	dac_send(V_MAX);
}

void square_wave_on_ccr1()
{
	dac_send(V_MIN);
}

void triangle_wave_on_uif()
{
	current_step = 0;
	TIM2_CCR1_reset();
	dac_send(V_MIN);
}

void triangle_wave_on_ccr1()
{
	int peak = n_steps / 2;
	static int step = 1;

	if (current_step == 0)
		step = 1;
	else if (current_step == peak) {
		step = -1;
	}

	current_step += step;

	TIM2_ccr1_increment();

	// double the step size so the same amplitude is reached in half the steps
	dac_send(current_step * 2 * voltage_increment);
}

void sine_wave_populate()
{
	static const float dx = 2 * M_PI / MAX_STEPS_PER_PERIOD;

	for (size_t i = 0; i < MAX_STEPS_PER_PERIOD; i++) {
		SINES[i] = sinf(dx);
	}
}

void sine_wave_on_uif()
{
	current_step = 0;
	TIM2_CCR1_reset();
	dac_send(V_MIN);
}

void sine_wave_on_ccr1()
{
	current_step += 1;

	// modulate fineness
	int multiplier = MAX_STEPS_PER_PERIOD / n_steps;

	float sine = SINES[current_step * multiplier];
	float vout = V_MAX * sine;
	
	TIM2_ccr1_increment();
	dac_send(vout);
}

void dac_send(float voltOut)
{
	uint16_t spi_out = SPI_volt_convert(voltOut);
	SPI_send_data(spi_out);
}

//--------------------------------------------------
// interrupt service routine
void TIM2_IRQHandler()
{
	if (TIM2->SR & TIM_SR_UIF) {
		TIM2_pause();

		callbacks.on_uif();

		// always clear flag
		TIM2->SR &= ~(TIM_SR_UIF);
		TIM2_restart();
		return;
	}

	if (TIM2->SR & TIM_SR_CC1IF) {
		callbacks.on_ccr1();

		// clear flag
		TIM2->SR &= ~(TIM_SR_CC1IF);
		return;
	}
}

//--------------------------------------------------
// low-level helpers
void TIM2_pause()
{
	TIM2->CR1 &= ~TIM_CR1_CEN;
}

void TIM2_restart()
{
	TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM2_ccr1_increment()
{
	TIM2->CCR1 += CCR1_increment;
}

void TIM2_CCR1_reset()
{
	TIM2->CCR1 = CCR1_increment;

	// reset the flag
	TIM2->SR &= ~TIM_SR_CC1IF;
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
	while (!(SPI2->SR & SPI_SR_TXE))
		;

	SPI2->DR = command;

	// wait until the transmit buffer is empty.
	// if we don't do this, we might check the BSY flag before
	// any data actually entered SPI, which would be result in a
	// premature end of transmission
	while (!(SPI2->SR & SPI_SR_TXE))
		;

	// after the data has exited the buffer, check that SPI is done
	// processing it.
	while (SPI2->SR & SPI_SR_BSY)
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

// PB13 and PB15 alternative mode
// PB13 = SCK
// PB15 = MOSI = PICO
// PB6 = CS
void SPI_init()
{
	// GPIO clocks
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

	// SPI2 clock
	RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN;

	// PB13 = SCK, PB15 = MOSI
	GPIOB->MODER &= ~(GPIO_MODER_MODE13_Msk | GPIO_MODER_MODE15_Msk);
	GPIOB->MODER |= (0b10 << GPIO_MODER_MODE13_Pos) | (0b10 << GPIO_MODER_MODE15_Pos);

	GPIOB->AFR[1] &= ~(GPIO_AFRH_AFSEL13_Msk | GPIO_AFRH_AFSEL15_Msk);
	GPIOB->AFR[1] |= (GPIO_AF5_SPI2 << GPIO_AFRH_AFSEL13_Pos) | (GPIO_AF5_SPI2 << GPIO_AFRH_AFSEL15_Pos);

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
	SPI2->CR1 &= ~SPI_CR1_SPE_Msk;

	// ----
	// CR1 config
	SPI2->CR1 |= SPI_CR1_MSTR;

	// baud rate calculation
	SPI2->CR1 &= ~(SPI_CR1_BR_Msk);
	SPI2->CR1 |= 1 << SPI_CR1_BR_Pos;

	// CPOL = 0 = idle low
	SPI2->CR1 &= ~(SPI_CR1_CPOL);

	// CPHA = 0 = rising edge sample
	SPI2->CR1 &= ~(SPI_CR1_CPHA);

	// LSBFIRST = 0 => Most Significant Bit first
	SPI2->CR1 &= ~(SPI_CR1_LSBFIRST);

	// SSM = 1 = NSS is managed by software through SSI instead of hardware
	SPI2->CR1 |= SPI_CR1_SSM;

	// SSI = 1 = internal NSS kept high in controller mode
	SPI2->CR1 |= SPI_CR1_SSI;

	// 16 bit frame
	SPI2->CR2 &= ~SPI_CR2_DS_Msk;
	SPI2->CR2 |= (0b1111 << SPI_CR2_DS_Pos);

	// -----------
	// re-enable SPI
	SPI2->CR1 |= SPI_CR1_SPE;
}
