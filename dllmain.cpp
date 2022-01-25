// Defines the entry point for the DLL application.

#include "pch.h"

#include "dllImplementation.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) double SolveForRoot(const char* expr, size_t exprLen, double initialGuess, int maxSize, double goalErr, double* results)
{
    return dllImplementation::SolveForRoot(expr, exprLen, initialGuess, maxSize, goalErr, results);
}
