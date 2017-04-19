
// #include <gl/glut.h>

#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

#include <sys/socket.h>//socket(), connect(), send(), recv()

#include <netinet/in.h>//sockaddr_in and in_addr structure
#include <string.h>//memset()
#include <arpa/inet.h>//for inet_addr()
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/msg.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>

#define PI 3.14159265
#define MAXLINE 1000
#define MAX_CLIENT 100
#define MAX_BULLET 1000

struct Point {
	GLint x;
	GLint y;
};

struct Point p1, p2;

struct bullet{
  int player_id;
  int bullet_id;
  int x,y;
  float u,v;
  int status;
};

struct stats{
	int curr_time;
	int curr_pu;
	int px, py, pscore;
	float pu,pv;
	int gx[100], gy[100],health[100],score[100],gp[100],beatenby[100];
	int g_radius[100],g_status[100];
	int powx[100],powy[100],ptype[100],pow_status[100];
	float gu[100],gv[100];
	struct bullet mybullets[100];
	int which_bullet[100][100];
	int bul_stack[100];
	int stack_ptr;
	int hit_flag[100][100];
	int i_am_dead;
	int pu_claimed;
	int login;
	int bullet_no;
	int player_count;
	int powerup_count;
	int myid;
	int lock;
	int all_bullets[100][100][3];

	char noteBuf[MAXLINE];
	char posBuf[MAXLINE] ;
	char playerName[100][100];
	int lvlup;
	int curr_level;

	char lBuffer[2][1000];
	char warn[1000] ;
	int fExit;
	int sExit;

	//discover
	int dBool ;
	int server_count;
	char server_ip[100];
	char serverNames[100][100];
	char dBuffer[100];
	int Resend;
};

//windowing variables
float window_width = 1024.0, window_height = 768.0;
int lim_x, lim_y, offset = 0;
int d_x = 5, d_y = 6;
float u=3,v=3;
float bFactor = 1; 

//game stats
struct stats* s_a;
int gHit = 0;
float radius2 = 800;
int sqsize = 100;

//discover
int serverSel = 0;
int dChild;
int uChild;
int lChild;


//s_a->login
char lBuffer[2][1000] = {"", ""}; 
int lPtr = 0, lSel = 0;
char warn[1000] = "Input Username, only alphanum";
int lFlag = 1;
int lineHight = 25; // the hight of each line
int lineMargin = 200; // the left margin for your text
int currentHight = 350; // the y position of the cursor.
int userMargin;
int userHight;
int shif = 5;
int try_rem = 3;
int factor = 1;

//initial color values
float linecolor_r=1.0,linecolor_g=0.0,linecolor_b=0.0;
float bg_r=0.0,bg_g=0.0,bg_b=0.0;
float alpha = 1;

//networking
int clientSocket;
int child;
int clientSocket, portNum, nBytes;
struct sockaddr_in serverAddr, ownAddr;
socklen_t addr_size;
int p_no, randPort;

//Stack
int isempty()
{
	return (s_a->stack_ptr==0)?1:0;
}

void push(int a)
{
	s_a->bul_stack[s_a->stack_ptr++] = a;
}

void pop()
{
	if(isempty())
	{
		printf("The stack is empty\n");
		return;
	}
	s_a->stack_ptr--;
}

int top()
{
	return s_a->bul_stack[s_a->stack_ptr-1];
}

//Stack ends

int min(int a,int b)
{
	return a<b?a:b;
}
int max(int a,int b)
{
	return a>b?a:b;
}

GLint FPS = 0;

void FPS1(void) {
  static GLint frameCounter = 0;         // frames averaged over 1000mS
  static GLuint currentClock;             // [milliSeconds]
  static GLuint previousClock = 0; // [milliSeconds]
  static GLuint nextClock = 0;     // [milliSeconds]

  ++frameCounter;
  currentClock = glutGet(GLUT_ELAPSED_TIME); //has limited resolution, so average over 1000mS
  if ( currentClock < nextClock ) return;

  FPS = frameCounter/1; // store the averaged number of frames per second

  previousClock = currentClock;
  nextClock = currentClock+1000; // set the next clock to aim for as 1 second in the future (1000 ms)
  frameCounter=0;
}

void output(int x, int y, float r, float g, float b, int font, char *string)
{
  glColor4f( r, g, b, alpha );
  glRasterPos2f(x, y);
  int len, i;
  len = (int)strlen(string);
  for (i = 0; i < len; i++) {
    glutBitmapCharacter(font, string[i]);
  }
}

void orthogonalStart() 
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0f,window_width,0.0,window_height);
    glMatrixMode(GL_MODELVIEW);
}

void orthogonalEnd()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
GLuint texture;

GLuint LoadTexture( const char * filename, int width, int height )
{
    GLuint texture;
    unsigned char * data;
    FILE * file;

    //The following code will read in our RAW file
    file = fopen( filename, "rb" );
    if ( file == NULL ) return 0;
    data = (unsigned char *)malloc( width * height * 3 );
    fread( data, width * height * 3, 1, file );
    fclose( file );

    glGenTextures( 1, &texture ); 
    glBindTexture( GL_TEXTURE_2D, texture ); 
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE ); 


    //even better quality, but this will do for now.
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR );


    //to the edge of our shape. 
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    //Generate the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,GL_RGB, GL_UNSIGNED_BYTE, data);
    free( data ); //free the texture
    return texture; //return whether it was successful
}

void background()
{
	texture = LoadTexture("grass2.bmp",1024,256);
    glBindTexture( GL_TEXTURE_2D, texture ); 

    orthogonalStart();

    // texture width/height
    const int iw = lim_x;
    const int ih = lim_y;

    glPushMatrix();
    glBegin(GL_QUADS);
        glTexCoord2i(0,0); glVertex2i(0, 0);
        glTexCoord2i(1,0); glVertex2i(iw, 0);
        glTexCoord2i(1,1); glVertex2i(iw, ih);
        glTexCoord2i(0,1); glVertex2i(0, ih);
    glEnd();
    glPopMatrix();

    orthogonalEnd();
}

void DrawCircle(float cx, float cy, float r, int num_segments) {
    glBegin(GL_LINE_LOOP);
    for (int ii = 0; ii < num_segments; ii++)   {
        float theta = 2.0f * 3.1415926f * ((float)ii) / ((float)num_segments);//get the current angle 
        float x = r * cosf(theta);//calculate the x component 
        float y = r * sinf(theta);//calculate the y component 
        glVertex2f(x + cx, y + cy);//output vertex 
    }
    glEnd();
}

int detectCollision2(int lx, int ly, int tx, int ty){

	float t = (lx-tx)*(lx-tx) + (ly-ty)*(ly-ty);
	
	//define global radius2 = radius*radius
	if(t<4650){
		
		// s_a->gFlag[s_a->myid] = -1;
		return 1;
	}
	return 0;
}

int detectCollision(int lx, int ly, int tx, int ty){

	float t = (lx-tx)*(lx-tx) + (ly-ty)*(ly-ty);
	
	//define global radius2 = radius*radius
	if(t<radius2){
		
		// s_a->gFlag[s_a->myid] = -1;
		return 1;
	}
	return 0;
}

