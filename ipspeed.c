#define TCPV40HDRS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <types.h>
#include <netinet\in.h>
#include <sys\socket.h>
#include <sys\ioctl.h>
#include <net\route.h>
#include <net\if.h>
#include <net\if_arp.h>
#define INCL_DOSMISC
#define INCL_DOSPROFILE
#define INCL_GPI
#define INCL_PM
#include <os2.h>
// #include <bsedos.h>
// #include <pmwin.h>
#include <utility.h>
#include "ipspeed.h"
#ifdef USE16BIT
#include "tcpipdll.h"
#endif

#define MAX_VALUES      35
#define DEF_EST_COUNT   3
#define RESTORE_WINDOW  (WM_USER + 1)

DEFTYPE(INTERFACE, {
    char Descr[45];
    short Type;
    dword Nominal;
    QWORD Time[MAX_VALUES];
    dword In[MAX_VALUES];
    dword InDiff[MAX_VALUES];
    dword Out[MAX_VALUES];
    dword OutDiff[MAX_VALUES];
    int Curr;
    int NormalEst;
    dword TotalIn;
    dword TotalOut;    
    dword SubTotalIn;
    dword SubTotalOut;
    dword EstIn;
    dword EstOut;
    dword MaxIn;
    dword MaxOut;    
    });
    
    
MRESULT EXPENTRY MainDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY AboutDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
void HideFrameControls(HWND hwndFrame, int AdjustSize);
void ShowFrameControls(HWND hwndFrame, int AdjustSize);
HSWITCH MakeMainWindow(HWND hwnd, PSZ Title, HPOINTER Icon);
MRESULT EXPENTRY NewFrameProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY NewDiagProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY NewTextProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

HAB hab;
int ControlsHidden = FALSE;
int AlwaysTop = FALSE;
HWND hwndTitleBar, hwndSysMenu, hwndMinMax;
PFNWP OldFrameProc;
PFNWP OldDiagProc;
PFNWP OldTextProc;

int OurSocket;
struct ifmib InterfaceData;
INTERFACE Interface[IFMIB_ENTRIES];
int ActiveIndex[IFMIB_ENTRIES];
int ActiveInterfaces;
int CurrInterface = -1;
int ShowMode = CMD_SHOWALL;
int Smoothed = FALSE;
int EstCount = DEF_EST_COUNT;
char *CmdInterface = NULL;

ULONG TimerFreq = 0;
int NormalTimer = FALSE;

void InitTimer(void)
{
    APIRET Result;

    Result = DosTmrQueryFreq(&TimerFreq);
    
    if ( Result ) {
        NormalTimer = TRUE;
        TimerFreq = 1000;
        }
    else {
        NormalTimer = FALSE;
        }

    return;
}

void GetTimer(PQWORD CurrTime)
{
    APIRET Result;

    if ( NormalTimer ) {
        CurrTime->ulHi = 0;
        Result = DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &(CurrTime->ulLo), sizeof(ULONG));        
        }
    else {
        Result = DosTmrQueryTime(CurrTime);
        }

    return;
}

long double DiffTime(PQWORD LastTime, PQWORD NewTime)
{
    ULONG LoDiff = NewTime->ulLo - LastTime->ulLo;
    ULONG HiDiff = NewTime->ulHi - LastTime->ulHi;
    
    if ( NewTime->ulLo < LastTime->ulLo )
        HiDiff--;

    return (((long double)HiDiff * 65536.0L * 65536.0L) + (long double)LoDiff) / 
           (long double)TimerFreq;
}

int AddInterfaceData(void)
{
    int iResult, i;
    APIRET Result;
    QWORD CurrTime;
    long double TimeDiff;
    int Last, InterfaceCount, InterfacesChanged;
    
    GetTimer(&CurrTime);
        
#ifdef USE16BIT
    iResult = _ioctl(OurSocket, SIOSTATIF, &InterfaceData, sizeof(InterfaceData));
#else
    iResult = ioctl(OurSocket, SIOSTATIF, (caddr_t)&InterfaceData, sizeof(InterfaceData));
#endif    
    
    InterfaceCount = 0;
    InterfacesChanged = FALSE;
    
    for ( i = 0; i < IFMIB_ENTRIES; i++ ) {
        if ( InterfaceData.iftable[i].ifType != 0 && 
             InterfaceData.iftable[i].ifDescr[0] != 0 ) {
            ActiveIndex[InterfaceCount++] = i;
            
            if ( Interface[i].Type != InterfaceData.iftable[i].ifType ||
                 0 != memcmp(Interface[i].Descr, InterfaceData.iftable[i].ifDescr, 45) ||
                 InterfaceData.iftable[i].ifInOctets < Interface[i].TotalIn ||
                 InterfaceData.iftable[i].ifOutOctets < Interface[i].TotalOut ) {
                memset(&(Interface[i]), 0, sizeof(INTERFACE));
                Interface[i].Type = InterfaceData.iftable[i].ifType;
                Interface[i].Nominal = InterfaceData.iftable[i].ifSpeed;
                memcpy(Interface[i].Descr, InterfaceData.iftable[i].ifDescr, 45);
                InterfacesChanged = TRUE;
                }
            
            Interface[i].Time[Interface[i].Curr] = CurrTime;
            Interface[i].TotalIn =
            Interface[i].In[Interface[i].Curr] = InterfaceData.iftable[i].ifInOctets;
            Interface[i].TotalOut =
            Interface[i].Out[Interface[i].Curr] = InterfaceData.iftable[i].ifOutOctets;

            if ( ! Interface[i].NormalEst ) {
                if ( Interface[i].Curr >= EstCount ) {
                    Interface[i].NormalEst = TRUE;
                    Last = Interface[i].Curr - EstCount;
                    }
                else {
                    Last = 0;
                    }
                }
            else {
                Last = ( Interface[i].Curr + ( MAX_VALUES - EstCount ) ) % MAX_VALUES;
                }

            TimeDiff = DiffTime(&(Interface[i].Time[Last]), &(Interface[i].Time[Interface[i].Curr]));                
            
            if ( TimeDiff != 0 ) {
                Interface[i].EstIn = 
                    ( Interface[i].In[Interface[i].Curr] - Interface[i].In[Last] ) / TimeDiff;
                        
                Interface[i].EstOut = 
                    ( Interface[i].Out[Interface[i].Curr] - Interface[i].Out[Last] ) / TimeDiff;
                        
                if ( Interface[i].EstIn > Interface[i].MaxIn )
                    Interface[i].MaxIn = Interface[i].EstIn;
                    
                if ( Interface[i].EstOut > Interface[i].MaxOut )
                    Interface[i].MaxOut = Interface[i].EstOut;
                }

            if ( Interface[i].Curr == 0 ) {
                if ( Interface[i].NormalEst )
                    Last = MAX_VALUES - 1;
                else
                    Last = 0;
                }
            else {
                Last = Interface[i].Curr - 1;
                }

            TimeDiff = DiffTime(&(Interface[i].Time[Last]), &(Interface[i].Time[Interface[i].Curr]));                
            
            if ( TimeDiff != 0 ) {
                Interface[i].InDiff[Interface[i].Curr] = 
                    ( Interface[i].In[Interface[i].Curr] - Interface[i].In[Last] ) / TimeDiff;
                        
                Interface[i].OutDiff[Interface[i].Curr] = 
                    ( Interface[i].Out[Interface[i].Curr] - Interface[i].Out[Last] ) / TimeDiff;                        
                }
            
            Interface[i].Curr++;
            if ( Interface[i].Curr == MAX_VALUES )
                Interface[i].Curr = 0;                    
            }
        else {
            if ( Interface[i].Type != 0 && 
                 Interface[i].Descr[0] != 0 ) {
                memset(&(Interface[i]), 0, sizeof(INTERFACE));
                InterfacesChanged = TRUE;
                }
            }
        }
    
    ActiveInterfaces = InterfaceCount;
    
    return InterfacesChanged;
}

