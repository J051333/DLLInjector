#pragma once
#include <Windows.h>
#include <TlHelp32.h>

#ifdef INJDLL_EXPORTS
#define INJDLL_API __declspec(dllexport)
#else
#define INJDLL_API __declspec(dllimport)
#endif

extern "C" {
	// Uses standard in and out to prepare injection
	INJDLL_API void consoleInject();
	// Injects the DLL at dllPath into processId
	INJDLL_API BOOL InjectDLL(DWORD processId, const char* dllPath);
	// Gets the Id of the first process with a matching name
	INJDLL_API DWORD GetProcessIdByName(const wchar_t* processName);
}