// Dear ImGui: standalone example application for Windows API + DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "ImGui\imgui.h"
#include "ImGui\imgui_internal.h"
#include "ImGui\imgui_impl_win32.h"
#include "ImGui\imgui_impl_dx11.h"
#include <d3d11.h>
#include <float.h>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <limits>
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <winreg.h>
#include <commdlg.h>
#include <tlhelp32.h>
#include <nlohmann/json.hpp>
#include "..\\shared.h"

#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif


static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static bool                     g_in_sizemove = false;

static HWND                     g_hwnd = nullptr;
static HWND                     g_titlebar_hwnd = nullptr;
static HWND                     g_host_hwnd = nullptr;

static ImFont* g_font_main = nullptr;
static ImFont* g_font_title = nullptr;

static bool g_dark_mode = true;
static bool g_need_theme_refresh = true;
static ULONGLONG g_last_theme_check = 0;
static int g_theme_mode = 0; 
static int g_language = 0;
static float g_min_window_w = 820.0f;
static float g_min_window_h = 560.0f;
static std::vector<HWND> g_styled_hwnds;
static std::string g_theme_items;
static std::string g_language_items;
static bool g_show_language_prompt = false;
static int g_language_prompt_choice = 0;
static bool g_show_disclaimer_prompt = false;

struct TitleBarState
{
    RECT bar;
    RECT btn_min;
    RECT btn_close;
};

static TitleBarState g_titlebar = {};

using json = nlohmann::json;

struct LauncherConfig
{
    std::wstring game_path;
    bool inject_enabled = true;
    bool inject_unlock_fps = false;
    int inject_target_fps = 120;
    bool use_launch_args = false;
    bool use_popupwindow = false;
    bool use_custom_args = false;
    std::string custom_args;
    int theme_mode = 0;
    int language = 0;
    bool language_set = false;
    bool disclaimer_accepted = false;
};

static LauncherConfig g_config = {};
static std::wstring g_config_path;

static ImVec4 g_clear_color = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
static ImVec4 g_accent = ImVec4(0.00f, 0.47f, 0.83f, 1.0f);
static ImVec4 g_app_bg = ImVec4(0.10f, 0.10f, 0.11f, 1.0f);
static ImVec4 g_panel_bg = ImVec4(0.13f, 0.13f, 0.14f, 1.0f);
static ImVec4 g_card_bg = ImVec4(0.16f, 0.16f, 0.18f, 1.0f);
static ImVec4 g_nav_bg = ImVec4(0.11f, 0.11f, 0.12f, 1.0f);
static ImVec4 g_nav_hover = ImVec4(0.16f, 0.16f, 0.18f, 1.0f);
static ImVec4 g_nav_active = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
static ImVec4 g_separator = ImVec4(0.26f, 0.26f, 0.28f, 1.0f);

static HANDLE g_shared_map = nullptr;
static SharedData* g_shared = nullptr;
static SharedData g_shared_last = {};
static bool g_is_admin = false;

static std::wstring Utf8ToWide(const std::string& text);
static void SaveConfig(const LauncherConfig& config);

enum class Language
{
    ZhCN = 0,
    ZhTW = 1,
    EnUS = 2
};

enum class TextKey
{
    SelectEndfieldTitle,
    SelectEndfieldTip,
    HintTitle,
    ErrorNeedAdmin,
    ErrorGameNotFound,
    ErrorInvalidProcessHandle,
    ErrorDllNotFound,
    ErrorVirtualAllocFailed,
    ErrorWriteProcessMemoryFailed,
    ErrorLoadLibraryMissing,
    ErrorCreateRemoteThreadFailed,
    ErrorLoadLibraryFailed,
    ErrorCreateProcessFailed,
    LaunchStatusReady,
    LaunchStatusStarted,
    NavLaunch,
    NavWindow,
    NavAbout,
    PageTitleLaunch,
    PageTitleWindow,
    PageTitleAbout,
    PageSubtitleLaunch,
    PageSubtitleWindow,
    PageSubtitleAbout,
    GamePath,
    NotSet,
    AdminRequiredForFeatures,
    GameNotFoundHint,
    SelectGameLocation,
    AutoDetectGamePath,
    AutoDetectSuccess,
    AutoDetectFail,
    SectionInject,
    InjectWarning,
    InjectDisabledNoAdmin,
    EnableInjection,
    UnlockFps,
    TargetFps,
    QuickFps,
    QuickFpsUnlimited,
    LaunchArgs,
    UseLaunchArgs,
    UsePopupWindow,
    CustomLaunchArgs,
    ArgsLabel,
    LaunchGame,
    StatusLabel,
    SectionAppearance,
    ThemeMode,
    ThemeFollowSystem,
    ThemeDark,
    ThemeLight,
    ThemeHint,
    LanguageLabel,
    LanguageZhCN,
    LanguageZhTW,
    LanguageEnUS,
    LanguagePromptTitle,
    LanguagePromptBody,
    LanguagePromptHint,
    LanguagePromptConfirm,
    CommonOk,
    CommonCancel,
    SectionAbout,
    ProjectSource,
    DisclaimerLine1,
    DisclaimerLine2,
    DisclaimerLine3,
    BuildTime,
    DisclaimerDialogTitle,
    DisclaimerDialogBody
};

struct LanguageOption
{
    Language id;
    TextKey name_key;
};

static const LanguageOption kLanguageOptions[] =
{
    { Language::ZhCN, TextKey::LanguageZhCN },
    { Language::ZhTW, TextKey::LanguageZhTW },
    { Language::EnUS, TextKey::LanguageEnUS }
};

static int ClampLanguageIndex(int value)
{
    const int count = (int)(sizeof(kLanguageOptions) / sizeof(kLanguageOptions[0]));
    if (value < 0 || value >= count)
        return 0;
    return value;
}

struct TextEntry
{
    const char* zh_cn;
    const char* zh_tw;
    const char* en;
};

