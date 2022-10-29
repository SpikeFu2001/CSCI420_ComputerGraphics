#include "TrackRenderer.h"
#include "point.h"
#include "catmull.h"

void TrackRenderer::InitializeRenderer(struct spline spline, GLuint groundTextureID, GLuint skyTextureID)
{
    this->groundTextureID = groundTextureID;
    this->skyTextureID = skyTextureID;
    Vec3 V = {0, 0.0, -1};
    T = Catmull(spline.points[0], spline.points[1], spline.points[2], spline.points[3]).GetNormalizedTangent(0.0);
    N = Vec3::CrossProduct(T, V);
    N.Normalize();
    B = Vec3::CrossProduct(T, N);
    B.Normalize();

    trackGlList = glGenLists(1);
    glNewList(trackGlList, GL_COMPILE);
    for (int i = 0; i < spline.numControlPoints - 3; i += 1)
    {
        DrawSpline(spline.points[i + 0], spline.points[i + 1], spline.points[i + 2], spline.points[i + 3]);
    }
    glEndList();

    enviromentGlList = glGenLists(1);
    glNewList(enviromentGlList, GL_COMPILE);
    RenderGround();
    RenderSky();
    glEndList();
}

void TrackRenderer::Render()
{
    glCallList(trackGlList);
    glCallList(enviromentGlList);
}

void TrackRenderer::DrawSpline(Vec3 &a, Vec3 &b, Vec3 &c, Vec3 &d)
{
    Catmull catmull(a, b, c, d);
    auto p1 = catmull.GetPoint(0);
    int i = 0;
    for (double t = stepSize; t <= 1.0; t += stepSize, i++)
    {
        auto p2 = catmull.GetPoint(t);
        UpdateNormal(t, catmull);
        glTranslated(0.1 * oldN.x, 0.1 * oldN.y, 0.1 * oldN.z);
        DrawOneBar(p1, p2, 0.005, 0.002);
        glTranslated(-0.1 * oldN.x, -0.1 * oldN.y, -0.1 * oldN.z);
        glTranslated(-0.1 * oldN.x, -0.1 * oldN.y, -0.1 * oldN.z);
        DrawOneBar(p1, p2, 0.005, 0.002);
        glTranslated(0.1 * oldN.x, 0.1 * oldN.y, 0.1 * oldN.z);
        if (i % 5 == 0)
        {
            DrawOneBar(p1, p2, 0.005, 0.1);
        }
        p1 = p2;
    }
}

void TrackRenderer::UpdateNormal(double t, class Catmull &catmull)
{
    oldT = T;
    oldN = N;
    oldB = B;
    T = catmull.GetNormalizedTangent(t);
    N = Vec3::CrossProduct(oldB, T);
    N.Normalize();
    B = Vec3::CrossProduct(T, N);
    B.Normalize();
}

void TrackRenderer::DrawCube(Vec3 &a, Vec3 &b, Vec3 &c, Vec3 &d, Vec3 &e, Vec3 &f, Vec3 &g, Vec3 &h)
{
    glColor3d(0, 0, 0);
    DrawFace(a, c, d, b);
    DrawFace(b, d, h, f);
    DrawFace(a, c, g, e);
    DrawFace(c, g, h, d);
    glColor3d(0.58, 0.43, 0.20);
    DrawFace(a, e, f, b);
    DrawFace(e, g, h, f);
    glColor3d(1, 1, 1);
}

void TrackRenderer::DrawFace(Vec3 &a, Vec3 &b, Vec3 &c, Vec3 &d)
{
    glBegin(GL_POLYGON);
    DrawVertex(a);
    DrawVertex(b);
    DrawVertex(c);
    DrawVertex(d);
    glEnd();
}

void TrackRenderer::DrawVertex(Vec3 &a)
{
    glVertex3d(a.x, a.y, a.z);
}

void TrackRenderer::DrawOneBar(Vec3 p1, Vec3 p2, double width, double height)
{
    p1 = {
        p1.x - 0.1 * oldB.x,
        p1.y - 0.1 * oldB.y,
        p1.z - 0.1 * oldB.z,
    };
    p2 = {
        p2.x - 0.1 * B.x,
        p2.y - 0.1 * B.y,
        p2.z - 0.1 * B.z,
    };
    Vec3 a = {
        p1.x + width * oldB.x - height * oldN.x,
        p1.y + width * oldB.y - height * oldN.y,
        p1.z + width * oldB.z - height * oldN.z,
    };
    Vec3 b = {
        p1.x + width * oldB.x + height * oldN.x,
        p1.y + width * oldB.y + height * oldN.y,
        p1.z + width * oldB.z + height * oldN.z,
    };
    Vec3 c = {
        p1.x - width * oldB.x - height * oldN.x,
        p1.y - width * oldB.y - height * oldN.y,
        p1.z - width * oldB.z - height * oldN.z,
    };
    Vec3 d = {
        p1.x - width * oldB.x + height * oldN.x,
        p1.y - width * oldB.y + height * oldN.y,
        p1.z - width * oldB.z + height * oldN.z,
    };
    Vec3 e = {
        p2.x + width * B.x - height * N.x,
        p2.y + width * B.y - height * N.y,
        p2.z + width * B.z - height * N.z,
    };
    Vec3 f = {
        p2.x + width * B.x + height * N.x,
        p2.y + width * B.y + height * N.y,
        p2.z + width * B.z + height * N.z,
    };
    Vec3 g = {
        p2.x - width * B.x - height * N.x,
        p2.y - width * B.y - height * N.y,
        p2.z - width * B.z - height * N.z,
    };
    Vec3 h = {
        p2.x - width * B.x + height * N.x,
        p2.y - width * B.y + height * N.y,
        p2.z - width * B.z + height * N.z,
    };
    DrawCube(a, b, c, d, e, f, g, h);
}

void TrackRenderer::RenderGround()
{
    glBindTexture(GL_TEXTURE_2D, groundTextureID);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 0.0);
    glVertex3d(-200, -200, -10);
    glTexCoord2d(0.0, 20);
    glVertex3d(-200, 200, -10);
    glTexCoord2d(20, 20);
    glVertex3d(200, 200, -10);
    glTexCoord2d(20, 0.0);
    glVertex3d(200, -200, -10);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void TrackRenderer::RenderSky()
{
    glRotated(180.0, 1, 0, 0);
    glBindTexture(GL_TEXTURE_2D, skyTextureID);
    glEnable(GL_TEXTURE_2D);
    static GLUquadric *qobj = gluNewQuadric();
    gluQuadricTexture(qobj, GL_TRUE);
    gluSphere(qobj, 100, 20, 20);
    glDisable(GL_TEXTURE_2D);
}
