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

#ifndef SINGLETON_H__
#define SINGLETON_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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
			DAEDALUS_ASSERT(mpInstance != NULL, __FUNCTION__ );

			return mpInstance;
		}


		static bool Create();


		static void Destroy()
		{
			DAEDALUS_ASSERT_Q(mpInstance != NULL);

			delete mpInstance;
			mpInstance = NULL;
		}

		inline static bool IsAvailable()
		{
			return (mpInstance != NULL);
		}

		static void Attach( T * p )
		{
			DAEDALUS_ASSERT_Q(mpInstance == NULL);

			mpInstance = p;
		}

	protected:
		static T * mpInstance;
};

template < class T > T * CSingleton< T >::mpInstance = NULL;

#endif //#ifndef SINGLETON_H__
