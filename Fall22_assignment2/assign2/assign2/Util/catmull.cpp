#include "catmull.h"
#include "point.h"
#include <math.h>
Catmull::Catmull(Vec3 &p1, Vec3 &p2, Vec3 &p3, Vec3 &p4, double s)
{
	m[0][0] = -s * p1.x + (2 - s) * p2.x + (s - 2) * p3.x + s * p4.x;
	m[0][1] = -s * p1.y + (2 - s) * p2.y + (s - 2) * p3.y + s * p4.y;
	m[0][2] = -s * p1.z + (2 - s) * p2.z + (s - 2) * p3.z + s * p4.z;

	m[1][0] = 2 * s * p1.x + (s - 3) * p2.x + (3 - 2 * s) * p3.x + -s * p4.x;
	m[1][1] = 2 * s * p1.y + (s - 3) * p2.y + (3 - 2 * s) * p3.y + -s * p4.y;
	m[1][2] = 2 * s * p1.z + (s - 3) * p2.z + (3 - 2 * s) * p3.z + -s * p4.z;

	m[2][0] = -s * p1.x + s * p3.x;
	m[2][1] = -s * p1.y + s * p3.y;
	m[2][2] = -s * p1.z + s * p3.z;

	m[3][0] = p2.x;
	m[3][1] = p2.y;
	m[3][2] = p2.z;
}

Vec3 Catmull::GetPoint(double t)
{
	double u[4];
	u[0] = t * t * t;
	u[1] = t * t;
	u[2] = t;
	u[3] = 1;

	double ans[3];
	Mult(u, m, ans);

	return {ans[0], ans[1], ans[2]};
}

Vec3 Catmull::GetNormalizedTangent(double t)
{
	double u[4];
	u[0] = 3 * t * t;
	u[1] = 2 * t;
	u[2] = 1;
	u[3] = 0;

	double ans[3];
	Mult(u, m, ans);

	double len = 0.0;
	len += ans[0] * ans[0];
	len += ans[1] * ans[1];
	len += ans[2] * ans[2];
	len = sqrt(len);

	return {ans[0] / len, ans[1] / len, ans[2] / len};
}

void Catmull::Mult(double u[4], double m[][3], double res[3])
{
	res[0] = u[0] * m[0][0] + u[1] * m[1][0] + u[2] * m[2][0] + u[3] * m[3][0];
	res[1] = u[0] * m[0][1] + u[1] * m[1][1] + u[2] * m[2][1] + u[3] * m[3][1];
	res[2] = u[0] * m[0][2] + u[1] * m[1][2] + u[2] * m[2][2] + u[3] * m[3][2];
}