static const TextEntry kTextTable[] =
{
    { "请选择 Endfield.exe", "請選擇 Endfield.exe", "Select Endfield.exe" },
    { "请选择 Endfield.exe。", "請選擇 Endfield.exe。", "Please select Endfield.exe." },
    { "提示", "提示", "Notice" },
    { "请以管理员权限运行启动器。", "請以管理員權限執行啟動器。", "Please run the launcher as administrator." },
    { "未找到游戏可执行文件。请选择游戏路径。", "未找到遊戲可執行檔。請選擇遊戲路徑。", "Game executable not found. Please select the game path." },
    { "无效的进程句柄。", "無效的行程控制代碼。", "Invalid process handle." },
    { "未找到 UnlockerIsland.dll。", "未找到 UnlockerIsland.dll。", "UnlockerIsland.dll not found." },
    { "VirtualAllocEx 失败。", "VirtualAllocEx 失敗。", "VirtualAllocEx failed." },
    { "WriteProcessMemory 失败。", "WriteProcessMemory 失敗。", "WriteProcessMemory failed." },
    { "LoadLibraryW 不可用。", "LoadLibraryW 無法使用。", "LoadLibraryW not available." },
    { "CreateRemoteThread 失败。", "CreateRemoteThread 失敗。", "CreateRemoteThread failed." },
    { "目标进程 LoadLibraryW 失败。", "目標行程 LoadLibraryW 失敗。", "LoadLibraryW failed in target process." },
    { "CreateProcess 失败。", "CreateProcess 失敗。", "CreateProcess failed." },
    { "就绪", "就緒", "Ready" },
    { "已启动", "已啟動", "Started" },
    { "启动游戏", "啟動遊戲", "Launch" },
    { "窗口设置", "視窗設定", "Settings" },
    { "关于", "關於", "About" },
    { "启动游戏", "啟動遊戲", "Launch" },
    { "窗口设置", "視窗設定", "Settings" },
    { "关于", "關於", "About" },
    { "启动与注入", "啟動與注入", "Launch & Injection" },
    { "外观与偏好", "外觀與偏好", "Appearance & Preferences" },
    { "说明", "說明", "Info" },
    { "游戏路径", "遊戲路徑", "Game Path" },
    { "未设置", "未設定", "Not set" },
    { "游戏文件管理/切换游戏服务器/注入功能需要管理员权限，目前无法获取权限，相关功能已禁用。", "遊戲檔案管理／切換遊戲伺服器／注入功能需要管理員權限，目前無法取得權限，相關功能已停用。", "Managing game files, switching servers, and injection require administrator rights. These features are disabled." },
    { "未找到游戏，请选择路径。", "未找到遊戲，請選擇路徑。", "Game not found. Please select a path." },
    { "选择游戏位置", "選擇遊戲位置", "Choose Game Location" },
    { "自动获取游戏路径", "自動取得遊戲路徑", "Auto Detect Game Path" },
    { "获取成功，已自动关闭游戏，请使用本启动器的启动游戏功能。", "取得成功，已自動關閉遊戲，請使用本啟動器的啟動遊戲功能。", "Path detected. The game was closed; please launch it from this launcher." },
    { "未检测到正在运行的游戏进程，请先启动游戏后再点击获取。", "未偵測到正在執行的遊戲行程，請先啟動遊戲後再點擊取得。", "No running game process detected. Start the game first, then try again." },
    { "注入", "注入", "Injection" },
    { "注入功能非常危险且有可能会造成严重后果，请谨慎使用。", "注入功能非常危險，且可能造成嚴重後果，請謹慎使用。", "Injection is dangerous and may cause serious issues. Use with caution." },
    { "未以管理员权限运行，注入功能已禁用。", "未以管理員權限執行，注入功能已停用。", "Not running as administrator. Injection is disabled." },
    { "启用注入功能 (将模块注入游戏，以便实现一些高级但危险的功能)", "啟用注入功能（將模組注入遊戲，以實現部分進階但危險的功能）", "Enable injection (inject modules to enable advanced but risky features)" },
    { "开启解帧", "啟用解幀", "Unlock FPS" },
    { "目标帧数", "目標幀數", "Target FPS" },
    { "快捷帧数", "快速幀數", "Quick FPS" },
    { "21亿(不限制)", "21 億（不限制）", "2.1B (Unlimited)" },
    { "启动参数", "啟動參數", "Launch Arguments" },
    { "使用启动参数", "使用啟動參數", "Use launch arguments" },
    { "使用 -popupwindow", "使用 -popupwindow", "Use -popupwindow" },
    { "自定义启动参数", "自訂啟動參數", "Custom launch arguments" },
    { "参数", "參數", "Arguments" },
    { "启动游戏", "啟動遊戲", "Launch Game" },
    { "状态: %s", "狀態：%s", "Status: %s" },
    { "外观", "外觀", "Appearance" },
    { "主题模式", "主題模式", "Theme Mode" },
    { "跟随系统", "跟隨系統", "Follow System" },
    { "深色", "深色", "Dark" },
    { "浅色", "淺色", "Light" },
    { "跟随Windows系统设置修改界面颜色。", "依照 Windows 系統設定變更介面顏色。", "Follow Windows settings for the UI theme." },
    { "语言", "語言", "Language" },
    { "简体中文", "簡體中文", "Simplified Chinese" },
    { "繁體中文", "繁體中文", "Traditional Chinese" },
    { "English", "English", "English" },
    { "选择语言", "選擇語言", "Select Language" },
    { "请选择要使用的语言。", "請選擇要使用的語言。", "Please select the language to use." },
    { "稍后可以在「窗口设置」中切换语言。", "稍後可在「視窗設定」中切換語言。", "You can change it later in Settings." },
    { "确定", "確定", "OK" },
    { "确定", "確定", "OK" },
    { "取消", "取消", "Cancel" },
    { "关于", "關於", "About" },
    { "本项目开源地址：", "本專案開源位址：", "Project source:" },
    { "免责声明：本工具仅供学习与测试使用。", "免責聲明：本工具僅供學習與測試使用。", "Disclaimer: This tool is for learning and testing only." },
    { "使用注入功能可能违反相关条款或造成数据损坏等后果，", "使用注入功能可能違反相關條款或造成資料損毀等後果，", "Injection may violate terms or cause data loss," },
    { "请自行承担所有风险与责任。", "請自行承擔所有風險與責任。", "use at your own risk." },
    { "构建时间: %s %s", "建置時間：%s %s", "Build time: %s %s" },
    { "免责声明", "免責聲明", "Disclaimer" },
    { "免责声明：本工具为第三方非官方程序，使用可能违反游戏条款并导致封号等后果，作者不承担任何责任。\n"
      "如有侵权请联系删除。\n\n"
      "是否继续？",
      "免責聲明：本工具為第三方非官方程式，使用可能違反遊戲條款並導致封號等後果，作者不承擔任何責任。\n"
      "如有侵權請聯絡刪除。\n\n"
      "是否繼續？",
      "Disclaimer: This is an unofficial third-party tool. Using it may violate game terms and lead to penalties. The author assumes no responsibility.\n"
      "If there is any infringement, please contact for removal.\n\n"
      "Continue?" }
};

static const char* T(TextKey key)
{
    const TextEntry& entry = kTextTable[static_cast<size_t>(key)];
    int lang_index = ClampLanguageIndex(g_language);
    if (lang_index == static_cast<int>(Language::EnUS) && entry.en && entry.en[0] != '\0')
        return entry.en;
    else if (lang_index == static_cast<int>(Language::ZhTW) && entry.zh_tw && entry.zh_tw[0] != '\0')
        return entry.zh_tw;
    return entry.zh_cn;
}

static const char* TByLanguage(TextKey key, Language lang)
{
    const TextEntry& entry = kTextTable[static_cast<size_t>(key)];
    if (lang == Language::EnUS && entry.en && entry.en[0] != '\0')
        return entry.en;
    else if (lang == Language::ZhTW && entry.zh_tw && entry.zh_tw[0] != '\0')
        return entry.zh_tw;
    return entry.zh_cn;
}

static std::wstring TW(TextKey key)
{
    return Utf8ToWide(T(key));
}

static std::string BuildComboItems(const std::vector<const char*>& items)
{
    std::string result;
    for (const char* item : items)
    {
        result += item ? item : "";
        result.push_back('\0');
    }
    result.push_back('\0');
    return result;
}

static void RefreshLanguageItems()
{
    g_theme_items = BuildComboItems({ T(TextKey::ThemeFollowSystem), T(TextKey::ThemeDark), T(TextKey::ThemeLight) });
    std::vector<const char*> items;
    items.reserve((size_t)(sizeof(kLanguageOptions) / sizeof(kLanguageOptions[0])));
    for (size_t i = 0; i < (sizeof(kLanguageOptions) / sizeof(kLanguageOptions[0])); ++i)
        items.push_back(TByLanguage(kLanguageOptions[i].name_key, kLanguageOptions[i].id));
    g_language_items = BuildComboItems(items);
}

static void RenderLanguagePromptModal(float scale)
{
    if (!g_show_language_prompt)
        return;

    static bool popup_opened = false;
    
    if (g_show_language_prompt && !popup_opened)
    {
        g_language_prompt_choice = ClampLanguageIndex(g_language);
        ImGui::OpenPopup("##language_prompt");
        popup_opened = true;
    }

    HMONITOR monitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = { sizeof(mi) };
    if (GetMonitorInfo(monitor, &mi))
    {
        int screen_width = mi.rcWork.right - mi.rcWork.left;
        int screen_height = mi.rcWork.bottom - mi.rcWork.top;
        float center_x = (float)(mi.rcWork.left + screen_width / 2);
        float center_y = (float)(mi.rcWork.top + screen_height / 2);
        ImGui::SetNextWindowPos(ImVec2(center_x, center_y), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }
    
    if (ImGui::BeginPopupModal("##language_prompt", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // 根据当前选择的语言索引获取对应的Language枚举值
        Language current_lang = kLanguageOptions[ClampLanguageIndex(g_language_prompt_choice)].id;
        
        ImGui::TextUnformatted(TByLanguage(TextKey::LanguagePromptTitle, current_lang));
        ImGui::Separator();
        ImGui::TextUnformatted(TByLanguage(TextKey::LanguagePromptBody, current_lang));
        ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));
        ImGui::TextDisabled(TByLanguage(TextKey::LanguagePromptHint, current_lang));
        ImGui::Separator();

        const int count = (int)(sizeof(kLanguageOptions) / sizeof(kLanguageOptions[0]));
        for (int i = 0; i < count; ++i)
        {
            ImGui::RadioButton(TByLanguage(kLanguageOptions[i].name_key, kLanguageOptions[i].id), &g_language_prompt_choice, i);
        }

        ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));
        if (ImGui::Button(TByLanguage(TextKey::LanguagePromptConfirm, current_lang)))
        {
            g_language = ClampLanguageIndex(g_language_prompt_choice);
            g_config.language = g_language;
            g_config.language_set = true;
            SaveConfig(g_config);
            RefreshLanguageItems();
            g_show_language_prompt = false;
            popup_opened = false;
            ImGui::CloseCurrentPopup();
            
            if (!g_config.disclaimer_accepted)
            {
                g_show_disclaimer_prompt = true;
            }
        }
        ImGui::EndPopup();
    }
    
    if (!ImGui::IsPopupOpen("##language_prompt") && popup_opened)
    {
        popup_opened = false;
        g_show_language_prompt = false;
    }
}

