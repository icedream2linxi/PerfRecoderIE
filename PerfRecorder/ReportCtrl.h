#pragma once
#include <string>
#include <memory>

class CReportCtrl : public CScrollWindowImpl<CReportCtrl>
{
public:
	//DECLARE_WND_CLASS(L"ReportCtrl")

	CReportCtrl();
	static LPCTSTR GetWndClassName();
	static ATL::CWndClassInfo& GetWndClassInfo();

	BEGIN_MSG_MAP(CReportCtrl)
		CHAIN_MSG_MAP(CScrollWindowImpl<CReportCtrl>);
	END_MSG_MAP()

	void DoPaint(CDCHandle dc);
	void update(const std::wstring &data);

private:
	void DrawData(CMemoryDC &dc);

private:
	COLORREF m_backgroundColor;
	std::wstring m_data;
	CSize m_size;
	uint32_t m_height;
	std::shared_ptr<CMemoryDC> m_memDc;
};