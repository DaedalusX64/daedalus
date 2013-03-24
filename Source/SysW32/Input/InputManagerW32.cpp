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

// Taken from DX8 sample

#include "stdafx.h"

#include "Input/InputManager.h"
#include "Debug/DBGConsole.h"
#include "SysW32/Utility/ConfigHandler.h"
#include "Utility/Mutex.h"
#include "Utility/Synchroniser.h"
#include "Utility/IO.h"
#include "Resources/resource.h"
#include "SysW32/Interface/CheckBox.h"
#include "Interface/MainWindow.h"

#define DIRECTINPUT_VERSION         DIRECTINPUT_HEADER_VERSION
#include <dinput.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)  { if(p) { (p)->Release();     (p)=NULL; } }
#endif

enum
{
	INPUT_ID_STICK_X = 0,
	INPUT_ID_STICK_Y,
	INPUT_ID_DPAD_UP,
	INPUT_ID_DPAD_DOWN,
	INPUT_ID_DPAD_LEFT,
	INPUT_ID_DPAD_RIGHT,
	INPUT_ID_A_BUTTON,
	INPUT_ID_B_BUTTON,
	INPUT_ID_Z_TRIG,
	INPUT_ID_L_TRIG,
	INPUT_ID_R_TRIG,
	INPUT_ID_C_UP,
	INPUT_ID_C_DOWN,
	INPUT_ID_C_LEFT,
	INPUT_ID_C_RIGHT,
	INPUT_ID_START,

	NUM_INPUT_IDS
};

typedef std::vector< GUID > GuidVector;


typedef struct
{
	TCHAR szName[50+1];
	u16 button;		// Button - 0 for stick values
	BOOL bIsAxis;		//
} N64ButtonDescriptor;

// This structure specifies a device for each input field.
// If s_N64Buttons[i].bIsButton is true, only dwOfsA is used
// If s_N64Buttons[i].bIsButton is false, both dwOfsA and dwOfsB are used
//    (unles the user has selected an axis on the joystick for the
//     input, in which case dwOfsA is used)
typedef struct
{
	DWORD dwDevice;		// Indexes g_CurrInputConfig.guidDevices. 0 is always the keyboard
	BOOL bIsAxis;		// TRUE if dwOfsA is axis (in which case dwOfsB is ignored)

	DWORD dwOfsA;
	DWORD dwOfsB;
} ButtonAssignment;

typedef struct
{
	DWORD dwDevice;
	DWORD dwOfs;

	BOOL bIsAxis;			// Used to indicate ofs points to byte value

	TCHAR szName[MAX_PATH+1];	// Id

	// For caller to use
	DWORD dwData;
	DWORD dwData2;
} ControlID;


typedef struct
{
	TCHAR szFileName[MAX_PATH+1];

	GuidVector guidDevices;
	BOOL bConnected[4];
	ButtonAssignment buttons[4][NUM_INPUT_IDS];

} InputConfiguration;


typedef struct
{
	union
	{
		u8 keys[256];
		DIJOYSTATE2 js;
	};

	u32 bytes;

} PollData;


//
// CInputManager implementation
//

class IInputManager : public CInputManager
{
	protected:


		friend class CSingleton< CInputManager >;
		IInputManager()
		{
			m_pDI = NULL;
			m_hWnd = NULL;
			m_dwNumDevices = 0;
			m_pDeviceInstances = NULL;

			m_nUnassignedIndex = 0;
			m_nUnassignedIndex2 = 0;
			m_pIC = NULL;
			m_bIsCurrent = FALSE;

			m_pPollData = NULL;

		}

		// Some useful constants
		enum
		{
			ANALOGUE_STICK_RANGE = 80,
			INPUT_CONFIG_VER0 = 0,
			OFF_UNASSIGNED = ~0,
			INPUT_AXIS_DEADZONE = 50
		};

	public:
		virtual ~IInputManager()
		{
			if ( m_pPollData )
			{
				delete [] m_pPollData;
				m_pPollData = NULL;
			}
		}

		bool Initialise( );
		void Finalise( );

		void Unaquire();

		bool GetState( OSContPad pPad[4] );

		void Configure(HWND hWndParent);

	protected:
		void NewConfig(InputConfiguration & ic);
		LONG AddConfig(InputConfiguration & ic);
		void RenameConfig(InputConfiguration & ic, LPCTSTR szNewName);
		BOOL GetConfig(LONG iConfig, InputConfiguration & ic) const;
		void SetConfig(LONG iConfig, InputConfiguration & ic);
		void LoadAllInputConfigs();
		HRESULT LoadConfig(InputConfiguration & ic);
		HRESULT SaveConfig(LONG iConfig);			// Can't be const - needs RemoveUnusedDevices

		HRESULT GenerateControlList(InputConfiguration & ic);
		void ResetConfig(InputConfiguration & ic);
		void AddMissingDevices(GuidVector & guids);


		static BOOL IsObjectOnExcludeList( DWORD dwOfs );
		HRESULT IIDToAsciiString(REFGUID refguid, LPSTR szStr) const;
		HRESULT AttemptDeviceCreate(REFGUID guid);

		void ReadConfig(InputConfiguration & ic);
		void WriteConfig(InputConfiguration & ic);
		HRESULT CreateDevices(InputConfiguration & ic);
		void FreeDevices();

		void RemoveUnusedDevices(InputConfiguration & ic);


		HRESULT FindPressedInputInit( );
		HRESULT FindPressedInputCheck( );
		HRESULT PollDevice( LPDIRECTINPUTDEVICE8 pDev, PollData * pd );

		//
		// Static enumeration procs
		//
		static BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext );
		static BOOL CALLBACK EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext );
		static BOOL CALLBACK EnumKeyboardKeys( const DIDEVICEOBJECTINSTANCE* pdidoi, LPVOID pContext );
		static BOOL CALLBACK EnumCheckButtons( const DIDEVICEOBJECTINSTANCE* pdidoi, LPVOID pContext );



		//
		// Input selection dialog
		//
		static BOOL CALLBACK InputSelectDialogProcStatic(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

		BOOL InputSelectDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

		BOOL OnInputSelect_InitDialog(HWND hWndDlg, WPARAM wParam, LPARAM lParam);
		void FillConfigCombo(HWND hWndDlg);


		//
		// Input config dialog
		//
		LONG DoConfigModal(HWND hWndParent, InputConfiguration * pIC, BOOL bIsCurrent);

		static BOOL CALLBACK InputConfigDialogProcStatic(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

		BOOL InputConfigDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


		BOOL OnInputConfig_InitDialog(HWND hWndDlg, WPARAM wParam, LPARAM lParam);

		void FillControllerCombo(HWND hWndDlg);
		void FillButtonList(HWND hWndDlg);
		void FillMapCombo(HWND hWndDlg);

		void InputDialog_AssignA(DWORD iButton, DWORD dwCont, DWORD iControl);
		void InputDialog_AssignB(DWORD iButton, DWORD dwCont, DWORD iControl);

		void InputDialog_SelectMapComboEntry(HWND hWndDl, DWORD dwCont, DWORD dwID);


	protected:
		LPDIRECTINPUT8       m_pDI;
		HWND				 m_hWnd;

		DWORD				 m_dwNumDevices;
		LPDIRECTINPUTDEVICE8 * m_pDeviceInstances;

		// This is set up either by reading the config, or by rescanning the devices
		InputConfiguration m_CurrInputConfig;

		std::vector<InputConfiguration> m_InputConfigs;

		static const N64ButtonDescriptor s_N64Buttons[NUM_INPUT_IDS];
		static const ButtonAssignment s_DefaultButtons[4][NUM_INPUT_IDS];

		//
		// Input config
		//
		std::vector< ControlID > m_ControlList;
		LONG m_nUnassignedIndex;
		LONG m_nUnassignedIndex2;
		InputConfiguration * m_pIC;
		BOOL m_bIsCurrent;

		PollData * m_pPollData;

		Mutex		mCritSect;
};

// This basically says if the n64 control is a button or a
// axis, and gives the button bits if it is a button
const N64ButtonDescriptor IInputManager::s_N64Buttons[NUM_INPUT_IDS] =
{
	{ TEXT("Analogue Stick X"), 0, TRUE },		// Analogue x-axis
	{ TEXT("Analogue Stick Y"), 0, TRUE },		// Analogue y-axis
	{ TEXT("DPad Up"), U_JPAD, FALSE },			// Up Digital
	{ TEXT("DPad Left"), L_JPAD, FALSE },		// Left Digital
	{ TEXT("DPad Right"), R_JPAD, FALSE },		// Right Digital
	{ TEXT("DPad Down"), D_JPAD, FALSE },		// Down Digital
	{ TEXT("A Button"), A_BUTTON, FALSE },		// A
	{ TEXT("B Button"), B_BUTTON, FALSE },		// B
	{ TEXT("Z Trigger"), Z_TRIG, FALSE },		// Z
	{ TEXT("Left Pan"), L_TRIG, FALSE },		// L Trig
	{ TEXT("Right Pan"), R_TRIG, FALSE },		// R Trig
	{ TEXT("C Up"), U_CBUTTONS, FALSE },		// Up C
	{ TEXT("C Left"), L_CBUTTONS, FALSE },		// Left C
	{ TEXT("C Right"), R_CBUTTONS, FALSE },		// Right C
	{ TEXT("C Down"), D_CBUTTONS, FALSE },		// Down C
	{ TEXT("Start"), START_BUTTON, FALSE },		// Start
};

// In the default assignment, all buttons are assigned to the keyboard
const ButtonAssignment IInputManager::s_DefaultButtons[4][NUM_INPUT_IDS] =
{
	// Controller0
	{
		// 0 is for keyboard, by default input device 0
		{ 0, FALSE, DIK_LEFT,	DIK_RIGHT },
		{ 0, FALSE, DIK_UP,		DIK_DOWN },
		{ 0, FALSE, DIK_T,		0 },			// Up Digital
		{ 0, FALSE, DIK_F,		0 },			// Left Digital
		{ 0, FALSE, DIK_G,		0 },			// Right Digital
		{ 0, FALSE, DIK_V,		0 },			// Down Digital
		{ 0, FALSE, DIK_A,		0 },			// A
		{ 0, FALSE, DIK_S,		0 },			// B
		{ 0, FALSE, DIK_X,		0 },			// Z
		{ 0, FALSE, DIK_LBRACKET, 0 },			// L Trig
		{ 0, FALSE, DIK_RBRACKET, 0 },			// R Trig
		{ 0, FALSE, DIK_I,		0 },			// Up C
		{ 0, FALSE, DIK_J,		0 },			// Left C
		{ 0, FALSE, DIK_K,		0 },			// Right C
		{ 0, FALSE, DIK_M,		0 },			// Down C
		{ 0, FALSE, DIK_RETURN, 0, },			// Start
	},

	// Controller1
	{
		// 0 is for keyboard, by default input device 0
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Up Digital
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Left Digital
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Right Digital
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Down Digital
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// A
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// B
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Z
		{ 0, FALSE, OFF_UNASSIGNED, OFF_UNASSIGNED },			// L Trig
		{ 0, FALSE, OFF_UNASSIGNED, OFF_UNASSIGNED },			// R Trig
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Up C
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Left C
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Right C
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Down C
		{ 0, FALSE, OFF_UNASSIGNED, OFF_UNASSIGNED, },			// Start
	},


	// Controller2
	{
		// 0 is for keyboard, by default input device 0
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Up Digital
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Left Digital
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Right Digital
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Down Digital
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// A
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// B
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Z
		{ 0, FALSE, OFF_UNASSIGNED, OFF_UNASSIGNED },			// L Trig
		{ 0, FALSE, OFF_UNASSIGNED, OFF_UNASSIGNED },			// R Trig
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Up C
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Left C
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Right C
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Down C
		{ 0, FALSE, OFF_UNASSIGNED, OFF_UNASSIGNED, },			// Start
	},


	// Controller3
	{
		// 0 is for keyboard, by default input device 0
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Up Digital
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Left Digital
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Right Digital
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Down Digital
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// A
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// B
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Z
		{ 0, FALSE, OFF_UNASSIGNED, OFF_UNASSIGNED },			// L Trig
		{ 0, FALSE, OFF_UNASSIGNED, OFF_UNASSIGNED },			// R Trig
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Up C
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Left C
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Right C
		{ 0, FALSE, OFF_UNASSIGNED,	OFF_UNASSIGNED },			// Down C
		{ 0, FALSE, OFF_UNASSIGNED, OFF_UNASSIGNED, },			// Start
	}

};





//
// CInputManager Creator
//
template<> bool	CSingleton< CInputManager >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new IInputManager();

	return true;
}







