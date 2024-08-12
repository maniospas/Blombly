namespace Terminal {
    #ifdef _WIN32
    #include <windows.h>

    // Function to enable virtual terminal processing and set UTF-8 encoding
    void enableVirtualTerminalProcessing() {
        // Enable Virtual Terminal Processing on Windows
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) {
            return;
        }

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) {
            return;
        }

        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(hOut, dwMode)) {
            return;
        }

        // Set console output to UTF-8
        SetConsoleOutputCP(CP_UTF8);
    }
    #else
    void enableVirtualTerminalProcessing(){}
    #endif
}
