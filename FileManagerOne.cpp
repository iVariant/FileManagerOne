#include "stdafx.h"
#include "FileManagerOne.h"

#define MAX_LOADSTRING 100
#define MAX_PATH 1024

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR path1[MAX_PATH], path2[MAX_PATH];
TCHAR selectedFile1[MAX_PATH], selectedFile2[MAX_PATH];
int width = 1024;
int height = 768;

HWND hWndComboBoxDisk1, hWndComboBoxDisk2,
	 hWndEdit1, hWndEdit2,
	 hWndListBox1, hWndListBox2,
	 hWndButtonRename,hWndButtonCopy,hWndButtonMove,hWndButtonDIR,hWndButtonDelete,hWndButtonRefresh,hWndButtonNotePad;

WNDPROC origWndProcListView;
TCHAR GAppname[] = TEXT("WinMain :: ComboBox");

int lastListBox = 0;
HANDLE copyThread;

typedef struct _REPARSE_DATA_BUFFER
{
	ULONG ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
	union
	{
		struct
		{
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG Flags;
			WCHAR PathBuffer[1];
		}
		SymbolicLinkReparseBuffer;
		struct
		{
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR PathBuffer[1];
		}
		MountPointReparseBuffer;
		struct
		{
			UCHAR  DataBuffer[1];
		}
		GenericReparseBuffer;
	};
}
REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);


