#include "stdafx.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <iostream>
#include <GL/glu.h>
#include <GL/glut.h>
#include <chrono>

#include "opencv2/core/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "Util/point.h"
#include "Util/catmull.h"
#include "Util/TrackRenderer.h"

struct spline *g_Splines;

int g_iNumOfSplines;

TrackRenderer trackRenderer;

/* Object where you can load an image */
cv::Mat3b groundTextureIMG;
cv::Mat3b skyTextureIMG;

int loadSplines(char *argv)
{
	char *cName = (char *)malloc(128 * sizeof(char));
	FILE *fileList;
	FILE *fileSpline;
	int iType, i = 0, j, iLength;

	/* load the track file */
	fileList = fopen(argv, "r");
	if (fileList == NULL)
	{
		printf("can't open file\n");
		exit(1);
	}

	/* stores the number of splines in a global variable */
	fscanf(fileList, "%d", &g_iNumOfSplines);
	printf("%d\n", g_iNumOfSplines);
	g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

	/* reads through the spline files */
	for (j = 0; j < g_iNumOfSplines; j++)
	{
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL)
		{
			printf("can't open file\n");
			exit(1);
		}

		/* gets length for spline file */
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		/* allocate memory for all the points */
		g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
		g_Splines[j].numControlPoints = iLength;

		/* saves the data to the struct */
		while (fscanf(fileSpline, "%lf %lf %lf",
					  &g_Splines[j].points[i].x,
					  &g_Splines[j].points[i].y,
					  &g_Splines[j].points[i].z) != EOF)
		{
			i++;
		}
	}

	free(cName);

	return 0;
}

/* Write a screenshot to the specified filename */
void saveScreenshot(char *filename)
{
	if (filename == NULL)
		return;

	// Allocate a picture buffer //
	cv::Mat3b bufferRGB = cv::Mat::zeros(480, 640, CV_8UC3); // rows, cols, 3-channel 8-bit.
	printf("File to save to: %s\n", filename);

	// use fast 4-byte alignment (default anyway) if possible
	glPixelStorei(GL_PACK_ALIGNMENT, (bufferRGB.step & 3) ? 1 : 4);
	// set length of one complete row in destination data (doesn't need to equal img.cols)
	glPixelStorei(GL_PACK_ROW_LENGTH, bufferRGB.step / bufferRGB.elemSize());
	glReadPixels(0, 0, bufferRGB.cols, bufferRGB.rows, GL_RGB, GL_UNSIGNED_BYTE, bufferRGB.data);
	// flip to account for GL 0,0 at lower left
	cv::flip(bufferRGB, bufferRGB, 0);
	// convert RGB to BGR
	cv::Mat3b bufferBGR(bufferRGB.rows, bufferRGB.cols, CV_8UC3);
	cv::Mat3b out[] = {bufferBGR};
	// rgb[0] -> bgr[2], rgba[1] -> bgr[1], rgb[2] -> bgr[0]
	int from_to[] = {0, 2, 1, 1, 2, 0};
	mixChannels(&bufferRGB, 1, out, 1, from_to, 3);

	if (cv::imwrite(filename, bufferBGR))
	{
		printf("File saved Successfully\n");
	}
	else
	{
		printf("Error in Saving\n");
	}
}

/* Function to get a pixel value.
Note: OpenCV images are in channel order BGR.
This means that:
chan = 0 returns BLUE,
chan = 1 returns GREEN,
chan = 2 returns RED. */
unsigned char getPixelValue(cv::Mat3b &image, int x, int y, int chan)
{
	return image.at<cv::Vec3b>(y, x)[chan];
}

/* Read an image into memory.
Set argument displayOn to true to make sure images are loaded correctly.
One image loaded, set to false so it doesn't interfere with OpenGL window.*/
int readImage(char *filename, cv::Mat3b &image, bool displayOn)
{
	std::cout << "reading image: " << filename << std::endl;
	image = cv::imread(filename);
	if (!image.data) // Check for invalid input
	{
		std::cout << "Could not open or find the image." << std::endl;
		return 1;
	}

	if (displayOn)
	{
		cv::imshow("TestWindow", image);
		cv::waitKey(0); // Press any key to enter.
	}
	return 0;
}

GLuint groundTexture = 1;
GLuint skyTexture = 2;

GLubyte ground[2160][3840][3];
GLubyte sky[4096][8192][3];

void groundInit()
{
	readImage("ground.jpg", groundTextureIMG, false);
	for (int r = 0; r < groundTextureIMG.rows; r++)
	{ // y-coordinate
		for (int c = 0; c < groundTextureIMG.cols; c++)
		{ // x-coordinate
			for (int channel = 0; channel < 3; channel++)
			{
				unsigned char blue = getPixelValue(groundTextureIMG, c, r, 0);
				unsigned char green = getPixelValue(groundTextureIMG, c, r, 1);
				unsigned char red = getPixelValue(groundTextureIMG, c, r, 2);
				ground[r][c][0] = red;
				ground[r][c][1] = green;
				ground[r][c][2] = blue;
			}
		}
	}

	glGenTextures(1, &groundTexture);
	glBindTexture(GL_TEXTURE_2D, groundTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, groundTextureIMG.cols, groundTextureIMG.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, ground);
}

