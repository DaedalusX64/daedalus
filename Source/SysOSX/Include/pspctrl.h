// This is a dummy version of pspctrl.h to get code compiling on OSX.

#ifndef PSPCTRL_H__
#define PSPCTRL_H__

#define PSP_CTRL_TRIANGLE 	(1<<0)
#define PSP_CTRL_CIRCLE 	(1<<1)
#define PSP_CTRL_CROSS 		(1<<2)
#define PSP_CTRL_SQUARE 	(1<<3)
#define PSP_CTRL_LEFT 		(1<<4)
#define PSP_CTRL_DOWN 		(1<<5)
#define PSP_CTRL_UP 		(1<<6)
#define PSP_CTRL_RIGHT 		(1<<7)
#define PSP_CTRL_LTRIGGER	(1<<8)
#define PSP_CTRL_RTRIGGER	(1<<9)
#define PSP_CTRL_START		(1<<10)
#define PSP_CTRL_SELECT		(1<<11)
#define PSP_CTRL_HOME		(1<<12)

struct SceCtrlData
{
	u16 Buttons;
	s8 Lx;
	s8 Ly;
};

int sceCtrlPeekBufferPositive(SceCtrlData * data, int num);

#endif // PSPCTRL_H__
