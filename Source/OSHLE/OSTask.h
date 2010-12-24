#ifndef __OSTASK_H__
#define __OSTASK_H__

#include "ultra_sptask.h"

class COSTask
{
public:
	COSTask(OSTask * pTask)
	{
		m_pTask = pTask;
	}

protected:
	OSTask * m_pTask;

};

#endif
