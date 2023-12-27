// PatchReadyCheck.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

void HandleLine(LPCTSTR p)
{
	// This fn assumes a specific and valid msg format.
	const TCHAR code = p[27];
	LPCTSTR lpMsg = p+88; 

	bool bMagnetReady			= _tcsstr(lpMsg, TEXT("Magnet monitoring performed successfully"))!=NULL;
	bool bStartingGUI			= _tcsicmp(lpMsg, TEXT("Starting Application Software"))==0;
	bool bBeginStopGUI			= _tcsicmp(lpMsg, TEXT("Beginning stop of Application Software"))==0;
	bool bFinishStopGUI			= _tcsicmp(lpMsg, TEXT("Finished stop of Application Software"))==0;
	bool bStartingBackground	= _tcsicmp(lpMsg, TEXT("Starting Background"))==0;
	bool bBeginStopBackground	= _tcsicmp(lpMsg, TEXT("Beginning stop of Background"))==0;
	bool bFinsihStopBackground	= _tcsicmp(lpMsg, TEXT("Finished stop of Background"))==0;
	bool bQPI					= _tcsstr(lpMsg, TEXT("QPI after correction"))!=NULL;
	bool bHumid					= _tcsstr(lpMsg, TEXT("Humidity of the examination room[%]:"))!=NULL;
	bool bTemp					= _tcsstr(lpMsg, TEXT("Temperature of the examination room[C]:"))!=NULL;
	bool bReconReady			= _tcsstr(lpMsg, TEXT("[---] Connection to data-monitoring-distributor opened"))!=NULL;
	static bool _bReconReady = false;

	if (bMagnetReady || bBeginStopBackground || bFinsihStopBackground || bStartingBackground || bStartingGUI 
		|| bReconReady || bHumid || bTemp || bBeginStopGUI || bFinishStopGUI || bQPI || code==TEXT('E'))
	{
		TCHAR date[12]; ZeroMemory(date,sizeof(date)); _tcsncpy_s(date,p+13,11); 
		TCHAR src[32]; ZeroMemory(src,sizeof(src)); _tcsncpy_s(src,p+29,31); 
		TCHAR msg[80]; ZeroMemory(msg,sizeof(msg)); _tcsncpy_s(msg,p+88,79); 
		_tprintf(TEXT("%s %c %s %s\n"),date,code,src,msg);

		if (bBeginStopBackground)
			_tprintf(TEXT("\n\nWAIT UNTIL READY TO START GUI (APPROX. 4 MIN.)\n\n"));

		if (bReconReady)
		{
			_tprintf(TEXT("\n->RECON READY\n"));
			_bReconReady = true;
		}

		if (bMagnetReady && _bReconReady)
			_tprintf(TEXT("\n->SCANNER READY\n"));

		if (bMagnetReady && _bReconReady)
			_tprintf(TEXT("\n\nREADY TO LOGOUT OR START GUI\n\n"));

		if (bFinishStopGUI)
			_tprintf(TEXT("\n\nREADY TO (DE)SELECT PATCH, LOGOUT OR START GUI\n\n"));
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	TCHAR DataBuffer[4096];
	TCHAR TheLine[1024];
#ifdef _DEBUG
	LPCTSTR lpFileName = TEXT("C:\\temp\\logcurrent.log");
#else
	LPCTSTR lpFileName = TEXT("G:\\log\\logcurrent.log");
#endif

	WIN32_FILE_ATTRIBUTE_DATA attr, tempattr;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwReportedErr = ERROR_SUCCESS;

	while (true)
	{
		DWORD nRetries = 0;
		while (hFile=CreateFile(lpFileName, GENERIC_READ, (FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE), NULL, OPEN_EXISTING, (FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN), NULL ), hFile==INVALID_HANDLE_VALUE) 
			// FILE_FLAG_NO_BUFFERING => has all kinds of negative side effects
		{
			if (++nRetries==1)
			{
				_tprintf(TEXT("Unable to open file \"%s\" for reading.\n"), lpFileName);
				_tprintf(TEXT("Will keep retrying until it exists...\n"));
			}
			Sleep(1000);
		}
		_tprintf(TEXT("opened file\n"));

#ifndef _DEBUG
		SetFilePointer(hFile, 0, NULL, FILE_END);
#endif

		GetFileAttributesEx(lpFileName,GetFileExInfoStandard,&attr);

		DWORD nBytesRead = 0;
		try
		{
			ZeroMemory((char*)TheLine,sizeof(TheLine));
			TCHAR* t0 = TheLine;
			size_t nSleep = 0;
			while (hFile!=INVALID_HANDLE_VALUE)
			{
				BOOL bReadOK = ReadFile(hFile, DataBuffer, sizeof(DataBuffer), &nBytesRead, NULL);
				DWORD err = GetLastError();
				if (!bReadOK)
					_tprintf(TEXT("ReadFile failed\n"));
				switch (err)
				{
				case ERROR_SUCCESS:
					break;

				default:
#ifndef _DEBUG
					if (err!=dwReportedErr) // only report an error once on release version
#endif
					{
						LPVOID lpMsgBuf;
						FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
						_tprintf(TEXT("failed with error %d: %s\n"), err, lpMsgBuf);
						LocalFree(lpMsgBuf);
						dwReportedErr = err;
					}
				}

				for (DWORD i=0; i<nBytesRead; i++)
				{
					const TCHAR c = DataBuffer[i];
					switch (c)
					{
					case TEXT('\n'):
						HandleLine(TheLine);
						ZeroMemory((char*)TheLine,sizeof(TheLine));
						t0 = TheLine;
						break;

					case TEXT('\r'):
						break;

					default:
						if (t0-TheLine<sizeof(TheLine))
						{
							*(t0++) = c;
							*t0 = L'\0';
						}
						break;
					}
				}
				if (nBytesRead==0)
				{
					Sleep(500); 
					nSleep++;

					if (nSleep>5 && t0!=TheLine)
					{
						_tprintf(TEXT("empty buffer at EOF\n"));
						HandleLine(TheLine);
						ZeroMemory((char*)TheLine,sizeof(TheLine));
						t0 = TheLine;
					}

					// check if the file was closed and recreated
					GetFileAttributesEx(lpFileName, GetFileExInfoStandard,&tempattr);
					if (tempattr.ftCreationTime.dwLowDateTime != attr.ftCreationTime.dwLowDateTime || tempattr.ftCreationTime.dwHighDateTime != attr.ftCreationTime.dwHighDateTime)
					{
						CloseHandle(hFile);
						hFile = INVALID_HANDLE_VALUE;
						_tprintf(TEXT("file closed\n"));
					}
				}
				else
					nSleep = 0;
			}
		}
		catch (...)
		{
			_tprintf(TEXT("Aborting after unhandled exception.\n"));
		}
	}

	return 0;
}

