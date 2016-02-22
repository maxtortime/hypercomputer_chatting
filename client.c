#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#define BUFSIZE 100
#define NAMESIZE 30
#define MAXLINE 127

void *send_message(void* arg);
void *recv_message(void* arg);
void send_file(int sock);
void recv_file(int sock);
void error_handling(char *message);

char name[NAMESIZE] = "[Default]";
char message[BUFSIZE];

int main(int argc, char **argv)
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void *thread_result;

	if(argc != 4)
	{
		printf("Usage : %s <IP> <PORT> <NAME> \n", argv[0]);
		exit(1);
	}
	
	sprintf(name, "[%s]", argv[3]);
	
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if(connect(sock,(struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error!");

	pthread_create(&snd_thread, NULL, send_message, (void*)sock);
	pthread_create(&rcv_thread, NULL, recv_message, (void*)sock);

	pthread_join(snd_thread, &thread_result);
	pthread_join(rcv_thread, &thread_result);

	close(sock);
	return 0;
}

void *send_message(void* arg)
{
	int sock = (int)arg;
	char name_message[NAMESIZE+BUFSIZE];
	while(1)
	{
		fgets(message, BUFSIZE, stdin);
		if(!strcmp(message, "q\n"))
		{
			close(sock);
			exit(0);
		}
		else if(!strncmp(message, "d", 1) && strlen(message) == 2) // 'd'입력 받으면 파일 전송모드로 변경
		{
			printf("send_file 실행!\n");
			write(sock, message, strlen(name_message)); 	// 이름 없이 'd'만 전송 // 파일전송 준비 신호
			send_file(sock);				// 파일전송
		}
		else
		{
			printf("메시지 전송 실행!\n");
			sprintf(name_message, "%s %s", name, message);
			write(sock, name_message, strlen(name_message));
		}
	}
}

void send_file(int sock)
{
	char filename[20];
	int sread, filesize, filenamesize, fp;
	int buf[MAXLINE+1], total=0;
	int sock_ = sock;

	if(fgets(filename, sizeof(filename), stdin) == NULL)
		error_handling("File send error(1)");

	filenamesize = strlen(filename);
	filename[filenamesize-1] = 0;

	if((fp = open(filename, O_RDONLY)) < 0)
	{
		error_handling("File send error(2)");
		exit(0);
	}
	send(sock_, filename, sizeof(filename), 0);
	filesize = lseek(fp, 0, SEEK_END);
	
	send(sock_, &filesize, sizeof(filesize), 0);
	lseek(fp, 0, SEEK_SET);

        while( total != filesize )
	{
		sread = read( fp, buf, 100 );
       		printf( "file is sending now.. \n" );
      	 	total += sread;
      	 	buf[sread] = 0;
       		send(sock_, buf, sread, 0 );
      	 	printf( "processing :%4.2f%% \n", total*100 / (float)filesize );
      	 	usleep(10000);
       	}
	printf("file transmission is complete\n");
	printf("filesize : %d, sending : %d\n", filesize, total);	

	close(fp);
	total = 0;
	filesize = 0;
}

void *recv_message(void* arg)
{
	int sock = (int)arg;
	char name_message[NAMESIZE+BUFSIZE];
	int str_len;
	while(1)
	{
		str_len = read(sock, name_message, NAMESIZE+BUFSIZE-1);
		if(strlen(name_message) == 2 && !strncmp(name_message, "d", 1))
		{
			recv_file(sock);
		}
		else if(str_len == -1)
		{
			return (void*)1;
		}	
		else
		{
			name_message[str_len] = 0;
			fputs(name_message, stdout);
		}
	}
}

void recv_file(int sock)
{
	char buf[MAXLINE+1];
	char filename[30];
	int filesize, fp;
	int total=0, sread;

	printf("recv_file(1)\n\n");
        bzero(filename, 30);
	printf("recv_file(1.5)\n\n");
        recv(sock, filename, sizeof(filename), 0 );
	printf("recv_file(2)\n\n");

        printf( "%s \n", filename );

        read(sock, &filesize, sizeof(filesize) );
        printf( "%d \n", filesize );
	printf("recv_file(3)\n\n");

        fp = open( filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	printf("recv_file(4)\n\n");

        while( total != filesize )
        {
                sread = recv(sock, buf, 100, 0 );
                printf( "file is receiving now.. \n" );
                total += sread;
                buf[sread] = 0;
                write( fp, buf, sread );
                bzero( buf, sizeof(buf) );
                printf( "processing : %4.2f%% \n", total*100 / (float)filesize );
                usleep(1000);
        }
	printf("recv_file(5)\n\n");
        printf( "file traslating is completed \n" );
        printf( "filesize : %d, received : %d \n", filesize, total );

        close(fp);
        total = 0;
        filesize = 0;
	
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
