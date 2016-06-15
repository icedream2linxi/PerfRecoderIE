// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include <thread>
#include <mutex>
#include <string>
#include <set>
#include "ReportCtrl.h"

#define WM_REPORT	(WM_USER + 100)

class CMainDlg : public CDialogImpl<CMainDlg>, public CUpdateUI<CMainDlg>, public CWinDataExchange<CMainDlg>,
	public CDialogResize<CMainDlg>,
		public CMessageFilter, public CIdleHandler
{
public:
	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_REPORT, OnReport)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_HANDLER(IDC_NETWORK_ADAPTER_CMB, CBN_SELCHANGE, OnCbnSelChangeNetworkAdapter)
		COMMAND_HANDLER(IDC_MODULE_FILTER_CMB, CBN_SELCHANGE, OnCbnSelChangeModuleFilter)
		CHAIN_MSG_MAP(CDialogResize<CMainDlg>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CMainDlg)
		DDX_CONTROL(IDC_REPORT_CTRL, m_reportCtrl)
	END_DDX_MAP()

	BEGIN_DLGRESIZE_MAP(CMainDlg)
		DLGRESIZE_CONTROL(IDC_NETWORK_ADAPTER_CMB, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_MODULE_FILTER_CMB, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_REPORT_CTRL, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnCbnSelChangeNetworkAdapter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCbnSelChangeModuleFilter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnReport(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void CloseDialog(int nVal);

private:
	void initNetworkAdapter();
	void initModuleFilter();
	void clearAndResetModuleFilter();
	void loadModuleFilter();
	void saveModuleFilter();
	std::wstring getConfigFile() const;
	void run();
	std::wstring getProcessFileName(DWORD pid);
	std::wstring formatSize(uint64_t size);
	void OnModuleFilterInputed();
	std::set<DWORD> filterProcessId();
	void setNewModuleFilter(const wchar_t *filter);

private:
	CComboBox m_cmbNewtorkAdapter;
	CComboBox m_cmbModuleFilter;
	HWND m_hEdModuleFilter;
	CReportCtrl m_reportCtrl;

	std::set<std::wstring> m_modules;
	bool m_modulesChanged;
	std::mutex m_modulesMutex;

	std::thread m_recordThread;
	bool m_stopRecord;
	std::wstring m_usageReport;
	std::mutex m_usageReportMutex;
};
