#define F_CPU 16000000UL
#include "Game.h"
#include "lcd.h"	// LCD 관련 라이브러리
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

// 최대 도전 횟수
#define		MAX_ATTEMPTS	30

// 게임 state를 표시(STOP:종료, GAME:게임 진행중)
#define		STOP	0
#define		GAME	1

// 게임 mode를 표시(10진수 모드, 16진수 모드)
#define		MODE_DEC	10
#define		MODE_HEX	16	
#define		MODE_NO		0

// FND에 출력할 세그먼트들 미리 배열로 선언
unsigned char FND[4];
unsigned char hex_seg[16] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x67, 0x77, 0x7c, 0x58, 0x5e, 0x79, 0x71};
unsigned char fnd_sel[4] = {0x01, 0x02, 0x04, 0x08};
unsigned char seg_INIT[4] = {0x30, 0x54, 0x30, 0x78};
unsigned char seg_WIN[4] = {0x00, 0x6a, 0x30, 0x54};
unsigned char seg_LOSE[4] = {0x38, 0x5c, 0x6d, 0x79};
unsigned char seg_OUT[4] = {0x00, 0x5c, 0x1c, 0x78};
unsigned char seg_EASY[4] = {0x79, 0x77, 0x6d, 0x6e};
unsigned char seg_HARD[4] = {0x76, 0x77, 0x50, 0x5e};
unsigned char seg_HI[4] = {0x00, 0x76, 0x30, 0x00};
unsigned char seg_BYE[4] = {0x00, 0x7f, 0x6e, 0x79};

