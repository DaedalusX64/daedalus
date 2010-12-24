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

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef BLENDCONSTANT_H_
#define BLENDCONSTANT_H_

#include "Graphics/ColourValue.h"
#include "HLEGraphics/DaedalusVtx.h"

enum EBlendConstant
{
	BC_SHADE,
	BC_PRIMITIVE,
	BC_ENVIRONMENT,
	BC_PRIMITIVE_ALPHA,
	BC_ENVIRONMENT_ALPHA,
	BC_1,
	BC_0,
};

class CBlendConstantExpression
{
public:
	virtual ~CBlendConstantExpression() {}
	virtual c32					Evaluate( c32 shade, c32 primitive, c32 environment ) const = 0;
	virtual c32					EvaluateConstant( c32 primitive, c32 environment ) const = 0;
	virtual bool				TryEvaluateConstant( const SRenderState & state, c32 * out ) const = 0;

	virtual bool				IsShade() const { return false; }

	virtual void				ApplyExpressionRGB( const SRenderState & state ) const = 0;
	virtual void				ApplyExpressionAlpha( const SRenderState & state ) const = 0;

	virtual std::string			ToString() const = 0;
};

class CBlendConstantExpressionValue : public CBlendConstantExpression
{
public:
	CBlendConstantExpressionValue( EBlendConstant constant )
		:	mConstant( constant )
	{
	}

	virtual bool				IsShade() const { return mConstant == BC_SHADE; }

	c32			Evaluate( c32 shade, c32 primitive, c32 environment ) const
	{
		switch( mConstant )
		{
		case BC_SHADE:				return shade;
		case BC_PRIMITIVE:			return primitive;
		case BC_ENVIRONMENT:		return environment;
		case BC_PRIMITIVE_ALPHA:	return c32( primitive.GetA(), primitive.GetA(), primitive.GetA(), primitive.GetA() );
		case BC_ENVIRONMENT_ALPHA:	return c32( environment.GetA(), environment.GetA(), environment.GetA(), environment.GetA() );
		case BC_1:					return c32( 0xffffffff );
		case BC_0:					return c32( 0x00000000 );
		}

		DAEDALUS_ERROR( "Unhandled constant" );
		return c32( 0xffffffff );
	}

	c32			EvaluateConstant( c32 primitive, c32 environment ) const
	{
		switch( mConstant )
		{
		case BC_SHADE:				DAEDALUS_ERROR( "Shouldn't be here" ); return c32( 0xffffffff );
		case BC_PRIMITIVE:			return primitive;
		case BC_ENVIRONMENT:		return environment;
		case BC_PRIMITIVE_ALPHA:	return c32( primitive.GetA(), primitive.GetA(), primitive.GetA(), primitive.GetA() );
		case BC_ENVIRONMENT_ALPHA:	return c32( environment.GetA(), environment.GetA(), environment.GetA(), environment.GetA() );
		case BC_1:					return c32( 0xffffffff );
		case BC_0:					return c32( 0x00000000 );
		}

		DAEDALUS_ERROR( "Unhandled constant" );
		return c32( 0xffffffff );
	}

	virtual bool				TryEvaluateConstant( const SRenderState & state, c32 * out ) const
	{
		switch( mConstant )
		{
		case BC_SHADE:				return false;
		case BC_PRIMITIVE:			*out = state.PrimitiveColour; return true;
		case BC_ENVIRONMENT:		*out = state.EnvironmentColour; return true;
		case BC_PRIMITIVE_ALPHA:	*out = c32( state.PrimitiveColour.GetA(), state.PrimitiveColour.GetA(), state.PrimitiveColour.GetA(), state.PrimitiveColour.GetA() ); return true;
		case BC_ENVIRONMENT_ALPHA:	*out = c32( state.EnvironmentColour.GetA(), state.EnvironmentColour.GetA(), state.EnvironmentColour.GetA(), state.EnvironmentColour.GetA() ); return true;
		case BC_1:					*out = c32( 0xffffffff ); return true;
		case BC_0:					*out = c32( 0x00000000 ); return true;
		}

		DAEDALUS_ERROR( "Unhandled constant" );
		return false;
	}

	virtual void	ApplyExpressionRGB( const SRenderState & state ) const
	{
		// Applying the shade colour leaves the vertex untouched, so bail out
		if( mConstant != BC_SHADE )
		{
			c32		new_colour( EvaluateConstant( state.PrimitiveColour, state.EnvironmentColour ) );
			for( u32 i = 0; i < state.NumVertices; ++i )
			{
				state.Vertices[ i ].Colour.SetBits( new_colour, c32::MASK_RGB );
			}
		}
	}

