#pragma once

#include <math.h>

struct Vec3
{
    double x;
    double y;
    double z;
    void Normalize()
    {
        double len = 0.0;
        len += x * x;
        len += y * y;
        len += z * z;
        len = sqrt(len);
        x /= len;
        y /= len;
        z /= len;
    }
    static Vec3 CrossProduct(Vec3 &p1, Vec3 &p2)
    {
        Vec3 c;
        c.x = p1.y * p2.z - p1.z * p2.y;
        c.y = p1.z * p2.x - p1.x * p2.z;
        c.z = p1.x * p2.y - p1.y * p2.x;
        return c;
    }
};

struct spline
{
    int numControlPoints;
    struct Vec3 *points;
};