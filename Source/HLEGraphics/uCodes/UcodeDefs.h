/*
Copyright (C) 2010 StrmnNrmn
Copyright (C) 2003-2009 Rice1964

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

#ifndef _UCODE_DEFS_H_
#define _UCODE_DEFS_H_

/*
struct MicroCodeCommand
{
	union
	{
		u64		_u64;

		struct
		{
			u32		cmd1;
			u32		cmd0;
		};

		struct
		{
			int		: 32;
			int		: 24;
			unsigned int		cmd : 8;
		};
	};
};
*/

struct Instruction
{
	union 
	{
		u32 cmd0;
		struct 
		{
			u32 arg0:24;
			u32 cmd:8;
		};
	};
	union 
	{
		u32 cmd1;
		struct 
		{
			u32 arg1:24;
			u32 pad:8;
		};
	};
};

struct GBI1_Matrix
{
	u32	len:16;
	u32	projection:1;
	u32	load:1;
	u32	push:1;
	u32	:5;
	u32	cmd:8;
	u32 addr;
};
/*
struct GBI1_PopMatrix
{
	u32	:24;
	u32	cmd:8;
	u32	projection:1;
	u32	:31;
};
*/
struct GBI2_Matrix
{
	union 
	{
		struct 
		{
			u32	param:8;
			u32	len:16;
			u32	cmd:8;
		};
		struct 
		{
			u32	nopush:1;
			u32	load:1;
			u32	projection:1;
			u32	:5;
			u32	len2:16;
			u32	cmd2:8;
		};
	};
	u32 addr;
};

struct GBI0_Vtx
{
	u32 len:16;
	u32 v0:4;
	u32 n:4;
	u32 cmd:8;
	u32 addr;
};

struct GBI1_Vtx
{
	u32 len:10;
	u32 n:6;
	u32 :1;
	u32 v0:7;
	u32 cmd:8;
	u32 addr;
};

struct GBI2_Vtx
{
	u32 vend:8;
	u32 :4;
	u32 n:8;
	u32 :4;
	u32 cmd:8;
	u32 addr;
};

struct GBI1_BranchZ
{
    u32 pad0:1;      
    u32 vtx:11;     
    u32 pad1:12;       
    u32 cmd:8;         
    u32 value:32;     
}; 

struct GBI1_ModifyVtx
{
	u32 pad0:1;          
	u32 vtx:15;  
	u32 offset:8;    
	u32 cmd:8;           
	u32 value;
};

struct GBI_Texture
{
	u32	enable_gbi0:1;
	u32	enable_gbi2:1;
	u32	:6;
	u32	tile:3;
	u32	level:3;
	u32	:10;
	u32	cmd:8;
	u32	scaleT:16;
	u32	scaleS:16;
};

struct SetCullDL
{
    u32 pad0:1;             
    u32 first:15;   
    u32 pad2:8;            
    u32 cmd:8;             
    u32 pad3:1;            
    u32 end:15;    
    u32 pad4:8;             
};

struct SetTImg
{
	u32 width:12;
	u32 :7;
	u32 siz:2;
	u32 fmt:3;
	u32	cmd:8;
	u32 addr;
};

struct LoadTile
{
	u32	tl:12;
	u32	sl:12;
	u32	cmd:8;

	u32	th:12;
	u32	sh:12;
	u32	tile:3;
	u32	pad:5;
};

struct SetColor 
{
	u32	prim_level:8;
	u32	prim_min_level:8;
	u32	pad:8;
	u32	cmd:8;

	u32 a:8;
	u32 b:8;
	u32 g:8;
	u32 r:8;
};

struct GBI1_MoveWord
{
	u32	type:8;
	u32	offset:16;
	u32	cmd:8;
	u32	value;
};

struct GBI2_MoveWord
{
	u32	offset:16;
	u32	type:8;
	u32	cmd:8;
	u32	value;
};

struct GBI2_Tri1
{
	u32 v0:8;
	u32 v1:8;
	u32 v2:8;
	u32 cmd:8;
	u32 pad:24;
	u32 flag:8;
};

struct GBI2_Tri2
{
	u32 :1;
	u32 v3:7;
	u32 :1;
	u32 v4:7;
	u32 :1;
	u32 v5:7;
	u32 cmd:8;
	u32 :1;
	u32 v0:7;
	u32 :1;
	u32 v1:7;
	u32 :1;
	u32 v2:7;
	u32 flag:8;
};

