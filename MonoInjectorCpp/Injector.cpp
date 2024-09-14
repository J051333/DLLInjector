#include "Injector.h"
#include <iostream>
#include <fstream>
#include <string>

// Get Process ID by name
DWORD GetProcessIdByName(const wchar_t* processName) {
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
	DWORD processId = 0;

	// Take a snapshot of all running processes
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		std::cerr << "Error: Unable to create the toolhelp snapshot!!!" << std::endl;
		return 0;
	}

	// Set the size of the structure before using it
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve the first process
	if (!Process32First(hProcessSnap, &pe32)) {
		std::cerr << "Error: Unable to retrieve process." << std::endl;
		CloseHandle(hProcessSnap);
		return 0;
	}

	// Go through the processes and look for the right one
	do {
		if (_wcsicmp(pe32.szExeFile, processName) == 0) {
			processId = pe32.th32ProcessID;
			break;
		}

		std::wcout << L"" << pe32.szExeFile << std::endl;
	} while (Process32Next(hProcessSnap, &pe32)); // Continue to next process

	CloseHandle(hProcessSnap);
	return processId;
}

BOOL InjectDLL(DWORD processId, const char* dllPath) {
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	if (!hProcess) {
		std::cerr << "Failed to open target process." << std::endl;
		return false;
	}

	std::ifstream dll(dllPath);
	if (!dll.good()) {
		std::cerr << "Failed to get dll file!" << std::endl;
		return false;
	}
	
	void* dllPathRemote = VirtualAllocEx(hProcess, nullptr, strlen(dllPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!dllPathRemote) {
		std::cerr << "Failed to allocate memory in target process :(" << std::endl;
		CloseHandle(hProcess);
		return false;
	}

	// Write the dll path to the memory in the target process
	WriteProcessMemory(hProcess, dllPathRemote, dllPath, strlen(dllPath) + 1, nullptr);

	// Get the address of LoadLibraryA
	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
	FARPROC loadLibraryAddr = nullptr;
	if (hKernel32) {
		loadLibraryAddr = GetProcAddress(hKernel32, "LoadLibraryA");
	}
	else {
		std::cerr << "Error getting the kernel32 module" << std::endl;
		return false;
	}

	// Create a remote thread to call LoadLibraryA with the dll path
	HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, dllPathRemote, 0, nullptr);
	if (!hThread) {
		std::cerr << "Error, unable to create a remote thread." << std::endl;
		VirtualFreeEx(hProcess, dllPathRemote, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}

	// Wait for the thread to finish
	WaitForSingleObject(hThread, INFINITE);

	// Clean up
	VirtualFreeEx(hProcess, dllPathRemote, 0, MEM_RELEASE);
	CloseHandle(hThread);
	CloseHandle(hProcess);
	std::cout << "Success!" << std::endl;
	return true;
}

void consoleInject() {
	std::string processName;
	std::string dllPath;

	std::cout << "Please enter the process to attach to: " << std::flush;
	std::cin >> processName;

	std::cout << "Please enter the path to the dll: " << std::flush;
	std::cin >> dllPath;

	int id = GetProcessIdByName(std::wstring(processName.begin(), processName.end()).c_str());

	std::cout << InjectDLL(id, dllPath.c_str()) << std::endl;
}