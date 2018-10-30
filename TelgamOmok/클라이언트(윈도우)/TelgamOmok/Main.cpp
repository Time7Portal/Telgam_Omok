#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

typedef struct RoomInfoSocket // �� ������ �����ϴ� ����ü
{
	int roomNumber; // �� ��ȣ
	char player1Nick[32]; // 1�� �÷��̾� �г��� (�� �̸��̱⵵ �ϴ�)
	char player2Nick[32]; // 2�� �÷��̾� �г���
	bool player1Ready; // 1�� �÷��̾� �����߳���
	bool player2Ready; // 2�� �÷��̾� �����߳���
	int roomCount; // �濡 �� �� �÷��̾� ��
}RoomInfoSocket;

typedef struct InGameInfoSocket // �÷��� ������ �����ϴ� ����ü
{
	int omok_plate[16][16]; // �ٵ��� ����
	int turn; // �Ѱ�� ������ �� �� Ƚ��
	int player1Win; // �÷��̾�1 �� ��� �̰����
	int player2Win; // �÷��̾�2 �� ��� �̰����
}InGameInfoSocket;

typedef struct MyMoveInfoSocket // �÷��� ������ �����ϴ� ����ü
{
	int myMoveX;
	int myMoveY;
}MyMoveInfoSocket;

// ������ ����
WSADATA wsaData;
SOCKET hSocket;
SOCKADDR_IN servAddr;

char *omok_plate[16][16]; // �ٵ���
int change_X = 7, change_Y = 7;
int pointer_X = 14, pointer_Y = 7;
char playerNick[32];
int mySlotNumber; // ���� ī����
RoomInfoSocket roomInfo[10]; // ��� �� �ܺ� ����
InGameInfoSocket inGameInfo; // �� ���� ����
MyMoveInfoSocket myMoveInfo; // ���� �� ��ġ ����

void GomokuStart(int i);
void UpdateGomoku(); // ���� ���� ������Ʈ ������ �Լ�
void gotoXY(int x, int y);
void RL_Check(const char *);
void UD_Check(const char *);
void Left_diagonal_Check(const char *);
void Right_diagonal_Check(const char *);
void clear();

