#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include <windows.h>
#include <stdbool.h>
#include <math.h>
#include <iostream>
using namespace std;

#define target_fps 59
#define PI 3.14159265
bool SPkeyStates[256] = {false};
bool keyStates[256] = {false};
bool notFirstRun;
double TPF = 1000.0 / target_fps;
int start_time;
int previous_frame_num;
int current_frame_num;
int bulletCount = 0;
int maxArray = 0;

double playerSpeed = 2;
int fireRate = 10; //shots per second
struct player {
    double x;
    double y;
    double speed;
    bool shoot;
    int fire_rate;
    int lastShootFrame;
} player1;

struct Bullet {
    bool alive;
    double x;
    double y;
    double speed;
    double direction;
    int birthFrame;
    int lifeTime; //in frames
    //Bullet *next;
};

Bullet bulletList[9999] = {};

int getEmptyIndex()
{
    int i;
    for (i=0 ; i <= bulletCount ; i++)
    {
        if (!bulletList[i].alive)
            return i;
    }
    if (i>bulletCount) {return -1;}
}

void createBullet(int x, int y, int speed, int dir, int life)
{
    bulletCount++;
    maxArray++;
    int i = getEmptyIndex();
    bulletList[i].alive = true;
    bulletList[i].x = x;
    bulletList[i].y = y;
    bulletList[i].speed = speed;
    bulletList[i].direction = dir;
    bulletList[i].lifeTime = life;
    bulletList[i].birthFrame = current_frame_num;

}

void checkBulletsLife()
{
    int i;
    //system("CLS");
    //printf("\n Bullet Count: %d \n Max Array: %d", bulletCount, maxArray);
    for (i=0 ; i <= maxArray ; i++)
    {
        //printf("\n Array no: %d", i);
        if (bulletList[i].alive)
        {
            //printf("\n Alive");
            int age = current_frame_num - bulletList[i].birthFrame;
            //printf("\n Age: %d", age);
            if (bulletList[i].lifeTime < age)
            {
                bulletList[i].alive = false;
                bulletCount--;
            }
        } else {
            //printf("\n Empty");
        }
    }
}

void drawBullets()
{
    int i;
    for (i=0 ; i <= maxArray ; i++)
    {
        if (bulletList[i].alive)
        {
            glPushMatrix();
            glColor3f(1,0.1,0.1);
            glTranslatef(bulletList[i].x,bulletList[i].y,0);
            glutSolidSphere(1.5,20,16);
            glPopMatrix();

            bulletList[i].x += bulletList[i].speed * sin(bulletList[i].direction*PI/180);
            bulletList[i].y += bulletList[i].speed * cos(bulletList[i].direction*PI/180);
        }
    }
}

void reduceArray()
{
    if (!bulletList[maxArray].alive && maxArray > 0)
    {
        if (maxArray <= bulletCount)
        {
            maxArray = bulletCount;
        } else {
            maxArray--;
        }
    }
}

void drawPlayer()
{
    glTranslatef(player1.x, player1.y, 0);
    glutSolidSphere(5,20,16);
}

void playerShoots()
{
    if (player1.shoot)
    {
        int frame = current_frame_num - player1.lastShootFrame;
        if (frame >= player1.fire_rate)
        {
            createBullet(player1.x, player1.y, 6, 0, 50); // x , y , speed , direction (degree) , life (in frames)
            player1.lastShootFrame = current_frame_num;
            //printf("\n shoot");
        }
    }
}

///////////////////////////////
//    BACKGROUND SECTION     //
///////////////////////////////

void drawBackground(int x1,int y1,int x2,int y2)
{
    int depth = 10;
    glColor3f(0.2,0.2,0.2);
    glBegin(GL_POLYGON);
    glVertex3f(x1, y1, depth);
    glVertex3f(x2, y1, depth);
    glVertex3f(x2, y2, depth);
    glVertex3f(x1, y2, depth);
    glEnd();
    glColor3f(1,1,1);
}


///////////////////////////////
//    KEY HANDLER SECTION    //
///////////////////////////////

void keyboard(void)
{
    if (SPkeyStates[GLUT_KEY_UP])
        player1.y += player1.speed;
    if (SPkeyStates[GLUT_KEY_DOWN])
        player1.y -= player1.speed;
    if (SPkeyStates[GLUT_KEY_RIGHT])
        player1.x += player1.speed;
    if (SPkeyStates[GLUT_KEY_LEFT])
        player1.x -= player1.speed;
    if (keyStates['z'])
    {
        player1.shoot = true;
    } else {
        player1.shoot = false;
    }

}

void keyPressed (unsigned char key, int x, int y)
{
    keyStates[key] = true;
}

void keyRelease (unsigned char key, int x, int y)
{
    keyStates[key] = false;
}

void SPkeyPressed (int key, int x, int y)
{
    SPkeyStates[key] = true;
}

void SPkeyRelease (int key, int x, int y)
{
    SPkeyStates[key] = false;
}

///////////////////////////////
//      DISPLAY SECTION       //
///////////////////////////////

void display(void)
{
    keyboard();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(0, 0, -210);
    glRotatef(-20, 1, 0, 0);

    drawBackground(-100,210,100,-130);

    glPushMatrix();
    drawPlayer();
    glPopMatrix();
    playerShoots();

    checkBulletsLife();
    drawBullets();
    reduceArray();


    glutSwapBuffers();
    glFlush();
}



///////////////////////////////
//      SYSTEM SECTION       //
///////////////////////////////

void frameControl()
{
    double latest_frame_time, latest_rendering_time, waste_time;

    glutPostRedisplay();

    latest_frame_time = start_time + (current_frame_num + 1) * TPF;
    latest_rendering_time = glutGet(GLUT_ELAPSED_TIME);
    waste_time = latest_frame_time - latest_rendering_time;
    if (waste_time > 0.0)
        Sleep(waste_time / 1000.0);
    current_frame_num = current_frame_num + 1;
}

void init()
{
    player1.y = -100;
    player1.speed = playerSpeed;
    player1.fire_rate = 60/fireRate;

    glClearColor( 0.0, 0.0, 0.0, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(75, (GLdouble)900.0/(GLdouble)600.0,0,10);

    start_time = glutGet(GLUT_ELAPSED_TIME);
    current_frame_num = 0;
    glMatrixMode(GL_MODELVIEW);
}

void reshape(int w, int h)
{
    glutReshapeWindow(900,600);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(900,600);
    glutInitWindowPosition(100,100);
    glutCreateWindow(argv[0]);

    glutSpecialFunc(SPkeyPressed);
    glutSpecialUpFunc(SPkeyRelease);
    glutKeyboardFunc(keyPressed);
    glutKeyboardUpFunc(keyRelease);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(frameControl);
    init();
    glutMainLoop();
    return 0;
}


