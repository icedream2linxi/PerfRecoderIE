#include "stdafx.h"
#include "LineChart.h"

CLineChart::CLineChart()
	: m_backgroundColor(RGB(255, 255, 255))
{
	WNDCLASS wndCls;
	if (!::GetClassInfo(NULL, GetWndClassName(), &wndCls))
		::RegisterClassEx(&GetWndClassInfo().m_wc);
}

LPCTSTR CLineChart::GetWndClassName()
{
	return _T("LineChart");
}

ATL::CWndClassInfo& CLineChart::GetWndClassInfo()
{
	static ATL::CWndClassInfo wc =
	{
	{ sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, ::DefWindowProc,
		0, 0, NULL, NULL, NULL, (HBRUSH)(COLOR_WINDOW + 1), NULL, GetWndClassName(), NULL },
		NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
	};
	return wc;
}

LRESULT CLineChart::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// TODO: Add your message handler code here and/or call default
	CRect rect;
	GetClientRect(rect);
	CMemoryDC dc(GetDC(), rect);

	dc.FillSolidRect(rect, m_backgroundColor);

	DrawGrid(dc, rect);

	return 0;
}

void CLineChart::DrawGrid(CDC &dc, const CRect &rect)
{
	const int height = rect.Height();
	const int width = rect.Width();
	const int heightSpan = height / 10;
	const int widthSpan = width / (60 / 10);

	for (int h = 0; h < height; h += heightSpan)
	{
		if (h == 0)
			continue;
		dc.MoveTo(0, h);
		dc.LineTo(width, h);
	}

	for (int w = 0; w < width; w += widthSpan)
	{
		if (w == 0)
			continue;
		dc.MoveTo(w, 0);
		dc.LineTo(w, width);
	}
}
