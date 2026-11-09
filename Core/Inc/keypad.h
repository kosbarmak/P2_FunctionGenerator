// keypad.h
#ifndef keypad_h
#define keypad_h

/* On the Keypad, from left to right: Row1, Row2, Row3, Row4, Col.1, Col.2, Col.3
*/

#define ROW_COUNT 4
#define COL_COUNT 3

#define PA_ROW_START 5
#define PC_COLUMN_START 10

#define INVALID_VALUE -1

int get_pressed_key();

#endif
