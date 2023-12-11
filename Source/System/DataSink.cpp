
#include "Base/Types.h"

#include "Base/Assert.h"
#include "System/DataSink.h"

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

bool FileSink::Open(const std::filesystem::path &filename, const char * mode)
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT(Handle == NULL, "Already have an open file");
	#endif
	Handle = fopen(filename.string().c_str(), mode);
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
