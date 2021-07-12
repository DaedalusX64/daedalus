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

#include "BuildOptions.h"
#include "SysPSP/HLEGraphics/Combiner/BlendConstant.h"
#include "SysPSP/HLEGraphics/Combiner/CombinerTree.h"
#include "SysPSP/HLEGraphics/Combiner/CombinerExpression.h"
#include "SysPSP/HLEGraphics/Combiner/RenderSettings.h"

#include "HLEGraphics/RDP.h"
#include "Utility/Stream.h"

//*****************************************************************************
//
//*****************************************************************************
namespace
{

const ECombinerInput	CombinerInput32[ 32 ] =
{
	ECombinerInput::CI_COMBINED,	ECombinerInput::CI_TEXEL0, 			ECombinerInput::CI_TEXEL1, 			ECombinerInput::CI_PRIMITIVE, 			ECombinerInput::CI_SHADE, ECombinerInput::CI_ENV,	
	ECombinerInput::CI_1,			ECombinerInput::CI_COMBINED_ALPHA,  ECombinerInput::CI_TEXEL0_ALPHA,	ECombinerInput::CI_TEXEL1_ALPHA,		ECombinerInput::CI_PRIMITIVE_ALPHA,
	ECombinerInput::CI_SHADE_ALPHA,	ECombinerInput::CI_ENV_ALPHA,		ECombinerInput::CI_LOD_FRACTION,	ECombinerInput::CI_PRIM_LOD_FRACTION,	ECombinerInput::CI_K5,

	ECombinerInput::CI_UNKNOWN,			ECombinerInput::CI_UNKNOWN,			ECombinerInput::CI_UNKNOWN,			ECombinerInput::CI_UNKNOWN,			ECombinerInput::CI_UNKNOWN,			
	ECombinerInput::CI_UNKNOWN,			ECombinerInput::CI_UNKNOWN,			ECombinerInput::CI_UNKNOWN,			ECombinerInput::CI_UNKNOWN,			ECombinerInput::CI_UNKNOWN,
	ECombinerInput::CI_UNKNOWN,			ECombinerInput::CI_UNKNOWN,			ECombinerInput::CI_UNKNOWN,			ECombinerInput::CI_UNKNOWN,			ECombinerInput::CI_UNKNOWN,				ECombinerInput::CI_0,
};

const ECombinerInput	CombinerInput16[ 16 ] =
{
	ECombinerInput::CI_COMBINED,		ECombinerInput::CI_TEXEL0,			ECombinerInput::CI_TEXEL1,			ECombinerInput::CI_PRIMITIVE,		ECombinerInput::CI_SHADE,			
	ECombinerInput::CI_ENV,				ECombinerInput::CI_1,				ECombinerInput::CI_COMBINED_ALPHA,  ECombinerInput::CI_TEXEL0_ALPHA,	ECombinerInput::CI_TEXEL1_ALPHA,	
	ECombinerInput::CI_PRIMITIVE_ALPHA,	ECombinerInput::CI_SHADE_ALPHA,		ECombinerInput::CI_ENV_ALPHA,		ECombinerInput::CI_LOD_FRACTION,	ECombinerInput::CI_PRIM_LOD_FRACTION,	ECombinerInput::CI_0,
};

const ECombinerInput	CombinerInput8[ 8 ] =
{
	ECombinerInput::CI_COMBINED,		ECombinerInput::CI_TEXEL0,			ECombinerInput::CI_TEXEL1,			ECombinerInput::CI_PRIMITIVE,	ECombinerInput::CI_SHADE,			ECombinerInput::CI_ENV,				
	ECombinerInput::CI_1,				ECombinerInput::CI_0,
};

const ECombinerInput	CombinerInputAlphaC1_8[ 8 ] =
{
	ECombinerInput::CI_LOD_FRACTION,		ECombinerInput::CI_TEXEL0,			ECombinerInput::CI_TEXEL1,			ECombinerInput::CI_PRIMITIVE,	ECombinerInput::CI_SHADE,			ECombinerInput::CI_ENV,				
	ECombinerInput::CI_1,					ECombinerInput::CI_0,
};

const ECombinerInput	CombinerInputAlphaC2_8[ 8 ] =
{
	ECombinerInput::CI_COMBINED,		ECombinerInput::CI_TEXEL0,			ECombinerInput::CI_TEXEL1,			ECombinerInput::CI_PRIMITIVE,	ECombinerInput::CI_SHADE,			
	ECombinerInput::CI_ENV,				ECombinerInput::CI_1,				ECombinerInput::CI_0,
};


enum EBuildConstantExpressionOptions
{
	BCE_ALLOW_SHADE,
	BCE_DISALLOW_SHADE,
};

static const CBlendConstantExpression * BuildConstantExpression( const CCombinerOperand * operand, EBuildConstantExpressionOptions options )
{
	if(operand->IsInput())
	{
		const CCombinerInput *	input( static_cast< const CCombinerInput * >( operand ) );

		switch( input->GetInput() )
		{
		case ECombinerInput::CI_PRIMITIVE:			return new CBlendConstantExpressionValue( EBlendConstant::BC_PRIMITIVE );
		case ECombinerInput::CI_ENV:				return new CBlendConstantExpressionValue( EBlendConstant::BC_ENVIRONMENT );
		case ECombinerInput::CI_PRIMITIVE_ALPHA:	return new CBlendConstantExpressionValue( EBlendConstant::BC_PRIMITIVE_ALPHA );
		case ECombinerInput::CI_ENV_ALPHA:			return new CBlendConstantExpressionValue( EBlendConstant::BC_ENVIRONMENT_ALPHA );
		case ECombinerInput::CI_1:					return new CBlendConstantExpressionValue( EBlendConstant::BC_1 );
		case ECombinerInput::CI_0:					return new CBlendConstantExpressionValue( EBlendConstant::BC_0 );
		case ECombinerInput::CI_SHADE:
			if( options == BCE_ALLOW_SHADE )
				return new CBlendConstantExpressionValue( EBlendConstant::BC_SHADE );
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
					lhs = new CBlendConstantExpressionValue( EBlendConstant::BC_0 );
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
	RDP_Combine m;	m.mux = mux;
	//fprintf(fh, "\n\t\tcase 0x%08x%08xLL:\n", mux0, mux1);
	//fprintf(fh, "\t\t//aRGB0: (%s - %s) * %s + %s\n", sc_colcombtypes16[aRGB0], sc_colcombtypes16[bRGB0], sc_colcombtypes32[cRGB0], sc_colcombtypes8[dRGB0]);
	//fprintf(fh, "\t\t//aA0  : (%s - %s) * %s + %s\n", sc_colcombtypes8[aA0], sc_colcombtypes8[bA0], sc_colcombtypes8[cA0], sc_colcombtypes8[dA0]);
	//fprintf(fh, "\t\t//aRGB1: (%s - %s) * %s + %s\n", sc_colcombtypes16[aRGB1], sc_colcombtypes16[bRGB1], sc_colcombtypes32[cRGB1], sc_colcombtypes8[dRGB1]);
	//fprintf(fh, "\t\t//aA1  : (%s - %s) * %s + %s\n", sc_colcombtypes8[aA1],  sc_colcombtypes8[bA1], sc_colcombtypes8[cA1],  sc_colcombtypes8[dA1]);

	mCycle1 = BuildCycle1( CombinerInput16[m.aRGB0], CombinerInput16[m.bRGB0], CombinerInput32[m.cRGB0], CombinerInput8[m.dRGB0] );
	mCycle1A = BuildCycle1( CombinerInputAlphaC1_8[m.aA0], CombinerInputAlphaC1_8[m.bA0], CombinerInputAlphaC1_8[m.cA0], CombinerInputAlphaC1_8[m.dA0] );

	if( two_cycles )
	{
		mCycle2 = BuildCycle2( CombinerInput16[m.aRGB1], CombinerInput16[m.bRGB1], CombinerInput32[m.cRGB1], CombinerInput8[m.dRGB1], mCycle1 );
		mCycle2A = BuildCycle2( CombinerInputAlphaC2_8[m.aA1], CombinerInputAlphaC2_8[m.bA1], CombinerInputAlphaC2_8[m.cA1], CombinerInputAlphaC2_8[m.dA1], mCycle1A );
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
	CCombinerOperand *	input_a( a == ECombinerInput::CI_COMBINED ? cycle_1_output->Clone() : new CCombinerInput( a ) );
	CCombinerOperand *	input_b( b == ECombinerInput::CI_COMBINED ? cycle_1_output->Clone() : new CCombinerInput( b ) );
	CCombinerOperand *	input_c( c == ECombinerInput::CI_COMBINED ? cycle_1_output->Clone() : new CCombinerInput( c ) );
	CCombinerOperand *	input_d( d == ECombinerInput::CI_COMBINED ? cycle_1_output->Clone() : new CCombinerInput( d ) );

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
		case ECombinerInput::CI_TEXEL0:
			settings->AddTermTexel0();
			break;
		case ECombinerInput::CI_TEXEL1:
			settings->AddTermTexel1();
			break;
		case ECombinerInput::CI_SHADE:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_SHADE ) );
			break;
		case ECombinerInput::CI_PRIMITIVE:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_PRIMITIVE ) );
			break;
		case ECombinerInput::CI_ENV:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_ENVIRONMENT ) );
			break;
		case ECombinerInput::CI_PRIMITIVE_ALPHA:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_PRIMITIVE_ALPHA ) );
			break;
		case ECombinerInput::CI_ENV_ALPHA:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_ENVIRONMENT_ALPHA ) );
			break;
		case ECombinerInput::CI_0:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_0 ) );
			break;
		case ECombinerInput::CI_1:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_1 ) );
			break;

		default:
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			printf( "Unhandled Alpha Input: %s\n", GetCombinerInputName( input->GetInput() ) );
#endif
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
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			COutputStringStream	str;
			operand->Stream( str );
			printf( "\n********************************\n" );
			printf( "Unhandled alpha - not a simple term: %s\n", str.c_str() );
			printf( "********************************\n\n" );
