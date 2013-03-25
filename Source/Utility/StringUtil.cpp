#include "stdafx.h"
#include "StringUtil.h"

char * Tidy(char * s)
{
	if (s == NULL || *s == '\0')
		return s;

	char * p = s + strlen(s);

	p--;
	while (p >= s && (*p == ' ' || *p == '\r' || *p == '\n'))
	{
		*p = 0;
		p--;
	}
	return s;
}

void Split(ConstStringRef str, char split_char, std::vector<ConstStringRef> * pieces)
{
	ConstStringRef cur = str;

	for (const char * p = str.Begin; p < str.End; ++p)
	{
		if (*p == split_char)
		{
			cur.End = p;
			pieces->push_back(cur);
			cur.Begin = p+1;
		}
	}

	cur.End = str.End;
	if (cur.Begin <= cur.End)
	{
		pieces->push_back(cur);
	}
}

void SplitAt(ConstStringRef str, char split_char, ConstStringRef * left, ConstStringRef * right)
{
	for (const char * p = str.Begin; p < str.End; ++p)
	{
		if (*p == split_char)
		{
			left->Begin = str.Begin;
			left->End = p;
			right->Begin = p+1;
			right->End = str.End;
			return;
		}
	}

	// Split char wasn't found - just return the whole string as 'left'.
	*left = str;
	*right = ConstStringRef();
}

u32 ParseU32(ConstStringRef str, u32 base)
{
	u32 val = 0;
	for (const char * p = str.Begin; p < str.End; ++p)
	{
		char c = *p;

		// Bail as soon as we see a non-decimal value.
		u32 digit = base;
		if (c >= '0' && c <= '9')
			digit = c - '0';
		else if (c >= 'a' && c <= 'z')
			digit = 10 + (c - 'a');
		else if (c >= 'A' && c <= 'Z')
			digit = 10 + (c - 'A');

		if (digit >= base)
			break;

		val *= base;
		val += c - '0';
	}

	return val;
}

// void Print(const std::vector<ConstStringRef> & pieces)
// {
// 	for (size_t i = 0; i < pieces.size(); ++i)
// 	{
// 		printf("'%.*s',", pieces[i].size(), pieces[i].Begin);
// 	}
// 	printf("\n");
// }

// void TestSplit(const char * str)
// {
// 	ConstStringRef r(str);
// 	std::vector<ConstStringRef> v;
// 	Split(r, '&', &v);
// 	printf("%s -> %d pieces\n", str, v.size());
// 	Print(v);
// }

// void TestSplit()
// {
// 	TestSplit("");
// 	TestSplit("&");
// 	TestSplit("&&");
// 	TestSplit("&a");
// 	TestSplit("a&");
// 	TestSplit("abc&def");
// }
