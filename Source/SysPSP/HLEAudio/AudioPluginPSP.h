/*
Copyright (C) 2007 StrmnNrmn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef AUDIOPLUGIN_PSP_H_
#define AUDIOPLUGIN_PSP_H_

#include "Plugins/AudioPlugin.h"

class AudioCode;

class CAudioPluginPsp : public CAudioPlugin
{
private:
	CAudioPluginPsp();
public:
	static CAudioPluginPsp *		Create();


	virtual ~CAudioPluginPsp();
	virtual bool			StartEmulation();
	virtual void			StopEmulation();
	virtual void			AddBufferHLE(u8 *addr, u32 len);

	virtual void			DacrateChanged( ESystemType system_type );
	virtual void			LenChanged();
	virtual u32				ReadLength();
	virtual EProcessResult	ProcessAList();
	virtual void			RomClosed();

//			void			SetAdaptFrequecy( bool adapt );

private:
	AudioCode *			mAudioCode;
};


#endif // AUDIOPLUGIN_PSP_H_