int					FileOperation(TCHAR *from, TCHAR *to, UINT func);
HWND				CreateListBox(int x, int y, int width, int heigth, HWND hWnd, HMENU id);
void				LoadFileList(HWND hWndlistBox, TCHAR *path);
int CALLBACK		SortUpDir(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
void				AddIconToListBox(HWND hWndListBox, int size, TCHAR c_dir[MAX_PATH]);
void				DisplayError(TCHAR *header);
LRESULT CALLBACK	WndProcListView1(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK	WndProcListView2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	DialogRename1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	DialogRename2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	DialogCreateDir1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	DialogCreateDir2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);



#pragma region Region_Initialisation

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_FILEMANAGERONE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FILEMANAGERONE));
	InitCommonControls();
	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}




ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_FILEMANAGERONE);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));

	return RegisterClassEx(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   
   RECT rect;
   SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);


   hWnd = CreateWindow(szWindowClass, 
	   szTitle, 
	   WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
	   (rect.right - width) / 2.0,
	   (rect.bottom - height) / 2.0,
	   width,
	   height,
	  NULL,
	  NULL, 
	  hInstance, 
	  NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   RECT rec;
   GetClientRect(hWnd, &rec);
   width = rec.right;
   height = rec.bottom;
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

 
#pragma endregion Region_Initialisation

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	static int xCombo = 507;
	static int yCombo = 43;
	static int xCount = 43;



	LPNMHDR lpnmHdr = (LPNMHDR)lParam;
	LPNMLISTVIEW pnmLV = (LPNMLISTVIEW)lParam;
	TCHAR *selectedFile = 0;
	TCHAR selectedFileSize[MAX_PATH];
	TCHAR fullPathToFile[MAX_PATH];

	bool reloadFileList = 1;
	HWND hWndListBox = 0;
	HWND hWndEdit = 0;
	TCHAR *path = 0;
	int k=0;

	switch (message)
	{
	case WM_CREATE:
	{

	#pragma region Create_Windows
		hWndComboBoxDisk1 = CreateWindow(
			L"ComboBox",
			NULL,
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_SORT | CBS_DROPDOWNLIST,
			0, 0,
			40, 0,
			hWnd,
			(HMENU)ID_COMBO_BOX_1,
			NULL,
			NULL
			);



		hWndComboBoxDisk2 = CreateWindow(

			L"ComboBox",
			NULL,
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_SORT | CBS_DROPDOWNLIST ,
			xCombo, 0,
			40, 100,
			hWnd,
			(HMENU)ID_COMBO_BOX_2,
			NULL,
			NULL
			);


		hWndEdit1 = CreateWindow(

			L"Edit",
			NULL,
			WS_BORDER | WS_VISIBLE | WS_CHILD | ES_LEFT | ES_READONLY,
			0,25, 
			500, 20, 
			hWnd, 
			(HMENU)ID_EDIT_1,
			NULL, 
			NULL);


		hWndEdit2 = CreateWindow(
			L"Edit",
			NULL,
			WS_BORDER | WS_VISIBLE | WS_CHILD | ES_LEFT | ES_READONLY,
			507, 25,
			500, 20,
			hWnd,
			(HMENU)ID_EDIT_2,
			NULL,
			NULL);

		
		hWndListBox1 = CreateListBox(
			0,46,
			500,620,
			hWnd,
			(HMENU)ID_LISTBOX_1
			);
			

		hWndListBox2 = CreateListBox(
			507, 46,
			500, 620,
			hWnd,
			(HMENU)ID_LISTBOX_2
			);


		int x = 0, y = 666, dx = 201;


		hWndButtonRename = CreateWindow(
			L"Button",
			L"F2 Rename",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD,
			x,y,
			202,45,
			hWnd,
			(HMENU)ID_BUTTON_RENAME,
			NULL,
			NULL);
			
		x += dx;



		hWndButtonCopy = CreateWindow(
			L"Button",
			L"F5 Copy",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD,
			x, y,
			202, 45,
			hWnd,
			(HMENU)ID_BUTTON_COPY,
			NULL,
			NULL);

		x += dx;



		hWndButtonMove = CreateWindow(
			L"Button",
			L"F6 Move",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD,
			x, y,
			202, 45,
			hWnd,
			(HMENU)ID_BUTTON_MOVE,
			NULL,
			NULL);

		x += dx;




		hWndButtonDIR = CreateWindow(
			L"Button",
			L"F7 New Folder",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD,
			x, y,
			202, 45,
			hWnd,
			(HMENU)ID_BUTTON_DIR,
			NULL,
			NULL);

		x += dx;

		hWndButtonDelete = CreateWindow(
			L"Button",
			L"F8 Delete",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD,
			x, y,
			203, 45,
			hWnd,
			(HMENU)ID_BUTTON_DELETE,
			NULL,
			NULL);

		hWndButtonRefresh = CreateWindow(
			L"Button",
			L"",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_ICON | BS_PUSHBUTTON,
			50, 0,
			25, 25,
			hWnd,
			(HMENU)ID_BUTTON_REFRESH,
			NULL,
			NULL);

		SendMessage(hWndButtonRefresh, BM_SETIMAGE, IMAGE_ICON,
			(LPARAM)LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON2), IMAGE_ICON,
			0, 0, LR_DEFAULTCOLOR));


		hWndButtonNotePad = CreateWindow(
			L"Button",
			L"",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_ICON | BS_PUSHBUTTON,
			80, 0,
			25, 25,
			hWnd,
			(HMENU)ID_BUTTON_NOTEPAD,
			NULL,
			NULL);

		SendMessage(hWndButtonNotePad, BM_SETIMAGE, IMAGE_ICON,
			(LPARAM)LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON3), IMAGE_ICON,
			0, 0, LR_DEFAULTCOLOR));


		//SetMenuItemBitmaps(IDC_FILEMANAGERONE,)
	#pragma endregion Create_Windows

	#pragma region WinOpions

		//Add discs for ComboBox
		TCHAR *diskMyPC, *disk;
		diskMyPC = disk = new TCHAR[256];

		GetLogicalDrives();
		GetLogicalDriveStrings(256, (LPTSTR)disk);
		k = _tcslen(diskMyPC) + 1;

		while (*disk != '\0')
		{
			disk[1] = 0;


			SendMessage(hWndComboBoxDisk1, CB_ADDSTRING, 0, (LPARAM)disk);
			SendMessage(hWndComboBoxDisk2, CB_ADDSTRING, 0, (LPARAM)disk);
			disk += k;
		}

		delete[] diskMyPC;



		SendMessage(hWndComboBoxDisk1, CB_SETCURSEL, 0, 0L);
		SendMessage(hWndComboBoxDisk2, CB_SETCURSEL, 0, 0L);



		//Add patch
		path1[0] = path2[0] = 0;
		_tcscat_s(path1, L"C:\\");

		SetWindowText(hWndEdit1,path1);
		SetWindowText(hWndEdit2, path1);
		
		//add font 
		HFONT hFont = CreateFont(16, 0, 0, 0, 700, 1, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft Sans Serif");
		SendMessage(hWndComboBoxDisk1, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hWndComboBoxDisk2, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hWndButtonCopy, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hWndButtonDelete, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hWndButtonDIR, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hWndButtonMove, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hWndButtonRename, WM_SETFONT, (WPARAM)hFont, NULL);

		hFont = CreateFont(16, 0, 0, 0, 500, 1, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft Sans Serif");
		SendMessage(hWndEdit1, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hWndEdit2, WM_SETFONT, (WPARAM)hFont, NULL);
		//SendMessage(hWndEdit2, WM_SETCURSOR, (WPARAM)hFont, NULL);

		//add picture in menu
		HBITMAP hCross = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP1));
		SetMenuItemBitmaps(GetMenu(hWnd), ID_HELP_VIEWHELP, MF_BYCOMMAND, hCross, hCross);

		hCross = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP2));
		SetMenuItemBitmaps(GetMenu(hWnd), IDCLOSE, MF_BYCOMMAND, hCross, hCross);
		
		hCross = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP3));
		SetMenuItemBitmaps(GetMenu(hWnd), IDM_ABOUT, MF_BYCOMMAND, hCross, hCross);
		
		hCross = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP4));
		SetMenuItemBitmaps(GetMenu(hWnd), ID_ARCHIVE_A, MF_BYCOMMAND, hCross, hCross);

		hCross = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP5));
		SetMenuItemBitmaps(GetMenu(hWnd), ID_ARCHIVE_UNPACK, MF_BYCOMMAND, hCross, hCross);

		hCross = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP6));
		SetMenuItemBitmaps(GetMenu(hWnd), ID_FILE_OPEN, MF_BYCOMMAND, hCross, hCross);

		hCross = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP7));
		SetMenuItemBitmaps(GetMenu(hWnd), ID_FILE_NEW, MF_BYCOMMAND, hCross, hCross);

		SendMessage(hWnd, WM_SIZE, NULL, NULL);

		origWndProcListView = (WNDPROC)SetWindowLong(hWndListBox1,
			GWL_WNDPROC, (LONG)WndProcListView1);

		origWndProcListView = (WNDPROC)SetWindowLong(hWndListBox2,
			GWL_WNDPROC, (LONG)WndProcListView2);
		
		LoadFileList(hWndListBox1, path1);
		LoadFileList(hWndListBox2, path1);

	#pragma endregion WinOpions

	}
		break;




	case WM_SIZE:
