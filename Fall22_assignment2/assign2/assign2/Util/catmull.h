#pragma once

struct point;

class Catmull
{
public:
    Catmull(point &a, point &b, point &c, point &d, double s = 0.5);
    point GetPoint(double t);
    point GetNormalizedTangent(double t);

private:
    double m[4][3];
};