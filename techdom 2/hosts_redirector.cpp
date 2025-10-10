// hosts_redirector.cpp
// Requires Windows (Win32). Compile with MSVC. Link: none special.
// Example functions: EnableRedirects(...) and DisableRedirects(...)

#include <windows.h>
#include <shlwapi.h>   // PathAppend (link Shlwapi.lib if you use PathAppend)
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <iomanip>

#pragma comment(lib, "Shlwapi.lib")

// Default hosts path
static std::wstring GetHostsPath() {
    // Typically: C:\Windows\System32\drivers\etc\hosts
    wchar_t sysdir[MAX_PATH];
    if (GetSystemDirectoryW(sysdir, MAX_PATH) == 0) {
        // fallback
        return L"C:\\Windows\\System32\\drivers\\etc\\hosts";
    }
    std::wstring path = sysdir; // e.g. C:\Windows\System32
    path += L"\\drivers\\etc\\hosts";
    return path;
}

// Backup path (same folder + hosts.backup_TIMESTAMP)
static std::wstring MakeBackupPath() {
    std::wstring hostsPath = GetHostsPath();

    // get folder
    std::wstring folder = hostsPath;
    size_t pos = folder.find_last_of(L"\\/");
    if (pos != std::wstring::npos) folder.resize(pos);

    // timestamp
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &t);

    std::wostringstream oss;
    oss << folder << L"\\hosts.backup_"
        << std::put_time(&tm, L"%Y%m%d_%H%M%S") << L".bak";

    return oss.str();
}

// Simple admin check
static bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID administratorsGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &administratorsGroup)) {
        CheckTokenMembership(NULL, administratorsGroup, &isAdmin);
        FreeSid(administratorsGroup);
    }
    return isAdmin == TRUE;
}

