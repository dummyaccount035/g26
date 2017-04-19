#include <stdio.h>//printf(),fprintf()
#include <sys/socket.h>//socket(), connect(), send(), recv()

#include <netinet/in.h>//sockaddr_in and in_addr structure
#include <string.h>//memset()
#include <arpa/inet.h>//for inet_addr()f
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <bson.h>
#include <bcon.h>
#include <mongoc.h>


#define MAXLINE 1000
#define MAX_CLIENTS 100

pthread_mutex_t mlock ;
char *base_rank = "pupil" ;
int base_rating = 1300 ;
int health = 100 ;
int iU =3; iV =3;
int logged_in = 1; 
int min_x = 40, max_x = 780;
int min_y= 30, max_y = 530; 
int p_no;
int optval = 1 ;



struct smem{

	mongoc_client_t      *client;
	mongoc_database_t    *database;
	mongoc_collection_t  *collection;


	// int clientPtr;
	// int bufferPtr;
	int lock;
	int udpOver;
	int levels;
	// char bBuffer[MAX_CLIENTS][MAXLINE];
	// struct sockaddr_storage clientAddrs[MAX_CLIENTS];
	// int ids[MAX_CLIENTS];
	int tcpSocket;
	int udpSocket;
	struct sockaddr_storage clientaddresses[MAX_CLIENTS];
	char player_names[MAX_CLIENTS][MAXLINE];
	int gFlag[MAX_CLIENTS]; 
	int no_of_clients;
	int powerups_count;
};
struct smem *a;	
//socket stuff
struct sockaddr_in serverAddr;


int handleLogin(int sockfd, char un[], char passt[]){

	printf("----handling login----\n") ;
	char pass[MAXLINE];
	strcpy(pass, passt);
	mongoc_cursor_t *cursor;
	bson_t *query;
	const bson_t *doc;
	query = BCON_NEW ("username", BCON_UTF8 (un));
	char password[MAXLINE], *str;
	int id, t =0, x, y, ret;
	// id = rand()%1000 + 3242;
	// random spawning 
	x = min_x + rand()%(max_x - min_x);
	y = min_y + rand()%(max_y - min_x);

	//1. Find Username
	//cursor = mongoc_collection_find_with_opts (a->collection, query, NULL, NULL);
	cursor = mongoc_collection_find (a->collection,       /* IN */
												0,            /* IN */
												0,                         /* IN */
												0,                        /* IN */
												0,                   /* IN */
												query,                   /* IN */
												NULL,                  /* IN */
												NULL);

	
	bson_t* prev_doc = NULL ;
	while (mongoc_cursor_next (cursor, &doc)) {
		prev_doc = doc ;
		str = bson_as_json (doc, NULL);
		printf ("str1 = %s\n", str);
		t++;
	}
	doc = prev_doc ;
	printf("t = %d\n" , t) ;

	// 2. If not present make a new entry 
	if(t == 0){

		bson_t *insert;
		bson_error_t  error;
		pthread_mutex_lock(&mlock);

			id = a->no_of_clients++;

			insert = BCON_NEW ("username", BCON_UTF8 (un), 
								"password", BCON_UTF8 (pass),
								"id", BCON_INT32 (id),
								"is_loggedin" , BCON_INT32(logged_in),
								"rank" , BCON_UTF8(base_rank),
								"rating" , BCON_INT32(base_rating),
								"health" , BCON_INT32(health),
								"abrupt_exit" , BCON_INT32(0));    
		if (!mongoc_collection_insert (a->collection, MONGOC_INSERT_NONE, insert, NULL, &error)) {
				fprintf (stderr, "%s\n", error.message);
			}

			bson_destroy (insert);
		pthread_mutex_unlock(&mlock) ;

		char loc[MAXLINE];
		sprintf(loc, "#%d %d", id, a->no_of_clients);
		send(sockfd, loc, strlen(loc)+1, 0);
		return id;

	}
	// 3. If present, check for is_loggedin and password and action as needed.
	else{

		bson_iter_t iter;
		bson_iter_t ptr;
		assert(doc != NULL);
		assert(&iter != NULL );
		if (bson_iter_init (&iter, doc) &&
			bson_iter_find_descendant (&iter, "password", &ptr) &&
			BSON_ITER_HOLDS_UTF8 (&ptr)) {
			
			printf ("server pass = %s\n", bson_iter_utf8 (&ptr, NULL));
			strcpy(password, bson_iter_utf8 (&ptr, NULL));
		}

		int loggedflag = 0 ;
		printf("i am here 1.0\n") ;
		if (bson_iter_init (&iter, doc) &&
			bson_iter_find_descendant (&iter, "is_loggedin", &ptr) &&
			BSON_ITER_HOLDS_INT32 (&ptr)) {
			
			printf ("logcheck server  = %d\n", bson_iter_int32 (&ptr));
			loggedflag = bson_iter_int32(&ptr) ;

		}    
		printf("i am here\n") ;

		int cnt = 0;
		while(strcmp(password, pass)!=0){

			cnt++;
			if(cnt==3){
				
				return -1;
			}
			printf("Compare failed serverpass = %s , recvpass = %s\n" , password, pass);

			char loc[MAXLINE] = "XX";
			memset(pass,'\0' , sizeof(pass)) ;
			send(sockfd, loc, strlen(loc)+1, 0);
			recv(sockfd, pass, sizeof(pass), 0);
			// if(strcmp(password,pass) == 0){
			// 	break ;
			// }
		}
		char loc[MAXLINE];
		if(loggedflag){
			
			loc[0] = '&' ;
			loc[1] = '\0';
			ret = -2;
		}
		else{
			
			id = a->no_of_clients++;
			sprintf(loc, "Y%d %d", id, a->no_of_clients);
			ret =  id;
		}
		printf("sending loc = %s\n" , loc) ;
		send(sockfd, loc, strlen(loc)+1,0);
	}
	return ret;
}

