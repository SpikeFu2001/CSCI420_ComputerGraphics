// assign1.cpp : Defines the entry point for the console application.
//

/*
  CSCI 420 Computer Graphics
  Assignment 1: Height Fields
  Spike Fu
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

typedef enum
{
  POINT_MODE,
  LINE_MODE,
  TRIANGLE_MODE
} RENDERMODE;

RENDERMODE g_RenderMode = POINT_MODE;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 300.0};
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
  // set background color
  glClearColor(0.0, 0.0, 0.0, 0.0);
  // enable depth buffering
  glEnable(GL_DEPTH_TEST);
  // interpolate colors during rasterization
  glShadeModel(GL_SMOOTH); // Color the vertices using some smooth gradient
}

struct Color
{
  unsigned char r, g, b;
};

/**
 * interpolate 2 RGB colors
 * @param color1    integer containing color as 0x00RRGGBB
 * @param color2    integer containing color as 0x00RRGGBB
 * @param fraction  how much interpolation (0..1)
 * - 0: full color 1
 * - 1: full color 2
 * @return the new color after interpolation
 */
Color interpolate(int color1, int color2, float fraction)
{
  unsigned char r1 = (color1 >> 16) & 0xff;
  unsigned char r2 = (color2 >> 16) & 0xff;
  unsigned char g1 = (color1 >> 8) & 0xff;
  unsigned char g2 = (color2 >> 8) & 0xff;
  unsigned char b1 = color1 & 0xff;
  unsigned char b2 = color2 & 0xff;

  return {(unsigned char)((r2 - r1) * fraction + r1),
          (unsigned char)((g2 - g1) * fraction + g1),
          (unsigned char)((b2 - b1) * fraction + b1)};
}

int startColor = 0xFF66C4;
int endColor = 0xFCEAF1;

void heightmap()
{
  float offset_x = g_pHeightData->width() / -2.0;
  float offset_y = g_pHeightData->height() / -2.0;

  switch (g_RenderMode)
  {
  case POINT_MODE:
    glBegin(GL_POINTS);
    for (int i = 0; i < g_pHeightData->width(); i++)
    {
      for (int j = 0; j < g_pHeightData->height(); j++)
      {
        auto color = interpolate(startColor, endColor, ((*g_pHeightData)(i, j) / 256.0f));
        glColor3f(color.r / 256.0f, color.g / 256.0f, color.b / 256.0f);
        glVertex3f(offset_x + i, offset_y + j, (*g_pHeightData)(i, j));
      }
    }
    glEnd();
    break;
  case LINE_MODE:
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (int i = 1; i < g_pHeightData->width(); i++)
    {
      glBegin(GL_TRIANGLE_STRIP);
      for (int j = 0; j < g_pHeightData->height(); j++)
      {
        auto color = interpolate(startColor, endColor, ((*g_pHeightData)(i - 1, j) / 256.0f));
        glColor3f(color.r / 256.0f, color.g / 256.0f, color.b / 256.0f);
        glVertex3f(offset_x + (i - 1), offset_y + j, (*g_pHeightData)(i - 1, j));

        color = interpolate(startColor, endColor, ((*g_pHeightData)(i, j) / 256.0f));
        glColor3f(color.r / 256.0f, color.g / 256.0f, color.b / 256.0f);
        glVertex3f(offset_x + i, offset_y + j, (*g_pHeightData)(i, j));
      }
      glEnd();
    }
    break;
  case TRIANGLE_MODE:
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    for (int i = 1; i < g_pHeightData->width(); i++)
    {
      glBegin(GL_TRIANGLE_STRIP);
      for (int j = 0; j < g_pHeightData->height(); j++)
      {
        auto color = interpolate(startColor, endColor, ((*g_pHeightData)(i - 1, j) / 256.0f));
        glColor3f(color.r / 256.0f, color.g / 256.0f, color.b / 256.0f);
        glVertex3f(offset_x + (i - 1), offset_y + j, (*g_pHeightData)(i - 1, j));

        color = interpolate(startColor, endColor, ((*g_pHeightData)(i, j) / 256.0f));
        glColor3f(color.r / 256.0f, color.g / 256.0f, color.b / 256.0f);
        glVertex3f(offset_x + i, offset_y + j, (*g_pHeightData)(i, j));
      }
      glEnd();
    }
    break;
  }
}

void display()
{
  glClear(GL_COLOR_BUFFER_BIT |
          GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(-g_vLandTranslate[0], -g_vLandTranslate[1], -g_vLandTranslate[2]);
  glRotatef(g_vLandRotate[0], 1.0, 0.0, 0.0);
  glRotatef(g_vLandRotate[1], 0.0, 1.0, 0.0);
  glRotatef(g_vLandRotate[2], 0.0, 0.0, 1.0);
  glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);
  heightmap();
  glutSwapBuffers();
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
  // g_vLandRotate[0] += 0.05;
  // if (g_vLandRotate[0] > 360.0)
  //   g_vLandRotate[0] -= 360.0;
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
      g_vLandTranslate[0] += vMouseDelta[0] * 0.1;
      g_vLandTranslate[1] -= vMouseDelta[1] * 0.1;
    }
    if (g_iMiddleMouseButton)
    {
      g_vLandTranslate[2] += vMouseDelta[1] * 0.1;
    }
    break;
  case ROTATE:
    if (g_iLeftMouseButton)
    {
      g_vLandRotate[0] += vMouseDelta[1] * 2;
      g_vLandRotate[1] += vMouseDelta[0] * 2;
    }
    if (g_iMiddleMouseButton)
    {
      g_vLandRotate[2] += vMouseDelta[1] * 2;
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

void keyboard(unsigned char key,
              int x, int y)
{
  if (key == '1')
  {
    g_RenderMode = POINT_MODE;
  }
  else if (key == '2')
  {
    g_RenderMode = LINE_MODE;
  }
  else if (key == '3')
  {
    g_RenderMode = TRIANGLE_MODE;
  }
}

void reshape(int w, int h)
{
  // setup image size
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  // setup camera
  gluPerspective(70.0, 1.0 * w / h, 0.1, 10000.0);

  glMatrixMode(GL_MODELVIEW);
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
  // request double buffer
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
  glEnable(GL_DEPTH_TEST);

  // set window size
  glutInitWindowSize(500, 500);
  // set window position
  glutInitWindowPosition(0, 0);
  // creates a window
  glutCreateWindow("Spike's Window");
  /* allow the user to quit using the right mouse button menu */
  g_iMenuId = glutCreateMenu(menufunc);
  glutSetMenu(g_iMenuId);
  glutAddMenuEntry("Quit", 0);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  // called every time window is resized to
  // update projection matrix
  glutReshapeFunc(reshape);
  /* tells glut to use a particular display function to redraw */
  glutDisplayFunc(display);

  /* replace with any animate code */
  glutIdleFunc(doIdle);

  /* callback for mouse drags */
  glutMotionFunc(mousedrag);
  /* callback for idle mouse movement */
  glutPassiveMotionFunc(mouseidle);
  /* callback for mouse button changes */
  glutMouseFunc(mousebutton);
  /* callback for keyboard changes */
  glutKeyboardFunc(keyboard);

  /* do initialization */
  myinit();

  glutMainLoop();
  return 0;
}