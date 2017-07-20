#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <windows.h>
#include <stdbool.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include "headers/json/json.h"

using namespace std;

#define target_fps 100
#define PI 3.14159265
ifstream npcfile;
ifstream stagefile;
ifstream bpatfile;



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

int currentStage = 0;
int stagecount;

int previous_second = 0;
int current_second = 0;
int previous_frame_count = 0;
int current_fps = 0;
float latest_frame_time;
float latest_rendering_time;
float waste_time;

const int font = (int)GLUT_BITMAP_TIMES_ROMAN_24;

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
{"geometry":{"paths" : [[ [-150, 315], [-150,-185], [150,-185], [150,315],[-150,315] ],

 [ [-70,325], [-90,210], [90, 190], [-80, 160], [70, 140], [70, -210] ]]}}
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
    int spawnAt; //in seconds
    int npcCount;
};

struct spawnerList {
    spawnNode s[500];
    int nodeCount = 0;
};

struct npcType {
    float radius;
    int health;
    int bptype;
};

struct patternTypes {
    int type;
    int slice;
    float rate;
    float speed;
    float rad;
};

struct pPList {
    double x;
    double y;
    double speed;
    int shoot;
};

struct pathList {
    pPList p[20];
};

stageList stages[10] = {};
spawnerList spawners[10] = {};
npcType npcTypes[10] = {};
pathList paths[100] = {};
patternTypes patterns[10] = {};

///////////////////////////////
//         PLAY DATA         //
///////////////////////////////

struct stage {
    int stageNum;
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
    bool alive;
    bool shoot;
    bool immortal;
    int radius;
    int fire_rate;
    int lastShootFrame;
    int score;
    int life;
} player1;

struct Bullet {
    bool alive;
    double x;
    double y;
    double speed;
    double direction;
    float rad;
    int owner;
    int birthFrame;
    int lifeTime; //in frames
};

struct npc {
    int id;
    bool alive;
    double x;
    double y;
    float rad;
    int bptype;
    int health;
    int pathID;
    int node = 0;
    int lastShootFrame;
};

int npcCount = 0;
int npcMaxArray = 0;
int bulletCount = 0;
int maxArray = 0;
int npcBulletCount = 0;
int npcBMaxArray = 0;
npc npcList[100] = {};
Bullet bulletList[100] = {};

//////////////////////////////////////
//      TEXT RENDERING SECTION      //
//////////////////////////////////////

void writeText(float x, float y, void *font, const char *text){
    glPushMatrix();
    const char *c;
    glColor3f(1,1,1);
    glRasterPos2f(x, y);
    for (c=text; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
    glPopMatrix();
}

string numToStr(int num){
    stringstream ss;
    ss << num;
    return ss.str();
}

//////////////////////////////////
//         MENU SECTION         //
//////////////////////////////////

int getNPCBullet()
{
    int i;
    for (i=0; i <= maxArray ; i++){
        if (bulletList[i].alive && bulletList[i].owner == 0){
            return i;
        }
    }
}

void drawStatsBoard(float x, float y, float z )
{
    float w = 120;
    float h = 80;
    float p = 2;
    float tx = x+10;
    float ty = y-20;
    float vts = 15;
    float hts = 60;
    glColor3f(0.9,0.9,0.9);
    glRectf(x,y,x+w,y-h);
    glColor3f(0.1,0.1,0.1);
    glRectf(x+p,y-p,x+w-p,y-h+p);
    glColor3f(1,1,1);

    writeText(tx,ty,(void *)font, "Score: ");
    const char *text = numToStr(player1.score).c_str();
    writeText(tx+hts,ty,(void *)font, text);

    writeText(tx,ty-vts,(void *)font, "Life : ");
    const char *text2 = numToStr(player1.life).c_str();
    writeText(tx+hts,ty-vts,(void *)font, text2);

    int id = getNPCBullet();
    writeText(tx,ty-2*vts,(void *)font, "NPCB x: ");
    const char *text3 = numToStr(bulletList[id].x).c_str();
    writeText(tx+hts,ty-2*vts,(void *)font, text3);
    writeText(tx,ty-3*vts,(void *)font, "NPCB y: ");
    const char *text4 = numToStr(bulletList[id].y).c_str();
    writeText(tx+hts,ty-3*vts,(void *)font, text4);
}

void drawMainMenu()
{

}


///////////////////////////////
//      HITBOX SECTION       //
///////////////////////////////

float proximity(float Ax, float Ay, float Bx, float By) //returns distance between A and B
{
    float dx, dy;
    if(Ax >= Bx)
        dx = Ax - Bx;
    else
        dx = Bx - Ax;

    if(Ay >= By)
        dy = Ay - By;
    else
        dy = By - Ay;

    if(dx >= dy)
        return dx;
    else
        return dy;
}

int pBulletHitCheck(int i)
{
    for (int j = 0; j < npcMaxArray; j++){
        if (npcList[j].alive){
            float dist = proximity(bulletList[i].x,bulletList[i].y,npcList[j].x,npcList[j].y);
            if (dist < npcList[j].rad + 1.5){ //bullet hit
                npcList[j].health -= 1;
                if (npcList[j].health <= 0){ //npc zero health
                    npcList[j].alive = false;
                    player1.score += 10;
                }
                bulletList[i].alive = false;
                return 1;
            }
        }
    }
    return 0;
}

int nBulletHitCheck(int i)
{
    float dist = proximity(bulletList[i].x,bulletList[i].y,player1.x,player1.y);
    if (dist < player1.radius + bulletList[i].rad){ //bullet hit
        if (!player1.immortal){
            player1.alive = false;
            player1.immortal = true;
            player1.life--;
            bulletList[i].alive = false;
            return 1;
        }
    }
    return 0;
}

//////////////////////////////////////
//      PLAYER BULLET SECTION       //
//////////////////////////////////////

int getEmptyIndex()
{
    int i;
    for (i=0 ; i <= bulletCount ; i++){
        if (!bulletList[i].alive)
            return i;
    }
    if (i>bulletCount) {return -1;}
}

void createBullet(int x, int y, float rad, float speed, float dir, int life, int owner)
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
    bulletList[i].rad = rad;
    bulletList[i].owner = owner;

}

