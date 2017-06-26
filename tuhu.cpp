#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include <windows.h>
#include <stdbool.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include "headers/json/json.h"
using namespace std;

#define target_fps 100
#define PI 3.14159265
ifstream npcfile;
ifstream pathfile;
ifstream stagefile;

Json::Reader reader;
Json::Value obj;
bool shiftpress;
bool SPkeyStates[256] = {false};
bool keyStates[256] = {false};
bool notFirstRun;
double TPF = 1000.0 / target_fps;
int start_time;
int previous_frame_num;
int current_frame_num;
int bulletCount = 0;
int maxArray = 0;
int currentStage;
int stagecount;

int previous_second = 0;
int previous_frame_count = 0;
int current_fps = 0;


double playerSpeed = 3;
int fireRate = 13; //shots per second
double halfspeed = playerSpeed/2;

bool play;
bool menu;
bool trans;

struct border {
    int top = 315;
    int left = -150;
    int right = 150;
    int bottom = -185;
} b;

/*
{"geometry":{"paths" : [[ [-100, 210], [-100,-130], [100,-130], [100,210],[-100,210] ],

 [ [-70,220], [-70,160], [70, 140], [-70, 120], [70, 100], [70, -140] ]]}}
*/

///////////////////////////////
//         JSON DATA         //
///////////////////////////////

struct stageList {
    string name;
};

struct spawnNPC {
    int npcid;
    int pathid;
};

struct spawnNode {
    spawnNPC npcs[10];
    double spawnAt; //in seconds
};

struct spawnerList {
    spawnNode s[9999];
};

struct npcTypes {
    string graphic;
};

struct patternTypes {

};

struct pPList {
    double x;
    double y;
    double speed;
};

struct pathList {
    pPList p[100];
};

stageList stages[10] = {};
spawnerList spawners[10] = {};
npcTypes npcs[100] = {};
pathList paths[100] = {};
patternTypes patterns[100] = {};

///////////////////////////////
//         PLAY DATA         //
///////////////////////////////

struct stage {
    int started_at; //in frames
    bool passed;
    int score;
    int kills;
    int prog;
};

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
};

struct pathpoint{
    bool passed;
    double x;
    double y;
    double speed;
};

struct npcpath {
    int id;
    bool passed;
    pathpoint p[100];
};

struct npc {
    int id;
    bool alive;
    double x;
    double y;
    npcpath paths[100];
};



npc npcList[100] = {};
Bullet bulletList[200] = {};

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

///////////////////////////////
//      PLAYER SECTION       //
///////////////////////////////

void drawPlayer()
{
    glTranslatef(player1.x, player1.y, 0);
    glutSolidSphere(5,20,16);
}

void bordercheck()
{
    if(player1.x < b.left)
        player1.x = b.left;
    if(player1.x > b.right)
        player1.x = b.right;
    if(player1.y < b.bottom)
        player1.y = b.bottom;
    if(player1.y > b.top)
        player1.y = b.top;
}

void playerShoots()
{
    if (player1.shoot)
    {
        int frame = current_frame_num - player1.lastShootFrame;
        if (frame >= player1.fire_rate)
        {
            createBullet(player1.x, player1.y, 10, 0, 50); // x , y , speed , direction (degree) , life (in frames)
            player1.lastShootFrame = current_frame_num;
            //printf("\n shoot");
        }
    }
}

///////////////////////////////
//        NPC SECTION        //
///////////////////////////////

