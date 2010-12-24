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

#include "stdafx.h"
#include "CombinerTree.h"

#include "CombinerExpression.h"
#include "RenderSettings.h"
#include "BlendConstant.h"

#include "Utility/Stream.h"

//*****************************************************************************
//
//*****************************************************************************
namespace
{

const ECombinerInput	CombinerInput32[ 32 ] = 
{
	CI_COMBINED,		CI_TEXEL0,			CI_TEXEL1,			CI_PRIMITIVE,	CI_SHADE,			CI_ENV,				CI_1,					CI_COMBINED_ALPHA,
	CI_TEXEL0_ALPHA,	CI_TEXEL1_ALPHA,	CI_PRIMITIVE_ALPHA,	CI_SHADE_ALPHA,	CI_ENV_ALPHA,		CI_LOD_FRACTION,	CI_PRIM_LOD_FRACTION,	CI_K5,

	CI_UNKNOWN,			CI_UNKNOWN,			CI_UNKNOWN,			CI_UNKNOWN,		CI_UNKNOWN,			CI_UNKNOWN,			CI_UNKNOWN,				CI_UNKNOWN,
	CI_UNKNOWN,			CI_UNKNOWN,			CI_UNKNOWN,			CI_UNKNOWN,		CI_UNKNOWN,			CI_UNKNOWN,			CI_UNKNOWN,				CI_0,
};

const ECombinerInput	CombinerInput16[ 16 ] = 
{
	CI_COMBINED,		CI_TEXEL0,			CI_TEXEL1,			CI_PRIMITIVE,	CI_SHADE,			CI_ENV,				CI_1,					CI_COMBINED_ALPHA,
	CI_TEXEL0_ALPHA,	CI_TEXEL1_ALPHA,	CI_PRIMITIVE_ALPHA,	CI_SHADE_ALPHA,	CI_ENV_ALPHA,		CI_LOD_FRACTION,	CI_PRIM_LOD_FRACTION,	CI_0,
};

const ECombinerInput	CombinerInput8[ 8 ] = 
{
	CI_COMBINED,		CI_TEXEL0,			CI_TEXEL1,			CI_PRIMITIVE,	CI_SHADE,			CI_ENV,				CI_1,					CI_0,
};

const ECombinerInput	CombinerInputAlphaC1_8[ 8 ] = 
{
	CI_LOD_FRACTION,		CI_TEXEL0,			CI_TEXEL1,			CI_PRIMITIVE,	CI_SHADE,			CI_ENV,				CI_1,					CI_0,
};

const ECombinerInput	CombinerInputAlphaC2_8[ 8 ] = 
{
	CI_COMBINED,		CI_TEXEL0,			CI_TEXEL1,			CI_PRIMITIVE,	CI_SHADE,			CI_ENV,				CI_1,					CI_0,
};


enum EBuildConstantExpressionOptions
{
	BCE_ALLOW_SHADE,
	BCE_DISALLOW_SHADE,
};

const CBlendConstantExpression *	BuildConstantExpression( const CCombinerOperand * operand, EBuildConstantExpressionOptions options )
{
	if(operand->IsInput())
	{
		const CCombinerInput *	input( static_cast< const CCombinerInput * >( operand ) );

		switch( input->GetInput() )
		{
		case CI_PRIMITIVE:			return new CBlendConstantExpressionValue( BC_PRIMITIVE );
		case CI_ENV:				return new CBlendConstantExpressionValue( BC_ENVIRONMENT );
		case CI_PRIMITIVE_ALPHA:	return new CBlendConstantExpressionValue( BC_PRIMITIVE_ALPHA );
		case CI_ENV_ALPHA:			return new CBlendConstantExpressionValue( BC_ENVIRONMENT_ALPHA );
		case CI_1:					return new CBlendConstantExpressionValue( BC_1 );
		case CI_0:					return new CBlendConstantExpressionValue( BC_0 );
		case CI_SHADE:		
			if( options == BCE_ALLOW_SHADE )
				return new CBlendConstantExpressionValue( BC_SHADE );
			else
				return NULL;
		default:
			return NULL;
		}
	}
	else if(operand->IsSum())
	{
		const CCombinerSum *	sum( static_cast< const CCombinerSum * >( operand ) );

		const CBlendConstantExpression *	sum_expr( NULL );

		for( u32 i = 0; i < sum->GetNumOperands(); ++i )
		{
			const CCombinerOperand *			sum_term( sum->GetOperand( i ) );
			const CBlendConstantExpression *	lhs( sum_expr );
			const CBlendConstantExpression *	rhs( BuildConstantExpression( sum_term, options ) );

			if( rhs == NULL )
			{
				delete sum_expr;
				return NULL;
			}

			if( sum->IsTermNegated( i ) )
			{
				if( lhs == NULL )
				{
					lhs = new CBlendConstantExpressionValue( BC_0 );
				}

				sum_expr = new CBlendConstantExpressionSub( lhs, rhs );		
			}
			else
			{
				if( lhs == NULL )
				{
					sum_expr = rhs;
				}
				else
				{
					sum_expr = new CBlendConstantExpressionAdd( lhs, rhs );
				}		
			}
		}

		return sum_expr;
	}
	else if(operand->IsProduct())
	{
		const CCombinerProduct *	product( static_cast< const CCombinerProduct * >( operand ) );

		const CBlendConstantExpression *	product_expr( NULL );

		for( u32 i = 0; i < product->GetNumOperands(); ++i )
		{
			const CCombinerOperand *			product_term( product->GetOperand( i ) );
			const CBlendConstantExpression *	lhs( product_expr );
			const CBlendConstantExpression *	rhs( BuildConstantExpression( product_term, options ) );

			if( rhs == NULL )
			{
				delete product_expr;
				return NULL;
			}

			if( lhs == NULL )
			{
				product_expr = rhs;
			}
			else
			{
				product_expr = new CBlendConstantExpressionMul( lhs, rhs );
			}
		}

		return product_expr;
	}
	else
	{
		return NULL;
	}
}


}

