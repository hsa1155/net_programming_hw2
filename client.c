
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

int sockfd;
int filefd,G=0,IsMe=0,challenging=0,invited=0;

short PORT = 6969;
typedef struct sockaddr sockaddr;
char name[30];
char passwd[30];
int oppofd;
char opponame[100],table[9],le1='O',le2='X';
void init(){
	sockfd = socket(PF_INET,SOCK_STREAM,0);
	struct sockaddr_in addr;
	addr.sin_family = PF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr=htonl(INADDR_ANY);

	if (connect(sockfd,(sockaddr*)&addr,sizeof(addr)) == -1){
		perror("無法連接");
		exit(-1);
	}
	printf("Client start Success\n");
}

int iswin(char le)
{
	if(table[0]==le&&table[1]==le&&table[2]==le)return 1;
	if(table[3]==le&&table[4]==le&&table[5]==le)return 1;
	if(table[6]==le&&table[7]==le&&table[8]==le)return 1;
	if(table[0]==le&&table[3]==le&&table[6]==le)return 1;
	if(table[1]==le&&table[4]==le&&table[7]==le)return 1;
	if(table[2]==le&&table[5]==le&&table[8]==le)return 1;
	if(table[0]==le&&table[4]==le&&table[8]==le)return 1;
	if(table[2]==le&&table[4]==le&&table[6]==le)return 1;
	return 0;
}
void print()
{
	printf("%c|%c|%c\n",table[0],table[1],table[2]);
	printf("------\n");
	printf("%c|%c|%c\n",table[3],table[4],table[5]);
	printf("------\n");
	printf("%c|%c|%c\n",table[6],table[7],table[8]);
}
int istie()
{
	for(int i=0;i<9;i++)
	{
		if(table[i]==' ')return 0;
	}
	return 1;
}
void start(){

	char buf2[1000] = {};
    //login
	while(1){
        memset(buf2,0,sizeof(buf2));
		recv(sockfd,buf2,sizeof(buf2),0);
        //printf("receive:%s\n",buf2);
		if (strcmp(buf2,"account") == 0){
            printf("please input account\n");
            scanf("%s",name);
			send(sockfd,name,strlen(name),0);
            
			continue;
		}
        else if (strcmp(buf2,"pwd") == 0){
            printf("please input password\n");
            scanf("%s",passwd);
			send(sockfd,passwd,strlen(passwd),0);
			continue;
		}
        else if (strcmp(buf2,"account_err") == 0){
            printf("account not exist. try again\n");
            send(sockfd,"ok",strlen("ok"),0);
            continue;
		}
        else if (strcmp(buf2,"pwd_err") == 0){
            printf("password error. try again\n");
            send(sockfd,"ok",strlen("ok"),0);
            continue;
		}
        else if (strcmp(buf2,"login") == 0){
            printf("login success\n");
            break;
		}
	}
    memset(buf2,0,sizeof(buf2));
	//end of login
    pthread_t id;
	void* recv_thread(void*);
	pthread_create(&id,0,recv_thread,0);
	//int f=1;
	while(1){
		char buf[1024] = {};
		fgets(buf, sizeof(buf), stdin);
		buf[strlen(buf)-1]='\0';
        //printf("len=%ld buf= %s\n",strlen(buf),buf);
		char msg[131] = {};
		if (strcmp(buf,"logout") == 0)
        {
            if(G==1)
            {
                printf("you are gaming.you cant afk\n");
                continue;
            }
			send(sockfd,"logout",strlen("logout"),0);
			break;
		}
		if (strcmp(buf,"ls") == 0)//list other player
        {
			send(sockfd,"ls",strlen("ls"),0);
		}
		else if(buf[0]=='@')//challenge fd
		{
            if(G==1)
            {
                printf("you are gaming.you cant challenge others\n");
                continue;
            }
            if(challenging==1)
            {
                printf("you are already asking someone.you cant challenge again.\n");
                continue;
            }
            if(invited==1)
            {
                printf("you are being challenged.answer him first\n");
                continue;
            }
            challenging=1;
			send(sockfd,buf,strlen(buf),0);
		}
		else if(buf[0]=='#')//play chess
		{
			if(G==0)printf("you are not in a game\n");
			else if(IsMe==0)printf("not your turn.Please wait your opponent.\n");
			else{
				int n=atoi(&buf[1]);//location of play
				if(table[n]!=' '||n>9||n<0)
				{
					printf("Please enter another number.#(0-8)\n");
				}
				else
				{
					table[n]=le1;
					printf("----------\n");
					print();
					if(iswin(le1))
					{
						printf("You win!\n\n");
						G=0;
					}
					else if(istie())
					{
						printf("Tie!\n\n");
						G=0;
					}
					else printf("\nWait for your opponent....\n");
					IsMe=0;
					sprintf(msg,"#%d %d",n,oppofd);
					send(sockfd,msg,strlen(msg),0);
				}
			}
		}
		else if(strcmp(buf,"yes")==0)
		{
            if(G==1) 
            {
                printf("you are already gaming\nenter#0-8 or type help\n");
                continue;
            }
            if(invited!=1)
            {
                printf("you are not being invited\n");
                continue;
            }
            invited=0;
			printf("connected with your opponent\n");
			sprintf(buf,"AGREE %d",oppofd);
			send(sockfd,buf,strlen(buf),0);
			printf("Game Start!\n");
			int i;
			for(i=0;i<9;i++)table[i]=' ';
			le1='O';le2='X';
			G=1;
			IsMe=1;
            print();
            printf("enter #0~8 to play chess\n");
		}
        else if(strcmp(buf,"no")==0)
        {
            if(G==1) 
            {
                printf("you are already gaming\nenter#0-8 or type help\n");
                continue;
            }
            if(invited!=1)
            {
                printf("you are not being invited\n");
                continue;
            }
            printf("you denied his challenge.\n");
            sprintf(buf,"DENY %d",oppofd);
            send(sockfd,buf,strlen(buf),0);;
            invited=0;
            G=0;
            
        }
		else if(buf[0]=='#')
		{
            printf("else # why happen?\n");
			table[atoi(&buf[1])]=le2;
			if(iswin(le2))
			{
				printf("You lose!\n\n\n\n");
				G=0;
			}
			else if(istie())
			{
				printf("Tie!\n\n\n\n");
				G=0;
			}
			IsMe=1;;
		}
		else if(strcmp(buf,"print")==0)
		{
			if(G==1)print();
            else printf("you are not in a game\n");
		}
        else if(strcmp(buf,"help")==0)
        {
            printf("enter ls to list all online player\n");
            printf("enter @(fd) to challenge a player\n");
            printf("enter logout to logout\n");
            printf("enter print to print table\n");
            printf("when in a game.type #0-8 to play chess.\n");
            printf("when challenged.type yes to agree or no to deny.\n");
        }
		else
        {
            printf("nothing happened.type again or type help to learn instruction\n");
        }
	}
	close(sockfd);
}

