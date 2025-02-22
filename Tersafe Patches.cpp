#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <windows.h>
#include <codecvt>
#include <TlHelp32.h>
#include <shlobj_core.h>
#include <iomanip>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include <shlwapi.h>
#pragma comment( lib, "shlwapi.lib")

using namespace std;
INT getpid(TCHAR* ProcessName)
{
    INT ProcessId = 0;
    INT ThreadCount = 0;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    else
    {
        PROCESSENTRY32 ProcessEntry32;
        ProcessEntry32.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &ProcessEntry32))
        {
            do
            {
                if (!_wcsicmp(ProcessEntry32.szExeFile, ProcessName) && ProcessEntry32.cntThreads > ThreadCount)
                {
                    ThreadCount = ProcessEntry32.cntThreads;
                    ProcessId = (INT)ProcessEntry32.th32ProcessID;
                }
            } while (Process32Next(hSnapshot, &ProcessEntry32));
        }

        CloseHandle(hSnapshot);
        return ProcessId;
    }

    return 0;
}

bool afp(const std::string& filename, const std::vector<uint8_t>& aob, const std::vector<uint8_t>& patch) {
    // Open the file for reading and writing
    std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    // Read the entire file into memory
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> fileData(fileSize);
    file.read(reinterpret_cast<char*>(fileData.data()), fileSize);

    // Search for the AOB in the file data
    auto it = std::search(fileData.begin(), fileData.end(), aob.begin(), aob.end());
    if (it != fileData.end()) {
        // Apply the patch at the found location
        std::copy(patch.begin(), patch.end(), it);

        // Write the modified data back to the file
        file.seekp(0, std::ios::beg);
        file.write(reinterpret_cast<const char*>(fileData.data()), fileSize);
        file.close();
       // std::cout << "Patch applied successfully." << std::endl;
        return true;
    }

    std::cerr << "AOB not found in file." << std::endl;
    file.close();
    return false;
}
bool app(HANDLE hProcess, const std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>>& patches) {

    for (const auto& patch : patches) {
        const auto& originalBytes = patch.first;
        const auto& newBytes = patch.second;

        // Search for the original bytes in the process memory
        // This is a simplified example; you may need a more complex search
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        MEMORY_BASIC_INFORMATION memInfo;
        BYTE* address = nullptr;

        for (address = 0; address < (BYTE*)sysInfo.lpMaximumApplicationAddress; address += memInfo.RegionSize) {
            if (VirtualQueryEx(hProcess, address, &memInfo, sizeof(memInfo)) && memInfo.State == MEM_COMMIT) {
                std::vector<uint8_t> buffer(memInfo.RegionSize);
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProcess, address, buffer.data(), memInfo.RegionSize, &bytesRead)) {
                    auto pos = std::search(buffer.begin(), buffer.end(), originalBytes.begin(), originalBytes.end());
                    if (pos != buffer.end()) {
                        // Apply the patch
                        SIZE_T offset = (SIZE_T)address + (pos - buffer.begin());
                        WriteProcessMemory(hProcess, (LPVOID)offset, newBytes.data(), newBytes.size(), nullptr);
                    }
                }
            }
        }
    }
}