//*****************************************************************************
//
//*****************************************************************************
CCombinerTree::CCombinerTree( u64 mux, bool two_cycles )
:	mMux( mux )
,	mCycle1( NULL )
,	mCycle1A( NULL )
,	mCycle2( NULL )
,	mCycle2A( NULL )
{
	u32 mux0 = (u32)(mux>>32);
	u32 mux1 = (u32)(mux);
	
	u32 aRGB0  = (mux0>>20)&0x0F;	// c1 c1		// a0
	u32 bRGB0  = (mux1>>28)&0x0F;	// c1 c2		// b0
	u32 cRGB0  = (mux0>>15)&0x1F;	// c1 c3		// c0
	u32 dRGB0  = (mux1>>15)&0x07;	// c1 c4		// d0

	u32 aA0    = (mux0>>12)&0x07;	// c1 a1		// Aa0
	u32 bA0    = (mux1>>12)&0x07;	// c1 a2		// Ab0
	u32 cA0    = (mux0>>9 )&0x07;	// c1 a3		// Ac0
	u32 dA0    = (mux1>>9 )&0x07;	// c1 a4		// Ad0

	u32 aRGB1  = (mux0>>5 )&0x0F;	// c2 c1		// a1
	u32 bRGB1  = (mux1>>24)&0x0F;	// c2 c2		// b1
	u32 cRGB1  = (mux0    )&0x1F;	// c2 c3		// c1
	u32 dRGB1  = (mux1>>6 )&0x07;	// c2 c4		// d1
	
	u32 aA1    = (mux1>>21)&0x07;	// c2 a1		// Aa1
	u32 bA1    = (mux1>>3 )&0x07;	// c2 a2		// Ab1
	u32 cA1    = (mux1>>18)&0x07;	// c2 a3		// Ac1
	u32 dA1    = (mux1    )&0x07;	// c2 a4		// Ad1

	//fprintf(fh, "\n\t\tcase 0x%08x%08xLL:\n", mux0, mux1);
	//fprintf(fh, "\t\t//aRGB0: (%s - %s) * %s + %s\n", sc_colcombtypes16[aRGB0], sc_colcombtypes16[bRGB0], sc_colcombtypes32[cRGB0], sc_colcombtypes8[dRGB0]);		
	//fprintf(fh, "\t\t//aA0  : (%s - %s) * %s + %s\n", sc_colcombtypes8[aA0], sc_colcombtypes8[bA0], sc_colcombtypes8[cA0], sc_colcombtypes8[dA0]);
	//fprintf(fh, "\t\t//aRGB1: (%s - %s) * %s + %s\n", sc_colcombtypes16[aRGB1], sc_colcombtypes16[bRGB1], sc_colcombtypes32[cRGB1], sc_colcombtypes8[dRGB1]);		
	//fprintf(fh, "\t\t//aA1  : (%s - %s) * %s + %s\n", sc_colcombtypes8[aA1],  sc_colcombtypes8[bA1], sc_colcombtypes8[cA1],  sc_colcombtypes8[dA1]);

	mCycle1 = BuildCycle1( CombinerInput16[aRGB0], CombinerInput16[bRGB0], CombinerInput32[cRGB0], CombinerInput8[dRGB0] );
	mCycle1A = BuildCycle1( CombinerInputAlphaC1_8[aA0], CombinerInputAlphaC1_8[bA0], CombinerInputAlphaC1_8[cA0], CombinerInputAlphaC1_8[dA0] );

	if( two_cycles )
	{
		mCycle2 = BuildCycle2( CombinerInput16[aRGB1], CombinerInput16[bRGB1], CombinerInput32[cRGB1], CombinerInput8[dRGB1], mCycle1 );
		mCycle2A = BuildCycle2( CombinerInputAlphaC2_8[aA1], CombinerInputAlphaC2_8[bA1], CombinerInputAlphaC2_8[cA1], CombinerInputAlphaC2_8[dA1], mCycle1A );
		mBlendStates = GenerateBlendStates( mCycle2, mCycle2A );
	}
	else
	{
		mBlendStates = GenerateBlendStates( mCycle1, mCycle1A );
	}
}

