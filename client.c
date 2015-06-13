#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#define BUF_SIZ 1024

int send_rate=200;
int recv_rate=200;

int send_off=200;
int recv_off=200;

void credit_20103318(void)
{
    // add print
    printf("20103318 \n");
}
void credit_20103327(void)
{
    printf("20103327 HeeTae designed function credit_20103327 and make client.c\n");
}
void credit_20103376(void)
{
    printf("20103376 Gisung Im desgined function connection(), put(),get(), create_20103376 and make server.c\n");
}
void credit_20123360(void)
{
    printf("20123360 Min Gyeongmin designed function credit_20123360()\n");
}
void credit_20133342(void)
{
    // add print
    printf("20103342 \n");
}
int sendAll(int sockid,const unsigned char *buffer,int len)
{
	int total_len=len;
	int tran_len=0;

	while(tran_len<total_len)
	{
		tran_len+=send(sockid,&buffer[tran_len],total_len-tran_len,0);
	}
	return total_len;
}
void sig_handler(int signo)
{
	send_rate=send_off;
	recv_rate=recv_off;
	alarm(1);
}

void sig_end(int signo)
{
	send_rate=send_off;
	recv_rate=recv_off;
}
void fileSizePrint(int size)
{
	int x;
	if((x=size/1000000)>0)
		printf("(SIZE: %d MB)",x);
	else if((x=size/1000)>0)
		printf("(SIZE: %d KB)",x);
	else
		printf("(SIZE: %d B)",size);
}

void progressBar(int tran_sz,int total_sz)
{
	int tmp = (float)tran_sz / total_sz * 10;
	int i;
	fflush(stdout);
	printf("\r[");
	for(i=0; i<10; i++)
	{
		if(i <tmp )
			printf("*");
		else 
			printf(" ");
	}
	if(tmp == 10)
		printf("] // progress bar\n");
	else
		printf("] // progress bar");

}


int connectServer(int sockid, struct sockaddr_in *server_addr,char *ip,char *port)
{
	struct hostent *hostp;

	if ((hostp = gethostbyname(ip)) == 0) {
		fprintf(stderr,"%s: unknown host\n", ip);
		exit(1);
	}

    	//Client: creating addr structure for server
    	bzero(server_addr, sizeof(*server_addr));
    	server_addr->sin_family = AF_INET;
    	memcpy((void*)&server_addr->sin_addr,hostp->h_addr,hostp->h_length);
    	server_addr->sin_port = htons(atoi(port));

	// connect check;
	
	if(connect(sockid, (struct sockaddr*)server_addr,sizeof(*server_addr)) < 0)
	{
		perror("Conection failed");
		return -1;
	}
	else
	{
		printf("Successfully connected.\n");
		return 0;
	}
}

int put(int sockid,char *fileName )
{
	FILE *fp;	
	int nread, retcode;
	char temp[128];
	unsigned char buffer[BUF_SIZ];
	int tran_sz=0,file_sz;

	fp = fopen(fileName,"rb");
	if(fp == NULL){perror("Bad File Name: "); exit(1); }

	// get file length

	bzero(temp,127);
	fseek(fp,0,SEEK_END);
	file_sz = ftell(fp);
	fseek(fp,0,SEEK_SET);
	sprintf(temp,"%i",file_sz);

	retcode = send(sockid,temp,127,0);
	if(retcode < 0) { perror("Err : Can't Send To Server"); }


	printf("[%s]",fileName);
	fileSizePrint(file_sz);
	printf(" is being sent\n");
	
	bzero(buffer,BUF_SIZ);
	signal(SIGALRM,sig_handler);
	alarm(1);
	// data trasfer
	do{
		if(send_rate>0)
		{
			nread=fread(buffer,sizeof(unsigned char),BUF_SIZ,fp);

			if(nread<BUF_SIZ){
				retcode = sendAll(sockid,buffer,nread);
				if(retcode <0) { perror("Err Can't Send To Server"); }
			}
			else{
				retcode = sendAll(sockid,buffer,BUF_SIZ);
				if(retcode <0) { perror("Err Can't Send To Server"); }
			}
			
			bzero(buffer,BUF_SIZ-1);
			// progress bar
			tran_sz+=retcode;
			progressBar(tran_sz,file_sz);
			send_rate--;
		}
	}while(tran_sz<file_sz);

	fclose(fp);

	if(tran_sz<file_sz)
	{
		printf("\nFailed transfferred\n");
		return -1;
	}
	else
	{
		printf("Successfully transfferred\n");
		return 0;
	}

}
 
