#include <windows.h>
#include <psapi.h>
#include <cstdint>

#define SUN_OFFSET 0x5560

// 03 82 60 55 00 00   add eax,[edx+00005560]
const char* SIG_BYTES = "\x03\x82\x60\x55\x00\x00";
const char* SIG_MASK  = "xxxxxx";

extern "C" {
    __attribute__((used)) uintptr_t g_structBase = 0;
    __attribute__((used)) uintptr_t g_retAddr    = 0;
}

HMODULE g_hModule = nullptr;

uintptr_t FindPattern(const char* module, const char* pattern, const char* mask) {
    MODULEINFO mi{};
    HMODULE hMod = module ? GetModuleHandleA(module)
                          : GetModuleHandleA(nullptr);
    if (!hMod) return 0;

    GetModuleInformation(GetCurrentProcess(), hMod, &mi, sizeof(mi));

    uintptr_t base = (uintptr_t)mi.lpBaseOfDll;
    size_t size = mi.SizeOfImage;

    for (size_t i = 0; i < size; i++) {
        bool found = true;
        for (size_t j = 0; mask[j]; j++) {
            if (mask[j] == 'x' &&
                pattern[j] != *(char*)(base + i + j)) {
                found = false;
                break;
            }
        }
        if (found)
            return base + i;
    }
    return 0;
}

extern "C" __attribute__((naked)) void SunHook() {
    __asm__ volatile (
        ".intel_syntax noprefix\n"

        "mov dword ptr [_g_structBase], edx\n"
        "add eax, dword ptr [edx + 0x5560]\n"
        "jmp dword ptr [_g_retAddr]\n"

        ".att_syntax prefix\n"
    );
}

void InstallHook() {
    uintptr_t addr = FindPattern(nullptr, SIG_BYTES, SIG_MASK);
    if (!addr) return;

    g_retAddr = addr + 6;

    DWORD old;
    VirtualProtect((LPVOID)addr, 6, PAGE_EXECUTE_READWRITE, &old);

    *(BYTE*)addr = 0xE9;
    *(DWORD*)(addr + 1) = (DWORD)SunHook - (addr + 5);

    VirtualProtect((LPVOID)addr, 6, old, &old);
}

int GetSun() {
    if (!g_structBase) return -1;
    return *(int*)(g_structBase + SUN_OFFSET);
}

void SetSun(int value) {
    if (!g_structBase) return;
    *(int*)(g_structBase + SUN_OFFSET) = value;
}

HWND g_hEdit = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateWindowA("STATIC", "Sun:",
            WS_CHILD | WS_VISIBLE,
            10, 10, 100, 20,
            hwnd, nullptr, g_hModule, nullptr);

        g_hEdit = CreateWindowA("EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            10, 35, 120, 22,
            hwnd, nullptr, g_hModule, nullptr);

        CreateWindowA("BUTTON", "Set",
            WS_CHILD | WS_VISIBLE,
            40, 65, 60, 25,
            hwnd, (HMENU)1, g_hModule, nullptr);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            char buf[32];
            GetWindowTextA(g_hEdit, buf, sizeof(buf));
            SetSun(atoi(buf));
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

DWORD WINAPI UIThread(LPVOID) {
    WNDCLASSA wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = g_hModule;
    wc.lpszClassName = "SunHackWnd";

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA(
        "SunHackWnd", "Sun Editor",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        300, 300, 160, 140,
        nullptr, nullptr, g_hModule, nullptr);

    ShowWindow(hwnd, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

DWORD WINAPI MainThread(LPVOID) {
    InstallHook();

    while (true) {
        if (GetAsyncKeyState(VK_F1) & 1)
            CreateThread(nullptr, 0, UIThread, nullptr, 0, nullptr);
        Sleep(50);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
