#include <iostream>
#include <windows.h>

//#define LEVEL_OUTPUT

LSTATUS getRegEntryWide(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValue, std::wstring& result);
static std::wstring getNextLevelInfo(std::wstring levArg);

typedef enum 
{
	EXIT_END = 1,
	EXIT_LAUNCHER = -0x3F4,
	EXIT_INSTANCE = -0x3F5,
	EXIT_RESTART = 0x3E9,
}ExitCodeEnum;

int main()
{
	HKEY hKey = HKEY_CURRENT_USER;
	static const std::wstring desktopHwnd{ L"3735928559" };
	const LPCWSTR subKey{ L"SOFTWARE\\Empire Interactive\\Starsky&Hutch\\Install\\" };
	const LPCWSTR installPathKey{ L"InstallPath" };
	const LPCWSTR cmdLineKey{ L"CommandLine" };
	std::wstring currentDir{};

	// Not really needed, it seems that game doesn't actually use it
	//HWND desktopHwnd = GetDesktopWindow();
	std::cout << "Starsky & Hutch Launcher v1.3b" << std::endl;

	if (getRegEntryWide(hKey, subKey, installPathKey, currentDir) != ERROR_SUCCESS)
	{
		hKey = HKEY_LOCAL_MACHINE;
		if (getRegEntryWide(hKey, subKey, installPathKey, currentDir) != ERROR_SUCCESS)
		{
			std::wcout << "Failed to read key: " << subKey << installPathKey << std::endl;
			return 1;
		}
	}

	std::wstring exePath = currentDir + L"\\StarskyPC.exe";
	std::wcout << "Found executable: " << exePath.c_str() << std::endl;

	//Check if an instance is already running, exit if so
	const LPCWSTR mutexName{ L"minds-eye.starsky.and.hutch.mutex" };
	auto mutexHandle = OpenMutex(MUTEX_ALL_ACCESS, FALSE, mutexName);
	if (mutexHandle != NULL)
	{
		std::cout << "Another game instance detected! Exiting...\n";
		CloseHandle(mutexHandle);
		return 2;
	}

	// Start launcher work
	std::wstring cmdArg{ L"/HWND=" + desktopHwnd };
	cmdArg.append(L" /SpawnerFirstTime");
	PROCESS_INFORMATION processInfo;
	STARTUPINFO StartUpInfo{ sizeof(STARTUPINFO) };
	DWORD exitCode;
	bool appCode = true;

	while (appCode)
	{
		std::wcout << L"Starting exe with args:" << cmdArg.c_str() << std::endl;
		if (CreateProcess(
			exePath.c_str(),
			&cmdArg[0],
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			currentDir.c_str(),
			&StartUpInfo,
			&processInfo) == FALSE)
		{
			std::cerr << "Failed to launch application, exiting..." << std::endl;
			return 1;
		}

		WaitForSingleObject(processInfo.hProcess, INFINITE);
		GetExitCodeProcess(processInfo.hProcess, &exitCode);

		printf("Exit Code: %X\n", exitCode);

		std::wstring levelArg{};

		// Original Launcher logic is slightly different, might have to change this later
		switch (exitCode)
		{
		case EXIT_END:
		case EXIT_LAUNCHER:
			appCode = false;
			break;
		case EXIT_INSTANCE:
			std::cout << "Another game instance detected! Exiting...\n";
			appCode = false;
			break;
		case EXIT_RESTART:
			cmdArg = L"/HWND=" + desktopHwnd + L" /SpawnerRespawn ";

			if (getRegEntryWide(hKey, subKey, cmdLineKey, levelArg) == ERROR_SUCCESS)
			{
				cmdArg.append(levelArg);
#ifdef LEVEL_OUTPUT
				std::wcout <<
					L"Launching: " <<
					getNextLevelInfo(levelArg).c_str() << L"\n";
#endif
			}
			else
			{
				std::cout << "Failed to find CommandLine argument, skipping." << std::endl;
			}
			break;
		default:
			std::cout << "Game likely crashed, attempting to relaunch...";
			break;
		}

		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}

	return 0;
}

LSTATUS getRegEntryWide(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValue, std::wstring& result)
{
	constexpr DWORD BUF_LEN = 128;
	wchar_t value[BUF_LEN];
	DWORD bufLen = sizeof(value);
	auto status = RegGetValue(hKey,
		lpSubKey,
		lpValue,
		RRF_RT_REG_SZ | RRF_SUBKEY_WOW6432KEY,
		NULL,
		static_cast<PVOID>(value),
		&bufLen);

	if (status == ERROR_SUCCESS)
	{
		result = value;
	}

	return status;
}

static std::wstring getNextLevelInfo(std::wstring levArg)
{
	auto found = levArg.find(L"=", 0);
	if (found != std::string::npos)
	{
		//Menu or Level
		if (levArg.substr(found + 1, 2) == L"63")
			return std::wstring{ L"Main Menu" };

		wchar_t levStr[5];
		swprintf(levStr, 5, L"S%cE%c", levArg[found + 6], levArg[found + 5]);

		return std::wstring{ levStr };
	}
	return {};
}