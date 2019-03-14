#include<stdio.h>	//fputs
#include<sys/types.h>	//socket, connect, open
#include<sys/stat.h>	//open
#include<sys/socket.h>	//socket, connect
#include<stdlib.h>	//exit
#include<string.h>	//explicit_bzero
#include<arpa/inet.h>	//inet_pton
#include<unistd.h>	//read
#include<fcntl.h>	//open
#include<netinet/in.h>	//INET_ADDRSTRLEN
#include<sys/time.h>
#include<time.h>
#include<netdb.h>	//getaddrinfo

#define MAXCLIENTQ 100	//backlog
#define MAXLEN 102400
#define MSGLEN 50
#define PORT 1024

int main(int argc, char **argv)
{
	int listenfd, connfd, n;
	struct sockaddr_in servaddr;
	char buff[MAXLEN+1];
	
	if((listenfd=socket(AF_INET, SOCK_STREAM, 0))==-1)
	{
		perror("socket");
		exit(1);
	}
	int opt=1;
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
	{
	perror("setsockopt"); 
	exit(EXIT_FAILURE); 
	}
	
	explicit_bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(atoi(argv[1]));
	if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("bind");
		exit(2);
	}
	
	if(listen(listenfd, MAXCLIENTQ)==-1)
	{
		perror("listen");
		exit(3);
	}
	
	while(1)
	{		
		if((connfd=accept(listenfd, NULL, NULL))==-1)
		{
			perror("accept");
			exit(4);
		}
		
		if(fork()==0)
		{
			close(listenfd);
			char getRequest[MAXLEN+1];
			read(connfd, getRequest, MAXLEN);
			printf("%s",getRequest);
			char host[MAXLEN+1];
			char fname[501]="./", temp[499], temp2[100], temp0[100], temp3[10240], temp4[10240];
			sscanf(getRequest,"GET %s/%s %s\r\n%[a-zA-Z.,/0-9!@#$%^&*()\r\n\t{};:<>.?]\r\n\r\n", host, temp, temp2, temp4);
			//sscanf(getRequest,"GET http://%s %[^\n]", host, temp2);
			//if(host[0]=='w')
			//	printf("www\n");
			printf("temp2: %s\n",temp2);
			sscanf(host,"%s/%s",temp0);
			//printf("\n\ntemp4: %s\n\n",temp4);
			//sscanf(temp4,"Host: %s:%[^\n]", host, temp3);
			
			printf("host: %s\n",host);
			
			strncat(fname,temp,strlen(temp));
			printf("file: %s\n",fname);
			//explicit_bzero(buff, sizeof(buff));
			
			char statusOK[100]="HTTP/1.1 200 OK\r\n\r\n";
			char statusNotFound[100]="HTTP/1.1 404 PageNotFound\r\n\r\n";
			FILE *fp=fopen(fname, "r"); 
			if(fp==NULL)
			{
				//perror("fopen");
				int sockfd, n;
				
				struct addrinfo hints, *servinfo, *p;
				int rv;

				memset(&hints, 0, sizeof hints);
				hints.ai_family = AF_UNSPEC; // can use AF_INET6 to force IPv6
				hints.ai_socktype = SOCK_STREAM;

				if((rv = getaddrinfo(host, "http", &hints, &servinfo)) != 0)
				{
				    	printf("getaddrinfo: %s\n", gai_strerror(rv));
				    	exit(1);
				}

				// loop through all the results and connect to the first we can
				for(p = servinfo; p != NULL; p = p->ai_next)
				{
					if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
					{
						perror("socket");
						continue;
					}

					if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
					{
						perror("connect");
						close(sockfd);
						continue;
					}

					break; // if we get here, we must have connected successfully
				}

				if(p==NULL)
				{
					// looped off the end of the list with no connection
					printf("failed to connect\n");
					
					//exit(2);
				}
						
				write(sockfd, getRequest, strlen(getRequest));
				char ch;
				int print=0;
				while(1)
				{
					if(read(sockfd, &ch, sizeof(char))!=1)
					{
						perror("read");
						break;//exit(7);
					}
					if(ch==EOF)
						break;
					if(ch=='<')
						print=1;
					if(print)
						printf("%c", ch);
				}
				
				close(connfd);
				exit(5);
			}
			else
			{
				fseek(fp, 0L, SEEK_END);
				long len = ftell(fp);
				write(connfd, statusOK, strlen(statusOK));
				rewind(fp);
				char  ch;
				while(len--)
				{
					ch = fgetc(fp);
					write(connfd, &ch, sizeof(char));
				}
				fclose(fp);
			}
			printf("Sent\n");
			close(connfd);
			exit(0);
		}
		close(connfd);
	}
}