// ��� ����
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
		puts("1.���ӽ���");
		gotoXY(33, 11);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
		puts("2.����");
		gotoXY(33, 12);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
		puts("3.����");
		gotoXY(33, 14);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
		printf("����[   ]\b\b\b");

		scanf_s("%d", &menu);
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
		if (menu == 1) // 1. ���ӽ��� �޴� ����
		{
			system("cls"); // ȭ������

						   // �г��� �Է� ȭ�� ǥ��
			system("cls"); // ȭ������
			printf("�г����� �Է��ϼ��� : ");
			scanf_s("%s", playerNick, sizeof(playerNick));

			char recvMessage[1024];
			char sendMessage[1024];
			int strLen;
			char* serverAdress[2]; // �����ǿ� ��Ʈ �����

			 // �������� ����
			serverAdress[0] = "14.35.252.217";
			serverAdress[1] = "9999";
			printf("������ ������... : <IP> %s <Port> %s \n", serverAdress[0], serverAdress[1]);

			// ���� ����
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

			strLen = recv(hSocket, recvMessage, sizeof(recvMessage) - 1, 0); // ���� ���������� �����κ��� �޼��� ����
			if (strLen == -1)
			{
				ErrorHandling("read() error!");
			}

			printf("Message from server: %s \n", recvMessage); // ���� Ȯ�� �޼��� ����

			send(hSocket, playerNick, sizeof(playerNick), 0); // �� �г��� ����

			// ����� ������ �� ����
			for (int i = 0; i < 10; i++)
			{
				recv(hSocket, (char*)&roomInfo[i], sizeof(RoomInfoSocket), 0);
				printf("��ȣ : %d ���̸� : %s �ο��� : %d\n", roomInfo[i].roomNumber, roomInfo[i].player1Nick, roomInfo[i].roomCount);

				Sleep(20);
			}

			// �� �����ϴ� �޼��� ���
			char choose[2]; // ��� �� �����ߴ���
			bool firstTimeIntoRoom = true; // ó������ �濡 �����ϴ� ��� true 
			bool thisPlayerReady = false; // ���� ���� �ߴ��� ����
			fflush(stdin);
			printf("��� ���� �����Ͻðڽ��ϱ�?");
			scanf_s("%s", choose, 2);

			// ������ �� ��ȣ ������ ����
			send(hSocket, choose, sizeof(choose), 0);

			// ������ �� ��ȣ ����
			int i = atoi(choose);

			// �������� ���� �� ���� ������ �ٽ� ����
			recv(hSocket, (char*)&roomInfo[i], sizeof(RoomInfoSocket), 0);

			// ȭ�� �����
			system("cls");

			while (1)
			{
				// ó�� �濡 �������� ���� ���° ���Կ� ���� ���
				if (firstTimeIntoRoom)
				{
					mySlotNumber = roomInfo[i].roomCount;
				}

				char playerReady[2] = "F";

				if (GetKeyState(0x52) & 0x8001) // ����ڰ� RŰ ������ Ȯ��
				{
					strcpy(playerReady, "T");
				}

				// ���� �����ߴٰ� �������� ������
				send(hSocket, playerReady, sizeof(playerReady), 0);

				// �� ������ �ٽ� �޴´�
				recv(hSocket, (char*)&roomInfo[i], sizeof(RoomInfoSocket), 0);

				// ó�� ����� ǥ���� ����
				if (firstTimeIntoRoom)
				{
					gotoXY(20, 10);
					printf("%d �� �� [ %s ] - �ο� �� : %d \n", roomInfo[i].roomNumber, roomInfo[i].player1Nick, roomInfo[i].roomCount);
					gotoXY(20, 11);
					printf("( R Ű�� ���� Ready �ϼ��� ) \n");
					gotoXY(20, 12);
					printf("Player 1 : %s \n", roomInfo[i].player1Nick);
					gotoXY(51, 12);
					printf("<%s> \n", (roomInfo[i].player1Ready == true) ? "  Ready  " : "Not ready");
					gotoXY(20, 13);
					printf("Player 2 : %s \n", roomInfo[i].player2Nick);
					gotoXY(51, 13);
					printf("<%s> \n", (roomInfo[i].player2Ready == true) ? "  Ready  " : "Not ready");
					firstTimeIntoRoom = false; // ���� ���� ī���͸� ���̻� �������� �ʴ´�
				}

				// ���Ŀ� �� �ȿ� �� ������ ������ ��� ���� (������ ����)
				gotoXY(20, 12);
				printf("Player 1 : %s                  \n", roomInfo[i].player1Nick);
				gotoXY(50, 12);
				printf("<%s> \n", (roomInfo[i].player1Ready == true) ? "  Ready  " : "Not ready");
				gotoXY(20, 13);
				printf("Player 2 : %s                  \n", roomInfo[i].player2Nick);
				gotoXY(50, 13);
				printf("<%s> \n", (roomInfo[i].player2Ready == true) ? "  Ready  " : "Not ready");


				// ��� ���� �߳� ���� ����
				if ((roomInfo[i].player1Ready == true) && (roomInfo[i].player2Ready == true)) // �� ����������
				{
					Sleep(1000); // ��� ���� ��¦ ���缭 ���尨�� �ش�
					break; // ���� �Ϸ� ������
				}

				Sleep(100);
			}

			//���� ��� ����
			GomokuStart(i);

			// ���� ���� ����
			WSACleanup();
			closesocket(hSocket);
		}
		else if (menu == 2) // 2. ���� �޴� ����
		{
			system("cls");

			gotoXY(33, 1);
			puts("[����]");
			puts("");
			puts("���� ������ ����� ���� �ؼ��մϴ�.");
			puts("����, ����, �밢�� ������� ���� 5���� ���� �������� �������� �ϴ� ����� �̱�ϴ�.");
			puts("3-3�� ����մϴ�.");
			puts("6���� �������� �ƴϳ�, �����ص� �¸��� �������� �ʽ��ϴ�.");
			puts("���� ���������� �����ϱ� ������ ���� ���� �̻��� �Ƿ��ڵ��� �Ϲݷ�� ����ϸ� ���� ����.");
			puts("������ �� 2���� ����Ǹ�, �¸� ������ 2�� �� 2���� ���� �ϼž� �մϴ�.");
			puts("");

			system("pause");
		}
		else // �ٸ��� �Է��ϸ� ����
		{
			return 0;
		}
	}

	return 0;
}


