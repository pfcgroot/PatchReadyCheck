// CreateLogfile.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	TCHAR DataBuffer[1024];
	LPCTSTR lpFileNameIn = TEXT("L:\\basic\\divi\\Ima\\parrec\\!logfiles\\AMC-Z0-MR-01\\2015-02\\log\\log201502040004.log");
	LPCTSTR lpFileNameOut = TEXT("C:\\temp\\logcurrent.log");

	DeleteFile(lpFileNameOut);
	Sleep(1000);
	HANDLE hOut = CreateFile(lpFileNameOut, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, (FILE_ATTRIBUTE_NORMAL), NULL ); //|FILE_FLAG_WRITE_THROUGH

	if (hOut == INVALID_HANDLE_VALUE) 
    { 
        _tprintf(TEXT("Terminal failure: Unable to open file \"%s\" for reading.\n"), lpFileNameOut);
        return -1;
    }

	DWORD nBytesWritten = 0;
	FILE* fIn = _tfopen(lpFileNameIn,TEXT("rt"));
	while (_fgetts(DataBuffer,sizeof(DataBuffer),fIn))
	{
		_tprintf(TEXT("%s"),DataBuffer);
		WriteFile(hOut, DataBuffer, _tcslen(DataBuffer), &nBytesWritten, NULL);
//		Sleep((600.0*rand())/RAND_MAX);
	}
	fclose(fIn);
	CloseHandle(hOut);

	return 0;
}

