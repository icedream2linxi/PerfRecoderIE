// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"

#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <codecvt>
#include <locale>
#include <boost/algorithm/string.hpp>
#include <psapi.h>
#include <winbase.h>
#include <TlHelp32.h>
#include <ProcessResourceUsage.hpp>
#include <GPUResourceUsage.hpp>

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		if (pMsg->wParam == VK_ESCAPE)
			return TRUE;
	}
	if (pMsg->message == WM_KEYUP && pMsg->wParam == VK_RETURN && pMsg->hwnd == m_hEdModuleFilter)
		OnModuleFilterInputed();
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	UIUpdateChildWindows();
	return FALSE;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	DoDataExchange();
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	DlgResize_Init();

	auto msg = ProcessResourceUsage::detectEnvironment();
	if (!msg.empty()) {
		MessageBox(msg.c_str(), L"错误", MB_OK | MB_ICONERROR);
	}

	initNetworkAdapter();
	initModuleFilter();
	m_chkRecordToFile.Attach(GetDlgItem(IDC_RECORD_TO_FILE_CHK));
	m_chkRecordToFile.SetCheck(BST_UNCHECKED);
	m_recordStatus = rsTerminated;

	m_stopRecord = false;
	m_recordThread = std::thread(&CMainDlg::run, this);

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_stopRecord = true;
	m_recordThread.join();

	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	return 0;
}

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add validation code 
	//CloseDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}


void CMainDlg::initNetworkAdapter()
{
	m_cmbNewtorkAdapter.Attach(GetDlgItem(IDC_NETWORK_ADAPTER_CMB));

	auto adapters = ProcessResourceUsage::getInstance().getNetworkInterfaceNames();
	for each (auto adapter in adapters) {
		m_cmbNewtorkAdapter.AddString(adapter.c_str());
	}

	if (m_cmbNewtorkAdapter.GetCount() != 0) {
		m_cmbNewtorkAdapter.SetCurSel(0);
		ProcessResourceUsage::getInstance().selectNetworkInterface(0);
	}
}

void CMainDlg::initModuleFilter()
{
	m_cmbModuleFilter.Attach(GetDlgItem(IDC_MODULE_FILTER_CMB));
	COMBOBOXINFO comboBoxInfo;
	comboBoxInfo.cbSize = sizeof(COMBOBOXINFO);
	m_cmbModuleFilter.GetComboBoxInfo(&comboBoxInfo);
	m_hEdModuleFilter = comboBoxInfo.hwndItem;

	loadModuleFilter();

	if (m_cmbModuleFilter.GetCount() == 0) {
		std::vector<std::wstring> filters = {
			L"osgviewerMFC.exe"
		};

		for each (auto filter in filters) {
			m_cmbModuleFilter.AddString(filter.c_str());
		}

		saveModuleFilter();
	}

	m_cmbModuleFilter.SetCurSel(0);
	m_cmbModuleFilter.AddString(L"清除并重置...");

	CString filter;
	m_cmbModuleFilter.GetLBText(0, filter.GetBuffer(m_cmbModuleFilter.GetLBTextLen(0)));
	filter.ReleaseBuffer();
	setNewModuleFilter(filter);
}

void CMainDlg::clearAndResetModuleFilter()
{
	m_cmbModuleFilter.ResetContent();

	if (m_cmbModuleFilter.GetCount() == 0) {
		std::vector<std::wstring> filters = {
			L"osgviewerMFC.exe"
		};

		for each (auto filter in filters) {
			m_cmbModuleFilter.AddString(filter.c_str());
		}

		saveModuleFilter();
	}

	m_cmbModuleFilter.SetCurSel(0);
	m_cmbModuleFilter.AddString(L"清除并重置...");

	CString filter;
	m_cmbModuleFilter.GetLBText(0, filter.GetBuffer(m_cmbModuleFilter.GetLBTextLen(0)));
	filter.ReleaseBuffer();
	setNewModuleFilter(filter);
}

void CMainDlg::loadModuleFilter()
{
	auto cfgFile = getConfigFile();

	std::wifstream fin(cfgFile);
	fin.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::consume_header>()));
	std::wstring filter;
	while (std::getline(fin, filter)) {
		m_cmbModuleFilter.AddString(filter.c_str());
	}
}

void CMainDlg::saveModuleFilter()
{
	auto cfgFile = getConfigFile();

	std::wofstream fin(cfgFile);
	fin.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::generate_header>()));
	int count = m_cmbModuleFilter.GetCount() - 1;
	for (int i = 0; i < count; ++i) {
		CString filter;
		m_cmbModuleFilter.GetLBText(i, filter.GetBuffer(m_cmbModuleFilter.GetLBTextLen(i)));
		filter.ReleaseBuffer();
		fin << static_cast<const wchar_t*>(filter) << std::endl;
	}
}

