#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

typedef struct RoomInfoSocket // 방 정보를 저장하는 구조체
{
	int roomNumber; // 방 번호
	char player1Nick[32]; // 1번 플레이어 닉네임 (방 이름이기도 하다)
	char player2Nick[32]; // 2번 플레이어 닉네임
	bool player1Ready; // 1번 플레이어 레뒤했나요
	bool player2Ready; // 2번 플레이어 레뒤했나요
	int roomCount; // 방에 들어간 총 플레이어 수
}RoomInfoSocket;

typedef struct InGameInfoSocket // 플레이 정보를 저장하는 구조체
{
	int omok_plate[16][16]; // 바둑판 상태
	int turn; // 한경기 내에서 총 둔 횟수
	int player1Win; // 플레이어1 이 몇번 이겼는지
	int player2Win; // 플레이어2 이 몇번 이겼는지
}InGameInfoSocket;

typedef struct MyMoveInfoSocket // 플레이 정보를 저장하는 구조체
{
	int myMoveX;
	int myMoveY;
}MyMoveInfoSocket;

// 서버에 연결
WSADATA wsaData;
SOCKET hSocket;
SOCKADDR_IN servAddr;

char *omok_plate[16][16]; // 바둑판
int change_X = 7, change_Y = 7;
int pointer_X = 14, pointer_Y = 7;
char playerNick[32];
int mySlotNumber; // 슬롯 카운터
RoomInfoSocket roomInfo[10]; // 모든 방 외부 정보
InGameInfoSocket inGameInfo; // 방 내부 정보
MyMoveInfoSocket myMoveInfo; // 내가 둔 위치 정보

void GomokuStart(int i);
void UpdateGomoku(); // 오목 정보 업데이트 스레드 함수
void gotoXY(int x, int y);
void RL_Check(const char *);
void UD_Check(const char *);
void Left_diagonal_Check(const char *);
void Right_diagonal_Check(const char *);
void clear();

// 통신 관련
void ErrorHandling(char* message);