static void RenderDisclaimerModal(float scale)
{
    if (!g_show_disclaimer_prompt)
        return;

    static bool popup_opened = false;
    
    if (g_show_disclaimer_prompt && !popup_opened)
    {
        ImGui::OpenPopup("##disclaimer_prompt");
        popup_opened = true;
    }

    HMONITOR monitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = { sizeof(mi) };
    if (GetMonitorInfo(monitor, &mi))
    {
        int screen_width = mi.rcWork.right - mi.rcWork.left;
        int screen_height = mi.rcWork.bottom - mi.rcWork.top;
        float center_x = (float)(mi.rcWork.left + screen_width / 2);
        float center_y = (float)(mi.rcWork.top + screen_height / 2);
        float window_width = 500.0f * scale;
        float window_height = 280.0f * scale;
        ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(center_x, center_y), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }
    
    if (ImGui::BeginPopupModal("##disclaimer_prompt", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        ImGui::TextUnformatted(T(TextKey::DisclaimerDialogTitle));
        ImGui::Separator();
        
        ImGui::BeginChild("##disclaimer_text", ImVec2(0, 150.0f * scale), true, ImGuiWindowFlags_None);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextWrapped("%s", T(TextKey::DisclaimerDialogBody));
        ImGui::PopTextWrapPos();
        ImGui::EndChild();
        
        ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));
        
        if (ImGui::Button(T(TextKey::CommonOk), ImVec2(100.0f * scale, 0)))
        {
            g_config.disclaimer_accepted = true;
            SaveConfig(g_config);
            g_show_disclaimer_prompt = false;
            popup_opened = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(T(TextKey::CommonCancel), ImVec2(100.0f * scale, 0)))
        {
            g_show_disclaimer_prompt = false;
            popup_opened = false;
            ImGui::CloseCurrentPopup();
            PostQuitMessage(0);
        }
        ImGui::EndPopup();
    }

    if (!ImGui::IsPopupOpen("##disclaimer_prompt") && popup_opened)
    {
        popup_opened = false;
        g_show_disclaimer_prompt = false;
    }
}

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

#pragma comment(lib, "dwmapi.lib")

static ImVec4 ColorFromBytes(int r, int g, int b, int a = 255)
{
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

static std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty())
        return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (len <= 1)
        return L"";
    std::wstring wide;
    wide.resize(static_cast<size_t>(len - 1));
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), len);
    return wide;
}

static std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty())
        return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1)
        return "";
    std::string utf8;
    utf8.resize(static_cast<size_t>(len - 1));
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, utf8.data(), len, nullptr, nullptr);
    return utf8;
}

static bool FileExistsW(const std::wstring& path)
{
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static std::wstring GetModuleDirectory()
{
    wchar_t buffer[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
        return L".";
    std::wstring full(buffer, len);
    size_t pos = full.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
        return L".";
    return full.substr(0, pos);
}

static std::wstring GetDefaultDllPath()
{
    std::wstring base = GetModuleDirectory();
    std::wstring local = base + L"\\UnlockerIsland.dll";
    if (FileExistsW(local))
        return local;
}

static std::wstring GetDirectoryFromPath(const std::wstring& path)
{
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
        return L".";
    return path.substr(0, pos);
}

static bool IsRunningAsAdmin()
{
    BOOL is_admin = FALSE;
    PSID admin_group = nullptr;
    SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&nt_authority, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &admin_group))
    {
        CheckTokenMembership(nullptr, admin_group, &is_admin);
        FreeSid(admin_group);
    }
    return is_admin == TRUE;
}

static void LoadConfig(LauncherConfig* config)
{
    if (!config)
        return;
    if (g_config_path.empty())
        g_config_path = GetModuleDirectory() + L"\\config.json";
    if (!FileExistsW(g_config_path))
        return;
    std::ifstream file(g_config_path);
    if (!file.is_open())
        return;
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    try
    {
        json j = json::parse(buffer.str(), nullptr, false);
        if (j.is_discarded())
            return;
        if (j.contains("game_path"))
            config->game_path = Utf8ToWide(j.value("game_path", ""));
        config->inject_enabled = j.value("inject_enabled", config->inject_enabled);
        config->inject_unlock_fps = j.value("inject_unlock_fps", config->inject_unlock_fps);
        config->inject_target_fps = j.value("inject_target_fps", config->inject_target_fps);
        config->use_launch_args = j.value("use_launch_args", config->use_launch_args);
        config->use_popupwindow = j.value("use_popupwindow", config->use_popupwindow);
        config->use_custom_args = j.value("use_custom_args", config->use_custom_args);
        config->custom_args = j.value("custom_args", config->custom_args);
        config->theme_mode = j.value("theme_mode", config->theme_mode);
        config->language = j.value("language", config->language);
        config->language_set = j.value("language_set", config->language_set);
        config->disclaimer_accepted = j.value("disclaimer_accepted", config->disclaimer_accepted);
    }
    catch (...)
    {
    }
}

static void SaveConfig(const LauncherConfig& config)
{
    if (g_config_path.empty())
        g_config_path = GetModuleDirectory() + L"\\config.json";
    json j;
    j["game_path"] = WideToUtf8(config.game_path);
    j["inject_enabled"] = config.inject_enabled;
    j["inject_unlock_fps"] = config.inject_unlock_fps;
    j["inject_target_fps"] = config.inject_target_fps;
    j["use_launch_args"] = config.use_launch_args;
    j["use_popupwindow"] = config.use_popupwindow;
    j["use_custom_args"] = config.use_custom_args;
    j["custom_args"] = config.custom_args;
    j["theme_mode"] = config.theme_mode;
    j["language"] = config.language;
    j["language_set"] = config.language_set;
    j["disclaimer_accepted"] = config.disclaimer_accepted;
    std::ofstream file(g_config_path, std::ios::trunc);
    if (!file.is_open())
        return;
    file << j.dump(4);
    file.close();
}

static std::wstring SelectGameExecutable(HWND owner)
{
    wchar_t file_path[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Endfield.exe\0Endfield.exe\0";
    std::wstring title = TW(TextKey::SelectEndfieldTitle);
    ofn.lpstrTitle = title.c_str();
    ofn.lpstrFile = file_path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameW(&ofn))
    {
        std::wstring selected(file_path);
        size_t pos = selected.find_last_of(L"\\/");
        std::wstring filename = (pos == std::wstring::npos) ? selected : selected.substr(pos + 1);
        if (_wcsicmp(filename.c_str(), L"Endfield.exe") == 0)
            return selected;
        MessageBoxW(owner, TW(TextKey::SelectEndfieldTip).c_str(), TW(TextKey::HintTitle).c_str(), MB_OK | MB_ICONWARNING);
    }
    return L"";
}

static bool FindRunningGamePath(std::wstring* out_path, DWORD* out_pid)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    std::wstring result;
    DWORD pid = 0;
    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(entry.szExeFile, L"Endfield.exe") == 0)
            {
                HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
                if (process)
                {
                    wchar_t path[MAX_PATH] = {};
                    DWORD size = (DWORD)(sizeof(path) / sizeof(path[0]));
                    if (QueryFullProcessImageNameW(process, 0, path, &size))
                    {
                        result = path;
                        pid = entry.th32ProcessID;
                    }
                    CloseHandle(process);
                }
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    if (!result.empty())
    {
        if (out_path)
            *out_path = result;
        if (out_pid)
            *out_pid = pid;
        return true;
    }
    return false;
}

static bool ReadSystemDarkMode()
{
    DWORD value = 1;
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size) == ERROR_SUCCESS)
        return value == 0;
    return true;
}

