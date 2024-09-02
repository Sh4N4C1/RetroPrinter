#include <windows.h>
#include <sddl.h>
#include <stdio.h>
#include "Token.h"

void PrintTokenInformation(HANDLE hToken) {
    DWORD dwSize;

    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
    PTOKEN_USER pTokenUser = (PTOKEN_USER)malloc(dwSize);
    if (GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
        LPSTR lpSidString;
        ConvertSidToStringSid(pTokenUser->User.Sid, &lpSidString);
        wprintf(L"User SID: %s\n", lpSidString);
        LocalFree(lpSidString);
    }

    GetTokenInformation(hToken, TokenGroups, NULL, 0, &dwSize);
    PTOKEN_GROUPS pTokenGroups = (PTOKEN_GROUPS)malloc(dwSize);
    if (GetTokenInformation(hToken, TokenGroups, pTokenGroups, dwSize,
                            &dwSize)) {
        LPSTR lpSidString;
        for (DWORD i = 0; i < pTokenGroups->GroupCount; i++) {
            ConvertSidToStringSid(pTokenGroups->Groups[i].Sid, &lpSidString);
            wprintf(L"[+++] Group SID %d: %s\n", i + 1, lpSidString);
            LocalFree(lpSidString);
        }
    }
    free(pTokenUser);
    free(pTokenGroups);
}