fs::path CMainDlg::getExeFile() const
{
	DWORD nSize = 256;
	wchar_t *buffer = new wchar_t[nSize];
	nSize = GetModuleFileName(_Module.GetModuleInstance(), buffer, nSize);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		delete[] buffer;
		buffer = new wchar_t[nSize];
		nSize = GetModuleFileName(_Module.GetModuleInstance(), buffer, nSize);
	}

	fs::path path = buffer;
	delete[] buffer;

	return path;
}

std::wstring CMainDlg::getConfigFile() const
{
	auto path = getExeFile();
	path.replace_extension(L"cfg");
	return path.wstring();
}

std::wstring CMainDlg::getReportFile(const SYSTEMTIME &sysTime) const
{
	auto path = getExeFile();
	wchar_t buffer[256] = { 0 };
	swprintf_s(buffer, L"%d-%02d-%02d_%02d-%02d-%02d", sysTime.wYear, sysTime.wMonth, sysTime.wDay,
		sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	std::wstring filename(L"PerfReport_");
	filename += buffer;
	filename += L".csv";
	path = path.replace_filename(filename);
	return path.wstring();
}

std::wstring FormatSysTime(const SYSTEMTIME &sysTime)
{
	wchar_t buffer[256] = { 0 };
	swprintf_s(buffer, L"%d-%02d-%02d %02d:%02d:%02d:%03d", sysTime.wYear, sysTime.wMonth, sysTime.wDay,
		sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
	return std::wstring(buffer);
}

void CMainDlg::run()
{
	const std::wstring tab = L"    ";
	const std::wstring CRLN = L"\r\n";

	std::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>> cvt;

	auto prevTime = std::chrono::high_resolution_clock::now() - std::chrono::seconds(1);
	auto interval = std::chrono::milliseconds(500);
	std::set<DWORD> prevPids;
	std::wofstream reportFout;
	uint32_t processIdSeed = 1;

	while (!m_stopRecord) {
		auto nowTime = std::chrono::high_resolution_clock::now();
		auto span = nowTime - prevTime;
		if (span < interval) {
			std::this_thread::sleep_for(interval - span);
			continue;
		}

		ProcessResourceUsage::getInstance().record();
		prevTime = std::chrono::high_resolution_clock::now();

		using namespace std;
		std::wstringstream ss;
		ss.precision(2);
		auto totalUsage = ProcessResourceUsage::getTotalUsage();
		auto gpuUsages = GPUResourceUsage::getInstance().getUsages();
		for each (auto usage in totalUsage) {
			ss << usage->name << CRLN << flush;
			ss << tab << L"GPU利用率：" << fixed << (usage->usage * 100) << L"%" << CRLN << flush;
			ss << tab << L"专用内存：" << formatSize(usage->dedicatedUsage) << L" / " << formatSize(usage->dedicatedLimit) << CRLN << flush;
			ss << tab << L"共享内存：" << formatSize(usage->sharedUsage) << L" / " << formatSize(usage->sharedLimit) << CRLN << flush;
		}

		for each (auto usage in gpuUsages) {
			ss << cvt.from_bytes(usage->name) << CRLN << flush;
			ss << tab << L"GPU利用率：" << fixed << (usage->gpuUsage * 100) << L"%" << CRLN << flush;
			ss << tab << L"专用内存：" << formatSize(usage->usedMemory) << L" / " << formatSize(usage->totalMemory) << CRLN << flush;
		}

		auto usages = ProcessResourceUsage::getInstance().getUsages();
		for each (auto usage in usages) {
			ss << CRLN << flush;
			ss << getProcessFileName(usage->processId) << L"[" << usage->processId << L"]" << CRLN << flush;
			ss << tab << L"CPU利用率：" << fixed << (usage->cpuUsage * 100) << L"%" << CRLN << flush;
			ss << tab << L"内存：" << formatSize(usage->pagefileUsage) << CRLN << flush;
			ss << tab << L"接收速率：" << formatSize(usage->recvSpeed) << L"/S" << CRLN << flush;
			ss << tab << L"发送速率：" << formatSize(usage->sendSpeed) << L"/S" << CRLN << flush;
			for each (auto gpu in usage->gpuUsages) {
				ss << tab << gpu->adapterName << CRLN << flush;
				ss << tab << tab << L"GPU利用率：" << fixed << (gpu->usage * 100) << L"%" << CRLN << flush;
				ss << tab << tab << L"专用内存：" << formatSize(gpu->dedicatedUsage) << CRLN << flush;
				ss << tab << tab << L"共享内存：" << formatSize(gpu->sharedUsage) << CRLN << flush;
			}
		}

		auto report = ss.str();
		std::lock_guard<std::mutex> lock(m_usageReportMutex);
		m_usageReport.swap(report);
		PostMessage(WM_REPORT);

		SYSTEMTIME sysTime;
		GetLocalTime(&sysTime);
		if (m_recordStatus == rsWaitToStart) {
			m_recordStatus = rsRecording;
			reportFout.open(getReportFile(sysTime));
			reportFout.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::generate_header>()));
			reportFout.precision(2);

			for (size_t i = 0; i < totalUsage.size(); ++i) {
				auto usage = totalUsage[i];
				reportFout << L"GPU" << i << L"," << usage->name
					<< L",专用显存," << usage->dedicatedLimit
					<< L",共享显存," << usage->sharedLimit
					<< endl;
			}

			reportFout << L"时间,进程ID,CPU利用率,内存,接收速率,发送速率,";
			for (size_t i = 0; i < totalUsage.size(); ++i) {
				auto usage = totalUsage[i];
				reportFout << L"GPU" << i << L"利用率,";
				reportFout << L"GPU" << i << L"专用内存,";
				reportFout << L"GPU" << i << L"共享内存,";
			}
			
			reportFout << endl;
		}
		else if (m_recordStatus == rsWaitToTerminate) {
			m_recordStatus = rsTerminated;
			reportFout.flush();
			reportFout.close();
		}
		else if (m_recordStatus == rsRecording) {
			for each (auto usage in usages) {
				reportFout << FormatSysTime(sysTime) << L"," << m_processUniqueId[usage->processId] << L","
					<< fixed << (usage->cpuUsage * 100) << L"%" << L","
					<< usage->pagefileUsage << L","
					<< usage->recvSpeed << L"B/S" << L","
					<< usage->sendSpeed << L"B/S" << L",";

				for each (auto gpu in usage->gpuUsages) {
					reportFout << fixed << (gpu->usage * 100) << L"%" << L","
						<< gpu->dedicatedUsage << L","
						<< gpu->sharedUsage << L",";
				}

				reportFout << endl;
			}
		}

		// 更新要监测的进程
		//if (!m_modulesChanged)
		//	continue;
		auto pids = filterProcessId();
		m_modulesChanged = false;
		std::set<DWORD> diffPids;
		// 在原进程集中而不在现进程集，则将其删除
		std::set_difference(prevPids.begin(), prevPids.end(), pids.begin(), pids.end(), std::inserter(diffPids, diffPids.end()));
		for each (auto pid in diffPids) {
			ProcessResourceUsage::getInstance().removeProcess(pid);
			m_processUniqueId.erase(pid);
		}

		// 在现进程集而不在原进程集，则添加到监测中
		diffPids.clear();
		std::set_difference(pids.begin(), pids.end(), prevPids.begin(), prevPids.end(), std::inserter(diffPids, diffPids.end()));
		for each (auto pid in diffPids) {
			ProcessResourceUsage::getInstance().addProcess(pid);
			m_processUniqueId.insert(std::make_pair(pid, processIdSeed++));
		}

		prevPids.swap(pids);
	}
}