static void ApplyTheme(bool dark, float scale)
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f * scale;
    style.ChildRounding = 8.0f * scale;
    style.FrameRounding = 6.0f * scale;
    style.PopupRounding = 6.0f * scale;
    style.WindowBorderSize = 1.0f * scale;
    style.FrameBorderSize = 1.0f * scale;
    style.ScrollbarSize = 12.0f * scale;
    style.GrabRounding = 6.0f * scale;
    style.WindowPadding = ImVec2(14.0f * scale, 10.0f * scale);
    style.FramePadding = ImVec2(10.0f * scale, 6.0f * scale);
    style.ItemSpacing = ImVec2(8.0f * scale, 6.0f * scale);
    style.ItemInnerSpacing = ImVec2(6.0f * scale, 4.0f * scale);
    style.CellPadding = ImVec2(6.0f * scale, 6.0f * scale);
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);

    if (dark)
    {
        g_clear_color = ColorFromBytes(20, 20, 22, 220);
        g_accent = ColorFromBytes(0, 120, 212);
        g_app_bg = ColorFromBytes(26, 26, 28, 200);
        g_panel_bg = ColorFromBytes(33, 33, 35, 210);
        g_card_bg = ColorFromBytes(40, 40, 43, 220);
        g_nav_bg = ColorFromBytes(29, 29, 31, 200);
        g_nav_hover = ColorFromBytes(45, 45, 48, 210);
        g_nav_active = ColorFromBytes(52, 52, 56, 220);
        g_separator = ColorFromBytes(70, 70, 74);
    }
    else
    {
        g_clear_color = ColorFromBytes(240, 240, 242, 200);
        g_accent = ColorFromBytes(0, 120, 212);
        g_app_bg = ColorFromBytes(244, 244, 246, 200);
        g_panel_bg = ColorFromBytes(255, 255, 255, 210);
        g_card_bg = ColorFromBytes(248, 248, 250, 220);
        g_nav_bg = ColorFromBytes(246, 246, 248, 200);
        g_nav_hover = ColorFromBytes(230, 230, 235, 210);
        g_nav_active = ColorFromBytes(224, 224, 230, 220);
        g_separator = ColorFromBytes(210, 210, 214);
    }

    style.Colors[ImGuiCol_Text] = dark ? ColorFromBytes(240, 240, 240) : ColorFromBytes(20, 20, 20);
    style.Colors[ImGuiCol_TextDisabled] = dark ? ColorFromBytes(140, 140, 140) : ColorFromBytes(120, 120, 120);
    style.Colors[ImGuiCol_WindowBg] = g_app_bg;
    style.Colors[ImGuiCol_ChildBg] = g_panel_bg;
    style.Colors[ImGuiCol_PopupBg] = g_panel_bg;
    style.Colors[ImGuiCol_Border] = g_separator;
    style.Colors[ImGuiCol_FrameBg] = dark ? ColorFromBytes(40, 40, 45) : ColorFromBytes(235, 235, 238);
    style.Colors[ImGuiCol_FrameBgHovered] = dark ? ColorFromBytes(55, 55, 60) : ColorFromBytes(220, 220, 224);
    style.Colors[ImGuiCol_FrameBgActive] = dark ? ColorFromBytes(64, 64, 70) : ColorFromBytes(210, 210, 216);
    style.Colors[ImGuiCol_Button] = dark ? ColorFromBytes(45, 45, 50) : ColorFromBytes(232, 232, 236);
    style.Colors[ImGuiCol_ButtonHovered] = dark ? ColorFromBytes(58, 58, 64) : ColorFromBytes(220, 220, 225);
    style.Colors[ImGuiCol_ButtonActive] = dark ? ColorFromBytes(70, 70, 76) : ColorFromBytes(210, 210, 216);
    style.Colors[ImGuiCol_CheckMark] = g_accent;
    style.Colors[ImGuiCol_SliderGrab] = g_accent;
    style.Colors[ImGuiCol_SliderGrabActive] = g_accent;
    style.Colors[ImGuiCol_Separator] = g_separator;
    style.Colors[ImGuiCol_SeparatorHovered] = g_separator;
    style.Colors[ImGuiCol_SeparatorActive] = g_separator;
    style.Colors[ImGuiCol_ResizeGrip] = g_accent;
    style.Colors[ImGuiCol_ResizeGripHovered] = g_accent;
    style.Colors[ImGuiCol_ResizeGripActive] = g_accent;
    style.Colors[ImGuiCol_Header] = dark ? ColorFromBytes(45, 45, 50) : ColorFromBytes(230, 230, 234);
    style.Colors[ImGuiCol_HeaderHovered] = dark ? ColorFromBytes(55, 55, 60) : ColorFromBytes(220, 220, 224);
    style.Colors[ImGuiCol_HeaderActive] = dark ? ColorFromBytes(64, 64, 70) : ColorFromBytes(210, 210, 216);
    style.Colors[ImGuiCol_ScrollbarBg] = g_panel_bg;
    style.Colors[ImGuiCol_ScrollbarGrab] = dark ? ColorFromBytes(60, 60, 66) : ColorFromBytes(200, 200, 205);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = dark ? ColorFromBytes(70, 70, 76) : ColorFromBytes(188, 188, 194);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = dark ? ColorFromBytes(80, 80, 86) : ColorFromBytes(176, 176, 184);
    style.Colors[ImGuiCol_TitleBg] = g_panel_bg;
    style.Colors[ImGuiCol_TitleBgActive] = g_panel_bg;
    style.Colors[ImGuiCol_TitleBgCollapsed] = g_panel_bg;
}

static void SetDwmDarkMode(HWND hwnd, bool dark)
{
    if (!hwnd)
        return;
    const BOOL use_dark = dark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark, sizeof(use_dark));
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &use_dark, sizeof(use_dark));
}

static void UpdateViewportEffects(bool dark)
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int i = 0; i < platform_io.Viewports.Size; ++i)
    {
        ImGuiViewport* vp = platform_io.Viewports[i];
        HWND hwnd = (HWND)vp->PlatformHandle;
        if (!hwnd || hwnd == g_host_hwnd)
            continue;
        SetDwmDarkMode(hwnd, dark);
        bool styled = false;
        for (size_t idx = 0; idx < g_styled_hwnds.size(); ++idx)
        {
            if (g_styled_hwnds[idx] == hwnd)
            {
                styled = true;
                break;
            }
        }
        if (!styled)
        {
            LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
            LONG_PTR new_style = (style & ~WS_MAXIMIZEBOX) | WS_MINIMIZEBOX;
            if (new_style != style)
            {
                SetWindowLongPtr(hwnd, GWL_STYLE, new_style);
                SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            }
            g_styled_hwnds.push_back(hwnd);
        }
    }
}

static void InitSharedMemory()
{
    if (g_shared_map)
        return;
    g_shared_map = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
        (DWORD)sizeof(SharedData), kMapName);
    if (!g_shared_map)
        return;
    g_shared = (SharedData*)MapViewOfFile(g_shared_map, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    if (!g_shared)
    {
        CloseHandle(g_shared_map);
        g_shared_map = nullptr;
        return;
    }
    g_shared->magic = kMagic;
    g_shared->seq = 0;
    g_shared->alive = 1;
    g_shared->unlock_enabled = 0;
    g_shared->target_fps = 0;
    g_shared_last = *g_shared;
}

static void ShutdownSharedMemory()
{
    if (g_shared)
    {
        g_shared->alive = 0;
        InterlockedIncrement(&g_shared->seq);
        UnmapViewOfFile(g_shared);
        g_shared = nullptr;
    }
    if (g_shared_map)
    {
        CloseHandle(g_shared_map);
        g_shared_map = nullptr;
    }
}

static void UpdateSharedMemoryFromConfig()
{
    if (!g_shared)
        return;

    const int32_t kMin = (std::numeric_limits<int32_t>::min)();
    const int32_t kMax = (std::numeric_limits<int32_t>::max)();
    if (g_config.inject_target_fps < kMin)
        g_config.inject_target_fps = kMin;
    if (g_config.inject_target_fps > kMax)
        g_config.inject_target_fps = kMax;

    LONG new_unlock = (g_config.inject_enabled && g_config.inject_unlock_fps) ? 1 : 0;
    int32_t new_fps = (int32_t)g_config.inject_target_fps;

    if (g_shared_last.unlock_enabled != new_unlock ||
        g_shared_last.target_fps != new_fps)
    {
        g_shared->magic = kMagic;
        g_shared->unlock_enabled = new_unlock;
        g_shared->target_fps = new_fps;
        InterlockedIncrement(&g_shared->seq);
        g_shared_last = *g_shared;
    }
}

