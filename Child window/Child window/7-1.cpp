#include <windows.h>
#include <tchar.h>
#include <atlimage.h>
#include <strsafe.h>
#include <wchar.h>
#include "resource.h"

#define MAX_IMAGE_COUNT 10
#define MOVE_STEP 20

HINSTANCE g_hInst;

LPCTSTR lpszClass = L"My Window Class";
LPCTSTR lpszChildClass = L"My Child Image Window";
LPCTSTR lpszWindowName = L"Window Programming Lab";

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

HWND g_hChildCanvas;
HWND g_hListBox;
HWND g_hEditOrder;

CImage g_image[MAX_IMAGE_COUNT];

int g_imageOrder[MAX_IMAGE_COUNT];
int g_imageCount = 0;

int g_chainX = 0;
int g_currentImage = 0;
int g_targetImage = 0;
int g_targetX = 0;

BOOL g_isDone = FALSE;
BOOL g_isMoving = FALSE;
BOOL g_isAutoPlaying = FALSE;

void LoadImages();
void DeleteImages();
void CreateControls(HWND hWnd);
void AddListBoxItems();
void AddSelectedImage(HWND hWnd, int side);
void SelectAndAddImage(HWND hWnd);
void UpdateOrderEdit();
int GetCanvasWidth();
void AlignCurrentImage();
void StartSlide(HWND hWnd, int direction);
void StartMove(HWND hWnd);
void StopMove(HWND hWnd);
void CompleteImage(HWND hWnd);
void ResetImages(HWND hWnd);
BOOL MoveImages();
void DrawChildCanvas(HWND hWnd, HDC hDC);
void DrawOneImage(HDC hDC, int imageIndex, int x, int y, int width, int height);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;
	WNDCLASSEX ChildClass;

	g_hInst = hInstance;

	WndClass.cbSize = sizeof(WNDCLASSEX);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = lpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	RegisterClassEx(&WndClass);

	ChildClass.cbSize = sizeof(WNDCLASSEX);
	ChildClass.style = CS_HREDRAW | CS_VREDRAW;
	ChildClass.lpfnWndProc = ChildWndProc;
	ChildClass.cbClsExtra = 0;
	ChildClass.cbWndExtra = 0;
	ChildClass.hInstance = hInstance;
	ChildClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	ChildClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	ChildClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	ChildClass.lpszMenuName = NULL;
	ChildClass.lpszClassName = lpszChildClass;
	ChildClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	RegisterClassEx(&ChildClass);

	hWnd = CreateWindow(
		lpszClass,
		lpszWindowName,
		WS_OVERLAPPEDWINDOW,
		100,
		100,
		1150,
		700,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&Message, 0, 0, 0))
	{
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	HBRUSH hBrush;
	int commandId;

	switch (iMessage)
	{
	case WM_CREATE:
		LoadImages();
		CreateControls(hWnd);
		UpdateOrderEdit();
		break;

	case WM_COMMAND:
		commandId = LOWORD(wParam);

		switch (commandId)
		{
		case IDC_BTN_LEFT:
			StartSlide(hWnd, -1);
			break;

		case IDC_BTN_RIGHT:
			StartSlide(hWnd, 1);
			break;

		case IDC_BTN_SELECT:
			SelectAndAddImage(hWnd);
			break;

		case IDC_BTN_MOVE:
			StartMove(hWnd);
			break;

		case IDC_BTN_STOP:
			StopMove(hWnd);
			break;

		case IDC_BTN_DONE:
			CompleteImage(hWnd);
			break;

		case IDC_BTN_RESET:
			ResetImages(hWnd);
			break;

		}
		break;

	case WM_TIMER:
		if (wParam == TIMER_MOVE)
		{
			if (MoveImages() == FALSE)
			{
				if (g_isAutoPlaying == TRUE)
				{
					if (g_currentImage < g_imageCount - 1)
					{
						g_targetImage = g_currentImage + 1;
					}
					else
					{
						// 마지막 이미지 뒤에 첫 이미지를 한 장 더 배치해
						// 화면이 끊기지 않도록 순환한다.
						g_targetImage = g_imageCount;
					}

					g_targetX = -g_targetImage * GetCanvasWidth();
					g_isMoving = TRUE;
				}
				else
				{
					g_isAutoPlaying = FALSE;
					StopMove(hWnd);
				}
			}

			InvalidateRect(g_hChildCanvas, NULL, FALSE);
		}
		break;

	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);

		hBrush = CreateSolidBrush(RGB(55, 55, 55));
		FillRect(hDC, &ps.rcPaint, hBrush);
		DeleteObject(hBrush);

		SetBkMode(hDC, TRANSPARENT);
		SetTextColor(hDC, RGB(255, 255, 255));

		TextOut(hDC, 900, 50, L"Image List", 10);
		TextOut(
			hDC,
			330,
			15,
			L"현재 선택된 이미지의 순서",
			lstrlen(L"현재 선택된 이미지의 순서")
		);

		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		KillTimer(hWnd, TIMER_MOVE);
		DeleteImages();
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	HDC hMemoryDC;
	HBITMAP hMemoryBitmap;
	HBITMAP hOldBitmap;
	RECT rect;

	switch (iMessage)
	{
	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);
		GetClientRect(hWnd, &rect);

		hMemoryDC = CreateCompatibleDC(hDC);
		hMemoryBitmap = CreateCompatibleBitmap(
			hDC,
			rect.right - rect.left,
			rect.bottom - rect.top
		);
		hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hMemoryBitmap);

		DrawChildCanvas(hWnd, hMemoryDC);
		BitBlt(
			hDC,
			0,
			0,
			rect.right - rect.left,
			rect.bottom - rect.top,
			hMemoryDC,
			0,
			0,
			SRCCOPY
		);

		SelectObject(hMemoryDC, hOldBitmap);
		DeleteObject(hMemoryBitmap);
		DeleteDC(hMemoryDC);
		EndPaint(hWnd, &ps);
		break;
	}

	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

