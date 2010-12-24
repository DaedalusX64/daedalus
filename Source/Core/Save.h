/*
Copyright (C) 2008-2009 Howard Su (howard0su@gmail.com)

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

#pragma once

class Save{
private:
	static char szSaveFileName[MAX_PATH+1];
	static bool SaveDirty;
	static u32	nSaveSize;

	static char szMempackFileName[MAX_PATH+1];
	static bool mempackDirty;

public:
	static void Reset();
	static void Flush(bool force = false);
	static void Fini() { Flush(true);}

	static void MarkSaveDirty() { SaveDirty = true; }
	static void MarkMempackDirty() { mempackDirty = true; }
};
