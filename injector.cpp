#include <windows.h>
#include <string>
#include <iostream>

std::string GetSelfDir() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string s(path);
    return s.substr(0, s.find_last_of("\\/") + 1);
}

bool InjectDLL(HANDLE hProcess, const char* dllPath) {
    size_t len = strlen(dllPath) + 1;

    LPVOID remoteMem = VirtualAllocEx(
        hProcess,
        nullptr,
        len,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );
    if (!remoteMem)
        return false;

    if (!WriteProcessMemory(
            hProcess,
            remoteMem,
            dllPath,
            len,
            nullptr))
        return false;

    LPVOID loadLib = (LPVOID)GetProcAddress(
        GetModuleHandleA("kernel32.dll"),
        "LoadLibraryA"
    );
    if (!loadLib)
        return false;

    HANDLE hThread = CreateRemoteThread(
        hProcess,
        nullptr,
        0,
        (LPTHREAD_START_ROUTINE)loadLib,
        remoteMem,
        0,
        nullptr
    );
    if (!hThread)
        return false;

    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hThread);

    return true;
}

int main() {
    std::string dir = GetSelfDir();

    std::string exePath = dir + "PlantsVsZombies.exe";
    std::string dllPath = dir + "dllpatch.dll";

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    if (!CreateProcessA(
            exePath.c_str(),
            nullptr,
            nullptr,
            nullptr,
            FALSE,
            CREATE_SUSPENDED,
            nullptr,
            dir.c_str(),
            &si,
            &pi)) {

        MessageBoxA(nullptr, "CreateProcess failed", "Injector", MB_ICONERROR);
        return 1;
    }

    if (!InjectDLL(pi.hProcess, dllPath.c_str())) {
        MessageBoxA(nullptr, "DLL injection failed", "Injector", MB_ICONERROR);
        TerminateProcess(pi.hProcess, 0);
        return 1;
    }

    ResumeThread(pi.hThread);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    MessageBoxA(nullptr, "Injected successfully", "Injector", MB_OK);
    return 0;
}