void loadNPC()
{

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

////////////////////////////////////
//    STAGE CONTROLLER SECTION    //
////////////////////////////////////

void stagePlay()
{
    if(play)
    {

    }
}


///////////////////////////////
//    KEY HANDLER SECTION    //
///////////////////////////////

void keyboard(void)
{
    if (SPkeyStates[GLUT_KEY_UP])
    {
        player1.y += player1.speed;
    }

    if (SPkeyStates[GLUT_KEY_DOWN])
    {
        player1.y -= player1.speed;
    }
    if (SPkeyStates[GLUT_KEY_RIGHT])
    {
        player1.x += player1.speed;
    }
    if (SPkeyStates[GLUT_KEY_LEFT])
    {
        player1.x -= player1.speed;
    }
    if (keyStates['z'])
    {
        player1.shoot = true;
    } else {
        player1.shoot = false;
    }
    if (keyStates['c'])
    {
        player1.speed = halfspeed;
    } else {
        player1.speed = halfspeed*2;
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
    bordercheck();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(0, 0, -310);
    glRotatef(-20, 1, 0, 0);

    drawBackground(-150,315,150,-185);

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

void loadStages()
{
    reader.parse(stagefile, obj);
    const Json::Value& fStages = obj["stages"];
    stagecount = fStages.size();
    for (unsigned int i = 0; i < fStages.size(); i++)
    {
        stages[i].name = fStages[i]["name"].asString();
    }
}

void loadSpawners()
{
    for (int i = 0; i < stagecount; i++)
    {
        ifstream spawner;
        switch(i)
        {
        case 0:
            spawner.open("spawners/stage1.json");
            break;
        case 1:
            spawner.open("spawners/stage2.json");
            break;
        default:
            spawner.open("spawners/stage1.json");
            break;
        }

        reader.parse(spawner, obj);
        const Json::Value& fspawnNodes = obj["spawnNodes"];
        for (int j = 0; j < fspawnNodes.size(); j++)
        {
            const Json::Value& fspawnNPC = fspawnNodes[j]["npcs"];
            for (int k = 0; k < fspawnNPC.size(); k++)
            {
                spawners[i].s[j].npcs[k].npcid = fspawnNPC[k]["id"].asInt();
                spawners[i].s[j].npcs[k].pathid = fspawnNPC[k]["path"].asInt();
            }
            spawners[i].s[j].spawnAt = fspawnNodes[j]["spawnAt"].asDouble();
        }
        spawner.close();
        spawner.clear();
    }
}

void loadPaths()
{
    reader.parse(pathfile, obj);
    const Json::Value& fPaths = obj["stages"];
    for (int i = 0; i < fPaths.size(); i++)
    {
        const Json::Value& fPPoints = fPaths["points"];
        for (int j = 0; j < fPPoints.size(); j++)
        {
            paths[i].p[j].x = fPPoints[j]["x"].asDouble();
            paths[i].p[j].y = fPPoints[j]["y"].asDouble();
            paths[i].p[j].speed = fPPoints[j]["speed"].asDouble();
        }
    }
}

void loadFiles()
{
    npcfile.open("npcs.json");
    pathfile.open("paths.json");
    stagefile.open("stages.json");

    loadStages();
    loadSpawners();
    loadPaths();
}

void frameControl()
{
    double latest_frame_time, latest_rendering_time, waste_time, current_second;

    glutPostRedisplay();

    latest_frame_time = start_time + ((current_frame_num + 1) * TPF);
    latest_rendering_time = glutGet(GLUT_ELAPSED_TIME);
    waste_time = latest_frame_time - latest_rendering_time;
    if (waste_time > 0.0)
        Sleep(waste_time);
    current_frame_num = current_frame_num + 1;
    /*
    current_second = latest_rendering_time / 1000;
    if( current_second >= previous_second + 1)
    {
        current_fps = current_frame_num - previous_frame_count;
        previous_frame_count = current_frame_num;
        previous_second = current_second;
    }
    system("CLS");
    printf("%d \n %f", current_fps, waste_time);
    */
}

void init()
{
    player1.y = -100;
    player1.speed = playerSpeed;
    player1.fire_rate = 60/fireRate;
    loadFiles();

    glClearColor( 0.0, 0.0, 0.0, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(75, (GLdouble)900.0/(GLdouble)600.0,0,50);

    start_time = glutGet(GLUT_ELAPSED_TIME);
    current_frame_num = 0;
    glMatrixMode(GL_MODELVIEW);
}

void reshape(int w, int h)
{
    glutReshapeWindow(1350,900);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1350,900);
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