int main(void)
{
	int menu;

	while (1)
	{
		system("cls");
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN);
		puts("");
		puts("        ##### # #         #                                          ");
		puts("        # #   ###  #####  #          ####### #     # ####### #    # ");
		puts("        ##### # #     #   ###        #     # ##   ## #     # #   # ");
		puts("            #####    #    #          #     # # # # # #     # #  # ");
		puts("                #   #     #    ##### #     # #  #  # #     # ### ");
		puts("            #####     #####          #     # #     # #     # #  # ");
		puts("            #         #   #          #     # #     # #     # #   # ");
		puts("            #####     #####          ####### #     # ####### #    # ");
		puts("                                                                     ");

		gotoXY(33, 10);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_BLUE | BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
		puts("1.게임시작");
		gotoXY(33, 11);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
		puts("2.도움말");
		gotoXY(33, 12);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
		puts("3.종료");
		gotoXY(33, 14);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
		printf("선택[   ]\b\b\b");

		scanf_s("%d", &menu);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
		if (menu == 1) // 1. 게임시작 메뉴 선택
		{
			system("cls"); // 화면정리

						   // 닉네임 입력 화면 표시
			system("cls"); // 화면정리
			printf("닉네임을 입력하세요 : ");
			scanf_s("%s", playerNick, sizeof(playerNick));

			char recvMessage[1024];
			char sendMessage[1024];
			int strLen;
			char* serverAdress[2]; // 아이피와 포트 저장용

			 // 서버정보 저장
			serverAdress[0] = "14.35.252.217";
			serverAdress[1] = "9999";
			printf("서버에 연결중... : <IP> %s <Port> %s \n", serverAdress[0], serverAdress[1]);

			// 서버 연결
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			{
				ErrorHandling("WSAStartup() error!");
			}

			hSocket = socket(PF_INET, SOCK_STREAM, 0);
			if (hSocket == INVALID_SOCKET)
			{
				ErrorHandling("socket() error");
			}

			memset(&servAddr, 0, sizeof(servAddr));
			servAddr.sin_family = AF_INET;
			servAddr.sin_addr.s_addr = inet_addr(serverAdress[0]);
			servAddr.sin_port = htons(atoi(serverAdress[1]));

			if (connect(hSocket, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
			{
				ErrorHandling("connect() error!");
			}

			strLen = recv(hSocket, recvMessage, sizeof(recvMessage) - 1, 0); // 연결 성공했으면 서버로부터 메세지 받음
			if (strLen == -1)
			{
				ErrorHandling("read() error!");
			}

			printf("Message from server: %s \n", recvMessage); // 연결 확인 메세지 받음

			send(hSocket, playerNick, sizeof(playerNick), 0); // 내 닉네임 보냄

			// 방들의 정보를 다 받음
			for (int i = 0; i < 10; i++)
			{
				recv(hSocket, (char*)&roomInfo[i], sizeof(RoomInfoSocket), 0);
				printf("번호 : %d 방이름 : %s 인원수 : %d\n", roomInfo[i].roomNumber, roomInfo[i].player1Nick, roomInfo[i].roomCount);

				Sleep(20);
			}

			// 방 선택하는 메세지 출력
			char choose[2]; // 몇번 방 선택했는지
			bool firstTimeIntoRoom = true; // 처음으로 방에 입장하는 경우 true 
			bool thisPlayerReady = false; // 내가 레디 했는지 여부
			fflush(stdin);
			printf("몇번 방을 선택하시겠습니까?");
			scanf_s("%s", choose, 2);

			// 선택한 방 번호 서버로 보냄
			send(hSocket, choose, sizeof(choose), 0);

			// 선택한 방 번호 저장
			int i = atoi(choose);

			// 서버에게 지금 들어간 방의 정보만 다시 받음
			recv(hSocket, (char*)&roomInfo[i], sizeof(RoomInfoSocket), 0);

			// 화면 지우기
			system("cls");

			while (1)
			{
				// 처음 방에 들어갔을때만 내가 몇번째 슬롯에 들어갔나 기억
				if (firstTimeIntoRoom)
				{
					mySlotNumber = roomInfo[i].roomCount;
				}

				char playerReady[2] = "F";

				if (GetKeyState(0x52) & 0x8001) // 사용자가 R키 눌렀나 확인
				{
					strcpy(playerReady, "T");
				}

				// 내가 레뒤했다고 서버에게 보낸다
				send(hSocket, playerReady, sizeof(playerReady), 0);

				// 방 정보를 다시 받는다
				recv(hSocket, (char*)&roomInfo[i], sizeof(RoomInfoSocket), 0);

				// 처음 입장시 표시할 내용
				if (firstTimeIntoRoom)
				{
					gotoXY(20, 10);
					printf("%d 번 방 [ %s ] - 인원 수 : %d \n", roomInfo[i].roomNumber, roomInfo[i].player1Nick, roomInfo[i].roomCount);
					gotoXY(20, 11);
					printf("( R 키를 눌러 Ready 하세요 ) \n");
					gotoXY(20, 12);
					printf("Player 1 : %s \n", roomInfo[i].player1Nick);
					gotoXY(51, 12);
					printf("<%s> \n", (roomInfo[i].player1Ready == true) ? "  Ready  " : "Not ready");
					gotoXY(20, 13);
					printf("Player 2 : %s \n", roomInfo[i].player2Nick);
					gotoXY(51, 13);
					printf("<%s> \n", (roomInfo[i].player2Ready == true) ? "  Ready  " : "Not ready");
					firstTimeIntoRoom = false; // 이제 슬롯 카운터를 더이상 갱신하지 않는다
				}

				// 이후에 방 안에 들어간 상태의 정보를 계속 갱신 (깜빡임 방지)
				gotoXY(20, 12);
				printf("Player 1 : %s                  \n", roomInfo[i].player1Nick);
				gotoXY(50, 12);
				printf("<%s> \n", (roomInfo[i].player1Ready == true) ? "  Ready  " : "Not ready");
				gotoXY(20, 13);
				printf("Player 2 : %s                  \n", roomInfo[i].player2Nick);
				gotoXY(50, 13);
				printf("<%s> \n", (roomInfo[i].player2Ready == true) ? "  Ready  " : "Not ready");


				// 모두 레뒤 했나 정보 받음
				if ((roomInfo[i].player1Ready == true) && (roomInfo[i].player2Ready == true)) // 다 레뒤했으면
				{
					Sleep(1000); // 경기 전에 살짝 멈춰서 긴장감을 준다
					break; // 게임 하러 나가자
				}

				Sleep(100);
			}

			//오목 대결 시작
			GomokuStart(i);

			// 서버 연결 종로
			WSACleanup();
			closesocket(hSocket);
		}
		else if (menu == 2) // 2. 도움말 메뉴 선택
		{
			system("cls");

			gotoXY(33, 1);
			puts("[도움말]");
			puts("");
			puts("땔감 오목은 고모쿠 룰을 준수합니다.");
			puts("가로, 세로, 대각선 상관없이 먼저 5개의 돌이 연속으로 나오도록 하는 사람이 이깁니다.");
			puts("3-3목도 허용합니다.");
			puts("6목은 금지수는 아니나, 착수해도 승리로 인정되지 않습니다.");
			puts("흑이 절대적으로 유리하기 때문에 일정 수준 이상의 실력자들이 일반룰로 대결하면 백의 필패.");
			puts("게임은 총 2판이 진행되며, 승리 조건은 2판 중 2승을 전부 하셔야 합니다.");
			puts("");

			system("pause");
		}
		else // 다른거 입력하면 종료
		{
			return 0;
		}
	}

	return 0;
}


void GomokuStart(int i)
{
	int turnBefore = 0; // 이전 턴 상태 저장 (바둑판 갱신 내용 있는지 확인용)
	bool screenNeedToChange = true; // 바둑판 내용이 바뀌었는지 여부 저장

	// 처음에 움직이지 않았을때를 구분하기 위해 이동정보 99로 초기화
	myMoveInfo.myMoveX = 99;
	myMoveInfo.myMoveY = 99;
	
	// 업데이트 정보를 받는 스레드 열기
	std::thread updateGomokuThread(UpdateGomoku);

	// 바둑판 그리기 시작
	system("cls");
	while (true)
	{
		// 바둑판이 바뀐 경우에 다시 그리기
		if (screenNeedToChange == true)
		{
			// 바둑판 그리기
			gotoXY(0, 0);
			for (int i = 0; i < 16; i++)
			{
				for (int j = 0; j < 16; j++)
				{
					if (inGameInfo.omok_plate[i][j] == 1)
					{
						omok_plate[i][j] = "●";
					}
					else if (inGameInfo.omok_plate[i][j] == 2)
					{
						omok_plate[i][j] = "○";
					}
					else
					{
						omok_plate[i][j] = "┼";
					}

					printf("%s", omok_plate[i][j]);
				}
				puts("");
			}

			// 게임 상태 표시하기
			if (inGameInfo.turn % 2 == 0)
			{
				gotoXY(33, 2); printf(">>");
				gotoXY(33, 3); printf("  ");
			}
			else
			{
				gotoXY(33, 2); printf("  ");
				gotoXY(33, 3); printf(">>");
			}
			(mySlotNumber == 1) ? gotoXY(35, 2) : gotoXY(35, 3);
			printf("나:");
			gotoXY(38, 2);
			printf("%s (%d승)", roomInfo[i].player1Nick, inGameInfo.player1Win);
			gotoXY(38, 3);
			printf("%s (%d승)", roomInfo[i].player2Nick, inGameInfo.player2Win);

			screenNeedToChange = false;
		}

		// 커서 이동
		gotoXY(pointer_X, pointer_Y);

		// 키 입력받기
		if (GetAsyncKeyState(VK_UP) & 0x0001 && change_Y > 0)
		{
			change_Y = change_Y - 1;
			pointer_Y = pointer_Y - 1;
			screenNeedToChange = true;
		}

		if (GetAsyncKeyState(VK_DOWN) & 0x0001 && change_Y < 15)
		{
			change_Y = change_Y + 1;
			pointer_Y = pointer_Y + 1;
			screenNeedToChange = true;
		}

		if (GetAsyncKeyState(VK_RIGHT) & 0x0001 && change_X < 15)
		{
			change_X = change_X + 1;
			pointer_X = pointer_X + 2;
			screenNeedToChange = true;
		}

		if (GetAsyncKeyState(VK_LEFT) & 0x0001 && change_X > 0)
		{
			change_X = change_X - 1;
			pointer_X = pointer_X - 2;
			screenNeedToChange = true;
		}

		if (GetAsyncKeyState(VK_RETURN) & 0x0001)
		{
			// 내가 둘 차례인 경우 둔 정보 갱신
			if (mySlotNumber == 1 && (inGameInfo.turn % 2 == 0)) // 흑돌일때
			{
				myMoveInfo.myMoveX = change_X;
				myMoveInfo.myMoveY = change_Y;
			}
			else if (mySlotNumber == 2 && (inGameInfo.turn % 2 == 1)) // 백돌일때
			{
				myMoveInfo.myMoveX = change_X;
				myMoveInfo.myMoveY = change_Y;
			}
			screenNeedToChange = true;
		}

		// 승리 판단하기 (흑)
		/*RL_Check("○");
		UD_Check("○");
		Left_diagonal_Check("○");
		Right_diagonal_Check("○");*/

		// 승리 판단하기 (백)
		/*RL_Check("●");
		UD_Check("●");
		Left_diagonal_Check("●");
		Right_diagonal_Check("●");*/
	}

	// 잠시 대기
	Sleep(100);
}


void UpdateGomoku()
{
	while (true)
	{
		// 서버로 부터 인게임 상태 정보 받아오기
		recv(hSocket, (char*)&inGameInfo, sizeof(InGameInfoSocket), 0);

		// 내가 둔 정보 보내기
		send(hSocket, (char*)&myMoveInfo, sizeof(MyMoveInfoSocket), 0);

		// 잠시 대기
		Sleep(100);
	}

	// 수신 버퍼 남은거 확인하고 다 읽어서 비우기
	u_long tmp_long = 0;
	if (ioctlsocket(hSocket, FIONREAD, &tmp_long) != SOCKET_ERROR)
	{
		for (int j = 0; j < tmp_long; ++j)
		{
			recv(hSocket, (char*)&inGameInfo, sizeof(InGameInfoSocket), 0);
		}
	}
}


void gotoXY(int x, int y)
{
	COORD Cursor;
	Cursor.X = x;
	Cursor.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cursor);
}