// glVertex2i(0, 0);
// 		glVertex2i(lim_x, 0);
// 	    glVertex2i(lim_x,lim_y);
// 		glVertex2i(0,lim_y); 

void draw () 
{ 
	int t,k,i,j;
	glClearColor (0.0,0.0,0.0,1.0);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glEnable( GL_TEXTURE_2D );

    background();
    glDeleteTextures(1,&texture);
    glDisable(GL_TEXTURE_2D);

    //glutSwapBuffers();

	//glClear (GL_COLOR_BUFFER_BIT);  // Clear display window. 
    glColor3f (0.0,0.0,1.0);      // Set line segment color to blue. 
    int t_x = window_width/d_x;
	int t_y = window_height/d_y;
    
    char s[100];
    
    // output(0,460,1,0,0,GLUT_BITMAP_TIMES_ROMAN_24,s);
    int tx, ty, lx, ly;
    float temp_u,temp_v;
    int temp_x,temp_y;
    glColor4f(0.0,0.0,1.0, alpha);
	glLineWidth(5.0);
	
	glBegin(GL_LINE_STRIP);
		glVertex2f(0, t_y*(d_y-1));
		glVertex2f(t_x*(d_x-1), t_y*(d_y-1));
	glEnd();	

	glBegin(GL_LINE_STRIP);
		glVertex2f(t_x*(d_x-1), 0);
		glVertex2f(t_x*(d_x-1), t_y*(d_y-1));
	glEnd();
    for(i=0;i<s_a->player_count;i++)
    {
    	if(s_a->health[i]>0)
    	{
    		for(j=0;j<100;j++)
	    	{
	    			lx = s_a->all_bullets[i][j][0];
		    		ly = s_a->all_bullets[i][j][1];
		    		if(i!=s_a->myid&&(s_a->curr_pu&(1)))
		    		{
		    			if(detectCollision2(lx,ly,s_a->px,s_a->py)==1)
		    			{
		    				s_a->all_bullets[i][j][2] = 0;
			    			s_a->hit_flag[i][j] = 1;
			    			s_a->which_bullet[i][s_a->beatenby[i]++] = j;
		    			}
		    		}
		    		else if(i!=s_a->myid&&detectCollision(lx,ly,s_a->px,s_a->py)==1)
		    		{
		    			if(s_a->hit_flag[i][j]==0)
		    			{
		    				s_a->health[s_a->myid]-=10;
		    				if(s_a->health[s_a->myid]<=0)
		    					s_a->i_am_dead = 1;
			    			s_a->all_bullets[i][j][2] = 0;
			    			s_a->hit_flag[i][j] = 1;
			    			s_a->which_bullet[i][s_a->beatenby[i]++] = j;
		    			}
		    		}
		    		if(s_a->hit_flag[i][j]==0&&s_a->all_bullets[i][j][2]==1)
		    		{
		    			if(lx>0||ly>0)
			    		{
			    			temp_u = 0.25*u;
				    		temp_v = 0.25*v;
				    		glBegin(GL_POLYGON);
					    	glVertex2i(lx+4*temp_u-2*temp_v, ly+4*temp_v+2*temp_u);
					    	glVertex2i(lx-4*temp_u-2*temp_v, ly-4*temp_v+2*temp_u);
					    	glVertex2i(lx-4*temp_u+2*temp_v, ly-4*temp_v-2*temp_u);
					    	glVertex2i(lx+4*temp_u+2*temp_v, ly+4*temp_v-2*temp_u);
					    	glEnd();
			    		} 
		    		}
	    	}
    	}
    }

    for(int i = 0;i<s_a->powerup_count;i++)
    {
    	if(s_a->pow_status[i]==1)
    	{
    		lx = s_a->powx[i];
	    	ly = s_a->powy[i];
	    	temp_u = 3;
	    	temp_v = 3;
	    	tx = lx + 7*temp_u;
			ty = ly + 7*temp_v;
			if(s_a->ptype[i]==4)
			{
				glColor4f (1.0,0.0,0.0,alpha);
				glLineWidth(10.0);
			    glBegin(GL_LINE_STRIP);
			    	glVertex2i(lx-10, ly);
			    	glVertex2i(lx+10, ly);
			    glEnd();
			    glBegin(GL_LINE_STRIP);
			    	glVertex2i(lx, ly-10);
			    	glVertex2i(lx, ly+10);
			    glEnd();
			}
			else if(s_a->ptype[i]==0)
			{
				glColor3f (1.0,1.0,0.0);
				DrawCircle(lx,ly,10,100);
				DrawCircle(lx,ly,15,100);
			}
			else if(s_a->ptype[i]==1)
			{
				glColor3f (1.0,0.0,1.0);
				DrawCircle(lx,ly,10,100);
				DrawCircle(lx,ly,15,100);
			}
			else
			{
				glColor3f (0.0,0.0,1.0);
				glBegin(GL_POLYGON);
		    	glVertex2i(lx+4*temp_u-2*temp_v, ly+4*temp_v+2*temp_u);
		    	glVertex2i(lx-4*temp_u-2*temp_v, ly-4*temp_v+2*temp_u);
		    	glVertex2i(lx-4*temp_u+2*temp_v, ly-4*temp_v-2*temp_u);
		    	glVertex2i(lx+4*temp_u+2*temp_v, ly+4*temp_v-2*temp_u);
		 
		    	glEnd(); 
			}
			// glBegin(GL_POLYGON);
		 //    	glVertex2i(lx+4*temp_u-2*temp_v, ly+4*temp_v+2*temp_u);
		 //    	glVertex2i(lx-4*temp_u-2*temp_v, ly-4*temp_v+2*temp_u);
		 //    	glVertex2i(lx-4*temp_u+2*temp_v, ly-4*temp_v-2*temp_u);
		 //    	glVertex2i(lx+4*temp_u+2*temp_v, ly+4*temp_v-2*temp_u);
		 
		 //    	glEnd(); 
    	}
    }
    glColor4f(1.0,0.0,0.0, alpha);
    //glColor3f(1.0,0.0,0.0);
    for (int i = 0; i < s_a->player_count; i++)
    {
    	if(s_a->health[i]>0)
    	{
    		sprintf(s,"Health: %d",s_a->health[i]);
	    	lx = s_a->gx[i];
	    	ly = s_a->gy[i];
	    	temp_u = s_a->gu[i];
	    	temp_v = s_a->gv[i];
			tx = lx + 7*temp_u;
			ty = ly + 7*temp_v; 

		    glLineWidth(10.0);
		    glBegin(GL_LINE_STRIP);
		    	glVertex2i(lx, ly);
		    	glVertex2i(tx, ty);
		    glEnd();

		    glBegin(GL_POLYGON);
		    	glVertex2i(lx+4*temp_u-2*temp_v, ly+4*temp_v+2*temp_u);
		    	glVertex2i(lx-4*temp_u-2*temp_v, ly-4*temp_v+2*temp_u);
		    	glVertex2i(lx-4*temp_u+2*temp_v, ly-4*temp_v-2*temp_u);
		    	glVertex2i(lx+4*temp_u+2*temp_v, ly+4*temp_v-2*temp_u);
		 
		    glEnd();
		    if((s_a->gp[i]&(1)))
		    	DrawCircle(lx,ly,40,100);

		    output(lx,ly,1,0,0,GLUT_BITMAP_TIMES_ROMAN_10,s);

    	}
    }

    lx = s_a->px, ly = s_a->py;
    for(i=0;i<s_a->powerup_count;i++)
    {
    	tx = s_a->powx[i]; ty = s_a->powy[i];

	    	//sprintf(s_a->noteBuf, "..%f..", t);
	    	// output(lx,ly,1,0,0,GLUT_BITMAP_9_BY_15, );
	    	if(detectCollision(lx, ly, tx, ty)){
				s_a->pu_claimed = i;
				output(lx, ly, 1, 1, 1	, GLUT_BITMAP_9_BY_15, "CollisionHere.");
				//sprintf(s_a->noteBuf, "CollisionHere");
	    	}
    }

    if(s_a->curr_level==3)
    {
    	glColor3f (0.0,0.0,0.0);
	    glBegin(GL_POLYGON);
		    	glVertex2i(0,0);
		    	glVertex2i(s_a->px-sqsize, 0);
		    	glVertex2i(s_a->px-sqsize,lim_y);
		    	glVertex2i(0, lim_y);
		 
		    glEnd(); 
		    glBegin(GL_POLYGON);
		    	glVertex2i(lim_x, lim_y);
		    	glVertex2i(s_a->px+sqsize, lim_y);
		    	glVertex2i(s_a->px+sqsize, 0);
		    	glVertex2i(lim_x, 0);
		 
		    glEnd();
		     glBegin(GL_POLYGON);
		    	glVertex2i(0,0);
		    	glVertex2i(0,s_a->py-sqsize);
		    	glVertex2i(lim_x,s_a->py-sqsize);
		    	glVertex2i(lim_x,0);
		 
		    glEnd();
		    glBegin(GL_POLYGON);
		    	glVertex2i(lim_x, lim_y);
		    	glVertex2i(lim_x,s_a->py+sqsize);
		    	glVertex2i(0,s_a->py+sqsize);
		    	glVertex2i(0, lim_y);
		    glEnd();
    }  
    //glutSwapBuffers(); 
}