void mRecv(int sockfd, struct sockaddr_storage cli_addr, socklen_t len){

	char msg[MAXLINE], username[20], password[20];
	char sendBuffer[MAXLINE];
	int n, out; 
	struct sockaddr_in *sin;
		
	n = recv(sockfd, msg, MAXLINE, 0);
	printf("Recieved message: %s\n", msg);

	if(n>0){

		//Handle user Authentication
		if(msg[0]=='!' && msg[1]=='@'){

			sscanf(msg, "!@%s %s", username, password);
			printf("1 yeye . user login us:%s and pass:%s ", username, password);
			printf("\nhandling pro\n") ;          
			
			out = handleLogin(sockfd, username, password);
			if(out == -1){
				printf("UserAuthFailed.\n");
			}
			else if(out == -2){
				printf("UserAuthDenied.\n");
			}
			else{
				printf("UserAuthSuccess.\n");
				strcpy(a->player_names[out], username);
				//save the player's address for udp transfers
			
				// sin = &cli_addr;
				// recv(sockfd, msg, MAXLINE, 0);
				// sscanf(msg, "%d", &sin->sin_port);
				// sin->sin_port = htons(sin->sin_port);
				// printf("Augmenting port to %d %s\n", sin->sin_port, msg);
				// a->gFlag[out] = 1;
				// a->clientaddresses[out] = cli_addr;

				// unsigned char *ip = (unsigned char *)&sin->sin_addr.addr;	
				// printf("Adding %d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], ntohs(sin->sin_port));

				
				//send a list of current players
				
				for (int i = 0; i < a->no_of_clients; ++i)
				{	
					sprintf(sendBuffer, "%% %d %s", i, a->player_names[i]);
					printf("Sending: %s\n", sendBuffer);
					send(sockfd, sendBuffer, strlen(sendBuffer)+1, 0);
				}
			}

		}
		// Handle user leave-out	
		else{
			;
		}
	}
	close(sockfd);
	

}

