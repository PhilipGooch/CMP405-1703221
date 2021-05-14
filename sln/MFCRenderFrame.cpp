
#include "MFCRenderFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

CChildRender::CChildRender()
{
}

CChildRender::~CChildRender()
{
}


BEGIN_MESSAGE_MAP(CChildRender, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()



// CChildView message handlers

BOOL CChildRender::PreCreateWindow(CREATESTRUCT& cs)
{
	///
	//cs.style |= WS_CLIPCHILDREN;
	///

	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	//cs.dwExStyle |= WS_EX_CLIENTEDGE;			// <--- commented out to fix offset with SetCursorPos
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
		/*::LoadCursor(NULL, IDC_ARROW),*/ NULL, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), NULL);	// <--- setting cursor to be NULL so SetCursor isnt overwritten


	return TRUE;
}

void CChildRender::OnPaint()
{
	CPaintDC dc(this); // device context for painting
}