static void OpenUrl(const char* url)
{
    if (!url || !*url)
        return;
    std::wstring wide = Utf8ToWide(url);
    if (wide.empty())
        return;
    ShellExecuteW(nullptr, L"open", wide.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

static void RenderLinkText(const char* label, const char* url, const ImVec4& normal, const ImVec4& hover)
{
    ImGui::TextColored(normal, "%s", label);
    bool hovered = ImGui::IsItemHovered();
    if (hovered)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        ImGui::SetTooltip("%s", url);
        ImVec2 underline_min = ImGui::GetItemRectMin();
        ImVec2 underline_max = ImGui::GetItemRectMax();
        underline_min.y += ImGui::GetTextLineHeight();
        underline_max.y += 1.0f;
        ImGui::GetWindowDrawList()->AddLine(
            underline_min,
            underline_max,
            ImGui::ColorConvertFloat4ToU32(hover),
            1.0f);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            OpenUrl(url);
    }
}

static std::wstring BuildLaunchArguments(const LauncherConfig& config)
{
    std::wstring args;
    if (config.use_launch_args)
    {
        if (config.use_popupwindow)
            args += L" -popupwindow";
        if (config.use_custom_args && !config.custom_args.empty())
        {
            args += L" ";
            args += Utf8ToWide(config.custom_args);
        }
    }
    return args;
}

static bool InjectDllLoadLibrary(HANDLE process, const std::wstring& dll_path, std::wstring* error)
{
    if (!process)
    {
        if (error)
            *error = TW(TextKey::ErrorInvalidProcessHandle);
            return false;
        }
    if (!FileExistsW(dll_path))
    {
        if (error)
            *error = TW(TextKey::ErrorDllNotFound);
            return false;
        }

    size_t bytes = (dll_path.size() + 1) * sizeof(wchar_t);
    LPVOID remote_mem = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_mem)
    {
        if (error)
            *error = TW(TextKey::ErrorVirtualAllocFailed);
            return false;
        }

    SIZE_T written = 0;
    if (!WriteProcessMemory(process, remote_mem, dll_path.c_str(), bytes, &written) || written != bytes)
    {
        VirtualFreeEx(process, remote_mem, 0, MEM_RELEASE);
        if (error)
            *error = TW(TextKey::ErrorWriteProcessMemoryFailed);
            return false;
        }

    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    FARPROC load_library = kernel ? GetProcAddress(kernel, "LoadLibraryW") : nullptr;
    if (!load_library)
    {
        VirtualFreeEx(process, remote_mem, 0, MEM_RELEASE);
        if (error)
            *error = TW(TextKey::ErrorLoadLibraryMissing);
            return false;
        }

    HANDLE thread = CreateRemoteThread(process, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(load_library), remote_mem, 0, nullptr);
    if (!thread)
    {
        VirtualFreeEx(process, remote_mem, 0, MEM_RELEASE);
        if (error)
            *error = TW(TextKey::ErrorCreateRemoteThreadFailed);
            return false;
        }

    WaitForSingleObject(thread, 5000);
    DWORD exit_code = 0;
    GetExitCodeThread(thread, &exit_code);
    CloseHandle(thread);
    VirtualFreeEx(process, remote_mem, 0, MEM_RELEASE);
    if (exit_code == 0)
    {
        if (error)
            *error = TW(TextKey::ErrorLoadLibraryFailed);
            return false;
        }
    return true;
}

static bool LaunchGameWithOptionalInjection(const LauncherConfig& config, std::wstring* error)
{
    if (!g_is_admin)
    {
        if (error)
            *error = TW(TextKey::ErrorNeedAdmin);
            return false;
        }
    if (config.game_path.empty() || !FileExistsW(config.game_path))
    {
        if (error)
            *error = TW(TextKey::ErrorGameNotFound);
            return false;
        }

    std::wstring current_dir = GetDirectoryFromPath(config.game_path);
    std::wstring args = BuildLaunchArguments(config);
    std::wstring cmdline = L"\"" + config.game_path + L"\"" + args;
    std::vector<wchar_t> cmd_buffer(cmdline.begin(), cmdline.end());
    cmd_buffer.push_back(L'\0');

    STARTUPINFOW si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);
    DWORD create_flags = config.inject_enabled ? CREATE_SUSPENDED : 0;
    BOOL ok = CreateProcessW(
        nullptr,
        cmd_buffer.data(),
        nullptr,
        nullptr,
        FALSE,
        create_flags,
        nullptr,
        current_dir.c_str(),
        &si,
        &pi);
    if (!ok)
    {
        if (error)
            *error = TW(TextKey::ErrorCreateProcessFailed);
            return false;
        }

    bool result = true;
    if (config.inject_enabled)
    {
        std::wstring dll_path = GetDefaultDllPath();
        if (!InjectDllLoadLibrary(pi.hProcess, dll_path, error))
        {
            TerminateProcess(pi.hProcess, 1);
            result = false;
        }
        else
        {
            ResumeThread(pi.hThread);
        }
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return result;
}

static void LimitFrameRate(double target_fps)
{
    if (target_fps <= 0.0)
        return;
    static LARGE_INTEGER freq = {};
    static LARGE_INTEGER last = {};
    if (freq.QuadPart == 0)
    {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&last);
        return;
    }

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double elapsed_ms = (double)(now.QuadPart - last.QuadPart) * 1000.0 / (double)freq.QuadPart;
    double target_ms = 1000.0 / target_fps;
    if (elapsed_ms < target_ms)
    {
        DWORD sleep_ms = (DWORD)(target_ms - elapsed_ms);
        if (sleep_ms > 1)
            Sleep(sleep_ms - 1);
        do
        {
            QueryPerformanceCounter(&now);
            elapsed_ms = (double)(now.QuadPart - last.QuadPart) * 1000.0 / (double)freq.QuadPart;
        } while (elapsed_ms < target_ms);
    }
    last = now;
}

static double DetermineTargetFps()
{
    HWND fg = ::GetForegroundWindow();
    if (g_titlebar_hwnd && fg == g_titlebar_hwnd)
        return 120.0;
    if (g_hwnd && fg == g_hwnd)
        return 120.0;
    return 30.0;
}

static float LerpFloat(float a, float b, float t)
{
    return a + (b - a) * t;
}

static float SmoothStep(float t)
{
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

static bool NavigationItem(const char* label, bool active, float width, float height, float scale, ImVec2* out_min, ImVec2* out_max)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(label, ImVec2(width, height));
    bool pressed = ImGui::IsItemClicked();
    bool hovered = ImGui::IsItemHovered();
    if (out_min)
        *out_min = ImGui::GetItemRectMin();
    if (out_max)
        *out_max = ImGui::GetItemRectMax();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImU32 bg = ImGui::ColorConvertFloat4ToU32(g_nav_bg);
    if (active)
        bg = ImGui::ColorConvertFloat4ToU32(g_nav_active);
    else if (hovered)
        bg = ImGui::ColorConvertFloat4ToU32(g_nav_hover);
    draw->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), bg, 6.0f * scale);

    ImVec2 text_pos(pos.x + 14.0f * scale, pos.y + (height - ImGui::GetFontSize()) * 0.5f);
    draw->AddText(text_pos, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), label);
    return pressed;
}

static bool PillButton(const char* label, bool active, const ImVec4& tint, const ImVec2& size = ImVec2(0, 0))
{
    ImVec4 base = tint;
    if (!active)
    {
        base.x *= 0.6f;
        base.y *= 0.6f;
        base.z *= 0.6f;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 999.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, base);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(base.x + 0.05f, base.y + 0.05f, base.z + 0.05f, base.w));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, base);
    bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    return pressed;
}

