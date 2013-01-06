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

#include "stdafx.h"

#include "RomSettingsDialog.h"
#include "Interface/RomDB.h"
#include "Interface/CheckBox.h"

#include "Core/RomSettings.h"

#include "Utility/ResourceString.h"

//*****************************************************************************
//
//*****************************************************************************
CRomSettings::CRomSettings( const RomID & id )
{
	mRomID = id;

	if ( !CRomSettingsDB::Get()->GetSettings( id, &mRomSettings ) )
	{
		DAEDALUS_ERROR( "There are no settings present for this ROM, so how can we modify them?" );
	}
}


//*****************************************************************************
//
//*****************************************************************************
LRESULT	CRomSettings::OnOk( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
	UpdateEntry();
	EndDialog( IDOK );
	return TRUE;
}

//*****************************************************************************
//
//*****************************************************************************
LRESULT	CRomSettings::OnCancel( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
	EndDialog( IDCANCEL );
	return TRUE;
}

//*****************************************************************************
//
//*****************************************************************************
LRESULT CRomSettings::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	FillCommentCombo( mRomSettings.Comment.c_str() );

	// Initialise the controls
	SetDlgItemText(IDC_NAME_EDIT, mRomSettings.GameName.c_str());
	// Comment initialised by FillCommentCombo()
	SetDlgItemText(IDC_COMMENT2_EDIT, mRomSettings.Info.c_str());

	CheckBox_SetCheck(GetDlgItem(IDC_DYNAREC_SUPPORTED_CHECK), mRomSettings.DynarecSupported ? BST_CHECKED : BST_UNCHECKED);
// XXXX	CheckBox_SetCheck(GetDlgItem(IDC_DYNARECSTACKOPTIMISATION_CHECK), mRomSettings.DynarecStackOptimisation ? BST_CHECKED : BST_UNCHECKED);

	// Epak stuff:
	CheckBox_SetCheck( GetDlgItem( IDC_EPAK_UNKNOWN_RADIO ),	mRomSettings.ExpansionPakUsage == PAK_STATUS_UNKNOWN ?	BST_CHECKED : BST_UNCHECKED);
	CheckBox_SetCheck( GetDlgItem( IDC_EPAK_UNUSED_RADIO ),		mRomSettings.ExpansionPakUsage == PAK_UNUSED ?			BST_CHECKED : BST_UNCHECKED);
	CheckBox_SetCheck( GetDlgItem( IDC_EPAK_USED_RADIO ),		mRomSettings.ExpansionPakUsage == PAK_USED ?			BST_CHECKED : BST_UNCHECKED);
	CheckBox_SetCheck( GetDlgItem( IDC_EPAK_REQUIRED_RADIO ),	mRomSettings.ExpansionPakUsage == PAK_REQUIRED ?		BST_CHECKED : BST_UNCHECKED);

	// Eeprom stuff:
	CheckBox_SetCheck( GetDlgItem( IDC_SAVETYPE_UNK_RADIO ),    mRomSettings.SaveType == SAVE_TYPE_UNKNOWN ? BST_CHECKED : BST_UNCHECKED );
	CheckBox_SetCheck( GetDlgItem( IDC_SAVETYPE_EEP4K_RADIO ),  mRomSettings.SaveType == SAVE_TYPE_EEP4K   ? BST_CHECKED : BST_UNCHECKED );
	CheckBox_SetCheck( GetDlgItem( IDC_SAVETYPE_EEP16K_RADIO ), mRomSettings.SaveType == SAVE_TYPE_EEP16K  ? BST_CHECKED : BST_UNCHECKED );
	CheckBox_SetCheck( GetDlgItem( IDC_SAVETYPE_SRAM_RADIO ),   mRomSettings.SaveType == SAVE_TYPE_SRAM    ? BST_CHECKED : BST_UNCHECKED );
	CheckBox_SetCheck( GetDlgItem( IDC_SAVETYPE_FLASH_RADIO ),  mRomSettings.SaveType == SAVE_TYPE_FLASH   ? BST_CHECKED : BST_UNCHECKED );

	::SetFocus(GetDlgItem(IDC_NAME_EDIT));

	// We set the focus, return false
	return FALSE;
}

//*****************************************************************************
//
//*****************************************************************************
LONG CRomSettings::GetCommentType(LPCTSTR szComment)
{
	// NB: Don't use resource strings, as they might be in a different
	// language. Use the English defaults
	if (lstrcmp(szComment, TEXT("Playable")) == 0)
		return COMMENT_PLAYABLE;

	if (lstrcmp(szComment, TEXT("Almost Playable")) == 0)
		return COMMENT_ALMOSTPLAYABLE;

	if (lstrcmp(szComment, TEXT("Not Playable")) == 0)
		return COMMENT_NOTPLAYABLE;

	if (lstrcmp(szComment, TEXT("Doesn't Work")) == 0)
		return COMMENT_DOESNTWORK;

	return COMMENT_BLANK;
}