#pragma region SIZE1
		RECT rec;
		GetClientRect(hWnd, &rec);
		LVCOLUMN column;
		column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

			xCombo = rec.right / 2;
			yCombo = rec.bottom-90;
			xCount = rec.right / 5;
			if (rec.right > width)
			{
				SetWindowPos(hWndComboBoxDisk2, 0, xCombo, 0, 40, 100, 0);

				SetWindowPos(hWndEdit2, 0, xCombo, 25, xCombo, 20, 0);
				SetWindowPos(hWndEdit1, 0, 0, 25, xCombo - 4, 20, 0);

				SetWindowPos(hWndListBox2, 0, xCombo, 46, xCombo, yCombo, 0);
				SetWindowPos(hWndListBox1, 0, 0, 46, xCombo - 4, yCombo, 0);


				SetWindowPos(hWndButtonRename, 0, 0, rec.bottom - 44, xCount, 44, 0);
				SetWindowPos(hWndButtonCopy, 0, xCount, rec.bottom - 44, xCount, 44, 0);

				SetWindowPos(hWndButtonMove, 0, xCount*2, rec.bottom - 44, xCount, 44, 0);
				SetWindowPos(hWndButtonDIR, 0, xCount*3, rec.bottom - 44, xCount, 44, 0);
				SetWindowPos(hWndButtonDelete, 0, xCount*4, rec.bottom - 44, xCount+3, 44, 0);

			}
			else if (rec.right < width)
			{

				SetWindowPos(hWndComboBoxDisk2, 0, xCombo, 0, 40, 100, 0);

				SetWindowPos(hWndEdit2, 0, xCombo, 25, xCombo, 20, 0);
				SetWindowPos(hWndEdit1, 0, 0, 25, xCombo - 4, 20, 0);

				SetWindowPos(hWndListBox2, 0, xCombo, 46, xCombo, yCombo, 0);
				SetWindowPos(hWndListBox1, 0, 0, 46, xCombo - 4, yCombo, 0);

				SetWindowPos(hWndButtonRename, 0, 0, rec.bottom - 44, xCount, 44, 0);
				SetWindowPos(hWndButtonCopy, 0, xCount, rec.bottom - 44, xCount, 44, 0);

				SetWindowPos(hWndButtonMove, 0, xCount * 2, rec.bottom - 44, xCount, 44, 0);
				SetWindowPos(hWndButtonDIR, 0, xCount * 3, rec.bottom - 44, xCount, 44, 0);
				SetWindowPos(hWndButtonDelete, 0, xCount * 4, rec.bottom - 44, xCount+3, 44, 0);
			}


			else if (rec.bottom < height)
			{
				SetWindowPos(hWndListBox2, 0, xCombo, 46, xCombo, yCombo, 0);
				SetWindowPos(hWndListBox1, 0, 0, 45, xCombo - 4, yCombo, 0);

				SetWindowPos(hWndListBox2, 0, xCombo, 46, xCombo, yCombo, 0);
				SetWindowPos(hWndListBox1, 0, 0, 46, xCombo - 4, yCombo, 0);

				SetWindowPos(hWndButtonRename, 0, 0, rec.bottom - 44, xCount, 44, 0);
				SetWindowPos(hWndButtonCopy, 0, xCount, rec.bottom - 44, xCount, 44, 0);

				SetWindowPos(hWndButtonMove, 0, xCount * 2, rec.bottom - 44, xCount, 44, 0);
				SetWindowPos(hWndButtonDIR, 0, xCount * 3, rec.bottom - 44, xCount, 44, 0);
				SetWindowPos(hWndButtonDelete, 0, xCount * 4, rec.bottom - 44, xCount+3, 44, 0);
			}


			else if (rec.bottom > height)
			{
				SetWindowPos(hWndListBox2, 0, xCombo, 46, xCombo, yCombo, 0);
				SetWindowPos(hWndListBox1, 0, 0, 45, xCombo - 4, yCombo, 0);


				SetWindowPos(hWndListBox2, 0, xCombo, 46, xCombo, yCombo, 0);
				SetWindowPos(hWndListBox1, 0, 0, 46, xCombo - 4, yCombo, 0);

				SetWindowPos(hWndButtonRename, 0, 0, rec.bottom - 44, xCount, 44, 0);
				SetWindowPos(hWndButtonCopy, 0, xCount, rec.bottom - 44, xCount, 44, 0);

				SetWindowPos(hWndButtonMove, 0, xCount * 2, rec.bottom - 44, xCount, 44, 0);
				SetWindowPos(hWndButtonDIR, 0, xCount * 3, rec.bottom - 44, xCount, 44, 0);
				SetWindowPos(hWndButtonDelete, 0, xCount * 4, rec.bottom - 44, xCount+3, 44, 0);
			}

	
			
			column.iSubItem = 0;			// номер колонки
			column.pszText = L"Name";	// имя колонки
			column.cx = xCombo/2;               // Ширина колонки
			column.fmt = LVCFMT_LEFT;		// Прижать влево текст
			ListView_SetColumn(hWndListBox1, 0, &column);
			ListView_SetColumn(hWndListBox2, 0, &column);

			column.iSubItem = 1;			// номер колонки
			column.pszText = L"Type";	// имя колонки
			column.cx = xCombo / 4;               // Ширина колонки
			ListView_SetColumn(hWndListBox1, 1, &column);
			ListView_SetColumn(hWndListBox2, 1, &column);

			column.iSubItem = 2;			// номер колонки
			column.pszText = L"Date";	// имя колонки
			column.cx = xCombo / 4+10;               // Ширина колонки
			ListView_SetColumn(hWndListBox1, 2, &column);
			ListView_SetColumn(hWndListBox2, 2, &column);

			width = rec.right;
			height = rec.bottom;