//-----------------------------------------------------------------------------
// Name: Initialise()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
bool IInputManager::Initialise( )
{
	m_hWnd = CMainWindow::Get()->GetWindow();

    // Register with the DirectInput subsystem and get a pointer
    // to a IDirectInput interface we can use.
    // Create a DInput object
	if( FAILED( DirectInput8Create( g_hInstance, DIRECTINPUT_VERSION,
									IID_IDirectInput8, (VOID**)&m_pDI, NULL ) ) )
        return false;


	AUTO_CRIT_SECT( mCritSect );

	// Try and read the current confing and initialise all the devices.
	// If this fails, reset to defaults and then recreate
	ReadConfig(m_CurrInputConfig);
	/*hr = */CreateDevices(m_CurrInputConfig);
	if (m_dwNumDevices == 0)
	{
		// Reset config to default settings
		ResetConfig(m_CurrInputConfig);
		/*hr = */CreateDevices(m_CurrInputConfig);
	}

	// At this point the devices will have been created, and
	// the initial button assigments will have been created

    return true;
}



///////////////////////////////////////////////////////////////////////////////
// Name: FreeDirectInput()
// Desc: Initialize the DirectInput variables.
///////////////////////////////////////////////////////////////////////////////
void IInputManager::Finalise()
{
	AUTO_CRIT_SECT( mCritSect );

	// Save the current config before it is destroyed
	WriteConfig(m_CurrInputConfig);
	FreeDevices();

    SAFE_RELEASE( m_pDI );
}


///////////////////////////////////////////////////////////////////////////////
// Unaquire the input focus, if we have it
///////////////////////////////////////////////////////////////////////////////
void IInputManager::Unaquire()
{
	AUTO_CRIT_SECT( mCritSect );

	// Release any other devices we have acquired
	if (m_pDeviceInstances != NULL)
	{
		for ( u32 i = 0; i < m_dwNumDevices; i++ )
		{
			if (m_pDeviceInstances[i] != NULL)
				m_pDeviceInstances[i]->Unacquire();
		}
	}
}




