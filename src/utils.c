#include "utils.h"
#include <windows.h>

double now_ms(void) {
    static LARGE_INTEGER frequency;
    static int initialized = 0;

    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = 1;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    return (double)counter.QuadPart * 1000.0 / (double)frequency.QuadPart;
}