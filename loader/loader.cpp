#pragma comment(lib, "..\\detours\\detours.lib")

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include "..\detours\detours.h"

#define szDll	"extractor.dll"

int _tmain(int argc, TCHAR *argv[]) {
    if (argc != 2) {
        _tprintf(_T("usage: loader.exe <file>\n"));
        return 1;
    }
	
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	TCHAR szFullPathTarget[MAX_PATH] = {0};
	char szFullPathDll[MAX_PATH] = {0};

    GetFullPathName(argv[1], MAX_PATH, szFullPathTarget, NULL);
	GetFullPathName(szDll, MAX_PATH, szFullPathDll, NULL);
	
	if (FALSE == DetourCreateProcessWithDllA(
			szFullPathTarget, 
			NULL, 
			NULL, 
			NULL, 
			FALSE, 
			0,
			NULL, 
			NULL, 
			&si, 
			&pi, 
			szFullPathDll, 
			NULL)
		) {
		MessageBox(0, _T("Can't create process"), _T(""), MB_ICONERROR);
	}

    return 0;
}
