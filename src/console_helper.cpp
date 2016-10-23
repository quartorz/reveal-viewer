#include <Windows.h>

#include <string>

int APIENTRY wWinMain(HINSTANCE hinst, HINSTANCE, LPWSTR cmdline, int)
{
	wchar_t *cmdline_rest = nullptr;
	HANDLE handles[2] = {
		reinterpret_cast<HANDLE>(std::wcstoull(cmdline, &cmdline_rest, 10))
	};

	if (handles[0] == nullptr) {
		return 0;
	}

	::AllocConsole();

	STARTUPINFOW si = {};

	si.cb = sizeof(STARTUPINFOW);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);

	PROCESS_INFORMATION pi = {};

	auto result = ::CreateProcessW(
		nullptr,
		cmdline_rest + 1,
		nullptr,
		nullptr,
		TRUE,
		0,
		nullptr,
		nullptr,
		&si,
		&pi);

	if (result == FALSE) {
		return 0;
	}

	handles[1] = pi.hProcess;

	DWORD exit_code = 0;

	::SetConsoleCtrlHandler([](DWORD) -> BOOL {return TRUE; }, TRUE);

	switch (::WaitForMultipleObjects(2, handles, FALSE, INFINITE)) {
	case WAIT_OBJECT_0 + 0:
		::GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0);
		::WaitForSingleObject(handles[1], INFINITE);
		break;

	case WAIT_OBJECT_0 + 1:
		::GetExitCodeProcess(handles[1], &exit_code);
		break;
	}

	::FreeConsole();

	return 0;
}