static void RenderWinUI(float scale)
{
    static int active_page = 0;
    static ImGuiID floating_viewport_id = 0;
    static float tab_anim[3] = { 1.0f, 0.0f, 0.0f };
    static float page_alpha[3] = { 1.0f, 0.0f, 0.0f };
    static float indicator_y = 0.0f;
    static float indicator_h = 0.0f;
    static bool custom_args_inited = false;
    static char custom_args_buf[512] = {};
    static bool window_positioned = false;
    enum class LaunchStatusState
    {
        Ready,
        Started,
        ErrorText
    };
    static LaunchStatusState launch_status_state = LaunchStatusState::Ready;
    static std::string launch_status_text;

    const ImVec2 window_size(1300.0f * scale, 800.0f * scale);
    const ImVec2 window_pos(60.0f * scale, 40.0f * scale);
    const float header_height = 44.0f * scale;
    const float nav_width = 220.0f * scale;
    const float content_pad = 18.0f * scale;
    const float content_vpad = 14.0f * scale;
    const float nav_content_gap = 16.0f * scale;
    const float title_btn_w = 36.0f * scale;
    const float title_btn_h = 28.0f * scale;
    const float title_btn_gap = 6.0f * scale;
    const float title_btn_pad = 8.0f * scale;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse;

    ImGui::SetNextWindowSizeConstraints(
        ImVec2(g_min_window_w * scale, g_min_window_h * scale),
        ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);
    
    // 鑾峰彇鏄剧ず鍣ㄥ垎杈ㄧ巼淇℃伅骞惰绠楀眳涓綅缃紝涓€寮€濮嬪啓鍌婚€间簡 蹇樹簡鍔犲垽鏂?
    if (!window_positioned)
    {
        HMONITOR monitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = { sizeof(mi) };
        if (GetMonitorInfo(monitor, &mi))
        {
            int screen_width = mi.rcWork.right - mi.rcWork.left;
            int screen_height = mi.rcWork.bottom - mi.rcWork.top;
            float center_x = (float)(mi.rcWork.left + screen_width / 2);
            float center_y = (float)(mi.rcWork.top + screen_height / 2);
            ImGui::SetNextWindowPos(ImVec2(center_x, center_y), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        }
        window_positioned = true;
    }
    ImGui::PushStyleColor(ImGuiCol_WindowBg, g_app_bg);
    ImGui::Begin("EndField.Launcher", nullptr, window_flags);
    ImGui::PopStyleColor();

    if (g_font_main)
        ImGui::PushFont(g_font_main, 0.0f);

    RenderLanguagePromptModal(scale);
    RenderDisclaimerModal(scale);

    HWND viewport_hwnd = (HWND)ImGui::GetWindowViewport()->PlatformHandle;
    if (viewport_hwnd && viewport_hwnd != g_hwnd)
    {
        g_hwnd = viewport_hwnd;
        SetDwmDarkMode(g_hwnd, g_dark_mode);
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 win_pos = ImGui::GetWindowPos();
    ImVec2 win_size = ImGui::GetWindowSize();
    draw->AddRectFilled(win_pos, ImVec2(win_pos.x + win_size.x, win_pos.y + header_height),
        ImGui::ColorConvertFloat4ToU32(g_panel_bg), 8.0f * scale, ImDrawFlags_RoundCornersTop);
    draw->AddLine(ImVec2(win_pos.x, win_pos.y + header_height),
        ImVec2(win_pos.x + win_size.x, win_pos.y + header_height),
        ImGui::ColorConvertFloat4ToU32(g_separator), 1.0f);
    float sep_x = win_pos.x + content_pad + nav_width + (nav_content_gap * 0.5f);
    draw->AddLine(ImVec2(sep_x, win_pos.y + header_height + content_vpad),
        ImVec2(sep_x, win_pos.y + win_size.y - content_vpad),
        ImGui::ColorConvertFloat4ToU32(g_separator), 1.0f);

    ImGui::SetCursorScreenPos(ImVec2(win_pos.x + 16.0f * scale, win_pos.y + 10.0f * scale));
    if (g_font_title)
        ImGui::PushFont(g_font_title, 0.0f);
    ImGui::TextUnformatted("EndField.Launcher");
    if (g_font_title)
        ImGui::PopFont();

    g_titlebar_hwnd = viewport_hwnd;
    g_titlebar.bar = { (LONG)win_pos.x, (LONG)win_pos.y, (LONG)(win_pos.x + win_size.x), (LONG)(win_pos.y + header_height) };
    float btn_top = win_pos.y + (header_height - title_btn_h) * 0.5f;
    float btn_right = win_pos.x + win_size.x - title_btn_pad;
    float close_left = btn_right - title_btn_w;
    float min_left = close_left - title_btn_gap - title_btn_w;
    g_titlebar.btn_min = { (LONG)min_left, (LONG)btn_top, (LONG)(min_left + title_btn_w), (LONG)(btn_top + title_btn_h) };
    g_titlebar.btn_close = { (LONG)close_left, (LONG)btn_top, (LONG)(close_left + title_btn_w), (LONG)(btn_top + title_btn_h) };

    ImU32 btn_bg = ImGui::ColorConvertFloat4ToU32(g_panel_bg);
    ImU32 btn_hover = ImGui::ColorConvertFloat4ToU32(g_nav_hover);
    ImU32 btn_close_hover = ImGui::ColorConvertFloat4ToU32(ColorFromBytes(210, 70, 70));
    ImU32 btn_text = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]);

    ImGui::SetCursorScreenPos(ImVec2((float)g_titlebar.btn_min.left, (float)g_titlebar.btn_min.top));
    ImGui::PushID("title_min");
    ImGui::InvisibleButton("##min", ImVec2(title_btn_w, title_btn_h));
    bool min_hover = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked() && viewport_hwnd)
        ::ShowWindow(viewport_hwnd, SW_MINIMIZE);
    draw->AddRectFilled(ImVec2((float)g_titlebar.btn_min.left, (float)g_titlebar.btn_min.top),
        ImVec2((float)g_titlebar.btn_min.right, (float)g_titlebar.btn_min.bottom), min_hover ? btn_hover : btn_bg, 6.0f * scale);
    float min_y = (float)g_titlebar.btn_min.bottom - 8.0f * scale;
    draw->AddLine(ImVec2((float)g_titlebar.btn_min.left + 10.0f * scale, min_y),
        ImVec2((float)g_titlebar.btn_min.right - 10.0f * scale, min_y), btn_text, 1.6f * scale);
    ImGui::PopID();

    ImGui::SetCursorScreenPos(ImVec2((float)g_titlebar.btn_close.left, (float)g_titlebar.btn_close.top));
    ImGui::PushID("title_close");
    ImGui::InvisibleButton("##close", ImVec2(title_btn_w, title_btn_h));
    bool close_hover = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked())
        ::PostQuitMessage(0);
    draw->AddRectFilled(ImVec2((float)g_titlebar.btn_close.left, (float)g_titlebar.btn_close.top),
        ImVec2((float)g_titlebar.btn_close.right, (float)g_titlebar.btn_close.bottom), close_hover ? btn_close_hover : btn_bg, 6.0f * scale);
    draw->AddLine(ImVec2((float)g_titlebar.btn_close.left + 12.0f * scale, (float)g_titlebar.btn_close.top + 8.0f * scale),
        ImVec2((float)g_titlebar.btn_close.right - 12.0f * scale, (float)g_titlebar.btn_close.bottom - 8.0f * scale), btn_text, 1.5f * scale);
    draw->AddLine(ImVec2((float)g_titlebar.btn_close.right - 12.0f * scale, (float)g_titlebar.btn_close.top + 8.0f * scale),
        ImVec2((float)g_titlebar.btn_close.left + 12.0f * scale, (float)g_titlebar.btn_close.bottom - 8.0f * scale), btn_text, 1.5f * scale);
    ImGui::PopID();

    ImGui::SetCursorScreenPos(ImVec2(win_pos.x, win_pos.y + header_height));
    ImGui::BeginChild("##body", ImVec2(win_size.x, win_size.y - header_height), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPos(ImVec2(content_pad, content_vpad));
    ImVec2 body_avail = ImGui::GetContentRegionAvail();
    ImVec2 nav_size(nav_width, body_avail.y - content_vpad);
    ImVec2 content_size(body_avail.x - nav_width - nav_content_gap - content_pad, body_avail.y - content_vpad);

    ImGui::BeginChild("##nav", nav_size, false, ImGuiWindowFlags_NoScrollbar);
    ImDrawList* nav_draw = ImGui::GetWindowDrawList();
    ImVec2 nav_pos = ImGui::GetWindowPos();
    ImVec2 nav_window_size = ImGui::GetWindowSize();
    nav_draw->AddRectFilled(nav_pos, ImVec2(nav_pos.x + nav_window_size.x, nav_pos.y + nav_window_size.y),
        ImGui::ColorConvertFloat4ToU32(g_nav_bg), 0.0f);
    ImGui::Dummy(ImVec2(0.0f, 8.0f * scale));
    ImGuiIO& io = ImGui::GetIO();
    float anim_speed = 16.0f * io.DeltaTime;
    for (int i = 0; i < 3; ++i)
    {
        float target = (active_page == i) ? 1.0f : 0.0f;
        tab_anim[i] = LerpFloat(tab_anim[i], target, anim_speed);
        page_alpha[i] = LerpFloat(page_alpha[i], target, anim_speed * 1.4f);
    }

    ImVec2 tab_min[3] = {};
    ImVec2 tab_max[3] = {};

    if (NavigationItem(T(TextKey::NavLaunch), active_page == 0, nav_width - 16.0f * scale, 40.0f * scale, scale, &tab_min[0], &tab_max[0]))
        active_page = 0;
    ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));
    if (NavigationItem(T(TextKey::NavWindow), active_page == 1, nav_width - 16.0f * scale, 40.0f * scale, scale, &tab_min[1], &tab_max[1]))
        active_page = 1;
    ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));
    if (NavigationItem(T(TextKey::NavAbout), active_page == 2, nav_width - 16.0f * scale, 40.0f * scale, scale, &tab_min[2], &tab_max[2]))
        active_page = 2;

    if (tab_max[active_page].y > tab_min[active_page].y)
    {
        float target_y = tab_min[active_page].y;
        float target_h = tab_max[active_page].y - tab_min[active_page].y;
        indicator_y = LerpFloat(indicator_y, target_y, anim_speed);
        indicator_h = LerpFloat(indicator_h, target_h, anim_speed);
    }
    ImVec2 indicator_pos(tab_min[active_page].x, indicator_y);
    ImVec2 indicator_end(tab_min[active_page].x + 4.0f * scale, indicator_y + indicator_h);
    ImVec4 accent = g_accent;
    accent.w = 0.9f;
    nav_draw->AddRectFilled(indicator_pos, indicator_end, ImGui::ColorConvertFloat4ToU32(accent), 6.0f * scale);
    ImGui::EndChild();

    ImGui::SameLine(0.0f, nav_content_gap);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(content_pad, content_vpad));
    ImGui::BeginChild("##content", content_size, false, ImGuiWindowFlags_NoScrollbar);
    ImDrawList* content_draw = ImGui::GetWindowDrawList();
    ImVec2 content_pos = ImGui::GetWindowPos();
    ImVec2 content_window_size = ImGui::GetWindowSize();
    content_draw->AddRectFilled(content_pos, ImVec2(content_pos.x + content_window_size.x, content_pos.y + content_window_size.y),
        ImGui::ColorConvertFloat4ToU32(g_panel_bg), 0.0f);

    ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));
    if (g_font_title)
        ImGui::PushFont(g_font_title, 0.0f);
    const char* page_title = active_page == 0 ? T(TextKey::PageTitleLaunch) : (active_page == 1 ? T(TextKey::PageTitleWindow) : T(TextKey::PageTitleAbout));
    const char* page_subtitle = active_page == 0 ? T(TextKey::PageSubtitleLaunch) : (active_page == 1 ? T(TextKey::PageSubtitleWindow) : T(TextKey::PageSubtitleAbout));
    ImGui::TextColored(g_accent, "%s", page_title);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", page_subtitle);
    if (g_font_title)
        ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 8.0f * scale));

    float ease = SmoothStep(page_alpha[active_page]);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, LerpFloat(0.4f, 1.0f, ease));
    if (active_page == 0)
    {
        if (!custom_args_inited)
        {
            strncpy_s(custom_args_buf, sizeof(custom_args_buf), g_config.custom_args.c_str(), _TRUNCATE);
            custom_args_inited = true;
        }

        std::string game_path_utf8 = WideToUtf8(g_config.game_path);
        bool game_valid = !g_config.game_path.empty() && FileExistsW(g_config.game_path);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, g_card_bg);
        ImGui::BeginChild("##game_path", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
        ImGui::TextUnformatted(T(TextKey::GamePath));
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(game_path_utf8.empty() ? T(TextKey::NotSet) : game_path_utf8.c_str());
        ImGui::PopTextWrapPos();
        if (!g_is_admin)
        {
            ImGui::TextColored(ImVec4(0.98f, 0.45f, 0.45f, 1.0f),
                T(TextKey::AdminRequiredForFeatures));
        }
        if (!game_valid)
            ImGui::TextColored(ImVec4(0.98f, 0.55f, 0.55f, 1.0f), T(TextKey::GameNotFoundHint));
        if (ImGui::Button(T(TextKey::SelectGameLocation)))
        {
            std::wstring selected = SelectGameExecutable(viewport_hwnd);
            if (!selected.empty())
            {
                g_config.game_path = selected;
                SaveConfig(g_config);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(T(TextKey::AutoDetectGamePath)))
        {
            std::wstring detected;
            DWORD pid = 0;
            if (FindRunningGamePath(&detected, &pid))
            {
                g_config.game_path = detected;
                SaveConfig(g_config);
                if (pid != 0)
                {
                    HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                    if (process)
                    {
                        TerminateProcess(process, 0);
                        CloseHandle(process);
                    }
                }
                MessageBoxW(viewport_hwnd, TW(TextKey::AutoDetectSuccess).c_str(), TW(TextKey::HintTitle).c_str(), MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                MessageBoxW(viewport_hwnd, TW(TextKey::AutoDetectFail).c_str(), TW(TextKey::HintTitle).c_str(), MB_OK | MB_ICONWARNING);
            }
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0.0f, 10.0f * scale));
        ImGui::BeginChild("##inject", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
        ImGui::TextUnformatted(T(TextKey::SectionInject));
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.98f, 0.45f, 0.45f, 1.0f), T(TextKey::InjectWarning));
        if (!g_is_admin)
            ImGui::TextColored(ImVec4(0.98f, 0.45f, 0.45f, 1.0f), T(TextKey::InjectDisabledNoAdmin));

        ImGui::BeginDisabled(!g_is_admin);
        if (ImGui::Checkbox(T(TextKey::EnableInjection), &g_config.inject_enabled))
            SaveConfig(g_config);
        ImGui::BeginDisabled(!g_config.inject_enabled);
        if (ImGui::Checkbox(T(TextKey::UnlockFps), &g_config.inject_unlock_fps))
            SaveConfig(g_config);
        ImGui::BeginDisabled(!g_config.inject_unlock_fps);
        ImGui::InputInt(T(TextKey::TargetFps), &g_config.inject_target_fps);
        ImGui::TextUnformatted(T(TextKey::QuickFps));
        const int quick_fps[] = { 30, 45, 60, 120, 144, 165, 200, 240, (std::numeric_limits<int32_t>::max)() };
        for (int i = 0; i < (int)(sizeof(quick_fps) / sizeof(quick_fps[0])); ++i)
        {
            const int value = quick_fps[i];
            const char* label = (value == (std::numeric_limits<int32_t>::max)()) ? T(TextKey::QuickFpsUnlimited) : nullptr;
            char buf[32] = {};
            if (!label)
            {
                snprintf(buf, sizeof(buf), "%d", value);
                label = buf;
            }
            if (PillButton(label, g_config.inject_target_fps == value, g_accent))
            {
                g_config.inject_target_fps = value;
                SaveConfig(g_config);
            }
            if (i != (int)(sizeof(quick_fps) / sizeof(quick_fps[0])) - 1)
                ImGui::SameLine();
        }
        const int32_t kMin = (std::numeric_limits<int32_t>::min)();
        const int32_t kMax = (std::numeric_limits<int32_t>::max)();
        if (g_config.inject_target_fps < kMin)
            g_config.inject_target_fps = kMin;
        if (g_config.inject_target_fps > kMax)
            g_config.inject_target_fps = kMax;
        if (ImGui::IsItemDeactivatedAfterEdit())
            SaveConfig(g_config);
        ImGui::EndDisabled();
        ImGui::EndDisabled();
        ImGui::EndDisabled();
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0.0f, 10.0f * scale));
        ImGui::BeginChild("##launch_args", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
        ImGui::TextUnformatted(T(TextKey::LaunchArgs));
        ImGui::Separator();
        if (ImGui::Checkbox(T(TextKey::UseLaunchArgs), &g_config.use_launch_args))
            SaveConfig(g_config);

        ImGui::BeginDisabled(!g_config.use_launch_args);
        if (ImGui::Checkbox(T(TextKey::UsePopupWindow), &g_config.use_popupwindow))
            SaveConfig(g_config);
        if (ImGui::Checkbox(T(TextKey::CustomLaunchArgs), &g_config.use_custom_args))
            SaveConfig(g_config);
        if (g_config.use_custom_args)
        {
            ImGui::InputText(T(TextKey::ArgsLabel), custom_args_buf, sizeof(custom_args_buf));
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                g_config.custom_args = custom_args_buf;
                SaveConfig(g_config);
            }
        }
        ImGui::EndDisabled();
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0.0f, 10.0f * scale));
        bool can_launch = game_valid && g_is_admin;
        if (!can_launch)
            ImGui::BeginDisabled();
        if (ImGui::Button(T(TextKey::LaunchGame), ImVec2(140.0f * scale, 0.0f)))
        {
            std::wstring error;
            if (LaunchGameWithOptionalInjection(g_config, &error))
            {
                launch_status_state = LaunchStatusState::Started;
                launch_status_text.clear();
            }
            else
            {
                launch_status_state = LaunchStatusState::ErrorText;
                launch_status_text = WideToUtf8(error);
            }
        }
        if (!can_launch)
            ImGui::EndDisabled();
        ImGui::SameLine();
        const char* status_text = nullptr;
        if (launch_status_state == LaunchStatusState::Ready)
            status_text = T(TextKey::LaunchStatusReady);
        else if (launch_status_state == LaunchStatusState::Started)
            status_text = T(TextKey::LaunchStatusStarted);
        else
            status_text = launch_status_text.c_str();
        ImGui::Text(T(TextKey::StatusLabel), status_text);
        ImGui::PopStyleColor();
    }
    else if (active_page == 1)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, g_card_bg);
        ImGui::BeginChild("##appearance", ImVec2(0.0f, 220.0f * scale), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::TextUnformatted(T(TextKey::SectionAppearance));
        ImGui::Separator();
        const char* theme_items = g_theme_items.c_str();
        if (ImGui::Combo(T(TextKey::ThemeMode), &g_theme_mode, theme_items))
        {
            g_need_theme_refresh = true;
            g_config.theme_mode = g_theme_mode;
            SaveConfig(g_config);
        }
        ImGui::TextDisabled(T(TextKey::ThemeHint));
        const char* language_items = g_language_items.c_str();
        if (ImGui::Combo(T(TextKey::LanguageLabel), &g_language, language_items))
        {
            g_language = ClampLanguageIndex(g_language);
            g_config.language = g_language;
            g_config.language_set = true;
            SaveConfig(g_config);
            RefreshLanguageItems();
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, g_card_bg);
        ImGui::BeginChild("##about", ImVec2(0.0f, 220.0f * scale), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::TextUnformatted(T(TextKey::SectionAbout));
        ImGui::Separator();
        ImGui::TextUnformatted(T(TextKey::ProjectSource));
        ImGui::SameLine();
        RenderLinkText("GitHub", "https://github.com/isxlan0/EndField.Fps.UnlockerIsland", ImVec4(0.55f, 0.75f, 1.0f, 1.0f), ImVec4(0.65f, 0.85f, 1.0f, 1.0f));
        ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));
        ImGui::TextColored(ImVec4(0.98f, 0.45f, 0.45f, 1.0f), T(TextKey::DisclaimerLine1));
        ImGui::TextColored(ImVec4(0.98f, 0.45f, 0.45f, 1.0f), T(TextKey::DisclaimerLine2));
        ImGui::TextColored(ImVec4(0.98f, 0.45f, 0.45f, 1.0f), T(TextKey::DisclaimerLine3));
        ImGui::Dummy(ImVec2(0.0f, 6.0f * scale));
        ImGui::Text(T(TextKey::BuildTime), __DATE__, __TIME__);
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::EndChild();

    UpdateSharedMemoryFromConfig();

    if (g_font_main)
        ImGui::PopFont();

    ImGui::End();
}

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int, char**)
{
#ifdef NDEBUG
    HWND console = GetConsoleWindow();
    if (console)
        ShowWindow(console, SW_HIDE);
    FreeConsole();
#endif
    
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    LoadConfig(&g_config);
    g_theme_mode = g_config.theme_mode;
    g_language = ClampLanguageIndex(g_config.language);
    g_config.language = g_language;
    
    RefreshLanguageItems();
    
    if (!g_config.language_set)
    {
        g_language_prompt_choice = g_language;
        g_show_language_prompt = true;
    }
    else if (!g_config.disclaimer_accepted)
    {
        g_show_disclaimer_prompt = true;
    }
    
    g_is_admin = IsRunningAsAdmin();
    InitSharedMemory();

    
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd_hidden = ::CreateWindowExW(0, wc.lpszClassName, L"ImGui Host",
        WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, wc.hInstance, nullptr);
    g_host_hwnd = hwnd_hidden;
    ::ShowWindow(hwnd_hidden, SW_HIDE);

    HWND hwnd = hwnd_hidden;
    g_hwnd = nullptr;
    
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    
    g_dark_mode = ReadSystemDarkMode();
    if (g_theme_mode == 1)
        g_dark_mode = true;
    else if (g_theme_mode == 2)
        g_dark_mode = false;
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigViewportsNoDecoration = true;
    
    ImGuiStyle& style = ImGui::GetStyle();
    const float ui_scale = 1.0f;
    style.ScaleAllSizes(ui_scale);
    style.FontScaleDpi = ui_scale;
    io.ConfigDpiScaleFonts = false;
    io.ConfigDpiScaleViewports = false;
    ApplyTheme(g_dark_mode, ui_scale);

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    
    ImFontConfig Font_cfg;
    Font_cfg.OversampleH = 2;
    Font_cfg.OversampleV = 2;
    Font_cfg.PixelSnapH = true;
    Font_cfg.FontDataOwnedByAtlas = true;

    ImFont* Font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 18.0f, &Font_cfg, io.Fonts->GetGlyphRangesChineseFull());
    ImFont* Font_Big = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 22.0f, &Font_cfg, io.Fonts->GetGlyphRangesChineseFull());
    if (!Font || !Font_Big)
    {
        Font = io.Fonts->AddFontDefault();
        Font_Big = Font;
    }
    g_font_main = Font;
    g_font_title = Font_Big;
    
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (g_theme_mode == 0)
        {
            ULONGLONG now = GetTickCount64();
            if (g_need_theme_refresh || (now - g_last_theme_check) > 1000)
            {
                bool system_dark = ReadSystemDarkMode();
                if (system_dark != g_dark_mode)
                {
                    g_dark_mode = system_dark;
                    ApplyTheme(g_dark_mode, ui_scale);
                }
                g_need_theme_refresh = false;
                g_last_theme_check = now;
            }
        }
        else
        {
            bool desired_dark = (g_theme_mode == 1);
            if (g_need_theme_refresh || desired_dark != g_dark_mode)
            {
                g_dark_mode = desired_dark;
                ApplyTheme(g_dark_mode, ui_scale);
                g_need_theme_refresh = false;
            }
        }

        RenderWinUI(ui_scale);

        ImGui::Render();
        const float clear_color_with_alpha[4] = { g_clear_color.x * g_clear_color.w, g_clear_color.y * g_clear_color.w, g_clear_color.z * g_clear_color.w, g_clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            UpdateViewportEffects(g_dark_mode);
            ImGui::RenderPlatformWindowsDefault();
        }

        HRESULT hr = g_pSwapChain->Present(g_in_sizemove ? 0 : 1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);

        if (!g_in_sizemove)
            LimitFrameRate(DetermineTargetFps());
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    ShutdownSharedMemory();

    return 0;
}


