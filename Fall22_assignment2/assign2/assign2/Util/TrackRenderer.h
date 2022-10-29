#pragma once

#include <GL/glut.h>
#include "point.h"

class TrackRenderer
{
public:
    void InitializeRenderer(struct spline spline);
    void Render();

private:
    void DrawSpline(point &a, point &b, point &c, point &d);
    GLuint trackGlList = 1;
};