void drawNotify(){
	
  
    int x = window_width*(d_x-1)/(d_x);
    int y = window_height/(d_y*2);
    output(x/6.0, y, 1, 1, 1, GLUT_BITMAP_9_BY_15, s_a->noteBuf);
    output(x*5/6.0, y, 1, 1, 1, GLUT_BITMAP_9_BY_15, s_a->posBuf);

    //glutSwapBuffers();
}

void drawList(){


    char s[] = "  ARENA";
    
    int x = window_width/d_x;
    int y = window_height*(d_x-1)/d_x;
    output(x/6, y, 1, 1, 1, GLUT_BITMAP_9_BY_15, s);
    char toprint[20];
    for (int i = 0; i < s_a->player_count; ++i)
    {
    	//printf("Player list: %s\n", s_a->playerName[i]);	
    	y -= 25;
    	sprintf(toprint,"%s : %d",s_a->playerName[i],s_a->score[i]);
		output(x/6, y, 1, 1, 1, GLUT_BITMAP_9_BY_15,toprint);
    }
    //glutSwapBuffers();
}
void *xmalloc (size_t size)
{
    register void *value = malloc (size);

    if (value == 0)
    {
        fprintf (stderr, "xmalloc: virtual memory exhausted");
        exit (1);
    }

    return value;
}

void increaseangle(float angle)
{
	float e1=u,e2=v;
	u = e1*cos(angle)- e2*sin(angle);
	v = e1*sin(angle) + e2*cos(angle);
}

void decreaseangle(float angle)
{
	float e1=u,e2=v;
	u = e1*cos(angle) + e2*sin(angle);
	v = e2*cos(angle) - e1*sin(angle);
}

void scale(float x)
{
	u*=x;
	v*=x;
}

void display();

void caeser_hash(char* buff){
	int l = strlen(buff) ;
	int i ;
	for(i = 0 ; i < l ; i++){
		if('a' <= buff[i] && buff[i] <= 'z'){
			buff[i] = (buff[i] - 'a' + shif)%26 + 'a' ;
		}
		else if('A' <= buff[i] && buff[i] <= 'Z'){
			buff[i] = (buff[i] - 'A' + shif)%26 + 'A' ;		
		}
		else if('0' <= buff[i] && buff[i] <= '9'){
			buff[i] = (buff[i] - '0' + shif)%10 + '0' ;
		}
	}
}

void handleLogin(int portno){

	//imp declaration
	int welcomeSocket;

	//create the network socket

	welcomeSocket= socket(AF_INET, SOCK_STREAM, 0);
	if(welcomeSocket == -1){
		perror("socket") ;
		exit(1) ;
	}

	//Start Login
	int t =0 , n, temp ;
	char username[MAXLINE], password[MAXLINE], sendBuffer[MAXLINE], recvBuffer[MAXLINE];
	if(s_a->login == 1){
		pause();
		//server address
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(portno);
		printf("server selected :%s \n", s_a->server_ip);
		serverAddr.sin_addr.s_addr = inet_addr(s_a->server_ip);
		memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
		addr_size = sizeof(serverAddr);
	}
	else{
		//server address
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(portno);
		// printf("server selected :%s \n", s_a->server_ip);
		serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
		addr_size = sizeof(serverAddr);
		
	}
	
	if(connect(welcomeSocket, (struct sockaddr *)&serverAddr , addr_size ) == -1){
		perror("connect") ;
		exit(1) ;	
	}

	n = recv(welcomeSocket, recvBuffer, MAXLINE, 0);
	printf("Response initial connection from server is %s\n", recvBuffer);
	pause();
	strcpy(username, s_a->lBuffer[0]);
	strcpy(password, s_a->lBuffer[1]);
	printf("I am freeeeeee %s, %s\n", username, password);
	caeser_hash(password);
	sprintf(sendBuffer, "!@%s %s", username, password);
	printf("Sending %s credentials\n", sendBuffer);
	fflush(stdout);
	while(try_rem){


		if(send(welcomeSocket, sendBuffer, strlen(sendBuffer)+1, 0) == -1){
			perror("send") ;
			exit(1) ;
		}
		n = recv(welcomeSocket, recvBuffer, MAXLINE, 0);

		printf("Response from server is %s\n", recvBuffer);
		fflush(stdout);
		if(recvBuffer[0] == '#'){
			sscanf(recvBuffer, "#%d %d", &t, &temp);
			sprintf(s_a->warn, "Welcome new user. Id is %d", t);
			break;
		}
		else if(recvBuffer[0] == '&'){
			sprintf(s_a->warn, "Already logged in. Exiting due to mulitple logins") ;
			s_a->fExit = 1;
			break;
		}
		else if(recvBuffer[0]== 'X'){
			try_rem--;
			sprintf(s_a->warn, "Password doesn't match: %d tries remaining..  please enter password" , try_rem);
			//take user input
			printf("Login calling kill \n");
			fflush(stdout);
			kill(getppid(), SIGINT);
			pause();
			strcpy(password, s_a->lBuffer[1]);
			caeser_hash(password) ;
			printf("resending .. %s\n" , password) ;
			fflush(stdout);
			strcpy(sendBuffer, password);
		}
		else if(recvBuffer[0] == 'Y'){
			sscanf(recvBuffer, "Y%d %d", &t, &temp);
			sprintf(s_a->warn, "Login Successful. Id is %d", t);
			break;
		}

		if(try_rem == 0){
			sprintf(s_a->warn, "3 Attempts Failed...try later.\n");
			s_a->fExit = 1;
			break;
		}
	}

	s_a->sExit = 1;
	printf("Login calling kill \n");
	fflush(stdout);
	kill(getppid(), SIGINT);

	s_a->myid = t;
	s_a->player_count = temp;
	printf("The id assigned to me is %d .\n",s_a->myid);
	// sprintf(sendBuffer, "%d", randPort);

	// if(	send(welcomeSocket, sendBuffer, strlen(sendBuffer)+1, 0) == -1){
	// 	perror("send") ;
	// 	exit(1) ;
	// }

	//take the existing players and update list
	
	int tem;
	int locBuf[MAXLINE];
	for (int i = 0; i < temp; ++i)
	{
		recv(welcomeSocket, recvBuffer, MAXLINE, 0);
		printf("Received: %s\n", recvBuffer);	
		sscanf(recvBuffer, "%% %d %s", &tem, locBuf);
		strcpy(s_a->playerName[tem], locBuf);
		printf(" name of old player: %s\n", s_a->playerName[tem]);
	}
	close(welcomeSocket);

	sprintf(sendBuffer, "!@%d", s_a->myid);
	sendto(clientSocket, sendBuffer, strlen(sendBuffer)+1, 0, (struct sockaddr *)&serverAddr , addr_size);

}