	virtual void	ApplyExpressionAlpha( const SRenderState & state ) const
	{
		// Applying the shade colour leaves the vertex untouched, so bail out
		if( mConstant != BC_SHADE )
		{
			c32		new_colour( EvaluateConstant( state.PrimitiveColour, state.EnvironmentColour ) );
			for( u32 i = 0; i < state.NumVertices; ++i )
			{
				state.Vertices[ i ].Colour.SetBits( new_colour, c32::MASK_A );
			}
		}
	}

	virtual std::string			ToString() const
	{
		switch( mConstant )
		{
		case BC_SHADE:				return "Shade";
		case BC_PRIMITIVE:			return "Primitive";
		case BC_ENVIRONMENT:		return "Environment";
		case BC_PRIMITIVE_ALPHA:	return "PrimitiveAlpha";
		case BC_ENVIRONMENT_ALPHA:	return "EnvironmentAlpha";
		case BC_1:					return "1";
		case BC_0:					return "0";
		}	

		DAEDALUS_ERROR( "Unhandled constant" );
		return "?";
	}
private:
	EBlendConstant					mConstant;
};

template< typename ColOp >
class CBlendConstantExpression2 : public CBlendConstantExpression
{
public:
	CBlendConstantExpression2( const CBlendConstantExpression * a, const CBlendConstantExpression * b )
		:	mA( a )
		,	mB( b )
	{
	}

	virtual ~CBlendConstantExpression2()
	{
		delete mA;
		delete mB;
	}

	c32			Evaluate( c32 shade, c32 primitive, c32 environment ) const
	{
		c32		a( mA->Evaluate( shade, primitive, environment ) );
		c32		b( mB->Evaluate( shade, primitive, environment ) );

		return ColOp::Process( a, b );
	}

	c32			EvaluateConstant( c32 primitive, c32 environment ) const
	{
		c32		a( mA->EvaluateConstant( primitive, environment ) );
		c32		b( mB->EvaluateConstant( primitive, environment ) );

		return ColOp::Process( a, b );
	}

	virtual bool				TryEvaluateConstant( const SRenderState & state, c32 * out ) const
	{
		return false;
	}

	virtual void	ApplyExpressionRGB( const SRenderState & state ) const
	{
		c32	a; bool have_a( mA->TryEvaluateConstant( state, &a ) );
		c32 b; bool have_b( mB->TryEvaluateConstant( state, &b ) );
		if( have_a && have_b )
		{
			c32	col( ColOp::Process( a, b ) );
			for( u32 i = 0; i < state.NumVertices; ++i )
			{
				state.Vertices[ i ].Colour.SetBits( col, c32::MASK_RGB );
			}
		}
		else if( have_a )
		{
			if( mB->IsShade() )
			{
				for( u32 i = 0; i < state.NumVertices; ++i )
				{
					c32	col( ColOp::Process( a, state.Vertices[ i ].Colour ) );

					state.Vertices[ i ].Colour.SetBits( col, c32::MASK_RGB );
				}
			}
			else
			{
				for( u32 i = 0; i < state.NumVertices; ++i )
				{
					b = mB->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour );

					state.Vertices[ i ].Colour.SetBits( ColOp::Process( a, b ), c32::MASK_RGB );
				}
			}
		}
		else if( have_b )
		{
			if( mA->IsShade() )
			{
				for( u32 i = 0; i < state.NumVertices; ++i )
				{
					c32	col( ColOp::Process( state.Vertices[ i ].Colour, b ) );

					state.Vertices[ i ].Colour.SetBits( col, c32::MASK_RGB );
				}
			}
			else
			{
				for( u32 i = 0; i < state.NumVertices; ++i )
				{
					a = mA->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour );

					state.Vertices[ i ].Colour.SetBits( ColOp::Process( a, b ), c32::MASK_RGB );
				}
			}
		}
		else
		{
			for( u32 i = 0; i < state.NumVertices; ++i )
			{
				a = mA->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour );
				b = mB->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour );

				state.Vertices[ i ].Colour.SetBits( ColOp::Process( a, b ), c32::MASK_RGB );
			}
		}
	}

	virtual void	ApplyExpressionAlpha( const SRenderState & state ) const
	{
		c32	a; bool have_a( mA->TryEvaluateConstant( state, &a ) );
		c32 b; bool have_b( mB->TryEvaluateConstant( state, &b ) );
		if( have_a && have_b )
		{
			c32	col( ColOp::Process( a, b ) );
			for( u32 i = 0; i < state.NumVertices; ++i )
			{
				state.Vertices[ i ].Colour.SetBits( col, c32::MASK_A );
			}
		}
		else if( have_a )
		{
			if( mB->IsShade() )
			{
				for( u32 i = 0; i < state.NumVertices; ++i )
				{
					c32	col( ColOp::Process( a, state.Vertices[ i ].Colour ) );

					state.Vertices[ i ].Colour.SetBits( col, c32::MASK_A );
				}
			}
			else
			{
				for( u32 i = 0; i < state.NumVertices; ++i )
				{
					b = mB->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour );

					state.Vertices[ i ].Colour.SetBits( ColOp::Process( a, b ), c32::MASK_A );
				}
			}
		}
		else if( have_b )
		{
			if( mA->IsShade() )
			{
				for( u32 i = 0; i < state.NumVertices; ++i )
				{
					c32	col( ColOp::Process( state.Vertices[ i ].Colour, b ) );

					state.Vertices[ i ].Colour.SetBits( col, c32::MASK_A );
				}
			}
			else
			{
				for( u32 i = 0; i < state.NumVertices; ++i )
				{
					a = mA->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour );

					state.Vertices[ i ].Colour.SetBits( ColOp::Process( a, b ), c32::MASK_A );
				}
			}
		}
		else
		{
			for( u32 i = 0; i < state.NumVertices; ++i )
			{
				a = mA->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour );
				b = mB->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour );

				state.Vertices[ i ].Colour.SetBits( ColOp::Process( a, b ), c32::MASK_A );
			}
		}
	}

	virtual std::string			ToString() const
	{
		std::string	str;
		str += "(" + mA->ToString() + ColOp::OpString() + mB->ToString() + ")";
		return str;
	}

