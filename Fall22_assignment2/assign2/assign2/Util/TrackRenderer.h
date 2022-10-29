#pragma once

#include <GL/glut.h>
#include "point.h"

class TrackRenderer
{
public:
    void InitializeRenderer(struct spline spline, GLuint groundTextureID, GLuint skyTextureID);
    void Render();

private:
    void DrawSpline(point &a, point &b, point &c, point &d);
    void UpdateNormal(double t, class Catmull &catmull);
    void DrawCube(point &a, point &b, point &c, point &d, point &e, point &f, point &g, point &h);
    void DrawFace(point &a, point &b, point &c, point &d);
    void DrawVertex(point &v);
    void DrawOneBar(point p1, point p2, double width, double height);
    void RenderGround();
    void RenderSky();
    point T = {0, 0, 0};
    point N = {0, 0, 0};
    point B = {0, 0, 0};
    point oldT = {0, 0, 0};
    point oldN = {0, 0, 0};
    point oldB = {0, 0, 0};
    GLuint trackGlList = 1;
    double stepSize = 0.005;
    GLuint groundTextureID;
    GLuint skyTextureID;
};