void RL_Check(const char *ch)
{
	int check_ACount = 0;

	for (int i = 0; i <= 4; i++) // 우로탐색 0 1 2 3 4
	{

		if (omok_plate[change_Y][change_X + i] == ch) // 내돌 계속 나오면
		{
			++check_ACount; // 카운트를 추가한다
		}
		else // 그러나 중간에 내 돌이 아니고 다른팀 돌이나 비어있는 바둑판만 나오면
		{
			break; // 포문 나간다
		}
	}
	for (int i = -1; i >= -5; i--) // 좌로탐색 -5 -4 -3 -2 -1
	{
		if (omok_plate[change_Y][change_X + i] == ch) // 내돌 계속 나오면
		{
			++check_ACount; // 카운트를 추가한다
		}
		else // 그러나 중간에 내 돌이 아니고 다른팀 돌이나 비어있는 바둑판만 나오면
		{
			break; // 포문 나간다
		}
	}

	if (check_ACount == 5 && ch == "○")
	{
		gotoXY(20, 20);
		printf("흑승리!!!");
		return;
	}
	else if (check_ACount == 5 && ch == "●")
	{
		gotoXY(20, 20);
		printf("백승리!!!");
		return;
	}
}


void UD_Check(const char *ch)
{
	int check_ACount = 0;

	for (int i = 0; i <= 4; i++) // 위로탐색 0 1 2 3 4
	{
		if (omok_plate[change_Y + i][change_X] == ch) // 내돌 계속 나오면
		{
			++check_ACount; // 카운트를 추가한다
		}
		else // 그러나 중간에 내 돌이 아니고 다른팀 돌이나 비어있는 바둑판만 나오면
		{
			break; // 포문 나간다
		}
	}
	for (int i = -1; i >= -5; i--) // 아래로탐색 -5 -4 -3 -2 -1
	{
		if (omok_plate[change_Y + i][change_X] == ch) // 내돌 계속 나오면
		{
			++check_ACount; // 카운트를 추가한다
		}
		else // 그러나 중간에 내 돌이 아니고 다른팀 돌이나 비어있는 바둑판만 나오면
		{
			break; // 포문 나간다
		}
	}

	if (check_ACount == 5 && ch == "○")
	{
		gotoXY(20, 20);
		printf("흑승리!!!");
		return;
	}
	else if (check_ACount == 5 && ch == "●")
	{
		gotoXY(20, 20);
		printf("백승리!!!");
		return;
	}
}