char hex[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
char answer[4];		// 랜덤으로 생성된 4자리 숫자가 저장되는 배열
char guess[4];		// 사용자가 공격한 4자리 숫자가 저장되는 배열

volatile int guess_index = 0;
volatile int rand_time = 0;		// answer를 초기화하기 위한 time
volatile int cur_time = 0;
volatile int time_rate = 2000;	// 한 라운드당 시간 비율

volatile int state = STOP;		// 게임 state
volatile int mode = MODE_NO;	// 게임 mode

volatile int attempt = 0;		// 현재 시도 횟수
volatile int now_see = 0;		// 현재 보고 있는 히스토리 번호
char history[50][16];			// 게임 진행 히스토리를 저장하는 배열

// FND배열에 저장된 4글자 문자를 FND에 출력
void displayFND() {
	for(int i=0; i<4; i++) {
		PORTG = fnd_sel[3-i];
		PORTC = FND[i];
		_delay_us(200);
	} 
}

// FND 배열 초기화
void clearFND() {
	for(int i=0; i<4; i++) {
		FND[i] = 0x00;
		guess[i] = 0;
	} 
	guess_index = 0;
	displayFND();
}

// 인자로 전달한 4개의 세그먼트 문자를 디스플레이에 대략 2초간 출력
// 인자로 크기 4의 배열을 전달함
void displayFND_2s(unsigned char array[]) {

	// FND 배열 값 인자로 받은 배열로 초기화
	for(int i=0;i<4;i++)
		FND[i] = array[i];
		
	// 약 2초간 FND에 출력
	int cnt = 0;
	while(1)
	{
		for(int i=0; i<4; i++) {
			PORTG = fnd_sel[3-i];
			PORTC = FND[i];
			_delay_us(200);
		} 
		cnt++;

		if (cnt==3000){
			break;
		}
	}
	clearFND();
}

// history 배열 초기화
void clearHistory() {
	for(int i=0;i<50;i++) {
		for(int j=0;j<16;j++)
			history[i][j] = '\0';
	}
}

// LCD에 현재까지의 게임 기록을 보여준다.
void showHistory() {
	// 현재 보고 있는 페이지(now_see)를 기준으로 2개의 히스토리 출력
	LCD_showMsg(history[now_see], history[now_see+1]);
}

// 현재 guess의 값과 공격 결과를 history에 저장
void recordHistory(int strike_cnt, int ball_cnt) {
	history[attempt][0] = (char)((attempt+1)/10 + 48);
	history[attempt][1] = (char)((attempt+1)%10 + 48);
	history[attempt][2] = ')';
	history[attempt][3] = ' ';
	history[attempt][4] = guess[0];
	history[attempt][5] = guess[1];
	history[attempt][6] = guess[2];
	history[attempt][7] = guess[3];
	history[attempt][8] = ' ';
	history[attempt][9] = ' ';
	history[attempt][14] = ' ';
	history[attempt][15] = '\0';

	// OUT이라면 'OUT'으로 저장
	if(strike_cnt == 0 && ball_cnt == 0) {
		history[attempt][10] = 'O';
		history[attempt][11] = 'U';
		history[attempt][12] = 'T';
		history[attempt][13] = ' ';
		
	// 그 이외의 경우에는 '_S_B' 형태로 저장
	} else {
		history[attempt][10] = (char)(strike_cnt + 48);
		history[attempt][11] = 'S';
		history[attempt][12] = (char)(ball_cnt + 48);
		history[attempt][13] = 'B';
	}
	attempt++;	// 시도 횟수 증가
	_delay_ms(20);
}

// 게임 LOSE, 기회 모두 소모 시 나오는 메시지
void Msg_YouDied() {
	LCD_showMsg("    Y        ", "");
	_delay_ms(300);
	LCD_showMsg("    Y    I   ", "");
	_delay_ms(300);
	LCD_showMsg("    Y U  I   ", "");
	_delay_ms(300);
	LCD_showMsg("    Y U  I D ", "");
	_delay_ms(300);
	LCD_showMsg("    YOU  I D ", "");
	_delay_ms(300);
	LCD_showMsg("    YOU  IED ", "");
	_delay_ms(300);
	LCD_showMsg("    YOU DIED ", "");
	_delay_ms(300);
	LCD_showMsg("    YOU DIED.", "");
	_delay_ms(800);
}

// 게임 CLEAR, 게임 성공 시 나오는 메시지
void Msg_CONGRATULATIONS() {
	LCD_showMsg("C","");
	_delay_ms(100);
	LCD_showMsg("CO ","");
	_delay_ms(100);
	LCD_showMsg("CON ","");
	_delay_ms(100);
	LCD_showMsg("CONG","");
	_delay_ms(100);
	LCD_showMsg("CONGR ","");
	_delay_ms(100);
	LCD_showMsg("CONGRA","");
	_delay_ms(100);
	LCD_showMsg("CONGRAT ","");
	_delay_ms(100);
	LCD_showMsg("CONGRATU ","");
	_delay_ms(100);
	LCD_showMsg("CONGRATUL","");
	_delay_ms(100);
	LCD_showMsg("CONGRATULA","");
	_delay_ms(100);
	LCD_showMsg("CONGRATULAT","");
	_delay_ms(100);
	LCD_showMsg("CONGRATULATI","");
	_delay_ms(100);
	LCD_showMsg("CONGRATULATIO","");
	_delay_ms(100);
	LCD_showMsg("CONGRATULATION","");
	_delay_ms(100);
	LCD_showMsg("CONGRATULATIONS","");
	_delay_ms(100);
	LCD_showMsg("CONGRATULATIONS!","");
	_delay_ms(200);
	LCD_showMsg("CONGRATULATIONS!","  GAME CLEAR!!");
	_delay_ms(200);
	LCD_showMsg("CONGRATULATIONS!","");
	_delay_ms(200);
	LCD_showMsg("CONGRATULATIONS!","  GAME CLEAR!!");
	_delay_ms(200);
	LCD_showMsg("CONGRATULATIONS!","");
	_delay_ms(200);
	LCD_showMsg("CONGRATULATIONS!","  GAME CLEAR!!");
	_delay_ms(400);
}

// 시간 증가
// 루프 한번 돌때마다 time변수 1 증가
// time.h가 없어서 사용
void timeUp() {
	cur_time++;
	rand_time++;
	if(cur_time >= 20000)
		cur_time = 30;
	if(rand_time >= 20000)
		rand_time = 30;
}

// 제한 시간 초기화(LED불 끄기)
void timeClear() {
	cur_time = 0;
	PORTA = 0x00;
}

// 시간 체크
// 시간이 지남에 따라 LED등 하나씩 카운트
void timeCheck() {
	if(cur_time == time_rate)
		PORTA = 0x01;
	else if(cur_time == time_rate*2)
		PORTA = 0x03;
	else if(cur_time == time_rate*3)
		PORTA = 0x07;
	else if(cur_time == time_rate*4)
		PORTA = 0x0f;
	else if(cur_time == time_rate*5)
		PORTA = 0x1f;
	else if(cur_time == time_rate*6)
		PORTA = 0x3f;
	else if(cur_time == time_rate*7)
		PORTA = 0x7f;
	else if(cur_time == time_rate*8)
		PORTA = 0xff;
	
	// LED 8개가 모두 차면 시간 초과로 게임 종료(LOSE)
	else if(cur_time == time_rate*8 + 127) {
		clearFND();
		Msg_YouDied();
		LCD_showMsg("PRESS ANY SWITCH", "  TO CONTINUE.");
		displayFND_2s(seg_LOSE);
		stopGame();
		_delay_ms(20);
	}
}

// 4x4 키패드에서 숫자를 입력받으면 해당 index로 변환
int pushHex() {
	int index = 99;
	PORTF = 0xFF;       
	
	// 키패드의 첫번째 row 검사
	PORTF = 0xFE;
	_delay_us(10);
	if ((PINF & 0x10) == 0) index = 4;
	else if ((PINF & 0x20) == 0) index = 3;
	else if ((PINF & 0x40) == 0) index = 2;
	else if ((PINF & 0x80) == 0) index = 1;

	// 키패드의 두번째 row 검사
	PORTF = 0xFD;
	_delay_us(10);
	if ((PINF & 0x10) == 0) index = 8;
	else if ((PINF & 0x20) == 0) index = 7;
	else if ((PINF & 0x40) == 0) index = 6;
	else if ((PINF & 0x80) == 0) index = 5;

	// 키패드의 세번째 row 검사
	PORTF = 0xFB;
	_delay_us(10);
	if ((PINF & 0x10) == 0) index = 11;
	else if ((PINF & 0x20) == 0) index = 10;
	else if ((PINF & 0x40) == 0) index = 0;
	else if ((PINF & 0x80) == 0) index = 9;

	// 키패드의 네번째 row 검사    
	PORTF = 0xF7;
	_delay_us(10);
	if ((PINF & 0x10) == 0) index = 15;
	else if ((PINF & 0x20) == 0) index = 14;
	else if ((PINF & 0x40) == 0) index = 13;
	else if ((PINF & 0x80) == 0) index = 12;
	
	// 입력받지 않는 상황에서는 index 99를 리턴
	// 99는 버튼이 눌리지 않은 것을 의미
	if (guess_index == 4)
		index = 99;
	if (state == STOP)
		index = 99;
	if (mode == MODE_DEC && index > 9)
		index = 99;

	return index;
}

// 4-button 스위치에서 입력받은 것을 cmd로 리턴
int pushCmd() {
	PORTB = 0x0f;    
	int cmd = 0;
	
	// cmd 명령어들
	// 0: No Action
	// 1: 게임 중단, 2: 스크롤 업, 3: 스크롤 다운, 4: 제출

	if ((PINB & 0x01) == 0)	// red button(게임 중단)
		cmd = 1;
	else if ((PINB & 0x02) == 0)	// yellow1(LCD 스크롤 업)
		cmd = 2;
	else if ((PINB & 0x04) == 0)	// yellow2(LCD 스크롤 다운)
		cmd = 3;
	else if ((PINB & 0x08) == 0)	// green button(공격 숫자 제출)
		cmd = 4;

	// 만약 모든 기회를 다 사용했다면 게임 중단 명령어 리턴
	if(attempt == MAX_ATTEMPTS) {
		Msg_YouDied();	// 사망 메시지
		cmd = 1;		// 중단 명령어
	}
	return cmd;
}

// 사용자가 입력한 숫자를 guess 배열에 저장
void enterGuess(int key) {
	if(key == 99)	// 입력된 key가 없으면 수행 X
		return;
	
	// 중복되는 문자가 없도록 flag를 사용하여 체크
	int flag = 0;
	for (int i=0; i<4; i++) {
		if(guess[i] == hex[key])
			flag = 1;
	}

	// 중복이 없다면 guess 변수에 추가
	if (flag == 0) {
		FND[guess_index] = hex_seg[key];
		guess[guess_index] = hex[key];
		guess_index++;
	}
}

// 입력받은 cmd값에 따라 액션 수행
void cmdSwitch(int cmd) {
	switch(cmd)
	{
		case 0:
			break;
		case 1:
			LCD_showMsg("PRESS ANY SWITCH", "  TO CONTINUE.");
			if (state == GAME)
				displayFND_2s(seg_BYE);
			stopGame();
			_delay_ms(20);
			break;
		case 2:
			if(now_see > 0) {
				now_see--;
				showHistory();
			}
			_delay_ms(20);
			break;
		case 3:
			if(now_see < attempt - 2) {
				now_see++;
				showHistory();
			}
			_delay_ms(20);
			break;
		case 4:
			compare();
			_delay_ms(20);
			break;
		default:
			// Error
			break;
	}
}

// 사용자가 switch로 모드를 선택했는지 체크
void checkModeSwitch() {
	// 위의 switch를 선택하면 EASY 모드(10진수 모드) 실행
	if ((PINE & 0x10) == 0) {
		mode = MODE_DEC;	// 10진수 모드 선택
		LCD_showMsg("   EASY  MODE","     START!");
		_delay_ms(2000);
		LCD_showMsg("  DECIMAL MODE","YOU CAN USE 0TO9");
		_delay_ms(20);
	}
	
	// 위의 switch를 선택하면 HARD 모드(16진수 모드) 실행
	if ((PINE & 0x20) == 0) {
		mode = MODE_HEX;	// 16진수 모드 선택
		LCD_showMsg("   HARD  MODE","     START!");
		_delay_ms(2000);
		LCD_showMsg("HEXADECIMAL MODE","YOU CAN USE 0TOF");
		_delay_ms(20);
	}
}

// green 버튼 클릭 시
// guess 배열의 값과 answer 배열의 값을 비교하여 결과 출력
void compare() {
	if(guess_index < 4)
		return;
	if(state == STOP)
		return;

	int ball_cnt = 0;		// 볼 갯수
	int strike_cnt = 0;		// 스트라이크 갯수
	timeClear();			// 시간 초기화

	for(int i=0; i<4; i++) {
		if(guess[i] == answer[i])
			strike_cnt++;

		for(int j=0; j<4; j++){
			if(guess[j] == answer[i])
				ball_cnt++;
		}
	}
	ball_cnt -= strike_cnt;
	
	recordHistory(strike_cnt, ball_cnt);	// history 기록
	now_see = attempt - 2;		// LCD에 최근 기록을 보여주도록 이동
	if(attempt == 1)
		now_see++;
	showHistory();
	clearFND();

	// 만약 0S0B 이라면 'OUT'을 FND에 출력
	if(strike_cnt == 0 && ball_cnt == 0) {
		displayFND_2s(seg_OUT);
		return;
	}
	
	// 4S 라면 FND에 'WIN'을 출력하고
	// 성공 메시지를 LCD에 출력한 후, 게임 종료
	if(strike_cnt == 4){
		displayFND_2s(seg_WIN);
		Msg_CONGRATULATIONS();
		stopGame();
		return;
	}
	
	// 그 이외의 경우라면 FND에 결과 출력
	unsigned char seg_array[4] = {hex_seg[strike_cnt], hex_seg[5],hex_seg[ball_cnt], hex_seg[8]};
	displayFND_2s(seg_array);
}

// 게임 중단
void stopGame() {
	mode = MODE_NO;		// 게임 모드 설정 X
	state = STOP;		// 게임 state를 중단 상태로 설정
	timeClear();		// 시간 초기화
	clearFND();			// FND 초기화
	now_see = 0;
	attempt = 0;		// 시도 횟수 초기화
	clearHistory();		// history 초기화
	LCD_showMsg(" SELECT MODE","      EASY/HARD");
}

// 게임 초기화
void initGame() {
	state = GAME;		// 게임 시작 상태로 설정
	int i=0;
	while(i<4) {		// 랜덤으로 answer 숫자 초기화
		answer[i] = hex[rand_time % mode];
		for(int j=0;j<i;j++) {
			if (answer[j] == answer[i]) {
				i--;
				break;
			}
		}
		i++;
		rand_time = (rand_time + 17);
		timeUp();
		_delay_us(20);
	}
	timeClear();

	// 선택한 모드를 FND에 출력해줌.
	if (mode == MODE_DEC)
		displayFND_2s(seg_EASY);
	if (mode == MODE_HEX)
		displayFND_2s(seg_HARD);
	_delay_ms(20);
}

// 게임 진행
void progress() {
	int key,cmd;

	initGame();	// 초기화

	// state가 게임이면 무한 루프
	// cmd로 red버튼 클릭 시 state가 STOP이 되어 루프 탈출
	while(state == GAME) {
		displayFND();
		timeCheck();

		key = pushHex();
		enterGuess(key);

		cmd = pushCmd();
		cmdSwitch(cmd);

		timeUp();
	}
}

// 게임 메인 함수
void game() {
	PORTB = 0x0f;
	// 게임 시작 메시지
	LCD_showMsg("    WELCOME!","  STOVE LEAGUE");
	displayFND_2s(seg_HI);

	LCD_showMsg(" SELECT MODE","      EASY/HARD");
	while(1) {
		checkModeSwitch();	// 모드 선택
		if (mode == MODE_NO)	// 모드 선택하지 않으면 무한 루프
			continue;
			
		progress();		// 모드가 선택되면 진행으로 넘어간다.
	}
}
