#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <map>

using namespace std;

#define CFG_FILE "config.ini"

struct Config {
    string key;
    bool run;
    bool hide;
};

// Key mappings
map<string, UINT> mods = {
    {"ctrl", MOD_CONTROL}, {"alt", MOD_ALT},
    {"shift", MOD_SHIFT}, {"win", MOD_WIN}
};

map<string, UINT> keys = {
    // Letters
    {"a", 'A'}, {"b", 'B'}, {"c", 'C'}, {"d", 'D'}, {"e", 'E'},
    {"f", 'F'}, {"g", 'G'}, {"h", 'H'}, {"i", 'I'}, {"j", 'J'},
    {"k", 'K'}, {"l", 'L'}, {"m", 'M'}, {"n", 'N'}, {"o", 'O'},
    {"p", 'P'}, {"q", 'Q'}, {"r", 'R'}, {"s", 'S'}, {"t", 'T'},
    {"u", 'U'}, {"v", 'V'}, {"w", 'W'}, {"x", 'X'}, {"y", 'Y'},
    {"z", 'Z'},
    // Numbers
    {"0", '0'}, {"1", '1'}, {"2", '2'}, {"3", '3'}, {"4", '4'},
    {"5", '5'}, {"6", '6'}, {"7", '7'}, {"8", '8'}, {"9", '9'},
    // Function keys
    {"f1", VK_F1}, {"f2", VK_F2}, {"f3", VK_F3}, {"f4", VK_F4},
    {"f5", VK_F5}, {"f6", VK_F6}, {"f7", VK_F7}, {"f8", VK_F8},
    {"f9", VK_F9}, {"f10", VK_F10}, {"f11", VK_F11}, {"f12", VK_F12},
    // Control keys
    {"enter", VK_RETURN}, {"esc", VK_ESCAPE}, {"tab", VK_TAB}, {"space", VK_SPACE},
    {"caps", VK_CAPITAL}, {"capslock", VK_CAPITAL},
    {"backspace", VK_BACK}, {"delete", VK_DELETE}, {"insert", VK_INSERT},
    {"home", VK_HOME}, {"end", VK_END}, {"pgup", VK_PRIOR}, {"pgdn", VK_NEXT},
    // Arrow keys
    {"up", VK_UP}, {"down", VK_DOWN}, {"left", VK_LEFT}, {"right", VK_RIGHT},
    // Symbols
    {"~", VK_OEM_3}, {"`", VK_OEM_3}, {"-", VK_OEM_MINUS}, {"=", VK_OEM_PLUS},
    {"[", VK_OEM_4}, {"]", VK_OEM_6}, {";", VK_OEM_1}, {"'", VK_OEM_7},
    {",", VK_OEM_COMMA}, {".", VK_OEM_PERIOD}, {"/", VK_OEM_2}, {"\\", VK_OEM_5}
};

// Taskbar control
void Hide() {
    HWND h1 = FindWindow(L"Shell_TrayWnd", NULL);
    HWND h2 = FindWindow(L"Button", NULL);
    HWND h3 = FindWindow(L"Shell_SecondaryTrayWnd", NULL);
    if (h1) {
        ShowWindow(h1, SW_HIDE);
    }
    if (h2) {
        ShowWindow(h2, SW_HIDE);
    }
    if (h3) {
        ShowWindow(h3, SW_HIDE);
    }
}

void Show() {
    HWND h1 = FindWindow(L"Shell_TrayWnd", NULL);
    HWND h2 = FindWindow(L"Button", NULL);
    HWND h3 = FindWindow(L"Shell_SecondaryTrayWnd", NULL);
    if (h1) {
        ShowWindow(h1, SW_SHOW);
    }
    if (h2) {
        ShowWindow(h2, SW_SHOW);
    }
    if (h3) {
        ShowWindow(h3, SW_SHOW);
    }
}

bool IsHidden() {
    HWND h = FindWindow(L"Shell_TrayWnd", NULL);
    return h ? !IsWindowVisible(h) : false;
}

void Toggle() {
    if (IsHidden()) {
        Show();
    }
    else {
        Hide();
    }
}

// Config file operations
void Save(const Config& c) {
    ofstream f(CFG_FILE);
    f << "[Settings]\n";
    f << "Hotkey=" << c.key << "\n";
    f << "AutoRun=" << (c.run ? 1 : 0) << "\n";
    f << "AutoHideOnStartup=" << (c.hide ? 1 : 0) << "\n";
    f.close();
}