private:
	const CBlendConstantExpression *			mA;
	const CBlendConstantExpression *			mB;
};

struct AddOp
{
	static inline c32 Process( c32 a, c32 b )	{ return a.Add( b ); }
	static inline const char * OpString()		{ return " + "; }
};
struct SubOp
{
	static inline c32 Process( c32 a, c32 b )	{ return a.Sub( b ); }
	static inline const char * OpString()		{ return " - "; }
};
struct MulOp
{
	static inline c32 Process( c32 a, c32 b )	{ return a.Modulate( b ); }
	static inline const char * OpString()		{ return " * "; }
};

typedef CBlendConstantExpression2< AddOp >	CBlendConstantExpressionAdd;
typedef CBlendConstantExpression2< SubOp >	CBlendConstantExpressionSub;
typedef CBlendConstantExpression2< MulOp >	CBlendConstantExpressionMul;



class CBlendConstantExpressionBlend : public CBlendConstantExpression
{
public:
	CBlendConstantExpressionBlend( const CBlendConstantExpression * a, const CBlendConstantExpression * b, const CBlendConstantExpression * f )
		:	mA( a )
		,	mB( b )
		,	mF( f )
	{
	}

	virtual ~CBlendConstantExpressionBlend()
	{
		delete mA;
		delete mB;
		delete mF;
	}

	c32			Evaluate( c32 shade, c32 primitive, c32 environment ) const
	{
		c32		a( mA->Evaluate( shade, primitive, environment ) );
		c32		b( mB->Evaluate( shade, primitive, environment ) );
		c32		f( mF->Evaluate( shade, primitive, environment ) );

		return a.Interpolate( b, f );
	}

	c32			EvaluateConstant( c32 primitive, c32 environment ) const
	{
		c32		a( mA->EvaluateConstant( primitive, environment ) );
		c32		b( mB->EvaluateConstant( primitive, environment ) );
		c32		f( mF->EvaluateConstant( primitive, environment ) );

		return a.Interpolate( b, f );
	}

	virtual bool				TryEvaluateConstant( const SRenderState & state, c32 * out ) const
	{
		return false;
	}

	virtual void	ApplyExpressionRGB( const SRenderState & state ) const
	{
		for( u32 i = 0; i < state.NumVertices; ++i )
		{
			c32		a( mA->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour ) );
			c32		b( mB->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour ) );
			c32		f( mF->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour ) );

			state.Vertices[ i ].Colour.SetBits( a.Interpolate( b, f ), c32::MASK_RGB );
		}
	}

	virtual void	ApplyExpressionAlpha( const SRenderState & state ) const
	{
		for( u32 i = 0; i < state.NumVertices; ++i )
		{
			c32		a( mA->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour ) );
			c32		b( mB->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour ) );
			c32		f( mF->Evaluate( state.Vertices[ i ].Colour, state.PrimitiveColour, state.EnvironmentColour ) );

			state.Vertices[ i ].Colour.SetBits( a.Interpolate( b, f ), c32::MASK_A );
		}
	}

	virtual std::string			ToString() const
	{
		std::string	str;
		str += "blend(" + mA->ToString() + ", " + mB->ToString() + ", " + mF->ToString() + ")";
		return str;
	}

private:
	const CBlendConstantExpression *			mA;
	const CBlendConstantExpression *			mB;
	const CBlendConstantExpression *			mF;
};

#endif // BLENDCONSTANT_H_
