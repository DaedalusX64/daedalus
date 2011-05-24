/* 
 * File:   easymessage.h
 * Author: joris
 *
 * Created on 18 augustus 2009, 19:39
 */

#ifndef _EASYMESSAGE_H
#define	_EASYMESSAGE_H

/**
 * Shows A message.
 *
 * @param message - Message which will be displayed to the user
 * @param yesno - boolean wchich enables the yes/no user choice
 *
 * @return 1 of user pressed yes (only when yesno is set to TRUE, otherwise 0
 */
bool ShowMessage(const char * message, bool yesno);

#endif	/* _EASYMESSAGE_H */

