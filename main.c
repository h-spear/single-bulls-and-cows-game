#define F_CPU 16000000UL
#include <avr/io.h>
#include "game.h"

void init() {
	DDRA = 0xff;	// LED(timer)
	DDRB = 0x30;	// 4-button switch(cmd)
	DDRC = 0xff;	// FND
	DDRD = 0xff;	// LCD
	DDRE = 0x0f;	// Switch
	DDRF = 0x0f;	// Keypad
	DDRG = 0x0f;	// FND Select
}

int main() {
	init();			// 초기화	
	game();			// 게임
}