void LoadImages()
{
	WCHAR fileName[MAX_PATH];
	WCHAR currentPath[50];
	WCHAR* filePart;
	DWORD pathLength;
	size_t remainingLength;
	int i;

	for (i = 0; i < MAX_IMAGE_COUNT; i++)
	{
		pathLength = GetModuleFileName(NULL, fileName, MAX_PATH);
		filePart = wcsrchr(fileName, L'\\');

		if (pathLength > 0 && pathLength < MAX_PATH && filePart != NULL)
		{
			filePart++;
			remainingLength = MAX_PATH - (filePart - fileName);
			StringCchPrintf(filePart, remainingLength, L"image%d.png", i + 1);
			g_image[i].Load(fileName);
		}

		if (g_image[i].IsNull() == TRUE)
		{
			StringCchPrintf(currentPath, 50, L"image%d.png", i + 1);
			g_image[i].Load(currentPath);
		}
	}
}

void DeleteImages()
{
	int i;

	for (i = 0; i < MAX_IMAGE_COUNT; i++)
	{
		if (g_image[i].IsNull() == FALSE)
		{
			g_image[i].Destroy();
		}
	}
}

void CreateControls(HWND hWnd)
{
	g_hEditOrder = CreateWindow(
		L"edit",
		L"",
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_READONLY,
		330,
		38,
		350,
		28,
		hWnd,
		(HMENU)IDC_EDIT_ORDER,
		g_hInst,
		NULL
	);

	g_hChildCanvas = CreateWindow(
		lpszChildClass,
		L"",
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		80,
		80,
		780,
		520,
		hWnd,
		(HMENU)IDC_CHILD_CANVAS,
		g_hInst,
		NULL
	);

	CreateWindow(
		L"button",
		L"<",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		25,
		315,
		45,
		45,
		hWnd,
		(HMENU)IDC_BTN_LEFT,
		g_hInst,
		NULL
	);

	CreateWindow(
		L"button",
		L">",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		875,
		315,
		45,
		45,
		hWnd,
		(HMENU)IDC_BTN_RIGHT,
		g_hInst,
		NULL
	);

	g_hListBox = CreateWindow(
		L"listbox",
		L"",
		WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
		950,
		80,
		150,
		230,
		hWnd,
		(HMENU)IDC_LIST_IMAGE,
		g_hInst,
		NULL
	);

	CreateWindow(
		L"button",
		L"Select",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		950,
		335,
		150,
		35,
		hWnd,
		(HMENU)IDC_BTN_SELECT,
		g_hInst,
		NULL
	);

	CreateWindow(
		L"button",
		L"Move",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		950,
		385,
		150,
		35,
		hWnd,
		(HMENU)IDC_BTN_MOVE,
		g_hInst,
		NULL
	);

	CreateWindow(
		L"button",
		L"Stop",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		950,
		435,
		150,
		35,
		hWnd,
		(HMENU)IDC_BTN_STOP,
		g_hInst,
		NULL
	);

	CreateWindow(
		L"button",
		L"Done",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		950,
		485,
		150,
		35,
		hWnd,
		(HMENU)IDC_BTN_DONE,
		g_hInst,
		NULL
	);

	CreateWindow(
		L"button",
		L"Reset",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		950,
		535,
		150,
		35,
		hWnd,
		(HMENU)IDC_BTN_RESET,
		g_hInst,
		NULL
	);

	AddListBoxItems();
}

