/*
GpuRamDrive proxy for ImDisk Virtual Disk Driver.

Copyright (C) 2016 Syahmi Azhar.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "stdafx.h"
#include "GpuRamDrive.h"
#include "GpuRamGui.h"
#include "resource.h"

#define GPU_GUI_CLASS L"GPURAMDRIVE_CLASS"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

std::wstring ToWide(const std::string& str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
	return convert.from_bytes(str);
}


GpuRamGui::GpuRamGui()
{
	INITCOMMONCONTROLSEX c;
	c.dwSize = sizeof(c);
	c.dwICC = 0;

	InitCommonControlsEx(&c);
}


GpuRamGui::~GpuRamGui()
{
}

bool GpuRamGui::Create(HINSTANCE hInst, const std::wstring& title, int nCmdShow)
{
	m_Instance = hInst;
	SetProcessDPIAware();

	MyRegisterClass();

	m_hWnd = CreateWindowW(GPU_GUI_CLASS, title.c_str(), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 700, 300, nullptr, nullptr, m_Instance, this);

	if (!m_hWnd) return false;

	ShowWindow(m_hWnd, nCmdShow);
	UpdateWindow(m_hWnd);

	return m_hWnd != NULL;
}

int GpuRamGui::Loop()
{
	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

void GpuRamGui::OnCreate()
{
	SetWindowLongPtr(m_hWnd, GWL_STYLE, GetWindowLongPtr(m_hWnd, GWL_STYLE) & ~WS_SIZEBOX & ~WS_MAXIMIZEBOX);

	HFONT FontBold = CreateFontA(-18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, "Segoe UI");
	HFONT FontNormal = CreateFontA(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, "Segoe UI");
	HWND hStatic;

	hStatic = CreateWindow(L"STATIC", L"Select Device:", WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 10, 13, 140, 20, m_hWnd, NULL, m_Instance, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)FontNormal, TRUE);

	hStatic = CreateWindow(L"STATIC", L"Memory Size (MB):", WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 10, 53, 140, 20, m_hWnd, NULL, m_Instance, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)FontNormal, TRUE);

	hStatic = CreateWindow(L"STATIC", L"Drive Letter:", WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 10, 93, 140, 20, m_hWnd, NULL, m_Instance, NULL);
	SendMessage(hStatic, WM_SETFONT, (WPARAM)FontNormal, TRUE);

	m_CtlGpuList = CreateWindow(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 150, 10, 150, 25, m_hWnd, NULL, m_Instance, NULL);
	SendMessage(m_CtlGpuList, WM_SETFONT, (WPARAM)FontNormal, TRUE);

	m_CtlMemSize = CreateWindow(L"EDIT", L"1", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER, 150, 50, 150, 25, m_hWnd, NULL, m_Instance, NULL);
	SendMessage(m_CtlMemSize, WM_SETFONT, (WPARAM)FontNormal, TRUE);

	m_CtlDriveLetter = CreateWindow(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 150, 90, 150, 25, m_hWnd, NULL, m_Instance, NULL);
	SendMessage(m_CtlDriveLetter, WM_SETFONT, (WPARAM)FontNormal, TRUE);

	m_CtlMountBtn = CreateWindow(L"BUTTON", L"Mount", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 150, 150, 150, 40, m_hWnd, NULL, m_Instance, NULL);
	SendMessage(m_CtlMountBtn, WM_SETFONT, (WPARAM)FontBold, TRUE);

	wchar_t szTemp[64];
	wcscpy_s(szTemp, L"X:");
	for (wchar_t c = 'A'; c <= 'Z'; c++) {
		szTemp[0] = c;
		ComboBox_AddString(m_CtlDriveLetter, szTemp);
	}

	int suggestedRamSize = 1;
	try
	{
		m_RamDrive.RefreshGPUInfo();
		auto v = m_RamDrive.GetGpuDevices();
		for (auto it = v.begin(); it != v.end(); it++)
		{
			ComboBox_AddString(m_CtlGpuList, ToWide(it->name + " (" + std::to_string(it->memsize / (1024 * 1024)) + " MB)").c_str());
			if (it->memsize) {
				suggestedRamSize = (int)(it->memsize * 0.8 / 1024 / 1024);
			}
		}
	}
	catch (const std::exception& ex)
	{
		ComboBox_AddString(m_CtlGpuList, ToWide(ex.what()).c_str());
	}

	ComboBox_SetCurSel(m_CtlGpuList, ComboBox_GetCount(m_CtlGpuList) - 1);
	ComboBox_SetCurSel(m_CtlDriveLetter, 'R' - 'A');

	wcscpy_s(szTemp, L"1");
	_itow_s(suggestedRamSize, szTemp, 10);
	Edit_SetText(m_CtlMemSize, szTemp);

	m_RamDrive.SetStateChangeCallback([&]() {
		m_UpdateState = true;
		InvalidateRect(m_hWnd, NULL, FALSE);
	});
	m_UpdateState = true;
}

void GpuRamGui::OnDestroy()
{
	PostQuitMessage(0);
}

void GpuRamGui::OnResize(WORD width, WORD height)
{
	MoveWindow(m_CtlGpuList, 150, 10, width - 150 - 20, 20, TRUE);
	MoveWindow(m_CtlMountBtn, width / 2 - 150, height - 90, 300, 70, TRUE);
}

void GpuRamGui::OnMountClicked()
{
	if (!m_RamDrive.IsMounted())
	{
		auto vGpu = m_RamDrive.GetGpuDevices();
		int n = ComboBox_GetCurSel(m_CtlGpuList);

		if (n >= (int)vGpu.size()) {
			MessageBox(m_hWnd, L"GPU selection is invalid", L"Error while selecting GPU", MB_OK);
			return;
		}

		wchar_t szTemp[64] = { 0 };
		Edit_GetText(m_CtlMemSize, szTemp, sizeof(szTemp) / sizeof(wchar_t));
		size_t memSize = (size_t)_wtoi64(szTemp) * 1024 * 1024;

		if (memSize >= vGpu[n].memsize) {
			MessageBox(m_hWnd, L"The memory size you specified is too large", L"Invalid memory size", MB_OK);
			return;
		}

		ComboBox_GetText(m_CtlDriveLetter, szTemp, sizeof(szTemp) / sizeof(wchar_t));

		try
		{
			m_RamDrive.CreateRamDevice(vGpu[n].platform_id, vGpu[n].device_id, L"GpuRamDev", memSize, szTemp);
		}
		catch (const std::exception& ex)
		{
			MessageBoxA(m_hWnd, ex.what(), "Error while mounting GPU Ram Drive", MB_OK);
		}
	}
	else
	{
		m_RamDrive.ImdiskUnmountDevice();
	}
}

void GpuRamGui::UpdateState()
{
	if (!m_UpdateState) return;

	if (m_RamDrive.IsMounted())
	{
		EnableWindow(m_CtlDriveLetter, FALSE);
		EnableWindow(m_CtlGpuList, FALSE);
		EnableWindow(m_CtlMemSize, FALSE);
		Edit_SetText(m_CtlMountBtn, L"Unmount");
	}
	else
	{
		EnableWindow(m_CtlDriveLetter, TRUE);
		EnableWindow(m_CtlGpuList, TRUE);
		EnableWindow(m_CtlMemSize, TRUE);
		Edit_SetText(m_CtlMountBtn, L"Mount");
	}

	m_UpdateState = false;
}

ATOM GpuRamGui::MyRegisterClass()
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = m_Instance;
	wcex.hIcon = LoadIcon(m_Instance, MAKEINTRESOURCE(IDI_ICON1));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = GPU_GUI_CLASS;
	wcex.hIconSm = wcex.hIcon;

	return RegisterClassExW(&wcex);
}

LRESULT CALLBACK GpuRamGui::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	GpuRamGui* _this = (GpuRamGui*)(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{
		case WM_CREATE:
		{
			LPCREATESTRUCTW pCreateParam = (LPCREATESTRUCTW)lParam;
			if (pCreateParam) {
				_this = (GpuRamGui*)pCreateParam->lpCreateParams;
				SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pCreateParam->lpCreateParams);
			}

			if (_this) {
				_this->m_hWnd = hWnd;
				_this->OnCreate();
			}
			break;
		}

		case WM_DESTROY:
			if (_this) _this->OnDestroy();
			break;

		case WM_SIZE:
			if (_this) _this->OnResize(LOWORD(lParam), HIWORD(lParam));
			break;

		case WM_PAINT:
			_this->UpdateState();
			return DefWindowProc(hWnd, message, wParam, lParam);
			break;

		case WM_COMMAND:
		{
			if (_this) {
				if ((HANDLE)lParam == _this->m_CtlMountBtn) {
					_this->OnMountClicked();
				}
			}
			break;
		}

		default:
		{
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}

	return 0;
}