#pragma endregion SIZE1
		break;


	case WM_NOTIFY:
#pragma region ListBox
		switch (lpnmHdr->code)
		{
		case NM_CLICK:
			switch (lpnmHdr->idFrom)
			{
			case ID_LISTBOX_1:
				hWndListBox = hWndListBox1;
				selectedFile = selectedFile1;
				lastListBox = 1;
				break;
			case ID_LISTBOX_2:
				hWndListBox = hWndListBox2;
				selectedFile = selectedFile2;
				lastListBox = 2;
				break;
			default:
				break;
			}
			if (hWndListBox)
			{
				ListView_GetItemText(lpnmHdr->hwndFrom, pnmLV->iItem, 0, selectedFile, MAX_PATH);
			}
			break;
			//		case NM_RETURN:
		case NM_DBLCLK:
			switch (lpnmHdr->idFrom)
			{
			case ID_LISTBOX_1:
				hWndListBox = hWndListBox1;
				hWndEdit = hWndEdit1;
				path = path1;
				break;
			case ID_LISTBOX_2:
				hWndListBox = hWndListBox2;
				hWndEdit = hWndEdit2;
				path = path2;
				break;
			default:
				break;
			}

			if (hWndListBox)
			{
				selectedFile = new TCHAR[MAX_PATH];
				ListView_GetItemText(lpnmHdr->hwndFrom, pnmLV->iItem, 0, selectedFile, MAX_PATH);
				//_________________________________________________________________________________
				if (_tcscmp(selectedFile, _T("..")) == 0)	// Вверх на одну дирректорию
				{
					path[_tcslen(path) - 1] = 0;
					for (int i = _tcslen(path) - 1; i > 0; i--)
					{
						TCHAR s;
						s = path[i];
						if (s == '\\')
						{
							k = i;
							break;
						}
					}
					path[k + 1] = 0;
				}
				else if (_tcscmp(selectedFile, _T(".")) == 0)	// В корень диска
				{
					path[3] = 0;
				}
				else
				{
					ListView_GetItemText(lpnmHdr->hwndFrom, pnmLV->iItem, 1, selectedFileSize, MAX_PATH);
					if (_tcscmp(selectedFileSize, _T("<DIR>")) == 0)
					{
						_tcscat_s(path, MAX_PATH, _T("\\"));
						path[_tcslen(path) - 1] = 0;
						_tcscat_s(path, MAX_PATH, selectedFile);
						_tcscat_s(path, MAX_PATH, _T("\\"));
					}
					else if (_tcscmp(selectedFileSize, _T("<Link>")) == 0)
					{
						HANDLE reparsePoint;
						BYTE *pBuffer;
						PREPARSE_DATA_BUFFER pReparseBuffer = NULL;
						DWORD dwRetCode;

						fullPathToFile[0] = 0;
						_tcscpy_s(fullPathToFile, _T("\\??\\"));
						//_tcscpy_s(fullPathToFile, _T("\\\\.\\"));
						_tcscat_s(fullPathToFile, path);
						_tcscat_s(fullPathToFile, selectedFile);

						reparsePoint = CreateFile(fullPathToFile,
							READ_CONTROL,
							FILE_SHARE_READ,
							0,
							OPEN_EXISTING,
							FILE_FLAG_OPEN_REPARSE_POINT |
							FILE_FLAG_BACKUP_SEMANTICS |
							0,
							0);

						if (reparsePoint == INVALID_HANDLE_VALUE)
						{
							DisplayError(_T("reParsePoint 1"));
							CloseHandle(reparsePoint);
						}
						else
						{
							pBuffer = new BYTE[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
							pReparseBuffer = (PREPARSE_DATA_BUFFER)pBuffer;
							if (DeviceIoControl(
								reparsePoint,
								FSCTL_GET_REPARSE_POINT,
								NULL,
								0,
								pReparseBuffer,
								MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
								&dwRetCode,
								NULL))
							{
								if (pReparseBuffer->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
								{
									if (*pReparseBuffer->MountPointReparseBuffer.PathBuffer)
									{
										_tcscpy_s(path, MAX_PATH, &(pReparseBuffer->MountPointReparseBuffer.PathBuffer)[4]);
										_tcscat_s(path, MAX_PATH, _T("\\"));
									}
									else
										MessageBox(0, _T("Ошибка при получении пути по ссылке"), _T("Ошибка"), 0);
								}
								else
								{
									_tcscat_s(path, MAX_PATH, _T("\\"));
									path[_tcslen(path) - 1] = 0;
									_tcscat_s(path, MAX_PATH, selectedFile);
									_tcscat_s(path, MAX_PATH, _T("\\"));
								}
							}
							else DisplayError(_T("DeviceIoControl 1"));
							delete pBuffer;
							CloseHandle(reparsePoint);
						}
					}
					else
					{
						fullPathToFile[0] = 0;
						_tcscpy_s(fullPathToFile, path);
						_tcscat_s(fullPathToFile, selectedFile);
						ShellExecute(0, _T("open"), fullPathToFile, NULL, NULL, SW_SHOWNORMAL);
						reloadFileList = 0;
					}
				}

				//_________________________________________________________________________________

				if (reloadFileList)
				{
					lastListBox = 0;
					SetWindowText(hWndEdit, path);
					LoadFileList(hWndListBox, path);
					switch (lpnmHdr->idFrom)
					{
					case ID_LISTBOX_1:
						selectedFile1[0] = 0;
						break;
					case ID_LISTBOX_2:
						selectedFile2[0] = 0;
						break;
					default:
						break;
					}

				}
				delete[]selectedFile;
			}
#pragma endregion ListBox
			break;



		case NM_RCLICK:
			switch (lpnmHdr->idFrom)
			{
			case ID_LISTBOX_1:
				hWndListBox = hWndListBox1;
				hWndEdit = hWndEdit1;
				path = path1;
				break;
			case ID_LISTBOX_2:
				hWndListBox = hWndListBox2;
				hWndEdit = hWndEdit2;
				path = path2;
				break;
			default:
				break;
			}
			if (hWndListBox)
			{
				selectedFile = new TCHAR[MAX_PATH];
				ListView_GetItemText(lpnmHdr->hwndFrom, pnmLV->iItem, 0, selectedFile, MAX_PATH);

				SHELLEXECUTEINFO fileInfo;

				_tcscpy_s(fullPathToFile, path);
				_tcscat_s(fullPathToFile, selectedFile);

				ZeroMemory(&fileInfo, sizeof(SHELLEXECUTEINFO));
				fileInfo.cbSize = sizeof(SHELLEXECUTEINFO);
				fileInfo.lpVerb = _T("properties");
				fileInfo.lpFile = fullPathToFile;
				fileInfo.nShow = SW_SHOW;
				fileInfo.fMask = SEE_MASK_INVOKEIDLIST;
				ShellExecuteEx(&fileInfo);
			}
			break;
		}

		break;


	case WM_COMMAND:
	

		if ((HWND)lParam == hWndEdit1) SetFocus(hWndComboBoxDisk1);
			else if ((HWND)lParam == hWndEdit2) SetFocus(hWndComboBoxDisk1);


			wmId = LOWORD(wParam);
			wmEvent = HIWORD(wParam);





			// Разобрать выбор в меню:
			switch (wmId)
			{
			case IDM_ABOUT:
				MessageBox(hWnd, L"Program: FileManagerOne v.1.0\nDeveloper: IX Variant\nE-MAIL: ix.variant@gmail.com", L"About", MB_OK);
				break;

			case IDCLOSE:
				DestroyWindow(hWnd);
				break;

			case ID_FILE_NEW:
				switch (lastListBox)
				{
				case 1:
					SendMessage(hWndListBox1, WM_KEYDOWN, VK_F7, 0);
					break;
				case 2:
					SendMessage(hWndListBox2, WM_KEYDOWN, VK_F7, 0);
					break;
				}
				break;


			case ID_BUTTON_REFRESH:
				//Add discs for ComboBox
				TCHAR *diskMyPC, *disk;
				diskMyPC = disk = new TCHAR[256];

				GetLogicalDrives();
				GetLogicalDriveStrings(256, (LPTSTR)disk);
				k = _tcslen(diskMyPC) + 1;


				

				for (int icount = SendMessage(hWndComboBoxDisk1, CB_GETCOUNT, 0, 0); icount != 0; icount--)
				{
				SendMessage(hWndComboBoxDisk1, CB_DELETESTRING, 0, 0);
				SendMessage(hWndComboBoxDisk2, CB_DELETESTRING, 0, 0);
				}
				while (*disk != '\0')
				{
					disk[1] = 0;


					SendMessage(hWndComboBoxDisk1, CB_ADDSTRING, 0, (LPARAM)disk);
					SendMessage(hWndComboBoxDisk2, CB_ADDSTRING, 0, (LPARAM)disk);
					disk += k;
				}

				delete[] diskMyPC;

				SendMessage(hWndComboBoxDisk1, CB_SETCURSEL, 0, 0L);
				SendMessage(hWndComboBoxDisk2, CB_SETCURSEL, 0, 0L);
				break;
				
			case ID_FILE_OPEN:

				break;
				

				




			case ID_BUTTON_RENAME:
				switch (lastListBox)
				{
				case 1:
					SendMessage(hWndListBox1, WM_KEYDOWN, VK_F2, 0);
					break;
				case 2:
					SendMessage(hWndListBox2, WM_KEYDOWN, VK_F2, 0);
					break;
				}
				break;

			case ID_BUTTON_COPY:
				switch (lastListBox)
				{
				case 1:
					SendMessage(hWndListBox1, WM_KEYDOWN, VK_F5, 0);
					break;
				case 2:
					SendMessage(hWndListBox2, WM_KEYDOWN, VK_F5, 0);
					break;
				}
				break;

			case ID_BUTTON_MOVE:
				switch (lastListBox)
				{
				case 1:
					SendMessage(hWndListBox1, WM_KEYDOWN, VK_F6, 0);
					break;
				case 2:
					SendMessage(hWndListBox2, WM_KEYDOWN, VK_F6, 0);
					break;
				}
				break;

			case ID_BUTTON_DIR:
				switch (lastListBox)
				{
				case 1:
					SendMessage(hWndListBox1, WM_KEYDOWN, VK_F7, 0);
					break;
				case 2:
					SendMessage(hWndListBox2, WM_KEYDOWN, VK_F7, 0);
					break;
				}
				break;







			case ID_COMBO_BOX_1:
					
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{

					hWndListBox = hWndListBox1;
					hWndEdit = hWndEdit1;
					selectedFile = selectedFile1;
					path = path1;
					GetWindowText(GetDlgItem(hWnd, wmId), path, MAX_PATH);
					_tcscat_s(path, MAX_PATH, _T(":\\"));
					SetWindowText(hWndEdit, path);
					LoadFileList(hWndListBox, path);
					lastListBox = 0;
					selectedFile[0] = 0;
				}
				break;
			case ID_COMBO_BOX_2:
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					hWndListBox = hWndListBox2;
					hWndEdit = hWndEdit2;
					selectedFile = selectedFile2;
					path = path2;
				
				GetWindowText(GetDlgItem(hWnd, wmId), path, MAX_PATH);
				_tcscat_s(path, MAX_PATH, _T(":\\"));
				SetWindowText(hWndEdit, path);
				LoadFileList(hWndListBox, path);
				lastListBox = 0;
				selectedFile[0] = 0;
				}
				
				break;




			case ID_BUTTON_DELETE:
				switch (lastListBox)
				{
				case 1:
					SendMessage(hWndListBox1, WM_KEYDOWN, VK_F8, 0);
					break;
				case 2:
					SendMessage(hWndListBox2, WM_KEYDOWN, VK_F8, 0);
					break;
				}
				break;

			default:




				return DefWindowProc(hWnd, message, wParam, lParam);
			}


		break;







	case WM_GETMINMAXINFO: //Получили сообщение от Винды
	{
		MINMAXINFO *pInfo = (MINMAXINFO *)lParam;
		POINT Min = { 600, 600 };
		POINT  Max = { 4000, 2000 };
		pInfo->ptMinTrackSize = Min; // Установили минимальный размер
		pInfo->ptMaxTrackSize = Max; // Установили максимальный размер
		return 0;
	}


	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

HWND CreateListBox(int x,int y, int width, int height, HWND hWnd, HMENU id)
{
	HWND hWndListBox;
	LVCOLUMN column;

	hWndListBox = CreateWindow(
		WC_LISTVIEW,
		L"",
		WS_CHILD
		| WS_VISIBLE	
		| WS_BORDER		
		| ES_READONLY		
		| LVS_REPORT,	// Style Table
		x,y,
		width,height,
		hWnd,
		id,
		0,
		0);
		
	ListView_SetExtendedListViewStyle(hWndListBox,
		ListView_GetExtendedListViewStyle(hWndListBox)
		| LVS_EX_FULLROWSELECT	// Select all rows
		
		//		| LVS_EX_GRIDLINES		// view grid table
		);
	
	//--Colors--//
	ListView_SetBkColor(hWndListBox, 0x00000000);	
	ListView_SetTextColor(hWndListBox, 0x0000ff00);
	ListView_SetTextBkColor(hWndListBox, 0x00000000);


	//Add columns
	column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	column.iSubItem = 0;			
	column.pszText = L"Name";	
	column.cx = 300;              
	column.fmt = LVCFMT_LEFT;		



	ListView_InsertColumn(hWndListBox, 0, &column);

	column.iSubItem = 1;			
	column.pszText = L"Type";	
	column.cx = 100;				
	column.fmt = LVCFMT_LEFT;		

	ListView_InsertColumn(hWndListBox, 1, &column);

	column.iSubItem = 2;			// номер колонки
	column.pszText = L"Date";	// имя колонки
	column.cx = 100;				// Ширина колонки
	column.fmt = LVCFMT_LEFT;		// Прижать вправо текст
	ListView_InsertColumn(hWndListBox, 2, &column);

	return hWndListBox;
}

void LoadFileList(HWND hWndListBox, TCHAR *path)
{
	LVITEM lvi;					
	WIN32_FIND_DATA fileInfo;	
	HANDLE findFile;			
	int i, j, k, iTmp;
	LARGE_INTEGER fileSize;		
	SYSTEMTIME fileDate;		
	TCHAR cTmp[256], cTmp2[256];
	TCHAR path2[MAX_PATH];

	SendMessage(hWndListBox, LVM_DELETEALLITEMS, 0, 0);

	path2[0] = 0;
	_tcscat_s(path2, path);
	_tcscat_s(path2, _T("*"));


	memset(&lvi, 0, sizeof(lvi));	// Zero struct's Members
	lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;;			
	lvi.cchTextMax = 256;			

	findFile = FindFirstFile(path2, &fileInfo);	
	if (findFile != INVALID_HANDLE_VALUE)
	{
		i = 0;
		do
		{
		
			lvi.iItem = i;							
			lvi.iImage = i;
			lvi.iSubItem = 0;						
			lvi.pszText = fileInfo.cFileName;		
			lvi.lParam = i;
			ListView_InsertItem(hWndListBox, &lvi);

			
			if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
				{
					ListView_SetItemText(hWndListBox, i, 1, _T("<link>"));
				}
				else
				{
					ListView_SetItemText(hWndListBox, i, 1, _T("<DIR>"));
				}
			}
			else
			{
				fileSize.LowPart = fileInfo.nFileSizeLow;
				fileSize.HighPart = fileInfo.nFileSizeHigh;
				_stprintf_s(cTmp, 256, _T("%lld"), fileSize);
				iTmp = _tcslen(cTmp);
				if (iTmp > 3)
				{
					_tcscpy_s(cTmp2, cTmp);
					k = 0;
					for (j = 0; j < iTmp; j++)
					{
						if ((iTmp - j) % 3 == 0 && j)
						{
							cTmp[k] = ' ';
							k++;
						}
						cTmp[k] = cTmp2[j];
						k++;
					}
					cTmp[k] = 0;
				}
				ListView_SetItemText(hWndListBox, i, 1, cTmp);
			}
		
			FileTimeToSystemTime(&fileInfo.ftLastWriteTime, &fileDate);		
			_stprintf_s(cTmp, 256, _T("%02d.%02d.%04d %02d:%02d"), fileDate.wDay, fileDate.wMonth, fileDate.wYear, fileDate.wHour, fileDate.wMinute);
			ListView_SetItemText(hWndListBox, i, 2, cTmp);

			i++;
		} while (FindNextFile(findFile, &fileInfo));		
	}
	AddIconToListBox(hWndListBox, i, path2);
	ListView_SortItemsEx(hWndListBox, SortUpDir, hWndListBox);
}

