#pragma once

class CLineChart : public CWindowImpl<CLineChart>
{
public:
	//DECLARE_WND_CLASS(L"LineChart")

	CLineChart();
	static LPCTSTR GetWndClassName();
	static ATL::CWndClassInfo& GetWndClassInfo();

	BEGIN_MSG_MAP(CLineChart)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
	END_MSG_MAP()

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void DrawGrid(CDC &dc, const CRect &rect);

private:
	COLORREF m_backgroundColor;
};