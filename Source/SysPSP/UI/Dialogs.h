
#ifndef _DIALOG_H
#define	_DIALOG_H

class CUIContext;

class Dialog 
{
public:
    virtual ~Dialog();
	bool ShowDialog(CUIContext * p_context, const char * message, bool only_dialog);
};


#endif	/* _DIALOG_H */

