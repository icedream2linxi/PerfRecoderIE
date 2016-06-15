#include "stdafx.h"
#include "ReportCtrl.h"

CReportCtrl::CReportCtrl()
	: m_backgroundColor(RGB(255, 255, 255))
	, m_height(0)
{
	WNDCLASS wndCls;
	if (!::GetClassInfo(NULL, GetWndClassName(), &wndCls))
		::RegisterClassEx(&GetWndClassInfo().m_wc);
}

LPCTSTR CReportCtrl::GetWndClassName()
{
	return _T("ReportCtrl");
}

ATL::CWndClassInfo& CReportCtrl::GetWndClassInfo()
{
	static ATL::CWndClassInfo wc =
	{
	{ sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, ::DefWindowProc,
		0, 0, NULL, NULL, NULL, (HBRUSH)(COLOR_WINDOW + 1), NULL, GetWndClassName(), NULL },
		NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
	};
	return wc;
}

void CReportCtrl::DoPaint(CDCHandle dc)
{
	if (m_memDc == nullptr)
		return;

	CRect rect(m_memDc->m_rcPaint);
	dc.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), *m_memDc, 0, 0, SRCCOPY);
}

void CReportCtrl::update(const std::wstring &data)
{
	m_data = data;
	CDCHandle dc(GetDC());
	auto oldFont = dc.SelectFont(GetParent().GetFont());
	CSize size;
	dc.GetTextExtent(m_data.c_str(), m_data.size(), &size);
	//dc.GetTextExtentExPoint(m_data.c_str(), m_data.size(), &size, MM_TEXT);
	auto count = std::count(m_data.begin(), m_data.end(), L'\r');
	uint32_t height = size.cy * count;
	dc.SelectFont(oldFont);

	if (m_height != height) {
		m_height = height;
		CRect rect;
		GetClientRect(rect);
		m_size.cx = rect.Width() - 20;
		m_size.cy = m_height;
		SetScrollSize(m_size);

		if (rect.Width() < m_size.cx)
			rect.right = rect.left + m_size.cx;
		if (rect.Height() < m_size.cy)
			rect.bottom = rect.top + m_size.cy;
		m_memDc.reset(new CMemoryDC(dc, rect));
	}

	DrawData(*m_memDc);

	Invalidate();
}

void CReportCtrl::DrawData(CMemoryDC &dc)
{
	dc.FillSolidRect(&dc.m_rcPaint, m_backgroundColor);

	auto oldFont = dc.SelectFont(GetParent().GetFont());

	//dc.TextOut(0, 0, m_data.c_str());
	dc.DrawText(m_data.c_str(), m_data.size(), &dc.m_rcPaint, DT_LEFT);

	dc.SelectFont(oldFont);
}