struct GBI2_Line3D
{
	u32 v3:8;
	u32 v4:8;
	u32 v5:8;
	u32 cmd:8;

	u32 v0:8;
	u32 v1:8;
	u32 v2:8;
	u32 flag:8;
};

struct GBI1_Line3D
{
	u32 w0;
	u32 v2:8;
	u32 v1:8;
	u32 v0:8;
	u32 v3:8;
};

struct GBI1_Tri1
{
	u32 w0;
	u32 v2:8;
	u32 v1:8;
	u32 v0:8;
	u32 flag:8;
};

struct GBI1_Tri2
{
	u32 v5:8;
	u32 v4:8;
	u32 v3:8;
	u32 cmd:8;

	u32 v2:8;
	u32 v1:8;
	u32 v0:8;
	u32 flag:8;
};

struct GBI0_Tri4
{
	u32 v0:4;
	u32 v3:4;
	u32 v6:4;
	u32 v9:4;
	u32 pad:8;
	u32 cmd:8;
	u32 v1:4;
	u32 v2:4;
	u32 v4:4;
	u32 v5:4;
	u32 v7:4;
	u32 v8:4;
	u32 v10:4;
	u32 v11:4;
};
/*
struct Conker_Tri4 
{
	// Tri 3
	u32 v6:5; 
	u32 v7:5;
	u32 v8:5;

	// Tri 4
	u32 v9hi:3;
	u32 v10:5;
	u32 v11:5;
	u32 cmd:4;

	// Tri 1
	u32 v0:5;
	u32 v1:5;
	u32 v2:5;

	// Tri 2
	u32 v3:5;
	u32 v4:5;
	u32 v5:5;
	u32 v9lo:2;
};
*/

struct GBI1_Dlist
{
	u32	:16;
	u32	param:8;
	u32	cmd:8;
	u32 addr;
};

struct SetScissor
{
	u32	y0:12;	    
	u32	x0:12;	   
	u32	cmd:8;	    
	u32	y1:12;	    
	u32	x1:12;	  
	u32	mode:2;
	u32	pad:6;	
};

struct SetLoadTile
{
	u32	tl:12;
	u32	sl:12;
	u32	cmd:8;

	u32	th:12;
	u32	sh:12;
	u32	tile:3;
	u32	pad:5;
};

struct SetFillRect
{
	u32 pad1	: 2;
	u32 y1		: 10;
	u32 pad0	: 2;
	u32 x1		: 10;
	u32 cmd		: 8;

	u32 pad3	: 2;
	u32 y0		: 10;
	u32 pad4	: 2;
	u32 x0		: 10;
	u32 pad2	: 8;
};

struct SetPrimDepth
{
	u32 pad0:24;
	u32 cmd:8; 
    u32 dz:16;   
    u32 z:15;   
	u32 pad:1;
};

struct SetOthermode
{
	u32	len:8;
	u32	sft:8;
	u32	cmd:8;
	u32	data;
};

/*
struct GBI1_LoadUcode
{
    u32 size:16;     
    u32 pad:8;
    u32 cmd:8;
    u32 base:24;
};
*/
union MicroCodeCommand
{
	Instruction		inst;
	GBI0_Vtx		vtx0;
	GBI1_Vtx		vtx1;
	GBI2_Vtx		vtx2;
	GBI1_ModifyVtx	modifyvtx;
	GBI1_BranchZ	branchz;
	GBI1_Matrix		mtx1;
	GBI2_Matrix		mtx2;
	//GBI1_PopMatrix	popmtx;
	GBI1_Line3D		gbi1line3d;
	GBI1_Tri1		gbi1tri1;
	GBI1_Tri2		gbi1tri2;
	GBI2_Line3D		gbi2line3d;
	GBI2_Tri1		gbi2tri1;
	GBI2_Tri2		gbi2tri2;
	GBI0_Tri4		tri4;
	//Conker_Tri4		conkertri4;
	GBI1_MoveWord	mw1;
	GBI2_MoveWord	mw2;
	GBI_Texture		texture;
	GBI1_Dlist		dlist;
	//GBI1_LoadUcode	loaducode;
	SetCullDL		culldl;		
	SetColor		color;
	SetTImg			img;
	SetScissor		scissor;
	SetLoadTile		loadtile;
	SetFillRect		fillrect;
	SetPrimDepth	primdepth;
	SetOthermode	othermode;

	u64	force_structure_alignment;
};


#endif
