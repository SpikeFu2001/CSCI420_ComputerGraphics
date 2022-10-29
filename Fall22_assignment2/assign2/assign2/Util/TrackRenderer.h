#pragma once

#include <GL/glut.h>
#include "point.h"

class TrackRenderer
{
public:
    void InitializeRenderer(struct spline spline, GLuint groundTextureID, GLuint skyTextureID);
    void Render();

private:
    void DrawSpline(Vec3 &a, Vec3 &b, Vec3 &c, Vec3 &d);
    void UpdateNormal(double t, class Catmull &catmull);
    void DrawCube(Vec3 &a, Vec3 &b, Vec3 &c, Vec3 &d, Vec3 &e, Vec3 &f, Vec3 &g, Vec3 &h);
    void DrawFace(Vec3 &a, Vec3 &b, Vec3 &c, Vec3 &d);
    void DrawVertex(Vec3 &v);
    void DrawOneBar(Vec3 p1, Vec3 p2, double width, double height);
    void RenderGround();
    void RenderSky();

    Vec3 T = {0, 0, 0};
    Vec3 N = {0, 0, 0};
    Vec3 B = {0, 0, 0};
    Vec3 oldT = {0, 0, 0};
    Vec3 oldN = {0, 0, 0};
    Vec3 oldB = {0, 0, 0};
    GLuint trackGlList = 1;
    GLuint enviromentGlList = 2;
    double stepSize = 0.005;
    GLuint groundTextureID;
    GLuint skyTextureID;
};