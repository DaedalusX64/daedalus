#include "Base/Types.h"
#include "Dialogs.h"
#include "UIContext.h"

#include "Graphics/ColourValue.h"
#include "DrawTextUtilities.h"

//*************************************************************************************
//
//*************************************************************************************
namespace DrawTextUtilities
{
	const c32 TextWhite = c32(255, 255, 255);
	const c32 TextWhiteDisabled = c32(208, 208, 208);
	const c32 TextBlue = c32(80, 80, 208);
	const c32 TextBlueDisabled = c32(80, 80, 178);
	const c32 TextRed = c32(255, 0, 0);
	const c32 TextRedDisabled = c32(208, 208, 208);

	static c32 COLOUR_SHADOW_HEAVY = c32(0x80000000);
	static c32 COLOUR_SHADOW_LIGHT = c32(0x50000000);
} // namespace DrawTextUtilities