void AddIconToListBox(HWND hWndListBox, int size, TCHAR c_dir[MAX_PATH])
{
	HIMAGELIST hSmall;
	SHFILEINFO lp;
	TCHAR buf1[MAX_PATH];
	DWORD num;

	hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_MASK | ILC_COLOR32, size + 2, 1);

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	hFind = FindFirstFile(c_dir, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		MessageBox(0, _T("Not found!"), _T("Error"), MB_OK | MB_ICONWARNING);
	}
	else
	{
		do
		{
			if (_tcscmp(FindFileData.cFileName, _T(".")) == 0)
			{
				_tcscpy_s(buf1, c_dir);
				_tcscat_s(buf1, FindFileData.cFileName);
				SHGetFileInfo(_T(""), FILE_ATTRIBUTE_DEVICE, &lp, sizeof(lp), SHGFI_ICONLOCATION | SHGFI_ICON | SHGFI_SMALLICON);

				ImageList_AddIcon(hSmall, lp.hIcon);
				DestroyIcon(lp.hIcon);

			}
			if (_tcscmp(FindFileData.cFileName, _T("..")) == 0)
			{
				_tcscpy_s(buf1, c_dir);
				_tcscat_s(buf1, FindFileData.cFileName);
				SHGetFileInfo(_T(""), FILE_ATTRIBUTE_DIRECTORY, &lp, sizeof(lp), SHGFI_ICONLOCATION | SHGFI_ICON | SHGFI_SMALLICON);

				ImageList_AddIcon(hSmall, lp.hIcon);
				DestroyIcon(lp.hIcon);
			}
			
			_tcscpy_s(buf1, c_dir);
			buf1[_tcslen(buf1) - 1] = 0;

			_tcscat_s(buf1, FindFileData.cFileName);
			num = GetFileAttributes(buf1);
			SHGetFileInfo(buf1, num, &lp, sizeof(lp), SHGFI_ICONLOCATION | SHGFI_ICON | SHGFI_SMALLICON);

			ImageList_AddIcon(hSmall, lp.hIcon);
			DestroyIcon(lp.hIcon);

		} while (FindNextFile(hFind, &FindFileData) != 0);

		FindClose(hFind);
	}
	ListView_SetImageList(hWndListBox, hSmall, LVSIL_SMALL);
}

