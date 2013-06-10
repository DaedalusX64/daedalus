#ifndef UTILITY_STRINGUTIL_H_
#define UTILITY_STRINGUTIL_H_

#include <vector>
#include "Utility/String.h"

// Splits at all instances of split_char.
void Split(ConstStringRef str, char split_char, std::vector<ConstStringRef> * pieces);

// Splits at the first instance of split_char.
void SplitAt(ConstStringRef str, char split_char, ConstStringRef * left, ConstStringRef * right);

// Unlike atoi, atol etc, this works with unterminated ranges.
u32 ParseU32(ConstStringRef str, u32 base);

char * Tidy(char * s);

#endif // UTILITY_STRINGUTIL_H_