int main(int argc, char **argv)
{
    HMQ hmq;
    HWND hwndDlg;
    BOOL bResult;
    QMSG Msg;    

    if ( argc >= 2 ) {
        CmdInterface = argv[1];
        }
    
#ifdef USE16BIT
    _sock_init();
#else
    sock_init();
#endif    
    
#ifdef USE16BIT
    OurSocket = _socket(PF_INET, SOCK_STREAM, 0);
#else
    OurSocket = socket(PF_INET, SOCK_STREAM, 0);
#endif    
    if ( OurSocket < 0 ) {
        return 1;
        }

    InitTimer();
    
    hab = WinInitialize(0);
    if ( hab == NULLHANDLE )
        return 2;

    hmq = WinCreateMsgQueue(hab, 0);
    if ( hmq == NULLHANDLE )
        return 2;

        
    hwndDlg = WinLoadDlg(HWND_DESKTOP, HWND_DESKTOP, (PFNWP)MainDlgProc,
                (HMODULE)NULL, DLG_IPSPEED, NULL);

    if ( hwndDlg == NULLHANDLE ) {
        return 2;
        } 

    while ( WinGetMsg(hab, &Msg, NULLHANDLE, 0, 0) ) {
        WinDispatchMsg(hab, &Msg);
        }

    bResult = WinDestroyWindow(hwndDlg);
    bResult = WinDestroyMsgQueue(hmq);
    bResult = WinTerminate(hab);
    
#ifdef USE16BIT
    _soclose(OurSocket);
#else
    soclose(OurSocket);
#endif    
 
    return 0;
}

void DrawDiag(HWND DiagWindow, int Update)
{
    static int RectDone = FALSE;
    static RECTL DiagRect;    
    static POINTL Points[MAX_VALUES];
    static int LastDrawZero = FALSE;
    LONG BackColor = CLR_PALEGRAY;
    int i;
    HPS hps;
    dword TopValue;
    
    if ( ! WinIsWindowShowing(DiagWindow) )
        return;
    
    if ( ! RectDone ) {
        WinQueryWindowRect(DiagWindow, &DiagRect);        
        DiagRect.xLeft++;
        DiagRect.yBottom++;
        DiagRect.xRight--;
        DiagRect.yTop--;
        for ( i = 0; i < MAX_VALUES; i++ ) 
            Points[i].x = DiagRect.xLeft + 
                (( DiagRect.xRight - DiagRect.xLeft - 1 ) * i ) / (MAX_VALUES - 1);
                      
        RectDone = TRUE;        
        }

    /*
    if ( 0 == WinQueryPresParam(WinQueryWindow(DiagWindow, QW_PARENT), 
         PP_BACKGROUNDCOLORINDEX, 0, NULL, sizeof(LONG), &BackColor, 0) )
        BackColor = CLR_BLACK;
    */

    hps = WinGetPS(DiagWindow);
    
    if ( CurrInterface == -1 ) {
        WinFillRect(hps, &DiagRect, BackColor);
        WinReleasePS(hps);
        LastDrawZero = FALSE;
        return;
        }

    TopValue = 0;
    for ( i = 0; i < MAX_VALUES; i++ ) {
        if ( TopValue < Interface[ActiveIndex[CurrInterface]].InDiff[i] ) 
            TopValue = Interface[ActiveIndex[CurrInterface]].InDiff[i];
        if ( TopValue < Interface[ActiveIndex[CurrInterface]].OutDiff[i] ) 
            TopValue = Interface[ActiveIndex[CurrInterface]].OutDiff[i];
        }
    
    if ( TopValue > 0 ) {
        WinFillRect(hps, &DiagRect, BackColor);
        
        for ( i = 0; i < MAX_VALUES; i++ ) {
            Points[i].y = DiagRect.yBottom +
                (Interface[ActiveIndex[CurrInterface]].OutDiff[(i + Interface[ActiveIndex[CurrInterface]].Curr) % MAX_VALUES] * 
                (DiagRect.yTop - DiagRect.yBottom - 1))  / TopValue;
            }

        GpiSetColor(hps, CLR_DARKPINK);
        GpiMove(hps, Points);
        if ( Smoothed )
            GpiPolyFillet(hps, MAX_VALUES-1, Points+1);
        else
            GpiPolyLine(hps, MAX_VALUES-1, Points+1);
        
        for ( i = 0; i < MAX_VALUES; i++ ) {
            Points[i].y = DiagRect.yBottom +
                (Interface[ActiveIndex[CurrInterface]].InDiff[(i + Interface[ActiveIndex[CurrInterface]].Curr) % MAX_VALUES] * 
                (DiagRect.yTop - DiagRect.yBottom - 1))  / TopValue;
            }

        GpiSetColor(hps, CLR_BLUE);
        GpiMove(hps, Points);
        if ( Smoothed )
            GpiPolyFillet(hps, MAX_VALUES-1, Points+1);
        else
            GpiPolyLine(hps, MAX_VALUES-1, Points+1);
        LastDrawZero = FALSE;
        }
    elif ( Update || ! LastDrawZero ) {
        WinFillRect(hps, &DiagRect, BackColor);
        GpiSetColor(hps, CLR_BLUE);
        Points[0].y = Points[MAX_VALUES-1].y = DiagRect.yBottom;
        GpiMove(hps, Points);
        GpiLine(hps, Points + MAX_VALUES-1);
        LastDrawZero = TRUE;
        }
    
    WinReleasePS(hps);
    
    return;
}