int CALLBACK SortUpDir(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	TCHAR word1[256], word2[256];
	LVFINDINFO find;
	int i1, i2;

	if (lParam1 == 0) return -1;
	else if (lParam2 == 0) return 1;

	find.flags = LVFI_PARAM;
	find.lParam = lParam1;
	ListView_GetItemText((HWND)lParamSort,
		i1 = ListView_FindItem((HWND)lParamSort, -1, &find),
		1, word1, 256);

	find.lParam = lParam2;
	ListView_GetItemText((HWND)lParamSort,
		i2 = ListView_FindItem((HWND)lParamSort, -1, &find),
		1, word2, 256);

	if (word1[0] == '<' && word2[0] == '<')
	{
		return 0;
	}
	else if (word1[0] == '<')
	{
		return -1;
	}
	else if (word2[0] == '<')
	{
		return 1;
	}
	else return 0;
}

LRESULT CALLBACK WndProcListView1(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
		TCHAR from[MAX_PATH], to[MAX_PATH];

		_tcscpy_s(from, path1);
		_tcscat_s(from, selectedFile1);
		_tcscpy_s(to, path2);
		_tcscat_s(to, selectedFile1);

		switch (wParam)
		{
		case VK_F2:
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, DialogRename1) == 0)
			{
				LoadFileList(hWndListBox1, path1);
			}
			break;

		case VK_F5:
			if (FileOperation(from, to, FO_COPY) == 0)
			{
				LoadFileList(hWndListBox2, path2);
			}
			break;

		case VK_F6:
			if (FileOperation(from, to, FO_MOVE) == 0)
			{
				LoadFileList(hWndListBox1, path1);
				LoadFileList(hWndListBox2, path2);
			}
			break;

		case VK_F7:
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DialogCreateDir1) == 1)
			{
				LoadFileList(hWndListBox1, path1);
			}
			break;

		case VK_DELETE:
		case VK_F8:
			if (FileOperation(from, 0, FO_DELETE) == 0)
			{
				LoadFileList(hWndListBox1, path1);
			}
			break;
		default:
			break;
		}
	default:
		return CallWindowProc(origWndProcListView, hWnd, message, wParam, lParam);
	}
}

