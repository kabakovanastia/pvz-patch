#include <windows.h>
#include <string>
#include <sstream>
#include <cstdint>  // для uintptr_t

#define GAME_LOGIC_PTR 0x006A9EC0

int GetSun() {
    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
    uintptr_t gameLogic = *(uintptr_t*)(base + GAME_LOGIC_PTR);
    if (!gameLogic) return -1;
    return *(int*)(gameLogic + 0x768);
}

void SetSun(int sun) {
    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
    uintptr_t gameLogic = *(uintptr_t*)(base + GAME_LOGIC_PTR);
    if (gameLogic) {
        *(int*)(gameLogic + 0x768) = sun;
    }
}

// Обработчик кнопки "Установить"
INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { // Кнопка "OK"
            char buffer[16] = {0};
            GetDlgItemTextA(hwnd, 100, buffer, sizeof(buffer));
            int sun = atoi(buffer);
            if (sun >= 0 && sun <= 999999) {
                SetSun(sun);
            }
            EndDialog(hwnd, 0);
        }
        break;
    case WM_CLOSE:
        EndDialog(hwnd, 0);
        break;
    }
    return FALSE;
}

// Функция, которая создаёт окошко
void ShowSunInputDialog() {
    // Создаём модальное диалоговое окно "на лету" через шаблон в памяти
    // Но проще — создать ресурс в проекте или использовать CreateWindow
    // Для простоты здесь используем CreateWindow (без ресурсов)

    HWND hwnd = CreateWindowA("STATIC", "Введите солнце:", WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU,
                              CW_USEDEFAULT, CW_USEDEFAULT, 200, 100, NULL, NULL, NULL, NULL);

    HWND hEdit = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
                               20, 30, 120, 20, hwnd, (HMENU)100, NULL, NULL);

    HWND hButton = CreateWindowA("BUTTON", "Установить", WS_CHILD | WS_VISIBLE,
                                 70, 60, 60, 25, hwnd, (HMENU)1, NULL, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

// ГЛОБАЛЬНАЯ ФУНКЦИЯ — совместима с WinAPI
DWORD WINAPI SunInputDialogThreadProc(LPVOID lpParam) {
    ShowSunInputDialog();
    return 0;
}

// В KeyboardProc:
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        if (p->vkCode == VK_F1) {
            CreateThread(NULL, 0, SunInputDialogThreadProc, NULL, 0, NULL);
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Точка входа DLL
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    static HHOOK hHook = NULL;
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        // Устанавливаем глобальный хук на клавиатуру
        hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hModule, 0);
        break;
    case DLL_PROCESS_DETACH:
        if (hHook) UnhookWindowsHookEx(hHook);
        break;
    }
    return TRUE;
}