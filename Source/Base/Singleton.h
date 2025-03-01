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
#include <memory>

#include "Base/Assert.h"

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

		inline static std::unique_ptr<T>& Get()
		{
			#ifdef DAEDALUS_ENABLE_ASSERTS
			DAEDALUS_ASSERT(mpInstance != nullptr, "%s", __PRETTY_FUNCTION__ );
			#endif
			return mpInstance;
		}


		static bool Create();


		static void Destroy()
		{
			#ifdef DAEDALUS_ENABLE_ASSERTS
			DAEDALUS_ASSERT_Q(mpInstance != nullptr);
			#endif
			mpInstance = nullptr;
		}

		inline static bool IsAvailable()
		{
			return (mpInstance != nullptr);
		}

		static void Attach( std::unique_ptr<T> p )
		{
			#ifdef DAEDALUS_ENABLE_ASSERTS
			DAEDALUS_ASSERT_Q(mpInstance == nullptr);
			#endif
			mpInstance = p;
		}

	protected:
		static std::unique_ptr<T> mpInstance;
};

template < class T > std::unique_ptr<T> CSingleton< T >::mpInstance = NULL;

#endif // UTILITY_SINGLETON_H_
