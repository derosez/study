﻿// RemoteCtl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include"PracticeTool.h"
#include "pch.h"
#include "framework.h"
#include "RemoteCtl.h"
#include "ServerSocket.h"
#include"Command.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif
// 唯一的应用程序对象
CWinApp theApp;
using namespace std;

int main()
{
    int nRetCode = 0;
    HMODULE hModule = ::GetModuleHandle(nullptr);


    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误  哈哈哈
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            CCommand cmd;
            CServerSocket* pserver = CServerSocket::getInstance();
            int count = 0;
            if (pserver->InitSocket() == false) {
                 MessageBox(NULL, _T("网络初始化异常，未能初始化成功，请检查网络状态！"), _T("网络初始化失败!"), MB_OK | MB_ICONERROR);
                 exit(0);
            }
            while(CServerSocket::getInstance() != NULL) {
                
                if (pserver->AcceptClient() == false) {
                    if (count >= 3) {
                        MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败!"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法正常接入用户，自动重试！"), _T("接入用户失败!"), MB_OK | MB_ICONERROR);
                    count++;
                }
                int ret = pserver->DealCommand();

                //TODO: 
                if (ret > 0) {
					ret = cmd.ExecuteCommand(pserver->GetPack().sCmd);
					if (ret != 0) {
						TRACE("执行命令失败:%d ret=%d\r\n", pserver->GetPack().sCmd,ret);
					}
                    pserver->CloseClient();
                    TRACE("CloseClient has done!\r\n");
                }
                
            }
            
           
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
