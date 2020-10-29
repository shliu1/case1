/*-----------------------------------------------------------------------------

    This is a part of the Microsoft Source Code Samples. 
    Copyright (C) 1995 Microsoft Corporation.
    All rights reserved. 
    This source code is only intended as a supplement to 
    Microsoft Development Tools and/or WinHelp documentation.
    See these sources for detailed information regarding the 
    Microsoft samples programs.

    MODULE: Init.c

    PURPOSE: Intializes global data and comm port connects.
                Closes comm ports and cleans up global data.

    FUNCTIONS:
        GlobalInitialize  - Init global variables and system objects
        GlobalCleanup     - cleanup global variables and system objects
        ClearTTYContents  - Clears the tty buffer
        InitNewFont       - Creates a new font for the TTY child window
        CreateTTYInfo     - Creates the dynamic tty info structure controlling
                            behavior of tty
        DestroyTTYInfo    - deallocates tty info structure
        StartThreads      - Starts worker threads when a port is opened
        SetupCommPort     - Opens the port for the first time
        WaitForThreads    - Sets the thread exit event and wait for worker
                            threads to exit
        BreakDownCommPort - Closes a connection to the comm port
        DisconnectOK      - Asks user if it is ok to disconnect

-----------------------------------------------------------------------------*/

#include <windows.h>
#include <commctrl.h>
#include "ttyinfo.h"

#define AMOUNT_TO_READ          512
#define NUM_READSTAT_HANDLES    4

/*
    Prototypes for functions called only within this file
*/
void StartThreads( void );
DWORD WaitForThreads( DWORD );

/*
    TimeoutsDefault
        We need ReadIntervalTimeout here to cause the read operations
        that we do to actually timeout and become overlapped.
        Specifying 1 here causes ReadFile to return very quickly
        so that our reader thread will continue execution.
*/
COMMTIMEOUTS gTimeoutsDefault = { 0x01, 0, 0, 0, 0 };


/*-----------------------------------------------------------------------------

FUNCTION: InitTTYInfo

PURPOSE: Initializes TTY structure

COMMENTS: This structure is a collection of TTY attributes
          used by all parts of this program

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it
            2/14/96   AllenD      Removed npTTYInfo

-----------------------------------------------------------------------------*/
BOOL InitTTYInfo()
{
    //
    // initialize generial TTY info
    //
    COMDEV( TTYInfo )        = NULL ;
    CONNECTED( TTYInfo )     = FALSE ;
    LOCALECHO( TTYInfo )     = FALSE ;
    CURSORSTATE( TTYInfo )   = CS_HIDE ;
    PORT( TTYInfo )          = 1 ;
    BAUDRATE( TTYInfo )      = 115200 ;
    BYTESIZE( TTYInfo )      = 8 ;
    PARITY( TTYInfo )        = NOPARITY ;
    STOPBITS( TTYInfo )      = ONESTOPBIT ;
    AUTOWRAP( TTYInfo )      = TRUE;
    NEWLINE( TTYInfo )       = FALSE;
    XSIZE( TTYInfo )         = 0 ;
    YSIZE( TTYInfo )         = 0 ;
    XSCROLL( TTYInfo )       = 0 ;
    YSCROLL( TTYInfo )       = 0 ;
    COLUMN( TTYInfo )        = 0 ;
    ROW( TTYInfo )           = MAXROWS - 1 ;
    DISPLAYERRORS( TTYInfo ) = TRUE ;

    //
    // timeouts
    //
    TIMEOUTSNEW( TTYInfo )   = gTimeoutsDefault;

    //
    // read state and status events
    //
    gdwReceiveState            = RECEIVE_TTY;
    EVENTFLAGS( TTYInfo )    = EVENTFLAGS_DEFAULT;
    FLAGCHAR( TTYInfo )      = FLAGCHAR_DEFAULT;

    //
    // Flow Control Settings
    //
    DTRCONTROL( TTYInfo )    = DTR_CONTROL_ENABLE;
    RTSCONTROL( TTYInfo )    = RTS_CONTROL_ENABLE;
    XONCHAR( TTYInfo )       = ASCII_XON;
    XOFFCHAR( TTYInfo )      = ASCII_XOFF;
    XONLIMIT( TTYInfo )      = 0;
    XOFFLIMIT( TTYInfo )     = 0;
    CTSOUTFLOW( TTYInfo )    = FALSE;
    DSROUTFLOW( TTYInfo )    = FALSE;
    DSRINFLOW( TTYInfo )     = FALSE;
    XONXOFFOUTFLOW(TTYInfo)  = FALSE;
    XONXOFFINFLOW(TTYInfo)   = FALSE;
    TXAFTERXOFFSENT(TTYInfo) = FALSE;

    NOREADING(TTYInfo)       = FALSE;
    NOWRITING(TTYInfo)       = FALSE;
    NOEVENTS(TTYInfo)        = FALSE;
    NOSTATUS(TTYInfo)        = FALSE;
    SHOWTIMEOUTS(TTYInfo)    = FALSE;

    //
    // setup default font information
    // 
    LFTTYFONT( TTYInfo ).lfHeight =         12 ;
    LFTTYFONT( TTYInfo ).lfWidth =          0 ;
    LFTTYFONT( TTYInfo ).lfEscapement =     0 ;
    LFTTYFONT( TTYInfo ).lfOrientation =    0 ;
    LFTTYFONT( TTYInfo ).lfWeight =         0 ;
    LFTTYFONT( TTYInfo ).lfItalic =         0 ;
    LFTTYFONT( TTYInfo ).lfUnderline =      0 ;
    LFTTYFONT( TTYInfo ).lfStrikeOut =      0 ;
    LFTTYFONT( TTYInfo ).lfCharSet =        OEM_CHARSET ;
    LFTTYFONT( TTYInfo ).lfOutPrecision =   OUT_DEFAULT_PRECIS ;
    LFTTYFONT( TTYInfo ).lfClipPrecision =  CLIP_DEFAULT_PRECIS ;
    LFTTYFONT( TTYInfo ).lfQuality =        DEFAULT_QUALITY ;
    LFTTYFONT( TTYInfo ).lfPitchAndFamily = FIXED_PITCH | FF_MODERN ;
    _tcscpy_s( LFTTYFONT( TTYInfo ).lfFaceName, 32, _T("FixedSys")) ;
    return ( TRUE ) ;
}

