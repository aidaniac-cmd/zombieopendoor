#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <string>
#include <accctrl.h>
#include <aclapi.h>

// Function to find Minecraft's Process ID
DWORD GetProcessIdByName(const wchar_t* processName) {
    DWORD pid = 15640; // Default PID for Minecraft.Windows.exe, change if needed
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(processName);
        if (Process32FirstW(snapshot, &entry)) {
            do {
                if (_wcsicmp(entry.szExeFile, processName) == 0) {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
    return pid;
}

// Function to fix UWP permissions on the DLL file
bool SetUWPPermissions(const std::wstring& dllPath) {
    PACL pOldACL = NULL, pNewACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESSW ea;

    if (GetNamedSecurityInfoW(dllPath.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldACL, NULL, &pSD) != ERROR_SUCCESS) {
        return false;
    }

    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESSW));
    ea.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea.Trustee.ptstrName = (LPWSTR)L"ALL APPLICATION PACKAGES";

    if (SetEntriesInAclW(1, &ea, pOldACL, &pNewACL) != ERROR_SUCCESS) {
        if (pSD) LocalFree(pSD);
        return false;
    }

    DWORD result = SetNamedSecurityInfoW((LPWSTR)dllPath.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewACL, NULL);
    
    if (pNewACL) LocalFree(pNewACL);
    if (pSD) LocalFree(pSD);

    return result == ERROR_SUCCESS;
}

int main() {
    // 1. Set your path (Use absolute path, use L for wide-string UWP compatibility)
    std::wstring dllPath = L"C:\\Users\\aidan\\Downloads\\zombieopendoor\\client.dll"; 
    
    std::wcout << L"Setting UWP sandbox permissions on DLL...\n";
    if (!SetUWPPermissions(dllPath)) {
        std::cout << "Failed to set UWP permissions. Run as Administrator.\n";
        return 1;
    }

    std::wcout << L"Looking for Minecraft...\n";
    DWORD pid = GetProcessIdByName(L"Minecraft.exe");
    if (pid == 0) {
        std::cout << "Minecraft is not running!\n";
        return 1;
    }
    std::cout << "Found Minecraft! PID: " << pid << "\n";

    // 2. Open process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cout << "Failed to open process. Error: " << GetLastError() << "\n";
        return 1;
    }

    // 3. Allocate memory (using bytes size of wide-string)
    size_t size = (dllPath.length() + 1) * sizeof(wchar_t);
    LPVOID pRemoteBuf = VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemoteBuf) {
        std::cout << "Failed to allocate memory.\n";
        CloseHandle(hProcess);
        return 1;
    }

    // 4. Write path to memory
    if (!WriteProcessMemory(hProcess, pRemoteBuf, dllPath.c_str(), size, NULL)) {
        std::cout << "Failed to write memory.\n";
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    // 5. Create remote thread using LoadLibraryW (for wide-strings)
    LPVOID pLoadLibrary = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW");
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteBuf, 0, NULL);

    if (!hThread) {
        std::cout << "Failed to create remote thread.\n";
    } else {
        std::cout << "Successfully injected!\n";
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    // 6. Cleanup
    VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return 0;
}