void logProc(){
 
	mongoc_init ();
	a->client = mongoc_client_new ("mongodb://localhost:27017");
	a->database = mongoc_client_get_database (a->client, "game");
	a->collection = mongoc_client_get_collection (a->client, "game", "default");


	//create the tcp socket
	a->tcpSocket= socket(PF_INET, SOCK_STREAM, 0);
	setsockopt(a->tcpSocket, SOL_SOCKET, SO_REUSEPORT, (const void *)&optval , sizeof(int));


	//bind the address struct to the socket
	if(bind(a->tcpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr))){
		printf("Bind Error at Login.\n");
		exit(0) ;
	}
	printf("Binding at Login Successful.\n");
	listen(a->tcpSocket, 10) ;
	printf("Listening....\n");
	struct sockaddr_storage cli_addr ;
	socklen_t cli_len ;
	int newsockfd ;
	
	while(1){
		cli_len = sizeof(cli_addr) ;
		newsockfd = accept(a->tcpSocket, (struct sockaddr *) &cli_addr , &cli_len);
		if(newsockfd < 0){
			printf("accept error\n") ;
			exit(0); 
		}
		if(fork() == 0){
			close(a->tcpSocket) ;
			char mssg[MAXLINE] ;
			strcpy(mssg, "connection established") ;
			send(newsockfd , mssg , strlen(mssg) + 1 , 0) ;
			mRecv(newsockfd, cli_addr, cli_len) ;
			close(newsockfd) ;
			break ;
		}
		else{
			close(newsockfd) ;
		}
	}

	exit(0);
	mongoc_collection_destroy (a->collection);
	mongoc_database_destroy (a->database);
	mongoc_client_destroy (a->client);
	mongoc_cleanup ();

}

int power_x[5] = {400,250,500,300,600};
int power_y[5] = {300,250,343,355,200};

void handleEcho(int sockfd)
{
	int n,i, id;
	struct sockaddr_storage pcliaddr;
	socklen_t len = sizeof(pcliaddr);
	char mesg[MAXLINE];
	char mymsg[100];
	struct sockaddr_in *sin;

	if(fork()==0)
	{
		srand(time(NULL));
	    while(1)
	    {
	    	sleep(10);
	    	int xc=rand()%700+1,yc=rand()%500+1;
	    	int p_to_send = (rand()%(3-(a->levels)+1));
	    	if(a->levels==3)
	    	{
	    		p_to_send = 0;
	    	}
	    	else if(a->levels==2)
	    	{
	    		p_to_send = rand()%5;
	    	}
	    	else
	    	{
	    		p_to_send = rand()%5;
	    	}
		      sprintf(mesg,"PU %d %d %d %d",p_to_send,xc,yc,a->powerups_count);
		      a->powerups_count++;
		      for(int j=0;j<a->no_of_clients;j++)
		      {
		      	sendto(sockfd,mesg,strlen(mesg)+1,0,(struct sockaddr*)&a->clientaddresses[j],len);
		        sendto(sockfd,mesg,strlen(mesg)+1,0,(struct sockaddr*)&a->clientaddresses[j],len);
		      }
	    }
	    return;
	}

	for ( ; ; ) {

		n = recvfrom(sockfd, mesg, MAXLINE, 0, (struct sockaddr*)&pcliaddr, &len);
		sin = &pcliaddr;
		unsigned char *ip = (unsigned char *)&sin->sin_addr.s_addr;	
		// printf("Receiving from %d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], ntohs(sin->sin_port));
		// printf("Receiving : %s\n", mesg);
		// New Player arrived
		if(mesg[0]=='!'&&mesg[1]=='@')
		{
			sscanf(mesg, "!@%d", &id);
			a->gFlag[id] = 1;
			a->clientaddresses[id] = pcliaddr;
			int xc=rand()%700+1,yc=rand()%500+1;
			//Do continuous sending here later
			sprintf(mymsg,"$$ %d %d %d 3.0 3.0 100 %s", id,xc,yc, a->player_names[id]);

			for(i=0;i<a->no_of_clients;i++)
			{
				if(a->gFlag[i]!=-1)
				sendto(sockfd,mymsg,strlen(mymsg)+1,0,(struct sockaddr*)&(a->clientaddresses[i]),len);
				//sendto(sockfd,mesg,strlen(mymsg),0,(struct sockaddr*)&clientaddresses[i],len);
			}
		}
		else if(mesg[0]=='*' && mesg[1] == '*'){

			int tid;
			sscanf(mesg, "** %d", &tid);
			a->gFlag[tid] = -1;
			printf("Player %d left/died the game.\n", tid);
			for(i=0;i<a->no_of_clients;i++)
			{
				if(a->gFlag[i]!=-1)
				sendto(sockfd, mesg, strlen(mymsg)+1, 0, (struct sockaddr*)&(a->clientaddresses[i]), len);
				//sendto(sockfd,mesg,strlen(mymsg)+1,0,(struct sockaddr*)&clientaddresses[i],len);
			}
		}
		else if(mesg[0]=='D' && mesg[1]=='I'){

			sprintf(mesg, "COOL");
			sendto(sockfd, mesg, strlen(mesg)+1, 0, (struct sockaddr*)&pcliaddr, sizeof(pcliaddr));
		}
		else
		{
			for(i=0;i<a->no_of_clients;i++)
			{
				if(a->gFlag[i]!=-1)
				sendto(sockfd,mesg,n,0,(struct sockaddr*)&(a->clientaddresses[i]),len);
			}
		}
	}
}

