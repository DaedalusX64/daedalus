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

#ifndef FUNCTOR_H_
#define FUNCTOR_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CFunctor
{
public:
	virtual ~CFunctor() {}
	virtual void operator()() = 0;
};

template< typename T >
class CFunctor1
{
public:
	virtual ~CFunctor1() {}
	virtual void operator()( T v ) = 0;
};

//
//	Invoke static functions with varying numbers of arguments
//
class CStaticFunctor : public CFunctor
{
public:
	CStaticFunctor( void (*function)() ) : mpFunction( function )					{}
	virtual void operator()()			{ (*mpFunction)(); }

private:
	void (*mpFunction)();
};

//
//	Invoke member functions with varying number of arguments
//
template< class B >
class CMemberFunctor : public CFunctor
{
public:
	CMemberFunctor( B * object, void (B::*function)() ) : mpObject( object ), mpFunction( function )	{}
	virtual void operator()()			{ (*mpObject.*mpFunction)(); }

private:
	B *		mpObject;
	void (B::*mpFunction)();
};

template< class B, typename T >
class CMemberFunctor1 : public CFunctor1< T >
{
public:
	CMemberFunctor1( B * object, void (B::*function)( T ) ) : mpObject( object ), mpFunction( function )	{}
	virtual void operator()( T v )		{ (*mpObject.*mpFunction)( v ); }

private:
	B *		mpObject;
	void (B::*mpFunction)( T );
};

//
// Invoke a Functor1 with a fixed argument
//
template< typename T >
class CCurriedFunctor : public CFunctor
{
public:
	CCurriedFunctor( CFunctor1< T > * functor, T value ) : mpFunctor( functor ), Value( value ) {}
	~CCurriedFunctor()					{ delete mpFunctor; }

	virtual void operator()()			{ (*mpFunctor)( Value ); }

private:
	CFunctor1< T > *		mpFunctor;
	T						Value;
};

#endif // FUNCTOR_H_
