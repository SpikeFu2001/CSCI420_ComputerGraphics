#include "point.h"

void MatrixMulti(double u[4], double m[][3], double ans[4])
{
    ans[0] = u[0] * m[0][0] + u[1] * m[1][0] + u[2] * m[2][0] + u[3] * m[3][0];
    ans[1] = u[0] * m[0][1] + u[1] * m[1][1] + u[2] * m[2][1] + u[3] * m[3][1];
    ans[2] = u[0] * m[0][2] + u[1] * m[1][2] + u[2] * m[2][2] + u[3] * m[3][2];
    ans[3] = u[0] * m[0][3] + u[1] * m[1][3] + u[2] * m[2][3] + u[3] * m[3][3];
}