#pragma GCC diagnostic ignored "-Wwrite-strings"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

void *New_connect(void *arg); // 새로운 연결을 만드는 스레드용 함수
void Error_handling(char *message); // 오류를 출력하는 함수

char* serverAdress[2]; // 서버 주소 (포트)
int serv_sock; // 서버 소켓 핸들
unsigned int t_Count = 0; // 몇번째 스레드인지 카운트

typedef struct RoomInfoSocket // 방 정보를 저장하는 구조체
{
	int roomNumber; // 방 번호
	char player1Nick[32]; // 1번 플레이어 닉네임 (방 이름이기도 하다)
	char player2Nick[32]; // 2번 플레이어 닉네임
	bool player1Ready; // 1번 플레이어 레뒤했나요
	bool player2Ready; // 2번 플레이어 레뒤했나요
	int roomCount; // 방에 들어간 총 플레이어 수
}RoomInfoSocket;
RoomInfoSocket roomInfo[10]; // 방 10개의 정보

typedef struct inGameInfoSocket // 플레이 상태를 저장하는 구조체
{
	int omok_plate[16][16]; // 바둑판 상태
	int turn; // 한경기 내에서 총 둔 횟수
	int player1Win; // 플레이어1 이 몇번 이겼는지
	int player2Win; // 플레이어2 이 몇번 이겼는지
}InGameInfoSocket;
InGameInfoSocket inGameInfo[10]; // 방 10개의 현재 플레이 상태 정보 저장용 구조체

typedef struct MyMoveInfoSocket // 오목을 둔 위치를 저장하는 구조체
{
	int myMoveX;
	int myMoveY;
}MyMoveInfoSocket; // 방 10개의 방금 둔 바둑돌 위치 정보 저장용 구조체
MyMoveInfoSocket myMoveInfo[10]; // 방 10개의 방금 둔 바둑돌 위치 정보 저장용 구조체
MyMoveInfoSocket myMoveInfoBefore[10]; // 이전 상태 비교용


int main(int argc, char *argv[])
{
	struct sockaddr_in serv_addr;
	
	serverAdress[1] = "9999";
	printf("Default Port : %s \n", serverAdress[1]);
	
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
	{
		Error_handling("socket() error");
	}
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(atoi(serverAdress[1]));
	
	// prevent bind error (소켓 재사용 허용하기)
	int nSockOpt = 1;
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &nSockOpt, sizeof(nSockOpt));
	
	if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1)
	{
		Error_handling("bind() error"); 
	}
	
	if(listen(serv_sock, 5)==-1)
	{
		Error_handling("listen() error");
	}
	
	// 방들의 정보 처음 시작시 초기화
	for(int i = 0; i < 10; i++)
	{
		roomInfo[i].roomNumber = i; // 룸 넘버 할당
		strcpy(roomInfo[i].player1Nick, "<< Room is empty >>"); // 룸 이름 할당 (첫번째 유저 닉네임)
		strcpy(roomInfo[i].player2Nick, "<< Room is empty >>"); // 룸 이름 할당 (두번째 유저 닉네임)
		roomInfo[i].player1Ready = false; // 첫번째 유저 레뒤 상태 초기화
		roomInfo[i].player2Ready = false; // 두번째 유저 레뒤 상태 초기화
		roomInfo[i].roomCount = 0; // 기본적으로 0명 들어있음
	}
	
	// 서버 시작시 첫 스레드 생성
	pthread_t firstThread; // 동시접속을 위한 스레드 추가 생성
	++t_Count; // 스레드 카운트 증가
	pthread_create(&firstThread, NULL, &New_connect, (void *)&t_Count); // 스레드 열기
	
	// 스레드가 전부 닫히면 서버를 종료하기 위해 검사 반복
	while(true)
	{
		usleep(10000000); // 잠시 대기 10초
	}
	
	close(serv_sock); // 서버 소켓 종료
	
	return 0; // 프로그램 정상 종료
}