///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK IInputManager::EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, LPVOID pContext )
{
    LPDIRECTINPUTDEVICE8 pDev = (LPDIRECTINPUTDEVICE8)pContext;

    DIPROPRANGE diprg;
    diprg.diph.dwSize       = sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diprg.diph.dwHow        = DIPH_BYID;
    diprg.diph.dwObj        = pdidoi->dwType; // Specify the enumerated axis

	// We set the range to -80 to +80 here
    diprg.lMin              = -ANALOGUE_STICK_RANGE;
    diprg.lMax              = +ANALOGUE_STICK_RANGE;

	// Set the range for the axis
	if( FAILED( pDev->SetProperty( DIPROP_RANGE, &diprg.diph ) ) )
		return DIENUM_STOP;

    return DIENUM_CONTINUE;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
HRESULT IInputManager::IIDToAsciiString(REFGUID refguid, LPSTR szStr) const
{
	HRESULT hr;
	LPOLESTR szString;
	IMalloc * pMalloc;

	hr = StringFromIID(refguid, &szString);
	if (FAILED(hr))
		return hr;

	wsprintfA(szStr, "%S", szString);			// %S (not %s) is to convert to widechar

	// Free the buffer
	pMalloc = NULL;
	hr = CoGetMalloc(1, &pMalloc);
	if (SUCCEEDED(hr))
	{
		pMalloc->Free(szString);
		pMalloc->Release();
	}

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void IInputManager::ReadConfig(InputConfiguration & ic)
{
	HRESULT hr;
	LONG cont;
	LONG i;
	LONG n;
	TCHAR szBuffer[200+1];
	ConfigHandler * pConfig = new ConfigHandler("Input");

	AUTO_CRIT_SECT( mCritSect );

	// Copy default values to current
	ic.guidDevices.clear();
	ic.bConnected[0] = TRUE;
	ic.bConnected[1] = FALSE;
	ic.bConnected[2] = FALSE;
	ic.bConnected[3] = FALSE;
	lstrcpyn(ic.szFileName, TEXT("<current>"), MAX_PATH);
	memcpy(ic.buttons, s_DefaultButtons, sizeof(ic.buttons));

	if (pConfig != NULL)
	{
		i = 0;
		do
		{
			pConfig->ReadString("Device", szBuffer, sizeof(szBuffer), "", i);

			if (lstrlen(szBuffer) > 0)
			{
				GUID guid;
				//ComBSTR bstrGuid(szBuffer);
				OLECHAR szWBuffer[200+1];
				MultiByteToWideChar(CP_ACP, 0, szBuffer, -1, szWBuffer, 200+1);
				BSTR bstrGuid = SysAllocString(szWBuffer);

				hr = IIDFromString(bstrGuid, &guid);
				if (SUCCEEDED(hr))
					ic.guidDevices.push_back(guid);

				SysFreeString(bstrGuid);
			}

			i++;
		}
		while (lstrlen(szBuffer) > 0);

		// Now read in controls
		for (cont = 0; cont < 4; cont++)
		{
			for (i = 0; i < NUM_INPUT_IDS; i++)
			{
				TCHAR szName[200+1];
				ButtonAssignment ba;

				wsprintf(szName, "Controller %d %s", cont, s_N64Buttons[i].szName);
				pConfig->ReadString(szName, szBuffer, sizeof(szBuffer), "");

				n = sscanf(szBuffer, "%d %d %d %d",
					&ba.dwDevice,
					&ba.bIsAxis,
					&ba.dwOfsA,
					&ba.dwOfsB);

				// Only assign if fields are valid
				if (n == 4 && ba.dwDevice < ic.guidDevices.size())
				{
					ic.buttons[cont][i] = ba;
				}
			}
		}

		delete pConfig;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void IInputManager::WriteConfig(InputConfiguration & ic)
{
	HRESULT hr;
	LONG cont;
	TCHAR szBuffer[200+1];

	ConfigHandler * pConfig = new ConfigHandler("Input");

	AUTO_CRIT_SECT( mCritSect );

	if (pConfig != NULL)
	{
		// Write out device data
		for ( u32 i = 0; i < ic.guidDevices.size(); i++ )
		{
			hr = IIDToAsciiString(ic.guidDevices[i], szBuffer);
			if (SUCCEEDED(hr))
			{
				pConfig->WriteString("Device", szBuffer, i);
			}
		}

		// Write out button assignments
		for (cont = 0; cont < 4; cont++)
		{
			for ( u32 i = 0; i < NUM_INPUT_IDS; i++ )
			{
				TCHAR szName[200+1];

				wsprintf(szName, "Controller %d %s", cont, s_N64Buttons[i].szName);
				pConfig->ReadString(szName, szBuffer, sizeof(szBuffer), "");

				wsprintf(szBuffer, "%d %d %d %d",
					ic.buttons[cont][i].dwDevice,
					ic.buttons[cont][i].bIsAxis,
					ic.buttons[cont][i].dwOfsA,
					ic.buttons[cont][i].dwOfsB);
				pConfig->WriteString(szName, szBuffer);
			}
		}

		delete pConfig;
	}
}





///////////////////////////////////////////////////////////////////////////////
// Rescan for any attached devices. This resets the current config
///////////////////////////////////////////////////////////////////////////////
void IInputManager::ResetConfig(InputConfiguration & ic)
{
	AUTO_CRIT_SECT( mCritSect );

	// Always include the KeyBoard!
	ic.guidDevices.clear();
	ic.guidDevices.push_back(GUID_SysKeyboard);

	// Copy default values to current
	ic.bConnected[0] = TRUE;
	ic.bConnected[1] = FALSE;
	ic.bConnected[2] = FALSE;
	ic.bConnected[3] = FALSE;
	// Leave as default
	//lstrcpyn(ic.szFileName, TEXT("<current>"), MAX_PATH);
	memcpy(ic.buttons, s_DefaultButtons, sizeof(ic.buttons));

	// Look for any attached joysticks - this basically
	// pushes the guids of any devices we disconver on
	// to ic.guidDevices
	AddMissingDevices( ic.guidDevices );

}

///////////////////////////////////////////////////////////////////////////////
// Used by AddMissingDevices
///////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK IInputManager::EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, LPVOID pContext )
{
	GuidVector * pGuids = (GuidVector *)pContext;

	// Maybe do some checks on the type here

	// Skip any devices we've already seen
	for ( u32 i = 0; i < pGuids->size(); i++ )
	{
		REFGUID guid = pGuids->at( i );

		if (IsEqualIID(guid, pdidInstance->guidInstance))
		    return DIENUM_CONTINUE;
	}

	pGuids->push_back(pdidInstance->guidInstance);

    return DIENUM_CONTINUE;
}

///////////////////////////////////////////////////////////////////////////////
// A bit of a hack to add any missing devices to the list of guids for the config
// This ensures that we see all the devices, even if the config has been "optimised"
// at some point
///////////////////////////////////////////////////////////////////////////////
void IInputManager::AddMissingDevices( GuidVector & guids )
{
	AUTO_CRIT_SECT( mCritSect );

	// Same call as above - it checks for duplicates
	/*hr = */m_pDI->EnumDevices( DI8DEVCLASS_GAMECTRL,
								 EnumJoysticksCallback,
								 (LPVOID)&guids, DIEDFL_ATTACHEDONLY );
}



///////////////////////////////////////////////////////////////////////////////
// Unacquire and release any created devices
///////////////////////////////////////////////////////////////////////////////
void IInputManager::FreeDevices()
{
	AUTO_CRIT_SECT( mCritSect );

	if (m_pDeviceInstances != NULL)
	{
		for ( u32 i = 0; i < m_dwNumDevices; i++ )
		{
			if (m_pDeviceInstances[i] != NULL)
			{
				m_pDeviceInstances[i]->Unacquire();
				m_pDeviceInstances[i]->Release();
			}
		}
		delete [] m_pDeviceInstances;
	}
	m_dwNumDevices = 0;
}

///////////////////////////////////////////////////////////////////////////////
// IMPORTANT! Must be called from within the critical section!
///////////////////////////////////////////////////////////////////////////////
HRESULT IInputManager::CreateDevices(InputConfiguration & ic)
{
	HRESULT hr;

	// Size g_DeviceInstances to the same size
	m_dwNumDevices = ic.guidDevices.size();
	m_pDeviceInstances = new LPDIRECTINPUTDEVICE8[m_dwNumDevices];
	if (m_pDeviceInstances == NULL)
	{
		m_dwNumDevices = 0;
		return E_OUTOFMEMORY;
	}

	// Clear array
	ZeroMemory(m_pDeviceInstances, sizeof(LPDIRECTINPUTDEVICE8) * m_dwNumDevices);

	for ( u32 i = 0; i < m_dwNumDevices; i++ )
	{
		LPDIRECTINPUTDEVICE8 pDevice = NULL;

		// Device 1 is the first joystick
	    hr = m_pDI->CreateDevice( ic.guidDevices[i], &pDevice, NULL );

		if (SUCCEEDED(hr))
		{
			// Handle the keyboard differently from joysticks
			if (IsEqualIID(ic.guidDevices[i], GUID_SysKeyboard))
			{
				hr = pDevice->SetDataFormat( &c_dfDIKeyboard );
				if (FAILED(hr))
				{
					pDevice->Release();
					continue;		// Try the next device
				}

			}
			else
			{
				// Set the data format to "simple joystick" - a predefined data format
				//
				// A data format specifies which controls on a device we are interested in,
				// and how they should be reported. This tells DInput that we will be
				// passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
				hr = pDevice->SetDataFormat( &c_dfDIJoystick2 );
				if (FAILED(hr))
				{
					pDevice->Release();
					continue;		// Try the next device
				}
			}

			// Set the cooperative level. We want non-exclusive access (so our window proc
			// can still check the keyboard), and only when our window has the focus.
			hr = pDevice->SetCooperativeLevel( m_hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND );
			if (FAILED(hr))
			{
				pDevice->Release();
				continue;
			}

			// Enumerate the axes of the joystick and set the range of each axis.
			hr = pDevice->EnumObjects( EnumAxesCallback, (void*)pDevice, DIDFT_AXIS );

			if (FAILED(hr))
			{
				pDevice->Release();
				continue;
			}
			m_pDeviceInstances[i] = pDevice;
		}

	}

	return S_OK;
}


// We need this struct to pass in both control list and the device instance info
typedef struct
{
	IInputManager * pInputManager;
	DWORD dwDevice;

	TCHAR szDeviceName[MAX_PATH+1];

} EnumObjectInfo;


///////////////////////////////////////////////////////////////////////////////
// Add a key to the
///////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK IInputManager::EnumKeyboardKeys( const DIDEVICEOBJECTINSTANCE* pdidoi, LPVOID pContext )
{
	EnumObjectInfo * pEOI = (EnumObjectInfo *)pContext;

	//
	// Add the object to the list if it is a key and not in the exclude list
	//
	if (pdidoi->guidType == GUID_Key && !IsObjectOnExcludeList(pdidoi->dwOfs))
	{
		ControlID cid;

		cid.dwDevice = pEOI->dwDevice;
		cid.dwOfs = pdidoi->dwOfs;
		wsprintf(cid.szName, "%s.%s", pEOI->szDeviceName, pdidoi->tszName);
		cid.bIsAxis = FALSE;

		pEOI->pInputManager->m_ControlList.push_back(cid);
	}

    return DIENUM_CONTINUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
BOOL IInputManager::IsObjectOnExcludeList( DWORD dwOfs )
{
	return (dwOfs == DIK_PREVTRACK  ||
	    dwOfs == DIK_NEXTTRACK  ||
	    dwOfs == DIK_MUTE       ||
	    dwOfs == DIK_CALCULATOR ||
	    dwOfs == DIK_PLAYPAUSE  ||
	    dwOfs == DIK_MEDIASTOP  ||
	    dwOfs == DIK_VOLUMEDOWN ||
	    dwOfs == DIK_VOLUMEUP   ||
	    dwOfs == DIK_WEBHOME    ||
	    dwOfs == DIK_SLEEP      ||
	    dwOfs == DIK_WEBSEARCH  ||
	    dwOfs == DIK_WEBFAVORITES ||
	    dwOfs == DIK_WEBREFRESH ||
	    dwOfs == DIK_WEBSTOP    ||
	    dwOfs == DIK_WEBFORWARD ||
	    dwOfs == DIK_WEBBACK    ||
	    dwOfs == DIK_MYCOMPUTER ||
	    dwOfs == DIK_MAIL       ||
		dwOfs == DIK_WAKE		||
		dwOfs == DIK_POWER		||
	    dwOfs == DIK_MEDIASELECT);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
HRESULT IInputManager::GenerateControlList(InputConfiguration & ic)
{
	HRESULT hr;
	DIDEVICEINSTANCE di;
	DIDEVCAPS diDevCaps;

	m_ControlList.clear();

	AUTO_CRIT_SECT( mCritSect );

	for ( u32 i = 0; i < ic.guidDevices.size(); i++ )
	{
		LPDIRECTINPUTDEVICE8 pDevice = NULL;

		// Create the device
	    hr = m_pDI->CreateDevice( ic.guidDevices[i], &pDevice, NULL );

		// Skip any devices we couldn't create
		if (FAILED(hr) || pDevice == NULL)
		{
			CHAR szBuf[100];
			IIDToAsciiString(ic.guidDevices[i], szBuf);
			DBGConsole_Msg(0, "Couldn't create device %s!", szBuf);
			continue;
		}

		// Get the device info so that we can pass it into the enumeration function
		di.dwSize = sizeof(di);
		hr = pDevice->GetDeviceInfo(&di);
		if (FAILED(hr))
		{
			pDevice->Release();
			continue;
		}

		if (IsEqualIID(ic.guidDevices[i], GUID_SysKeyboard))
		{
			EnumObjectInfo eoi;

			// For the keyboard, enumerate all the buttons so that we
			// can get the diplay names
			eoi.pInputManager = this;
			eoi.dwDevice = i;
			lstrcpyn(eoi.szDeviceName, di.tszInstanceName, sizeof(eoi.szDeviceName));

			/*hr = */pDevice->EnumObjects( EnumKeyboardKeys, (LPVOID)&eoi, DIDFT_AXIS|DIDFT_BUTTON);
		}
		else
		{
			ControlID cid;

			diDevCaps.dwSize = sizeof(DIDEVCAPS);
			hr = pDevice->GetCapabilities(&diDevCaps);
			if (FAILED(hr))
			{
				pDevice->Release();
				continue;
			}

			cid.dwDevice = i;

			if (diDevCaps.dwAxes >= 1)
			{
				cid.dwOfs = DIJOFS_X;
				wsprintf(cid.szName, "%s.X Axis", di.tszInstanceName);
				cid.bIsAxis = TRUE;
				m_ControlList.push_back(cid);
			}
			if (diDevCaps.dwAxes >= 2)
			{
				cid.dwOfs = DIJOFS_Y;
				wsprintf(cid.szName, "%s.Y Axis", di.tszInstanceName);
				cid.bIsAxis = TRUE;
				m_ControlList.push_back(cid);
			}
			// Ignore Z axis

			for ( u32 b = 0; b < diDevCaps.dwButtons; b++ )
			{
				cid.dwOfs = DIJOFS_BUTTON(b);
				cid.bIsAxis = FALSE;
				wsprintf(cid.szName, "%s.Button %d", di.tszInstanceName, b);
				m_ControlList.push_back(cid);
				// Add button
			}
		}

		// Free the device
		pDevice->Release();
	}

	return S_OK;
}





///////////////////////////////////////////////////////////////////////////////
// Poll the current devices to get the input state
///////////////////////////////////////////////////////////////////////////////
bool IInputManager::GetState( OSContPad pPad[4] )
{
	LONG c;
	u32			cont;
    HRESULT     hr;
	BYTE    diks[256];   // DirectInput keyboard state buffer
	DIJOYSTATE2 js;      // DirectInput joystick state
	BYTE * pbInput;
	DWORD dwInputSize;
	LONG nAttempts;

	// Clear the initial state
	for ( cont = 0; cont < 4; cont++ )
	{
		pPad[cont].button = 0;
		pPad[cont].stick_x = 0;
		pPad[cont].stick_y = 0;
	}
/*
	if ( !m_pPollData )
	{
		FindPressedInputInit();
	}
	FindPressedInputCheck();
*/
	AUTO_CRIT_SECT( mCritSect );

	for ( u32 i = 0; i < m_dwNumDevices; i++ )
	{
		LPDIRECTINPUTDEVICE8 pDev;

		pDev = m_pDeviceInstances[i];
		// Skip any devices we couldn't create
		if (pDev == NULL)
			continue;

		hr = pDev->Poll();
		if (FAILED(hr))
		{
			// DInput is telling us that the input stream has been
			// interrupted. We aren't tracking any state between polls, so
			// we don't have any special reset that needs to be done. We
			// just re-acquire and try again.
			hr = pDev->Acquire();
			nAttempts = 0;
			while (hr == DIERR_INPUTLOST && nAttempts < 5)
			{
				DBGConsole_Msg(0, "Retrying...");
				hr = pDev->Acquire();
				nAttempts++;
			}

			// hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
			// may occur when the app is minimized or in the process of
			// switching, so just try again later
			continue;
		}

		// Treat keyboard differently - use the diks structure
		if (IsEqualIID(m_CurrInputConfig.guidDevices[i], GUID_SysKeyboard))
		{
			pbInput = diks;
			dwInputSize = sizeof(diks);
		}
		else
		{
			pbInput = (BYTE*)&js;
			dwInputSize = sizeof(js);
		}

		ZeroMemory( pbInput, dwInputSize );

		hr = pDev->GetDeviceState( dwInputSize, pbInput );
		if (FAILED(hr))
			continue;		// The device should have already been acquired

		//
		// Keyboard checking was here - now removed as Keyboard is
		// created with NONEXCLUSIVE cooperative level
		//

		// Process each controller in turn
		for (cont = 0; cont < 4; cont++)
		{
			for (c = 0; c < NUM_INPUT_IDS; c++)
			{
				DWORD dwOfsA = m_CurrInputConfig.buttons[cont][c].dwOfsA;
				DWORD dwOfsB = m_CurrInputConfig.buttons[cont][c].dwOfsB;

				// Only process this input if it is assigned to the device we're looking at
				if (m_CurrInputConfig.buttons[cont][c].dwDevice != i)
					continue;

				// Check if this input is a button or a stick
				if (!s_N64Buttons[c].bIsAxis)
				{
					// The n64 control is a button - we
					// should only use the first offset
					if (dwOfsA != OFF_UNASSIGNED && pbInput[dwOfsA] & 0x80)
					{
						pPad[cont].button |= s_N64Buttons[c].button;
					}

				}
				else
				{
					LONG val;


					val = 0;
					// The n64 control is an axis. If bIsAxis
					// is set, then .dwOfsA refers to an axis value
					if (m_CurrInputConfig.buttons[cont][c].bIsAxis)
					{
						// Val should be in the range -80 to 80 already,
						// As we set this property of the joystick during init
						if (dwOfsA != OFF_UNASSIGNED)
							val = ((LONG*)pbInput)[dwOfsA/4];
					}
					else
					{
						if (dwOfsA != OFF_UNASSIGNED && pbInput[dwOfsA] & 0x80)
							val = -ANALOGUE_STICK_RANGE;
						else if (dwOfsB != OFF_UNASSIGNED && pbInput[dwOfsB] & 0x80)
							val = +ANALOGUE_STICK_RANGE;
					}

					// Check which axis it was. Later we might want the dpad
					// and c buttons to be treated as axis too
					if (c == INPUT_ID_STICK_X)
						pPad[cont].stick_x = s8( val );
					else if (c == INPUT_ID_STICK_Y)
						pPad[cont].stick_y = s8( -val );
				}
			}
		}
	}

	// Synchronise the input - this will overwrite the real pad data when playing back input
	for(u32 cont = 0; cont < 4; cont++)
	{
		SYNCH_DATA( pPad[cont] );
	}

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// See if this object has been pressed
///////////////////////////////////////////////////////////////////////////////



typedef struct
{
	PollData * pOrig;		// Input just before test - used to see if a button is held down before the test
	PollData * pCurrent;				// Input for this Poll()

	BOOL bFound;
	BOOL bIsAxis;
	DWORD dwOfs;
	TCHAR szName[ MAX_PATH ];

} EnumCheckButtonsInfo;




BOOL CALLBACK IInputManager::EnumCheckButtons( const DIDEVICEOBJECTINSTANCE* pdidoi, LPVOID pContext )
{
	EnumCheckButtonsInfo * pInfo = (EnumCheckButtonsInfo *)pContext;

	u8 * pOrig = (u8 *)&pInfo->pOrig->keys;
	u8 * pCurrent = (u8 *)&pInfo->pCurrent->keys;

	//
	// For keys/buttons, assure that it has changed since the first sample
	//
	if (pdidoi->guidType == GUID_Key || pdidoi->guidType == GUID_Button)
	{
		DBGConsole_Msg(0, "%s", pdidoi->tszName);
		if ( (pCurrent[ pdidoi->dwOfs ] & 0x80) &&
			!(pOrig[ pdidoi->dwOfs ] & 0x80) )
		{
			pInfo->bFound = TRUE;
			pInfo->bIsAxis = FALSE;
			pInfo->dwOfs = pdidoi->dwOfs;

			wsprintf(pInfo->szName, "%s", pdidoi->tszName);

			// This button was pressed
			return DIENUM_STOP;
		}
	}
	else if (pdidoi->guidType == GUID_XAxis)
	{
		s32 val = ((s32*)pCurrent)[pdidoi->dwOfs/4];

		if ( abs(val) > INPUT_AXIS_DEADZONE )
		{
			pInfo->bFound = TRUE;
			pInfo->bIsAxis = TRUE;
			pInfo->dwOfs = pdidoi->dwOfs;

			wsprintf(pInfo->szName, "%s %d", pdidoi->tszName, val);

			// This axis was used
		    return DIENUM_STOP;
		}
	}
	else if (pdidoi->guidType == GUID_YAxis)
	{
		s32 val = ((s32*)pCurrent)[pdidoi->dwOfs/4];

		if ( abs(val) > INPUT_AXIS_DEADZONE )
		{
			pInfo->bFound = TRUE;
			pInfo->bIsAxis = TRUE;
			pInfo->dwOfs = pdidoi->dwOfs;

			wsprintf(pInfo->szName, "%s %d", pdidoi->tszName, val);

			// This axis was used
		    return DIENUM_STOP;

		}
	}

    return DIENUM_CONTINUE;
}


///////////////////////////////////////////////////////////////////////////////
// Poll the current devices to get the input state
///////////////////////////////////////////////////////////////////////////////
HRESULT IInputManager::FindPressedInputInit(  )
{
	HRESULT hr;

	if ( m_pPollData )
	{
		delete [] m_pPollData;
		m_pPollData = NULL;
	}

	m_pPollData = new PollData[ m_dwNumDevices ];
	if ( m_pPollData == NULL )
		return E_OUTOFMEMORY;

	AUTO_CRIT_SECT( mCritSect );

	// Get the original polling state
	for ( u32 i = 0; i < m_dwNumDevices; i++)
	{
		if ( m_pDeviceInstances[i] == NULL )
		{
			CMainWindow::Get()->MessageBox("No devices failed");
			continue;
		}

		hr = PollDevice( m_pDeviceInstances[i], &m_pPollData[i] );
		if (FAILED(hr))
		{
			CMainWindow::Get()->MessageBox("Original failed");
			continue;
		}

	}

	return S_OK;

}

HRESULT IInputManager::FindPressedInputCheck(  )
{
	HRESULT hr;
	DIDEVICEINSTANCE di;
	BOOL bFound = FALSE;

	AUTO_CRIT_SECT( mCritSect );

	static u32 count = 0;

	count++;

	for ( u32 i = 0; i < m_dwNumDevices; i++)
	{
		PollData pd;

		// Get the device info so that we can pass it into the enumeration function
		di.dwSize = sizeof(di);
		hr = m_pDeviceInstances[i]->GetDeviceInfo(&di);
		if (FAILED(hr))
		{
			DBGConsole_Msg(0, "GetDeviceInfo failed");
			continue;
		}

		if ( count < 30 )
		{
			hr = PollDevice( m_pDeviceInstances[i], &m_pPollData[i] );
			if (FAILED(hr))
			{
				continue;
			}


		}

		hr = PollDevice( m_pDeviceInstances[i], &pd );
		if (FAILED(hr))
		{
			DBGConsole_Msg(0, "PollDevice failed");
			continue;
		}

		EnumCheckButtonsInfo info;

		info.pOrig = &m_pPollData[i];
		info.pCurrent = &pd;
		info.bFound = FALSE;

		hr = m_pDeviceInstances[i]->EnumObjects( EnumCheckButtons, (void*)&info, DIDFT_AXIS|DIDFT_BUTTON );
		if (FAILED(hr))
		{
			DBGConsole_Msg(0, "EnumObjects failed");

			continue;
		}

		if (info.bFound)
		{
			// Something was pressed
			// Device is i
			// Ofs is info.dwOfs;
			// isAxis is info.bIsAxis;

			DBGConsole_Msg(0, "pOrig is %d, pCurrent is %d",
				m_pPollData[i].keys[ info.dwOfs ],
				pd.keys[ info.dwOfs ] );


			DBGConsole_Msg(0, "UserHit %s.%s", di.tszInstanceName, info.szName);

			bFound = TRUE;
			break;
		}
	}

	if ( bFound )
		return S_OK;

	return S_FALSE;
}



HRESULT IInputManager::PollDevice( LPDIRECTINPUTDEVICE8 pDev, PollData * pd )
{
    HRESULT     hr;
	DIDEVICEINSTANCE di;
	LONG nAttempts;


	di.dwSize = sizeof(di);
	hr = pDev->GetDeviceInfo(&di);
	if (FAILED(hr))
		return hr;


	if (IsEqualIID(di.guidProduct, GUID_SysKeyboard))
	{
		pd->bytes = sizeof(pd->keys);
	}
	else
	{
		pd->bytes = sizeof(pd->js);
	}

	hr = pDev->Poll();
	if (FAILED(hr))
	{
		// DInput is telling us that the input stream has been
		// interrupted. We aren't tracking any state between polls, so
		// we don't have any special reset that needs to be done. We
		// just re-acquire and try again.
		hr = pDev->Acquire();
		nAttempts = 0;
		while ( hr == DIERR_INPUTLOST && nAttempts < 5)
		{
			hr = pDev->Acquire();
			nAttempts++;
		}

		// hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
		// may occur when the app is minimized or in the process of
		// switching, so just try again later
		return hr;
	}

	ZeroMemory( &pd->keys, pd->bytes );
	hr = pDev->GetDeviceState( pd->bytes, &pd->keys );
	if (FAILED(hr))
	{
		DBGConsole_Msg(0, "GetDeviceState(%d, %08x) failed 0x%08x", pd->bytes, &pd->keys, hr);
		return hr;		// The device should have already been acquired
	}


	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Remove unused devices from the list
// If only the keyboard and one gamepad is referenced, all other devices will
// be stripped from the config
///////////////////////////////////////////////////////////////////////////////
void IInputManager::RemoveUnusedDevices(InputConfiguration & ic)
{
	BOOL bFound;
	DWORD iOrig;
	DWORD iFound;
	std::vector<DWORD> map;
	std::vector<GUID> newguids;

	// This was to ensure keyboard was always 0 - not needed now as we just check guids
	//map.push_back(0);		// map 0 -> 0
	//newguids.push_back(ic.guidDevices[0]);

	for ( u32 cont = 0; cont < 4; cont++ )
	{
		for ( u32 c = 0; c < NUM_INPUT_IDS; c++ )
		{
			bFound = FALSE;
			iOrig = ic.buttons[cont][c].dwDevice;

			for ( u32 e = 0; e < map.size(); e++)
			{
				if (map[e] == iOrig)
				{
					iFound = e;
					bFound = TRUE;
					break;
				}
			}

			if (!bFound)
			{
				newguids.push_back(ic.guidDevices[iOrig]);
				map.push_back(iOrig);
				iFound = map.size() - 1;
			}
			ic.buttons[cont][c].dwDevice = iFound;


		}
	}

	DBGConsole_Msg(0, "Optimized map: %d -> %d", ic.guidDevices.size(), newguids.size());

	// Copy new guids across
	ic.guidDevices.clear();
	for ( u32 i = 0; i < newguids.size(); i++ )
	{
		ic.guidDevices.push_back(newguids[i]);
	}
}



///////////////////////////////////////////////////////////////////////////////
// Attempt to create the specified device
// Display a warning if we couldn't create an instance of the specified device
///////////////////////////////////////////////////////////////////////////////
HRESULT IInputManager::AttemptDeviceCreate(REFGUID guid)
{
	HRESULT hr;
	LPDIRECTINPUTDEVICE8 pDevice = NULL;

	// Create the device
	hr = m_pDI->CreateDevice( guid, &pDevice, NULL );

	// Skip any devices we couldn't create
	if (FAILED(hr) || pDevice == NULL)
	{
		CHAR szBuf[100];
		IIDToAsciiString(guid, szBuf);
		DBGConsole_Msg(0, "Couldn't create device %s!", szBuf);
		return hr;
	}

	pDevice->Release();
	return S_OK;

}


///////////////////////////////////////////////////////////////////////////////
// Load config from specified filename
///////////////////////////////////////////////////////////////////////////////
HRESULT IInputManager::LoadConfig(InputConfiguration & ic)
{
	HRESULT hr;
	FILE * fh;
	LONG n;
	DWORD dwNumDevices;
	DWORD dwVersion;

	// Attempt to open
	fh = fopen(ic.szFileName, "rb");
	if (fh == NULL)
		return E_FAIL;

	n = fread(&dwVersion, sizeof(dwVersion), 1, fh);
	if (n != 1)
		goto fail_cleanup;

	if (dwVersion != INPUT_CONFIG_VER0)
	{
		DBGConsole_Msg(0, "Unknown version: %d", dwVersion);
		goto fail_cleanup;
	}

	// Read in devices
	n = fread(&dwNumDevices, sizeof(dwNumDevices), 1, fh);
	if (n != 1)
		goto fail_cleanup;

	ic.guidDevices.clear();
	for ( u32 i = 0; i < dwNumDevices; i++ )
	{
		GUID guid;

		n = fread(&guid, sizeof(guid), 1, fh);
		if (n != 1)
			goto fail_cleanup;

		hr = AttemptDeviceCreate(guid);
		if (FAILED(hr))
			goto fail_cleanup;

		ic.guidDevices.push_back(guid);
	}

	// Read in connected/unconnected
	n = fread(ic.bConnected, sizeof(BOOL), 4, fh);
	if (n != 4)
		goto fail_cleanup;

	// Read in buttons
	n = fread(ic.buttons, sizeof(ic.buttons), 1, fh);
	if (n != 1)
		goto fail_cleanup;

	// Remove any devices that are needed
	RemoveUnusedDevices(ic);


	fclose(fh);
	return S_OK;

fail_cleanup:
	fclose(fh);
	return E_FAIL;
}




///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
HRESULT IInputManager::SaveConfig(LONG iConfig)
{
	FILE * fh;
	DWORD dwNumDevices;
	DWORD dwVersion;

	// Check bounds!
	if (iConfig < 0 || u32( iConfig ) >= m_InputConfigs.size())
		return E_FAIL;

	InputConfiguration & ic = m_InputConfigs[iConfig];

	DBGConsole_Msg(0, "Saving Config %s", ic.szFileName);

	fh = fopen(ic.szFileName, "wb");
	if (fh == NULL)
		return E_FAIL;

	// Strip any unused devices
	RemoveUnusedDevices(ic);

	dwVersion = INPUT_CONFIG_VER0;
	fwrite(&dwVersion, sizeof(dwVersion), 1, fh);

	// Write devices
	dwNumDevices = ic.guidDevices.size();
	fwrite(&dwNumDevices, sizeof(dwNumDevices), 1, fh);

	// Write each device
	for (u32 i = 0; i < ic.guidDevices.size(); i++)
	{
		GUID guid;

		guid = ic.guidDevices[i];
		fwrite(&guid, sizeof(guid), 1, fh);
	}

	fwrite(ic.bConnected, sizeof(BOOL), 4, fh);
	fwrite(ic.buttons, sizeof(ic.buttons), 1, fh);

	fclose(fh);
	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Iterate through all files in g_szDaedalueExeDir/Input
///////////////////////////////////////////////////////////////////////////////
void IInputManager::LoadAllInputConfigs()
{
	HRESULT hr;
	TCHAR szSearch[MAX_PATH+1];
	TCHAR szInputDir[MAX_PATH+1];
	WIN32_FIND_DATA fd;
	HANDLE hFind;
	InputConfiguration ic;

	m_InputConfigs.clear();

	IO::Path::Combine(szInputDir, gDaedalusExePath, "Input");
	IO::Path::Combine(szSearch, szInputDir, "*.din");

	hFind = FindFirstFile(szSearch, &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			// Skip current/parent dirs
			if (_strcmpi(fd.cFileName, TEXT(".")) == 0 ||
				_strcmpi(fd.cFileName, TEXT("..")) == 0)
				continue;

			IO::Path::Combine(ic.szFileName, szInputDir, fd.cFileName);

			//DBGConsole_Msg(0, "Loading input config %s", szFileName);
			hr = LoadConfig(ic);
			if (SUCCEEDED(hr))
			{
				m_InputConfigs.push_back(ic);
			}
			else
			{
				DBGConsole_Msg(0, "Unable to load input config [C%s]", ic.szFileName);
			}

		} while (FindNextFile(hFind, &fd));

		FindClose(hFind);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Generate a new config based on the current config
///////////////////////////////////////////////////////////////////////////////
void IInputManager::NewConfig(InputConfiguration & ic)
{
	// Copy the default config
	ic = m_CurrInputConfig;

	IO::Path::Combine(ic.szFileName, gDaedalusExePath, "Input");
	IO::Path::Append(ic.szFileName, TEXT("NewConfig.din"));

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
LONG IInputManager::AddConfig(InputConfiguration & ic)
{
	// Name properly!
	TCHAR szNewFileName[MAX_PATH+1];

	IO::Path::Combine(szNewFileName, gDaedalusExePath, "Input");
	IO::Path::Append(szNewFileName, ic.szFileName);
	IO::Path::AddExtension(szNewFileName, TEXT(".din"));

	lstrcpyn(ic.szFileName, szNewFileName, MAX_PATH);
	m_InputConfigs.push_back(ic);

	// Return the index we inserted at
	return m_InputConfigs.size() - 1;
}


///////////////////////////////////////////////////////////////////////////////
// Utility to change the name of the specified config
// This essentially updates the field in ic, and
// renames the config file on disk
///////////////////////////////////////////////////////////////////////////////
void IInputManager::RenameConfig(InputConfiguration & ic, LPCTSTR szNewName)
{
	TCHAR szOrigName[MAX_PATH+1];
	TCHAR szNewFileName[MAX_PATH+1];

	IO::Path::Combine(szOrigName, gDaedalusExePath, "Input");
	IO::Path::Append(szOrigName, ic.szFileName);
	IO::Path::AddExtension(szOrigName, TEXT(".din"));

	IO::Path::Combine(szNewFileName, gDaedalusExePath, "Input");
	IO::Path::Append(szNewFileName, szNewName);
	IO::Path::AddExtension(szNewFileName, TEXT(".din"));

	// Rename file on disk, if it exists
	IO::File::Move(szOrigName, szNewFileName);

	DBGConsole_Msg(0, "Renaming input config\n[C%s] -> [C%s]",
		szOrigName, szNewFileName);

	lstrcpyn(ic.szFileName, szNewFileName, MAX_PATH);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
BOOL IInputManager::GetConfig(LONG iConfig, InputConfiguration & ic) const
{
	if (iConfig == -1)
	{
		ic = m_CurrInputConfig;
		return TRUE;
	}
	else if (u32( iConfig ) < m_InputConfigs.size())
	{
		ic = m_InputConfigs[iConfig];
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void IInputManager::SetConfig(LONG iConfig, InputConfiguration & ic)
{
	if (iConfig == -1)
	{
		m_CurrInputConfig = ic;

		// Preserve name!
		lstrcpyn(m_CurrInputConfig.szFileName, TEXT("<current>"), MAX_PATH);

		// Recreate devices!
		/*hr = */CreateDevices(m_CurrInputConfig);
	}
	else if (u32( iConfig ) < m_InputConfigs.size())
	{
		// Hmm - need to find a better way of detecting changes
		// This picks them all up now because the dialog always
		// writes the display name to the filename field
		if (_strcmpi(m_InputConfigs[iConfig].szFileName, ic.szFileName) != 0)
		{
			RenameConfig(m_InputConfigs[iConfig], ic.szFileName);

			// Hmm hack to make sure filename is the same when it's synchronised below
			// Really should sort out the filename gubbins!
			lstrcpyn(ic.szFileName, m_InputConfigs[iConfig].szFileName, MAX_PATH);
		}
		m_InputConfigs[iConfig] = ic;
	}

}





///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Input selection dialog
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void IInputManager::Configure(HWND hWndParent)
{
	DialogBox( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_INPUTSELECT), hWndParent, InputSelectDialogProcStatic);
}

BOOL CALLBACK IInputManager::InputSelectDialogProcStatic(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return Get()->InputSelectDialogProc( hWndDlg, uMsg, wParam, lParam );
}


BOOL IInputManager::InputSelectDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bResult;
	LONG nRetVal;
	LONG nSelectedConfig;
	LONG iConfig;
	InputConfiguration ic;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		return OnInputSelect_InitDialog(hWndDlg, wParam, lParam);

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_NEW_BUTTON:
			nSelectedConfig =  ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_CONFIG_COMBO));
			if (nSelectedConfig == CB_ERR)
			{
				NewConfig(ic);
			}
			else
			{
				// Create a new configuration based on the selected config
				iConfig = ComboBox_GetItemData(GetDlgItem(hWndDlg, IDC_CONFIG_COMBO), nSelectedConfig);
				if (iConfig == -1)
				{
					NewConfig(ic);
				}
				else
				{
					TCHAR szFileName[MAX_PATH+1];

					GetConfig(iConfig, ic);

					// Change filename from abc.din to abcCopy.din
					lstrcpyn(szFileName, ic.szFileName, MAX_PATH);
					IO::Path::RemoveExtension(szFileName);
					lstrcat(szFileName, TEXT("Copy"));
					IO::Path::AddExtension(szFileName, TEXT(".din"));
					lstrcpyn(ic.szFileName, szFileName, MAX_PATH);
				}

			}

			nRetVal = DoConfigModal(hWndDlg, &ic, FALSE);
			if (nRetVal == IDOK)
			{
				// Add to global list
				iConfig = AddConfig(ic);

				SaveConfig(iConfig);

				// Refresh
				FillConfigCombo(hWndDlg);
			}

			break;
		case IDC_EDIT_BUTTON:

			//
			nSelectedConfig =  ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_CONFIG_COMBO));
			if (nSelectedConfig != CB_ERR)
			{
				BOOL bIsCurrent;

				iConfig = ComboBox_GetItemData(GetDlgItem(hWndDlg, IDC_CONFIG_COMBO), nSelectedConfig);

				bIsCurrent = iConfig == -1;

				bResult = GetConfig(iConfig, ic);
				if (bResult)
				{
					nRetVal = DoConfigModal(hWndDlg, &ic, bIsCurrent);
					if (nRetVal == IDOK)
					{
						// Update global list - works for iConfig == -1 too
						SetConfig(iConfig, ic);

						if (iConfig != -1)
							SaveConfig(iConfig);
						// Refresh
						FillConfigCombo(hWndDlg);
					}
				}
			}


			break;
		case IDOK:
			// Copy our copy of the data back, setting up the new values
			nSelectedConfig =  ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_CONFIG_COMBO));
			if (nSelectedConfig != CB_ERR)
			{
				iConfig = ComboBox_GetItemData(GetDlgItem(hWndDlg, IDC_CONFIG_COMBO), nSelectedConfig);

				bResult = GetConfig(iConfig, ic);
				if (bResult)
				{
					// Do this even if iConfig = -1. This will force the
					// input handler to recreate its devices
					SetConfig(-1, ic);
				}
			}
			EndDialog(hWndDlg, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWndDlg, IDCANCEL);
			return TRUE;
		}
		break;
	case WM_DESTROY:
		// Free resources that were allocated
		return TRUE;

	}


	return FALSE;
}

BOOL IInputManager::OnInputSelect_InitDialog(HWND hWndDlg, WPARAM wParam, LPARAM lParam)
{
	// Load up all the configuration data
	LoadAllInputConfigs();

	// Initialise the lists
	FillConfigCombo(hWndDlg);

	SetFocus(GetDlgItem(hWndDlg, IDC_CONFIG_COMBO));

	// We set the focus, return false
	return FALSE;
}

void IInputManager::FillConfigCombo(HWND hWndDlg)
{

	LONG i;
	HWND hWndCombo;
	CHAR szName[MAX_PATH+1];
	LONG nIndex;
	LONG nDefault;
	BOOL bResult;
	LONG nNumConfigs;
	InputConfiguration ic;

	hWndCombo = GetDlgItem(hWndDlg, IDC_CONFIG_COMBO);

	ComboBox_ResetContent(hWndCombo);

	nNumConfigs = m_InputConfigs.size();
	// entry -1 is the current config
	for (i = -1; i < nNumConfigs; i++)
	{
		bResult = GetConfig(i, ic);
		if (bResult)
		{
			if (i == -1)
			{
				// Just copy default name
				lstrcpyn(szName, ic.szFileName, MAX_PATH);
			}
			else
			{
				// Get name from path
				lstrcpyn(szName, IO::Path::FindFileName(ic.szFileName), MAX_PATH);
				IO::Path::RemoveExtension(szName);
			}

			nIndex = ComboBox_InsertString(hWndCombo, -1, szName);

			if (nIndex != CB_ERR && nIndex != CB_ERRSPACE)
			{
				// Set item data
				ComboBox_SetItemData(hWndCombo, nIndex, i);
			}

			if (i == -1)
				nDefault = nIndex;
		}
	}
	ComboBox_SetCurSel(hWndCombo, nDefault);

}














///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Input configuration dialog
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


LONG IInputManager::DoConfigModal(HWND hWndParent, InputConfiguration * pIC, BOOL bIsCurrent)
{
	LONG nRetVal;

	m_pIC = pIC;
	m_bIsCurrent = bIsCurrent;

	AddMissingDevices(pIC->guidDevices);

	nRetVal = DialogBox( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_INPUT), hWndParent, InputConfigDialogProcStatic);

	m_ControlList.clear();

	return nRetVal;
}

BOOL CALLBACK IInputManager::InputConfigDialogProcStatic(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return Get()->InputConfigDialogProc( hWndDlg, uMsg, wParam, lParam );
}

BOOL IInputManager::InputConfigDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bChecked;
	LONG iButton;
	LONG iController;
	LONG iControl;
	LONG nSelectedButton;
	LONG nSelectedController;
	LONG nSelectedControl;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		return OnInputConfig_InitDialog(hWndDlg, wParam, lParam);

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RESET_BUTTON:
			{
				ResetConfig(*m_pIC);

				// Update devices!
				//Input_GetConfig(-1, *m_pIC);

				// Refresh
				nSelectedController =  ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_CONTROLLER_COMBO));
				nSelectedButton = ListBox_GetCurSel(GetDlgItem(hWndDlg, IDC_BUTTON_LIST));
				if (nSelectedButton != LB_ERR)
				{
					iController = ComboBox_GetItemData(GetDlgItem(hWndDlg, IDC_CONTROLLER_COMBO), nSelectedController);
					iButton = ListBox_GetItemData(GetDlgItem(hWndDlg, IDC_BUTTON_LIST), nSelectedButton);
					InputDialog_SelectMapComboEntry(hWndDlg, iController, iButton);
				}

			}
			break;

		case IDC_C1_CHECK:
			bChecked = CheckBox_GetCheck(GetDlgItem(hWndDlg, IDC_C1_CHECK)) == BST_CHECKED;
			m_pIC->bConnected[0] = bChecked;
			break;

		case IDC_C2_CHECK:
			bChecked = CheckBox_GetCheck(GetDlgItem(hWndDlg, IDC_C2_CHECK)) == BST_CHECKED;
			m_pIC->bConnected[1] = bChecked;
			break;

		case IDC_C3_CHECK:
			bChecked = CheckBox_GetCheck(GetDlgItem(hWndDlg, IDC_C3_CHECK)) == BST_CHECKED;
			m_pIC->bConnected[2] = bChecked;
			break;

		case IDC_C4_CHECK:
			bChecked = CheckBox_GetCheck(GetDlgItem(hWndDlg, IDC_C4_CHECK)) == BST_CHECKED;
			m_pIC->bConnected[3] = bChecked;
			break;

		case IDC_BUTTON_LIST:
			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				nSelectedController =  ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_CONTROLLER_COMBO));
				nSelectedButton = ListBox_GetCurSel(GetDlgItem(hWndDlg, IDC_BUTTON_LIST));
				if (nSelectedButton != LB_ERR)
				{
					iController = ComboBox_GetItemData(GetDlgItem(hWndDlg, IDC_CONTROLLER_COMBO), nSelectedController);
					iButton = ListBox_GetItemData(GetDlgItem(hWndDlg, IDC_BUTTON_LIST), nSelectedButton);
					InputDialog_SelectMapComboEntry(hWndDlg, iController, iButton);
				}
			}
			break;
		case IDC_CONTROLLER_COMBO:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				nSelectedController =  ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_CONTROLLER_COMBO));
				nSelectedButton = ListBox_GetCurSel(GetDlgItem(hWndDlg, IDC_BUTTON_LIST));
				if (nSelectedButton != LB_ERR)
				{
					iController = ComboBox_GetItemData(GetDlgItem(hWndDlg, IDC_CONTROLLER_COMBO), nSelectedController);
					iButton = ListBox_GetItemData(GetDlgItem(hWndDlg, IDC_BUTTON_LIST), nSelectedButton);
					InputDialog_SelectMapComboEntry(hWndDlg, iController, iButton);
				}
			}
			break;

		case IDC_FIRST_MAP_COMBO:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				nSelectedController =  ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_CONTROLLER_COMBO));
				nSelectedControl = ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_FIRST_MAP_COMBO));
				nSelectedButton = ListBox_GetCurSel(GetDlgItem(hWndDlg, IDC_BUTTON_LIST));

				if (nSelectedController != CB_ERR &&
					nSelectedControl != CB_ERR &&
					nSelectedButton != LB_ERR)
				{
					iController = ComboBox_GetItemData(GetDlgItem(hWndDlg, IDC_CONTROLLER_COMBO), nSelectedController);
					iControl = ComboBox_GetItemData(GetDlgItem(hWndDlg, IDC_FIRST_MAP_COMBO), nSelectedControl);
					iButton = ListBox_GetItemData(GetDlgItem(hWndDlg, IDC_BUTTON_LIST), nSelectedButton);

					// iControl is index into m_ControlList
					InputDialog_AssignA(iButton, iController, iControl);

					// Refresh
					InputDialog_SelectMapComboEntry(hWndDlg, iController, iButton);
				}
			}
			break;

		case IDC_SECOND_MAP_COMBO:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				nSelectedController =  ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_CONTROLLER_COMBO));
				nSelectedControl = ComboBox_GetCurSel(GetDlgItem(hWndDlg, IDC_SECOND_MAP_COMBO));
				nSelectedButton = ListBox_GetCurSel(GetDlgItem(hWndDlg, IDC_BUTTON_LIST));

				if (nSelectedController != CB_ERR &&
					nSelectedControl != CB_ERR &&
					nSelectedButton != LB_ERR)
				{
					iController = ComboBox_GetItemData(GetDlgItem(hWndDlg, IDC_CONTROLLER_COMBO), nSelectedController);
					iControl = ComboBox_GetItemData(GetDlgItem(hWndDlg, IDC_SECOND_MAP_COMBO), nSelectedControl);
					iButton = ListBox_GetItemData(GetDlgItem(hWndDlg, IDC_BUTTON_LIST), nSelectedButton);

					// iControl is index into m_ControlList
					InputDialog_AssignB(iButton, iController, iControl);

					// Refresh
					//InputDialog_SelectMapComboEntry(hWndDlg, iController, iButton);
				}
			}
			break;

		case IDOK:
			{
				// Copy our copy of the data back, setting up the new values
				// Do this in SelectInput dialog now!

				// We set the szFileName param of ic. This is not quite right -
				// as the new name does not include path info.
				// When this dialog returns, the caller calls Input_SetConfig, which calls
				// Input_RenameConfig(ic, ic.szFileName) to update the filename correctly
				if (!m_bIsCurrent)
					GetDlgItemText(hWndDlg, IDC_NAME_EDIT, m_pIC->szFileName, MAX_PATH);


				EndDialog(hWndDlg, IDOK);
			}
			return TRUE;
		case IDCANCEL:
			EndDialog(hWndDlg, IDCANCEL);
			return TRUE;
		}
		break;
	case WM_DESTROY:
		// Free any plugins that were found
		return TRUE;

	}
	return FALSE;
}

