#include <windows.h>
#include <iostream>

class ConsoleWriter {
public:
    enum class Color {
        White = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        Green = FOREGROUND_GREEN,
        Red = FOREGROUND_RED,
        Blue = FOREGROUND_BLUE,
        Cyan = FOREGROUND_GREEN | FOREGROUND_BLUE,
        Magenta = FOREGROUND_RED | FOREGROUND_BLUE,
        Yellow = FOREGROUND_RED | FOREGROUND_GREEN,
        Gray = FOREGROUND_INTENSITY,
        Black = 0
    };

    void PrintMessage(const std::wstring& message, Color color) {
        // Set console text color
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), static_cast<int>(color));
        std::wcout << message << std::endl;
    }
};