int get(int sockid,char *fileName)
{
	int fd,n,r;
	int tran_sz,file_sz,remain_sz;
	unsigned char buffer[BUF_SIZ];
	char temp[127];
	FILE *fp;

	bzero(buffer,BUF_SIZ);
	// check to file exist
	n=recv(sockid,temp,127,0);
	if(n < 0) { perror("Err Can't Receive From Client"); }
	temp[n] = '\0';

	file_sz=atoi(temp);
	remain_sz = file_sz;
	tran_sz=0;

	// open file
	fp = fopen(fileName,"w+b");
	if(fp == NULL){
		fclose(fp);
		perror("File Write Issue: ");
		return;
	}

	printf("[%s]",fileName);
	fileSizePrint(file_sz);
	printf(" is being received\n");

	signal(SIGALRM,sig_handler);
	alarm(1);
	//data recv
	do{
		if(recv_rate>0)
		{
			if(remain_sz < BUF_SIZ){
				n=recv(sockid,buffer,remain_sz,0);
				if(n < 0) {perror("Err Can't Receive From Client"); }
			}
			else {
				n=recv(sockid,buffer,BUF_SIZ,0);
				if(n < 0) {perror("Err Can't Receive From Client"); }
			}
			tran_sz+=n;
			progressBar(tran_sz,file_sz);
			r=fwrite(buffer,sizeof(unsigned char),n,fp);
			if(r<0)
			{
				perror("Write error");
				exit(1);
			}
			fflush(fp);
			remain_sz-=n;

			bzero(buffer,BUF_SIZ);
			recv_rate--;
		}
	}while(remain_sz>0);
	printf("Successfully transferred\n");

	fclose(fp);
	signal(SIGALRM,sig_end);
	return 0;
}

int main(int argc, char *argv[])
{
    int sockid;
    struct sockaddr_in server_addr;
    int connectFlag=0;
    int r;
    char msg[128];
    char command[10]; 
    char val[2][12];	

	// client: creating socket
	if((sockid = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		printf("client socket failed: %d\n",errno);
		exit(0);
	}

	while(1)
	{
		fseek(stdin,0,SEEK_END);
		printf("> ");
		fscanf(stdin,"%s",command);

		if(strcmp("connect",command) == 0 && connectFlag==0)
		{
			fscanf(stdin,"%s %s",val[0],val[1]);
			if(connectServer(sockid, &server_addr,val[0],val[1])==0)
				connectFlag=1;
		}
		else if(strcmp("sendrate",command) ==0)
		{
			fscanf(stdin,"%s",val[0]);
			if(connectFlag)
			{
				int len=strlen(val[0]);
				int i;
				for(i=0; i<len; i++)
				{
					if(val[0][i]<'0' && val[0][i]>'9')
					{
						val[0][i]='\0';
						break;
					}
				}
				send_rate=atoi(val[0]);
				send_off=send_rate;
				printf("ok\n");
			}
			else
				printf("Not connected in server\n");
		}
		else if(strcmp("recvrate",command) ==0)
		{
			fscanf(stdin,"%s",val[0]);
			if(connectFlag)
			{
				int len=strlen(val[0]);
				int i;
				for(i=0; i<len; i++)
				{
					if(val[0][i]<'0' && val[0][i]>'9')
					{
						val[0][i]='\0';
						break;
					}
				}
				recv_rate=atoi(val[0]);
				recv_off=recv_rate;
				printf("ok\n");
			}
			else
				printf("Not connected in server\n");
		}
		else if(strcmp("ratecurr",command) ==0)
		{
			if(connectFlag)
				printf("send: %dK, recv: %dK\n",send_off,recv_off);
			else
				printf("Not connected in server\n");
		}
		else if(strcmp("credit",command) ==0)
		{
			fscanf(stdin,"%s",val[0]);
        		 int id=atoi(val[0]);
        		  switch(id)
        		  {
        		      case 20103318:
        		            credit_20103318();
        			    break;
		              case 20103327:
               			    credit_20103327();
               			    break;
                              case 20103376:
                		    credit_20103376();
                 		    break;
                	      case 20123360:
                    		    credit_20123360();
                    		    break;
                	      case 20133342:
                    		    credit_20133342();
                    		    break;
                	      default:
                    		printf("Invalid id %d\n",id);
                    		break;
        		 }
		}
		else if(strcmp("put",command) == 0)
		{
			if(connectFlag)
			{
				fscanf(stdin, "%s",val[0]);
				sprintf(msg,"put %s",val[0]);
				r=send(sockid,msg,sizeof(msg)-1,0);
				printf("msg:%s, sendByte: %d\n",msg,r);

				put(sockid,val[0]);
			}
			else
				printf("Not connected in Server\n");
		}
		else if(strcmp("get",command) == 0)
		{
			if(connectFlag)
			{
				fscanf(stdin,"%s",val[0]);
				sprintf(msg,"get %s",val[0]);
				send(sockid,msg,sizeof(msg),0);

				get(sockid,val[0]);
			}
			else
				printf("Not connected in Server\n");
		}
		else if(strcmp("close",command) == 0)
		{
			if(connectFlag)
			{
                		sprintf(msg,"quit");
                		send(sockid,msg,sizeof(msg),0);
				close(sockid);
				connectFlag=0;
				printf("Disconnected\n");


	            	if((sockid = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		            printf("client socket failed: %d\n",errno);
		            exit(0);
	            	}
			}
			else
				printf("Already not connected in server\n");
		}
		else if(strcmp("quit",command) == 0)
		{	
		 	if(connectFlag)
				printf("yet connected in server\n");
			else
				return 0;
		} 
		else 
			printf("Invalid command :%s\n",command);

		memset(&val[0],0,sizeof(val[0]));
		memset(&val[1],0,sizeof(val[1]));

	}
}
