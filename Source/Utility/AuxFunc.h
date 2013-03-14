/*
Copyright (C) 2013 Corn

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

//Shouldn't this be in math.h?

//Speedy random returns a number 1 to (2^32)-1 //Corn
inline u32 FastRand()
{
	static u32 IO_RAND=0x12345678;
	IO_RAND = (IO_RAND << 1) | (((IO_RAND >> 31) ^ (IO_RAND >> 28)) & 1);
	return IO_RAND;
}
