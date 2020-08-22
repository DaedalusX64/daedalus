#ifndef OSHLE_OSTASK_H_
#define OSHLE_OSTASK_H_

#include "Ultra/ultra_sptask.h"

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

#endif // OSHLE_OSTASK_H_
