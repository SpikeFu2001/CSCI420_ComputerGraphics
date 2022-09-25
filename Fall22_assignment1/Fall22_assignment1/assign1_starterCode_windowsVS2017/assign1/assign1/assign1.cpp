// assign1.cpp : Defines the entry point for the console application.
//

/*
  CSCI 420 Computer Graphics
  Assignment 1: Height Fields
  <your name>
*/

#include "stdafx.h"
#include <windows.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include "CImg-2.3.5\CImg.h"

using namespace cimg_library;

int g_iMenuId;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0; /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

typedef enum
{
  ROTATE,
  TRANSLATE,
  SCALE
} CONTROLSTATE;

CONTROLSTATE g_ControlState = ROTATE;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

CImg<unsigned char> *g_pHeightData;

/* This line is required for CImg to be able to read jpg/png format files. */
/* Please install ImageMagick and replace the path below to the correct path to convert.exe on your computer */
void initializeImageMagick()
{
  cimg::imagemagick_path("convert.exe", true);
}

/* Write a screenshot to the specified filename */
void saveScreenshot(char *filename)
{
  int i, j;

  if (filename == NULL)
    return;

  /* Allocate a picture buffer */
  CImg<unsigned char> in(640, 480, 1, 3, 0);

  printf("File to save to: %s\n", filename);

  for (i = 479; i >= 0; i--)
  {
    glReadPixels(0, 479 - i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                 in.data());
  }

  if (in.save_jpeg(filename))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");
}

void myinit()
{
  /* setup gl view here */
  glClearColor(0.0, 0.0, 0.0, 0.0);
}

void display()
{
  /* draw 1x1 cube about origin */
  /* replace this code with your height field implementation */
  /* you may also want to precede it with your
rotation/translation/scaling */

  glBegin(GL_POLYGON);

  glColor3f(1.0, 1.0, 1.0);
  glVertex3f(-0.5, -0.5, 0.0);
  glColor3f(0.0, 0.0, 1.0);
  glVertex3f(-0.5, 0.5, 0.0);
  glColor3f(0.0, 0.0, 0.0);
  glVertex3f(0.5, 0.5, 0.0);
  glColor3f(1.0, 1.0, 0.0);
  glVertex3f(0.5, -0.5, 0.0);

  glEnd();
}

void menufunc(int value)
{
  switch (value)
  {
  case 0:
    exit(0);
    break;
  }
}

void doIdle()
{
  /* do some stuff... */

  /* make the screen update */
  glutPostRedisplay();
}

/* converts mouse drags into information about
rotation/translation/scaling */
void mousedrag(int x, int y)
{
  int vMouseDelta[2] = {x - g_vMousePos[0], y - g_vMousePos[1]};

  switch (g_ControlState)
  {
  case TRANSLATE:
    if (g_iLeftMouseButton)
    {
      g_vLandTranslate[0] += vMouseDelta[0] * 0.01;
      g_vLandTranslate[1] -= vMouseDelta[1] * 0.01;
    }
    if (g_iMiddleMouseButton)
    {
      g_vLandTranslate[2] += vMouseDelta[1] * 0.01;
    }
    break;
  case ROTATE:
    if (g_iLeftMouseButton)
    {
      g_vLandRotate[0] += vMouseDelta[1];
      g_vLandRotate[1] += vMouseDelta[0];
    }
    if (g_iMiddleMouseButton)
    {
      g_vLandRotate[2] += vMouseDelta[1];
    }
    break;
  case SCALE:
    if (g_iLeftMouseButton)
    {
      g_vLandScale[0] *= 1.0 + vMouseDelta[0] * 0.01;
      g_vLandScale[1] *= 1.0 - vMouseDelta[1] * 0.01;
    }
    if (g_iMiddleMouseButton)
    {
      g_vLandScale[2] *= 1.0 - vMouseDelta[1] * 0.01;
    }
    break;
  }
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{

  switch (button)
  {
  case GLUT_LEFT_BUTTON:
    g_iLeftMouseButton = (state == GLUT_DOWN);
    break;
  case GLUT_MIDDLE_BUTTON:
    g_iMiddleMouseButton = (state == GLUT_DOWN);
    break;
  case GLUT_RIGHT_BUTTON:
    g_iRightMouseButton = (state == GLUT_DOWN);
    break;
  }

  switch (glutGetModifiers())
  {
  case GLUT_ACTIVE_CTRL:
    g_ControlState = TRANSLATE;
    break;
  case GLUT_ACTIVE_SHIFT:
    g_ControlState = SCALE;
    break;
  default:
    g_ControlState = ROTATE;
    break;
  }

  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

int main(int argc, char *argv[])
{
  // I've set the argv[1] to spiral.jpg.
  // To change it, on the "Solution Explorer",
  // right click "assign1", choose "Properties",
  // go to "Configuration Properties", click "Debugging",
  // then type your texture name for the "Command Arguments"
  if (argc < 2)
  {
    printf("usage: %s heightfield.jpg\n", argv[0]);
    exit(1);
  }

  initializeImageMagick();

  g_pHeightData = new CImg<unsigned char>((char *)argv[1]);
  if (!g_pHeightData)
  {
    printf("error reading %s.\n", argv[1]);
    exit(1);
  }

  glutInit(&argc, (char **)argv);

  /*
    create a window here..should be double buffered and use depth testing

      the code past here will segfault if you don't have a window set up....
      replace the exit once you add those calls.
  */
  exit(0);

  /* tells glut to use a particular display function to redraw */
  glutDisplayFunc(display);

  /* allow the user to quit using the right mouse button menu */
  g_iMenuId = glutCreateMenu(menufunc);
  glutSetMenu(g_iMenuId);
  glutAddMenuEntry("Quit", 0);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  /* replace with any animate code */
  glutIdleFunc(doIdle);

  /* callback for mouse drags */
  glutMotionFunc(mousedrag);
  /* callback for idle mouse movement */
  glutPassiveMotionFunc(mouseidle);
  /* callback for mouse button changes */
  glutMouseFunc(mousebutton);

  /* do initialization */
  myinit();

  glutMainLoop();
  return 0;
}