void skyInit()
{
	readImage("sky.jpg", skyTextureIMG, false);
	for (int r = 0; r < skyTextureIMG.rows; r++)
	{ // y-coordinate
		for (int c = 0; c < skyTextureIMG.cols; c++)
		{ // x-coordinate
			for (int channel = 0; channel < 3; channel++)
			{
				unsigned char blue = getPixelValue(skyTextureIMG, c, r, 0);
				unsigned char green = getPixelValue(skyTextureIMG, c, r, 1);
				unsigned char red = getPixelValue(skyTextureIMG, c, r, 2);
				sky[r][c][0] = red;
				sky[r][c][1] = green;
				sky[r][c][2] = blue;
			}
		}
	}
	glGenTextures(1, &skyTexture);
	glBindTexture(GL_TEXTURE_2D, skyTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, skyTextureIMG.cols, skyTextureIMG.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, sky);
}

void myinit()
{
	/* setup gl view here */
	glClearColor(0.0, 0.0, 0.0, 0.0);
	// enable depth buffering
	glEnable(GL_DEPTH_TEST);
	// interpolate colors during rasterization
	glShadeModel(GL_SMOOTH);

	groundInit();
	skyInit();
}

void render()
{
	// ground
	{
		glTranslated(100.0, 0.0, 0.0);
		glBindTexture(GL_TEXTURE_2D, groundTexture);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glTexCoord2d(0.0, 0.0);
		glVertex3d(-192, -108, -10);
		glTexCoord2d(0.0, 1.0);
		glVertex3d(-192, 108, -10);
		glTexCoord2d(1.0, 1.0);
		glVertex3d(192, 108, -10);
		glTexCoord2d(1.0, 0.0);
		glVertex3d(192, -108, -10);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}

	// sky
	{
		glRotated(180.0, 1, 0, 0);
		glBindTexture(GL_TEXTURE_2D, skyTexture);
		glEnable(GL_TEXTURE_2D);
		GLUquadric *qobj = gluNewQuadric();
		gluQuadricTexture(qobj, GL_TRUE);
		gluSphere(qobj, 200, 20, 20);
		gluDeleteQuadric(qobj);
		glDisable(GL_TEXTURE_2D);
	}
}

void UpdateCamera()
{
	using namespace std::chrono;
	static double incrementPerSecond = 0.4;
	static double currentT = 0.0;
	static int currentSegmentIndex = 0;
	static auto currentCatmull = Catmull(g_Splines[0].points[currentSegmentIndex + 0], g_Splines[0].points[currentSegmentIndex + 1], g_Splines[0].points[currentSegmentIndex + 2], g_Splines[0].points[currentSegmentIndex + 3]);
	if (!g_iNumOfSplines)
	{
		return;
	}
	static auto start(std::chrono::high_resolution_clock::now());
	auto end(std::chrono::high_resolution_clock::now());
	auto duration(std::chrono::duration_cast<std::chrono::milliseconds>(end - start));
	start = end;
	auto increment = incrementPerSecond * duration.count() / 1000.0;
	currentT += increment;
	if (currentT > 1.0)
	{
		currentT = 0.0;
		currentSegmentIndex += 1;
		if (currentSegmentIndex > g_Splines[0].numControlPoints - 3)
		{
			currentSegmentIndex = 0;
		}
		currentCatmull = Catmull(g_Splines[0].points[currentSegmentIndex + 0], g_Splines[0].points[currentSegmentIndex + 1], g_Splines[0].points[currentSegmentIndex + 2], g_Splines[0].points[currentSegmentIndex + 3]);
	}
	auto eye = currentCatmull.GetPoint(currentT);
	auto T = currentCatmull.GetNormalizedTangent(currentT);
	gluLookAt(eye.x, eye.y, eye.z, T.x + eye.x, T.y + eye.y, T.z + eye.z, 0, 0, 1);
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	UpdateCamera();

	render();
	glutSwapBuffers();
}

void reshape(int x, int y)
{
	glViewport(0, 0, x, y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, 1.0 * x / y, 0.1, 1000.0);
	glMatrixMode(GL_MODELVIEW);
}

void doIdle()
{
	glutPostRedisplay();
}

int _tmain(int argc, _TCHAR *argv[])
{
	if (argc < 2)
	{
		printf("usage: %s <trackfile>\n", argv[0]);
		exit(0);
	}

	loadSplines((char *)argv[1]);

	glutInit(&argc, (char **)argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);

	glutInitWindowSize(640, 480);
	glEnable(GL_DEPTH_TEST);

	glutInitWindowPosition(0, 0);

	glutCreateWindow("Spike");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	glutIdleFunc(doIdle);

	glEnable(GL_DEPTH_TEST);

	myinit();

	glutMainLoop();

	return 0;
}
