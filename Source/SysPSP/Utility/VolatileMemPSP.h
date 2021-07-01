#ifndef SYSPSP_UTILITY_VOLATILEEMEMPSP_H_
#define SYSPSP_UTILITY_VOLATILEEMEMPSP_H_

void* malloc_volatile_PSP(size_t size);
void free_volatile_PSP(void* ptr);
void VolatileMemInit();


#endif //SYSPSP_UTILITY_VOLATILEEMEMPSP_H_