std::wstring CMainDlg::getProcessFileName(DWORD pid)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (hProcess == NULL)
		return L"";
	DWORD nSize = 256;
	wchar_t *buffer = new wchar_t[nSize];
	BOOL ret = QueryFullProcessImageName(hProcess, 0, buffer, &nSize);
	CloseHandle(hProcess);

	hProcess = NULL;
	if (nSize == 0) {
		delete[] buffer;
		return L"";
	}

	std::wstring name(buffer);
	delete[] buffer;
	return name;
}

std::wstring CMainDlg::formatSize(uint64_t size)
{
	wchar_t buffer[128] = { 0 };
	if (size <= 1024)
		swprintf_s(buffer, L"%I64dB", size);
	else if (size <= 1024 * 1024)
		swprintf_s(buffer, L"%.2lfKB", size / 1024.0);
	else if (size <= 1024ull * 1024 * 1024)
		swprintf_s(buffer, L"%.2lfMB", size / (1024.0 * 1024.0));
	else if (size <= 1024ull * 1024 * 1024 * 1024)
		swprintf_s(buffer, L"%.2lfGB", size / (1024.0 * 1024.0 * 1024.0));
	return std::wstring(buffer);
}

void CMainDlg::OnModuleFilterInputed()
{
	CString inputedFilter;
	int len = m_cmbModuleFilter.GetWindowTextLength() + 1;
	m_cmbModuleFilter.GetWindowText(inputedFilter.GetBuffer(len), len);
	inputedFilter.ReleaseBuffer(len);

	int count = m_cmbModuleFilter.GetCount() - 1;
	int idx = -1;
	for (int i = 0; i < count; ++i) {
		CString filter;
		m_cmbModuleFilter.GetLBText(i, filter.GetBuffer(m_cmbModuleFilter.GetLBTextLen(i)));
		filter.ReleaseBuffer();

		if (filter.CompareNoCase(inputedFilter) == 0) {
			idx = i;
			break;
		}
	}

	if (idx >= 0)
		m_cmbModuleFilter.DeleteString(idx);
	m_cmbModuleFilter.InsertString(0, inputedFilter);

	saveModuleFilter();

	CString filter;
	m_cmbModuleFilter.GetLBText(0, filter.GetBuffer(m_cmbModuleFilter.GetLBTextLen(0)));
	filter.ReleaseBuffer();
	setNewModuleFilter(filter);
}