//*****************************************************************************
//
//*****************************************************************************
CCombinerTree::~CCombinerTree()
{
	delete mCycle1;
	delete mCycle1A;
	delete mCycle2;
	delete mCycle2A;
}


//*****************************************************************************
//
//*****************************************************************************
CCombinerOperand *	CCombinerTree::BuildCycle1( ECombinerInput a, ECombinerInput b, ECombinerInput c, ECombinerInput d )
{
	CCombinerOperand *	input_a( new CCombinerInput( a ) );
	CCombinerOperand *	input_b( new CCombinerInput( b ) );
	CCombinerOperand *	input_c( new CCombinerInput( c ) );
	CCombinerOperand *	input_d( new CCombinerInput( d ) );

	return Build( input_a, input_b, input_c, input_d );
}

//*****************************************************************************
//
//*****************************************************************************
CCombinerOperand *	CCombinerTree::BuildCycle2( ECombinerInput a, ECombinerInput b, ECombinerInput c, ECombinerInput d, const CCombinerOperand * cycle_1_output )
{
	CCombinerOperand *	input_a( a == CI_COMBINED ? cycle_1_output->Clone() : new CCombinerInput( a ) );
	CCombinerOperand *	input_b( b == CI_COMBINED ? cycle_1_output->Clone() : new CCombinerInput( b ) );
	CCombinerOperand *	input_c( c == CI_COMBINED ? cycle_1_output->Clone() : new CCombinerInput( c ) );
	CCombinerOperand *	input_d( d == CI_COMBINED ? cycle_1_output->Clone() : new CCombinerInput( d ) );

	return Build( input_a, input_b, input_c, input_d );
}

//*****************************************************************************
//	Build an expression of the form output = (A-B)*C + D, and simplify.
//*****************************************************************************
CCombinerOperand *	CCombinerTree::Build( CCombinerOperand * a, CCombinerOperand * b, CCombinerOperand * c, CCombinerOperand * d )
{
	CCombinerSum * sum( new CCombinerSum( NULL ) );
	sum->Add( a );
	sum->Sub( b );

	CCombinerProduct * product( new CCombinerProduct( sum ) );
	product->Mul( c );

	CCombinerSum * output( new CCombinerSum( product ) );
	output->Add( d );

	return Simplify( output );
}

//*****************************************************************************
//
//*****************************************************************************
COutputStream &	CCombinerTree::Stream( COutputStream & stream ) const
{
	stream << "RGB:   "; mCycle2->Stream( stream ); stream << "\n";
	stream << "Alpha: "; mCycle2A->Stream( stream ); stream << "\n";
	return stream;
}

//*****************************************************************************
//
//*****************************************************************************
CBlendStates *	CCombinerTree::GenerateBlendStates( const CCombinerOperand * colour_operand, const CCombinerOperand * alpha_operand ) const
{
	CBlendStates *		states( new CBlendStates );

	states->SetAlphaSettings( GenerateAlphaRenderSettings( alpha_operand ) );

	GenerateRenderSettings( states, colour_operand );

	return states;
}