void AddListBoxItems()
{
	WCHAR text[30];
	int i;

	for (i = 0; i < MAX_IMAGE_COUNT; i++)
	{
		wsprintf(text, L"Image%d", i + 1);
		SendMessage(g_hListBox, LB_ADDSTRING, 0, (LPARAM)text);
	}

	SendMessage(g_hListBox, LB_SETCURSEL, 0, 0);
}

void SelectAndAddImage(HWND hWnd)
{
	AddSelectedImage(hWnd, 1);
}

void AddSelectedImage(HWND hWnd, int side)
{
	int selectedIndex;
	int i;
	BOOL wasEmpty;

	if (g_isDone == TRUE)
	{
		MessageBox(hWnd, L"이미 완성된 상태입니다.", L"알림", MB_OK);
		return;
	}

	selectedIndex = (int)SendMessage(g_hListBox, LB_GETCURSEL, 0, 0);

	if (selectedIndex == LB_ERR)
	{
		MessageBox(hWnd, L"리스트 박스에서 이미지를 먼저 선택하세요.", L"알림", MB_OK);
		return;
	}

	if (g_imageCount >= MAX_IMAGE_COUNT)
	{
		MessageBox(hWnd, L"이미지는 최대 10개까지만 연결할 수 있습니다.", L"알림", MB_OK);
		return;
	}

	wasEmpty = (g_imageCount == 0);

	if (side < 0)
	{
		for (i = g_imageCount; i > 0; i--)
		{
			g_imageOrder[i] = g_imageOrder[i - 1];
		}

		g_imageOrder[0] = selectedIndex;
	}
	else
	{
		g_imageOrder[g_imageCount] = selectedIndex;
	}

	g_imageCount++;

	if (wasEmpty == TRUE)
	{
		g_currentImage = 0;
	}
	else if (side < 0)
	{
		g_currentImage++;
	}

	StopMove(hWnd);
	AlignCurrentImage();
	UpdateOrderEdit();
	InvalidateRect(g_hChildCanvas, NULL, TRUE);
}

void UpdateOrderEdit()
{
	WCHAR text[512];
	WCHAR temp[50];
	int i;

	if (g_imageCount == 0)
	{
		lstrcpy(text, L"이미지 없음");
	}
	else
	{
		lstrcpy(text, L"");

		for (i = 0; i < g_imageCount; i++)
		{
			wsprintf(temp, L"Image%d", g_imageOrder[i] + 1);
			lstrcat(text, temp);

			if (i < g_imageCount - 1)
			{
				lstrcat(text, L" -> ");
			}
		}
	}

	SetWindowText(g_hEditOrder, text);
}

