#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <unordered_set>
#include <string>
#include <thread>
#include <sstream>
#include <algorithm>
#include <wininet.h> 
#include "ConsoleWriter.h"

#pragma comment(lib, "wininet.lib")

ConsoleWriter writer;


DWORD GetProcessIDByName(const std::wstring& processName) {
    DWORD processID = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        writer.PrintMessage(L"Failed to create snapshot. Error: " + std::to_wstring(GetLastError()), ConsoleWriter::Color::Red);
        return 0;
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &processEntry)) {
        do {
            if (_wcsicmp(processEntry.szExeFile, processName.c_str()) == 0) {
                processID = processEntry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &processEntry));
    }
    else {
        writer.PrintMessage(L"Failed to enumerate processes. Error: " + std::to_wstring(GetLastError()), ConsoleWriter::Color::Red);
    }

    CloseHandle(snapshot);
    return processID;
}


bool ValidateModuleWithServer(const std::wstring& moduleName) {
    HINTERNET hSession = InternetOpen(L"ModuleValidator", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hSession) {
        writer.PrintMessage(L"Failed to open Internet session.", ConsoleWriter::Color::Red);
        return false;
    }


    HINTERNET hConnect = InternetConnect(hSession, L"serveriphere", 5000, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        writer.PrintMessage(L"Failed to connect to server.", ConsoleWriter::Color::Red);
        InternetCloseHandle(hSession);
        return false;
    }


    std::wstringstream url;
    url << L"/check?query=" << moduleName;


    HINTERNET hRequest = HttpOpenRequest(hConnect, L"GET", url.str().c_str(), NULL, NULL, NULL, 0, 0);
    if (!hRequest) {
        writer.PrintMessage(L"Failed to open HTTP request.", ConsoleWriter::Color::Red);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return false;
    }


    if (!HttpSendRequest(hRequest, NULL, 0, NULL, 0)) {
        writer.PrintMessage(L"Failed to send HTTP request.", ConsoleWriter::Color::Red);
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return false;
    }


    char responseBuffer[4096];
    DWORD bytesRead = 0;
    std::string response;

    while (InternetReadFile(hRequest, responseBuffer, sizeof(responseBuffer) - 1, &bytesRead) && bytesRead > 0) {
        responseBuffer[bytesRead] = '\0';
        response += responseBuffer;
    }


    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hSession);


    if (response.find("\"result\":true") != std::string::npos) {
        return true;
    }
    return false;
}


bool IsSuspiciousModule(const std::wstring& modulePath) {
    std::wstring lowerPath = modulePath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);

    if (lowerPath.find(L"\\temp\\") != std::wstring::npos || lowerPath.find(L"\\desktop\\") != std::wstring::npos) {
        return true;
    }
    return false;
}


int ScanModules(DWORD processID, std::unordered_set<std::wstring>& existingModules, int& totalScanned) {
    int scannedCount = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);
    if (snapshot == INVALID_HANDLE_VALUE) {
        writer.PrintMessage(L"Failed to create snapshot. Error: " + std::to_wstring(GetLastError()), ConsoleWriter::Color::Red);
        return scannedCount;
    }

    MODULEENTRY32 moduleEntry;
    moduleEntry.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(snapshot, &moduleEntry)) {
        do {
            std::wstring moduleName(moduleEntry.szModule);
            std::wstring modulePath(moduleEntry.szExePath);

            if (existingModules.find(moduleName) == existingModules.end()) {

                existingModules.insert(moduleName);


                if (IsSuspiciousModule(modulePath)) {
                    writer.PrintMessage(L"Highly suspicious module: " + moduleName + L" (Path: " + modulePath + L")", ConsoleWriter::Color::Red);
                }
                else {

                    bool isValid = ValidateModuleWithServer(moduleName);
                    if (!isValid) {
                        writer.PrintMessage(L"External Module (Not Win32): " + moduleName + L" (Path: " + modulePath + L")", ConsoleWriter::Color::Yellow);
                    }
                    else
                    {
                        writer.PrintMessage(L"Scanned " + moduleName + L" (Path: " + modulePath + L")", ConsoleWriter::Color::Green);
                    }
                }
                scannedCount++;
                totalScanned++;
            }
        } while (Module32Next(snapshot, &moduleEntry));
    }
    else {
        writer.PrintMessage(L"Failed to enumerate modules. Error: " + std::to_wstring(GetLastError()), ConsoleWriter::Color::Red);
    }

    CloseHandle(snapshot);
    return scannedCount;
}

#define STATUS_INFO_LENGTH_MISMATCH   0xC0000004


int main() {
    std::wstring processName = L"Cuphead.exe";
    std::unordered_set<std::wstring> existingModules;
    int totalScanned = 0;

    DWORD processID = GetProcessIDByName(processName);
    if (processID == 0) {
        writer.PrintMessage(L"Process not found: " + processName, ConsoleWriter::Color::Red);
        return 1;
    }

    writer.PrintMessage(L"Scanning process with PID: " + std::to_wstring(processID), ConsoleWriter::Color::Green);

    while (true) {

        int scannedModules = ScanModules(processID, existingModules, totalScanned);

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