/*-----------------------------------------------------------------------------

FUNCTION: DestroyTTYInfo

PURPOSE: Frees objects associated with the TTYInfo structure

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it
            2/14/96   AllenD      Removed npTTYInfo

-----------------------------------------------------------------------------*/
void DestroyTTYInfo()
{
    DeleteObject(HTTYFONT(TTYInfo));
}

/*-----------------------------------------------------------------------------

FUNCTION: StartThreads

PURPOSE: Creates the Reader/Status and Writer threads

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it

-----------------------------------------------------------------------------*/
void StartThreads(void)
{
    DWORD dwReadStatId;
    DWORD dwWriterId;

    READSTATTHREAD(TTYInfo) = 
            CreateThread( NULL, 
                          0,
                          (LPTHREAD_START_ROUTINE) ReaderAndStatusProc,
                          (LPVOID) NULL, 
                          0, 
                          &dwReadStatId);

    if (READSTATTHREAD(TTYInfo) == NULL)
        ErrorInComm(_T("CreateThread(Reader/Status)"));

    return;
}

/*-----------------------------------------------------------------------------

FUNCTION: SetupCommPort( void )

PURPOSE: Setup Communication Port with our settings

RETURN: 
    Handle of comm port is successful
    NULL is error occurs

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it

-----------------------------------------------------------------------------*/
HANDLE SetupCommPort(CString strComNum)
{
   
    //
    // open communication port handle
    //
    COMDEV( TTYInfo ) = CreateFile( strComNum.GetBuffer(strComNum.GetLength()),  
                                      GENERIC_READ | GENERIC_WRITE, 
                                      0, 
                                      0, 
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                      0);

    if (COMDEV(TTYInfo) == INVALID_HANDLE_VALUE) {   
        ErrorReporter(_T("CreateFile"));
        return NULL;
    }

    //
    // Save original comm timeouts and set new ones
    //
    if (!GetCommTimeouts( COMDEV(TTYInfo), &(TIMEOUTSORIG(TTYInfo))))
        ErrorReporter(_T("GetCommTimeouts"));

    //
    // Set port state
    //
    UpdateConnection();

    //
    // set comm buffer sizes
    //
    SetupComm(COMDEV(TTYInfo), MAX_READ_BUFFER, MAX_WRITE_BUFFER);

    //
    // raise DTR
    //
    if (!EscapeCommFunction(COMDEV(TTYInfo), SETDTR))
        ErrorReporter(_T("EscapeCommFunction (SETDTR)"));

    //
    // start threads and set initial thread state to not done
    //
    StartThreads();

    //
    // set overall connect flag
    //
    CONNECTED( TTYInfo ) = TRUE ;

    return COMDEV(TTYInfo);
}

