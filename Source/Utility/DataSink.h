#ifndef DATASINK_H_
#define DATASINK_H_

class DataSink
{
public:
	virtual ~DataSink();
	virtual size_t Write(const void * p, size_t len) = 0;
	virtual void Flush() = 0;
};

class FileSink : public DataSink
{
public:
	FileSink();
	~FileSink();

	bool Open(const char * filename);

	virtual size_t Write(const void * p, size_t len);
	virtual void Flush();

private:
	FILE * 		Handle;
};

#endif // DATASINK_H_