void GomokuStart(int i)
{
	int turnBefore = 0; // ���� �� ���� ���� (�ٵ��� ���� ���� �ִ��� Ȯ�ο�)
	bool screenNeedToChange = true; // �ٵ��� ������ �ٲ������ ���� ����

	// ó���� �������� �ʾ������� �����ϱ� ���� �̵����� 99�� �ʱ�ȭ
	myMoveInfo.myMoveX = 99;
	myMoveInfo.myMoveY = 99;
	
	// ������Ʈ ������ �޴� ������ ����
	std::thread updateGomokuThread(UpdateGomoku);

	// �ٵ��� �׸��� ����
	system("cls");
	while (true)
	{
		// �ٵ����� �ٲ� ��쿡 �ٽ� �׸���
		if (screenNeedToChange == true)
		{
			// �ٵ��� �׸���
			gotoXY(0, 0);
			for (int i = 0; i < 16; i++)
			{
				for (int j = 0; j < 16; j++)
				{
					if (inGameInfo.omok_plate[i][j] == 1)
					{
						omok_plate[i][j] = "��";
					}
					else if (inGameInfo.omok_plate[i][j] == 2)
					{
						omok_plate[i][j] = "��";
					}
					else
					{
						omok_plate[i][j] = "��";
					}

					printf("%s", omok_plate[i][j]);
				}
				puts("");
			}

			// ���� ���� ǥ���ϱ�
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
			printf("��:");
			gotoXY(38, 2);
			printf("%s (%d��)", roomInfo[i].player1Nick, inGameInfo.player1Win);
			gotoXY(38, 3);
			printf("%s (%d��)", roomInfo[i].player2Nick, inGameInfo.player2Win);

			screenNeedToChange = false;
		}

		// Ŀ�� �̵�
		gotoXY(pointer_X, pointer_Y);

		// Ű �Է¹ޱ�
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
			// ���� �� ������ ��� �� ���� ����
			if (mySlotNumber == 1 && (inGameInfo.turn % 2 == 0)) // �浹�϶�
			{
				myMoveInfo.myMoveX = change_X;
				myMoveInfo.myMoveY = change_Y;
			}
			else if (mySlotNumber == 2 && (inGameInfo.turn % 2 == 1)) // �鵹�϶�
			{
				myMoveInfo.myMoveX = change_X;
				myMoveInfo.myMoveY = change_Y;
			}
			screenNeedToChange = true;
		}

		// �¸� �Ǵ��ϱ� (��)
		/*RL_Check("��");
		UD_Check("��");
		Left_diagonal_Check("��");
		Right_diagonal_Check("��");*/

		// �¸� �Ǵ��ϱ� (��)
		/*RL_Check("��");
		UD_Check("��");
		Left_diagonal_Check("��");
		Right_diagonal_Check("��");*/
	}

	// ��� ���
	Sleep(100);
}


void UpdateGomoku()
{
	while (true)
	{
		// ������ ���� �ΰ��� ���� ���� �޾ƿ���
		recv(hSocket, (char*)&inGameInfo, sizeof(InGameInfoSocket), 0);

		// ���� �� ���� ������
		send(hSocket, (char*)&myMoveInfo, sizeof(MyMoveInfoSocket), 0);

		// ��� ���
		Sleep(100);
	}

	// ���� ���� ������ Ȯ���ϰ� �� �о ����
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

	for (int i = 0; i <= 4; i++) // ���Ž�� 0 1 2 3 4
	{

		if (omok_plate[change_Y][change_X + i] == ch) // ���� ��� ������
		{
			++check_ACount; // ī��Ʈ�� �߰��Ѵ�
		}
		else // �׷��� �߰��� �� ���� �ƴϰ� �ٸ��� ���̳� ����ִ� �ٵ��Ǹ� ������
		{
			break; // ���� ������
		}
	}
	for (int i = -1; i >= -5; i--) // �·�Ž�� -5 -4 -3 -2 -1
	{
		if (omok_plate[change_Y][change_X + i] == ch) // ���� ��� ������
		{
			++check_ACount; // ī��Ʈ�� �߰��Ѵ�
		}
		else // �׷��� �߰��� �� ���� �ƴϰ� �ٸ��� ���̳� ����ִ� �ٵ��Ǹ� ������
		{
			break; // ���� ������
		}
	}

	if (check_ACount == 5 && ch == "��")
	{
		gotoXY(20, 20);
		printf("��¸�!!!");
		return;
	}
	else if (check_ACount == 5 && ch == "��")
	{
		gotoXY(20, 20);
		printf("��¸�!!!");
		return;
	}
}


