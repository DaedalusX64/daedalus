#include "stdafx.h"
#include "HLEGraphics/BaseRenderer.h"

BaseRenderer * gRenderer = NULL;

class RendererImpl : public BaseRenderer
{
};

bool CreateRenderer()
{
	DAEDALUS_ASSERT_Q(gRenderer == NULL);
	gRenderer = new RendererImpl();
	return true;
}
void DestroyRenderer()
{
	delete gRenderer;
	gRenderer = NULL;
}
