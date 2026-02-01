#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <Psapi.h>
#include <chrono>
#include <atomic>
#include <cstdio>

#pragma comment(lib, "psapi.lib")

namespace PatternScanner {

#ifdef _DEBUG
#define PATTERN_SCAN_LOG_RESULT(...) PatternScanner::LogScanResult(__VA_ARGS__)
#define PATTERN_SCAN_LOG_BEGIN(idVar) \
    const uint32_t idVar = PatternScanner::NextScanIndex(); \
    const auto _ps_t0_##idVar = std::chrono::high_resolution_clock::now();
#define PATTERN_SCAN_LOG_END(idVar, pattern, moduleName, found, address) \
    do { \
        const auto _ps_t1_##idVar = std::chrono::high_resolution_clock::now(); \
        const double _ps_ms_##idVar = std::chrono::duration<double, std::milli>(_ps_t1_##idVar - _ps_t0_##idVar).count(); \
        PATTERN_SCAN_LOG_RESULT(idVar, pattern, moduleName, found, _ps_ms_##idVar, address); \
    } while (0)
#else
#define PATTERN_SCAN_LOG_RESULT(...) do { } while (0)
#define PATTERN_SCAN_LOG_BEGIN(idVar) do { } while (0)
#define PATTERN_SCAN_LOG_END(idVar, pattern, moduleName, found, address) do { } while (0)
#endif

    struct RegionInfo {
        uintptr_t base;
        size_t size;
    };

    inline std::atomic<uint32_t> g_scanIndex{ 0 };

    inline uint32_t NextScanIndex() {
        return g_scanIndex.fetch_add(1, std::memory_order_relaxed);
    }

    inline void LogScanResult(uint32_t id, const std::string& pattern, const wchar_t* moduleName, bool found, double ms, uintptr_t address) {
        if (moduleName && moduleName[0] != L'\0') {
            if (found) {
                std::printf("[PatternScanner] id=%u module=%ls pattern=\"%s\" time=%.3fms addr=%p\n",
                    id, moduleName, pattern.c_str(), ms, reinterpret_cast<void*>(address));
            }
            else {
                std::printf("[PatternScanner] id=%u module=%ls pattern=\"%s\" time=%.3fms addr=<not found>\n",
                    id, moduleName, pattern.c_str(), ms);
            }
        }
        else {
            if (found) {
                std::printf("[PatternScanner] id=%u module=<all> pattern=\"%s\" time=%.3fms addr=%p\n",
                    id, pattern.c_str(), ms, reinterpret_cast<void*>(address));
            }
            else {
                std::printf("[PatternScanner] id=%u module=<all> pattern=\"%s\" time=%.3fms addr=<not found>\n",
                    id, pattern.c_str(), ms);
            }
        }
    }

    inline bool IsReadableOrExecutable(DWORD protect) {
        return
            protect == PAGE_EXECUTE_READ ||
            protect == PAGE_EXECUTE_READWRITE ||
            protect == PAGE_EXECUTE_WRITECOPY ||
            protect == PAGE_EXECUTE ||
            protect == PAGE_READONLY ||
            protect == PAGE_READWRITE ||
            protect == PAGE_WRITECOPY;
    }

    inline void ParsePattern(const std::string& pattern, std::vector<std::pair<uint8_t, bool>>& parsed) {
        std::istringstream iss(pattern);
        std::string byteStr;

        while (iss >> byteStr) {
            if (byteStr == "?" || byteStr == "??") {
                parsed.emplace_back(0x00, true);
            }
            else {
                parsed.emplace_back(static_cast<uint8_t>(std::strtoul(byteStr.c_str(), nullptr, 16)), false);
            }
        }
    }

    inline bool SafeCompare(uint8_t* base, size_t size,
        const std::vector<std::pair<uint8_t, bool>>* pattern,
        uintptr_t* result) {
        __try {
            const size_t patternSize = pattern->size();
            if (patternSize == 0 || size < patternSize) return false;

            for (size_t i = 0; i <= size - patternSize; ++i) {
                bool found = true;
                for (size_t j = 0; j < patternSize; ++j) {
                    if (!(*pattern)[j].second && base[i + j] != (*pattern)[j].first) {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    *result = reinterpret_cast<uintptr_t>(&base[i]);
                    return true;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        return false;
    }

    inline std::vector<RegionInfo> GetMemoryRegionsInRange(uintptr_t start, uintptr_t end) {
        std::vector<RegionInfo> regions;
        if (start >= end) return regions;

        MEMORY_BASIC_INFORMATION mbi{};
        uintptr_t cur = start;

        while (cur < end) {
            if (VirtualQuery(reinterpret_cast<void*>(cur), &mbi, sizeof(mbi)) == 0)
                break;

            const uintptr_t regionBase = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            const uintptr_t regionEnd = regionBase + mbi.RegionSize;

            const uintptr_t clippedBase = (regionBase < start) ? start : regionBase;
            const uintptr_t clippedEnd = (regionEnd > end) ? end : regionEnd;

            if (clippedBase < clippedEnd) {
                if (mbi.State == MEM_COMMIT && IsReadableOrExecutable(mbi.Protect)) {
                    regions.push_back({ clippedBase, static_cast<size_t>(clippedEnd - clippedBase) });
                }
            }

            cur = regionEnd;
            if (cur <= regionBase) break;
        }

        return regions;
    }

    inline bool GetModuleRange(const wchar_t* moduleName, uintptr_t& base, size_t& size) {
        HMODULE hMod = nullptr;

        if (moduleName == nullptr || moduleName[0] == L'\0') {
            hMod = GetModuleHandleW(nullptr);
        }
        else {
            hMod = GetModuleHandleW(moduleName);
        }

        if (!hMod) return false;

        MODULEINFO mi{};
        if (!GetModuleInformation(GetCurrentProcess(), hMod, &mi, sizeof(mi)))
            return false;

        base = reinterpret_cast<uintptr_t>(mi.lpBaseOfDll);
        size = static_cast<size_t>(mi.SizeOfImage);
        return true;
    }

    inline uintptr_t Scan(const std::string& pattern) {
        PATTERN_SCAN_LOG_BEGIN(scanId);
        std::vector<std::pair<uint8_t, bool>> parsedPattern;
        ParsePattern(pattern, parsedPattern);

        SYSTEM_INFO sysInfo{};
        GetSystemInfo(&sysInfo);
        const uintptr_t start = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
        const uintptr_t end = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);

        auto regions = GetMemoryRegionsInRange(start, end);
        for (const auto& region : regions) {
            uintptr_t found = 0;
            if (SafeCompare(reinterpret_cast<uint8_t*>(region.base), region.size, &parsedPattern, &found)) {
                PATTERN_SCAN_LOG_END(scanId, pattern, nullptr, true, found);
                return found;
            }
        }
        PATTERN_SCAN_LOG_END(scanId, pattern, nullptr, false, 0);
        return 0;
    }

    inline uintptr_t ScanModule(const std::string& pattern, const wchar_t* moduleName) {
        PATTERN_SCAN_LOG_BEGIN(scanId);
        uintptr_t modBase = 0;
        size_t modSize = 0;

        if (!GetModuleRange(moduleName, modBase, modSize)) {
            PATTERN_SCAN_LOG_END(scanId, pattern, moduleName, false, 0);
            return 0;
        }

        std::vector<std::pair<uint8_t, bool>> parsedPattern;
        ParsePattern(pattern, parsedPattern);

        const uintptr_t start = modBase;
        const uintptr_t end = modBase + modSize;

        auto regions = GetMemoryRegionsInRange(start, end);
        for (const auto& region : regions) {
            uintptr_t found = 0;
            if (SafeCompare(reinterpret_cast<uint8_t*>(region.base), region.size, &parsedPattern, &found)) {
                PATTERN_SCAN_LOG_END(scanId, pattern, moduleName, true, found);
                return found;
            }
        }
        PATTERN_SCAN_LOG_END(scanId, pattern, moduleName, false, 0);
        return 0;
    }


    // 解析跳转类指令，如 call/jmp（E8/E9）
    // instructionAddr: 指令地址（E8/E9开头）
    // offset: 跳转偏移位置（E8后面是 +1）
    // instructionSize: 整个指令大小（E8 xx xx xx xx 是 5 字节）
    inline uintptr_t ResolveRelativeAddress(uintptr_t instructionAddr, size_t offset = 1, size_t instructionSize = 5) {
        int32_t rel = *reinterpret_cast<int32_t*>(instructionAddr + offset);
        return instructionAddr + instructionSize + rel;
    }

} // namespace PatternScanner