//*****************************************************************************
//
//*****************************************************************************
namespace
{
void	ApplyAlphaModulateTerm( CAlphaRenderSettings * settings, const CCombinerOperand * operand )
{
	if( operand->IsInput() )
	{
		const CCombinerInput *	input( static_cast< const CCombinerInput * >( operand ) );

		switch( input->GetInput() )
		{
		case CI_TEXEL0:
			settings->AddTermTexel0();
			break;
		case CI_TEXEL1:
			settings->AddTermTexel1();
			break;
		case CI_SHADE:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_SHADE ) );
			break;
		case CI_PRIMITIVE:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_PRIMITIVE ) );
			break;
		case CI_ENV:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_ENVIRONMENT ) );
			break;
		case CI_PRIMITIVE_ALPHA:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_PRIMITIVE_ALPHA ) );
			break;
		case CI_ENV_ALPHA:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_ENVIRONMENT_ALPHA ) );
			break;
		case CI_0:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_0 ) );
			break;
		case CI_1:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_1 ) );
			break;

		default:
			printf( "Unhandled Alpha Input: %s\n", GetCombinerInputName( input->GetInput() ) );
			settings->SetInexact();
			break;
		}
	}
	else
	{
		//
		//	Try to reduce to a constant term, and add that
		//
		const CBlendConstantExpression *	constant_expression( BuildConstantExpression( operand, BCE_ALLOW_SHADE ) );
		if( constant_expression != NULL )
		{
			settings->AddTermConstant( constant_expression );
		}
		else
		{
			COutputStringStream	str;
			operand->Stream( str );
			printf( "\n********************************\n" );
			printf( "Unhandled alpha - not a simple term: %s\n", str.c_str() );
			printf( "********************************\n\n" );

			settings->SetInexact();
		}
	}
}


}

//*****************************************************************************
//
//*****************************************************************************
CAlphaRenderSettings * CCombinerTree::GenerateAlphaRenderSettings( const CCombinerOperand * operand ) const
{
	COutputStringStream			str;
	operand->Stream( str );

	CAlphaRenderSettings *		settings( new CAlphaRenderSettings( str.c_str() ) );

	if(operand->IsProduct())
	{
		const CCombinerProduct *	product( static_cast< const CCombinerProduct * >( operand ) );
		for( u32 i = 0; i < product->GetNumOperands(); ++i )
		{
			ApplyAlphaModulateTerm( settings, product->GetOperand( i ) );
		}
	}
	else 
	{
		ApplyAlphaModulateTerm( settings, operand );
	}

	settings->Finalise();

	return settings;
}

//*****************************************************************************
//
//*****************************************************************************
namespace
{
void	ApplyModulateTerm( CRenderSettingsModulate * settings, const CCombinerOperand * operand )
{
	if( operand->IsInput() )
	{
		const CCombinerInput *	input( static_cast< const CCombinerInput * >( operand ) );

		switch( input->GetInput() )
		{
		case CI_TEXEL0:
			settings->AddTermTexel0();
			break;
		case CI_TEXEL1:
			settings->AddTermTexel1();
			break;
		case CI_SHADE:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_SHADE ) );
			break;
		case CI_PRIMITIVE:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_PRIMITIVE ) );
			break;
		case CI_ENV:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_ENVIRONMENT ) );
			break;
		case CI_PRIMITIVE_ALPHA:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_PRIMITIVE_ALPHA ) );
			break;
		case CI_ENV_ALPHA:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_ENVIRONMENT_ALPHA ) );
			break;
		case CI_0:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_0 ) );
			break;
		case CI_1:
			settings->AddTermConstant( new CBlendConstantExpressionValue( BC_1 ) );
			break;

		default:
			printf( "Unhandled Input: %s\n", GetCombinerInputName( input->GetInput() ) );
			settings->SetInexact();
			break;
		}
	}
	else
	{
		//
		//	Try to reduce to a constant term, and add that
		//
		const CBlendConstantExpression *	constant_expression( BuildConstantExpression( operand, BCE_ALLOW_SHADE ) );
		if( constant_expression != NULL )
		{
			settings->AddTermConstant( constant_expression );
		}
		else
		{
			COutputStringStream	str;
			operand->Stream( str );
			printf( "\n********************************\n" );
			printf( "Unhandled rgb - not a simple term: %s\n", str.c_str() );
			printf( "********************************\n\n" );

			settings->SetInexact();
		}
	}
}


}