void UD_Check(const char *ch)
{
	int check_ACount = 0;

	for (int i = 0; i <= 4; i++) // ����Ž�� 0 1 2 3 4
	{
		if (omok_plate[change_Y + i][change_X] == ch) // ���� ��� ������
		{
			++check_ACount; // ī��Ʈ�� �߰��Ѵ�
		}
		else // �׷��� �߰��� �� ���� �ƴϰ� �ٸ��� ���̳� ����ִ� �ٵ��Ǹ� ������
		{
			break; // ���� ������
		}
	}
	for (int i = -1; i >= -5; i--) // �Ʒ���Ž�� -5 -4 -3 -2 -1
	{
		if (omok_plate[change_Y + i][change_X] == ch) // ���� ��� ������
		{
			++check_ACount; // ī��Ʈ�� �߰��Ѵ�
		}
		else // �׷��� �߰��� �� ���� �ƴϰ� �ٸ��� ���̳� ����ִ� �ٵ��Ǹ� ������
		{
			break; // ���� ������
		}
	}

	if (check_ACount == 5 && ch == "��")
	{
		gotoXY(20, 20);
		printf("��¸�!!!");
		return;
	}
	else if (check_ACount == 5 && ch == "��")
	{
		gotoXY(20, 20);
		printf("��¸�!!!");
		return;
	}
}


void Left_diagonal_Check(const char *ch)
{
	int check_ACount = 0;
	for (int i = -1; i >= -5; i--) // ������ �밢��Ž��
	{
		if (omok_plate[change_Y + i][change_X + i] == ch) // ���� ��� ������
		{
			++check_ACount; // ī��Ʈ�� �߰��Ѵ�
		}
		else // �׷��� �߰��� �� ���� �ƴϰ� �ٸ��� ���̳� ����ִ� �ٵ��Ǹ� ������
		{
			break; // ���� ������
		}
	}

	for (int i = 0; i <= 4; i++) // ������ �밢��Ž��
	{
		if (omok_plate[change_Y + i][change_X + i] == ch) // ���� ��� ������
		{
			++check_ACount; // ī��Ʈ�� �߰��Ѵ�
		}
		else // �׷��� �߰��� �� ���� �ƴϰ� �ٸ��� ���̳� ����ִ� �ٵ��Ǹ� ������
		{
			break; // ���� ������
		}
	}
	if (check_ACount == 5 && ch == "��")
	{
		gotoXY(20, 20);
		printf("��¸�!!!");
		return;
	}
	else if (check_ACount == 5 && ch == "��")
	{
		gotoXY(20, 20);
		printf("��¸�!!!");
		return;
	}
}


void Right_diagonal_Check(const char *ch)
{
	int check_ACount = 0;
	for (int i = -1; i >= -5; i--) // �������� �밢��Ž��
	{
		if (omok_plate[change_Y + i][change_X - i] == ch) // ���� ��� ������
		{
			++check_ACount; // ī��Ʈ�� �߰��Ѵ�
		}
		else // �׷��� �߰��� �� ���� �ƴϰ� �ٸ��� ���̳� ����ִ� �ٵ��Ǹ� ������
		{
			break; // ���� ������
		}
	}

	for (int i = 0; i <= 4; i++) // �������� �밢��Ž��
	{
		if (omok_plate[change_Y + i][change_X - i] == ch) // ���� ��� ������
		{
			++check_ACount; // ī��Ʈ�� �߰��Ѵ�
		}
		else // �׷��� �߰��� �� ���� �ƴϰ� �ٸ��� ���̳� ����ִ� �ٵ��Ǹ� ������
		{
			break; // ���� ������
		}
	}
	if (check_ACount == 5 && ch == "��")
	{
		gotoXY(20, 20);
		printf("��¸�!!!");
		return;
	}
	else if (check_ACount == 5 && ch == "��")
	{
		gotoXY(20, 20);
		printf("��¸�!!!");
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
				omok_plate[i][j] = "��";
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