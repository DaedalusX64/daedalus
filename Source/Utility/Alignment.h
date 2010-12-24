/*
Copyright (C) 2001-2007 StrmnNrmn

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

#if _MSC_VER > 1000
#pragma once
#endif


#ifndef ALIGNMENT_H__
#define ALIGNMENT_H__

#define ALIGNED_TYPE(type, type_name, alignval) type __attribute__((aligned(alignval))) type_name
#define ALIGNED_GLOBAL(type, var, alignval) __attribute__((aligned(alignval))) type var
#define ALIGNED_MEMBER(type, var, alignval) __attribute__((aligned(alignval))) type var
#define ALIGNED_EXTERN(type, var, alignval) extern __attribute__((aligned(alignval))) type var

#define DATA_ALIGN	16
#define CACHE_ALIGN	64
#define PAGE_ALIGN	64

#endif //ALIGNMENT_H__
