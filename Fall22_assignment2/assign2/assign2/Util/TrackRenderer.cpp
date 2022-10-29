#include "TrackRenderer.h"
#include "point.h"
#include "catmull.h"

void TrackRenderer::InitializeRenderer(struct spline spline)
{
}

void TrackRenderer::Render()
{
    glCallList(trackGlList);
}
