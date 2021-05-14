#pragma once

#include <afxwin.h> 
#include <afxext.h>
#include <afx.h>
#include "pch.h"
#include "Game.h"
#include "ToolMain.h"
#include "resource.h"
#include "MFCFrame.h"
#include "SelectDialogue.h"

class MFCMain : public CWinApp 
{
public:
	MFCMain();
	~MFCMain();

	BOOL InitInstance();
	int  Run();

	///
	HCURSOR CreateCursor(COLORREF color);
	///

private:

	CMyFrame * m_frame;	//handle to the frame where all our UI is
	HWND m_toolHandle;	//Handle to the MFC window
	ToolMain m_ToolSystem;	//Instance of Tool System that we interface to. 
	CRect WindowRECT;	//Window area rectangle. 
	SelectDialogue m_ToolSelectDialogue;			//for modeless dialogue, declare it here

	///
	HCURSOR m_red_cursor;
	HCURSOR m_green_cursor;
	HCURSOR m_blue_cursor;
	///

	int m_width;		
	int m_height;
	


	//Interface funtions for menu and toolbar etc requires
	afx_msg void MenuFileQuit();
	afx_msg void Save();
	//afx_msg void MenuEditSelect();
	afx_msg void MenuEditUndo();
	afx_msg void MenuEditRedo();
	afx_msg void MenuEditDuplicate();
	

	// Interface funtions for toolbar
	afx_msg	void Free();
	afx_msg	void TranslationX();
	afx_msg	void TranslationY();
	afx_msg	void TranslationZ();
	afx_msg	void RotationX();
	afx_msg	void RotationY();
	afx_msg	void RotationZ();
	afx_msg	void ScaleX();
	afx_msg	void ScaleY();
	afx_msg	void ScaleZ();
	afx_msg	void TerrainBuild();
	afx_msg	void TerrainDig();
	afx_msg	void TerrainFlatten();
	afx_msg	void TerrainSmooth();


	DECLARE_MESSAGE_MAP()	// required macro for message map functionality  One per class
};
