#ifndef __LANGUAGETABLE_H__
#define __LANGUAGETABLE_H__

struct LanguageTableEntry
{
	WORD nLangID;
	LPCTSTR pszLanguageName;
};

extern LanguageTableEntry g_LanguageTable[];

#endif //__LANGUAGETABLE_H__
