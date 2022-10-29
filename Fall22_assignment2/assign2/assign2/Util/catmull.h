#pragma once

struct Vec3;

class Catmull
{
public:
    Catmull(Vec3 &a, Vec3 &b, Vec3 &c, Vec3 &d, double s = 0.5);
    Vec3 GetPoint(double t);
    Vec3 GetNormalizedTangent(double t);
    void Mult(double u[4], double m[][3], double res[3]);

private:
    double m[4][3];
};