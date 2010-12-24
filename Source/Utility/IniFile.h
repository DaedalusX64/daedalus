/******************************************************************************
 * Copyright (C) 2001 CyRUS64 (http://www.boob.co.uk)
 * Copyright (C) 2006 StrmnNrmn
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 ******************************************************************************/

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __INIFILE_H__
#define __INIFILE_H__

//*****************************************************************************
//	
//*****************************************************************************
class CIniFileProperty
{
	public:
		virtual								~CIniFileProperty();

		virtual const char *				GetName() const = 0;
		virtual const char *				GetValue() const = 0;

		virtual bool						GetBooleanValue( bool default_value ) const = 0;
		virtual int							GetIntValue( int default_value ) const = 0;
		virtual float						GetFloatValue( float default_value ) const = 0;
};

//*****************************************************************************
//
//*****************************************************************************
class CIniFileSection
{
	public:
		virtual								~CIniFileSection();

		virtual const char *				GetName() const = 0;
		virtual bool						FindProperty( const char * p_name, const CIniFileProperty ** p_property ) const = 0;

};

//*****************************************************************************
//
//*****************************************************************************
class CIniFile
{
	public:
		virtual								~CIniFile();

		static CIniFile *					Create( const char * filename );

		virtual const CIniFileSection *		GetDefaultSection() const = 0;

		virtual u32							GetNumSections() const = 0;
		virtual const CIniFileSection *		GetSection( u32 section_idx ) const = 0;

		virtual const CIniFileSection *		GetSectionByName( const char * section_name ) const = 0;
};

#endif //__INIFILE_H__