BOOL IInputManager::OnInputConfig_InitDialog(HWND hWndDlg, WPARAM wParam, LPARAM lParam)
{
	TCHAR szName[MAX_PATH+1];

	// Make a copy of the data
	if (m_bIsCurrent)
	{
		lstrcpyn(szName, m_pIC->szFileName, MAX_PATH);
		EnableWindow(GetDlgItem(hWndDlg, IDC_NAME_EDIT), FALSE);
	}
	else
	{
		lstrcpyn(szName, IO::Path::FindFileName(m_pIC->szFileName), MAX_PATH);
		IO::Path::RemoveExtension(szName);
		EnableWindow(GetDlgItem(hWndDlg, IDC_NAME_EDIT), TRUE);

	}
	SetDlgItemText(hWndDlg, IDC_NAME_EDIT, szName);

	// Initialise the checkboxes
	CheckBox_SetCheck(GetDlgItem(hWndDlg, IDC_C1_CHECK), m_pIC->bConnected[0] ? BST_CHECKED : BST_UNCHECKED);
	CheckBox_SetCheck(GetDlgItem(hWndDlg, IDC_C2_CHECK), m_pIC->bConnected[1] ? BST_CHECKED : BST_UNCHECKED);
	CheckBox_SetCheck(GetDlgItem(hWndDlg, IDC_C3_CHECK), m_pIC->bConnected[2] ? BST_CHECKED : BST_UNCHECKED);
	CheckBox_SetCheck(GetDlgItem(hWndDlg, IDC_C4_CHECK), m_pIC->bConnected[3] ? BST_CHECKED : BST_UNCHECKED);


	// Initialise the lists
	FillControllerCombo(hWndDlg);
	FillMapCombo(hWndDlg);			// Have to do this before button list is filled
	FillButtonList(hWndDlg);


	SetFocus(GetDlgItem(hWndDlg, IDC_CONTROLLER_COMBO));

	// We set the focus, return false
	return FALSE;
}