void* recv_thread(void* p){
	char *ptr,*qtr;
	while(1){
		char buf[1000] = {};
		if (recv(sockfd,buf,sizeof(buf),0) <= 0){
			return -1;
		}
        buf[strlen(buf)]='\0';
		else if(strncmp(buf,"CONNECT ",8)==0)//you are invited
		{
            char opponame[1024],useless[30];
            char oldpponame[1024],oldfd=oppofd;
            sscanf(buf,"%s %s %d",useless,opponame,&oppofd);
            if(invited==1||G==1||challenging==1)//already being busy auto deny
            {
            sprintf(buf,"DENY %d",oppofd);
            send(sockfd,buf,strlen(buf),0);;
            oppofd=oldfd;
            continue;
            }
            invited=1;
            printf("Do you agree to start a new game with [%s]?\ntype yes to agree no to deny\n",opponame);
		}
        else if(strncmp(buf,"DENY ",5)==0)//deny %s %d and you challenge someone he ret deny
		{
            printf("your challenge was denied\n");
            G=0;
            challenging=0;
            char opponame[1024],useless[30];
            sscanf(buf,"%s %s %d",useless,opponame,&oppofd);
            printf("[%s] denied to play with you\n",opponame);
		}
		else if(strncmp(buf,"AGREE ",6)==0)//AGREE %s %d and you challenge someone he ret agree
		{
            challenging=0;
            char opponame[1024],useless[30];
            sscanf(buf,"%s %s %d",useless,opponame,&oppofd);
            printf("[%s] agree to play with you\n",opponame);
            printf("Game Start\n");
            for(int i=0;i<9;i++)table[i]=' ';
            IsMe=0;
            G=1;
            le1='X';
            le2='O';
            print();
            printf("wait for your opponent\n");
		}
		else if(buf[0]=='#')
		{
			table[atoi(&buf[1])]=le2;
			if(iswin(le2))
			{
				printf("You lost!\n\n");
				G=0;
			}
			else if(istie())
			{
				printf("Tie!\n\n");
                G=0;
			}
			else{
				IsMe=1;
				print();
				printf("Please enter #(0-8)\n");
			}
		}
		else{
			printf("%s\n",buf);
		}
	}
}

int main(){
	init();
	start();
	return 0;
}
