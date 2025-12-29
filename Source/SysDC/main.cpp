#include <kos.h>
#include "conio/conio.h"

#include "main.h"
#include "RDP.h"
#include "ROM.h"
#include "SR.h"
#include "Memory.h"
#include "DBGConsole.h"

void outputMessage( MSG_TYPES type, char *format, ... )
{
    char buffer[1024];
    
    va_list va;
    va_start( va, format );

	vsprintf( buffer, format, va );
	strcat( buffer, "\r\n");
    printf( buffer );
    
    va_end( va );
}

int min( int a, int b )
{
    if( a < b )
        return a;
    else
        return b;
}
extern bool g_bCPURunning;
BOOL StartCPUThread(LPSTR szReason, LONG nLen);
KOS_INIT_FLAGS( INIT_DEFAULT | INIT_MALLOCSTATS );
int main( int argc, char *argv[] )
{
    // Debugausgabe initialisieren (serielles Kabel)
	dbgio_init();
	irq_enable();
    // Video-Mode festlegen
	vid_set_mode( DM_640x480_PAL_IL, PM_RGB565 );
    pvr_init_defaults();

	// Emulator initialisieren
	if (!Memory_Init()) return 0;
    
    if (FAILED(SR_Init(30000)))
		return 1;
		
    if( FAILED(RDPInit()) )
        return -1;

	// Don't care about failures
	DBGConsole_Enable(g_bShowDebug);

	// CD-Inhalt auflisten
	uint32 hnd = fs_open("/cd", O_RDONLY | O_DIR);
	if (!hnd) {
		conio_printf("Error accesing disk\n");
		return -1;
	}

	conio_init(CONIO_TTY_PVR, CONIO_INPUT_LINE);
	
	// So lange ausf�hren, bis eine ROM-Datei ausgew�hlt wurde
	bool bSelected = false;
	char strFilename[256];
	while(!bSelected)
	{
		dirent_t *ent = fs_readdir(hnd);
		if(ent == NULL)
		{
			// Verzeichnis schlie�en und anschlie�end noch einmal neu auflisten
			fs_close(hnd);
			hnd = fs_open("/cd", O_RDONLY | O_DIR);
			if (!hnd) {
				conio_printf("Error accesing disk\n");
				return -1;
			}

			ent = fs_readdir(hnd);
			if(ent == NULL)
			{
				conio_printf("Empty disk\n");
				return -1;
			}
		}

		// Bildschirm l�schen
		conio_clear();
		conio_gotoxy(0, 0);

		// Aktuelle Datei ausgeben
		conio_printf("Press Start to boot the ROM.\nPress A to list next ROM.\n");
		conio_printf(ent->name);

		// So lange ausf�hren, bis eine der beiden erlaubten Tasten gedr�ckt wird
		bool bPressed = false;
		bool bLastPressedA = false;
		while(!bPressed)
		{
			// F�r jeden angeschlossenen Kontroller ausf�hren
			MAPLE_FOREACH_BEGIN( MAPLE_FUNC_CONTROLLER, cont_state_t, st )
				// A-Taste gedr�ckt, n�chste ROM-Datei anzeigen
				if(st->buttons & CONT_A)
					bLastPressedA = true;
				if( bLastPressedA && !(st->buttons & CONT_A) )
					bPressed = true;

				// Auswahl get�tigt
				if( st->buttons & CONT_START )
				{
					// ROM-Pfad kopieren
					sprintf(strFilename, "/cd/%s", ent->name);
					bPressed = true;
					bSelected = true;
				}

			MAPLE_FOREACH_END()
		}
	}

	// Men� aufr�umen
	fs_close(hnd);
	conio_shutdown();

	ROM_LoadFile(strFilename);	
    
    // Emulation beendet --> Aufr�umen
    pvr_mem_reset();
    SR_Fini();
    ROM_Unload();
	DBGConsole_Enable(FALSE);
	Memory_Fini();
    
    return 0;
}

void printFPS()
{
	// Startzeit der Funktion speichern
	static uint64 u64Start = timer_ms_gettime64();

	// Speichert die FPS
	static int iFPS = 0;

	// Vergangene Zeit berechnen
	float fDiff = (float)( timer_ms_gettime64() - u64Start ) / 1000.0f ;

	// �berpr�fen, ob eine Sekunde vergangen ist
	if( fDiff > 1.0f )
	{
		// Wenn ja, dann die Frames per Second ausgeben
		printf( "FPS: %.2f\n", (float)iFPS / fDiff );

		// Alles wieder zur�cksetzen
		iFPS = 0;
		u64Start = timer_ms_gettime64();
	}
	else
		iFPS++;

}
