#include "MFCMain.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(MFCMain, CWinApp)
	ON_COMMAND(ID_FILE_QUIT,	&MFCMain::MenuFileQuit)
	ON_COMMAND(ID_FILE_SAVETERRAIN, &MFCMain::Save)
	//ON_COMMAND(ID_EDIT_SELECT, &MFCMain::MenuEditSelect)
	ON_COMMAND(ID_EDIT_UNDO, &MFCMain::MenuEditUndo)
	ON_COMMAND(ID_EDIT_REDO, &MFCMain::MenuEditRedo)
	ON_COMMAND(ID_EDIT_DUPLICATE, &MFCMain::MenuEditDuplicate)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TOOL, &CMyFrame::OnUpdatePage)
	ON_COMMAND(ID_FILE_SAVETERRAIN, &MFCMain::Save)
	ON_COMMAND(ID_BUTTON40001, &MFCMain::Free)
	ON_COMMAND(ID_BUTTON40002, &MFCMain::TranslationX)
	ON_COMMAND(ID_BUTTON40003, &MFCMain::TranslationY)
	ON_COMMAND(ID_BUTTON40004, &MFCMain::TranslationZ)
	ON_COMMAND(ID_BUTTON40005, &MFCMain::ScaleX)
	ON_COMMAND(ID_BUTTON40006, &MFCMain::ScaleY)
	ON_COMMAND(ID_BUTTON40007, &MFCMain::ScaleZ)
	ON_COMMAND(ID_BUTTON40008, &MFCMain::RotationX)
	ON_COMMAND(ID_BUTTON40009, &MFCMain::RotationY)
	ON_COMMAND(ID_BUTTON40010, &MFCMain::RotationZ)
	ON_COMMAND(ID_BUTTON40011, &MFCMain::TerrainBuild)
	ON_COMMAND(ID_BUTTON40012, &MFCMain::TerrainDig)
	ON_COMMAND(ID_BUTTON40013, &MFCMain::TerrainFlatten)
	ON_COMMAND(ID_BUTTON40014, &MFCMain::TerrainSmooth)
END_MESSAGE_MAP()

BOOL MFCMain::InitInstance()
{
	//instanciate the mfc frame
	m_frame = new CMyFrame();
	m_pMainWnd = m_frame;

	m_frame->Create(	NULL,
					_T("CMP405 - 1703221"),
					WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
					CRect(100, 100, 1024, 768),
					NULL,
					NULL,
					0,
					NULL
				);

	///
	m_red_cursor = CreateCursor(RGB(255, 0, 0));
	m_green_cursor = CreateCursor(RGB(0, 255, 0));
	m_blue_cursor = CreateCursor(RGB(0, 0, 255));
	///

	//show and set the window to run and update. 
	m_frame->ShowWindow(SW_SHOW);
	m_frame->UpdateWindow();

	//get the rect from the MFC window so we can get its dimensions
	m_toolHandle = m_frame->m_DirXView.GetSafeHwnd();				//handle of directX child window
	m_frame->m_DirXView.GetClientRect(&WindowRECT);
	m_width		= WindowRECT.Width();
	m_height	= WindowRECT.Height();

	m_ToolSystem.onActionInitialise(m_toolHandle, m_width, m_height, m_red_cursor, m_green_cursor, m_blue_cursor);

	return TRUE;
}

int MFCMain::Run()
{
	MSG msg;
	BOOL bGotMsg;

	PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);

	while (WM_QUIT != msg.message)
	{
		
		if (true)
		{
			bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);
		}
		else
		{
			bGotMsg = (GetMessage(&msg, NULL, 0U, 0U) != 0);
		}

		if (bGotMsg)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			m_ToolSystem.UpdateInput(&msg);
		}
		else
		{	
			int ID = m_ToolSystem.getCurrentSelectionID();
			std::wstring statusString = L"Selected Object: " + std::to_wstring(ID);
			m_ToolSystem.Tick(&msg);

			//send current object ID to status bar in The main frame
			//m_frame->m_wndStatusBar.SetPaneText(1, statusString.c_str(), 1);	
		}
	}

	return (int)msg.wParam;
}

void MFCMain::MenuFileQuit()
{
	//will post message to the message thread that will exit the application normally
	PostQuitMessage(0);
}

void MFCMain::Save()
{
	m_ToolSystem.onActionSave();
	m_ToolSystem.onActionSaveTerrain();
}

void MFCMain::MenuEditUndo()
{
	m_ToolSystem.Undo();
}

void MFCMain::MenuEditRedo()
{
	m_ToolSystem.Redo();
}

void MFCMain::MenuEditDuplicate()
{
	m_ToolSystem.Duplicate();
}


//void MFCMain::MenuEditSelect()
//{
//	//SelectDialogue m_ToolSelectDialogue(NULL, &m_ToolSystem.m_sceneGraph);		//create our dialoguebox //modal constructor
//	//m_ToolSelectDialogue.DoModal();	// start it up modal
//
//	//modeless dialogue must be declared in the class.   If we do local it will go out of scope instantly and destroy itself
//	m_ToolSelectDialogue.Create(IDD_DIALOG1);	//Start up modeless
//	m_ToolSelectDialogue.ShowWindow(SW_SHOW);	//show modeless
//	m_ToolSelectDialogue.SetObjectData(&m_ToolSystem.m_scene_objects, &m_ToolSystem.m_selected_object_ID);
//}

