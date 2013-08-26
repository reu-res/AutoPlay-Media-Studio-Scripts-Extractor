#pragma comment(lib, "..\\detours\\detours.lib")

#include <stdio.h>
#include <windows.h>
#include "..\detours\detours.h"

typedef int (*_luaL_loadbuffer)(void *lpLua_Statte, const char *buffer, size_t size, const char *name);
_luaL_loadbuffer orig_luaL_loadbuffer = NULL;
HMODULE hLua;

int get_filename(const char *buff, size_t size, char **name) {
	char *temp = (char *)buff;
	int len = 0;

	if (strncmp(buff, "local", 5) != 0) {
		return 0;
	}

	while (temp != buff + size) {
		if (*(WORD *)temp == MAKEWORD('\r', '\n')) {
			len = temp - buff;
			int start = 0, i = 0;
			
			while (i < len - 4) {
				if (*(DWORD *)&buff[i] == MAKELONG('--', '@@')) {
					start = i + 4;
					break;
				}
				i++;
			}
			
			*name = new char[len - start + 1 + 4];
			strncpy(*name, &buff[start], len - start);
			(*name)[len - start] = '\0';

			i = 0;
			while (i < len - start) {
				if ((*name)[i] == '>') {
					(*name)[i] = '-';
				}
				i++;
			}

			strcat(*name, ".lua");

			break;
		}
		temp++;
	}
	
	return len;
}

__declspec(dllexport) int fake_luaL_loadbuffer(void *lpLua_Statte, const char *buff, size_t size, const char *name) {
	char *szName = NULL;
	int name_len = 0;
	
	if ((name_len = get_filename(buff, size, &szName))) {
		OutputDebugString(szName);

		HANDLE hFile = CreateFileA(szName, FILE_GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0);
		if (hFile == INVALID_HANDLE_VALUE) {
			OutputDebugString("Can't create file");
		} else {
			DWORD dwBytes;
			if (!WriteFile(hFile, &buff[name_len], size - name_len, &dwBytes, NULL)) {
				OutputDebugString("Can't write to file");
			}
			CloseHandle(hFile);
		}
		
		delete [] szName;
	}
	
	return orig_luaL_loadbuffer(lpLua_Statte, buff, size, name);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	LONG error;

	if (fdwReason == DLL_PROCESS_ATTACH) {
		DetourRestoreAfterWith();
		hLua = GetModuleHandleA("lua5.1.dll");
		if (!hLua) {
			OutputDebugString("Can't found lua5.1.dll");
		} else {
			orig_luaL_loadbuffer = (_luaL_loadbuffer)GetProcAddress(hLua, "luaL_loadbuffer");
			if (!orig_luaL_loadbuffer) {
				OutputDebugString("Can't found luaL_loadbuffer");
			} else {
				DetourTransactionBegin();
				DetourUpdateThread(GetCurrentThread());
				DetourAttach(&(PVOID&)orig_luaL_loadbuffer, fake_luaL_loadbuffer);
				error = DetourTransactionCommit();
				if (error == NO_ERROR) {
					OutputDebugString("Hooked luaL_loadbuffer");
				} else {
					char *szErrorCode = new char[MAX_PATH];
					sprintf_s(szErrorCode, MAX_PATH, "Error detouring luaL_loadbuffer: %d", error);
					OutputDebugString(szErrorCode);
					delete [] szErrorCode;
				}
			}
		}
	}
	
	if (fdwReason == DLL_PROCESS_DETACH) {
		if (orig_luaL_loadbuffer) {
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourDetach(&(PVOID&)orig_luaL_loadbuffer, fake_luaL_loadbuffer);
			error = DetourTransactionCommit();
			if (error == NO_ERROR) {
				OutputDebugString("UnHooked luaL_loadbuffer");
			} else {
				char *szErrorCode = new char[MAX_PATH];
				sprintf_s(szErrorCode, MAX_PATH, "Error undetouring luaL_loadbuffer: %d", error);
				OutputDebugString(szErrorCode);
				delete [] szErrorCode;
			}
		}
    }

    return TRUE;
}