bool CreateDeviceD3D(HWND hWnd)
{
    
    
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 0;
    sd.Flags = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    UINT createDeviceFlags = 0;
    
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) 
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    IDXGIFactory* pSwapChainFactory;
    if (SUCCEEDED(g_pSwapChain->GetParent(IID_PPV_ARGS(&pSwapChainFactory))))
    {
        pSwapChainFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
        pSwapChainFactory->Release();
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_NCHITTEST:
    {
        if (hWnd != g_titlebar_hwnd)
            break;
        LRESULT hit = ::DefWindowProcW(hWnd, msg, wParam, lParam);
        if (hit != HTCLIENT)
            return hit;
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (PtInRect(&g_titlebar.btn_min, pt) || PtInRect(&g_titlebar.btn_close, pt))
            return HTCLIENT;
        if (PtInRect(&g_titlebar.bar, pt))
            return HTCAPTION;
        return hit;
    }
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); 
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_ENTERSIZEMOVE:
        g_in_sizemove = true;
        return 0;
    case WM_SIZING:
    {
        RECT* rect = reinterpret_cast<RECT*>(lParam);
        if (rect)
        {
            g_in_sizemove = true;
            RECT client_rect;
            if (::GetClientRect(hWnd, &client_rect))
            {
                UINT w = (UINT)(client_rect.right - client_rect.left);
                UINT h = (UINT)(client_rect.bottom - client_rect.top);
                if (w > 0 && h > 0 && g_pSwapChain)
                {
                    CleanupRenderTarget();
                    g_pSwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
                    CreateRenderTarget();
                    g_ResizeWidth = g_ResizeHeight = 0;
                }
            }
        }
        return 0;
    }
    case WM_EXITSIZEMOVE:
    {
        g_in_sizemove = false;
        RECT rect;
        if (::GetClientRect(hWnd, &rect))
        {
            g_ResizeWidth = (UINT)(rect.right - rect.left);
            g_ResizeHeight = (UINT)(rect.bottom - rect.top);
        }
        return 0;
    }
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) 
            return 0;
        break;
    case WM_SETTINGCHANGE:
        g_need_theme_refresh = true;
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

