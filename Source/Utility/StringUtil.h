#ifndef STRINGUTIL_H_
#define STRINGUTIL_H_

#include <vector>
#include "Utility/String.h"

void Split(ConstStringRef str, char split_char, std::vector<ConstStringRef> * pieces);
char * Tidy(char * s);

#endif // STRINGUTIL_H_
