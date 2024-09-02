#pragma once
#include <windows.h>

HANDLE CreateSimplePipe(LPSTR pPipeName);
HANDLE ConnectSimplePipe(HANDLE hPipe);
DWORD WINAPI TriggerSimplePipeConnection(LPSTR pPipeName);