//*****************************************************************************
//
//*****************************************************************************
void	CCombinerTree::GenerateRenderSettings( CBlendStates * states, const CCombinerOperand * operand ) const
{
	if(operand->IsInput())
	{
		COutputStringStream	str;
		operand->Stream( str );

		CRenderSettingsModulate *	settings( new CRenderSettingsModulate( str.c_str() ) );

		ApplyModulateTerm( settings, operand );

		settings->Finalise();

		states->AddColourSettings( settings );
	}
	else if(operand->IsSum())
	{
		const CCombinerSum *	sum( static_cast< const CCombinerSum * >( operand ) );

		for( u32 i = 0; i < sum->GetNumOperands(); ++i )
		{
			const CCombinerOperand *	sum_term( sum->GetOperand( i ) );

			// Recurse
			if( sum->IsTermNegated( i ) )
			{
				printf( "Negative term!!\n" );
				COutputStringStream	str;
				str << "- ";
				sum->Stream( str );
				states->AddColourSettings( new CRenderSettingsInvalid( str.c_str() ) );
			}
			else
			{
				GenerateRenderSettings( states, sum_term );
			}
		}
	}
	else if(operand->IsProduct())
	{
		const CCombinerProduct *	product( static_cast< const CCombinerProduct * >( operand ) );

		COutputStringStream	str;
		product->Stream( str );

		CRenderSettingsModulate *	settings( new CRenderSettingsModulate( str.c_str() ) );

		for( u32 i = 0; i < product->GetNumOperands(); ++i )
		{
			ApplyModulateTerm( settings, product->GetOperand( i ) );
		}

		settings->Finalise();

		states->AddColourSettings( settings );
	}
	else if(operand->IsBlend())
	{
		const CCombinerBlend *	blend( static_cast< const CCombinerBlend * >( operand ) );

		const CCombinerOperand *	operand_a( blend->GetInputA() );		// Needs to be a constant factor /w shade
		const CCombinerOperand *	operand_b( blend->GetInputB() );		// Needs to be a constant factor
		const CCombinerOperand *	operand_f( blend->GetInputF() );

		COutputStringStream	str;
		blend->Stream( str );
		bool	handled( false );

		if( operand_f->IsInput( CI_TEXEL0 ) )
		{
			const CBlendConstantExpression *		expr_a( BuildConstantExpression( operand_a, BCE_ALLOW_SHADE ) );
			const CBlendConstantExpression *		expr_b( BuildConstantExpression( operand_b, BCE_DISALLOW_SHADE ) );
			if( expr_a != NULL && expr_b != NULL )
			{
				states->AddColourSettings( new CRenderSettingsBlend( str.c_str(), expr_a, expr_b ) );
				handled = true;
			}
			else
			{
				delete expr_a;
				delete expr_b;
			}
		}
		else
		{
			const CBlendConstantExpression *		expr_a( BuildConstantExpression( operand_a, BCE_ALLOW_SHADE ) );
			const CBlendConstantExpression *		expr_b( BuildConstantExpression( operand_b, BCE_ALLOW_SHADE ) );
			const CBlendConstantExpression *		expr_f( BuildConstantExpression( operand_f, BCE_ALLOW_SHADE ) );

			if( expr_a != NULL && expr_b != NULL && expr_f != NULL )
			{
				const CBlendConstantExpressionBlend *	expr_blend( new CBlendConstantExpressionBlend( expr_a, expr_b, expr_f ) );
				CRenderSettingsModulate *				settings( new CRenderSettingsModulate( str.c_str() ) );

				settings->AddTermConstant( expr_blend );
				settings->Finalise();
				states->AddColourSettings( settings );
				handled = true;
			}
			else
			{
				delete expr_a;
				delete expr_b;
				delete expr_f;
			}
		}

		if( !handled )
		{
			printf( "CANNOT BLEND!\n" );
			states->AddColourSettings( new CRenderSettingsInvalid( str.c_str() ) );
		}

	}
	else
	{
		COutputStringStream	str;
		operand->Stream( str );

		printf( "\n********************************\n" );
		printf( "Unhandled - inner operand is not an input/product/sum: %s\n", str.c_str() );
		printf( "********************************\n\n" );

		states->AddColourSettings( new CRenderSettingsInvalid( str.c_str() ) );
	}

}

//*****************************************************************************
//
//*****************************************************************************
CCombinerOperand * CCombinerTree::Simplify( CCombinerOperand * operand )
{
	bool	did_something;
	do
	{
		CCombinerOperand *	new_tree( operand->SimplifyAndReduce() );

		did_something = !new_tree->IsEqual( *operand );
		delete operand;
		operand = new_tree;
	}
	while( did_something );

	return operand;
}