void checkBulletsLife()
{
    int i;
    //system("CLS");
    //printf("\n Bullet Count: %d \n Max Array: %d", bulletCount, maxArray);
    for (i=0 ; i <= maxArray ; i++){
        //printf("\n Array no: %d", i);
        if (bulletList[i].alive){
            //printf("\n Alive");
            if (bulletList[i].owner == 0){
                int hit = pBulletHitCheck(i);
                if (!hit){
                    int age = current_frame_num - bulletList[i].birthFrame;
                    //printf("\n Age: %d", age);
                    if (bulletList[i].lifeTime < age){
                        bulletList[i].alive = false;
                        bulletCount--;
                    }
                }
            } else if (bulletList[i].owner == 1){
                int hit = nBulletHitCheck(i);
                if (!hit){
                    if (bulletList[i].x >= b.right || bulletList[i].x <= b.left){
                        bulletList[i].alive = false;
                        bulletCount--;
                    } else if (bulletList[i].y >= b.top || bulletList[i].y <= b.bottom){
                        bulletList[i].alive = false;
                        bulletCount--;
                    }
                }
            }
        }
    }
}

void drawBullets()
{
    int i;
    for (i=0 ; i <= maxArray ; i++){
        if (bulletList[i].alive){
            glPushMatrix();
            glColor3f(0.1,0.1,1);
            glTranslatef(bulletList[i].x,bulletList[i].y,0);
            glutSolidSphere(bulletList[i].rad,20,16);
            glPopMatrix();

            bulletList[i].x += bulletList[i].speed * sin(bulletList[i].direction*PI/180);
            bulletList[i].y += bulletList[i].speed * cos(bulletList[i].direction*PI/180);
        }
    }
}

void reduceArray()
{
    if (!bulletList[maxArray].alive && maxArray > 0){
        if (maxArray <= bulletCount){
            maxArray = bulletCount;
        } else {
            maxArray--;
        }
    }
}

///////////////////////////////
//      PLAYER SECTION       //
///////////////////////////////

void playerShoots()
{
    if (player1.shoot)
    {
        int frame = current_frame_num - player1.lastShootFrame;
        if (frame >= player1.fire_rate)
        {
            createBullet(player1.x, player1.y, 1.5, 10, 0, 50, 0); // x , y , rad, speed , direction (degree) , life (in frames)
            player1.lastShootFrame = current_frame_num;
            //printf("\n shoot");
        }
    }
}

void drawPlayer()
{
    playerShoots();
    glPushMatrix();
    glColor3f(0.1,1,0.1);
    glTranslatef(player1.x, player1.y, 0);
    glutSolidSphere(player1.radius,20,16);
    glPopMatrix();
}

