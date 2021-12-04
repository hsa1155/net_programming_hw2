#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

int sockfd;
int fds[100];
int size =100 ;
short PORT = 6969;
typedef struct sockaddr SA;
char* account[105];
int account_check(char buf[])//check if account exist if yes ret1
{
	FILE *fp;
	char tmp1[100];
	fp=fopen("accounts","r");
	while(fscanf(fp,"%s",tmp1)!=EOF)
	{
		if(strcmp(tmp1,buf)==0)return 1;
	}
	return 0;
}
int pwd_check(char buf[])//check if pwd is right
{
	FILE *fp;
	char tmp1[100];
	fp=fopen("passwd","r");
	while(fscanf(fp,"%s",tmp1)!=EOF)
	{
		if(strcmp(tmp1,buf)==0)return 1;
	}
	return 0;
}
void init(){
	sockfd = socket(PF_INET,SOCK_STREAM,0);
	if (sockfd == -1){
		perror("Create socket Failed");
		exit(-1);
	}
	struct sockaddr_in addr;
	addr.sin_family = PF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
    int reuseAddrOpt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseAddrOpt, sizeof(reuseAddrOpt));

	if (bind(sockfd,(SA*)&addr,sizeof(addr)) == -1){
		perror("Bind Failed");
		exit(-1);
	}
	if (listen(sockfd,100) == -1){
		perror("Listen Failed");
		exit(-1);
	}

	for(int i=0; i<105; i++)
		account[i] = (char *)malloc(sizeof(char)*11);
}

void ls(int fd){

	char bar[100]="\n--------------------\n";
	//printf("%d ls.\n", fd);
	send(fd,bar,strlen(bar),0);
	send(fd,"[online member]\n",strlen("[online member]\n"),0);
	for (int i = 0;i < size;i++){
		if (fds[i] != 0){
			char buf[100] = {};
			if(fds[i]!=fd){
				sprintf(buf, "user: [%s] fd:%d\n" ,account[fds[i]] ,fds[i]);
				send(fd,buf,strlen(buf),0);
			}
		}
	}
	send(fd,bar,strlen(bar),0);
	char buf[100] = {};
	strcpy(buf,"enter @fd to choose opponent\n");
	send(fd,buf,strlen(buf),0);
}


void* service_thread(void* p){
	int fd = *(int*)p,z;
	char *ptr,tmp[1000];
	printf("pthread = %d\n",fd);

	char buf[1000] = {};
	/*!LOGIN*/
	while(1)
    {
		send(fd ,"account",strlen("account"),0);
        memset(buf,0,sizeof(buf));
		recv(fd,buf,sizeof(buf),0);//收msg
		//printf("buf=%s len=%ld\n",buf,strlen(buf));
        if(account_check(buf)==1)
        {
            char name[1024];
            strcpy(name,buf);
            //printf("account ok account=%s\n",buf);
            strcpy(tmp,buf);
            send(fd ,"pwd",strlen("pwd"),0);
            memset(buf,0,sizeof(buf));
            recv(fd,buf,sizeof(buf),0);
            sprintf(tmp,"%s:#%s",tmp,buf);
            //printf("%s\n",tmp);
            if(pwd_check(tmp)==1)
            {
                send(fd,"login",strlen("login"),0);
                printf("fd %d login\n",fd);
                strcpy(account[fd],name);
                break;
            }
            else
            {
                printf("pwd_err\n");
                send(fd,"pwd_err",strlen("pwd_err"),0);
                recv(fd,buf,sizeof(buf),0);
                continue;

            }
        }
        else
        {
            send(fd,"account_err",strlen("account_err"),0);
            recv(fd,buf,sizeof(buf),0);
            continue;
        }	
	}
	//end of login
    while(1){
		char buf2[1000] = {};

		if (recv(fd,buf2,sizeof(buf2),0) <= 0){
			// 對方關閉了
			int i;
			for (i = 0;i < size;i++){
				if (fd == fds[i]){
					fds[i] = 0; //設為這個fd不存在了
					break;
				}
			}
			printf("退出：fd = %d quit\n",fd);
			pthread_exit((void*)i);
		}

		if (strcmp(buf2,"ls") == 0){
			ls(fd);
		}
		else if(buf2[0]=='@')//challenge @fd
		{
			int oppofd=atoi(&buf2[1]);
            char match[1024];
            sprintf(match,"CONNECT %s %d",account[fd],fd);//to oppo
			send(oppofd,match,strlen(match),0);
			send(fd,"wait for response\n\0",strlen("wait for respose\n\0"),0);//to me
		}
		else if(strncmp(buf2,"AGREE ",6)==0)//agree challenge AGREE %d
		{
			int oppofd=atoi(&buf2[6]);
            char agree[1024];
            sprintf(agree,"AGREE %s %d",account[fd],fd);
			send(oppofd,agree,strlen(agree),0);
		}
        else if(strncmp(buf2,"DENY ",5)==0)//deny challenge DENY %d
		{
			int oppofd=atoi(&buf2[5]);
            char deny[1024];
            sprintf(deny,"DENY %s %d",account[fd],fd);
			send(oppofd,deny,strlen(deny),0);
		}
		else if(buf2[0]=='#')
		{
            char chess[1024];
            char useless;
            int n,oppofd;
            sscanf(buf2,"%c %d %d",&useless,&n,&oppofd);
            sprintf(chess,"#%d",n);
            send(oppofd,chess,strlen(chess),0);
		}
		else{
			printf("undifined input %s\n",buf2);
		}
        
		memset(buf2,0,sizeof(buf2));
	}
}


int main(){
	init();
	printf("Server Initial Successful!\n");
	while(1){
		struct sockaddr_in fromaddr;
		socklen_t len = sizeof(fromaddr);
		int fd = accept(sockfd,(SA*)&fromaddr,&len);
		if (fd == -1){
			printf("Client occurr Error ...\n");
			continue;
		}
		int i = 0;
		for (i = 0;i < size;i++){
			if (fds[i] == 0){
				fds[i] = fd;
				pthread_t tid;
				pthread_create(&tid,0,service_thread,&fd);
				break;
			}
			if (size == i){
				char* str = "Sorry, the room is full!";
				send(fd,str,strlen(str),0);
				close(fd);
			}
		}
	}
}
