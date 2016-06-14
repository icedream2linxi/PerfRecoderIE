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
#include <psapi.h>
#include <winbase.h>
#include <ProcessResourceUsage.hpp>
#include <GPUResourceUsage.hpp>

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
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

	m_cmbNewtorkAdapter.Attach(GetDlgItem(IDC_NETWORK_ADAPTER_CMB));

	initNetworkAdapter();

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
	CloseDialog(wID);
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
	auto adapters = ProcessResourceUsage::getInstance().getNetworkInterfaceNames();
	for each (auto adapter in adapters) {
		m_cmbNewtorkAdapter.AddString(adapter.c_str());
	}

	if (m_cmbNewtorkAdapter.GetCount() != 0) {
		m_cmbNewtorkAdapter.SetCurSel(0);
		ProcessResourceUsage::getInstance().selectNetworkInterface(0);
	}
}

#include <codecvt>
#include <locale>
void CMainDlg::run()
{
	//ProcessResourceUsage::getInstance().addProcess(40080);
	ProcessResourceUsage::getInstance().addProcess(7236);
	const std::wstring tab = L"    ";
	const std::wstring CRLN = L"\r\n";

	std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;

	auto prevTime = std::chrono::high_resolution_clock::now() - std::chrono::seconds(1);
	auto interval = std::chrono::milliseconds(500);

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
	//auto pos = m_edResourceUsage.GetScrollPos(SB_VERT);
	////m_edResourceUsage.SetWindowText(report.c_str());
	//m_edResourceUsage.SetSelAll(TRUE);
	//m_edResourceUsage.ReplaceSel(report.c_str());
	//m_edResourceUsage.SetScrollPos(SB_VERT, pos);
	return 0;
}
