#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>

#define BUFSIZE 100
#define MAXLINE 127

void *clnt_connection(void *arg);
void send_message(char *message, int len, int clnt_sock);
void recv_file(int clnt_sock);
void send_file(char *filename, int clnt_sock);
void error_handling(char *message);

int clnt_number = 0;
int clnt_socks[10];
char filename_[30];

pthread_mutex_t mutx;

int main(int argc, char **argv)
{
	int serv_sock;
	int clnt_sock;
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	int clnt_addr_size;
	pthread_t thread;

	if(argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	if(pthread_mutex_init(&mutx, NULL))
		error_handling("mutex init error");
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("bind() error");
	if(listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	while(1)
	{
		clnt_addr_size = sizeof(clnt_addr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

		pthread_mutex_lock(&mutx);

		clnt_socks[clnt_number++] = clnt_sock;
		pthread_mutex_unlock(&mutx);

		pthread_create(&thread, NULL, clnt_connection, (void*)clnt_sock);
		printf("접속된 클라이언트 IP : %s\n", inet_ntoa(clnt_addr.sin_addr));
	}
	return 0;
}


void *clnt_connection(void *arg)
{
	int clnt_sock = (int)arg;
	int str_len = 0;
	char message[BUFSIZE];
	char filename[30];
	int i;

	while((str_len = read(clnt_sock, message, sizeof(message))) != 0)
	{
		message[str_len] = 0x00;
		if(strlen(message) == 2 && !strncmp(message, "d", 1))
		{
			send_message(message, str_len, clnt_sock); // 클라이언트들에게  'd'신호를 보냄으로써 파일 받을 준비를 하게끔 한다.
			recv_file(clnt_sock); 
			send_file(filename, clnt_sock); // 받은 파일을 접속된 클라이언트들에게 모두 뿌려준다.
		}
		else
		{
			send_message(message, str_len, clnt_sock);
		}	
	}	

	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_number; i++)
	{
		if(clnt_sock == clnt_socks[i])
		{
			clnt_socks[i] = clnt_socks[clnt_number - 1]; // 클라이언트 1명 나갈시 클라이언트 체크하는 배열변수를 한칸씩 앞으로 땡김.
			break;
		}
	}
	clnt_number--;

	pthread_mutex_unlock(&mutx);

	close(clnt_sock);
	return 0;
}

void recv_file(int clnt_sock)
{
	char filename[30];
	char buf[MAXLINE+1];
	int filesize = 0, total = 0;
	int sread, fp;
	int i;
		
	bzero(filename, 30);
	recv(clnt_sock, filename, sizeof(filename), 0 );
			
	printf("받은 파일 이름 : %s \n", filename );

        read(clnt_sock, &filesize, sizeof(filesize) ); 
        printf("받은 파일 크기 : %d \n", filesize );

	//fp = open( filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	fp = open( filename, O_RDWR | O_CREAT | O_TRUNC, 0777);

  	while( total != filesize )
        {
        	sread = recv(clnt_sock, buf, 100, 0 );
                printf( "file is receiving now.. \n" );
             	total += sread;
                buf[sread] = 0;
                write( fp, buf, sread );
                bzero( buf, sizeof(buf) );
		printf( "processing : %4.2f%% \n", total*100 / (float)filesize );
	      	usleep(1000);
 	}
	printf( "file traslating is completed \n" );
	printf( "filesize : %d, received : %d \n", filesize, total );

	close(fp);
	total = 0;
	filesize = 0;
	
	strcpy(filename_, filename);
}

void send_file(char *filename, int clnt_sock)
{
	int filenamesize, filesize, fp;
	int i, sread, total = 0;
	char buf[MAXLINE+1];
	char filename_2[30];

	strcpy(filename_2, filename_); 
	printf("보낼 파일 이름 : %s \n", filename_2);

	for(i=0; i<clnt_number; i++)
	{
		if(!(clnt_sock == clnt_socks[i]))
		{
		        filenamesize = strlen(filename_2);
	               	filename_2[filenamesize-1] = 0;

                	if((fp = open(filename_2, O_RDONLY )) < 0)
                	{
                        	printf( "open failed\n" );
                        	exit(0);
                	}
			
                	send(clnt_sock, filename_2, sizeof(filename_2), 0 );
                	filesize = lseek( fp, 0, SEEK_END );

                	send(clnt_sock, &filesize, sizeof(filesize), 0 );
                	lseek(fp, 0, SEEK_SET );

                	while( total != filesize )
                	{
                        	sread = read(fp, buf, 100 );
                        	printf( "file is sending now.. \n" );
                        	total += sread;
                        	buf[sread] = 0;
                        	send(clnt_sock, buf, sread, 0 );
                        	printf( "processing :%4.2f%% \n", total*100 / (float)filesize );
                        	usleep(10000);
                	}
                	printf( "file translating is completed \n" );
                	printf( "filesize : %d, sending : %d \n", filesize, total );

                	close(fp);
                	total = 0;
                	filesize = 0;

		}	
	}
}

void send_message(char *message, int len, int clnt_sock)
{
	int i;
	int compare_clnt_sock = clnt_sock;
	pthread_mutex_lock(&mutx);
	
	for(i=0; i<clnt_number; i++)
	{
		if(!(compare_clnt_sock == clnt_socks[i]))
			write(clnt_socks[i], message, len);
	}
	pthread_mutex_unlock(&mutx);
}


void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