//*****************************************************************************
//
//*****************************************************************************
void CRomSettings::FillCommentCombo(const CHAR * comment )
{
	HWND hWndCombo;
	LONG nIndex;
	LONG nDefault;
	LONG nType;

	hWndCombo = GetDlgItem(IDC_COMMENT_COMBO);

	ComboBox_ResetContent(hWndCombo);

	nType = GetCommentType( comment );

	nIndex = ComboBox_InsertString(hWndCombo, -1, TEXT(""));
	ComboBox_SetItemData(hWndCombo, nIndex, COMMENT_BLANK);

	// Set the default to blank (assuming none of the others match below)
	nDefault = nIndex;

	nIndex = ComboBox_InsertString(hWndCombo, -1, CResourceString(IDS_PLAYABLE));
	ComboBox_SetItemData(hWndCombo, nIndex, COMMENT_PLAYABLE);
	if (nType == COMMENT_PLAYABLE)
		nDefault = nIndex;

	nIndex = ComboBox_InsertString(hWndCombo, -1, CResourceString(IDS_ALMOSTPLAYABLE));
	ComboBox_SetItemData(hWndCombo, nIndex, COMMENT_ALMOSTPLAYABLE);
	if (nType == COMMENT_ALMOSTPLAYABLE)
		nDefault = nIndex;

	nIndex = ComboBox_InsertString(hWndCombo, -1, CResourceString(IDS_NOTPLAYABLE));
	ComboBox_SetItemData(hWndCombo, nIndex, COMMENT_NOTPLAYABLE);
	if (nType == COMMENT_NOTPLAYABLE)
		nDefault = nIndex;

	nIndex = ComboBox_InsertString(hWndCombo, -1, CResourceString(IDS_DOESNTWORK));
	ComboBox_SetItemData(hWndCombo, nIndex, COMMENT_DOESNTWORK);
	if (nType == COMMENT_DOESNTWORK)
		nDefault = nIndex;

	ComboBox_SetCurSel(hWndCombo, nDefault);
}

//*****************************************************************************
//
//*****************************************************************************
void CRomSettings::UpdateEntry()
{
	LONG nSelComment;
	LONG nType;

	// Copy fields across
	char	text[ 200 ];
	GetDlgItemText(IDC_NAME_EDIT, text, sizeof(text));
	mRomSettings.GameName = text;

	// Get comment string
	nSelComment =  ComboBox_GetCurSel(GetDlgItem(IDC_COMMENT_COMBO));
	if (nSelComment != CB_ERR)
	{
		nType = ComboBox_GetItemData(GetDlgItem(IDC_COMMENT_COMBO), nSelComment);

		// NB: Copy English text strings - don't use resource defaults
		switch (nType)
		{
		case COMMENT_BLANK:				mRomSettings.Comment = ""; break;
		case COMMENT_PLAYABLE:			mRomSettings.Comment = "Playable"; break;
		case COMMENT_ALMOSTPLAYABLE:	mRomSettings.Comment = "Almost Playable"; break;
		case COMMENT_NOTPLAYABLE:		mRomSettings.Comment = "Not Playable"; break;
		case COMMENT_DOESNTWORK:		mRomSettings.Comment = "Doesn't Work"; break;
		// default?
		}
	}

	// Comment done above
	GetDlgItemText(IDC_COMMENT2_EDIT, text, sizeof(text));
	mRomSettings.Info = text;

	mRomSettings.DynarecSupported		= CheckBox_IsChecked(GetDlgItem(IDC_DYNAREC_SUPPORTED_CHECK));
//XXXX	mRomSettings.DynarecStackOptimisation	= CheckBox_IsChecked(GetDlgItem(IDC_DYNARECSTACKOPTIMISATION_CHECK));
	if( CheckBox_IsChecked( GetDlgItem( IDC_EPAK_UNKNOWN_RADIO ) ) )
		mRomSettings.ExpansionPakUsage = PAK_STATUS_UNKNOWN;
	else if( CheckBox_IsChecked( GetDlgItem( IDC_EPAK_UNUSED_RADIO ) ) )
		mRomSettings.ExpansionPakUsage = PAK_UNUSED;
	else if( CheckBox_IsChecked( GetDlgItem( IDC_EPAK_USED_RADIO ) ) )
		mRomSettings.ExpansionPakUsage = PAK_USED;
	else if( CheckBox_IsChecked( GetDlgItem( IDC_EPAK_REQUIRED_RADIO ) ) )
		mRomSettings.ExpansionPakUsage = PAK_REQUIRED;

	if ( CheckBox_IsChecked( GetDlgItem( IDC_SAVETYPE_UNK_RADIO ) ) )
		mRomSettings.SaveType = SAVE_TYPE_UNKNOWN;
	else if ( CheckBox_IsChecked( GetDlgItem( IDC_SAVETYPE_EEP4K_RADIO ) ) )
		mRomSettings.SaveType = SAVE_TYPE_EEP4K;
	else if ( CheckBox_IsChecked( GetDlgItem( IDC_SAVETYPE_EEP16K_RADIO ) ) )
		mRomSettings.SaveType = SAVE_TYPE_EEP16K;
	else if ( CheckBox_IsChecked( GetDlgItem( IDC_SAVETYPE_SRAM_RADIO ) ) )
		mRomSettings.SaveType = SAVE_TYPE_SRAM;
	else if ( CheckBox_IsChecked( GetDlgItem( IDC_SAVETYPE_FLASH_RADIO ) ) )
		mRomSettings.SaveType = SAVE_TYPE_FLASH;

	// Update inifile values
	CRomSettingsDB::Get()->SetSettings( mRomID, mRomSettings );
}
