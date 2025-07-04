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

// enable optimus!
// use discrete GPU if possible
extern "C"
{
	_declspec(dllexport) DWORD NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

struct spline *g_Splines;

int g_iNumOfSplines;

TrackRenderer trackRenderer;

GLuint groundTextureID = 1;
GLuint skyTextureID = 2;

GLubyte groundBuffer[4096][4096][3];
GLubyte skyBuffer[4096][8192][3];

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
		g_Splines[j].points = (struct Vec3 *)malloc(iLength * sizeof(struct Vec3));
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

Vec3 T;
Vec3 N;
Vec3 B;
Vec3 oldT;
Vec3 oldN;
Vec3 oldB;

void CameraNormalInit()
{
	Vec3 V = {0, 0.0, -1};
	T = Catmull(g_Splines[0].points[0], g_Splines[0].points[1], g_Splines[0].points[2], g_Splines[0].points[3]).GetNormalizedTangent(0.0);
	N = Vec3::CrossProduct(T, V);
	N.Normalize();
	B = Vec3::CrossProduct(T, N);
	B.Normalize();
}

void UpdateNormal(double t, class Catmull &catmull)
{
	oldT = T;
	oldN = N;
	oldB = B;
	T = catmull.GetNormalizedTangent(t);
	N = Vec3::CrossProduct(oldB, T);
	N.Normalize();
	B = Vec3::CrossProduct(T, N);
	B.Normalize();
}

void GroundTextureInit()
{
	cv::Mat3b img;
	readImage("ground.jpg", img, false);
	for (int r = 0; r < img.rows; r++)
	{ // y-coordinate
		for (int c = 0; c < img.cols; c++)
		{ // x-coordinate
			for (int channel = 0; channel < 3; channel++)
			{
				unsigned char blue = getPixelValue(img, c, r, 0);
				unsigned char green = getPixelValue(img, c, r, 1);
				unsigned char red = getPixelValue(img, c, r, 2);
				groundBuffer[r][c][0] = red;
				groundBuffer[r][c][1] = green;
				groundBuffer[r][c][2] = blue;
			}
		}
	}

	glGenTextures(1, &groundTextureID);
	glBindTexture(GL_TEXTURE_2D, groundTextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.cols, img.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, groundBuffer);
}

void SkyTextureInit()
{
	cv::Mat3b img;
	readImage("sky.jpg", img, false);
	for (int r = 0; r < img.rows; r++)
	{ // y-coordinate
		for (int c = 0; c < img.cols; c++)
		{ // x-coordinate
			for (int channel = 0; channel < 3; channel++)
			{
				unsigned char blue = getPixelValue(img, c, r, 0);
				unsigned char green = getPixelValue(img, c, r, 1);
				unsigned char red = getPixelValue(img, c, r, 2);
				skyBuffer[r][c][0] = red;
				skyBuffer[r][c][1] = green;
				skyBuffer[r][c][2] = blue;
			}
		}
	}
	glGenTextures(1, &skyTextureID);
	glBindTexture(GL_TEXTURE_2D, skyTextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.cols, img.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, skyBuffer);
}

void myinit()
{
	/* setup gl view here */
	glClearColor(0.0, 0.0, 0.0, 0.0);
	// enable depth buffering
	glEnable(GL_DEPTH_TEST);
	// interpolate colors during rasterization
	glShadeModel(GL_SMOOTH);

	GroundTextureInit();
	SkyTextureInit();
	trackRenderer.InitializeRenderer(g_Splines[0], groundTextureID, skyTextureID);
	CameraNormalInit();
}

void RenderWorld()
{
	trackRenderer.Render();
}

double GetDeltaTime()
{
	using namespace std::chrono;
	static auto prev = high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev).count() / 1000.0;
	prev = now;
	return deltaTime;
}

void UpdateCamera()
{
	static bool finished = false;
	static double deltaPerSecond = 0.4;
	static double t = 0.0;
	static int segmentI = 0;
	static auto currentCatmull = Catmull(g_Splines[0].points[segmentI + 0], g_Splines[0].points[segmentI + 1], g_Splines[0].points[segmentI + 2], g_Splines[0].points[segmentI + 3]);
	if (!g_iNumOfSplines || finished)
	{
		return;
	}
	auto delta = deltaPerSecond * GetDeltaTime();
	t += delta;
	if (t > 1.0)
	{
		t = 0.0;
		segmentI += 1;
		if (segmentI >= g_Splines[0].numControlPoints - 3)
		{
			finished = true;
			return;
		}
		else
		{
			currentCatmull = Catmull(g_Splines[0].points[segmentI + 0], g_Splines[0].points[segmentI + 1], g_Splines[0].points[segmentI + 2], g_Splines[0].points[segmentI + 3]);
		}
	}
	auto eye = currentCatmull.GetPoint(t);
	UpdateNormal(t, currentCatmull);
	gluLookAt(eye.x, eye.y, eye.z, T.x + eye.x, T.y + eye.y, T.z + eye.z, B.x, B.y, B.z);
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	UpdateCamera();

	RenderWorld();

	glutSwapBuffers();
}

void reshape(int x, int y)
{
	glViewport(0, 0, x, y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, 1.0 * x / y, 0.01, 1000.0);
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
