#pragma once

struct point
{
    double x;
    double y;
    double z;
};

struct spline
{
    int numControlPoints;
    struct point *points;
};

void MatrixMulti(double u[4], double m[][3], double ans[4]);