void IInputManager::FillControllerCombo(HWND hWndDlg)
{

	LONG i;
	HWND hWndCombo;
	CHAR szName[100];
	LONG nIndex;
	LONG nSelection;

	hWndCombo = GetDlgItem(hWndDlg, IDC_CONTROLLER_COMBO);

	ComboBox_ResetContent(hWndCombo);

	nSelection = -1;

	for (i = 0; i < 4; i++)
	{
		wsprintf(szName, "Controller %d", i+1);

		nIndex = ComboBox_InsertString(hWndCombo, -1, szName);

		if (nIndex != CB_ERR && nIndex != CB_ERRSPACE)
		{
			// Controller 1 is the initial selection
			if (i == 0)
			{
				nSelection = nIndex;
			}

			// Set item data
			ComboBox_SetItemData(hWndCombo, nIndex, i);
		}
	}

	if (nSelection == -1)
		nSelection = 0;

	ComboBox_SetCurSel(hWndCombo, nSelection);

}

void IInputManager::FillButtonList(HWND hWndDlg)
{

	LONG i;
	HWND hWndList;
	LONG nSelection;
	LONG nIndex;

	hWndList = GetDlgItem(hWndDlg, IDC_BUTTON_LIST);

	ListBox_ResetContent(hWndList);

	// Set selected item to first in list
	nSelection = -1;

	for (i = 0; i < NUM_INPUT_IDS; i++)
	{
		nIndex = ListBox_AddString(hWndList, s_N64Buttons[i].szName);
		if (nIndex != LB_ERR && nIndex != LB_ERRSPACE)
		{
			if (i == 0)
			{
				nSelection = nIndex;
			}

			// Set item data
			ListBox_SetItemData(hWndList, nIndex, i);	// The item data is the ID value

		}
	}
	if (nSelection == -1)
		nSelection = 0;

	ListBox_SetCurSel(hWndList, nSelection);
	InputDialog_SelectMapComboEntry(hWndDlg, 0, 0);

}

