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
	const float height = (float)rect.Height();
	const float width = (float)rect.Width();
	const float heightSpan = height / 10;
	const float widthSpan = width / (60 / 10);

	for (float h = 0; h < height; h += heightSpan)
	{
		if (h == 0)
			continue;
		dc.MoveTo(0, (int)(height - h));
		dc.LineTo((int)width, (int)(height - h));
	}

	for (float w = 0; w < width; w += widthSpan)
	{
		if (w == 0)
			continue;
		dc.MoveTo((int)w, 0);
		dc.LineTo((int)w, (int)width);
	}
}
