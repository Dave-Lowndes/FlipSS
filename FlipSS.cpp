#include <windows.h>

const char szAppName[] = "Flip Screen Saver";

int CALLBACK WinMain( HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int )
{
	/* Parse the command line, looking for a switch */
	LPCSTR pCmd = lpCmdLine;
	char szMsg[260];
	enum { Report, On, Off, Start } SwitchOn = Report;	// Default = report current state

	for ( ; *pCmd != '\0'; pCmd++ )
	{
		/* Is it a switch character? */
		if ( *pCmd == '/' )
		{
			/* OK, what's the command? */
			if ( 0 == lstrcmpi( pCmd+1, "ON" ) )
			{
				SwitchOn = On;
			}
			else
			if ( 0 == lstrcmpi( pCmd+1, "OFF" ) )
			{
				SwitchOn = Off;
			}
			else
			if ( 0 == lstrcmpi( pCmd+1, "START" ) )
			{
				SwitchOn = Start;
			}
			else
			{
				wsprintf( szMsg, "Unrecognised command line parameter - %s", pCmd );
				MessageBox( NULL, szMsg, szAppName, MB_OK );
			}
			break;
		}
	}

	BOOL bActive;

	switch( SwitchOn )
	{
	case Report:
		/* If we don't have a command line switch, report usage & current state of the saver */
		SystemParametersInfo( SPI_GETSCREENSAVEACTIVE, 0, &bActive, 0 );

		wsprintf( szMsg, "Usage: FlipSS [/on, /off, or /start]\n\nThe screen saver is currently %s", bActive ? "active" : "inactive" );

		MessageBox( NULL, szMsg, szAppName, MB_OK );
		break;

	case On:
	case Off:
		bActive = SwitchOn == On;

		if ( 0 == SystemParametersInfo( SPI_SETSCREENSAVEACTIVE, bActive, 0, SPIF_SENDWININICHANGE ) )
		{
			MessageBox( NULL, "Failed SPI!", szAppName, MB_OK | MB_ICONHAND );
		}

		if ( !bActive )
		{
			POINT pt;

			/* Clear the screen saver by simulating mouse input */
			if ( GetCursorPos( &pt ) )
			{
				if ( 0 == SetCursorPos( pt.x+30, pt.y+30 ) )
				{
					MessageBox( NULL, "Failed SCP", szAppName, MB_OK | MB_ICONHAND );
				}
			}
			else
			{
				MessageBox( NULL, "Failed GCP", szAppName, MB_OK | MB_ICONHAND );
			}

			/* Try this old method if its still running */
			HWND hSaver = FindWindow ("WindowsScreenSaverClass", NULL);

			if ( hSaver != NULL )
			{
				PostMessage( hSaver, WM_CLOSE, 0, 0 );
				MessageBeep( MB_OK );
			}
		}
		break;

	case Start:
		/* Start the screen saver */
		PostMessage(GetForegroundWindow(), WM_SYSCOMMAND, SC_SCREENSAVE, 0);
		break;
	}

	return 0;
}

