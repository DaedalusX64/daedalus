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

#pragma once

#ifndef UTILITY_SINGLETON_H_
#define UTILITY_SINGLETON_H_

#include <stdlib.h>

#include "Debug/DaedalusAssert.h"

//
// CSingleton is an abstract base class for classes where only one
// instance exists throughout the execution of the program
// Typical usuage is:
//
// CMyUniqueClass::Create();
// ..
// CMyUniqueClass::Get()->DoSomething();
// etc
// ..
// CMyUniqueClass::Destroy();
//
//

template < class T > class CSingleton
{
	public:
		//CSingleton();
		virtual ~CSingleton() {}

		inline static T * Get()
		{
			#ifdef DAEDALUS_ENABLE_ASSERTS
			DAEDALUS_ASSERT(mpInstance != NULL, "%s", __PRETTY_FUNCTION__ );
			#endif
			return mpInstance;
		}


		static bool Create();


		static void Destroy()
		{
			#ifdef DAEDALUS_ENABLE_ASSERTS
			DAEDALUS_ASSERT_Q(mpInstance != NULL);
			#endif
			delete mpInstance;
			mpInstance = NULL;
		}

		inline static bool IsAvailable()
		{
			return (mpInstance != NULL);
		}

		static void Attach( T * p )
		{
			#ifdef DAEDALUS_ENABLE_ASSERTS
			DAEDALUS_ASSERT_Q(mpInstance == NULL);
			#endif
			mpInstance = p;
		}

	protected:
		static T * mpInstance;
};

template < class T > T * CSingleton< T >::mpInstance = NULL;

#endif // UTILITY_SINGLETON_H_