void echoRecieve(int portno){

	char buffer[1024];
	char tosend[30];
	int curr_id,curr_x,curr_y,curr_score,i,j;
	float curr_u,curr_v;
	
	int milliseconds = 20;
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    int new_child;

    if(s_a->login == 1){
		pause();
		//server address
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(portno);
		printf("server selected :%s \n", s_a->server_ip);
		serverAddr.sin_addr.s_addr = inet_addr(s_a->server_ip);
		memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
		addr_size = sizeof(serverAddr);
	}
	else{
		//server address
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(portno);
		// printf("server selected :%s \n", s_a->server_ip);
		serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
		addr_size = sizeof(serverAddr);
		
	}

	while(s_a->login != 0);
	if((new_child = fork())==0)
  	{
  		char tosend[100];
  		while(1)
  		{
  			for(i=0;i<50;i++)
  			{
  				if(s_a->mybullets[i].status==1)
  				{
  					sprintf(tosend,">> %d %d %d %d %f %f %d",s_a->mybullets[i].player_id,s_a->mybullets[i].bullet_id,s_a->mybullets[i].x,s_a->mybullets[i].y,s_a->mybullets[i].u,s_a->mybullets[i].v,s_a->health[s_a->myid]);
	  				sendto(clientSocket,tosend,strlen(tosend)+1,0,(struct sockaddr*)&serverAddr,addr_size);
	  				s_a->mybullets[i].x+=s_a->mybullets[i].u;
	  				s_a->mybullets[i].y+=s_a->mybullets[i].v;
	  				if(s_a->mybullets[i].x>lim_x||s_a->mybullets[i].x<0||s_a->mybullets[i].y>lim_y||s_a->mybullets[i].y<0)
	  				{
	  					s_a->mybullets[i].status = 0;
	  					push(s_a->mybullets[i].bullet_id);
	  					sprintf(tosend,">> %d %d %d %d %f %f %d",s_a->mybullets[i].player_id,s_a->mybullets[i].bullet_id,s_a->mybullets[i].x,s_a->mybullets[i].y,s_a->mybullets[i].u,s_a->mybullets[i].v,s_a->health[s_a->myid]);
	  					sendto(clientSocket,tosend,strlen(tosend)+1,0,(struct sockaddr*)&serverAddr,addr_size);
	  				}
  				}
  			}
  			for(i=0;i<s_a->player_count;i++)
  			{
  				char phoda[50];
  				for(j=0;j<s_a->beatenby[i];j++)
  				{
  					sprintf(phoda,"ph %d %d",i,s_a->which_bullet[i][j]);
  					sendto(clientSocket,phoda,strlen(phoda)+1,0,(struct sockaddr*)&serverAddr,addr_size);
  				}
  				s_a->beatenby[i] = 0;
  			}
  			if(s_a->pu_claimed>=0)
  			{
  				char pu_claim[100];
	  			sprintf(pu_claim,"?? %d %d",s_a->myid,s_a->pu_claimed);
	  			sendto(clientSocket,pu_claim,strlen(pu_claim)+1,0,(struct sockaddr*)&serverAddr,addr_size);
  			}
  			sprintf(tosend,"%d %d %d %f %f %d %d %d",s_a->myid,s_a->px,s_a->py,s_a->pu,s_a->pv,s_a->score[s_a->myid],s_a->curr_pu,s_a->health[s_a->myid]);
  			sendto(clientSocket,tosend,strlen(tosend)+1,0,(struct sockaddr*)&serverAddr,addr_size);
  			// To be decreased later.
  			nanosleep(&ts, NULL);
  		}
  		exit(1);
  	}
    while(1)
    {
      nBytes = recvfrom(clientSocket,buffer,1024,0,NULL, NULL);
      //printf("Received from server: %s\n",buffer);
      int curr_id,curr_x,curr_y,curr_score;
      	float curr_u,curr_v;
      if(buffer[0]=='$'&&buffer[1]=='$')
      {
      	// if((fork()==0))
      	// {
      	// 	s_a->show_flag = 1;
      	// 	sleep(1);
      	// 	s_a->show_flag = 0;
      	// }
      	printf("The init message is %s  .\n",buffer);
      	sscanf(buffer,"$$ %d %d %d %f %f %d %s",&curr_id,&curr_x,&curr_y,&curr_u,&curr_v,&curr_score,tosend);
      	//printf("%d %d %d %f %f",curr_id,curr_x,curr_y,curr_u,curr_v);
      	s_a->player_count++;
      	strcpy(s_a->playerName[curr_id], tosend);
      	s_a->gx[curr_id] = curr_x;
      	s_a->gy[curr_id] = curr_y;
      	s_a->gu[curr_id] = curr_u;
      	s_a->gv[curr_id] = curr_v;
      	s_a->score[curr_id] = 0;
      	s_a->health[curr_id]=99;
      	if(s_a->myid==curr_id)
      	{
      		s_a->px = curr_x;
      		s_a->py = curr_y;
      		s_a->pu = curr_u;
      		s_a->pv = curr_v;
      		s_a->player_count--;
      		printf("Sssshhh.. %d\n", s_a->player_count);	
      		//s_a->ps = curr_score;
      	}
      	printf("New Player entered the arena\n");
      	sprintf(tosend,"%d %d %d %f %f %d %d %d",s_a->myid,s_a->px,s_a->py,s_a->pu,s_a->pv,s_a->score[curr_id],s_a->curr_pu,s_a->health[curr_id]);
      	sendto(clientSocket,tosend,strlen(tosend)+1,0,(struct sockaddr*)&serverAddr,addr_size);
      	sendto(clientSocket,tosend,strlen(tosend)+1,0,(struct sockaddr*)&serverAddr,addr_size);
      }
      else if(buffer[0]=='P'&&buffer[1]=='U')
      {
      	int type,xc,yc,pu_id;
      	printf("Woah man!! A new power just appeared.\n");
      	sscanf(buffer,"PU %d %d %d %d",&type,&xc,&yc,&pu_id);
      	s_a->powerup_count++;
      	s_a->powx[pu_id] = xc;
      	s_a->powy[pu_id] = yc;
      	s_a->ptype[pu_id] = type;
      	s_a->pow_status[pu_id] = 1;
      }
      else if(buffer[0]=='?'&&buffer[1]=='?')
      {
      	int player_id,pow_id;
      	sscanf(buffer,"?? %d %d",&player_id,&pow_id);
      	if(player_id==s_a->myid&&(s_a->pow_status[pow_id]==1))
      	{
      		printf("YAY! I Received a powerup of id %d and type %d\n",pow_id,s_a->ptype[pow_id]);
      		if(s_a->ptype[pow_id]==4)
      		{
      			s_a->health[s_a->myid] = min(99,s_a->health[s_a->myid]+10);
      			sendto(clientSocket,tosend,strlen(tosend)+1,0,(struct sockaddr*)&serverAddr,addr_size);
      			sendto(clientSocket,tosend,strlen(tosend)+1,0,(struct sockaddr*)&serverAddr,addr_size);
      		}
      		else if(fork()==0)
      		{
      			s_a->curr_pu|=(1<<s_a->ptype[pow_id]);
      			sleep(7);
      			s_a->curr_pu&=(~(1<<s_a->ptype[pow_id]));
      			sprintf(tosend,"%d %d %d %f %f %d %d %d",s_a->myid,s_a->px,s_a->py,s_a->pu,s_a->pv,s_a->score[curr_id],s_a->curr_pu,s_a->health[curr_id]);
      			sendto(clientSocket,tosend,strlen(tosend)+1,0,(struct sockaddr*)&serverAddr,addr_size);
      			sendto(clientSocket,tosend,strlen(tosend)+1,0,(struct sockaddr*)&serverAddr,addr_size);
      			exit(1);
      		}
      	}
      	s_a->pow_status[pow_id] = 0;
      	s_a->powx[pow_id] = 0;
      	s_a->powy[pow_id] = 0;
      	s_a->pu_claimed = -1;

      }
      else if(buffer[0]=='>'&&buffer[1]=='>')
      {
      	int bull_id,temp_score;
      	sscanf(buffer,">> %d %d %d %d %f %f %d",&curr_id,&bull_id,&curr_x,&curr_y,&curr_u,&curr_v,&temp_score);
      		//char phoda[50];
      		//sprintf(phoda,"ph %d",curr_id);
      		//sendto(clientSocket,phoda,strlen(phoda)+1,0,(struct sockaddr*)&serverAddr,addr_size);
      		s_a->all_bullets[curr_id][bull_id][0] = curr_x;
	      	s_a->all_bullets[curr_id][bull_id][1] = curr_y;
	      	s_a->all_bullets[curr_id][bull_id][2] = 1;
	      	s_a->health[curr_id] = temp_score;
      	if(curr_x>lim_x||curr_x<0||curr_y>lim_y||curr_y<0)
      	{
      		s_a->all_bullets[curr_id][bull_id][2] = 0;
      		s_a->hit_flag[curr_id][bull_id] = 0;
      	}
      }
      else if(buffer[0]=='p'&&buffer[1]=='h')
      {
      	int pl_id,bulid;
      	sscanf(buffer,"ph %d %d",&pl_id,&bulid);
      	if(pl_id==s_a->myid)
      	{
      		s_a->score[pl_id]+=10;
      		s_a->mybullets[bulid].x = 899;
	  		s_a->mybullets[bulid].y+=699;
      	}
      }
      else if(buffer[0] == '*' && buffer[1] == '*'){
	      	sscanf(buffer, "** %d", &curr_id);
	      	s_a->health[curr_id] = 0;
	      	//s_a->gFlag[curr_id] = -1;
  			sprintf(s_a->noteBuf, "Player Left: %d", curr_id);
	      }
      else if(buffer[0] =='L' && buffer[1]=='V'){
      	s_a->lvlup =1;
      	s_a->curr_level++;
      	sprintf(s_a->noteBuf, "LEVEL UP");
      }
      else
      {
      	int pu_code,curr_health;
      	//printf("%s yeah yo\n",buffer);
      	sscanf(buffer,"%d %d %d %f %f %d %d %d",&curr_id,&curr_x,&curr_y,&curr_u,&curr_v,&curr_score,&pu_code,&curr_health);
      	s_a->gx[curr_id] = curr_x;
      	s_a->gy[curr_id] = curr_y;
      	s_a->gu[curr_id] = curr_u;
      	s_a->gv[curr_id] = curr_v;
      	s_a->score[curr_id] = curr_score;
      	s_a->gp[curr_id] = pu_code;
      	s_a->health[curr_id] = curr_health;
      	if(curr_id==s_a->myid)		
      		sprintf(s_a->posBuf, "%d, %d", curr_x, curr_y);
      }
    }
}