static int __cdecl InputDialog_CompareControls(const void * p1, const void * p2)
{
	ControlID * cid1 = (ControlID *)p1;
	ControlID * cid2 = (ControlID *)p2;

	return (int)_strcmpi(cid1->szName, cid2->szName);
}

void IInputManager::FillMapCombo(HWND hWndDlg)
{
	HRESULT hr;
	HWND hWndCombo;
	HWND hWndCombo2;
	LONG nIndex;

	hr = GenerateControlList(*m_pIC);
	if (FAILED(hr))
		return;

	// Sort the items
	qsort(&m_ControlList[0],
		m_ControlList.size(),
		sizeof(m_ControlList[0]),
		InputDialog_CompareControls);

	hWndCombo = GetDlgItem(hWndDlg, IDC_FIRST_MAP_COMBO);
	hWndCombo2 = GetDlgItem(hWndDlg, IDC_SECOND_MAP_COMBO);

	ComboBox_ResetContent(hWndCombo);
	ComboBox_ResetContent(hWndCombo2);

	m_nUnassignedIndex = ComboBox_InsertString(hWndCombo, -1, "<Unassigned>");
	ComboBox_SetItemData(hWndCombo, m_nUnassignedIndex, -1);

	m_nUnassignedIndex2 = ComboBox_InsertString(hWndCombo2, -1, "<Unassigned>");
	ComboBox_SetItemData(hWndCombo, m_nUnassignedIndex2, -1);

	for (u32 i = 0; i < m_ControlList.size(); i++)
	{
		nIndex = ComboBox_InsertString(hWndCombo, -1, m_ControlList[i].szName);

		if (nIndex != CB_ERR && nIndex != CB_ERRSPACE)
		{
			// Set data to the index of this item
			m_ControlList[i].dwData = nIndex;

			// Set item data
			ComboBox_SetItemData(hWndCombo, nIndex, i);
		}

		nIndex = ComboBox_InsertString(hWndCombo2, -1, m_ControlList[i].szName);

		if (nIndex != CB_ERR && nIndex != CB_ERRSPACE)
		{
			// Set data to the index of this item
			m_ControlList[i].dwData2 = nIndex;

			// Set item data
			ComboBox_SetItemData(hWndCombo2, nIndex, i);
		}
	}
}

