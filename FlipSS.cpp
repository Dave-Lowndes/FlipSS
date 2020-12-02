#include <windows.h>
#include <atlstr.h>

const char szAppName[] = "Flip Screen Saver";
#define AMOUNT_TO_MOVE_CURSOR 1

static void MyReportError( HANDLE hEvtLog, LPCTSTR pMsg )
{
	if ( hEvtLog != NULL )
	{
		ReportEvent( hEvtLog, EVENTLOG_ERROR_TYPE, 0, 0, nullptr, 1, 0, &pMsg, nullptr );
	}
}

static void MyReportInfo( HANDLE hEvtLog, LPCTSTR pMsg )
{
	if ( hEvtLog != NULL )
	{
		ReportEvent( hEvtLog, EVENTLOG_INFORMATION_TYPE, 0, 0, nullptr, 1, 0, &pMsg, nullptr );
	}
}

int CALLBACK WinMain( _In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR lpCmdLine, _In_ int /*nShowCmd*/ )
{
	int RetCode{ 0 };

	/* Parse the command line, looking for a switch */
	LPCSTR pCmd = lpCmdLine;
	enum class OptionSwitch { Report, On, Off, Start, Stop } SwitchOn = OptionSwitch::Report;	// Default = report current state
	bool bDebug = false;

	for ( ; *pCmd != '\0'; pCmd++ )
	{
		/* Is it a switch character? */
		if ( *pCmd == '/' )
		{
			/* OK, what's the command? */
			if ( 0 == _strnicmp( pCmd+1, "ON", _countof("ON")-1 ) )
			{
				pCmd += _countof( "ON" );
				SwitchOn = OptionSwitch::On;
			}
			else
			if ( 0 == _strnicmp( pCmd+1, "OFF", _countof( "OFF" )-1 ) )
			{
				pCmd += _countof( "OFF" );
				SwitchOn = OptionSwitch::Off;
			}
			else
			if ( 0 == _strnicmp( pCmd+1, "START", _countof("START")-1 ) )
			{
				pCmd += _countof( "START" );
				SwitchOn = OptionSwitch::Start;
			}
			else
			if ( 0 == _strnicmp( pCmd+1, "STOP", _countof("STOP")-1 ) )
			{
				pCmd += _countof( "STOP" );
				SwitchOn = OptionSwitch::Stop;
			}
			else
			if ( 0 == _strnicmp( pCmd+1, "DEBUG", _countof("DEBUG")-1 ) )
			{
				pCmd += _countof( "DEBUG" );
				bDebug = true;
			}
			else
			{
				CString sMsg;
				sMsg.Format( "Unrecognised command line parameter - %s", pCmd );
				MessageBox( NULL, sMsg, szAppName, MB_OK );
			}
		}
	}

	// If not debugging, the log handle will be NULL which is taken to mean "don't log"
	auto evtLog = bDebug ?
					RegisterEventSource( NULL, "FlipSS" ) :
					NULL;
	{
		CString sMsg;
		sMsg.Format( "FlipSS %d", SwitchOn );
		MyReportInfo( evtLog, sMsg );
	}

	switch ( SwitchOn )
	{
	case OptionSwitch::Report:
		/* If we don't have a command line switch, report usage & current state of the saver */
		{
			BOOL bActive;

			// Get the active/inactive setting (which doesn't appear to have any
			// visibility in the Windows 10 UI, but currently still works to
			// enable/disable the idle timer aspect of the screen saver)
			SystemParametersInfo( SPI_GETSCREENSAVEACTIVE, 0, &bActive, 0 );

			// This: https://mskb.pkisolutions.com/kb/318781 this is how to determine whether a screen saver is set
			char szMsg[260];
			{
				DWORD dwLen = _countof( szMsg ) - 1;
				// If this exists, this is the screen saver
				if ( RegGetValue( HKEY_CURRENT_USER, "Control Panel\\Desktop", "SCRNSAVE.EXE", RRF_RT_REG_SZ, NULL, szMsg, &dwLen ) != ERROR_SUCCESS )
				{
					lstrcpy( szMsg, "None" );
				}
			}

			CString sMsg;
			sMsg.Format( "Usage: FlipSS [/on, /off, /start, or /stop] [/debug (logging to the Windows event log)]\n\n"
						"The screen saver '%s' is currently %s",
							szMsg,
							bActive ? "active" : "inactive" );

			MessageBox( NULL, sMsg, szAppName, MB_OK );
		}
		break;

	case OptionSwitch::On:
	case OptionSwitch::Off:
		{
			const BOOL bActive{ SwitchOn == OptionSwitch::On };

			// Although this doesn't show any visible effect in the Windows 10
			// UI, it still seems to have the effect of enabling/disabling the
			// screen saver timer. The screen saver can still be started using
			// /start, so this only affects the automatic timer aspect.
			if ( !SystemParametersInfo( SPI_SETSCREENSAVEACTIVE, bActive, 0, SPIF_SENDWININICHANGE ) )
			{
				RetCode = GetLastError();
				CString sMsg;
				sMsg.Format( "Failed SystemParametersInfo. Error 0x%x", RetCode );
				MyReportError( evtLog, sMsg );
			}
		}
		break;

	case OptionSwitch::Stop:
		{
			auto hdsk = OpenInputDesktop( 0, FALSE, READ_CONTROL | DESKTOP_SWITCHDESKTOP );

			if ( hdsk == NULL )
			{
				RetCode = GetLastError();
				CString sMsg;
				sMsg.Format( "Failed OpenInputDesktop. Error 0x%x", RetCode );
				MyReportError( evtLog, sMsg );
			}

			if ( hdsk != NULL )
			{
				// Switch to the current desktop (the screen saver one)
				// before attempting to do things, otherwise they fail.
				if ( !SetThreadDesktop( hdsk ) )
				{
					RetCode = GetLastError();
					CString sMsg;
					sMsg.Format( "Failed SetThreadDesktop. Error 0x%x", RetCode );
					MyReportError( evtLog, sMsg );
				}
			}

			{
				POINT pt;

				/* Clear the screen saver by simulating mouse cursor movement */
				if ( GetCursorPos( &pt ) )
				{
					// Loop a number of times, moving the cursor because a single move doesn't seem to do the trick!
					for ( int cnt = 0; cnt < 7; ++cnt )
					{
						if ( SetCursorPos( pt.x + cnt*AMOUNT_TO_MOVE_CURSOR, pt.y + cnt*AMOUNT_TO_MOVE_CURSOR ) )
						{
							Sleep( 20 );
						}
						else
						{
							RetCode = GetLastError();
							CString sMsg;
							sMsg.Format( "Failed SCP. Error 0x%x", RetCode );
							MyReportError( evtLog, sMsg );
							break;
						}
					}

					// Move it back again
					SetCursorPos( pt.x, pt.y );
				}
				else
				{
					RetCode = GetLastError();
					CString sMsg;
					sMsg.Format( "Failed GCP. Error 0x%x", RetCode );
					MyReportError( evtLog, sMsg );
				}

				/* Try this old method if its still running */
				{
					HWND hSaver = FindWindow( "WindowsScreenSaverClass", NULL );

					if ( hSaver != NULL )
					{
						PostMessage( hSaver, WM_CLOSE, 0, 0 );
						MyReportInfo( evtLog, "Close screen saver window" );
					}
				}
			}

			if ( hdsk != NULL )
			{
				CloseDesktop( hdsk );
			}
		}
		break;

	case OptionSwitch::Start:
		/* Start the screen saver */
		{
			char szMsg[260];
			DWORD dwLen = _countof( szMsg )-1;
			if ( RegGetValue( HKEY_CURRENT_USER, "Control Panel\\Desktop", "SCRNSAVE.EXE", RRF_RT_REG_SZ, NULL, szMsg, &dwLen ) == ERROR_SUCCESS )
			{
				// Initialise COM for any possible case where ShellExecute may require it
				// (though it's not normally needed to invoke a screen saver).
				// Note: Intentionally discard the return value, if it fails, so be it,
				// there's nothing critical if it does!
				static_cast<void>( CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE ) );

				int ErrVal = reinterpret_cast<int>( ShellExecute( NULL, NULL, szMsg, NULL, NULL, SW_NORMAL ) );
				if ( ErrVal < 32 )
				{
					// If it's this error, it's likely because the path is system32 and a 32-bit
					// application gets redirected to where there is no copy of the file, so...
					if ( ErrVal == ERROR_FILE_NOT_FOUND )
					{
						// Replace system32 with sysnative to allow 32-bit application to side-step file system redirection
						CString str{szMsg};
						str.MakeUpper();
						str.Replace( "SYSTEM32", "sysnative" );
						/*int ErrVal = */reinterpret_cast<int>(ShellExecute( NULL, NULL, str, NULL, NULL, SW_NORMAL ));
					}

				}
				CoUninitialize();
			}
			else
			{
				HWND hWnd;
				//hWnd = GetConsoleWindow();
				//wsprintf( szMsg, "Console hWnd 0x%x", hWnd );
				//MyReportInfo( evtLog, szMsg, bDebug );

				hWnd = GetForegroundWindow();
				CString sMsg;
				sMsg.Format( "Foreground hWnd 0x%p", hWnd );
				MyReportInfo( evtLog, sMsg );

				if ( !PostMessage( hWnd, WM_SYSCOMMAND, SC_SCREENSAVE, 0 ) )
				{
					RetCode = GetLastError();
					sMsg.Format( "Failed PostMessage. Error 0x%x", RetCode );
					MyReportError( evtLog, sMsg );
				}
			}
		}
		break;
	}

	return RetCode;
}

