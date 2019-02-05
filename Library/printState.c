#include "ses_lcd.h"
#include "ses_uart.h"

void printState(const char* state)
{
	/* Initialization of LCD and UART */
	//uart_init(57600);
	//lcd_init();
	fprintf(uartout, "%s\n", state);
	lcd_clear();
	lcd_setCursor(0,1);
	fprintf(lcdout,"%s\n", state);


}
