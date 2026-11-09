#include "keypad.h"
#include "stdbool.h"
#include "stm32l476xx.h"
#include <stdint.h>

/* On the Keypad, from left to right: Row1, Row2, Row3, Row4, Col.1, Col.2, Col.3
*/

#define ROW_COUNT 4
#define COL_COUNT 3
#define PA_ROW_START 5
#define PC_COLUMN_START 10
#define PA_LED_START 5 

bool row_is_high(uint8_t i)
{
	return GPIOA->IDR & (1 << (PA_ROW_START + i));
}

void col_set(uint8_t j, bool high)
{
	GPIOC->ODR &= ~(1 << (PC_COLUMN_START + j));
	if (high)
		GPIOC->ODR |= (1 << (PC_COLUMN_START + j));
}

void cols_set_low()
{
	for (uint8_t j = 0; j < COL_COUNT; j++) {
		col_set(j, false);
	}
}

void col_isolate(uint8_t j)
{
	cols_set_low();
	col_set(j, true);
}

bool key_is_pressed(uint8_t row, uint8_t col)
{
	col_isolate(col);
	return row_is_high(row);
}

int get_key_value(uint8_t row, uint8_t col)
{
	// treat all rows but the last one as a regularly ordered numerical grid
	if (row < ROW_COUNT - 1)
		return (row * COL_COUNT) + col + 1;

	// treat the last row with special hardcoded values
	if (col == 0)
		return 10;

	if (col == 1)
		return 0;

	if (col == 2)
		return 15;

	return INVALID_VALUE;
}

int get_pressed_key()
{
	for (uint8_t i = 0; i < ROW_COUNT; i++)
		for (uint8_t j = 0; j < COL_COUNT; j++)
			if (key_is_pressed(i, j))
				return get_key_value(i, j);

	return INVALID_VALUE;
}