void IInputManager::InputDialog_SelectMapComboEntry(HWND hWndDlg, DWORD dwCont, DWORD dwID)
{
	HWND hWndCombo;
	HWND hWndCombo2;
	DWORD dwDevice;
	DWORD dwOfsA;
	DWORD dwOfsB;
	BOOL bNeedTwoCombos;
	BOOL bFirstIsAxis;

	// Check the ID is in range
	if (dwID >= NUM_INPUT_IDS)
		return;

	hWndCombo = GetDlgItem(hWndDlg, IDC_FIRST_MAP_COMBO);
	hWndCombo2 = GetDlgItem(hWndDlg, IDC_SECOND_MAP_COMBO);

	// Get the currently assigned control/offset
	dwDevice = m_pIC->buttons[dwCont][dwID].dwDevice;
	dwOfsA   = m_pIC->buttons[dwCont][dwID].dwOfsA;
	dwOfsB   = m_pIC->buttons[dwCont][dwID].dwOfsB;


	// Search through list of controls, looking for the correct index
	bFirstIsAxis = FALSE;
	if (dwOfsA == ~0)
	{
		// Select <unassigned> entry
		ComboBox_SetCurSel(hWndCombo, m_nUnassignedIndex);
	}
	else
	{
		for (u32 i = 0; i < m_ControlList.size(); i++)
		{
			if (m_ControlList[i].dwDevice == dwDevice &&
				m_ControlList[i].dwOfs    == dwOfsA)
			{
				// The data item is the index of the specified control
				ComboBox_SetCurSel(hWndCombo, m_ControlList[i].dwData);

				bFirstIsAxis = m_ControlList[i].bIsAxis;
				break;

			}
		}
	}

	if (s_N64Buttons[dwID].bIsAxis && !bFirstIsAxis)
		bNeedTwoCombos = TRUE;
	else
		bNeedTwoCombos = FALSE;

	// Enable the second combo if this is an axis and the first control
	// is a button
	if (bNeedTwoCombos)
	{
		EnableWindow(hWndCombo2, TRUE);
		EnableWindow(GetDlgItem(hWndDlg, IDC_SECOND_MAP_STATIC), TRUE);

		if (dwOfsB == ~0)
		{
			// Select <unassigned> entry
			ComboBox_SetCurSel(hWndCombo2, m_nUnassignedIndex2);

		}
		else
		{
			// Fill in second combo
			for (u32 i = 0; i < m_ControlList.size(); i++)
			{
				if (m_ControlList[i].dwDevice == dwDevice &&
					m_ControlList[i].dwOfs    == dwOfsB)
				{
					// The data item is the index of the specified control
					ComboBox_SetCurSel(hWndCombo2, m_ControlList[i].dwData2);
					break;

				}
			}
		}

	}
	else
	{
		EnableWindow(hWndCombo2, FALSE);
		EnableWindow(GetDlgItem(hWndDlg, IDC_SECOND_MAP_STATIC), FALSE);

		// Clear second combo?
		ComboBox_SetCurSel(hWndCombo2, m_nUnassignedIndex2);
	}


	// Not found!
}

