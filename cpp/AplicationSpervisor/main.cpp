#define _WIN32_IE 0x0500
#include <windows.h>
#include <process.h>

#include <iostream>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <sstream>
#include <string>
#include <ctime>
#include <time.h>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <vector>

#include "resource.h"

using namespace std;

//variables needed for tray
#define ID_TRAY1   777
#define CMSG_TRAY1 0x8001
#define NIIF_NONE 0x00000000
#define NIIF_INFO 0x00000001
#define NIIF_WARNING 0x00000002
#define NIIF_ERROR 0x00000003
#define NIIF_USER 0x00000004
#define NIIF_NOSOUND 0x00000010
#define NIIF_LARGE_ICON 0x00000020
#define NIIF_RESPECT_QUIET_TIME 0x00000080

#define NIF_INFO 0x00000010

int checkApplication(string co_robie);
void sendSMS(string wiadomosc);
string url_encode(const string &value);

void __cdecl smsDetal(void * Args);

HWND hwnd;
HINSTANCE hInstanceGlobal;

//struct needed for tray
typedef struct _NOTIFYICONDATA {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    #if _WIN32_IE >= 0x0500
    CHAR szTip[ 128 ];
    DWORD dwState;
    DWORD dwStateMask;
    CHAR szInfo[ 256 ];
    union {
        UINT uTimeout;
        UINT uVersion;
    } DUMMYUNIONNAME;
    CHAR szInfoTitle[ 64 ];
    DWORD dwInfoFlags;
    #else
    CHAR szTip[ 64 ];
    #endif
    #if _WIN32_IE >= 0x600
    HICON hBalloonIcon;
    GUID guidItem;
    #endif
}_NOTIFYICONDATA;


//start program in tray
void startTray(){
	NOTIFYICONDATA nid;
	LPSTR sTip = (char*)"Supervising";

	nid.cbSize = sizeof( NOTIFYICONDATA );
	nid.hWnd = hwnd;
	nid.uID = ID_TRAY1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = CMSG_TRAY1;
	nid.uTimeout = 10000;
	nid.hIcon = LoadIcon( hInstanceGlobal,  MAKEINTRESOURCE( IDI_ICON1 ) );
	lstrcpy( nid.szTip, sTip );

	Shell_NotifyIcon( NIM_ADD, & nid );
}

//stop program in tray
void stopTray(){
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof( NOTIFYICONDATA );
	nid.hWnd = hwnd;
	nid.uID = ID_TRAY1;
	nid.uFlags = 0;
	Shell_NotifyIcon( NIM_DELETE, & nid );
	PostQuitMessage( 0 );
}


// This is where all the input to the window goes to
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch(Message) {
		// Upon destruction, tell the main thread to stop
		case CMSG_TRAY1: // clicking on icon
		{
		    if( wParam == ID_TRAY1 )
		    if( lParam == WM_RBUTTONDOWN ){ //RMB - closing
		    	if(MessageBox( NULL, "Do you want to close supervisor?", "Closing", MB_ICONINFORMATION | MB_YESNO ) == IDYES)
		    	{
				    stopTray();
		         	exit(0);
				}
			}
		}
		break;
		case WM_DESTROY: {
		    stopTray();
		}
		break;

		// All other messages (a lot of them) are processed using default procedures
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

//=========================================================================================================
//=========================================================================================================


// The 'main' function of Win32 GUI programs: this is where execution starts
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc; // A properties struct of our window
	 // A 'HANDLE', hence the H, or a pointer to our window
	MSG msg; // A temporary location for all messages
	// zero out the struct and set the stuff we want to modify
	hInstanceGlobal = hInstance;
	memset(&wc,0,sizeof(wc));
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.lpfnWndProc	 = WndProc; // This is where we will send messages to
	wc.hInstance	 = hInstance;
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);

	// White, COLOR_WINDOW is just a #define for a system color, try Ctrl+Clicking it
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszClassName = "WindowClass";
	wc.hIcon		 = LoadIcon(hInstanceGlobal, MAKEINTRESOURCE( IDI_ICON1 )); // Load a standard icon
	wc.hIconSm		 = LoadIcon(hInstanceGlobal, MAKEINTRESOURCE( IDI_ICON1 )); // use the name "A" to use the project icon

	if(!RegisterClassEx(&wc)) {
		MessageBox(NULL, "Window Registration Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,"WindowClass","Caption",WS_MINIMIZE|WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, // x
		CW_USEDEFAULT, // y
		640, // width
		480, // height
		NULL,NULL,hInstance,NULL);

	if(hwnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}
	//starting
	startTray();

    //	HANDLE hThread =( HANDLE ) _beginthread( smsDetal, 0, NULL );
    _beginthread( smsDetal, 0, NULL );


	while(GetMessage(&msg, NULL, 0, 0) > 0) { // If no error is received...
		TranslateMessage(&msg); // Translate key codes to chars if present
		DispatchMessage(&msg); // Send it to WndProc
	}
	return msg.wParam;
}