MRESULT EXPENTRY MainDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    BOOL bResult;
    static char Buffer[256];
    static char DefaultInterface[256];
    static int DefIntSet = FALSE;
    static int DefIntIndex = -1;
    int i, NewInterface;
    static int CacheTotalIn, CacheTotalOut;
    static int CacheEstIn, CacheEstOut;
    static int CacheMaxIn, CacheMaxOut;
    static int Minimized = FALSE;
    static SWP InitialPos[14];
    static SWP NewPos[3];
    static LONG TitleBarHeight;
    int TempIn, TempOut;
    static HACCEL hAccel;
    static HWND hMenu;
    PSWP WindowPos;
    HPOINTER hptrIcon;
    HSWITCH hSwitch;
    MENUITEM MenuItem;
    ULONG MenuStyle;
    char TempBuffer[12];

    switch ( msg ) {
      case WM_INITDLG:
        AlwaysTop = PrfQueryProfileInt(HINI_USERPROFILE, "IPSPEED", "ALWAYSTOP", FALSE);
        ControlsHidden = PrfQueryProfileInt(HINI_USERPROFILE, "IPSPEED", "CONTROLSHIDDEN", FALSE);
        ShowMode = PrfQueryProfileInt(HINI_USERPROFILE, "IPSPEED", "SHOWMODE", CMD_SHOWALL);
        Smoothed = PrfQueryProfileInt(HINI_USERPROFILE, "IPSPEED", "SMOOTHED", FALSE);
        EstCount = PrfQueryProfileInt(HINI_USERPROFILE, "IPSPEED", "AVERAGE_INTERVAL", DEF_EST_COUNT);
        if ( CmdInterface != NULL ) {
            WinEnableMenuItem(hMenu, CMD_DEFAULTINTERFACE, FALSE);
            strncpy(DefaultInterface, CmdInterface, sizeof(DefaultInterface));
            DefaultInterface[sizeof(DefaultInterface)-1] = 0;
            }
        else {
            PrfQueryProfileString(HINI_USERPROFILE, "IPSPEED", "DEFAULTINTERFACE", "", 
                DefaultInterface, sizeof(DefaultInterface));
            }
        strupr(DefaultInterface);
        
        hAccel = WinLoadAccelTable(hab, (HMODULE)NULL, DLG_IPSPEED);
        WinSetAccelTable(hab, hAccel, hwnd);
        hMenu = WinLoadMenu(HWND_DESKTOP, (HMODULE)NULL, MENU_IPSPEED);
        if ( AlwaysTop ) {
            WinCheckMenuItem(hMenu, CMD_ALWAYSONTOP, TRUE);
            }
        if ( Smoothed ) {
            WinCheckMenuItem(hMenu, CMD_SMOOTHED, TRUE);
            }
        WinCheckMenuItem(hMenu, CMD_AVERAGEVAL + EstCount, TRUE);
        if ( ! ControlsHidden ) {
            TitleBarHeight = WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR);
            WinCheckMenuItem(hMenu, CMD_TITLEBAR, TRUE);
            }
        else {
            TitleBarHeight = 0;
            }

        WinSendMsg(hMenu, MM_SETITEMATTR, MPFROM2SHORT(CMD_DEFAULTINTERFACE, FALSE), 
            MPFROM2SHORT(MIA_DISABLED, MIA_DISABLED));                

        WinSendMsg(hMenu, MM_QUERYITEM, MPFROM2SHORT(CMD_RESETMENU, TRUE),
            MPFROMP(&MenuItem));
 
        MenuStyle = WinQueryWindowULong(MenuItem.hwndSubMenu, QWL_STYLE);
        MenuStyle |= MS_CONDITIONALCASCADE;
        WinSetWindowULong(MenuItem.hwndSubMenu, QWL_STYLE, MenuStyle);

        WinSendMsg(MenuItem.hwndSubMenu, MM_SETDEFAULTITEMID, 
            MPFROM2SHORT(CMD_RESET, FALSE), NULL);
 
        WinSendMsg(hMenu, MM_QUERYITEM, MPFROM2SHORT(CMD_SHOWMENU, TRUE),
            MPFROMP(&MenuItem));

        WinSendMsg(MenuItem.hwndSubMenu, MM_SETITEMATTR, MPFROM2SHORT(ShowMode, FALSE), 
            MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
         
        /*           
        MenuStyle = WinQueryWindowULong(MenuItem.hwndSubMenu, QWL_STYLE);
        MenuStyle |= MS_CONDITIONALCASCADE;
        WinSetWindowULong(MenuItem.hwndSubMenu, QWL_STYLE, MenuStyle);

        WinSendMsg(MenuItem.hwndSubMenu, MM_SETDEFAULTITEMID, 
            MPFROM2SHORT(CMD_SHOWALL, FALSE), NULL);
        */            
            
        OldDiagProc = WinSubclassWindow(WinWindowFromID(hwnd, FRAME_DIAG), NewDiagProc);
        
        OldTextProc = WinSubclassWindow(WinWindowFromID(hwnd, ST_TOTALOUT), NewTextProc);
        OldTextProc = WinSubclassWindow(WinWindowFromID(hwnd, ST_TOTALIN), NewTextProc);
        OldTextProc = WinSubclassWindow(WinWindowFromID(hwnd, ST_ESTOUT), NewTextProc);
        OldTextProc = WinSubclassWindow(WinWindowFromID(hwnd, ST_ESTIN), NewTextProc);
        OldTextProc = WinSubclassWindow(WinWindowFromID(hwnd, ST_MAXOUT), NewTextProc);
        OldTextProc = WinSubclassWindow(WinWindowFromID(hwnd, ST_MAXIN), NewTextProc);
        OldTextProc = WinSubclassWindow(WinWindowFromID(hwnd, ST_INTERFACE), NewTextProc);
        OldTextProc = WinSubclassWindow(WinWindowFromID(hwnd, ST_NOMINALSPEEDCPS), NewTextProc);
            
        hptrIcon = WinLoadPointer(HWND_DESKTOP, 0, DLG_IPSPEED);
        hSwitch = MakeMainWindow(hwnd, "IP Speed", hptrIcon);
        
        // WinShowWindow(hwnd, FALSE);
        // WinPostMsg(hwnd, RESTORE_WINDOW, NULL, NULL);

        WinQueryWindowPos(hwnd, &InitialPos[0]);
        WinQueryWindowPos(WinWindowFromID(hwnd, ST_INTERFACE), &InitialPos[1]);
        WinQueryWindowPos(WinWindowFromID(hwnd, CX_INTERFACE), &InitialPos[2]);
        WinQueryWindowPos(WinWindowFromID(hwnd, ST_NOMINALSPEEDCPS), &InitialPos[3]);
        WinQueryWindowPos(WinWindowFromID(hwnd, FRAME_DIAG), &InitialPos[4]);
        WinQueryWindowPos(WinWindowFromID(hwnd, GB_TOTAL), &InitialPos[5]);
        WinQueryWindowPos(WinWindowFromID(hwnd, ST_TOTALIN), &InitialPos[6]);
        WinQueryWindowPos(WinWindowFromID(hwnd, ST_TOTALOUT), &InitialPos[7]);
        WinQueryWindowPos(WinWindowFromID(hwnd, GB_ESTIMATED), &InitialPos[8]);
        WinQueryWindowPos(WinWindowFromID(hwnd, ST_ESTIN), &InitialPos[9]);
        WinQueryWindowPos(WinWindowFromID(hwnd, ST_ESTOUT), &InitialPos[10]);
        WinQueryWindowPos(WinWindowFromID(hwnd, GB_MAXCPS), &InitialPos[11]);
        WinQueryWindowPos(WinWindowFromID(hwnd, ST_MAXIN), &InitialPos[12]);
        WinQueryWindowPos(WinWindowFromID(hwnd, ST_MAXOUT), &InitialPos[13]);
        for ( i = 0; i < 14; i++ )
            InitialPos[i].fl = SWP_MOVE;
        
        if ( ControlsHidden ) 
            HideFrameControls(hwnd, TRUE);
                        
        if ( CmdInterface != NULL ) {
            WinRestoreWindowPos("IPSPEED", CmdInterface, hwnd);
            }
        else {
            WinRestoreWindowPos("IPSPEED", "WINDOWPOS", hwnd);
            }
        WinSendMsg(hwnd, WM_COMMAND, MPFROMSHORT(ShowMode), NULL);
        WinSetActiveWindow(HWND_DESKTOP, hwnd);
                        
        bResult = WinStartTimer(hab, hwnd, 1, 1000);
        WinSendMsg(hwnd, WM_TIMER, MPFROMLONG(1), NULL);        
        return NULL;

      case RESTORE_WINDOW:
        WinShowWindow(hwnd, TRUE);
        return NULL;
                
      case WM_MINMAXFRAME:
        if ( ((PSWP)PVOIDFROMMP(mp1))->fl & (SWP_MINIMIZE) ) {
            Minimized = TRUE;
            }
        elif ( ((PSWP)PVOIDFROMMP(mp1))->fl & (SWP_RESTORE | SWP_MAXIMIZE) ) {
            Minimized = FALSE;
            }
        break;

      /*
      case WM_ADJUSTWINDOWPOS:
        WindowPos = (PSWP)PVOIDFROMMP(mp1);
        if ( WindowPos->fl & (SWP_ZORDER | SWP_DEACTIVATE) ) {
            WindowPos->fl |= SWP_ZORDER;
            WindowPos->hwndInsertBehind = HWND_TOP;
            }
        break;
        */
                             
      case WM_TIMER:
        if ( LONGFROMMP(mp1) == 1 ) {
            WinSendMsg(WinWindowFromID(hwnd, ST_TOTALOUT), WM_HITTEST, NULL, NULL);
            WinSendMsg(WinWindowFromID(hwnd, GB_ESTIMATED), WM_HITTEST, NULL, NULL);
            
            if ( AlwaysTop && ! Minimized && WinIsWindowVisible(hwnd) ) {
                WinSetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_ZORDER);
                }

            if ( AddInterfaceData() ) {
                WinSendMsg(WinWindowFromID(hwnd, CX_INTERFACE), LM_DELETEALL, NULL, NULL);
                
                if ( CmdInterface != NULL ) {
                    DefIntSet = FALSE;
                    }
                for ( i = 0; i < ActiveInterfaces; i++ ) {
                    strcpy(Buffer, Interface[ActiveIndex[i]].Descr);
                    strupr(Buffer);
                    if ( ! DefIntSet && NULL != strstr(Buffer, DefaultInterface) ) {
                        DefIntIndex = i;
                        }

                    WinSendMsg(WinWindowFromID(hwnd, CX_INTERFACE), LM_INSERTITEM, 
                        MPFROMSHORT(i), Interface[ActiveIndex[i]].Descr);
                    }

                if ( ! DefIntSet && DefIntIndex != -1 ) {
                    DefIntSet = TRUE;
                    WinSendMsg(WinWindowFromID(hwnd, CX_INTERFACE), LM_SELECTITEM, 
                        MPFROMSHORT(DefIntIndex), MPFROMSHORT(TRUE));
                    }
                elif ( ActiveInterfaces > 0 ) {
                    if ( CurrInterface == -1 || CurrInterface >= ActiveInterfaces ) {
                        WinSendMsg(WinWindowFromID(hwnd, CX_INTERFACE), LM_SELECTITEM, 
                            MPFROMSHORT(0), MPFROMSHORT(TRUE));
                        }
                    else {
                        WinSendMsg(WinWindowFromID(hwnd, CX_INTERFACE), LM_SELECTITEM, 
                            MPFROMSHORT(CurrInterface), MPFROMSHORT(TRUE));
                        }
                    }
                else {
                    CurrInterface = -1;                
                    }

                if ( CmdInterface == NULL ) {
                    WinSendMsg(hMenu, MM_SETITEMATTR, MPFROM2SHORT(CMD_DEFAULTINTERFACE, FALSE), 
                        MPFROM2SHORT(MIA_DISABLED, (CurrInterface == -1 ? MIA_DISABLED : 0)));                
                    }
                }

                    
            if ( CurrInterface != -1 ) {                
                if ( CacheTotalIn != Interface[ActiveIndex[CurrInterface]].TotalIn -
                                     Interface[ActiveIndex[CurrInterface]].SubTotalIn ) {
                    CacheTotalIn = Interface[ActiveIndex[CurrInterface]].TotalIn - 
                                   Interface[ActiveIndex[CurrInterface]].SubTotalIn;
                    sprintf(Buffer, "%u in", CacheTotalIn);
                    WinSetWindowText(WinWindowFromID(hwnd, ST_TOTALIN), Buffer);
                    }

                if ( CacheTotalOut != Interface[ActiveIndex[CurrInterface]].TotalOut -
                                      Interface[ActiveIndex[CurrInterface]].SubTotalOut ) { 
                    CacheTotalOut = Interface[ActiveIndex[CurrInterface]].TotalOut -
                                    Interface[ActiveIndex[CurrInterface]].SubTotalOut;
                    sprintf(Buffer, "%u out", CacheTotalOut);
                    WinSetWindowText(WinWindowFromID(hwnd, ST_TOTALOUT), Buffer);
                    }

                if ( CacheEstIn != Interface[ActiveIndex[CurrInterface]].EstIn ) {
                    CacheEstIn = Interface[ActiveIndex[CurrInterface]].EstIn;
                    sprintf(Buffer, "%u in", Interface[ActiveIndex[CurrInterface]].EstIn);
                    WinSetWindowText(WinWindowFromID(hwnd, ST_ESTIN), Buffer);
                    }

                if ( CacheEstOut != Interface[ActiveIndex[CurrInterface]].EstOut ) {
                    CacheEstOut = Interface[ActiveIndex[CurrInterface]].EstOut;
                    sprintf(Buffer, "%u out", Interface[ActiveIndex[CurrInterface]].EstOut);
                    WinSetWindowText(WinWindowFromID(hwnd, ST_ESTOUT), Buffer);
                    }

                if ( CacheMaxIn != Interface[ActiveIndex[CurrInterface]].MaxIn ) {
                    CacheMaxIn = Interface[ActiveIndex[CurrInterface]].MaxIn;
                    sprintf(Buffer, "%u in", Interface[ActiveIndex[CurrInterface]].MaxIn);
                    WinSetWindowText(WinWindowFromID(hwnd, ST_MAXIN), Buffer);
                    }

                if ( CacheMaxOut != Interface[ActiveIndex[CurrInterface]].MaxOut ) {
                    CacheMaxOut = Interface[ActiveIndex[CurrInterface]].MaxOut;
                    sprintf(Buffer, "%u out", Interface[ActiveIndex[CurrInterface]].MaxOut);
                    WinSetWindowText(WinWindowFromID(hwnd, ST_MAXOUT), Buffer);
                    }

                DrawDiag(WinWindowFromID(hwnd, FRAME_DIAG), FALSE);
                }
            
            return NULL;
            }

        break;

      case WM_CONTROL:
        if ( SHORT1FROMMP(mp1) == CX_INTERFACE ) {
            if ( SHORT2FROMMP(mp1) == LN_SELECT ) {
                NewInterface = (SHORT)WinSendMsg(WinWindowFromID(hwnd, CX_INTERFACE), 
                    LM_QUERYSELECTION, MPFROMSHORT(0), NULL);
                   
                if ( NewInterface != CurrInterface ) {
                    CurrInterface = NewInterface;
                    sprintf(Buffer, "Nominal speed: %u bps", Interface[ActiveIndex[CurrInterface]].Nominal);
                    WinSetWindowText(WinWindowFromID(hwnd, ST_NOMINALSPEEDCPS), Buffer);
                    WinSendMsg(hwnd, WM_TIMER, MPFROMLONG(1), NULL);
                    }
                
                return NULL;                    
                }
            }
        break;

      case WM_COMMAND:
        switch ( SHORT1FROMMP(mp1) ) {
          case CMD_RESET:
            if ( CurrInterface != -1 ) {
                TempIn = Interface[ActiveIndex[CurrInterface]].TotalIn;
                TempOut = Interface[ActiveIndex[CurrInterface]].TotalOut;
                    
                memset(Interface[ActiveIndex[CurrInterface]].Time, 0, 
                    sizeof(INTERFACE) - FIELD_OFFSET(INTERFACE, Time));
                    
                Interface[ActiveIndex[CurrInterface]].SubTotalIn = TempIn;
                Interface[ActiveIndex[CurrInterface]].SubTotalOut = TempOut;
                }
            break;

          case CMD_RESETMAX:
            if ( CurrInterface != -1 ) {
                Interface[ActiveIndex[CurrInterface]].MaxIn = 0;
                Interface[ActiveIndex[CurrInterface]].MaxOut = 0;
                }
            break;

          case CMD_RESETTOTAL:
            if ( CurrInterface != -1 ) {
                Interface[ActiveIndex[CurrInterface]].SubTotalIn =
                    Interface[ActiveIndex[CurrInterface]].TotalIn;
                Interface[ActiveIndex[CurrInterface]].SubTotalOut =
                    Interface[ActiveIndex[CurrInterface]].TotalOut;
                }
            break;

          case CMD_ALWAYSONTOP:
            AlwaysTop = ! AlwaysTop;            
            WinSendMsg(hMenu, MM_SETITEMATTR, MPFROM2SHORT(CMD_ALWAYSONTOP, FALSE), 
                MPFROM2SHORT(MIA_CHECKED, (AlwaysTop ? MIA_CHECKED : 0)));
            break;
              
          case CMD_SMOOTHED:
            Smoothed = ! Smoothed;            
            WinSendMsg(hMenu, MM_SETITEMATTR, MPFROM2SHORT(CMD_SMOOTHED, FALSE), 
                MPFROM2SHORT(MIA_CHECKED, (Smoothed ? MIA_CHECKED : 0)));
            break;
              
          case CMD_TITLEBAR:
            if ( ControlsHidden ) {
                TitleBarHeight = WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR);
                ShowFrameControls(hwnd, TRUE);
                }
            else {
                TitleBarHeight = 0;
                HideFrameControls(hwnd, TRUE);
                }

            // WinQueryWindowPos(hwnd, &InitialPos[0]);
            WinSendMsg(hMenu, MM_SETITEMATTR, MPFROM2SHORT(CMD_TITLEBAR, FALSE), 
                MPFROM2SHORT(MIA_CHECKED, (ControlsHidden ? 0 : MIA_CHECKED)));
            break;

          case CMD_DEFAULTINTERFACE:
            if ( CurrInterface != -1 ) {
                strcpy(DefaultInterface, Interface[ActiveIndex[CurrInterface]].Descr);
                DefIntSet = TRUE;
                }
            break;

          case CMD_SHOWALL:
            WinSendMsg(hMenu, MM_QUERYITEM, MPFROM2SHORT(CMD_SHOWMENU, TRUE),
                MPFROMP(&MenuItem));
            WinSendMsg(MenuItem.hwndSubMenu, MM_SETITEMATTR, MPFROM2SHORT(ShowMode, FALSE), 
                MPFROM2SHORT(MIA_CHECKED, 0));
            ShowMode = CMD_SHOWALL;
            WinSendMsg(MenuItem.hwndSubMenu, MM_SETITEMATTR, MPFROM2SHORT(CMD_SHOWALL, FALSE), 
                MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
            WinSetMultWindowPos(hab, InitialPos+1, 13);
            // WinSetWindowPos(hwnd, NULLHANDLE, 0, 0, InitialPos[0].cx, InitialPos[0].cy, SWP_SIZE);
            WinShowWindow(WinWindowFromID(hwnd, ST_INTERFACE), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, CX_INTERFACE), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, ST_NOMINALSPEEDCPS), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, FRAME_DIAG), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, GB_TOTAL), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, ST_TOTALIN), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, ST_TOTALOUT), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, GB_ESTIMATED), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, ST_ESTIN), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, ST_ESTOUT), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, GB_MAXCPS), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, ST_MAXIN), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, ST_MAXOUT), TRUE);
            WinSetWindowPos(hwnd, NULLHANDLE, 0, 0, 
                InitialPos[4].cx + InitialPos[4].x + InitialPos[5].x, 
                InitialPos[4].cy + InitialPos[4].y + InitialPos[5].y + TitleBarHeight, SWP_SIZE);
            break;
            
          case CMD_SHOWDIAG:
            WinSendMsg(hMenu, MM_QUERYITEM, MPFROM2SHORT(CMD_SHOWMENU, TRUE),
                MPFROMP(&MenuItem));
            WinSendMsg(MenuItem.hwndSubMenu, MM_SETITEMATTR, MPFROM2SHORT(ShowMode, FALSE), 
                MPFROM2SHORT(MIA_CHECKED, 0));
            ShowMode = CMD_SHOWDIAG;
            WinSendMsg(MenuItem.hwndSubMenu, MM_SETITEMATTR, MPFROM2SHORT(CMD_SHOWDIAG, FALSE), 
                MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
            for ( i = 0; i < 1; i++ ) {
                NewPos[i] = InitialPos[i+5];
                NewPos[i].hwnd = InitialPos[i+4].hwnd;
                }
            WinShowWindow(WinWindowFromID(hwnd, ST_INTERFACE), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, CX_INTERFACE), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_NOMINALSPEEDCPS), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, FRAME_DIAG), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, GB_TOTAL), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_TOTALIN), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_TOTALOUT), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, GB_ESTIMATED), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_ESTIN), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_ESTOUT), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, GB_MAXCPS), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_MAXIN), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_MAXOUT), FALSE);
            WinSetMultWindowPos(hab, NewPos, 1);
            WinSetWindowPos(hwnd, NULLHANDLE, 0, 0, 
                InitialPos[5].cx + 2 * InitialPos[5].x, 
                InitialPos[5].cy + 2 * InitialPos[5].y + TitleBarHeight, SWP_SIZE);
            break;
            
          case CMD_SHOWTOTAL:
            WinSendMsg(hMenu, MM_QUERYITEM, MPFROM2SHORT(CMD_SHOWMENU, TRUE),
                MPFROMP(&MenuItem));
            WinSendMsg(MenuItem.hwndSubMenu, MM_SETITEMATTR, MPFROM2SHORT(ShowMode, FALSE), 
                MPFROM2SHORT(MIA_CHECKED, 0));
            ShowMode = CMD_SHOWTOTAL;
            WinShowWindow(WinWindowFromID(hwnd, ST_INTERFACE), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, CX_INTERFACE), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_NOMINALSPEEDCPS), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, FRAME_DIAG), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, GB_TOTAL), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, ST_TOTALIN), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, ST_TOTALOUT), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, GB_ESTIMATED), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_ESTIN), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_ESTOUT), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, GB_MAXCPS), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_MAXIN), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_MAXOUT), FALSE);
            WinSendMsg(MenuItem.hwndSubMenu, MM_SETITEMATTR, MPFROM2SHORT(CMD_SHOWTOTAL, FALSE), 
                MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
            WinSetWindowPos(hwnd, NULLHANDLE, 0, 0, 
                InitialPos[5].cx + 2 * InitialPos[5].x, 
                InitialPos[5].cy + 2 * InitialPos[5].y + TitleBarHeight, SWP_SIZE);
            break;
            
          case CMD_SHOWESTIMATED:
            WinSendMsg(hMenu, MM_QUERYITEM, MPFROM2SHORT(CMD_SHOWMENU, TRUE),
                MPFROMP(&MenuItem));
            WinSendMsg(MenuItem.hwndSubMenu, MM_SETITEMATTR, MPFROM2SHORT(ShowMode, FALSE), 
                MPFROM2SHORT(MIA_CHECKED, 0));
            ShowMode = CMD_SHOWESTIMATED;
            WinSendMsg(MenuItem.hwndSubMenu, MM_SETITEMATTR, MPFROM2SHORT(CMD_SHOWESTIMATED, FALSE), 
                MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
            for ( i = 0; i < 3; i++ ) {
                NewPos[i] = InitialPos[i+5];
                NewPos[i].hwnd = InitialPos[i+8].hwnd;
                }
            WinShowWindow(WinWindowFromID(hwnd, ST_INTERFACE), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, CX_INTERFACE), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_NOMINALSPEEDCPS), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, FRAME_DIAG), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, GB_TOTAL), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_TOTALIN), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_TOTALOUT), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, GB_ESTIMATED), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, ST_ESTIN), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, ST_ESTOUT), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, GB_MAXCPS), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_MAXIN), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_MAXOUT), FALSE);
            WinSetMultWindowPos(hab, NewPos, 3);
            WinSetWindowPos(hwnd, NULLHANDLE, 0, 0, 
                InitialPos[5].cx + 2 * InitialPos[5].x, 
                InitialPos[5].cy + 2 * InitialPos[5].y + TitleBarHeight, SWP_SIZE);
            break;
            
          case CMD_SHOWMAX:
            WinSendMsg(hMenu, MM_QUERYITEM, MPFROM2SHORT(CMD_SHOWMENU, TRUE),
                MPFROMP(&MenuItem));
            WinSendMsg(MenuItem.hwndSubMenu, MM_SETITEMATTR, MPFROM2SHORT(ShowMode, FALSE), 
                MPFROM2SHORT(MIA_CHECKED, 0));
            ShowMode = CMD_SHOWMAX;
            WinSendMsg(MenuItem.hwndSubMenu, MM_SETITEMATTR, MPFROM2SHORT(CMD_SHOWMAX, FALSE), 
                MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
            for ( i = 0; i < 3; i++ ) {
                NewPos[i] = InitialPos[i+5];
                NewPos[i].hwnd = InitialPos[i+11].hwnd;
                }
            WinShowWindow(WinWindowFromID(hwnd, ST_INTERFACE), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, CX_INTERFACE), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_NOMINALSPEEDCPS), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, FRAME_DIAG), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, GB_TOTAL), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_TOTALIN), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_TOTALOUT), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, GB_ESTIMATED), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_ESTIN), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, ST_ESTOUT), FALSE);
            WinShowWindow(WinWindowFromID(hwnd, GB_MAXCPS), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, ST_MAXIN), TRUE);
            WinShowWindow(WinWindowFromID(hwnd, ST_MAXOUT), TRUE);
            WinSetMultWindowPos(hab, NewPos, 3);
            WinSetWindowPos(hwnd, NULLHANDLE, 0, 0, 
                InitialPos[5].cx + 2 * InitialPos[5].x, 
                InitialPos[5].cy + 2 * InitialPos[5].y + TitleBarHeight, SWP_SIZE);
            break;
            
          case CMD_ABOUT:
            WinDlgBox(HWND_DESKTOP, hwnd, AboutDlgProc, NULLHANDLE, DLG_ABOUT, NULL);
            break;
            
          case CMD_QUIT:
            WinSendMsg(hwnd, WM_CLOSE, NULL, NULL);
            break;
            
          default:
            if ( SHORT1FROMMP(mp1) > CMD_AVERAGEVAL && SHORT1FROMMP(mp1) < CMD_AVERAGEVALMAX ) {
                WinCheckMenuItem(hMenu, CMD_AVERAGEVAL + EstCount, FALSE);
                EstCount = SHORT1FROMMP(mp1) - CMD_AVERAGEVAL;
                WinCheckMenuItem(hMenu, CMD_AVERAGEVAL + EstCount, TRUE);
                }
            break;
            }

        return NULL;
                    
      case WM_BUTTON1MOTIONSTART:
      case WM_BUTTON2MOTIONSTART:
	    return WinSendMsg(hwnd, WM_TRACKFRAME, MPFROMSHORT(TF_MOVE), NULL);

      case WM_CONTEXTMENU:
        if ( SHORT2FROMMP(mp2) ) {
            POINTL PointerPos;
            RECTL WindowRect;
            
            WinQueryPointerPos(HWND_DESKTOP, &PointerPos);
            WinMapWindowPoints(HWND_DESKTOP, hwnd, &PointerPos, 1);
            /*
            WinQueryWindowRect(hwnd, &WindowRect);
            
            if ( PointerPos.x < WindowRect.xLeft )
                PointerPos.x = WindowRect.xLeft;
            elif ( PointerPos.x >= WindowRect.xRight )
                PointerPos.x = WindowRect.xRight - 1;
                
            if ( PointerPos.y < WindowRect.yBottom )
                PointerPos.y = WindowRect.yBottom;
            elif ( PointerPos.y >= WindowRect.yTop )
                PointerPos.y = WindowRect.yTop - 1;
            */
                
            ((POINTS *)(&mp1))->x = (SHORT)(PointerPos.x);
            ((POINTS *)(&mp1))->y = (SHORT)(PointerPos.y);
            }

        WinPopupMenu(hwnd, hwnd, hMenu, ((POINTS *)(&mp1))->x, ((POINTS *)(&mp1))->y, 0, 
            PU_HCONSTRAIN | PU_VCONSTRAIN | PU_KEYBOARD | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2);            
        return (MRESULT)TRUE;
          
      case WM_BUTTON1DBLCLK:
        if ( ! Minimized ) {
            if ( ShowMode != CMD_SHOWALL &&
                 ((POINTS *)(&mp1))->x >= InitialPos[5].x &&  
                 ((POINTS *)(&mp1))->y >= InitialPos[5].y &&  
                 ((POINTS *)(&mp1))->x < InitialPos[5].x + InitialPos[5].cx &&  
                 ((POINTS *)(&mp1))->y < InitialPos[5].y + InitialPos[5].cy ) {
                WinSendMsg(hwnd, WM_COMMAND, MPFROMSHORT(CMD_SHOWALL), NULL);
                }
            elif ( ((POINTS *)(&mp1))->x >= InitialPos[4].x &&  
                 ((POINTS *)(&mp1))->y >= InitialPos[4].y &&  
                 ((POINTS *)(&mp1))->x < InitialPos[4].x + InitialPos[4].cx &&  
                 ((POINTS *)(&mp1))->y < InitialPos[4].y + InitialPos[4].cy ) {
                WinSendMsg(hwnd, WM_COMMAND, MPFROMSHORT(CMD_SHOWDIAG), NULL);
                }
            elif ( ((POINTS *)(&mp1))->x >= InitialPos[5].x &&  
                 ((POINTS *)(&mp1))->y >= InitialPos[5].y &&  
                 ((POINTS *)(&mp1))->x < InitialPos[5].x + InitialPos[5].cx &&  
                 ((POINTS *)(&mp1))->y < InitialPos[5].y + InitialPos[5].cy ) {
                WinSendMsg(hwnd, WM_COMMAND, MPFROMSHORT(CMD_SHOWTOTAL), NULL);
                }
            elif ( ((POINTS *)(&mp1))->x >= InitialPos[8].x &&  
                 ((POINTS *)(&mp1))->y >= InitialPos[8].y &&  
                 ((POINTS *)(&mp1))->x < InitialPos[8].x + InitialPos[8].cx &&  
                 ((POINTS *)(&mp1))->y < InitialPos[8].y + InitialPos[8].cy ) {
                WinSendMsg(hwnd, WM_COMMAND, MPFROMSHORT(CMD_SHOWESTIMATED), NULL);
                }
            elif ( ((POINTS *)(&mp1))->x >= InitialPos[11].x &&  
                 ((POINTS *)(&mp1))->y >= InitialPos[11].y &&  
                 ((POINTS *)(&mp1))->x < InitialPos[11].x + InitialPos[11].cx &&  
                 ((POINTS *)(&mp1))->y < InitialPos[11].y + InitialPos[11].cy ) {
                WinSendMsg(hwnd, WM_COMMAND, MPFROMSHORT(CMD_SHOWMAX), NULL);
                }
            else {
                WinSendMsg(hwnd, WM_COMMAND, MPFROMSHORT(CMD_TITLEBAR), NULL);
                }
    	    return NULL;
            }

        
	    break;

      case WM_CLOSE:
        WinStopTimer(hab, hwnd, 1);
        PrfWriteProfileString(HINI_USERPROFILE, "IPSPEED", "ALWAYSTOP", 
            (AlwaysTop ? "1" : "0"));
        PrfWriteProfileString(HINI_USERPROFILE, "IPSPEED", "CONTROLSHIDDEN", 
            (ControlsHidden ? "1" : "0"));
        PrfWriteProfileString(HINI_USERPROFILE, "IPSPEED", "SHOWMODE", 
            itoa(ShowMode, TempBuffer, 10));
        PrfWriteProfileString(HINI_USERPROFILE, "IPSPEED", "SMOOTHED", 
            (Smoothed ? "1" : "0"));
        PrfWriteProfileString(HINI_USERPROFILE, "IPSPEED", "AVERAGE_INTERVAL", 
            _itoa(EstCount, TempBuffer, 10));
        if ( CmdInterface == NULL ) {
            PrfWriteProfileString(HINI_USERPROFILE, "IPSPEED", "DEFAULTINTERFACE", 
                DefaultInterface);
            }
        if ( CmdInterface != NULL ) {
            WinStoreWindowPos("IPSPEED", CmdInterface, hwnd);
            }
        else {
            WinStoreWindowPos("IPSPEED", "WINDOWPOS", hwnd);
            }
        WinPostMsg(hwnd, WM_QUIT, 0L, 0L);
        return NULL;
        
      case WM_DESTROY:
        return NULL;                       
        }

    return WinDefDlgProc( hwnd, msg, mp1, mp2);    
}