void IInputManager::InputDialog_AssignA(DWORD iButton, DWORD dwCont, DWORD iControl)
{
	m_pIC->buttons[dwCont][iButton].dwDevice = m_ControlList[iControl].dwDevice;
	m_pIC->buttons[dwCont][iButton].dwOfsA   = m_ControlList[iControl].dwOfs;
	m_pIC->buttons[dwCont][iButton].bIsAxis  = m_ControlList[iControl].bIsAxis;

	DBGConsole_Msg(0, "Assigning %s (%d %d %d) to %s",
		m_ControlList[iControl].szName, m_pIC->buttons[dwCont][iButton].dwDevice,
		m_pIC->buttons[dwCont][iButton].dwOfsA, m_pIC->buttons[dwCont][iButton].bIsAxis,
		s_N64Buttons[iButton].szName);
}

void IInputManager::InputDialog_AssignB(DWORD iButton, DWORD dwCont, DWORD iControl)
{
	m_pIC->buttons[dwCont][iButton].dwDevice = m_ControlList[iControl].dwDevice;
	m_pIC->buttons[dwCont][iButton].dwOfsB   = m_ControlList[iControl].dwOfs;
	m_pIC->buttons[dwCont][iButton].bIsAxis  = m_ControlList[iControl].bIsAxis;

	DBGConsole_Msg(0, "Assigning %s (%d %d %d) to %s",
		m_ControlList[iControl].szName, m_pIC->buttons[dwCont][iButton].dwDevice,
		m_pIC->buttons[dwCont][iButton].dwOfsB, m_pIC->buttons[dwCont][iButton].bIsAxis,
		s_N64Buttons[iButton].szName);
}


