/*

  Copyright (C) 2002 StrmNrmn

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


#ifndef RDP_H__
#define RDP_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define RDP_EMULATE_TMEM	0


typedef struct _SetTImgInfo
{
	u32 Format;
	u32 Size;
	u32 Width;
	u32 Address;
} SetImgInfo;



//*****************************************************************************
// Types
//*****************************************************************************
#include "PushStructPack1.h"

typedef struct
{
	union
	{
		struct
		{
			// Low bits
			unsigned int		alpha_compare : 2;			// 0..1
			unsigned int		depth_source : 1;			// 2..2

		//	unsigned int		render_mode : 13;			// 3..15
			unsigned int		aa_en : 1;					// 3
			unsigned int		z_cmp : 1;					// 4
			unsigned int		z_upd : 1;					// 5
			unsigned int		im_rd : 1;					// 6
			unsigned int		clr_on_cvg : 1;				// 7

			unsigned int		cvg_dst : 2;				// 8..9
			unsigned int		zmode : 2;					// 10..11

			unsigned int		cvg_x_alpha : 1;			// 12
			unsigned int		alpha_cvg_sel : 1;			// 13
			unsigned int		force_bl : 1;				// 14
			unsigned int		tex_edge : 1;				// 15 - Not used

			unsigned int		blender : 16;				// 16..31


			// High bits
			unsigned int		blend_mask : 4;				// 0..3 - not supported
			unsigned int		alpha_dither : 2;			// 4..5
			unsigned int		rgb_dither : 2;				// 6..7
			
			unsigned int		comb_key : 1;				// 8..8
			unsigned int		text_conv : 3;				// 9..11
			unsigned int		text_filt : 2;				// 12..13
			unsigned int		text_tlut : 2;				// 14..15

			unsigned int		text_lod : 1;				// 16..16
			unsigned int		text_detail : 2;			// 17..18
			unsigned int		text_persp : 1;				// 19..19
			unsigned int		cycle_type : 2;				// 20..21
			unsigned int		color_dither : 1;			// 22..22 - not supported
			unsigned int		pipeline : 1;				// 23..23

			unsigned int		pad : 8;					// 24..31 - padding

		};
		u64			_u64;
		u32			_u32[2];
	};
} RDP_OtherMode;


typedef struct
{
	union
	{
		u32		_u32[4];

		struct
		{
			u32 cmd3;
			u32 cmd2;
			u32 cmd1;
			u32 cmd0;
		};

		struct
		{
			// cmd3
			signed int			dtdy : 16;
			signed int			dsdx : 16;

			// cmd2
			unsigned int		t : 16;
			unsigned int		s : 16;

			// cmd1
			unsigned int		y0 : 12;
			unsigned int		x0 : 12;
			unsigned int		tile_idx : 3;
			int					pad1 : 5;

			// cmd0
			unsigned int		y1 : 12;
			unsigned int		x1 : 12;

			int					pad0 : 8;
		};

	};
} RDP_TexRect;


struct RDP_Tile
{
	union
	{
		struct
		{
			u32		cmd1;
			u32		cmd0;
		};

		struct
		{
			// cmd1
			unsigned int		shift_s : 4;
			unsigned int		mask_s : 4;
			unsigned int		mirror_s : 1;
			unsigned int		clamp_s : 1;

			unsigned int		shift_t : 4;
			unsigned int		mask_t : 4;
			unsigned int		mirror_t : 1;
			unsigned int		clamp_t : 1;

			unsigned int		palette : 4;
			unsigned int		tile_idx : 3;

			unsigned int		pad1 : 5;

			// cmd0
			unsigned int		tmem : 9;
			unsigned int		line : 9;
			unsigned int		pad0 : 1;
			unsigned int		size : 2;
			unsigned int		format : 3;

			int					cmd : 8;
		};

	};

	bool	operator==( const RDP_Tile & rhs ) const
	{
		return cmd0 == rhs.cmd0 && cmd1 == rhs.cmd1;
	}
	bool	operator!=( const RDP_Tile & rhs ) const
	{
		return cmd0 != rhs.cmd0 || cmd1 != rhs.cmd1;
	}
};


struct RDP_TileSize
{
	union
	{
		struct
		{
			u32		cmd1;
			u32		cmd0;
		};

		struct
		{
			// cmd1
			unsigned int		bottom : 12;
			unsigned int		right : 12;

			unsigned int		tile_idx : 3;
			int					pad1 : 5;

			// cmd0
			unsigned int		top : 12;
			unsigned int		left : 12;

			int					cmd : 8;
		};
	};

	bool	operator==( const RDP_TileSize & rhs ) const
	{
		return cmd0 == rhs.cmd0 && cmd1 == rhs.cmd1;
	}
	bool	operator!=( const RDP_TileSize & rhs ) const
	{
		return cmd0 != rhs.cmd0 || cmd1 != rhs.cmd1;
	}
};


typedef struct
{
	union
	{
		u64		_u64;
		u32		_u32[2];

		struct
		{
			u32		cmd1;
			u32		cmd0;
		};	};
} RDP_Mux;

typedef struct
{
	union
	{
		u64		_u64;
		u32		_u32[2];

		struct
		{
			u32		cmd1;
			u32		cmd0;
		};
	};
} RDPCommand;



#include "PopStructPack.h"

//*****************************************************************************
// Externs
//*****************************************************************************
extern RDP_OtherMode		gRDPOtherMode;
extern RDP_Mux				gRDPMux;


extern u8 gTextureMemory[ 4096 ];
extern 	RDP_TileSize gRDPTileSizes[8];
extern	RDP_Tile gRDPTiles[8];
extern	u32 gTextureTile;


//*****************************************************************************
// Functions
//*****************************************************************************
void	RDP_SetOtherMode( u32 dwCmd0, u32 dwCmd1 );
void	RDP_SetMux( u64	mux );
void	RDP_SetTile( RDP_Tile tile );
void	RDP_SetTileSize( RDP_TileSize tile_tile );
void	RDP_LoadBlock( RDP_TileSize command );
void	RDP_LoadTile( RDP_TileSize tile_size  );
void	RDP_LoadTLut( RDP_TileSize load_tlut);

//*****************************************************************************
//
//*****************************************************************************
static inline bool	IsZModeDecal()
{
	return gRDPOtherMode.zmode == 3;		// TODO enum or #def!
}


#endif // RDP_H__