LRESULT CALLBACK WndProcListView2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_RBUTTONUP:

		break;
	case WM_KEYDOWN:
		TCHAR from[MAX_PATH], to[MAX_PATH];

		_tcscpy_s(from, path2);
		_tcscat_s(from, selectedFile2);
		_tcscpy_s(to, path1);
		_tcscat_s(to, selectedFile2);

		switch (wParam)
		{
		case VK_F2:
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, DialogRename2) == 0)
			{
				LoadFileList(hWndListBox2, path2);
			}
			break;

		case VK_F5:
			if (FileOperation(from, to, FO_COPY) == 0)
			{
				LoadFileList(hWndListBox1, path1);
			}
			break;

		case VK_F6:
			if (FileOperation(from, to, FO_MOVE) == 0)
			{
				LoadFileList(hWndListBox1, path1);
				LoadFileList(hWndListBox2, path2);
			}
			break;

		case VK_F7:
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, DialogCreateDir2) == 1)
			{
				LoadFileList(hWndListBox2, path2);
			}
			break;

		case VK_DELETE:
		case VK_F8:
			if (FileOperation(from, 0, FO_DELETE) == 0)
			{
				LoadFileList(hWndListBox2, path2);
			}
			break;

		default:
			break;
		}
	default:
		return CallWindowProc(origWndProcListView, hWnd, message, wParam, lParam);
	}
}

