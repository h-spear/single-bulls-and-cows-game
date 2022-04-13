#ifndef LCD_H_
#define LCD_H_

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define sbi(x, y) (x |= (1 << y))
#define cbi(x, y) (x &= ~(1 << y))

#define LCD_CON      PORTE
#define LCD_DATA     PORTD
#define LCD_DATA_DIR DDRD
#define LCD_DATA_IN  PIND

#define LCD_RS   0
#define LCD_RW   1
#define LCD_E    2

unsigned char LCD_rCommand(void) {
	unsigned char temp=1;
	LCD_DATA_DIR = 0X00;
	
	cbi(LCD_CON, LCD_RS);
	sbi(LCD_CON, LCD_RW);
	sbi(LCD_CON, LCD_E); 
	_delay_us(1);
	
	temp = LCD_DATA_IN;
	_delay_us(1);
	
	cbi(LCD_CON, LCD_E);
	
	LCD_DATA_DIR = 0XFF;
	_delay_us(1);
	
	return temp;
}

char LCD_BusyCheck(unsigned char temp) {
	return temp & 0x80;
}

void LCD_wCommand(char cmd) {
	cbi(LCD_CON, LCD_RS);
	cbi(LCD_CON, LCD_RW);
	sbi(LCD_CON, LCD_E);
	LCD_DATA = cmd;     
	_delay_us(1);
	cbi(LCD_CON, LCD_E);
	_delay_us(1);
}

void LCD_wBCommand(char cmd) {
	while(LCD_BusyCheck(LCD_rCommand()))
	_delay_us(1);
	cbi(LCD_CON, LCD_RS);
	cbi(LCD_CON, LCD_RW);
	sbi(LCD_CON, LCD_E); 
	
	LCD_DATA = cmd;
	_delay_us(1);
	cbi(LCD_CON, LCD_E);
	_delay_us(1);
}

// LCD 첫번째 줄 선택
void LCD_Cursor1() {
	LCD_wBCommand(0x80);
}

// LCD 두번째 줄 선택
void LCD_Cursor2() {
	LCD_wBCommand(0xc0);
}

// LCD 초기화
void LCD_Init(void)
{
	_delay_ms(100);
	LCD_wCommand(0x38);
	_delay_ms(10);
	LCD_wCommand(0x38);
	_delay_us(200);
	LCD_wCommand(0x38);
	_delay_us(200);
	
	// Function Set
	LCD_wBCommand(0x38);
	_delay_us(200);

	// Display On/Off Control
	LCD_wBCommand(0x0c);
	_delay_us(200);

	// Clear Display
	LCD_wBCommand(0x01);
	_delay_us(200);
}

void LCD_wData(char dat)
{
	while(LCD_BusyCheck(LCD_rCommand())) {
		_delay_us(1);
	}
	sbi(LCD_CON, LCD_RS);
	cbi(LCD_CON, LCD_RW);
	sbi(LCD_CON, LCD_E);
	LCD_DATA = dat;
	_delay_us(1);
	cbi(LCD_CON, LCD_E);
	_delay_us(1);
}

void LCD_wString(char *str)
{
	while(*str)
	LCD_wData(*str++);
}

// LCD에 입력한 메시지 출력
void LCD_showMsg(char *str1, char *str2) {
	LCD_Init();
	LCD_Cursor1();
	LCD_wString(str1);
	LCD_Cursor2();
	LCD_wString(str2);
}

#endif /* LCD_H_ */
