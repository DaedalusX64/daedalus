/* 
 * File:   easymessage.h
 * Author: joris
 *
 * Created on 18 augustus 2009, 19:39
 */

#ifndef _EASYMESSAGE_H
#define	_EASYMESSAGE_H

#include <psputility.h>

class EasyMessage {
public:
    EasyMessage();
    virtual ~EasyMessage();

    /**
     * Shows A message.
     *
     * @param message - Message which will be displayed to the user
     * @param yesno - boolean wchich enables the yes/no user choice
     *
     * @return 1 of user pressed yes (only when yesno is set to TRUE, otherwise 0
     */
	u32 ShowMessage(const char * message, bool yesno);

    /**
     * Shows a psp error.
     *
     * NOTE:
     * Not all error codes shows a message.
     *
     * @param error - Error code
     */
    //void ShowError(unsigned int error);

    /**
     * Sets the background color of the dialog.
     *
     * @param color - the background collor
     */
    //void SetBGColor(unsigned int color);

    /** Inits the GU with the recommanded settings. Should be usable for most
     * applications. (compatible with intrafont)
     */
    //void InitGU();

    /**
     * Terminates the GU.
     */
    //void TermGU();
private:
    void _RunDialog();
    pspUtilityMsgDialogParams params;
};

#endif	/* _EASYMESSAGE_H */

