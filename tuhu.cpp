#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include <windows.h>
#include <stdbool.h>
#include <math.h>
#include <iostream>
using namespace std;

#define target_fps 60
#define PI 3.14159265
bool SPkeyStates[256] = {false};
bool keyStates[256] = {false};
double TPF = 1000.0 / target_fps;
int start_time;
int current_frame_num;
int bulletCount = 0;

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


}

void checkBulletsLife()
{

}

void drawBullets()
{

}

/*class bulletList
{
    private:
        Bullet *head, *tail;
    public:
        bulletList()
        {
            head = NULL;
            tail = NULL;
        }
        void createBullet(int x, int y, int speed, int dir, int life)
        {
            printf("test");
            bulletCount++;
            Bullet *temp = new Bullet;
            temp->x = x;
            temp->y = y;
            temp->speed = speed;
            temp->direction = dir;
            temp->lifeTime = life;
            temp->birthFrame = current_frame_num;
            temp->next = NULL;

            if (head == NULL)
            {
                head = temp;
                tail = temp;
                temp = NULL;
            } else {
                tail->next = temp;
                tail = temp;
            }
            printf("%d",bulletCount);
        }
        void drawBullets()
        {
            Bullet *temp = new Bullet;
            temp = head;
            while (temp != NULL)
            {
                printf("draw");
                glPushMatrix();
                glTranslatef(temp->x,temp->y, 0);
                glutSolidSphere(1.5,20,16);
                glPopMatrix();

                temp->x += temp->speed * sin(temp->direction*PI/180);
                temp->y += temp->speed * cos(temp->direction*PI/180);

                temp = temp->next;
            }
        }
        void checkBulletsLife()
        {
            Bullet *temp = new Bullet;
            Bullet *del = new Bullet;
            Bullet *previous = new Bullet;
            temp = head;
            for (int i=1 ; temp!=NULL ; i++)
            {
                int age = current_frame_num - temp->birthFrame;
                if (temp->lifeTime > age)
                {
                    if (i == 1)
                    {
                        head = head->next;
                        del = temp;
                        temp = head;
                        delete del;
                        i--;
                        bulletCount--;
                    } else if (i == bulletCount)
                    {
                        tail = previous;
                        previous->next = NULL;
                        del = temp;
                        temp = tail;
                        delete del;
                        i--;
                        bulletCount--;
                    } else
                    {
                        previous->next = temp->next;
                        del = temp;
                        temp = temp->next;
                        delete del;
                        i--;
                        bulletCount--;
                    }
                } else {
                    previous = temp;
                    temp = temp->next;
                }
            }
        }
};
bulletList onscreenBullets;
*/


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
            createBullet(player1.x, player1.y, 5, 0, 120);
            player1.lastShootFrame = current_frame_num;
        }
    }
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

    glPushMatrix();
    drawPlayer();
    glPopMatrix();
    playerShoots();

    checkBulletsLife();
    drawBullets();


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
    player1.speed = playerSpeed;
    player1.fire_rate = 60/fireRate;

    glClearColor( 0.0, 0.0, 0.0, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    start_time = glutGet(GLUT_ELAPSED_TIME);
    current_frame_num = 0;
}

void reshape(int w, int h)
{
    glViewport(0,0, (GLsizei) w, (GLsizei) h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w<=h)
        glOrtho(-100.0,100.0,
                -100.0*(GLfloat)h/(GLfloat)w,
                100.0*(GLfloat)h/(GLfloat)w,
                -100.0,100.0);
    else
        glOrtho(-100.0*(GLfloat)w/(GLfloat)h,
                100.0*(GLfloat)w/(GLfloat)h,
                -100.0,100.0,
                -100.0,100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800,600);
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

