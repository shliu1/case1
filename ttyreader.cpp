/*-----------------------------------------------------------------------------

    This is a part of the Microsoft Source Code Samples. 
    Copyright (C) 1995 Microsoft Corporation.
    All rights reserved. 
    This source code is only intended as a supplement to 
    Microsoft Development Tools and/or WinHelp documentation.
    See these sources for detailed information regarding the 
    Microsoft samples programs.

    MODULE: Reader.c

    PURPOSE: Read from comm port

    FUNCTIONS:
        OutputABufferToWindow - process incoming data destined for tty window
        OutputABufferToFile   - process incoming data destined for a file
        OutputABuffer         - called when data is read from port

-----------------------------------------------------------------------------*/

#include <windows.h>
#include <commctrl.h>

#include "ttyinfo.h"

#define AMOUNT_TO_READ          512
#define NUM_READSTAT_HANDLES    4

/*
    TimeoutsDefault
        We need ReadIntervalTimeout here to cause the read operations
        that we do to actually timeout and become overlapped.
        Specifying 1 here causes ReadFile to return very quickly
        so that our reader thread will continue execution.
*/
COMMTIMEOUTS gTimeoutsDefault = { 0x01, 0, 0, 0, 0 };

/*-----------------------------------------------------------------------------

FUNCTION: CheckModemStatus(BOOL)

PURPOSE: Check new status and possibly report it

PARAMETERS:
    bUpdateNow - if TRUE, update should be done
                 if FALSE, update is done only if status is different

COMMENTS: Reports status if bUpdateNow is true or
          new status is different from old status

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it

-----------------------------------------------------------------------------*/
void CheckModemStatus( BOOL bUpdateNow )
{
    //
    // dwOldStatus needs to be static so that it is maintained 
    // between function calls by the same thread.
    // It also needs to be __declspec(thread) so that it is 
    // initialized when a new thread is created.
    //

    __declspec(thread) static DWORD dwOldStatus = 0;

    DWORD dwNewModemStatus;

    if (!GetCommModemStatus(COMDEV(TTYInfo), &dwNewModemStatus))
        ErrorReporter(_T("GetCommModemStatus"));

    //
    // Report status if bUpdateNow is true or status has changed
    //
    if (bUpdateNow || (dwNewModemStatus != dwOldStatus)) {
        ReportModemStatus(dwNewModemStatus);
        dwOldStatus = dwNewModemStatus;
    }

    return;
}


/*-----------------------------------------------------------------------------

FUNCTION: CheckComStat(BOOL)

PURPOSE: Calls ClearCommError and reports the results.

PARAMETERS:
    bUpdateNow - If TRUE, then ReportComStat is called with new results.
                 if FALSE, then ReportComStat is called only if
                 new results differ from old.

COMMENTS: Called when the ReaderAndStatusProc times out after waiting on
          its event handles.  Also called from ReportStatusEvent.

HISTORY:   Date:      Author:     Comment:
           12/18/95   AllenD      Wrote it

-----------------------------------------------------------------------------*/
void CheckComStat(BOOL bUpdateNow)
{
    COMSTAT ComStatNew;
    DWORD dwErrors;

    __declspec(thread) static COMSTAT ComStatOld = {0};
    __declspec(thread) static DWORD dwErrorsOld = 0;

    BOOL bReport = bUpdateNow;

    if (!ClearCommError(COMDEV(TTYInfo), &dwErrors, &ComStatNew))
        AfxMessageBox(_T("ClearCommError"));

    if (dwErrors != dwErrorsOld) {
        bReport = TRUE;
        dwErrorsOld = dwErrors;
    }

    if (memcmp(&ComStatOld, &ComStatNew, sizeof(COMSTAT))) {
        bReport = TRUE;
        ComStatOld = ComStatNew;
    }
    
    if (bReport)
        ReportComStat(ComStatNew);

    ComStatOld = ComStatNew;

    return;
}