void MFCMain::Free()
{
	m_ToolSystem.setObjectMode(ToolMain::OBJECT_MODE::FREE);
}

void MFCMain::TranslationX()
{
	m_ToolSystem.setObjectMode(ToolMain::OBJECT_MODE::TRANSLATE_X);
}

void MFCMain::TranslationY()
{
	m_ToolSystem.setObjectMode(ToolMain::OBJECT_MODE::TRANSLATE_Y);
}

void MFCMain::TranslationZ()
{
	m_ToolSystem.setObjectMode(ToolMain::OBJECT_MODE::TRANSLATE_Z);
}

void MFCMain::RotationX()
{
	m_ToolSystem.setObjectMode(ToolMain::OBJECT_MODE::ROTATE_X);
}

void MFCMain::RotationY()
{
	m_ToolSystem.setObjectMode(ToolMain::OBJECT_MODE::ROTATE_Y);
}

void MFCMain::RotationZ()
{
	m_ToolSystem.setObjectMode(ToolMain::OBJECT_MODE::ROTATE_Z);
}

void MFCMain::ScaleX()
{
	m_ToolSystem.setObjectMode(ToolMain::OBJECT_MODE::SCALE_X);
}

void MFCMain::ScaleY()
{
	m_ToolSystem.setObjectMode(ToolMain::OBJECT_MODE::SCALE_Y);
}

void MFCMain::ScaleZ()
{
	m_ToolSystem.setObjectMode(ToolMain::OBJECT_MODE::SCALE_Z);
}

void MFCMain::TerrainBuild()
{
	m_ToolSystem.setTerrainMode(ToolMain::TERRAIN_MODE::BUILD);
}

void MFCMain::TerrainDig()
{
	m_ToolSystem.setTerrainMode(ToolMain::TERRAIN_MODE::DIG);
}

void MFCMain::TerrainFlatten()
{
	m_ToolSystem.setTerrainMode(ToolMain::TERRAIN_MODE::FLATTEN);
}

void MFCMain::TerrainSmooth()
{
	m_ToolSystem.setTerrainMode(ToolMain::TERRAIN_MODE::SMOOTH);
}


MFCMain::MFCMain()
{
}


MFCMain::~MFCMain()
{
}

HCURSOR MFCMain::CreateCursor(COLORREF color)
{
	// AND bitmap is mask
	// XOR bitmap is color

	HDC hDC = GetDC(NULL);

	HDC hAndDC = CreateCompatibleDC(hDC);
	HDC hXorDC = CreateCompatibleDC(hDC);

	HBITMAP hAndBitmap = CreateCompatibleBitmap(hDC, 13, 19);
	HBITMAP hXorBitmap = CreateCompatibleBitmap(hDC, 13, 19);

	HBRUSH hBrush = CreateSolidBrush(color);

	HPEN hPen = CreatePen(PS_SOLID, 1, RGB(1, 1, 1)); // <--- can't be RGB(0, 0, 0) as this is used for transparency.

	POINT points[7];
	points[0].x = 0;
	points[0].y = 0;
	points[1].x = 12;
	points[1].y = 12;
	points[2].x = 7;
	points[2].y = 12;
	points[3].x = 9;
	points[3].y = 17;
	points[4].x = 7;
	points[4].y = 18;
	points[5].x = 4;
	points[5].y = 13;
	points[6].x = 0;
	points[6].y = 17;

	SelectObject(hXorDC, hXorBitmap);
	SelectObject(hXorDC, hBrush);
	SelectObject(hXorDC, hPen);
	Polygon(hXorDC, points, 7);

	DeleteObject(hBrush);
	DeleteObject(hPen);

	BITMAP xorBitmap;
	GetObject(hXorBitmap, sizeof(BITMAP), &xorBitmap);

	SelectObject(hAndDC, hAndBitmap);

	COLORREF pixelColor;
	for (int x = 0; x < xorBitmap.bmWidth; x++)
	{
		for (int y = 0; y < xorBitmap.bmHeight; y++)
		{
			pixelColor = GetPixel(hXorDC, x, y);
			if (pixelColor == RGB(0, 0, 0))					
			{
				SetPixel(hAndDC, x, y, RGB(255, 255, 255));
			}
			else
			{
				SetPixel(hAndDC, x, y, RGB(0, 0, 0));
			}
		}
	}

	DeleteDC(hAndDC);
	DeleteDC(hXorDC);

	ReleaseDC(NULL, hDC);

	ICONINFO iconInfo = { 0 };
	iconInfo.fIcon = FALSE;
	iconInfo.xHotspot = 0;
	iconInfo.yHotspot = 0;
	iconInfo.hbmMask = hAndBitmap;
	iconInfo.hbmColor = hXorBitmap;

	HCURSOR cursor = CreateIconIndirect(&iconInfo);

	return cursor;
}