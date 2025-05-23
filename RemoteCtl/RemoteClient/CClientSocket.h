#pragma once
#include"pch.h"
#include "framework.h"
#include<iostream>
#include<vector>

#pragma pack(push)
#pragma pack(1)
void Dump(BYTE* pData, size_t nSize);

class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(const CPacket& pak) {
		sHead = pak.sHead;
		nLength = pak.nLength;
		sCmd = pak.sCmd;
		strData = pak.strData;
		sSum = pak.sSum;
	}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sSum += BYTE(strData[j]) & 0xFF;
		}

	}
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		for (; i < nSize; ++i) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 8 > nSize) {// 包数据可能不全，包头未能全部接收到
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) {// 包未完全接受到
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 4);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			//TRACE("%s\r\n",strData.c_str()+ 12);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i;
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	CPacket& operator = (const CPacket& pak) {
		if (this != &pak) {
			sHead = pak.sHead;
			nLength = pak.nLength;
			sCmd = pak.sCmd;
			strData = pak.strData;
			sSum = pak.sSum;
		}
		return *this;
	}
	int Size() {
		return nLength + 6;
	}
	const char* Data(std::string& strOut) const {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;

		return strOut.c_str();
	}

public:
	WORD sHead;//固定位 FE FF
	DWORD nLength;// 包长度 （从控制命令开始，到和校验结束）
	WORD sCmd; // 控制命令
	std::string strData; // 包数据
	WORD sSum;// 和校验
	//std::string strOut;//整个包的数据
};
#pragma pack(pop)

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//点击，移动，双击
	WORD nButton;//左键、右键、滚轮
	POINT ptXY;//坐标

} MOUSEEV, * PMOUSEEV;

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFilename, 0, 256);
	}
	BOOL IsInvalid;//是否有效
	BOOL HasNext;// 是否还有后续 0 没有 1 有
	BOOL IsDirectory; //是否为目录 0 否 1 是
	char szFilename[256];//文件名

}FILEINFO, * PFILEINFO;

std::string GetErrInfo(int wsaErrCode);

class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		if (m_instance == NULL) { // 静态函数没有this指针，所以无法直接访问成员变量 
			m_instance = new CClientSocket();
		}
		return m_instance;
	}
	bool InitSocket() {
		if (m_sock != INVALID_SOCKET) CloseSocket();
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (m_sock == -1) {
			return false;
		}
		//TODO:校验
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		//TRACE("inet_Addr: %08X  nIP: %08X\r\n", inet_addr("127.0.0.1"), nIP);
		serv_addr.sin_addr.s_addr = htonl(m_nIP);
		serv_addr.sin_port = htons(m_nPort);
		if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox("指定的IP地址不存在！");
			return false;
		}

		if (connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
			AfxMessageBox("连接失败");
			TRACE("连接失败：%d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;
	}
#define BUFFER_SIZE 2048000
	int DealCommand() {
		if (m_sock == -1) {
			return -1;
		}
		char* buffer = m_buffer.data();
		static size_t index = 0;
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if ((len <= 0) && (index  <= 0)) return -1;
			//Dump((BYTE*)buffer, index);
			index += len;
			len = index;
			m_pack = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, index - len);
				index -= len;
				return m_pack.sCmd;
			}
		}
		//TODO:处理命令 
		//return len;
		return -1;
	}

	bool Send(const char* pData, int nSize) {
		if (m_sock == -1) {
			return false;
		}
		return send(m_sock, pData, nSize, 0) > 0;
	}

	bool Send(const CPacket& pak) {
		if (m_sock == -1) return false;
		TRACE("m_sock = %d\r\n", m_sock);
		std::string strOut;
		pak.Data(strOut);
		return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
	}
	bool GetFilePath(std::string& strPath) {
		if ((m_pack.sCmd == 2) || (m_pack.sCmd == 3) || (m_pack.sCmd == 4)) {
			strPath = m_pack.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_pack.sCmd == 5) {
			memcpy(&mouse, m_pack.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}
	CPacket& GetPack() {
		return m_pack;
	}
	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	void UpdateAddress(int nIP, int nPort) {
		m_nIP = nIP;
		m_nPort = nPort;
	}

private:
	int m_nIP;//地址
	int m_nPort;//端口
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_pack;
	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss)
	{
		m_sock = ss.m_sock;
		m_nIP = ss.m_nIP;
		m_nPort = ss.m_nPort;
	}
	CClientSocket() :m_nIP(INADDR_ANY), m_nPort(0)
	{
		m_sock = -1;
		if (InitSocketEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置!"), _T("初始化错误!"), MB_OK | MB_ICONERROR);
			exit(0);
		}
	//	m_sock = socket(PF_INET, SOCK_STREAM, 0);
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);
	}
	~CClientSocket() {
		closesocket(m_sock);
		WSACleanup();
	}
	BOOL InitSocketEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			// TODO ：返回值处理
			return FALSE;
		}
		return TRUE;
	}
	static void releaseInstance() {
		if (m_instance != NULL) {
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CClientSocket* m_instance;

	class CHelper {
	public:
		CHelper() {
			CClientSocket::getInstance();
		}
		~CHelper() {
			CClientSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};

//extern CServerSocket server;