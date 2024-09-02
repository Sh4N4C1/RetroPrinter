#pragma once

#include <windows.h>
#include <stdio.h>

typedef enum{
    EOpenPrinter,
    ERpcRemoteFindFirstPrinterChangeNotificationEx,
    Default
}RpcFunction;

typedef struct _SERVEROPTIONS{
    DWORD ReferentId;
    DWORD MaxCount;
    DWORD Offset;
    DWORD ActualCount;
}SERVEROPTIONS, *PSERVEROPTIONS;

// rpc command ids
#define RPC_CMD_ID_OPEN_SC_MANAGER 27
#define RPC_CMD_ID_CREATE_SERVICE 24
#define RPC_CMD_ID_START_SERVICE 31
#define RPC_CMD_ID_DELETE_SERVICE 2

// rpc command output lengths
#define RPC_OUTPUT_LENGTH_OPEN_SC_MANAGER 24
#define RPC_OUTPUT_LENGTH_CREATE_SERVICE 28
#define RPC_OUTPUT_LENGTH_START_SERVICE 4
#define RPC_OUTPUT_LENGTH_DELETE_SERVICE 4

#define MAX_RPC_PACKET_LENGTH 4096
#define MAX_PROCEDURE_DATA_LENGTH 2048

#define CALC_ALIGN_PADDING(VALUE_LENGTH, ALIGN_BYTES) ((((VALUE_LENGTH + ALIGN_BYTES - 1) / ALIGN_BYTES) * ALIGN_BYTES) - VALUE_LENGTH)
typedef struct RpcBaseHeader {
    WORD wVersion;
    BYTE bPacketType;
    BYTE bPacketFlags;
    DWORD dwDataRepresentation;
    WORD wFragLength;
    WORD wAuthLength;
    DWORD dwCallIndex;
} RpcBaseHeaderStruct;

typedef struct RpcRequestHeader {
    DWORD dwAllocHint;
    WORD wContextID;
    WORD wProcedureNumber;
} RpcRequestHeaderStruct;

typedef struct RpcResponseHeader {
    DWORD dwAllocHint;
    WORD wContextID;
    BYTE bCancelCount;
    BYTE bAlign[1];
} RpcResponseHeaderStruct;

typedef struct RpcBindRequestContextEntryStruct {
    WORD wContextID;
    WORD wTransItemCount;
    BYTE bInterfaceUUID[16];
    DWORD dwInterfaceVersion;
    BYTE bTransferSyntaxUUID[16];
    DWORD dwTransferSyntaxVersion;
} RpcBindRequestContextEntry;

typedef struct RpcBindRequestHeader {
    WORD wMaxSendFrag;
    WORD wMaxRecvFrag;
    DWORD dwAssocGroup;
    BYTE bContextCount;
    BYTE bAlign[3];
    RpcBindRequestContextEntry Context;
} RpcBindRequestHeaderStruct;

typedef struct RpcBindResponseContextEntryStruct {
    WORD wResult;
    WORD wAlign;
    BYTE bTransferSyntax[16];
    DWORD dwTransferSyntaxVersion;
} RpcBindResponseContextEntry;

typedef struct RpcBindResponseHeader1 {
    WORD wMaxSendFrag;
    WORD wMaxRecvFrag;
    DWORD dwAssocGroup;
} RpcBindResponseHeader1Struct;

typedef struct RpcBindResponseHeader2 {
    DWORD dwContextResultCount;
    RpcBindResponseContextEntry Context;
} RpcBindResponseHeader2Struct;

typedef struct RpcConnection {
    // Replace with appropriate handle type for your platform (e.g., SOCKET)
    void *hFile;
    DWORD dwCallIndex;
    DWORD dwInputError;
    DWORD dwRequestInitialised;
    BYTE bProcedureInputData[MAX_PROCEDURE_DATA_LENGTH];
    DWORD dwProcedureInputDataLength;
    BYTE bProcedureOutputData[MAX_PROCEDURE_DATA_LENGTH];
    DWORD dwProcedureOutputDataLength;
} RpcConnectionStruct;


DWORD RpcConvertUUID(char *pString, BYTE *pUUID, DWORD dwMaxLength);
DWORD RpcBind(RpcConnectionStruct *pRpcConnection, char *pInterfaceUUID, DWORD dwInterfaceVersion);
DWORD RpcConnect(char *pPipeName, char *pInterfaceUUID, DWORD dwInterfaceVersion, RpcConnectionStruct *pRpcConnection);
DWORD RpcSendRequest(RpcConnectionStruct *pRpcConnection, DWORD dwProcedureNumber, RpcFunction RpcFunction);
DWORD RpcInitialiseRequestData(RpcConnectionStruct *pRpcConnection);
DWORD RpcAppendRequestData_Binary(RpcConnectionStruct *pRpcConnection, BYTE *pData, DWORD dwDataLength);
DWORD RpcAppendRequestData_Dword(RpcConnectionStruct *pRpcConnection, DWORD dwValue);
DWORD RpcDisconnect(RpcConnectionStruct *pRpcConnection);
void hexdump(const unsigned char *data, size_t size);
