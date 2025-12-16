// inject.cpp
#include <windows.h>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: inject.exe <process_name> <dll_path>\n";
        return 1;
    }

    // Находим процесс
    HWND hwnd = FindWindowA(NULL, "Plants vs. Zombies");
    if (!hwnd) { std::cout << "Game not found!\n"; return 1; }
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) { std::cout << "OpenProcess failed\n"; return 1; }

    void* mem = VirtualAllocEx(hProc, NULL, MAX_PATH, MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(hProc, mem, argv[2], strlen(argv[2]) + 1, NULL);

    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0,
        (LPTHREAD_START_ROUTINE)LoadLibraryA, mem, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProc);
    return 0;
}