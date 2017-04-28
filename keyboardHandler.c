#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include <windows.h>
#include <stdbool.h>

bool keyStates[256] = {false};

void keyboard(void)
{
    if (keyStates[GLUT_KEY_UP])
        player1.y += player1.speed;
    if (keyStates[GLUT_KEY_DOWN])
        player1.y -= player1.speed;
    if (keyStates[GLUT_KEY_RIGHT])
        player1.x += player1.speed;
    if (keyStates[GLUT_KEY_LEFT])
        player1.x -= player1.speed;
}

void keyPressed (int key, int x, int y)
{
    keyStates[key] = true;
}

void keyRelease (int key, int x, int y)
{
    keyStates[key] = false;
}