void *New_connect(void *arg)
{
	char playerNick[32] = ""; // 플레이어가 생성한 닉네임
	char playerChoosedRoomNumber[2] = ""; // 플레이어가 선택한 방 번호
	char playerSlotNumber = 0; // 플레이어가 몇번째로 방에 접속하였는지
	
	int clnt_sock;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;
	
	clnt_addr_size=sizeof(clnt_addr);  
	clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
	if(clnt_sock == -1)
	{
		Error_handling("accept() error");  
	}
	// 연결된 스레드 갯수 카운트 표시
	printf("Connect accepted : %d \n", *((int *)arg));
	printf("Connected client IP: %s \n", inet_ntoa(clnt_addr.sin_addr));
	
	// 연결 성공 확인 메세지 보냄
	write(clnt_sock, "Good", sizeof("Good"));
	printf("Message sended : %s \n", "Good");
	
	// 닉네임 받음
	read(clnt_sock, playerNick, sizeof(playerNick));
	printf("Player's nick name received : %s \n", playerNick);
	
	// 새로운 연결을 허용하기 위해 새로운 연결 스레드를 만듬
	pthread_t newThread; // 동시접속을 위한 스레드 추가 생성
	if(pthread_create(&newThread, NULL, &New_connect, (void *)&t_Count) != 0) // 새로운 스레드 열고 성공 했는지 검사
	{
		Error_handling("pthread_create() error");
	}
	printf("Connection count : %d \n", ++t_Count); // 스레드 카운트 증가
	
	// 방들의 정보를 다 보냄
	for(int i = 0; i < 10; i++)
	{		
		write(clnt_sock, (char*)&roomInfo[i], sizeof(RoomInfoSocket));
		printf("Room info sended : %d , %s, %d \n", roomInfo[i].roomNumber, roomInfo[i].player1Nick, roomInfo[i].roomCount);
	}
	
	// 사용자가 선택한 방 번호를 받음
	read(clnt_sock, playerChoosedRoomNumber, sizeof(playerChoosedRoomNumber));
	printf("%s Player chose room : %s \n", playerNick, playerChoosedRoomNumber);
	
	// 사용자가 선택한 방 번호를 int형으로 변환
	int i = atoi(playerChoosedRoomNumber);
	
	// 방에 들어간 사용자 정보를 방 정보에 갱신
	if(roomInfo[i].roomCount == 0) // 만약 방에 아무도 없어서 처음 접속한 경우
	{
		strcpy(roomInfo[i].player1Nick, playerNick); // 방 1번 슬롯에 플레이어 닉네임 저장
		roomInfo[i].roomCount++; // 방에 들어간 플레이어 수 추가
		playerSlotNumber = 1;
	}
	else if(roomInfo[i].roomCount == 1) // 만약 방에 두번째로 들어간 경우
	{
		strcpy(roomInfo[i].player2Nick, playerNick); // 방 1번 슬롯에 플레이어 닉네임 저장
		roomInfo[i].roomCount++; // 방에 들어간 플레이어 수 추가
		playerSlotNumber = 2;
	}
	else // 방이 가득 찼는데 들어간 경우
	{
		// 어차피 roomInfo[i].roomCount 가 2 이상이기 때문에 아무것도 하지 않는다
	}
	
	// 사용자가 선택한 방 정보를 보냄
	write(clnt_sock, (char*)&roomInfo[i], sizeof(RoomInfoSocket));
	
	// 모든 사용자가 레뒤 할때까지 반복해서 방 정보 전송
	while(true)
	{
		char player1Ready[2] = "F"; // 플레이어1 레뒤 여부를 임시로 저장
		char player2Ready[2] = "F"; // 플레이어2 레뒤 여부를 임시로 저장
		
		// 먼저 플레이어들의 레뒤 여부를 받는다
		if(playerSlotNumber == 1) // 플레이어1 이 보냈는지 체크
		{
			read(clnt_sock, player1Ready, sizeof(player1Ready)); // 사용자가 레뒤 했는지 여부를 받음
		}
		else if(playerSlotNumber == 2) // 플레이어2 가 보냈는지 체크
		{
			read(clnt_sock, player2Ready, sizeof(player2Ready)); // 사용자가 레뒤 했는지 여부를 받음
		}
		
		// 레뒤 정보를 방 정보에 반영한다
		if(playerSlotNumber == 1 && (strcmp(player1Ready, "T") == 0)) // 플레이어1 이 레뒤 했는지 체크 
		{
			roomInfo[i].player1Ready = true; // 사용자가 레뒤 했는지 여부를 방 정보에 저장
		}
		else if(playerSlotNumber == 2 && (strcmp(player2Ready, "T") == 0)) // 플레이어2 가 레뒤 했는지 체크 
		{
			roomInfo[i].player2Ready = true; // 사용자가 레뒤 했는지 여부를 방 정보에 저장
		}		
		
		// 사용자에게 지금 들어간 방의 정보만 다시 보냄
		write(clnt_sock, (char*)&roomInfo[i], sizeof(RoomInfoSocket));
		//printf("Into room : %d , %s-%d,  %s-%d, (%d)\n", roomInfo[i].roomNumber, roomInfo[i].player1Nick, roomInfo[i].player1Ready, roomInfo[i].player2Nick, roomInfo[i].player2Ready, roomInfo[i].roomCount);
		
		// 모두 레뒤 했으면 게임 준비 완료
		if((roomInfo[i].player1Ready == true) && (roomInfo[i].player2Ready == true))
		{
			// 레뒤 했으면 반복문 나감
			printf("%d : Room all ready \n", roomInfo[i].roomNumber);
			break;
		}
		
		usleep(100000); // 잠시 대기 0.1초
	}	
	
	// 본격적인 오목 대결 시작되고 클라이언트 게임 상태 초기화를 위해 게임 정보 보냄
	printf("%d : Room state %s %d vs %s %d (%d) \n", roomInfo[i].roomNumber, roomInfo[i].player1Nick, inGameInfo[i].player1Win, roomInfo[i].player1Nick, inGameInfo[i].player2Win, inGameInfo[i].turn);
	
	
	while(true)
	{
		// 방 정보 보내기
		write(clnt_sock, (char*)&inGameInfo[i], sizeof(InGameInfoSocket));
		
		// 플레이어가 놓은 바둑돌 위치 받기
		read(clnt_sock, (char*)&myMoveInfo[i], sizeof(MyMoveInfoSocket));

		usleep(100000); // 잠시 대기 0.1초
		
		// 플레이어가 둔 위치가 새로 갱신된 경우 inGameInfo에 반영
		if (myMoveInfo[i].myMoveX != 99 || myMoveInfo[i].myMoveY != 99) // 움직이지 않았을때의 초기값(99)인 경우 반영하지 않는다
		{
			if (myMoveInfo[i].myMoveX != myMoveInfoBefore[i].myMoveX || myMoveInfo[i].myMoveY != myMoveInfoBefore[i].myMoveY)
			{
				if (playerSlotNumber == 1 && (inGameInfo[i].turn % 2 == 0)) // 플레이어 1 차례
				{
					inGameInfo[i].omok_plate[myMoveInfo[i].myMoveY][myMoveInfo[i].myMoveX] = 1;
					inGameInfo[i].turn++;

					myMoveInfoBefore[i].myMoveX = myMoveInfo[i].myMoveX;
					myMoveInfoBefore[i].myMoveY = myMoveInfo[i].myMoveY;
				}
				if (playerSlotNumber == 2 && (inGameInfo[i].turn % 2 == 1)) // 플레이어 2 차례
				{
					inGameInfo[i].omok_plate[myMoveInfo[i].myMoveY][myMoveInfo[i].myMoveX] = 2;
					inGameInfo[i].turn++;

					myMoveInfoBefore[i].myMoveX = myMoveInfo[i].myMoveX;
					myMoveInfoBefore[i].myMoveY = myMoveInfo[i].myMoveY;
				}
			}
		}
		
		usleep(100000); // 잠시 대기 0.1초
	}
	
	// 수신 버퍼 남은거 확인하고 다 읽어서 비우기
	u_long tmp_long = 0;
	if (ioctl(clnt_sock, FIONREAD, &tmp_long) != -1)
	{
		for (int j = 0; j < tmp_long; ++j)
		{
			read(clnt_sock, (char*)&myMoveInfo[i], sizeof(MyMoveInfoSocket));
		}
	}
	
	close(clnt_sock); // 이 스레드와 연결된 클라이언트 소켓 종료
	
	return NULL; // 지금 이 스레드 종료
}


void Error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