MRESULT EXPENTRY AboutDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    switch ( msg ) {
      case WM_INITDLG:
        return NULL;

      case WM_COMMAND :
        switch (COMMANDMSG(&msg)->cmd) {
          case DID_OK :
          case DID_CANCEL :
            WinDismissDlg(hwnd, TRUE);
            return NULL;
            }

      default :
        return WinDefDlgProc(hwnd, msg, mp1, mp2);
        }
}


void HideFrameControls(HWND hwndFrame, int AdjustSize)
{        
    LONG TitleBarHeight = WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR);
    RECTL WindowRect;
    
    hwndTitleBar = WinWindowFromID(hwndFrame, FID_TITLEBAR);
    hwndSysMenu = WinWindowFromID(hwndFrame, FID_SYSMENU);
    hwndMinMax = WinWindowFromID(hwndFrame, FID_MINMAX);
 
    WinSetParent(hwndTitleBar, HWND_OBJECT, FALSE);
    WinSetParent(hwndSysMenu, HWND_OBJECT, FALSE);
    WinSetParent(hwndMinMax, HWND_OBJECT, FALSE);
        
    WinSendMsg(hwndFrame, WM_UPDATEFRAME, (MPARAM)(FCF_TITLEBAR | FCF_SYSMENU | FCF_MINMAX | FCF_MENU), NULL);
    if ( AdjustSize ) {
        WinQueryWindowRect(hwndFrame, &WindowRect);
        WinSetWindowPos(hwndFrame, NULL, 0, 0, WindowRect.xRight, WindowRect.yTop - TitleBarHeight,
            SWP_SIZE);
        }

    ControlsHidden = TRUE;
}


