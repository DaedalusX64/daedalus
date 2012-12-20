

#ifndef VOLATILEMEM_H_
#define VOLATILEMEM_H_

#ifdef DAEDALUS_PSP
void* malloc_volatile_PSP(size_t size);
void free_volatile_PSP(void* ptr);
#endif

inline void* malloc_volatile(size_t size)
{
#ifdef DAEDALUS_PSP
	return malloc_volatile_PSP(size);
#else
	return malloc(size);
#endif
}

inline void free_volatile(void* ptr)
{
#ifdef DAEDALUS_PSP
	free_volatile_PSP(ptr);
#else
	free(ptr);
#endif
}

#endif // VOLATILEMEM_H_
