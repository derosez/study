﻿// LockDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteCtl.h"
#include "afxdialogex.h"
#include "LockInfoDialog.h"


// CLockDialog 对话框

IMPLEMENT_DYNAMIC(CLockInfoDialog, CDialog)

CLockInfoDialog::CLockInfoDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_INFO, pParent)
{

}

CLockInfoDialog::~CLockInfoDialog()
{
}

void CLockInfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLockInfoDialog, CDialog)
END_MESSAGE_MAP()


// CLockDialog 消息处理程序
