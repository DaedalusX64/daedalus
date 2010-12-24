/*
Copyright (C) 2001 StrmnNrmn

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

#ifndef __ULTRA_MBI_H__
#define __ULTRA_MBI_H__

#define	M_GFXTASK	1
#define	M_AUDTASK	2
#define	M_VIDTASK	3
#define M_JPGTASK	4

#define	NUM_SEGMENTS			(16)
#define	SEGMENT_OFFSET(a)		((unsigned int)(a) & 0x00ffffff)
#define	SEGMENT_NUMBER(a)		((unsigned int)(a) >> 24)
#define	SEGMENT_ADDR(num, off)	(((num) << 24) + (off))


#endif // __ULTRA_MBI_H__