//targeted function - checking database and folder, sending sms
void __cdecl smsDetal ( void * Args )
{
    int queryResultLogin = 0;
    int queryResultTurnOff = 0;

    string path = "\\\\remoteDisc\\folderName";
    bool accesToDiscDefault = true;
    bool accesToDiscCurrent = true;


    while(1)
    {
        queryResultLogin = 0;
        queryResultTurnOff = 0;

        SYSTEMTIME st;
        GetSystemTime(&st);

        // every 10 minutes - check wheather or not it has access to remote disc
        if(st.wMinute % 10 == 0)
        {
            DWORD ftyp = GetFileAttributesA(path.c_str());

            if (ftyp == INVALID_FILE_ATTRIBUTES){
                accesToDiscDefault = false;
                accesToDiscCurrent = false;
                sendSMS("Supervisor: DO NOT have access to disc");
                //cout << "NO ACCESS" << endl;
            } else {
                accesToDiscCurrent = true;

                if(accesToDiscDefault == accesToDiscCurrent){
                   // cout << "DO HAVE ACCESS, DO NOT RESTART" << endl;
                } else{
                    sendSMS("Supervisor: REGAINED ACCESS - RESET");
                    //cout << "REGAINED ACCESS - RESET" << endl;

                    //cout << "TURN OFF APPLICATION" << endl;
                    system("taskkill /F /IM APPLICATION.EXE");

                    //cout << "WAIT 10 SECONDS" << endl;
                    Sleep(10000);

                    //cout << "START APPLICATION" << endl;
                    ShellExecuteA(0, 0 ,"\\\\remoteDisc\\folderName\\APPLICATION.mdb",0,0,SW_SHOW);
                }

                accesToDiscDefault = true;
            }
        }

        // at 5:45 - check wheather or not it logged in
        if((st.wHour == 5 && st.wMinute == 45))
        {
            queryResultLogin = checkApplication("login");
            if(queryResultLogin == 0)
            {
               // cout << "DID NOT LOG IN - RESET" << endl;
                sendSMS("Supervisor: DID NOT LOG IN - RESET");

                //cout << "TURN OFF APPLICATION" << endl;
                system("taskkill /F /IM APPLICATION.EXE");

                //cout << "WAIT 10 SECONDS" << endl;
                Sleep(10000);

                //cout << "START APPLICATION" << endl;
                ShellExecuteA(0, 0 ,"\\\\remoteDisc\\folderName\\APPLICATION.mdb",0,0,SW_SHOW);
            }
            else
            {
                //cout << "LOGGED IN - DO NOT RESET" << endl;
                sendSMS("Supervisor: LOGGED IN - DO NOT RESET");
            }
        }
        // at 12:15 - check wheather or not it turned off
        else if((st.wHour == 12 && st.wMinute == 15))
        {
            queryResultTurnOff = checkApplication("turnOff");
            if(queryResultTurnOff == 0)
            {
                //cout << "DID NOT TURN OFF - RESET" << endl;
                sendSMS("Supervisor: DID NOT TURN OFF - RESET");

                //cout << "TURN OFF APPLICATION" << endl;
                system("taskkill /F /IM APPLICATION.EXE");

                //cout << "WAIT 10 SECONDS" << endl;
                Sleep(10000);

                //cout << "START APPLICATION" << endl;
                ShellExecuteA(0, 0 ,"\\\\remoteDisc\\folderName\\APPLICATION.mdb",0,0,SW_SHOW);            }
            else
            {
                //cout << "TURNED OFF - DO NOT RESET" << endl;
                sendSMS("Supervisor: TURNED OFF - DO NOT RESET");
            }
       }
        Sleep(60000);
    }

    _endthread();
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

int checkApplication(string action)
{
    char szDSN[256] = "Driver={Microsoft Access Driver (*.mdb)};Dbq=\\\\remoteDisc\\folderName\\dataBase.mdb;";

    const char* DAM = "Direct ODBC";

    SYSTEMTIME st;
    GetSystemTime(&st);
    string data;
    ostringstream ss;
    int tmp_data;

    tmp_data = st.wYear;
    ss << tmp_data;
    data += ss.str();
    data += "-";
    ss.str("");
    ss.clear();

    tmp_data = st.wMonth;
    ss << tmp_data;
    if(ss.str().size() < 2)
    {
        data += "0";
    }
    data += ss.str();
    data += "-";
    ss.str("");
    ss.clear();

    tmp_data = st.wDay;
    ss << tmp_data;
    if(ss.str().size() < 2)
    {
        data += "0";
    }
    data += ss.str();
    ss.str("");
    ss.clear();

    //cout << data << endl << endl;

    string queryStr;

    if(action == "turnOff")
    {
        queryStr = "SELECT fields FROM table WHERE field LIKE '%condition%' AND fldDataWyslania LIKE '";
        queryStr += data;
        queryStr += "%'";
    }
    else if(action == "login")
    {
        queryStr = "SELECT fields FROM table WHERE field LIKE '%condition%' AND fldDataWyslania LIKE '";
        queryStr += data;
        queryStr += "%'";
    }

    SQLCHAR* query =(SQLCHAR*)queryStr.c_str();

    HENV    hEnv;
    HDBC    hDbc;

    RETCODE retc;

    int     iConnStrLength2Ptr;
    char    szConnStrOut[256];

    SQLCHAR         chval1[128], chval2[128], colName[128];
    int             ret1;
    int             ret2;

    SQLINTEGER      rowCount = 0;
    SQLSMALLINT     fieldCount = 0, currentField = 0;
    HSTMT           hStmt;

    retc = SQLAllocEnv(&hEnv);

    retc = SQLAllocConnect(hEnv, &hDbc);
    retc = SQLDriverConnect(hDbc, NULL, (unsigned char*)szDSN,
        SQL_NTS, (unsigned char*)szConnStrOut,
        255, (SQLSMALLINT*)&iConnStrLength2Ptr, SQL_DRIVER_NOPROMPT);

    if (SQL_SUCCEEDED(retc))
    {
        printf("Successfully connected to database.\n");


        printf("%s: SQL query:\n  %s\n", DAM, query);

        retc = SQLAllocStmt(hDbc,&hStmt);
        retc = SQLPrepare(hStmt, query, SQL_NTS);


        retc = SQLBindCol(hStmt, 1, SQL_C_CHAR, chval1, 128, (SQLINTEGER*)&ret1);
        retc = SQLBindCol(hStmt, 2, SQL_C_CHAR, chval2, 128, (SQLINTEGER*)&ret2);

        retc = SQLExecute(hStmt);
        if (SQL_SUCCEEDED(retc))
        {
            printf("%s: Retrieve schema info for the given result set:\n", DAM);
            SQLNumResultCols(hStmt, &fieldCount);
            if (fieldCount > 0)
            {
                for (currentField=1; currentField <= fieldCount; currentField++)
                {
                    SQLDescribeCol(hStmt, currentField,
                        colName, sizeof(colName), 0, 0, 0, 0, 0);
                    printf(" | %s", colName);
                }
                printf("\n");
            }
            else
            {
                printf("%s: Error: Number of fields in the result set is 0.\n", DAM);
            }

            printf("%s: Fetch the actual data:\n", DAM);

            retc = SQLFetch(hStmt);
            while (SQL_SUCCEEDED(retc))
            {
                printf(" | %s | \n", chval1);
                retc = SQLFetch(hStmt);
                rowCount++;
            };

            printf("%s: Total Row Count: %d\n", DAM, rowCount);
            retc = SQLFreeStmt(hStmt, SQL_DROP);
        }
        else
        {
            cout << "DID NOT WORK OUT" << endl;
        }

    }
    else
    {
        printf("%s: Couldn't connect to %s.\n", DAM, szDSN);
    }


    SQLDisconnect(hDbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

    return rowCount;
}


void sendSMS(string message)
{
    string futureURL = "index.php/send_sms?login=LOGIN&pass=PASSWOR&to=123456789&message=";
    futureURL += message;

    string host_sms = "128.0.0.1";

    WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "\nWSAStartup failed.\n";

		return;
	}
	SOCKET Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct hostent *host;
	host = gethostbyname(host_sms.c_str());
	SOCKADDR_IN SockAddr;
	SockAddr.sin_port = htons(80);
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

	//cout << "\nConnecting...";
	if (connect(Socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0) {
		cout << "\nCould not connect";

		return;
	}
	//cout << "\nConnected!" << endl;


	string request = "GET /" + url_encode(futureURL) + " HTTP/1.1\r\n"
		"Host: " + host->h_name + "\r\nConnection: close\r\n\r\n";

	send(Socket, request.c_str(), strlen(request.c_str()), 0);

	char buffer[1024] = { "" };

	recv(Socket, buffer, 1024, 0);

	string buf(buffer);

    closesocket(Socket);
	WSACleanup();
}


// it changes message to url
string url_encode(const string &value)
{
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '_' || c == '.' || c == '~' || c == '/' || c == '?'
            || c == '&' || c == '=') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char) c);
        escaped << nouppercase;
    }

    return escaped.str();
}