#endif
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
		case ECombinerInput::CI_TEXEL0:
			settings->AddTermTexel0();
			break;
		case ECombinerInput::CI_TEXEL1:
			settings->AddTermTexel1();
			break;
		case ECombinerInput::CI_SHADE:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_SHADE ) );
			break;
		case ECombinerInput::CI_PRIMITIVE:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_PRIMITIVE ) );
			break;
		case ECombinerInput::CI_ENV:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_ENVIRONMENT ) );
			break;
		case ECombinerInput::CI_PRIMITIVE_ALPHA:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_PRIMITIVE_ALPHA ) );
			break;
		case ECombinerInput::CI_ENV_ALPHA:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_ENVIRONMENT_ALPHA ) );
			break;
		case ECombinerInput::CI_0:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_0 ) );
			break;
		case ECombinerInput::CI_1:
			settings->AddTermConstant( new CBlendConstantExpressionValue( EBlendConstant::BC_1 ) );
			break;

		default:
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			printf( "Unhandled Input: %s\n", GetCombinerInputName( input->GetInput() ) );
#endif
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
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			COutputStringStream	str;
			operand->Stream( str );
			printf( "\n********************************\n" );
			printf( "Unhandled rgb - not a simple term: %s\n", str.c_str() );
			printf( "********************************\n\n" );
#endif
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

		if( operand_f->IsInput( ECombinerInput::CI_TEXEL0 ) )
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
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			printf( "CANNOT BLEND!\n" );
#endif
			states->AddColourSettings( new CRenderSettingsInvalid( str.c_str() ) );
		}

	}
	else
	{
		COutputStringStream	str;
		operand->Stream( str );
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		printf( "\n********************************\n" );
		printf( "Unhandled - inner operand is not an input/product/sum: %s\n", str.c_str() );
		printf( "********************************\n\n" );
#endif
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
