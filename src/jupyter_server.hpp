#pragma once

#include <Windows.h>

#include <cstdint>

#include "server_base.hpp"
#include "utility.hpp"

class jupyter_server : public server_base {
	boost::filesystem::path root_;

	bool running_ = false;

	HANDLE helper_process_ = nullptr;
	HANDLE cancel_event_ = nullptr;
	HANDLE hstdout_ = nullptr;

	unsigned short port_ = 0;

public:
	bool start(const boost::filesystem::path &root) override
 	{
		if (running_) {
			if (root_ == root) {
				return true;
			} else {
				return false;
			}
		}

		SECURITY_ATTRIBUTES sa = {};

		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;

		cancel_event_ = ::CreateEventW(&sa, TRUE, FALSE, nullptr);

		if (cancel_event_ == nullptr) {
			return false;
		}

		HANDLE hstdout_read = nullptr;
		HANDLE hstdout_write = nullptr;
		HANDLE hstderr_write = nullptr;

		if (::CreatePipe(&hstdout_read, &hstdout_write, &sa, 0) == FALSE) {
			return false;
		}

		if (::DuplicateHandle(
			::GetCurrentProcess(), hstdout_write, ::GetCurrentProcess(), &hstderr_write,
			0, TRUE, DUPLICATE_SAME_ACCESS) == FALSE)
		{
			::CloseHandle(hstdout_read);
			::CloseHandle(hstdout_write);
			return false;
		}

		if (::DuplicateHandle(
			::GetCurrentProcess(), hstdout_read, ::GetCurrentProcess(), &hstdout_,
			0, FALSE, DUPLICATE_SAME_ACCESS) == FALSE)
		{
			::CloseHandle(hstdout_read);
			::CloseHandle(hstdout_write);
			::CloseHandle(hstderr_write);
			return false;
		}

		STARTUPINFOW si = {};

		si.cb = sizeof(STARTUPINFOW);
		si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.hStdOutput = hstdout_write;
		si.hStdError = hstderr_write;
		si.wShowWindow = SW_HIDE;

		PROCESS_INFORMATION pi = {};

		std::wstring command = L'\"' + (get_exe_path().remove_filename() / L"console-helper.exe").wstring();

		command += L"\" ";
		command += std::to_wstring(reinterpret_cast<std::uintptr_t>(cancel_event_));
		command += L' ';
		command += L"jupyter notebook --no-browser ";
		command += L"--notebook-dir=\"";
		command += root.wstring();
		command += L'\"';

		auto result = ::CreateProcessW(
			nullptr,
			const_cast<LPWSTR>(command.c_str()),
			nullptr,
			nullptr,
			TRUE,
			CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
			nullptr,
			nullptr,
			&si,
			&pi);

		::CloseHandle(hstdout_read);
		::CloseHandle(hstdout_write);
		::CloseHandle(hstderr_write);

		if (result == FALSE) {
			::CloseHandle(hstdout_);
			return false;
		}

		running_ = true;
		helper_process_ = pi.hProcess;

		char buf_array[256];
		DWORD read;

		std::string buf;

		const char *text = "The Jupyter Notebook is running at: ";
		auto text_len = ::strlen(text);

		for (;;) {
			if (::ReadFile(hstdout_, (LPVOID)buf_array, 255, &read, nullptr) == FALSE) {
				return false;
			}

			buf_array[read] = '\0';
			buf += buf_array;

			auto pos = buf.find(text);

			if (pos != std::string::npos) {
				pos += text_len;

				auto pos_end = buf.find_first_of("\r\n", pos);

				if (pos_end != std::string::npos) {
					auto s = buf.substr(pos + 7, pos_end - pos - 7);
					pos = s.find(':');
					pos_end = s.find('/', pos);
					auto aa = s.substr(pos + 1, pos_end - pos - 1);
					if (pos != std::string::npos && pos_end != std::string::npos) {
						try {
							port_ = static_cast<unsigned short>(std::stoul(s.substr(pos + 1, pos_end - pos - 1)));
						} catch (std::exception &e) {
							stop();
							throw e;
						}
					}
					break;
				}
			}
		}

		return port_ != 0;
	}
	void stop() override
	{
		if (!running_) {
			return;
		}

		::SetEvent(cancel_event_);
		::WaitForSingleObject(helper_process_, INFINITE);

		running_ = false;
	}
	unsigned short get_port() override
	{
		return port_;
	}
};