/*-----------------------------------------------------------------------------

FUNCTION: WaitForThreads(DWORD)

PURPOSE: Waits a specified time for the worker threads to exit

PARAMETERS:
    dwTimeout - milliseconds to wait until timeout

RETURN:
    WAIT_OBJECT_0 - successful wait, threads are not running
    WAIT_TIMEOUT  - at least one thread is still running
    WAIT_FAILED   - failure in WaitForMultipleObjects

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it

----------------------------------------------------------------------------*/
DWORD WaitForThreads(DWORD dwTimeout)
{	
    HANDLE hThreads[2];
    DWORD  dwRes;

    hThreads[0] = READSTATTHREAD(TTYInfo);

    //
    // set thread exit event here
    //
    SetEvent(ghThreadExitEvent);

    dwRes = WaitForMultipleObjects(1, hThreads, TRUE, dwTimeout);
    switch(dwRes)
    {
        case WAIT_OBJECT_0:
        case WAIT_OBJECT_0 + 1: 
            dwRes = WAIT_OBJECT_0;
            break;

        case WAIT_TIMEOUT:
            
            if (WaitForSingleObject(READSTATTHREAD(TTYInfo), 0) == WAIT_TIMEOUT)
                OutputDebugString(_T("Reader/Status Thread didn't exit.\n\r"));
            break;

        default:
            ErrorReporter(_T("WaitForMultipleObjects"));
            break;
    }

    //
    // reset thread exit event here
    //
    ResetEvent(ghThreadExitEvent);

    return dwRes;
}

/*-----------------------------------------------------------------------------

FUNCTION: BreakDownCommPort

PURPOSE: Closes a connection to a comm port

RETURN:
    TRUE  - successful breakdown of port
    FALSE - port isn't connected

COMMENTS: Waits for threads to exit,
          clears DTR, restores comm port timeouts, purges any i/o
          and closes all pertinent handles

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it

-----------------------------------------------------------------------------*/
BOOL BreakDownCommPort()
{
    if (!CONNECTED(TTYInfo))
        return FALSE;

    CONNECTED( TTYInfo ) = FALSE;

    //
    // wait for the threads for a small period
    //
    if (WaitForThreads(20000) != WAIT_OBJECT_0)
        /*
            if threads haven't exited, then they will
            interfere with a new connection.  I must abort
            the entire program.
        */
        ErrorHandler(_T("Error closing port."));

    //
    // lower DTR
    //
    if (!EscapeCommFunction(COMDEV(TTYInfo), CLRDTR))
        ErrorReporter(_T("EscapeCommFunction(CLRDTR)"));

    //
    // restore original comm timeouts
    //
    if (!SetCommTimeouts(COMDEV(TTYInfo),  &(TIMEOUTSORIG(TTYInfo))))
        ErrorReporter(_T("SetCommTimeouts (Restoration to original)"));

    //
    // Purge reads/writes, input buffer and output buffer
    //
    if (!PurgeComm(COMDEV(TTYInfo), PURGE_FLAGS))
        ErrorReporter(_T("PurgeComm"));

    CloseHandle(COMDEV(TTYInfo));
    CloseHandle(READSTATTHREAD(TTYInfo));


    return TRUE;
}
 
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

BOOL UpdateConnection()
{
    DCB dcb = {0};
    
    dcb.DCBlength = sizeof(dcb);

    //
    // get current DCB settings
    //
    if (!GetCommState(COMDEV(TTYInfo), &dcb))
	ErrorReporter(_T("GetCommState"));

    //
    // update DCB rate, byte size, parity, and stop bits size
    //
    dcb.BaudRate = BAUDRATE(TTYInfo);
    dcb.ByteSize = BYTESIZE(TTYInfo);
    dcb.Parity   = PARITY(TTYInfo);
    dcb.StopBits = STOPBITS(TTYInfo);

    //
    // update event flags
    //
    if (EVENTFLAGS(TTYInfo) & EV_RXFLAG)
	dcb.EvtChar = FLAGCHAR(TTYInfo);      
    else
	dcb.EvtChar = '\0';

    //
    // update flow control settings
    //
    dcb.fDtrControl     = DTRCONTROL(TTYInfo);
    dcb.fRtsControl     = RTSCONTROL(TTYInfo);

    dcb.fOutxCtsFlow    = CTSOUTFLOW(TTYInfo);
    dcb.fOutxDsrFlow    = DSROUTFLOW(TTYInfo);
    dcb.fDsrSensitivity = DSRINFLOW(TTYInfo);
    dcb.fOutX           = XONXOFFOUTFLOW(TTYInfo);
    dcb.fInX            = XONXOFFINFLOW(TTYInfo);
    dcb.fTXContinueOnXoff = TXAFTERXOFFSENT(TTYInfo);
    dcb.XonChar         = XONCHAR(TTYInfo);
    dcb.XoffChar        = XOFFCHAR(TTYInfo);
    dcb.XonLim          = XONLIMIT(TTYInfo);
    dcb.XoffLim         = XOFFLIMIT(TTYInfo);

    //
    // DCB settings not in the user's control
    //
    dcb.fParity = TRUE;

    //
    // set new state
    //
    if (!SetCommState(COMDEV(TTYInfo), &dcb))
	ErrorReporter(_T("SetCommState"));

    //
    // set new timeouts
    //
    if (!SetCommTimeouts(COMDEV(TTYInfo), &(TIMEOUTSNEW(TTYInfo))))
	ErrorReporter(_T("SetCommTimeouts"));

    return TRUE;
}