void ShowFrameControls(HWND hwndFrame, int AdjustSize)
{
    LONG TitleBarHeight = WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR);
    RECTL WindowRect;
    
    WinSetParent(hwndTitleBar, hwndFrame, FALSE);
    WinSetParent(hwndSysMenu, hwndFrame, FALSE);
    WinSetParent(hwndMinMax, hwndFrame, FALSE);

    WinSendMsg(hwndFrame, WM_UPDATEFRAME, (MPARAM)(FCF_TITLEBAR | FCF_SYSMENU | FCF_MINMAX | FCF_MENU), NULL);
    if ( AdjustSize ) {
        WinQueryWindowRect(hwndFrame, &WindowRect);
        WinSetWindowPos(hwndFrame, NULL, 0, 0, WindowRect.xRight, WindowRect.yTop + TitleBarHeight,
            SWP_SIZE);
        /*
        WindowRect.yBottom = WindowRect.yTop - 1;
        WindowRect.yTop += TitleBarHeight;
        WinInvalidateRect(hwndFrame, &WindowRect, TRUE);
        */
        }

    WinSendMsg(hwndTitleBar, TBM_SETHILITE, 
        MPFROMSHORT(TRUE), NULL);

    ControlsHidden = FALSE;
}


HSWITCH MakeMainWindow(HWND hwnd, PSZ Title, HPOINTER Icon)
{
    HSWITCH hswitch;
    SWCNTRL swcntrl;
    ULONG uResult; //, i;

    swcntrl.hwnd = hwnd;
    swcntrl.hwndIcon = Icon;
    swcntrl.hprog = NULLHANDLE;
    swcntrl.idProcess = 0;
    swcntrl.idSession = 0;
    swcntrl.uchVisibility = SWL_VISIBLE;
    swcntrl.fbJump = SWL_JUMPABLE;
    strncpy(swcntrl.szSwtitle, Title, MAXNAMEL);
    swcntrl.szSwtitle[MAXNAMEL] = 0;
    swcntrl.bProgType = 0; // PROG_PM;
    hswitch = WinCreateSwitchEntry(WinQueryAnchorBlock(hwnd), &swcntrl);

    WinSetWindowText(hwnd, Title);

    WinSendMsg(hwnd, WM_SETICON, MPFROMP(Icon), 0);
    WinSetWindowULong(hwnd, QWL_STYLE,
        FS_ICON | WinQueryWindowULong(hwnd, QWL_STYLE));
    WinSendMsg(hwnd, WM_UPDATEFRAME, 0, 0);

    return hswitch;
}

MRESULT EXPENTRY NewFrameProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    if ( msg == WM_WINDOWPOSCHANGED ) {
        MRESULT Res = OldFrameProc(hwnd, msg, mp1, mp2);
        return Res;
        }

    return OldFrameProc(hwnd, msg, mp1, mp2);
}


MRESULT EXPENTRY NewDiagProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    if ( msg == WM_PAINT ) {
        MRESULT Res = OldDiagProc(hwnd, msg, mp1, mp2);
        DrawDiag(hwnd, TRUE);
        return Res;
        }

    return OldDiagProc(hwnd, msg, mp1, mp2);
}

MRESULT EXPENTRY NewTextProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    if ( msg == WM_HITTEST ) {
        return (MRESULT)HT_TRANSPARENT;
        }

    return OldTextProc(hwnd, msg, mp1, mp2);
}


