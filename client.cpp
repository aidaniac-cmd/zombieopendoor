#include <windows.h>
#include <iostream>

// This function runs automatically the exact moment your DLL is injected
DWORD WINAPI ClientMain(LPVOID lpParam) {
    // 1. Create a debug console window so you can see your client's output
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    std::cout << "========================================\n";
    std::cout << "  Custom Minecraft Bedrock Client Loaded \n";
    std::cout << "========================================\n";
    std::cout << "Successfully attached to Minecraft.Windows.exe!\n";
    std::cout << "Press [END] on your keyboard to eject the client.\n\n";

    // 2. Main loop: Keep the client running until you press the END key
    while (true) {
        if (GetAsyncKeyState(VK_END) & 0x8000) {
            break; 
        }
        Sleep(100); // Prevents the loop from consuming 100% of your CPU
    }

    // 3. Clean up and eject safely when you press END
    std::cout << "Ejecting client...\n";
    Sleep(1000);
    if (f) fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

// The core entry point for any Windows DLL file
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // Create a new background thread so we don't freeze the game's main engine
        CreateThread(NULL, 0, ClientMain, hModule, 0, NULL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