void Left_diagonal_Check(const char *ch)
{
	int check_ACount = 0;
	for (int i = -1; i >= -5; i--) // 왼쪽위 대각선탐색
	{
		if (omok_plate[change_Y + i][change_X + i] == ch) // 내돌 계속 나오면
		{
			++check_ACount; // 카운트를 추가한다
		}
		else // 그러나 중간에 내 돌이 아니고 다른팀 돌이나 비어있는 바둑판만 나오면
		{
			break; // 포문 나간다
		}
	}

	for (int i = 0; i <= 4; i++) // 왼쪽위 대각선탐색
	{
		if (omok_plate[change_Y + i][change_X + i] == ch) // 내돌 계속 나오면
		{
			++check_ACount; // 카운트를 추가한다
		}
		else // 그러나 중간에 내 돌이 아니고 다른팀 돌이나 비어있는 바둑판만 나오면
		{
			break; // 포문 나간다
		}
	}
	if (check_ACount == 5 && ch == "○")
	{
		gotoXY(20, 20);
		printf("흑승리!!!");
		return;
	}
	else if (check_ACount == 5 && ch == "●")
	{
		gotoXY(20, 20);
		printf("백승리!!!");
		return;
	}
}


void Right_diagonal_Check(const char *ch)
{
	int check_ACount = 0;
	for (int i = -1; i >= -5; i--) // 오른쪽위 대각선탐색
	{
		if (omok_plate[change_Y + i][change_X - i] == ch) // 내돌 계속 나오면
		{
			++check_ACount; // 카운트를 추가한다
		}
		else // 그러나 중간에 내 돌이 아니고 다른팀 돌이나 비어있는 바둑판만 나오면
		{
			break; // 포문 나간다
		}
	}

	for (int i = 0; i <= 4; i++) // 오른쪽위 대각선탐색
	{
		if (omok_plate[change_Y + i][change_X - i] == ch) // 내돌 계속 나오면
		{
			++check_ACount; // 카운트를 추가한다
		}
		else // 그러나 중간에 내 돌이 아니고 다른팀 돌이나 비어있는 바둑판만 나오면
		{
			break; // 포문 나간다
		}
	}
	if (check_ACount == 5 && ch == "○")
	{
		gotoXY(20, 20);
		printf("흑승리!!!");
		return;
	}
	else if (check_ACount == 5 && ch == "●")
	{
		gotoXY(20, 20);
		printf("백승리!!!");
		return;
	}
}


void clear()
{
	if (GetKeyState(0x43) & 0x8001)
	{
		system("cls");
		for (int i = 0; i < 16; i++)
		{
			for (int j = 0; j < 16; j++)
			{
				omok_plate[i][j] = "┼";
			}
			puts("");
		}
	}
}


void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	system("pause");
	exit(1);
}