int GetCanvasWidth()
{
	RECT rect;

	GetClientRect(g_hChildCanvas, &rect);
	return rect.right - rect.left;
}

void AlignCurrentImage()
{
	int canvasWidth;

	canvasWidth = GetCanvasWidth();
	g_chainX = -g_currentImage * canvasWidth;
	g_targetImage = g_currentImage;
	g_targetX = g_chainX;
	g_isMoving = FALSE;
	g_isAutoPlaying = FALSE;
}

void StartSlide(HWND hWnd, int direction)
{
	int canvasWidth;
	int nextImage;

	if (g_imageCount == 0)
	{
		MessageBox(hWnd, L"Select 버튼으로 이미지를 먼저 선택하세요.", L"알림", MB_OK);
		return;
	}

	if (g_isMoving == TRUE)
	{
		StopMove(hWnd);
		AlignCurrentImage();
	}

	g_isAutoPlaying = FALSE;
	nextImage = g_currentImage + direction;

	if (nextImage < 0)
	{
		MessageBox(hWnd, L"첫 번째 이미지입니다.", L"알림", MB_OK);
		return;
	}

	if (nextImage >= g_imageCount)
	{
		MessageBox(hWnd, L"마지막 이미지입니다.", L"알림", MB_OK);
		return;
	}

	canvasWidth = GetCanvasWidth();
	g_targetImage = nextImage;
	g_targetX = -g_targetImage * canvasWidth;
	g_isMoving = TRUE;

	SetTimer(hWnd, TIMER_MOVE, 30, NULL);
}

void StartMove(HWND hWnd)
{
	int canvasWidth;

	if (g_imageCount == 0)
	{
		MessageBox(hWnd, L"먼저 이미지를 연결하세요.", L"알림", MB_OK);
		return;
	}

	if (g_isDone == FALSE)
	{
		MessageBox(hWnd, L"Done 버튼으로 이미지 연결을 먼저 완성하세요.", L"알림", MB_OK);
		return;
	}

	if (g_isAutoPlaying == TRUE && g_isMoving == TRUE)
	{
		SetTimer(hWnd, TIMER_MOVE, 30, NULL);
		return;
	}

	g_currentImage = 0;
	AlignCurrentImage();

	canvasWidth = GetCanvasWidth();
	g_targetImage = (g_imageCount > 1) ? 1 : g_imageCount;
	g_targetX = -g_targetImage * canvasWidth;
	g_isMoving = TRUE;
	g_isAutoPlaying = TRUE;

	InvalidateRect(g_hChildCanvas, NULL, FALSE);
	SetTimer(hWnd, TIMER_MOVE, 30, NULL);
}

void StopMove(HWND hWnd)
{
	KillTimer(hWnd, TIMER_MOVE);
}

void CompleteImage(HWND hWnd)
{
	WCHAR orderText[512];
	WCHAR message[700];

	if (g_imageCount == 0)
	{
		MessageBox(hWnd, L"먼저 이미지를 하나 이상 연결하세요.", L"알림", MB_OK);
		return;
	}

	if (g_isDone == TRUE)
	{
		MessageBox(hWnd, L"이미지 연결이 이미 완성되었습니다.", L"알림", MB_OK);
		return;
	}

	StopMove(hWnd);
	g_currentImage = 0;
	AlignCurrentImage();
	InvalidateRect(g_hChildCanvas, NULL, FALSE);

	g_isDone = TRUE;

	GetWindowText(g_hEditOrder, orderText, 512);

	lstrcpy(message, L"이미지 연결이 완성되었습니다.\n\n현재 순서:\n");
	lstrcat(message, orderText);

	MessageBox(hWnd, message, L"완성", MB_OK);

	EnableWindow(g_hListBox, FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_BTN_SELECT), FALSE);
	EnableWindow(GetDlgItem(hWnd, IDC_BTN_DONE), FALSE);
}