int FileOperation(TCHAR *from, TCHAR *to, UINT func)
{
	SHFILEOPSTRUCT shFileOpStr = { 0 };
	int i;

	i = 0;
	while (from[i]) i++;
	from[i + 1] = 0;

	if (to)
	{
		i = 0;
		while (to[i]) i++;
		to[i + 1] = 0;
	}

	shFileOpStr.hwnd = 0;
	shFileOpStr.wFunc = func;
	shFileOpStr.pFrom = from;
	shFileOpStr.pTo = to;
	shFileOpStr.fFlags = FOF_NOCONFIRMMKDIR;
	shFileOpStr.fAnyOperationsAborted = 0;
	shFileOpStr.hNameMappings = 0;
	shFileOpStr.lpszProgressTitle = 0;

	return SHFileOperation(&shFileOpStr);
}

INT_PTR CALLBACK DialogRename1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), selectedFile1);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			TCHAR from[MAX_PATH], to[MAX_PATH], buf[MAX_PATH];

			buf[GetWindowText(GetDlgItem(hDlg, IDC_EDIT1), buf, MAX_PATH)] = 0;

			_tcscpy_s(from, path1);
			_tcscat_s(from, selectedFile1);
			_tcscpy_s(to, path1);
			_tcscat_s(to, buf);

			EndDialog(hDlg, LOWORD(FileOperation(from, to, FO_RENAME)));
			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK DialogRename2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), selectedFile2);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			TCHAR from[MAX_PATH], to[MAX_PATH], buf[MAX_PATH];

			buf[GetWindowText(GetDlgItem(hDlg, IDC_EDIT1), buf, MAX_PATH)] = 0;

			_tcscpy_s(from, path2);
			_tcscat_s(from, selectedFile2);
			_tcscpy_s(to, path2);
			_tcscat_s(to, buf);

			EndDialog(hDlg, LOWORD(FileOperation(from, to, FO_RENAME)));
			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK DialogCreateDir1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			TCHAR to[MAX_PATH], buf[MAX_PATH];

			buf[GetWindowText(GetDlgItem(hDlg, IDC_EDIT1), buf, MAX_PATH)] = 0;

			_tcscpy_s(to, path1);
			_tcscat_s(to, buf);

			EndDialog(hDlg, LOWORD(CreateDirectory(to, 0)));
			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(0));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK DialogCreateDir2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			TCHAR to[MAX_PATH], buf[MAX_PATH];

			buf[GetWindowText(GetDlgItem(hDlg, IDC_EDIT1), buf, MAX_PATH)] = 0;

			_tcscpy_s(to, path2);
			_tcscat_s(to, buf);

			EndDialog(hDlg, LOWORD(CreateDirectory(to, 0)));
			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(0));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void DisplayError(TCHAR *header)
{
	TCHAR message[512];
	TCHAR buf[8];
	LPVOID lpvMessageBuffer;

	_itot_s(GetLastError(), buf, 10);
	_tcscpy_s(message, _T("Error "));
	_tcscat_s(message, buf);
	_tcscat_s(message, _T(": "));

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpvMessageBuffer, 0, NULL);

	_tcscat_s(message, (LPWSTR)lpvMessageBuffer);
	MessageBox(0, message, header, 0);
	LocalFree(lpvMessageBuffer);
}