void discover(int portno){

	struct ifaddrs *addrs,*tmp;
	struct sockaddr_in *sin;
	unsigned char *ip;
	char broad[100];

	getifaddrs(&addrs);
	tmp = addrs;

	while (tmp)
	{

		if(tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET){

	        // printf("Interface:%s\n", tmp->ifa_name);
	        // sin = (struct sockaddr_in *)tmp->ifa_addr;
	        // ip = (unsigned char *)&sin->sin_addr.s_addr;
	        // printf("	IP is %d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], ntohs(sin->sin_port));
			if(strcmp(tmp->ifa_name, "lo") == 0)
				;
	        
	        else if(IFF_BROADCAST & tmp->ifa_flags && tmp->ifa_ifu.ifu_broadaddr != NULL){
	        
	        	sin = (struct sockaddr_in *)tmp->ifa_ifu.ifu_broadaddr;
	        	ip = (unsigned char *)&sin->sin_addr.s_addr;	
				printf("%d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], ntohs(sin->sin_port));
				sprintf(broad, "%d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], ntohs(sin->sin_port));
		   		break;
		   	}
		}

	    tmp = tmp->ifa_next;
	}

	freeifaddrs(addrs);

	//////
	struct sockaddr_in serverAddr;
	int optval =1 ;
 	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const void *)&optval , sizeof(int));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(portno);
	serverAddr.sin_addr.s_addr = inet_addr(broad);
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

	char temp[] = "DISCOVER";
	char mesg[1000];
	struct sockaddr_storage pcliaddr;
	socklen_t len = sizeof(pcliaddr);


	// printf("Success send\n");

	int tem = 0;
	while(s_a->dBool){
		sprintf(s_a->dBuffer, "...Discovering...\n");
		
		if(s_a->Resend){
			if(sendto(fd, temp, strlen(temp)+1, 0, (struct sockaddr*)&serverAddr,sizeof(serverAddr)) == -1){
				perror("send");
				exit(0);
			}
			printf("Broadcasting ....................\n");
			s_a->Resend--;
		}
		int n = recvfrom(fd, mesg, sizeof(mesg), 0, (struct sockaddr*)&pcliaddr, &len);
		sin = &pcliaddr;
		unsigned char *ip = (unsigned char *)&sin->sin_addr.s_addr;	
		printf("Receiving from %d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], ntohs(sin->sin_port));
		printf("Receiving : %s\n", mesg);
		sprintf(s_a->serverNames[tem++], "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3] );
		s_a->server_count++;
	}

	close(fd);
}

int lF;
int localF = 1;
void process_Normal_Keys(unsigned char key, int x, int y) 
{
	if(s_a->login == 1){

		switch(key){
			case 'w':
				serverSel = max(0, --serverSel);
				break;
			case 's':
				serverSel = min(s_a->server_count, ++serverSel);
				break;
			case 13:
				if(s_a->server_count>0){
					
					printf("serverSel : %d, %d\n", serverSel, s_a->server_count);
					if(serverSel == s_a->server_count){
						s_a->Resend = 1;
					}
					else{
						s_a->dBool = 0;
						strcpy(s_a->server_ip, s_a->serverNames[serverSel]);
						printf("Copying %s into server ip\n", s_a->serverNames[serverSel]);
						kill(uChild, SIGINT);
						kill(lChild, SIGINT);
						s_a->login = 2;
						alarm(1);
					}
				}
				break;
		}
	}
	else if(s_a->login == 2){

		if (key == 13){ // enter key
	        // userHight -= lineHight; // change cursor y position
			lSel++;
			if(lSel != 2)
				lPtr = 0;
			lF = -1;
		}
		else if(key == 8){
			lPtr = ((lPtr>0)?(--lPtr):0);  
			lF = 0;
		}
	 	else if(isalnum(key)){
	 		s_a->lBuffer[lSel][lPtr++] = key;
	 		lF = 0;
	 	}
	 	else{
	 		lF = 1;
	 	}

 	 	if(lF == 0)
	 		sprintf(s_a->warn, "");
	 	if(lF == -1)
	 		sprintf(s_a->warn, "Enter Password.");
	 	if(lF == 1)
	 		sprintf(s_a->warn, "Unexpected input.");

	 	if(lSel == 2){
	 		alarm(0);
	 		lSel--;
	 		sprintf(s_a->warn, "..Processing..");
	 		displaySelect();
	 		printf("Display calling kill\n");
	 		fflush(stdout);
	 		kill(lChild, SIGINT);
	 		pause();
	 	}
	 	
	}
	else{

		switch (key) 
	    {    case 'w' :
			   {

			   	lim_x = window_width*(d_x-1)/d_x - offset;	
				lim_y = window_height*(d_y-1)/d_y - offset;
			   	if((s_a->curr_pu&(1<<1)))
		   		factor = 2;
			   	else
			   		factor = 1;

			   	if((s_a->curr_pu&(1<<2)))
			   	{
			   		s_a->px = ((int)(s_a->px+(factor*u)+lim_x))%lim_x;
			   		s_a->py = ((int)(s_a->py+(factor*v)+lim_y))%lim_y;
			   	}
			   	else
			   	{
			   		s_a->px= max(50,min(s_a->px+(factor*u),lim_x));
			   		s_a->py=max(50,min(s_a->py+(factor*v),lim_y));
			   	}
			   } 
			   break;
		   case 'a':
		   		{
					increaseangle(PI/15);
			   		//kill(child, SIGINT);	
				} ;  
				break;
			case 'd':
				{
					decreaseangle(PI/15);
			   		//kill(child, SIGINT);
				}
				break;
			case 'l':
				{
					sprintf(s_a->noteBuf, "Shot fired .....");
					int temp_index = top();
					pop();
					s_a->mybullets[temp_index].player_id = s_a->myid; 
					s_a->mybullets[temp_index].x = s_a->px; 
					s_a->mybullets[temp_index].y = s_a->py; 
					s_a->mybullets[temp_index].u = s_a->pu; 
					s_a->mybullets[temp_index].v = s_a->pv; 
					s_a->mybullets[temp_index].bullet_id = temp_index;
					s_a->mybullets[temp_index].status = 1; 
					s_a->bullet_no++;
				}
	    }
	    char tosend[50];
	    s_a->pu = u;
	    s_a->pv = v;
	    // printf("Sending to server loco/fire..\n");
	    sprintf(tosend,"%d %d %d %f %f %d %d %d",s_a->myid,s_a->px,s_a->py,s_a->pu,s_a->pv,s_a->score[s_a->myid],s_a->curr_pu,s_a->health[s_a->myid]);
	    sendto(clientSocket,tosend,strlen(tosend)+1,0,(struct sockaddr*)&serverAddr,addr_size);
	}
    displaySelect();	
}

void init() {
	glClearColor(bg_r, bg_g, bg_b, 0.0);
	glColor3f(linecolor_r,linecolor_g,linecolor_b);
	gluOrtho2D(0.0f, window_height, 0.0f, window_height);

	glPointSize(1.0f);

}


int once = 1;
void display(void) 
{
	// printf("Display %d timeth and %d lvlflag.\n", once, s_a->lvlup);
	glClear (GL_COLOR_BUFFER_BIT);  // Clear display window. 

	int t_x = window_width/d_x;
	int t_y = window_height/d_y;
	
	glViewport (0, 0, window_width, window_height);
	glMatrixMode (GL_PROJECTION);		/* Select The Projection Matrix */
	glLoadIdentity ();							/* Reset The Projection Matrix */
	gluOrtho2D(0.0f, window_width, 0.0f, window_height);

	glMatrixMode (GL_MODELVIEW);			/* Select The Modelview Matrix */
	glLoadIdentity ();								/* Reset The Modelview Matrix */
	glClear (GL_DEPTH_BUFFER_BIT);		/* Clear Depth Buffer */
	
	if(s_a->lvlup){
		glEnable(GL_BLEND); // Enable Blending
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Select The Type Of Blending
		
		// glColor4f(1, 1, 0, alpha); // Color our quad
		// glBegin(GL_QUADS);
		// glVertex2i( window_width*(0.2), window_height*(0.3)); // Bottom Left Of The Texture and Quad
		// glVertex2i( window_width*(0.9), window_height*(0.3)); // Bottom Right Of The Texture and Quad
		// glVertex2i( window_width*(0.8), window_height*(0.8)); // Top Right Of The Texture and Quad
		// glVertex2i( window_width*(0.3), window_height*(0.8)); // Top Left Of The Texture and Quad
		// glEnd();

		alpha = 0.1;
	}
	else{
		alpha = 1;
	}
	

	// glColor4f(0.0,0.0,1.0, alpha);
	// glLineWidth(5.0);
	
	// glBegin(GL_LINE_STRIP);
	// 	glVertex2f(0, t_y*(d_y-1));
	// 	glVertex2f(t_x*(d_x-1), t_y*(d_y-1));
	// glEnd();	

	// glBegin(GL_LINE_STRIP);
	// 	glVertex2f(t_x*(d_x-1), 0);
	// 	glVertex2f(t_x*(d_x-1), t_y*(d_y-1));
	// glEnd();

	//glutSwapBuffers();
		
	for (int i = 0; i < 3; ++i)
	{
		
		if(i == 0){

			// main play area
		// 	glViewport (0, 0, window_width*3/4.0, window_height*4/5.0);
		// 	glMatrixMode (GL_PROJECTION);
		// 	glLoadIdentity ();							/* Reset The Projection Matrix */
		// 	gluOrtho2D(0, window_width*3/4.0, 0, window_height*4/5.0);
		}
		else if(i == 1){

			// notification area
			glViewport (0, t_y*(d_y-1), t_x*(d_x-1), t_y);
			glMatrixMode (GL_PROJECTION);		/* Select The Projection Matrix */
			glLoadIdentity ();							/* Reset The Projection Matrix */
			gluOrtho2D(0, t_x*(d_x-1), 0, t_y);
		}
		else{
	
			//players list, level, score area
			glViewport (t_x*(d_x-1), 0, t_x, window_height);
			glMatrixMode (GL_PROJECTION);
			glLoadIdentity ();							/* Reset The Projection Matrix */
			gluOrtho2D(0, t_x, 0, window_height);
		}		

		glMatrixMode (GL_MODELVIEW);			/* Select The Modelview Matrix */
		glLoadIdentity ();								/* Reset The Modelview Matrix */
		glClear (GL_DEPTH_BUFFER_BIT);		/* Clear Depth Buffer */

		if(i == 0){
			draw();
		}
		else if(i ==1){
			drawNotify();
		}
		else{
			drawList();
		}
	}
	
	if(s_a->lvlup){

		glViewport (0, 0, window_width, window_height);
		glMatrixMode (GL_PROJECTION);		/* Select The Projection Matrix */
		glLoadIdentity ();							/* Reset The Projection Matrix */
		gluOrtho2D(0.0f, window_width, 0.0f, window_height);

		glMatrixMode (GL_MODELVIEW);			/* Select The Modelview Matrix */
		glLoadIdentity ();								/* Reset The Modelview Matrix */
		glClear (GL_DEPTH_BUFFER_BIT);		/* Clear Depth Buffer */


		glDisable(GL_BLEND);
		alpha=1;

		glColor3f(1, 1, 1); // Color our quad
		glBegin(GL_QUADS);
		glVertex2f(window_width*(0.3), window_height*(0.3)); // Bottom Left Of The Texture and Quad
		glVertex2f(window_width*(0.8), window_height*(0.3)); // Bottom Right Of The Texture and Quad
		glVertex2f(window_width*(0.8), window_height*(0.8)); // Top Right Of The Texture and Quad
		glVertex2f(window_width*(0.3), window_height*(0.8)); // Top Left Of The Texture and Quad
		glEnd();

		
		output(window_width*(0.4), window_height*(0.5), 0,0,1, GLUT_BITMAP_TIMES_ROMAN_24, ">>LEVEL UP<<");
		glutSwapBuffers();
		
		sleep(5);
		s_a->lvlup--;
		//display();
	}

	// if(once == 1){
	// 	kill(child, SIGINT);	
	// }
	once++;

	//glutPostRedisplay();
}

void loginDisplay(){

	glClear (GL_COLOR_BUFFER_BIT);  // Clear display window.
	
	glEnable(GL_BLEND); // Enable Blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Select The Type Of Blending


	glColor4f(1, 1, 0, 0.1); // Color our quad
	glBegin(GL_QUADS);
	glVertex2i( window_width*(0.3), window_height*(0.3)); // Bottom Left Of The Texture and Quad
	glVertex2i( window_width*(0.9), window_height*(0.3)); // Bottom Right Of The Texture and Quad
	glVertex2i( window_width*(0.7), window_height*(0.7)); // Top Right Of The Texture and Quad
	glVertex2i( window_width*(0.5), window_height*(0.7)); // Top Left Of The Texture and Quad
	glEnd();

	glDisable(GL_BLEND);

	glColor3f(1, 1, 1); // Color our quad
	glBegin(GL_QUADS);
	glVertex2i( lineMargin - 10, currentHight - lineHight - 10); // Bottom Left Of The Texture and Quad
	glVertex2i( lineMargin + 500, currentHight - lineHight-10); // Bottom Right Of The Texture and Quad
	glVertex2i( lineMargin + 500, currentHight + 20); // Top Right Of The Texture and Quad
	glVertex2i( lineMargin - 10, currentHight + 20); // Top Left Of The Texture and Quad
	glEnd();


	output(lineMargin, currentHight, 1.0, 0.0, 0.0, GLUT_BITMAP_9_BY_15, "USERNAME: ");
	output(lineMargin, currentHight - lineHight, 1.0, 0.0, 0.0, GLUT_BITMAP_9_BY_15, "PASSWORD: ");

	
	s_a->lBuffer[lSel][lPtr] = '\0'; 
	output(userMargin, userHight, 1.0, 0.0, 0.0, GLUT_BITMAP_9_BY_15, s_a->lBuffer[0]);
	output(userMargin, userHight - lineHight, 1.0, 0.0, 0.0, GLUT_BITMAP_9_BY_15, s_a->lBuffer[1]);
	output(userMargin, userHight - 3*lineHight, 1.0, 0.0, 0.0, GLUT_BITMAP_9_BY_15, s_a->warn);

	if(lFlag){
		glColor3f(1, 1, 1);
		lFlag = 0;
	}
	else{
		glColor3f(0, 0, 0);
		lFlag = 1;		
	}
	glBegin(GL_LINE_STRIP);
	glVertex2i(userMargin + 7*lPtr, userHight -lSel*lineHight);
	glVertex2i(userMargin + 7*lPtr, userHight -lSel*lineHight + lineHight/2);
	glEnd();

	//glutSwapBuffers();

}

void hostDisplay(){

	glClear (GL_COLOR_BUFFER_BIT);  // Clear display window.
	
	glEnable(GL_BLEND); // Enable Blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Select The Type Of Blending


	glColor4f(1, 1, 0, 0.1); // Color our quad
	glBegin(GL_QUADS);
	glVertex2i( window_width*(0.3), window_height*(0.3)); // Bottom Left Of The Texture and Quad
	glVertex2i( window_width*(0.9), window_height*(0.3)); // Bottom Right Of The Texture and Quad
	glVertex2i( window_width*(0.7), window_height*(0.7)); // Top Right Of The Texture and Quad
	glVertex2i( window_width*(0.5), window_height*(0.7)); // Top Left Of The Texture and Quad
	glEnd();

	glDisable(GL_BLEND);

	output(lineMargin, window_width*0.6, 1.0, 1.0, 1.0, GLUT_BITMAP_9_BY_15, s_a->dBuffer);

	float bel, top;
	int loc = 1, i;
	for ( i = 0; i < s_a->server_count; ++i)
	{
		loc = 1;
		bel = currentHight - lineHight*(i+1);
		top = currentHight - lineHight*(i);
		if(serverSel == i)
			loc = 0;
		glColor3f(1, 0, loc); // Color our quad
		glBegin(GL_QUADS);
		glVertex2i( lineMargin - 10, bel ); // Bottom Left Of The Texture and Quad
		glVertex2i( lineMargin + 500, bel ); // Bottom Right Of The Texture and Quad
		glVertex2i( lineMargin + 500, top); // Top Right Of The Texture and Quad
		glVertex2i( lineMargin - 10, top); // Top Left Of The Texture and Quad
		glEnd();

		output(lineMargin+50, (bel+top)/2, 1.0, 1.0, 1.0, GLUT_BITMAP_9_BY_15, s_a->serverNames[i]);

	}

		bel = currentHight - lineHight*(i+1) - 20;
		top = currentHight - lineHight*(i) - 20;
	loc =1;
	if(serverSel == i)
		loc = 0;
		
	glColor3f(1, 0, loc); // Color our quad
	glBegin(GL_QUADS);
	glVertex2i( lineMargin +100, bel ); // Bottom Left Of The Texture and Quad
	glVertex2i( lineMargin + 300, bel); // Bottom Right Of The Texture and Quad
	glVertex2i( lineMargin + 300, top); // Top Right Of The Texture and Quad
	glVertex2i( lineMargin + 100, top); // Top Left Of The Texture and Quad
	glEnd();

	glColor3f(1,1,1);
	output(lineMargin+200, (bel+top)/2, 1.0, 1.0, 1.0, GLUT_BITMAP_9_BY_15, "Retry");
	
	glutSwapBuffers();
	// glutPostRedisplay();
	// s_a->login = 2;
	// alarm(1);
	// displaySelect();
}


void displaySelect(){

	if(s_a->login == 1){
		hostDisplay();
	}
	else if(s_a->login == 2)
		loginDisplay();
	else
		display();

	glutSwapBuffers();
	if(s_a->login==0)
		glutPostRedisplay();

}

void resizeWindow(int Width, int Height)
{
  if (Height==0)				/* Prevent A Divide By Zero If The Window Is Too Small */
    Height=1;

  // glViewport(0, 0, Width, Height);		 Reset The Current Viewport And Perspective Transformation 
    // printf("Entering ressssssssssssssssssssssssssize:%d, %d", Width, Height);
    window_width = Width;
    window_height = Height;

     if(s_a->login == 1){
		hostDisplay();
    }
    else if(s_a->login == 2){
    	loginDisplay();
    }
    else{
    	lim_x = window_width*(d_x-1)/d_x - offset;	
		lim_y = window_height*(d_y-1)/d_y - offset;

    	display();
    }
}

void coordinateFunc(int x, int y){

	// if(x < window_width*3/4.0 && y < window_height*4/5.0)
		sprintf(s_a->posBuf, "%d, %d", x, y);
}

void onClose(void){

	printf("Bye");
	char tosend[50];
 	sprintf(tosend,"** %d",s_a->myid);
    sendto(clientSocket,tosend,strlen(tosend)+1,0,(struct sockaddr*)&serverAddr,addr_size);
    kill(child, SIGKILL);
	exit(0);
}

// void handler(int sno){

// 	if(child != 0){

// 		// printf("Recccccccccccccccc\n");
// 		// fflush(stdout);
// 		display();
// 	}
// }

void handler(int sno){

	printf("Called by %d process: %d and %d.\n", lChild, s_a->sExit, s_a->fExit);
	fflush(stdout);

	if(s_a->login == 2){
		if(lChild != 0){

			if(s_a->sExit == 0){

				alarm(1);
				lPtr = 0;
				loginDisplay();
			}
			else{
				if(s_a->fExit){

					loginDisplay();
					sleep(3);
					exit(0);
				}
				else{

					loginDisplay();
					s_a->login =0;
					sleep(2);
					display();
				}
			
			}
		}
	}

}

void handler1(int signo){

	if(s_a->login == 2){
		alarm(1);
		loginDisplay();
	}
	else if(s_a->login == 1){
		hostDisplay();
	}
}


int main(int argc, char **argv) {
	int i;
	if(argc!=2){
    	
    	printf("specify port\n");
    	return 0;
	}

	 signal(SIGINT, handler);
	signal(SIGALRM, handler1);
	srand(time(NULL));
	p_no = atoi(argv[1]);
	

	//shared memory
	int shmid = shmget(IPC_PRIVATE, sizeof(struct stats), S_IRUSR | S_IWUSR );	
	s_a = (struct stats*)shmat(shmid, NULL, 0);
	s_a->lock = 0;
 	s_a->lvlup = 0;
    strcpy(s_a->posBuf, "Start");
    // for (int i = 0; i < MAX_CLIENT; ++i)
    // {
    // 	s_a->gFlag[i] = -1;
    // }

    s_a->myid = 0;
    s_a->sExit = 0;
    s_a->fExit = 0;
    s_a->login = 1;
    strcpy(s_a->warn, "Welcome user. Enter Username.");
    alpha = 1;
    s_a->dBool =1;
    s_a->server_count = 0;
    s_a->Resend = 1;
	userMargin = lineMargin+100;
	userHight = currentHight;
	lim_x = window_width*(d_x-1)/d_x - offset;	
	lim_y = window_height*(d_y-1)/d_y - offset;

	/*****************************************************/
	//udpSocket	  
	clientSocket = socket(PF_INET, SOCK_DGRAM, 0);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(p_no);
	// serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

	for(i=0;i<50;i++)
    {
    	push(i);
    }
    s_a->curr_level = 1;
	
	// make receptor process
	// make receptor process
	uChild = fork();
	if(uChild == 0)
	{
		echoRecieve(p_no);
		exit(1);
	}

	//Start host discovery
	dChild = fork();
	if(dChild == 0){

		discover(p_no);
		printf("Exiting discover....\n");
		exit(1);
	}

	//Now contact the selected server: make handle login process:- this should be used to first contact server and then only take user input 
	lChild = fork();
	if(lChild == 0){
		handleLogin(p_no);
		printf("Login Exiting....\n");
		exit(1);
	}
	
	// make graphics
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
	glutInitWindowPosition(200, 200);
	glutInitWindowSize((int)window_width, (int)window_height);
	glutCreateWindow("NetGame");	
	
	init();

	glutDisplayFunc(displaySelect);
	// glutMouseFunc(myMouseFunc);
	glutReshapeFunc(resizeWindow);
	// glutPassiveMotionFunc(coordinateFunc);
	glutKeyboardFunc(process_Normal_Keys);
	glutCloseFunc(onClose);
	alarm(1);
	// glutIdleFunc(animation);
	glutMainLoop();
}
