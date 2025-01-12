#include <wintrust.h>
#include <mshtml.h>
#include <softpub.h>
ConsoleWriter writer2;
// Function to verify module signature using WinVerifyTrust
// Function to verify module signature using WinVerifyTrust
bool VerifyModuleSignature(const std::wstring& modulePath) {
    // Define the structure to hold the verification information
    WINTRUST_FILE_INFO fileData = { 0 };
    fileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileData.pcwszFilePath = modulePath.c_str();

    WINTRUST_DATA trustData = { 0 };
    trustData.cbStruct = sizeof(WINTRUST_DATA);
    trustData.dwUnionChoice = WTD_UI_NONE;  // No UI during verification (important to prevent the dialog)
    trustData.dwStateAction = WTD_STATEACTION_VERIFY;
    trustData.pFile = &fileData;

    GUID actionGuid = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    // Perform the verification with an attempt to silently ignore untrusted file warnings
    LONG status = WinVerifyTrust(NULL, &actionGuid, &trustData);

    // If the status indicates an untrusted file, try to ignore the verification
    if (status == TRUST_E_PROVIDER_UNKNOWN || status == TRUST_E_SUBJECT_NOT_TRUSTED) {
        // We might still log it but bypass the user prompt
        writer2.PrintMessage(L"Module " + modulePath + L" is not trusted (silently handled).", ConsoleWriter::Color::Red);
        return false;
    }

    if (status == ERROR_SUCCESS) {
        writer2.PrintMessage(L"Module " + modulePath + L" is trusted.", ConsoleWriter::Color::Green);
        return true;
    }

    writer2.PrintMessage(L"Module " + modulePath + L" verification failed with error code: " + std::to_wstring(status), ConsoleWriter::Color::Red);
    return false;
}