void echoProc(){


	// create udp socket
	a->udpSocket= socket(PF_INET, SOCK_DGRAM, 0);
	setsockopt(a->udpSocket, SOL_SOCKET, SO_REUSEPORT, (const void *)&optval , sizeof(int));

	//bind the address struct to the socket
	if(bind(a->udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr))){
		printf("Bind Error at Echo.\n");
		exit(0) ;
	}

	printf("Binding at Echo Successful.\n");

	//release udpOver lock
	a->udpOver = 0;
	handleEcho(a->udpSocket);

}

void handler1(int signo){

	int stat;
	pid_t pid;
	while((pid = waitpid(-1, &stat, WNOHANG))>0)
	;
}
void levelProcess(){

	while(a->no_of_clients==0);
	printf("Sending ALARM TO PARENT\n");
	kill(getppid(), SIGALRM);
}

void handler(int signo){

	char text[] = "LVL UP";
	alarm(40);
	if(a->levels!=3){
		printf("Sending LVELLLLLLLLLLLLLLLLLLLL\n");
		sendto(a->udpSocket, text, strlen(text), 0,(struct sockaddr *) &serverAddr, sizeof(serverAddr));
	}
	a->levels--;
	if(a->levels == 0)
		alarm(0);
}

int main(int argc, char** argv)
{

	if(argc!=2){
		printf("specify port\n");
		return 0;
	}
	srand(time(NULL));
	signal(SIGCHLD, handler1);
	signal(SIGALRM, handler);
	
	p_no = atoi(argv[1]); 
	
	//creating shared memory for broadcastBuffer and ...
	int shmid = shmget(IPC_PRIVATE, sizeof(struct smem), S_IRUSR | S_IWUSR );
	a = (struct smem*)shmat(shmid, NULL, 0);

	a->no_of_clients = 0;
	a->lock = 0;
	a->udpOver = 1;
	a->levels = 3;
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		a->gFlag[i] = -1;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(p_no);
	// serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

	//fork the level up process
	if(fork() == 0){
		levelProcess();
		exit(0);
	}
	//separate the login and echo processes
	int child = fork();
	if(child){

		echoProc();
	}
	else{
		while(a->udpOver);
		logProc();
	}
	return 0;
}