void P1() {

    std::string filename = "AndroidEmulatorEn.exe";

    std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> fpatches = {
        
        { {0x8B, 0x84, 0x24, 0x0C, 0x00, 0x00, 0x00, 0x89, 0x04, 0x24}, {0x90, 0x90, 0x90} },
        { {0x7E, 0x8D, 0xA4, 0x24, 0xFC, 0xFF, 0xFF, 0xFF}, {0x90, 0x90, 0x90, 0x90} },
        { {0x03, 0xBC, 0x24, 0x04, 0x00, 0x00, 0x00}, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90} },
        { {0x89, 0x14, 0x24}, {0x90, 0x90, 0x90} },
        { {0x33, 0xFF}, {0x90, 0x90} },
        { {0x8D, 0xA4, 0x24, 0xFC, 0xFF, 0xFF, 0xFF}, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90} }

    };



    for (const auto& patch : fpatches) {

        const auto& aob = patch.first;
        const auto& patchBytes = patch.second;

        if (afp(filename, aob, patchBytes)) {
            std::cout << "\nPatch applied successfully" << std::endl;
            std::cout << "AOB: ";
            for (const auto& byte : aob) {
                std::cout << "0x" << std::hex << std::uppercase << static_cast<int>(byte) << " ";
            }
            std::cout << std::endl;
            std::cout << "Patch: ";
            for (const auto& byte : patchBytes) {
                std::cout << "0x" << std::hex << std::uppercase << static_cast<int>(byte) << " ";
            }
            std::cout << std::endl;
            std::cout << std::endl;
        }
    }


   /* for (const auto& patch : patches) {
        const auto& aob = patch.first;
        const auto& patchBytes = patch.second;

        if (!ApplyFilePatch2(filename, aob, patchBytes)) {
            std::cout << "Failed to apply patch." << std::endl;
        }
      
    }*/
}
void P2(){


    TCHAR proc[256] = L"AndroidEmulatorEN.exe";
    DWORD id = getpid(proc);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, id);

    if (id == 0) {
        std::cout << "Failed to get process id." << std::endl;
    }
    if (!hProcess) {
        std::cout << "Failed to open process." << std::endl;
    }
    std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> patches = {

        { {0x33, 0xFF, 0x03, 0xBC, 0x24, 0x04, 0x00, 0x00, 0x00, 0xE8, 0x62, 0x1B, 0xFD, 0xFF}, {0xC7, 0x84, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
        { {0x8B, 0x84, 0x24, 0x0C, 0x00, 0x00, 0x00, 0x89, 0x04, 0x24}, {0x90, 0x90, 0x90} },
        { {0xC7, 0x84, 0x24, 0x04, 0x00, 0x00, 0x00, 0x2E, 0xD1, 0xEE, 0x78}, {0xC7, 0x84, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
        { {0xE9, 0xC3, 0xE4, 0xFE, 0xFF}, {0x90, 0x90, 0x90, 0x90, 0x90} },
        { {0x7E, 0x8D, 0xA4, 0x24, 0xFC, 0xFF, 0xFF, 0xFF}, {0x90, 0x90, 0x90, 0x90} },
        { {0xC7, 0x84, 0x24, 0x04, 0x00, 0x00, 0x00, 0x78, 0xEE, 0xD1, 0x2E}, {0xC7, 0x84, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
        { {0x03, 0xBC, 0x24, 0x04, 0x00, 0x00, 0x00}, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90} },
        { {0xE8, 0x62, 0x1B, 0xFD, 0xFF}, {0x90, 0x90, 0x90, 0x90, 0x90} },
        { {0xE9, 0xC3, 0xE4, 0xFE, 0xFF}, {0x90, 0x90, 0x90, 0x90, 0x90} },
        { {0xC7, 0xC0, 0x72, 0xE2, 0xFA, 0x70}, {0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00} },
        { {0x89, 0x14, 0x24}, {0x90, 0x90, 0x90} },
        { {0x33, 0xFF}, {0x90, 0x90} },
        { {0x81, 0xC4, 0x20, 0x00, 0x00, 0x00}, {0x81, 0xC4, 0x00, 0x00, 0x00, 0x00} },
        { {0x8D, 0xA4, 0x24, 0xFC, 0xFF, 0xFF, 0xFF}, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90} }
    };
       //get.handle();
    for (const auto& patch : patches) {

        const auto& aob = patch.first;
        const auto& patchBytes = patch.second;

        if (app(hProcess, patches)) {
            std::cout << "\nPatch applied successfully" << std::endl;
            std::cout << "AOB: ";
            for (const auto& byte : aob) {
                std::cout << "0x" << std::hex << std::uppercase << static_cast<int>(byte) << " ";
            }
            std::cout << std::endl;
            std::cout << "Patch: ";
            for (const auto& byte : patchBytes) {
                std::cout << "0x" << std::hex << std::uppercase << static_cast<int>(byte) << " ";
            }
            std::cout << std::endl;
            std::cout << std::endl;
        }
    }
    CloseHandle(hProcess);
}
void P3() {
    TCHAR proc[256] = L"AndroidEmulatorEN.exe";
    DWORD id = getpid(proc);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, id);

    if (id == 0) {
        std::cout << "Failed to get process id." << std::endl;
    }
    if (!hProcess) {
        std::cout << "Failed to open process." << std::endl;
    }

    std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> chks_patches = {

        // Patch 1
        {
            {0xA0, 0x3C, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0xD8, 0x00, 0x00, 0x00, 0x41, 0x38}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 2
        {
            {0x3C, 0x38, 0x51, 0x38, 0x5B, 0x38, 0x65, 0x38, 0x70, 0x38, 0x76, 0x38, 0x80, 0x38, 0x8A}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 3
        {
            {0x38, 0x95, 0x38, 0x9B, 0x38, 0xA5, 0x38, 0xAF, 0x38, 0xBA, 0x38, 0xC0, 0x38, 0xCA, 0x38, 0xD4}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 4
        {
            {0x38, 0xDF, 0x38, 0xE5, 0x38, 0xEF, 0x38, 0xF9, 0x38, 0x00, 0x39, 0x07, 0x39, 0x11, 0x39, 0x1B}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 5
        {
            {0x39, 0x31, 0x39, 0x3B, 0x39, 0x41, 0x39, 0x4B, 0x39, 0x59, 0x39, 0x5E, 0x39, 0x71, 0x39, 0x7B}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 6
        {
            {0x39, 0x81, 0x39, 0x8B, 0x39, 0x99, 0x39, 0x9E, 0x39, 0xB2, 0x39, 0xBD, 0x39, 0xC2, 0x39, 0xC9}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 7
        {
            {0x39, 0xE1, 0x39, 0xED, 0x39, 0xF2, 0x39, 0xF8, 0x39, 0x02, 0x3A, 0x0C, 0x3A, 0x17, 0x3A, 0x31}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 8
        {
            {0x3A, 0x40, 0x3A, 0x45, 0x3A, 0x4B, 0x3A, 0x55, 0x3A, 0x5F, 0x3A, 0x6A, 0x3A, 0x81, 0x3A, 0x8D}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 9
        {
            {0x3A, 0x92, 0x3A, 0x98, 0x3A, 0xA2, 0x3A, 0xAC, 0x3A, 0xB7, 0x3A, 0xD1, 0x3A, 0xE0, 0x3A, 0xE5}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 10
        {
            {0x3A, 0xEB, 0x3A, 0xF5, 0x3A, 0xFF, 0x3A, 0x0A, 0x3B, 0x22, 0x3B, 0x28, 0x3B, 0x32, 0x3B, 0x3E}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 11
        {
            {0x3B, 0x63, 0x3B, 0x71, 0x3B, 0x77, 0x3B, 0xAF, 0x3B, 0xC0, 0x3B, 0xDF, 0x3B, 0xF0, 0x3B, 0xF9}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 12
        {
            {0x3B, 0x15, 0x3C, 0x1C, 0x3C, 0x34, 0x3C, 0x46, 0x3C, 0x58, 0x3C, 0x6C, 0x3C, 0x76, 0x3C, 0x82}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 13
        {
            {0x3C, 0x90, 0x3C, 0x9C, 0x3C, 0xA5, 0x3C, 0xB2, 0x3C, 0xB8, 0x3C, 0xC1, 0x3C, 0xD1, 0x3C, 0xE3}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 14
        {
            {0x3D, 0x00, 0x3D, 0x0A, 0x3D, 0x12, 0x3D, 0x1C, 0x3D, 0x26, 0x3D, 0x30, 0x3D, 0x3A, 0x3D, 0x42}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 15
        {
            {0x3D, 0x54, 0x3D, 0x5E, 0x3D, 0x68, 0x3D, 0x72, 0x3D, 0x7C, 0x3D, 0x84, 0x3D, 0x90, 0x3D, 0x9A}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 16
        {
            {0x3D, 0xA5, 0x3D, 0xB0, 0x3D, 0xB8, 0x3D, 0xC2, 0x3D, 0xCC, 0x3D, 0xD4, 0x3D, 0xE0, 0x3D, 0xE7}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 17
        {
            {0x3D, 0xF6, 0x3E, 0x00, 0x3E, 0x08, 0x3E, 0x14, 0x3E, 0x1E, 0x3E, 0x2A, 0x3E, 0x34, 0x3E, 0x3E}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 18
        {
            {0x3E, 0x48, 0x3E, 0x54, 0x3E, 0x5E, 0x3E, 0x66, 0x3E, 0x72, 0x3E, 0x80, 0x3E, 0x8E, 0x3E, 0x9A}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 19
        {
            {0x3E, 0xA6, 0x3E, 0xB0, 0x3E, 0xB8, 0x3E, 0xC4, 0x3E, 0xCE, 0x3E, 0xD6, 0x3E, 0xE2, 0x3E, 0xEA}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 20
        {
            {0x3E, 0xF6, 0x3F, 0x02, 0x3F, 0x0C, 0x3F, 0x16, 0x3F, 0x22, 0x3F, 0x2A, 0x3F, 0x32, 0x3F, 0x3C}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 21
        {
            {0x3F, 0x4E, 0x3F, 0x58, 0x3F, 0x64, 0x3F, 0x6C, 0x3F, 0x74, 0x3F, 0x7A, 0x3F, 0x82, 0x3F, 0x8A}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

        // Patch 22
        {
            {0x3F, 0x94, 0x3F, 0x9A, 0x3F, 0xA4, 0x3F, 0xB2, 0x3F, 0xBE, 0x3F, 0xC8, 0x3F, 0xD0, 0x3F, 0xDA}, // search
            {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}  // replace
        },

    };

    //get.handle();
    for (const auto& patch : chks_patches) {

        const auto& aob = patch.first;
        const auto& patchBytes = patch.second;

        if (app(hProcess, chks_patches)) {
            std::cout << "\nPatch applied successfully" << std::endl;
            std::cout << "AOB: ";
            for (const auto& byte : aob) {
                std::cout << "0x" << std::hex << std::uppercase << static_cast<int>(byte) << " ";
            }
            std::cout << std::endl;
            std::cout << "Patch: ";
            for (const auto& byte : patchBytes) {
                std::cout << "0x" << std::hex << std::uppercase << static_cast<int>(byte) << " ";
            }
            std::cout << std::endl;
            std::cout << std::endl;
        }
    }
    CloseHandle(hProcess);
}

int main()
{
    int selection;
    std::cout << "Choose patching method" << endl;
    cout << "1. File strings patching" << endl;
    cout << "2. Process strings patching" << endl;
    cout << "3. Process security checks patching" << endl;
    cout << "Enter your selection : ";
    cin >> selection;

    if (selection != 1 && selection != 2 && selection != 3) exit(0);

    if (selection == 1) {
        P1();
    }
    else if (selection == 2) {
        P2();
    }
    else if (selection == 3) {
        P3();
    }
    else {
        exit(0);
    }

    cin.get();
    cout << "Press any key to close..." << endl;
    cin.get();

    return 0;
}