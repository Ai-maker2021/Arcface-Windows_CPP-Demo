
// ArcFaceDemo.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CArcFaceDemoApp: 
// �йش����ʵ�֣������ ArcFaceDemo.cpp
//

class CArcFaceDemoApp : public CWinApp
{
public:
	CArcFaceDemoApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��
	ULONG_PTR m_gdiplusToken; //GDI������Ա����

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
};

extern CArcFaceDemoApp theApp;