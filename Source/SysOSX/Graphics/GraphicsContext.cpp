#include "stdafx.h"
#include "Graphics/GraphicsContext.h"


static u32 SCR_WIDTH = 640;
static u32 SCR_HEIGHT = 480;


class IGraphicsContext : public CGraphicsContext
{
public:
	virtual ~IGraphicsContext();


	virtual bool Initialise() { return true; }
	virtual bool IsInitialised() const { return true; }

	virtual void ClearAllSurfaces() {}
	virtual void ClearZBuffer(u32 depth) {}
	virtual void ClearColBuffer(u32 col) {}
	virtual void Clear(bool clear_screen, bool clear_depth) {}
	virtual void Clear(u32 frame_buffer_col, u32 depth) {}
	virtual	void BeginFrame() {}
	virtual void EndFrame() {}
	virtual void UpdateFrame( bool wait_for_vbl ) {}

	virtual void GetScreenSize(u32 * width, u32 * height) const
	{
		*width = SCR_WIDTH;
		*height = SCR_HEIGHT;
	}
	virtual void SetDebugScreenTarget( ETargetSurface buffer ) {}

	virtual void ViewportType( u32 * d_width, u32 * d_height ) {}
	virtual void DumpNextScreen() {}
	virtual void DumpScreenShot() {}
};

template<> bool CSingleton< CGraphicsContext >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new IGraphicsContext();
	return mpInstance->Initialise();
}


IGraphicsContext::~IGraphicsContext()
{
}