void ResetImages(HWND hWnd)
{
	int i;

	StopMove(hWnd);

	for (i = 0; i < MAX_IMAGE_COUNT; i++)
	{
		g_imageOrder[i] = 0;
	}

	g_imageCount = 0;
	g_chainX = 0;
	g_currentImage = 0;
	g_targetImage = 0;
	g_targetX = 0;
	g_isDone = FALSE;
	g_isMoving = FALSE;
	g_isAutoPlaying = FALSE;

	SendMessage(g_hListBox, LB_SETCURSEL, 0, 0);
	EnableWindow(g_hListBox, TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_BTN_SELECT), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDC_BTN_DONE), TRUE);

	UpdateOrderEdit();
	InvalidateRect(g_hChildCanvas, NULL, FALSE);
}

BOOL MoveImages()
{
	if (g_imageCount == 0 || g_isMoving == FALSE)
	{
		return FALSE;
	}

	if (g_chainX < g_targetX)
	{
		g_chainX += MOVE_STEP;

		if (g_chainX >= g_targetX)
		{
			g_chainX = g_targetX;
		}
	}
	else
	{
		g_chainX -= MOVE_STEP;

		if (g_chainX <= g_targetX)
		{
			g_chainX = g_targetX;
		}
	}

	if (g_chainX == g_targetX)
	{
		if (g_isAutoPlaying == TRUE && g_targetImage == g_imageCount)
		{
			g_currentImage = 0;
			g_targetImage = 0;
			g_chainX = 0;
			g_targetX = 0;
		}
		else
		{
			g_currentImage = g_targetImage;
		}

		g_isMoving = FALSE;
		return FALSE;
	}

	return TRUE;
}

void DrawChildCanvas(HWND hWnd, HDC hDC)
{
	RECT rect;
	HBRUSH hBrush;
	int i;
	int x;
	int imageIndex;
	int canvasWidth;
	int canvasHeight;

	GetClientRect(hWnd, &rect);

	hBrush = CreateSolidBrush(RGB(230, 230, 230));
	FillRect(hDC, &rect, hBrush);
	DeleteObject(hBrush);

	SetBkMode(hDC, TRANSPARENT);
	SetTextColor(hDC, RGB(0, 0, 0));

	canvasWidth = rect.right - rect.left;
	canvasHeight = rect.bottom - rect.top;

	if (g_imageCount == 0)
	{
		TextOut(
			hDC,
			280,
			240,
			L"리스트에서 이미지를 선택하세요.",
			lstrlen(L"리스트에서 이미지를 선택하세요.")
		);
		return;
	}

	for (i = 0; i <= g_imageCount; i++)
	{
		imageIndex = g_imageOrder[i % g_imageCount];
		x = g_chainX + i * canvasWidth;

		if (x < canvasWidth && x + canvasWidth > 0)
		{
			DrawOneImage(hDC, imageIndex, x, 0, canvasWidth, canvasHeight);
		}
	}
}

void DrawOneImage(HDC hDC, int imageIndex, int x, int y, int width, int height)
{
	WCHAR text[50];
	RECT imageRect;
	HBRUSH hBrush;

	SetRect(&imageRect, x, y, x + width, y + height);

	if (g_image[imageIndex].IsNull() == TRUE)
	{
		hBrush = CreateSolidBrush(RGB(255, 255, 255));
		FillRect(hDC, &imageRect, hBrush);
		DeleteObject(hBrush);
		FrameRect(hDC, &imageRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

		wsprintf(text, L"image%d.png 없음", imageIndex + 1);
		TextOut(hDC, x + 25, y + 45, text, lstrlen(text));

		return;
	}

	g_image[imageIndex].Draw(hDC, x, y, width, height);

	FrameRect(hDC, &imageRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
}
