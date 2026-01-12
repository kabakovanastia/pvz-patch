# Plants vs. Zombies DLL Patch

Этот проект содержит патч для игры *Plants vs. Zombies* (PVZ), реализованный в виде DLL-библиотеки, которая внедряется в процесс игры с помощью простого injector'а.

## Сборка

Для сборки необходим кросс-компилятор `i686-w64-mingw32-g++` (для целевой архитектуры Windows 32-bit).

### 1. Сборка патча (`dllpatch.dll`)

```sh
i686-w64-mingw32-g++ dllpatch.cpp -shared -o dllpatch.dll -static-libgcc -static-libstdc++ -Wl,--subsystem,windows
```

### 2. Сборка injector'а (`injector.exe`)

```sh
i686-w64-mingw32-g++ injector.cpp -o injector.exe -static
```

## Установка

После успешной компиляции скопируйте оба файла в каталог с игрой:

```sh
cp dllpatch.dll injector.exe game/
```

## Запуск

Запустите игру **только** через injector:

```sh
cd game/
start injector.exe
```