std::set<DWORD> CMainDlg::filterProcessId()
{
	DWORD processIds[1024];
	DWORD processCount = 1024;
	EnumProcesses(processIds, sizeof(processIds), &processCount);
	processCount /= sizeof(DWORD);

	std::set<DWORD> pids;
	wchar_t moduleFilePath[1024];
	DWORD fileNameLen = 1024;
	for (DWORD i = 0; i < processCount; ++i) {
		DWORD pid = processIds[i];
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
		if (NULL == hProcess)
			continue;

		DWORD len = GetProcessImageFileName(hProcess, moduleFilePath, fileNameLen);
		if (len != 0) {
			std::lock_guard<std::mutex> lock(m_modulesMutex);
			LPWSTR fileName = PathFindFileName(moduleFilePath);
			for (DWORD m = 0; m < len; ++m)
				fileName[m] = tolower(fileName[m]);
			if (m_modules.count(fileName) != 0) {
				pids.insert(pid);
				continue;
			}
		}

		//HMODULE hModules[1024];
		//DWORD moduleCount = 1024;
		//if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &moduleCount)) {
		//	moduleCount /= sizeof(HMODULE);

		//	for (DWORD j = 0; j < moduleCount; ++j) {
		//		len = GetModuleFileNameEx(hProcess, hModules[j], moduleFilePath, fileNameLen);
		//		if (len != 0) {
		//			std::lock_guard<std::mutex> lock(m_modulesMutex);
		//			LPWSTR fileName = PathFindFileName(moduleFilePath);
		//			for (DWORD m = 0; m < len; ++m)
		//				fileName[m] = tolower(fileName[m]);
		//			if (m_modules.count(fileName) != 0) {
		//				pids.insert(pid);
		//				continue;
		//			}
		//		}
		//	}
		//}

		CloseHandle(hProcess);
	}

	return pids;
}

void CMainDlg::setNewModuleFilter(const wchar_t *filter)
{
	std::wstring tempFilter(filter);
	boost::to_lower(tempFilter);
	std::vector<std::wstring> modules;
	boost::split(modules, tempFilter, boost::is_any_of(L"|"));

	std::lock_guard<std::mutex> lock(m_modulesMutex);
	m_modules.clear();
	for each (auto &module in modules) {
		m_modules.insert(boost::trim_copy(module));
	}
	m_modulesChanged = true;
}

LRESULT CMainDlg::OnCbnSelChangeNetworkAdapter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	auto idx = m_cmbNewtorkAdapter.GetCurSel();
	ProcessResourceUsage::getInstance().selectNetworkInterface(idx);
	return 0;
}

LRESULT CMainDlg::OnReport(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	std::wstring report;
	{
		std::lock_guard<std::mutex> lock(m_usageReportMutex);
		m_usageReport.swap(report);
	}

	m_reportCtrl.update(report);
	return 0;
}


LRESULT CMainDlg::OnCbnSelChangeModuleFilter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	auto idx = m_cmbModuleFilter.GetCurSel();
	if (idx == 0)
		return 0;

	if (idx == m_cmbModuleFilter.GetCount() - 1)
		clearAndResetModuleFilter();
	else {
		CString filter;
		m_cmbModuleFilter.GetLBText(idx, filter.GetBuffer(m_cmbModuleFilter.GetLBTextLen(idx)));
		filter.ReleaseBuffer();
		setNewModuleFilter(filter);

		m_cmbModuleFilter.DeleteString(idx);
		m_cmbModuleFilter.InsertString(0, filter);
		m_cmbModuleFilter.SetCurSel(0);
		saveModuleFilter();
	}

	return 0;
}



LRESULT CMainDlg::OnBnClickedRecordToFileChk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (m_chkRecordToFile.GetCheck() == BST_CHECKED) {
		m_recordStatus = rsWaitToStart;
	}
	else if (m_chkRecordToFile.GetCheck() == BST_UNCHECKED) {
		m_recordStatus = rsWaitToTerminate;
	}
	return 0;
}
