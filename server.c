#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#define BUF_SIZ 1024
#define RECV_TYPE 100
#define SEND_TYPE 200

int pnt_res=1;

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
	pnt_res=1;
	alarm(1);
}

void sig_end(int signo)
{
	pnt_res=1;
}

void progressStatus(int trans_sz,int total_sz,char *filename,int type)
{
	int percent=(float)trans_sz/total_sz*100;
	int transSz,totalSz;

	printf("Transfer status: ");
	if(type==RECV_TYPE)
		printf("recv [%s]",filename);
	else if(type==SEND_TYPE)
		printf("send [%s]",filename);

	if((totalSz=total_sz/1000000)>0)
	{
		transSz=trans_sz/1000000;
		printf("[%d%%, %d MB/%d MB]",percent,transSz,totalSz);
	}
	else if((totalSz=total_sz/1000)>0)
	{
		transSz=trans_sz/1000;
		printf("[%d%%, %d KB/%d KB]",percent,transSz,totalSz);
	}
	else
		printf("[%d%%, %d B/%d B]",percent,trans_sz,total_sz);

	printf("\n");

}


int put(int sockid, char *fileName)
{
	int fd,r;
	int nread,retcode,tran_sz,file_sz,remain_sz;
	char temp[128];
	unsigned char buffer[BUF_SIZ];
	FILE *fp;

	bzero(buffer,BUF_SIZ);
	// file error check
	nread = recv(sockid,temp,127,0);
	if(nread < 0) { perror("Err Can't Receive From Client"); }
	temp[nread]='\0';

	file_sz=atoi(temp);
	remain_sz=file_sz;
	tran_sz=0;

	// open file
	fp = fopen(fileName,"w+b");
	if(fp == NULL){
		fclose(fp);
		perror("File Write Issue: ");
		return;
	}
	
	signal(SIGALRM,sig_handler);
	alarm(1);
	do{
		if(remain_sz < BUF_SIZ){
			nread = recv(sockid,buffer,remain_sz,0);
			if(nread < 0) {perror("Err Can't Receive From Client"); }
		}
		else{
			nread = recv(sockid,buffer,BUF_SIZ,0);
			if(nread < 0) { perror("Err : Can't Receive From Client"); }
		}
		tran_sz+=nread;
		if(pnt_res==1){
			progressStatus(tran_sz,file_sz,fileName,RECV_TYPE);
			pnt_res=0;
		}
		
		r=fwrite(buffer, sizeof(unsigned char), nread, fp);
		if(r<0)
		{
			perror("write error");
			exit(1);
		}
		fflush(fp);
		remain_sz-=nread;

		bzero(buffer,BUF_SIZ);
	}while(remain_sz>0);		
		
	
	progressStatus(tran_sz,file_sz,fileName,RECV_TYPE);
	printf("\n");

	close(fd);

	signal(SIGALRM,sig_end);
}

int get(int sockid,char *fileName)
{
	int fd;
	int nread,retcode,tran_sz=0,file_sz;
	unsigned char buffer[BUF_SIZ];
	char temp[128];
	FILE *fp;

	// file open 
	fp = fopen(fileName,"rb");
	if(fp == NULL) {perror("file open error: "); exit(1); }

	// get file length

	bzero(temp,127);
	fseek(fp,0,SEEK_END);
	file_sz = ftell(fp);
	fseek(fp,0,SEEK_SET);
	sprintf(temp,"%i",file_sz);

	// file size transfer
	retcode = send(sockid,temp,127,0);
	if(retcode < 0) { perror("Err : Can't Send To server"); }


	bzero(buffer,BUF_SIZ);
	signal(SIGALRM,sig_handler);
	alarm(1);
	// data transfer
	do{

		nread = fread(buffer,sizeof(unsigned char), BUF_SIZ,fp);

		if(nread<BUF_SIZ){
			retcode = sendAll(sockid,buffer,nread);
			if(retcode < 0) { perror("Err Can't Send To Server"); }
		}
		else{
			retcode = sendAll(sockid,buffer,BUF_SIZ);
			if(retcode < 0) { perror("Err Can't Send To Server"); }
		}
		tran_sz+=retcode;
	
		bzero(buffer,BUF_SIZ-1);	

		// progress status
		if(pnt_res==1)
		{
			progressStatus(tran_sz,file_sz,fileName,SEND_TYPE);
			pnt_res=0;
		}
	}while(tran_sz<file_sz);
	progressStatus(tran_sz,file_sz,fileName,SEND_TYPE);

	fclose(fp);
	printf("\n");
	signal(SIGALRM,sig_end);
	if(tran_sz>=file_sz)
		return 0;
	else
		return -1;
} 

int main(int argc, char *argv[])
{
   int sockid,newsockid, nread, addrlen,retcode;
   struct sockaddr_in my_addr, client_addr;
   char buffer[128];
   pid_t pid;

   if(argc != 2) {
	   printf("%s port: input port number\n", argv[0]);
       return 0;
	}
				 
	//Server: creating socket
	if((sockid = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
	   printf("Server: socket error: %d\n",errno); exit(0);
	}
					   
	//Server: binding my local socket
	memset((char*)&my_addr,0,sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr =htonl(INADDR_ANY);
	my_addr.sin_port = htons(atoi(argv[1]));

	if((bind(sockid, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0)) { 
	   printf("Server: bind fail: %d\n",errno); 
	   exit(0);
	}

	listen(sockid,5);
										 
    //Server: starting blocking message reada
	printf("Starting server \n");
	while(1)
	{
		printf("Waitting on port#: %s.\n",argv[1]);
		addrlen = sizeof(client_addr);

		// accept() queues the connection / if no pending connections are present block
		newsockid = accept(sockid, (struct sockaddr*)&client_addr, &addrlen);
		if(newsockid < 0)
			fprintf(stderr,"Can't Accept Connection.\n");
		
		printf("New incoming connection, block on receive. \n");


		// fork the connection
		pid = fork();
		if(pid == 0) {

			char token1[15],token2[15];
			bzero(buffer,sizeof(buffer));

			while(1){
				while((nread = recv(newsockid,buffer,sizeof(buffer)-1,0))>0){
					buffer[nread]='\0';
					// parse the string
					sscanf(buffer, "%s %s",token1, token2);

					if(strcmp("put",token1) == 0){
						put(newsockid, token2);
					}
					else if(strcmp("get",token1) == 0){
						get(newsockid, token2);
					}
					else if(strcmp("quit",token1) == 0){
						printf("Kill child %i\n",getpid());

						nread = send(newsockid, buffer, sizeof(buffer)-1,0);
						if(nread < 0)
							fprintf(stderr,"Can't Send To client");
						close(newsockid);
						kill(getpid(),SIGKILL);
					}

					memset(buffer,0,sizeof(buffer));
					memset(token1,0,sizeof(token1));
					memset(token2,0,sizeof(token2));
				}
				if(nread < 0)
					fprintf(stderr,"Can't Receive frome Client\n");
				kill(getpid(),SIGKILL);
			}
		} // end child if
	}// end server

	close(sockid);
	return 0;
}
