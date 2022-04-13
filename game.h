#ifndef _GAME_H
#define _GAME_H

// FND배열에 저장된 4글자 문자를 FND에 출력
void displayFND();

// FND 배열 초기화
void clearFND();

// 인자로 전달한 4개의 세그먼트 문자를 디스플레이에 대략 2초간 출력
void displayFND_2s(unsigned char array[]);

// LCD에 현재까지의 게임 기록을 보여준다.
void showHistory();

// 현재 guess의 값과 공격 결과를 history에 저장
void recordHistory(int strike_cnt, int ball_cnt);

// 시간 증가
void timeUp();

// 제한 시간 초기화(LED불 끄기)
void timeClear();

// 시간 체크
void timeCheck();

// 4x4 키패드에서 숫자를 입력받으면 해당 index로 변환
int pushHex();

// 4-button 스위치에서 입력받은 것을 cmd로 리턴
int pushCmd();

// 사용자가 입력한 숫자를 guess 배열에 저장
void enterGuess(int key);

// 입력받은 cmd값에 따라 액션 수행
void cmdSwitch(int cmd);

// 사용자가 switch로 모드를 선택했는지 체크
void checkModeSwitch();

// guess 배열의 값과 answer 배열의 값을 비교하여 결과 출력
void compare();

// 게임 종료
void stopGame();

// 게임 초기화
void initGame();

// 게임 진행
void progress();

// 게임 메인 함수
void game();

#endif
