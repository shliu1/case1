
/*-----------------------------------------------------------------------------
    This is a part of the Microsoft Source Code Samples. 
    Copyright (C) 1995 Microsoft Corporation.
    All rights reserved. 
    This source code is only intended as a supplement to 
    Microsoft Development Tools and/or WinHelp documentation.
    See these sources for detailed information regarding the 
    Microsoft samples programs.

    MODULE: TTYINFO.h

    PURPOSE: Contains global definitions for the TTYINFO structure

-----------------------------------------------------------------------------*/

#define AMOUNT_TO_READ          512
#define NUM_READSTAT_HANDLES    3

//
// constant definitions
//

//
// hard coded maximum number of ports
//
#define MAXPORTS        44

//
// terminal size
//
#define MAXROWS         50
#define MAXCOLS         80

//
// cursor states
//
#define CS_HIDE         0x00
#define CS_SHOW         0x01

//
// ascii definitions
//
#define ASCII_BEL       0x07
#define ASCII_BS        0x08
#define ASCII_LF        0x0A
#define ASCII_CR        0x0D
#define ASCII_XON       0x11
#define ASCII_XOFF      0x13

//
// data structures
//
struct TTYInfoStruct
{
    HANDLE  hCommPort, hReaderStatus, hWriter ;
    DWORD   dwEventFlags;
    TCHAR   Screen[MAXCOLS * MAXROWS];
    CHAR    chFlag, chXON, chXOFF;
    WORD    wXONLimit, wXOFFLimit;
    DWORD   fRtsControl;
    DWORD   fDtrControl;
    BOOL    fConnected, fTransferring, fRepeating,
            fLocalEcho, fNewLine, fCR2CRLF,
            fDisplayErrors, fAutowrap,
            fCTSOutFlow, fDSROutFlow, fDSRInFlow, 
            fXonXoffOutFlow, fXonXoffInFlow,
            fTXafterXoffSent,
            fNoReading, fNoWriting, fNoEvents, fNoStatus,
            fDisplayTimeouts;
    BYTE    bPort, bByteSize, bParity, bStopBits ;
    DWORD   dwBaudRate ;
    WORD    wCursorState ;
    HFONT   hTTYFont ;
    LOGFONT lfTTYFont ;
    DWORD   rgbFGColor ;
    COMMTIMEOUTS timeoutsorig;
    COMMTIMEOUTS timeoutsnew;
    int     xSize, ySize, xScroll, yScroll, xOffset, yOffset,
            nColumn, nRow, xChar, yChar , nCharPos;

} TTYInfo;

//
// macros ( for easier readability )
//
#define COMDEV( x )         (x.hCommPort)
#define CURSORSTATE( x )    (x.wCursorState)
#define PORT( x )           (x.bPort)
#define SCREEN( x )         (x.Screen)
#define CONNECTED( x )      (x.fConnected)
#define TRANSFERRING( x )   (x.fTransferring)
#define REPEATING( x )      (x.fRepeating)
#define LOCALECHO( x )      (x.fLocalEcho)
#define NEWLINE( x )        (x.fNewLine)
#define CR2CRLF( x )        (x.fCR2CRLF)
#define AUTOWRAP( x )       (x.fAutowrap)
#define BYTESIZE( x )       (x.bByteSize)
#define PARITY( x )         (x.bParity)
#define STOPBITS( x )       (x.bStopBits)
#define BAUDRATE( x )       (x.dwBaudRate)
#define HTTYFONT( x )       (x.hTTYFont)
#define LFTTYFONT( x )      (x.lfTTYFont)
#define FGCOLOR( x )        (x.rgbFGColor)
#define XSIZE( x )          (x.xSize)
#define YSIZE( x )          (x.ySize)
#define XSCROLL( x )        (x.xScroll)
#define YSCROLL( x )        (x.yScroll)
#define XOFFSET( x )        (x.xOffset)
#define YOFFSET( x )        (x.yOffset)
#define COLUMN( x )         (x.nColumn)
#define ROW( x )            (x.nRow)
#define XCHAR( x )          (x.xChar)
#define YCHAR( x )          (x.yChar)
#define DISPLAYERRORS( x )  (x.fDisplayErrors)
#define TIMEOUTSORIG( x )   (x.timeoutsorig)
#define TIMEOUTSNEW( x )    (x.timeoutsnew)
#define WRITERTHREAD( x )   (x.hWriter)
#define READSTATTHREAD( x ) (x.hReaderStatus)
#define EVENTFLAGS( x )     (x.dwEventFlags)
#define FLAGCHAR( x )       (x.chFlag)
#define SCREENCHAR( x, col, row )   (x.Screen[row * MAXCOLS + col])

#define DTRCONTROL( x )     (x.fDtrControl)
#define RTSCONTROL( x )     (x.fRtsControl)
#define XONCHAR( x )        (x.chXON)
#define XOFFCHAR( x )       (x.chXOFF)
#define XONLIMIT( x )       (x.wXONLimit)
#define XOFFLIMIT( x )      (x.wXOFFLimit)
#define CTSOUTFLOW( x )     (x.fCTSOutFlow)
#define DSROUTFLOW( x )     (x.fDSROutFlow)
#define DSRINFLOW( x )      (x.fDSRInFlow)
#define XONXOFFOUTFLOW( x ) (x.fXonXoffOutFlow)
#define XONXOFFINFLOW( x )  (x.fXonXoffInFlow)
#define TXAFTERXOFFSENT(x)  (x.fTXafterXoffSent)

#define NOREADING( x )      (x.fNoReading)
#define NOWRITING( x )      (x.fNoWriting)
#define NOEVENTS( x )       (x.fNoEvents)
#define NOSTATUS( x )       (x.fNoStatus)
#define SHOWTIMEOUTS( x )   (x.fDisplayTimeouts)

//---------------------------------------------------------------------------
//  End of File: ttyinfo.h
//---------------------------------------------------------------------------

//
// GLOBAL DEFINES
//
#define TTY_BUFFER_SIZE         MAXROWS * MAXCOLS
#define MAX_STATUS_BUFFER       20000
#define MAX_WRITE_BUFFER        1024
#define MAX_READ_BUFFER         2048
#define READ_TIMEOUT            500
#define STATUS_CHECK_TIMEOUT    500
#define WRITE_CHECK_TIMEOUT     500
#define PURGE_FLAGS             PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR 
#define EVENTFLAGS_DEFAULT      EV_BREAK | EV_CTS | EV_DSR | EV_ERR | EV_RING | EV_RLSD
#define FLAGCHAR_DEFAULT        '\n'

//
// Write request types
//
#define WRITE_CHAR          0x01
#define WRITE_FILE          0x02
#define WRITE_FILESTART     0x03
#define WRITE_FILEEND       0x04
#define WRITE_ABORT         0x05
#define WRITE_BLOCK         0x06

//
// Read states
//
#define RECEIVE_TTY         0x01
#define RECEIVE_CAPTURED    0x02

//
// window coords
//
#define MAXXWINDOW          750
#define MAXYWINDOW          530
#define STARTXWINDOW        80
#define STARTYWINDOW        70

#define SETTINGSFACTOR      6
#define STATUSFACTOR        5

//
// window timer ids
// 
#define TIMERID             1

//
// GLOBAL VARIABLES
//
#ifdef CAN_USE_GETVERSIONINFOEX
//OSVERSIONINFO gOSV;
#endif


//
HANDLE ghThreadExitEvent;

COMMTIMEOUTS gTimeoutsDefault = { 0x01, 0, 0, 0, 0 };
DWORD  gdwReceiveState;