/*-----------------------------------------------------------------------------

FUNCTION: ReaderAndStatusProc(LPVOID)

PURPOSE: Thread function controls comm port reading, comm port status 
         checking, and status messages.

PARMATERS:
    lpV - 4 byte value contains the tty child window handle

RETURN: always 1

COMMENTS: Waits on various events in the applications to handle
          port reading, status checking, status messages and modem status.

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it
           11/21/95   AllenD      Incorporated status message heap

-----------------------------------------------------------------------------*/
DWORD WINAPI ReaderAndStatusProc(LPVOID lpV)
{
    OVERLAPPED osReader = {0};  // overlapped structure for read operations
    OVERLAPPED osStatus = {0};  // overlapped structure for status operations
    HANDLE     hArray[NUM_READSTAT_HANDLES];
    DWORD      dwStoredFlags = 0xFFFFFFFF;      // local copy of event flags
    DWORD      dwCommEvent;     // result from WaitCommEvent
    DWORD      dwOvRes;         // result from GetOverlappedResult
    DWORD 	   dwRead;          // bytes actually read
    DWORD      dwRes;           // result from WaitForSingleObject
    BOOL       fWaitingOnRead = FALSE;
    BOOL       fWaitingOnStat = FALSE;
    BOOL       fThreadDone = FALSE;
    char   	   lpBuf[AMOUNT_TO_READ];

    //
    // create two overlapped structures, one for read events
    // and another for status events
    //
    osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (osReader.hEvent == NULL)
        AfxMessageBox(_T("CreateEvent (Reader Event)"));

    osStatus.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (osStatus.hEvent == NULL)
        AfxMessageBox(_T("CreateEvent (Status Event)"));

    //
    // We want to detect the following events:
    //   Read events (from ReadFile)
    //   Status events (from WaitCommEvent)
    //   Status message events (from our UpdateStatus)
    //   Thread exit evetns (from our shutdown functions)
    //
    hArray[0] = osReader.hEvent;
    hArray[1] = osStatus.hEvent;
    hArray[2] = ghStatusMessageEvent;
    hArray[3] = ghThreadExitEvent;

    //
    // initial check, forces updates
    //
    CheckModemStatus(TRUE);
    CheckComStat(TRUE);

    while ( !fThreadDone ) {

        //
        // If no reading is allowed, then set flag to
        // make it look like a read is already outstanding.
        //
        if (NOREADING( TTYInfo ))
            fWaitingOnRead = TRUE;
        
        //
        // if no read is outstanding, then issue another one
        //
        if (!fWaitingOnRead) {
            if (!ReadFile(COMDEV(TTYInfo), lpBuf, AMOUNT_TO_READ, &dwRead, &osReader)) {
                if (GetLastError() != ERROR_IO_PENDING)	  // read not delayed?
                    AfxMessageBox(_T("ReadFile in ReaderAndStatusProc"));

                fWaitingOnRead = TRUE;
            }
            else {    // read completed immediately
                if ((dwRead != MAX_READ_BUFFER) && SHOWTIMEOUTS(TTYInfo))
                    AfxMessageBox(_T("Read timed out immediately.\r\n"));

//                if (dwRead)
//                    OutputABuffer(hTTY, lpBuf, dwRead);
            }
        }

        //
        // If status flags have changed, then reset comm mask.
        // This will cause a pending WaitCommEvent to complete
        // and the resultant event flag will be NULL.
        //
        if (dwStoredFlags != EVENTFLAGS(TTYInfo)) {
            dwStoredFlags = EVENTFLAGS(TTYInfo);
            if (!SetCommMask(COMDEV(TTYInfo), dwStoredFlags))
                AfxMessageBox(_T("SetCommMask"));
        }

        //
        // If event checks are not allowed, then make it look
        // like an event check operation is outstanding
        //
        if (NOEVENTS(TTYInfo))
            fWaitingOnStat = TRUE;
        //
        // if no status check is outstanding, then issue another one
        //
        if (!fWaitingOnStat) {
            if (NOEVENTS(TTYInfo))
                fWaitingOnStat = TRUE;
            else {
                if (!WaitCommEvent(COMDEV(TTYInfo), &dwCommEvent, &osStatus)) {
                    if (GetLastError() != ERROR_IO_PENDING)	  // Wait not delayed?
                        AfxMessageBox(_T("WaitCommEvent"));
                    else
                        fWaitingOnStat = TRUE;
                }
                else
                    // WaitCommEvent returned immediately
                    ReportStatusEvent(dwCommEvent);
            }
        }

        //
        // wait for pending operations to complete
        //
        if ( fWaitingOnStat && fWaitingOnRead ) {
            dwRes = WaitForMultipleObjects(NUM_READSTAT_HANDLES, hArray, FALSE, STATUS_CHECK_TIMEOUT);
            switch(dwRes)
            {
                //
                // read completed
                //
                case WAIT_OBJECT_0:
                    if (!GetOverlappedResult(COMDEV(TTYInfo), &osReader, &dwRead, FALSE)) {
                        if (GetLastError() == ERROR_OPERATION_ABORTED)
                            AfxMessageBox(_T("Read aborted\r\n"));
                        else
                            AfxMessageBox(_T("GetOverlappedResult (in Reader)"));
                    }
                    else {      // read completed successfully
                        if ((dwRead != MAX_READ_BUFFER) && SHOWTIMEOUTS(TTYInfo))
                            AfxMessageBox(_T("Read timed out overlapped.\r\n"));

 //                       if (dwRead)
 //                           OutputABuffer(hTTY, lpBuf, dwRead);
                    }

                    fWaitingOnRead = FALSE;
                    break;

                //
                // status completed
                //
                case WAIT_OBJECT_0 + 1: 
                    if (!GetOverlappedResult(COMDEV(TTYInfo), &osStatus, &dwOvRes, FALSE)) {
                        if (GetLastError() == ERROR_OPERATION_ABORTED)
                            AfxMessageBox(_T("WaitCommEvent aborted\r\n"));
                        else
                            AfxMessageBox(_T("GetOverlappedResult (in Reader)"));
                    }
                    else       // status check completed successfully
                        ReportStatusEvent(dwCommEvent);

                    fWaitingOnStat = FALSE;
                    break;

                //
                // status message event
                //
                case WAIT_OBJECT_0 + 2:
                    //StatusMessage();
                    break;

                //
                // thread exit event
                //
                case WAIT_OBJECT_0 + 3:
                    fThreadDone = TRUE;
                    break;

                case WAIT_TIMEOUT:
                    //
                    // timeouts are not reported because they happen too often
                    // OutputDebugString("Timeout in Reader & Status checking\n\r");
                    //

                    //
                    // if status checks are not allowed, then don't issue the
                    // modem status check nor the com stat check
                    //
                    if (!NOSTATUS(TTYInfo)) {
                        CheckModemStatus(FALSE);    // take this opportunity to do
                        CheckComStat(FALSE);        //   a modem status check and
                                                    //   a comm status check
                    }

                    break;                       

                default:
                    AfxMessageBox(_T("WaitForMultipleObjects(Reader & Status handles)"));
                    break;
            }
        }
    }

    //
    // close event handles
    //
    CloseHandle(osReader.hEvent);
    CloseHandle(osStatus.hEvent);

    return 1;
}