Config Load() {
    Config c{ "", false, false };
    ifstream f(CFG_FILE);
    string line;
    while (getline(f, line)) {
        if (line.find("Hotkey=") == 0) {
            c.key = line.substr(7);
        }
        else if (line.find("AutoRun=") == 0) {
            c.run = (line.substr(8) == "1");
        }
        else if (line.find("AutoHideOnStartup=") == 0) {
            c.hide = (line.substr(18) == "1");
        }
    }
    return c;
}

// Auto-start registry
void SetRun(bool enable) {
    HKEY hKey;
    const char* path = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (RegOpenKeyA(HKEY_CURRENT_USER, path, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            char exe[MAX_PATH];
            GetModuleFileNameA(NULL, exe, MAX_PATH);
            RegSetValueExA(hKey, "HideTaskbarTool", 0, REG_SZ, (BYTE*)exe, (DWORD)(strlen(exe) + 1));
        }
        else {
            RegDeleteValueA(hKey, "HideTaskbarTool");
        }
        RegCloseKey(hKey);
    }
}

// Hotkey parser
bool Parse(const string& s, UINT& mod, UINT& vk) {
    mod = 0;
    vk = 0;
    vector<string> parts;

    // Split by '+'
    size_t start = 0, pos;
    while ((pos = s.find('+', start)) != string::npos) {
        string part = s.substr(start, pos - start);
        part.erase(remove(part.begin(), part.end(), ' '), part.end());
        if (!part.empty()) {
            parts.push_back(part);
        }
        start = pos + 1;
    }
    string last = s.substr(start);
    last.erase(remove(last.begin(), last.end(), ' '), last.end());
    if (!last.empty()) {
        parts.push_back(last);
    }

    bool hasMain = false;

    for (auto& p : parts) {
        string t = p;
        transform(t.begin(), t.end(), t.begin(), ::tolower);
        if (mods.count(t)) {
            mod |= mods[t];
        }
        else if (keys.count(t)) {
            if (hasMain) {
                cout << "错误：发现多个主键：" << p << "\n";
                return false;
            }
            vk = keys[t];
            hasMain = true;
        }
        else {
            cout << "无效键位：" << t << "\n";
            return false;
        }
    }

    if (!hasMain) {
        cout << "错误：热键必须包含一个主键（字母/数字/功能键）！\n";
        cout << "示例：ctrl+h, alt+f1, shift+a\n";
        return false;
    }

    return true;
}

// Main function
int main() {
    Config c;
    UINT mod = 0, vk = 0;

    if (GetFileAttributesA(CFG_FILE) == INVALID_FILE_ATTRIBUTES) {
        // First run setup
        cout << "第一次运行，开始配置...\n";

        while (true) {
            cout << "请输入热键（格式：ctrl+alt+h）：";
            string key;
            cin >> key;
            if (Parse(key, mod, vk)) {
                c.key = key;
                break;
            }
            else {
                cout << "输入错误，请重新输入！\n";
            }
        }

        char ch;
        cout << "是否开机自启？（y/n）：";
        cin >> ch;
        c.run = (ch == 'y' || ch == 'Y');
        if (c.run) {
            SetRun(true);
        }

        if (c.run) {
            cout << "是否开机时自动隐藏任务栏？（y/n）：";
            cin >> ch;
            c.hide = (ch == 'y' || ch == 'Y');
        }
        else {
            c.hide = false;
            cout << "注意：只有开机自启时，自动隐藏功能才有效。\n";
        }

        Save(c);
        cout << "配置完成，程序将后台运行。\n";

        HWND hwnd = GetConsoleWindow();
        ShowWindow(hwnd, SW_HIDE);
    }
    else {
        // Load existing config
        c = Load();
        if (!Parse(c.key, mod, vk)) {
            cout << "配置文件中的热键无效！\n";
            return 1;
        }

        if (c.run && c.hide) {
            Hide();
        }

        HWND hwnd = GetConsoleWindow();
        ShowWindow(hwnd, SW_HIDE);
    }

    if (!RegisterHotKey(NULL, 1, mod, vk)) {
        cout << "注册热键失败！可能与其他程序冲突。\n";
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            Toggle();
        }
    }

    UnregisterHotKey(NULL, 1);
    return 0;
}