void spawnPlayer()
{
    if (player1.y < -100){
        player1.y += player1.speed/1.5;
        glPushMatrix();
        glColor3f(0.1,1,0.1);
        glTranslatef(player1.x, player1.y, 0);
        glutSolidSphere(player1.radius,20,16);
        glPopMatrix();
    } else {
        player1.immortal = false;
        player1.alive = true;
    }
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

///////////////////////////////////
//      NPC BULLET SECTION       //
///////////////////////////////////

void NPCshoot(int i, int patternID)
{
    int frame = current_frame_num - npcList[i].lastShootFrame;
    if (frame >= patterns[patternID].rate)
    {
        if(patterns[patternID].type == 0){
            float dir = atan2(player1.y-npcList[i].y,player1.x-npcList[i].x) * 180/PI;
            createBullet(npcList[i].x, npcList[i].y, patterns[patternID].rad, patterns[patternID].speed, dir, 0, 1);
        } else {
            float dir = 360/patterns[patternID].slice;
            float dir2;
            for(int j = 0; j < patterns[patternID].slice; j++){
                dir2 = dir2 + dir;
                createBullet(npcList[i].x, npcList[i].y, patterns[patternID].rad, patterns[patternID].speed, dir2, 0, 1);
            }
        }

        npcList[i].lastShootFrame = current_frame_num;
    }
}

///////////////////////////////
//        NPC SECTION        //
///////////////////////////////

int NPCgetEmptyIndex()
{
    int i;
    for (i=0 ; i <= npcCount ; i++)
    {
        if (!npcList[i].alive)
            return i;
    }
    if (i>npcCount) {return -1;}
}

void NPCreduceArray()
{
    if (!npcList[npcMaxArray].alive && npcMaxArray > 0)
    {
        if (npcMaxArray <= npcCount)
        {
            npcMaxArray = npcCount;
        } else {
            npcMaxArray--;
        }
    }
}

void spawnNPC(int npcid, int pathid)
{
    npcCount++;
    npcMaxArray++;
    int i = NPCgetEmptyIndex();
    npcList[i].alive = true;
    npcList[i].id = npcid;
    npcList[i].rad = npcTypes[npcid].radius;
    npcList[i].health = npcTypes[npcid].health;
    npcList[i].pathID = pathid;
    npcList[i].bptype = npcTypes[npcid].bptype;
    npcList[i].x = paths[pathid].p[0].x;
    npcList[i].y = paths[pathid].p[0].y;
    npcList[i].node = 1;
}

float dx;
float dy;
float dist;
int moveNPC(int npcid, float x, float y, float speed)
{
    dx = x - npcList[npcid].x;
    dy = y - npcList[npcid].y;
    dist = sqrt(dx*dx+dy*dy);
    dx = (dx/dist)*speed;
    dy = (dy/dist)*speed;

    npcList[npcid].x += dx;
    npcList[npcid].y += dy;

    glPushMatrix();
    glColor3f(1,0.1,0.1);
    glTranslatef(npcList[npcid].x,npcList[npcid].y,0);
    glutSolidSphere(npcList[npcid].rad,20,16);
    glPopMatrix();

    float prox = proximity(npcList[npcid].x,npcList[npcid].y,x,y);
    if( prox <= 1)
        return 1;
    else
        return 0;
}

void drawNPC()
{
    int i;
    for(i=0 ; i<=npcMaxArray ; i++)
    {
        if(npcList[i].alive)
        {
            int pathid = npcList[i].pathID;
            int node = npcList[i].node;
            if(paths[pathid].p[node].shoot)
            {
                NPCshoot(i,npcList[i].bptype);
            }
            int pass = moveNPC(i,
                    paths[pathid].p[node].x,
                    paths[pathid].p[node].y,
                    paths[pathid].p[node].speed);
            if(pass == 1)
                npcList[i].node++;
            if(!paths[pathid].p[node].x)
            {
                npcList[i].alive = false;
                npcCount--;
                NPCreduceArray();
            }
        }
    }
}


///////////////////////////////
//    BACKGROUND SECTION     //
///////////////////////////////

int depth = 10;
void drawBackground(int x1,int y1,int x2,int y2)
{
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
    //int nodeCount = spawners[currentStage].nodeCount;

    //system("CLS");
    //printf("\n x: %f \n y: %f \n node: %d",npcList[0].x,npcList[0].y,npcList[0].node);

    if(current_second >= spawners[currentStage].s[spawners[currentStage].nodeCount].spawnAt && spawners[currentStage].s[spawners[currentStage].nodeCount].spawnAt != 0)
    {
        int i = 0;
        while(i < spawners[currentStage].s[spawners[currentStage].nodeCount].npcCount){
            spawnNPC(spawners[currentStage].s[spawners[currentStage].nodeCount].npcs[i].npcid,
                     spawners[currentStage].s[spawners[currentStage].nodeCount].npcs[i].pathid);
            i++;
        }
        spawners[currentStage].nodeCount++;
    }
}

void test()
{
    int i;
    for (i = 0 ; i < 2 ; i++)
    {
        int j;
        for (j = 0 ; j < 6 ; j++)
        {
            printf("\n x: %f y: %f speed: %f", paths[i].p[j].x,paths[i].p[j].y,paths[i].p[j].speed);
        }
        printf("\n");
    }
    system("CLS");
}


///////////////////////////////
//    KEY HANDLER SECTION    //
///////////////////////////////

void keyboard(void)
{
    if (player1.alive){
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
        if (keyStates['x'])
        {
            player1.shoot = true;
        } else {
            player1.shoot = false;
        }
        if (keyStates['z'])
        {
            player1.speed = halfspeed;
        } else {
            player1.speed = halfspeed*2;
        }
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
//      DISPLAY SECTION      //
///////////////////////////////

void display(void)
{

    //test();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if(play == true){
        keyboard();
        bordercheck();

        glTranslatef(0, 0, -310);
        drawStatsBoard(180, 180, 10);
        glLoadIdentity();

        glTranslatef(0, 0, -310);
        glRotatef(-20, 1, 0, 0);

        drawBackground(-150,315,150,-185);



        stagePlay();

        drawNPC();

        if (player1.alive){
            drawPlayer();
        } else {
            spawnPlayer();
        }

        checkBulletsLife();
        drawBullets();
        reduceArray();




    } else if (menu == true){
        glTranslatef(0,0, -100);
        drawMainMenu();
    }

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
    stagefile.close();
    stagefile.clear();
}

void loadNPC()
{
    reader.parse(npcfile, obj);
    const Json::Value& fNpcs = obj["npcs"];
    for (unsigned int i = 0; i < fNpcs.size(); i++)
    {
        npcTypes[i].radius = fNpcs[i]["rad"].asFloat();
        npcTypes[i].health = fNpcs[i]["hp"].asInt();
        npcTypes[i].bptype = fNpcs[i]["bpattern"].asInt();
    }
    npcfile.close();
    npcfile.clear();
}

void loadBPatterns()
{
    reader.parse(bpatfile, obj);
    const Json::Value& fbpat = obj["bpattern"];
    for (unsigned int i = 0; i < fbpat.size(); i++)
    {
        patterns[i].type = fbpat[i]["type"].asInt();
        patterns[i].slice = fbpat[i]["slice"].asInt();
        patterns[i].rate = 60/fbpat[i]["rate"].asFloat();
        patterns[i].speed = fbpat[i]["speed"].asFloat();
        patterns[i].rad = fbpat[i]["rad"].asFloat();
    }
    bpatfile.close();
    bpatfile.clear();
}

void loadPath()
{
    ifstream pathf;
    pathf.open("data/paths.json");

    reader.parse(pathf, obj);
    const Json::Value& fPath = obj["paths"];
    for (int j = 0; j < fPath.size(); j++)
    {
        const Json::Value& fspawnNPC = fPath[j]["points"];
        for (int k = 0; k < fspawnNPC.size(); k++)
        {
            paths[j].p[k].x = fspawnNPC[k]["x"].asDouble();
            paths[j].p[k].y = fspawnNPC[k]["y"].asDouble();
            paths[j].p[k].speed = fspawnNPC[k]["speed"].asDouble();
            paths[j].p[k].shoot = fspawnNPC[k]["shoot"].asInt();
        }
    }
    pathf.close();
    pathf.clear();
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
            spawners[i].s[j].spawnAt = fspawnNodes[j]["spawnAt"].asInt();
            spawners[i].s[j].npcCount = fspawnNodes[j]["npcCount"].asInt();
        }
        spawner.close();
        spawner.clear();
    }
}

void loadFiles()
{
    npcfile.open("data/npcs.json");
    stagefile.open("data/stages.json");
    bpatfile.open("data/bpattern.json");

    loadStages();
    loadPath();
    loadSpawners();
    loadNPC();
    loadBPatterns();
    printf("\nLoadfiles");
}

void frameControl()
{
    glutPostRedisplay();

    latest_frame_time = start_time + ((current_frame_num + 1) * TPF);
    latest_rendering_time = glutGet(GLUT_ELAPSED_TIME);
    waste_time = latest_frame_time - latest_rendering_time;
    if (waste_time > 0.0)
        Sleep(waste_time);
    current_frame_num = current_frame_num + 1;
    if ( current_second + 1 < latest_rendering_time / 1000 )
        current_second++;
    /*
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
    play = true;
    //menu = true;
    player1.radius = 8;
    player1.immortal = true;
    player1.y = -200;
    player1.score = 0;
    player1.life = 3;
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