// Flush DNS resolver cache (call ipconfig /flushdns)
static void FlushDnsCache() {
    // Use CreateProcess to avoid blocking on system()
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    std::wstring cmd = L"ipconfig /flushdns";
    // Create process with cmd.exe /C
    std::wstring cmdLine = L"cmd.exe /C " + cmd;
    CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (pi.hProcess) {
        WaitForSingleObject(pi.hProcess, 5000); // wait up to 5s
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

// Create a backup of current hosts file; returns true on success and sets backupPath
static bool BackupHosts(std::wstring& backupPath, std::wstring& outError) {
    std::wstring hosts = GetHostsPath();

    // Ensure file exists
    std::ifstream in(
        std::string(hosts.begin(), hosts.end()), std::ios::binary);
    if (!in.is_open()) {
        outError = L"Failed to open hosts file for reading.";
        return false;
    }
    // make backup path
    backupPath = MakeBackupPath();
    // Copy bytes
    std::ofstream out(std::string(backupPath.begin(), backupPath.end()), std::ios::binary);
    if (!out.is_open()) {
        outError = L"Failed to create backup file. Are you elevated?";
        return false;
    }
    out << in.rdbuf();
    in.close();
    out.close();
    return true;
}

// Restore hosts from backupPath
static bool RestoreHostsFromBackup(const std::wstring& backupPath, std::wstring& outError) {
    std::ifstream in(std::string(backupPath.begin(), backupPath.end()), std::ios::binary);
    if (!in.is_open()) {
        outError = L"Failed to open backup file for reading.";
        return false;
    }
    std::wstring hosts = GetHostsPath();

    std::ofstream out(std::string(hosts.begin(), hosts.end()), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        outError = L"Failed to open hosts file for writing. Are you elevated?";
        return false;
    }
    out << in.rdbuf();
    in.close();
    out.close();
    return true;
}

// Apply redirects: writes a new hosts file that keeps original content but ensures the redirects exist.
// domains: list of domains to redirect. redirectIP: IP to use (e.g., "127.0.0.1")
static bool ApplyRedirectsKeepingOriginal(const std::vector<std::string>& domains, const std::string& redirectIP, std::wstring& outError) {
    std::wstring hosts = GetHostsPath();

    // Read original file (narrow, hosts file is text ASCII/ANSI/UTF-8 typically)
    std::ifstream in(std::string(hosts.begin(), hosts.end()), std::ios::binary);
    if (!in.is_open()) {
        outError = L"Failed to open hosts file for reading.";
        return false;
    }
    std::string original((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // Build new content: keep original except remove previously-added lines we control (optional).
    // Simpler approach: append our block at end with markers so we can remove on restore.
    std::ostringstream oss;
    oss << original;
    // Ensure newline end
    if (!original.empty() && original.back() != '\n') oss << "\n";

    oss << "# --- BEGIN Redirector Block ---\n";
    for (const auto& d : domains) {
        // make sure to avoid duplicates: naive approach - append anyway.
        oss << redirectIP << " " << d << "\n";
    }
    oss << "# --- END Redirector Block ---\n";

    // Write out (atomic-ish: write to temp then move)
    // create temp file in same directory to avoid cross-volume rename issues
    std::wstring folder = hosts;
    size_t pos = folder.find_last_of(L"\\/");
    if (pos != std::wstring::npos) folder.resize(pos);

    std::wstring tempPath = folder + L"\\hosts_temp_write.tmp";
    std::ofstream out(std::string(tempPath.begin(), tempPath.end()), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        outError = L"Failed to open temporary hosts file for writing.";
        return false;
    }
    std::string newContent = oss.str();
    out.write(newContent.data(), (std::streamsize)newContent.size());
    out.close();

    // Move temp file to hosts (overwrite)
    if (!MoveFileExW(tempPath.c_str(), hosts.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        outError = L"Failed to replace hosts file (MoveFileExW failed). Are you elevated?";
        // cleanup temp (best-effort)
        DeleteFileW(tempPath.c_str());
        return false;
    }

    return true;
}

// Remove redirect block (if present) by restoring backup; or optionally remove markers.
// Here we will prefer restoring from backup when disabling, so no need for a special remove routine.

// PUBLIC API functions

// Enable redirects: backs up hosts and applies redirects.
// domains: list of domain names (narrow ASCII). redirectIP: "127.0.0.1" or other.
// returns 0 on success; non-zero otherwise. backupPathOut receives the backup path for later restore.
int EnableRedirects(const std::vector<std::string>& domains, const std::string& redirectIP, std::wstring& backupPathOut, std::wstring& errorOut) {
    if (!IsRunningAsAdmin()) {
        errorOut = L"Application is not running with administrative privileges.";
        return 1;
    }
    // Make backup
    std::wstring backupPath;
    if (!BackupHosts(backupPath, errorOut)) {
        return 2;
    }

    // Apply redirects
    if (!ApplyRedirectsKeepingOriginal(domains, redirectIP, errorOut)) {
        // Attempt to restore from backup on failure
        RestoreHostsFromBackup(backupPath, errorOut);
        return 3;
    }

    // Flush DNS
    FlushDnsCache();

    backupPathOut = backupPath;
    return 0;
}

// Disable redirects by restoring from backupPath (must be the path created earlier)
int DisableRedirects(const std::wstring& backupPath, std::wstring& errorOut) {
    if (!IsRunningAsAdmin()) {
        errorOut = L"Application is not running with administrative privileges.";
        return 1;
    }

    if (!PathFileExistsW(backupPath.c_str())) {
        errorOut = L"Backup file does not exist.";
        return 2;
    }

    if (!RestoreHostsFromBackup(backupPath, errorOut)) {
        return 3;
    }

    // Flush DNS
    FlushDnsCache();

    // Optionally delete backup (or keep it)
    // DeleteFileW(backupPath.c_str());

    return 0;
}

// Example usage (console)
#ifdef EXAMPLE_MAIN
int wmain() {
    std::vector<std::string> domains = { "example.com", "www.example.com", "annoying.site" };
    std::wstring backupPath;
    std::wstring err;
    int r = EnableRedirects(domains, "127.0.0.1", backupPath, err);
    if (r != 0) {
        std::wcerr << L"EnableRedirects failed: " << err << L" (code " << r << L")\n";
        return r;
    }
    std::wcout << L"Redirects enabled. Backup saved to: " << backupPath << L"\n";

    // simulate disable
    //int s = DisableRedirects(backupPath, err);
    //if (s != 0) std::wcerr << L"Disable failed: " << err << L"\n";
    return 0;
}
#endif
