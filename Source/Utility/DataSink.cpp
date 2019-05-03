#include "stdafx.h"
#include "DataSink.h"
#include "Debug/DaedalusAssert.h"

DataSink::~DataSink()
{
}

FileSink::FileSink()
:	Handle(NULL)
{
}

FileSink::~FileSink()
{
	if (Handle)
		fclose(Handle);
}

bool FileSink::Open(const char * filename, const char * mode)
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT(Handle == NULL, "Already have an open file");
	#endif
	Handle = fopen(filename, mode);
	return Handle != NULL;
}

size_t FileSink::Write(const void * p, size_t len)
{
	if (Handle)
		return fwrite(p, 1, len, Handle);

	return 0;
}

void FileSink::Flush()
{
	if (Handle)
		fflush(Handle);
}
