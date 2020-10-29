
#include "stdafx.h"
#include "CUartIO.h"
#include <exception>

CUartIO::CUartIO()
    : m_hCOMHandle(INVALID_HANDLE_VALUE)
{
}

CUartIO::~CUartIO()
{
    CloseDevice();
}

void CUartIO::CloseDevice()
{
#if 0

    try {
        CloseHandle(m_hCOMHandle);
    } catch (...) {
    }

#else

    __try {
        if (INVALID_HANDLE_VALUE != m_hCOMHandle) {
            CloseHandle(m_hCOMHandle);
            m_hCOMHandle = INVALID_HANDLE_VALUE;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

#endif
}

BOOL CUartIO::OpenDevice(CString strComNum)
{
    strComNum = _T("\\\\.\\") + strComNum;
    CloseDevice();
    m_hCOMHandle = CreateFile(strComNum.GetBuffer(strComNum.GetLength()), GENERIC_READ | GENERIC_WRITE, // ���\Ū�g
                              0,                     // ����������0
                              NULL,                  // no security attrs
                              OPEN_EXISTING,         //�]�m���ͤ覡
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,  // �ڭ̷ǳƨϥΫD�P�B�q�H
                              NULL);

    if (m_hCOMHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    } else {
        COMMTIMEOUTS CommTimeOuts ; //�w�q�W�ɵ��c�A�ö�g�ӵ��c
        memset(&CommTimeOuts, 0, sizeof(CommTimeOuts));
        CommTimeOuts.ReadTotalTimeoutConstant = 20000;//ms
        SetCommTimeouts(m_hCOMHandle, &CommTimeOuts) ;   //�]�mŪ�g�ާ@�Ҥ��\���W��
        DCB dcb;                    //�w�q��Ʊ�������c
        //memset(&dcb, 0, sizeof(dcb));
        //  Initialize the DCB structure.
        SecureZeroMemory(&dcb, sizeof(DCB));
        dcb.DCBlength = sizeof(DCB);
        GetCommState(m_hCOMHandle, &dcb) ;        //Ū��f��Ӫ��ѼƳ]�m
        dcb.BaudRate = 115200;                    //Baudrate;
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT ;
        dcb.fBinary = TRUE ;
        dcb.fParity = FALSE;
        SetCommState(m_hCOMHandle, &dcb);                   //��f�Ѽưt�m
        SetCommMask(m_hCOMHandle, EV_RXCHAR | EV_TXEMPTY);  //�]�m�ƥ��X�ʪ�����
        SetupComm(m_hCOMHandle, 1024, 128) ;             //�]�m��J,��X�w�İϪ��j�p
        PurgeComm(m_hCOMHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR
                  | PURGE_RXCLEAR);                //�M���b��J�B��X�w�İ�
    }

    return TRUE;
}

BOOL CUartIO::ReadFile(char *pcBuffer, DWORD szMaxLen, DWORD *pdwLength, DWORD dwMilliseconds)
{
    //DWORD dwStart = GetTickCount();
    BOOL success = FALSE;   // success status is used for Debug purpose
    OVERLAPPED o = {0};
    o.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!::ReadFile(m_hCOMHandle, pcBuffer, szMaxLen, pdwLength, &o)) {
        if (GetLastError() == ERROR_IO_PENDING) {
            ResetEvent(o.hEvent);

            if (WaitForSingleObject(o.hEvent, dwMilliseconds) == WAIT_OBJECT_0) {
                success = 2;
            }
        }

        success += GetOverlappedResult(m_hCOMHandle, &o, pdwLength, FALSE);
    } else {
        success = 1;
    }

    CloseHandle(o.hEvent);
    return success;
}

BOOL CUartIO::WriteFile(const char *pcBuffer, DWORD szLen, DWORD *pdwLength, DWORD dwMilliseconds)
{
    PurgeComm(m_hCOMHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR
              | PURGE_RXCLEAR);
    DWORD dwLength = 0;
    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (pdwLength != NULL) {
        *pdwLength = 0;
    }

    if (::WriteFile(m_hCOMHandle, pcBuffer, szLen, NULL, &overlapped) == FALSE) {
        if (GetLastError() == ERROR_IO_PENDING) {
            GetOverlappedResult(m_hCOMHandle, &overlapped, &dwLength, TRUE);
        }
    }

    CloseHandle(overlapped.hEvent);
    return TRUE;
}
