#pragma once
#include "resource.h"
#include<map>
#include "ServerSocket.h"
#include <atlimage.h>
#include <direct.h>
#include "PracticeTool.h"
#include<stdio.h>
#include<io.h>
#include<list>
#include "LockInfoDialog.h"


class CCommand
{
public:
	CCommand();
	int ExecuteCommand(int nCmd);
protected:
	typedef int(CCommand::* CMDFUNC)();//��Ա����ָ��
	std::map<int, CMDFUNC> m_mapFunction;//������ŵ����ܵ�ӳ��
    CLockInfoDialog dlg;
    unsigned threadid;
protected:
    static unsigned _stdcall threadLockDlg(void* arg) {
        CCommand* thiz = (CCommand*)arg;
        thiz->threadLockDlgMain();
        _endthreadex(0);
        return 0;
    }
    void threadLockDlgMain() {
        dlg.Create(IDD_DIALOG_INFO, NULL);
        dlg.ShowWindow(SW_SHOW);
        //�ڱκ�̨����
        CRect rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
        rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
        rect.bottom = LONG(rect.bottom * 1.10);
        dlg.MoveWindow(rect);
        CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
        if (pText) {
            CRect rtText;
            pText->GetWindowRect(rtText);
            int nWidth = rtText.Width();
            int x = (rect.right - nWidth) / 2;
            int nHeight = rtText.Height();
            int y = (rect.bottom - nHeight) / 2;
            pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
        }
        //�����ö�
        dlg.SetWindowPos(&dlg.wndNoTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        //������깦��
        ShowCursor(false);
        //����������
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);
        dlg.GetWindowRect(rect);
        rect.left = 0;
        rect.top = 0;
        rect.right = 1;
        rect.bottom = 1;
        //rect.right = rect.left + 1;
        //rect.bottom = rect.top + 1;
        ClipCursor(rect);//���������Χ
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_KEYDOWN) {
                TRACE("msg:%08X wparam:%08x lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
                if (msg.wParam == 0x20) { //���ո��˳�
                    break;
                }
            }
        }
        ClipCursor(NULL);
        //�ָ�������
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
        dlg.DestroyWindow();
        //�ָ����
        ShowCursor(true);
    }

    int MakeDriverInfo() {
        std::string result;
        for (int i = 1; i <= 26; ++i) {
            if (_chdrive(i) == 0) {
                if (result.size() > 0)
                    result += ',';
                result += 'A' + i - 1;
            }
        }
        CPacket pak(1, (BYTE*)result.c_str(), result.size());//���
        CPracticeTool::Dump((BYTE*)pak.Data(), pak.Size());
        CServerSocket::getInstance()->Send(pak);
        return 0;
    }

    int MakeDirectoryInfo() {
        std::string strPath;
        //std::list<FILEINFO> lstFileInfos;
        if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
            OutputDebugString(_T("��ǰ��������ǻ�ȡ�ļ��б������������"));
            return -1;
        }
        if (_chdir(strPath.c_str()) != 0) {
            FILEINFO finfo;
            finfo.HasNext = FALSE;
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            CServerSocket::getInstance()->Send(pack);
            OutputDebugString(_T("û��Ȩ�޷���Ŀ¼����\n"));
            TRACE("strPath: (%s)\r\n", strPath.c_str());
            return -2;
        }
        _finddata_t fdata;
        intptr_t hfind = 0;
        if ((hfind = _findfirst("*", &fdata)) == -1) {
            OutputDebugString(_T("û���ҵ��κ��ļ�����"));
            FILEINFO finfo;
            finfo.HasNext = FALSE;
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            CServerSocket::getInstance()->Send(pack);
            return 0;
        }
        int count = 0;
        do {
            FILEINFO finfo;
            finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
            //       memcpy(finfo.szFilename, fdata.name, strlen(fdata.name));
            strcpy_s(finfo.szFilename, sizeof(finfo.szFilename), fdata.name);
            //        strncpy_s(finfo.szFilename, fdata.name, sizeof(finfo.szFilename) - 1);
             //       finfo.szFilename[sizeof(finfo.szFilename) - 1] = '\0';  // �ֶ�����ַ���������
            TRACE("%s\r\n", finfo.szFilename);
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            CServerSocket::getInstance()->Send(pack);
            count++;
            //        TRACE("send cnt = %d\r\n",count);
        } while (!_findnext(hfind, &fdata));
        TRACE("server:count = %d\r\n", count);
        _findclose(hfind);
        //������Ϣ�����ƶ�
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int RunFile() {
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        CPacket pack(2, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int DownLoadFile() {
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        long long data = 0;
        FILE* pFile = NULL;
        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
        if (err != 0 || (pFile == NULL)) {
            CPacket pack(4, (BYTE*)&data, 8);
            CServerSocket::getInstance()->Send(pack);
            return -1;
        }
        if (pFile != NULL) {
            fseek(pFile, 0, SEEK_END);
            data = _ftelli64(pFile);
            CPacket head(2, (BYTE*)&data, 8);
            CServerSocket::getInstance()->Send(head);
            fseek(pFile, 0, SEEK_SET);
            char buffer[1024] = "";
            size_t rlen = 0;
            do {
                rlen = fread(buffer, 1, 1024, pFile);
                CPacket pack(4, (BYTE*)buffer, rlen);
                CServerSocket::getInstance()->Send(pack);
            } while (rlen >= 1024);
            fclose(pFile);
        }
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);

        return 0;
    }

    int SendScreen() {
        CImage screen;
        HDC hScreen = ::GetDC(NULL);
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);;
        int nWidth = GetDeviceCaps(hScreen, HORZRES);
        int nHeight = GetDeviceCaps(hScreen, VERTRES);
        screen.Create(nWidth, nHeight, nBitPerPixel);
        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
        ReleaseDC(NULL, hScreen);
        // DWORD tick = GetTickCount();
         //screen.Save(_T("test2020.png"), Gdiplus::ImageFormatJPEG);
         //TRACE("png %d\n", GetTickCount() - tick);
         //tick = GetTickCount();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
        if (hMem == NULL) return -1;
        IStream* pStream = NULL;
        HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
        if (ret == S_OK) {
            screen.Save(pStream, Gdiplus::ImageFormatJPEG);
            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);
            PBYTE pData = (PBYTE)(GlobalLock(hMem));
            SIZE_T nSize = GlobalSize(hMem);
            CPacket pack(6, pData, nSize);
            GlobalUnlock(hMem);
            CServerSocket::getInstance()->Send(pack);
        }

        /*screen.Save(_T("test2020.jpg"), Gdiplus::ImageFormatJPEG);*/
        //TRACE("jpg %d\n", GetTickCount() - tick);
        pStream->Release();
        GlobalFree(hMem);
        screen.ReleaseDC();
        return 0;
    }

    int LockMachine() {
        if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
            //  _beginthread(threadLockDlg, 0, NULL);
            _beginthreadex(NULL, 0, CCommand::threadLockDlg, this, 0, &threadid);
        }
        CPacket pack(7, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int UnlockMachine() {
        //::SendMessage(dlg.m_hWnd,WM_KEYDOWN, 0x20, 0x00390001);
        PostThreadMessage(threadid, WM_KEYDOWN, 0x20, 0x00390001);
        CPacket pack(8, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int TestConnect() {
        CPacket pack(1981, NULL, 0);
        bool ret = CServerSocket::getInstance()->Send(pack);
        TRACE("Send ret = %d\r\n", ret);
        return 0;
    }

    int DeleteLocalFile() {
        //TODO:
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        TCHAR sPath[MAX_PATH] = _T("");
        //mbstowcs(sPath, strPath.c_str(), strPath.size());// ������������
        MultiByteToWideChar(CP_ACP, 0,
            strPath.c_str(), strPath.size(),
            sPath, sizeof(sPath) / sizeof(TCHAR));
        DeleteFile(sPath);
        CPacket pack(9, NULL, 0);
        bool ret = CServerSocket::getInstance()->Send(pack);
        TRACE("Send ret = %d\r\n", ret);
        return 0;
    }



    int MouseEvent() {
        MOUSEEV mouse;
        if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {

            DWORD nFlags = 0;
            switch (mouse.nButton) {
            case 0:
                nFlags = 1;//���
                break;
            case 1:
                nFlags = 2;//�Ҽ�
                break;
            case 2:
                nFlags = 4;//�м�
                break;
            case 4:
                nFlags = 8;
                break;
            }
            if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
            switch (mouse.nAction) {
            case 0:
                nFlags |= 0x10;//����
                break;
            case 1:
                nFlags |= 0x20;//˫��
                break;
            case 2:
                nFlags |= 0x40;//����
                break;
            case 3:
                nFlags |= 0x80;//�ſ�
                break;
            default:
                break;
            }
            TRACE("mouse event = %08X x:%d y:%d--------------------\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
            switch (nFlags) {
            case 0x21://���˫��
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x11://�������
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x41://�������
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x81://����ſ�
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x22://�Ҽ�˫��
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x12://�Ҽ�����
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x42://�Ҽ�����
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x82://�Ҽ��ſ�
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x24://�м�˫��
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x14://�м䵥��
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x44://�м�����
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x84://�м��ſ�
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x08://����ƶ�
                mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
                break;
            }
            CPacket pak(5, NULL, 0);
            CServerSocket::getInstance()->Send(pak);
        }
        else {
            OutputDebugString(_T("��ȡ����������ʧ�ܣ���"));
            return -1;
        }
        return 0;
    }



};

