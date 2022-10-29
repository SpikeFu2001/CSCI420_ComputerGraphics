#pragma once

struct point;

class Catmull
{
public:
    Catmull(point &a, point &b, point &c, point &d, double s = 0.5);
    point GetPoint(double t);
    point GetNormalizedTangent(double t);
    void Mult(double u[4], double m[][3], double res[4]);

private:
    double m[4][3];
};