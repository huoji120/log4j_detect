#include "pch.h"
namespace Tools {
auto DbgPrint(const char* format, ...) -> void {
    va_list ArgList;
    PCHAR String = NULL;
    ULONG Length = 0;

    va_start(ArgList, format);

    Length = _vscprintf(format, ArgList) + 1;
    String = (PCHAR)malloc(Length);
    if (String) {
        memset(String, 0x0, Length);
        vsprintf_s(String, Length, format, ArgList);
        OutputDebugStringA(String);
        free(String);
        va_end(ArgList);
    }
}
};  // namespace Tools
