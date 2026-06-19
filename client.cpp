#include <windows.h>
#include <iostream>
#include <random>

struct Vector3 {
    float x, y, z;
};

float GetRandomCoordinate(float min, float max) {
    std::random_device rd; 
    std::mt19937 gen(rd()); 
    std::uniform_real_distribution<float> dis(min, max); 
    return dis(gen);
}

void ExecuteMinecraftTeleport() {
    // 1. Your confirmed live Y-coordinate address from Cheat Engine
    uintptr_t liveYAddress = 0x25871B1ED24; 
    
    if (!liveYAddress) return;

    // Vector structures store X, Y, Z sequentially. 
    // Since we have Y, subtracting 4 bytes gives us the exact start of the X position.
    Vector3* clientPos = (Vector3*)(liveYAddress - 4);

    // In Minecraft Bedrock, the secondary physics verification arrays sit 
    // immediately adjacent to each other in 12-byte blocks (sizeof Vector3).
    Vector3* oldPos     = (Vector3*)((uintptr_t)clientPos + 12); // Old visual frame
    Vector3* serverPos  = (Vector3*)((uintptr_t)clientPos + 24); // Server validation transform

    if (clientPos != nullptr) {
        try {
            float randomX = GetRandomCoordinate(1.0f, 100000.0f);
            float randomZ = GetRandomCoordinate(1.0f, 100000.0f);
            float safeY = 90.0f; // High elevation prevents suffocating in dirt/stone

            // Overwrite all three positional states at once to eliminate rubberbanding
            clientPos->x = randomX; clientPos->y = safeY; clientPos->z = randomZ;
            oldPos->x    = randomX; oldPos->y    = safeY; oldPos->z    = randomZ;
            serverPos->x = randomX; serverPos->y = safeY; serverPos->z = randomZ;

            std::cout << "\n[!] Synced Teleport Success! Moved to X: " << randomX << " Z: " << randomZ << "\n";
        }
        catch (...) {
            std::cout << "\n[ERROR] Memory write violation caught. Address has changed.\n";
        }
    }
}

DWORD WINAPI ClientMain(LPVOID lpParam) {
    AllocConsole();
    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);

    std::cout << "========================================\n";
    std::cout << "       Synced Dynamic Teleport Client   \n";
    std::cout << "========================================\n";
    std::cout << "Press [NUMPAD 2] to Teleport Randomly.\n"; 
    std::cout << "Press [END] to safely detach the client.\n\n";

    uintptr_t liveYAddress = 0x25871B1ED24; 
    int printCounter = 0;

    while (true) {
        if (GetAsyncKeyState(VK_END) & 0x8000) {
            break; 
        }

       if ((GetAsyncKeyState(VK_NUMPAD2) & 0x8000) || (GetAsyncKeyState(0x32) & 0x8000)) {
            ExecuteMinecraftTeleport();
            
            // Brief pause after a successful trigger to prevent the randomizer 
            // from firing 10 times in a single second if you hold the key down.
            Sleep(300); 
        }
        if (printCounter >= 50) {
            if (liveYAddress != 0) {
                Vector3* currentCoords = (Vector3*)(liveYAddress - 4);
                try {
                    if (currentCoords != nullptr) {
                        std::cout << "\rLive Coordinates -> X: " << currentCoords->x 
                                  << "  Y: " << currentCoords->y 
                                  << "  Z: " << currentCoords->z << "        " << std::flush;
                    }
                }
                catch (...) {}
            }
            printCounter = 0; 
        }

        printCounter++;
        Sleep(10); 
    }

    std::cout << "\nClosing telemetry loop and ejecting...\n";
    Sleep(1000);
    if (f) fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, ClientMain, hModule, 0, NULL);
        break;
    }
    return TRUE;
}
