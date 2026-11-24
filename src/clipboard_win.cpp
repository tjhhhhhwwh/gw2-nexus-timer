#include "clipboard_win.h"
#include <windows.h>
#include <cstring>
#include <string>

bool CopyUnicodeText(const std::string& utf8) {
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), -1, nullptr, 0);
    if (count <= 0) return false;
    std::wstring wide(static_cast<std::size_t>(count), L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), -1, wide.data(), count))
        return false;
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, wide.size() * sizeof(wchar_t));
    if (!memory) return false;
    void* buffer = GlobalLock(memory);
    if (!buffer) { GlobalFree(memory); return false; }
    std::memcpy(buffer, wide.data(), wide.size() * sizeof(wchar_t));
    GlobalUnlock(memory);
    HWND owner = GetActiveWindow();
    if (!owner) owner = GetForegroundWindow();
    // A null owner after EmptyClipboard prevents SetClipboardData from succeeding.
    if (!owner || !OpenClipboard(owner)) { GlobalFree(memory); return false; }
    const bool ok = EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, memory) != nullptr;
    CloseClipboard();
    if (!ok) GlobalFree(memory); // On success, Windows owns the allocation.
    return ok;
}
