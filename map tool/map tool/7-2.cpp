#include <windows.h>
#include <windowsx.h>
#include <strsafe.h>

constexpr int kGridCount = 15;
constexpr int kMapCount = 3;
constexpr int kBackgroundCount = 4;
constexpr int kPlatformCount = 6;
constexpr int kCanvasSize = 600;
constexpr int kCellSize = kCanvasSize / kGridCount;
constexpr int kPlayerSize = 24;
constexpr int kMoveSpeed = 5;
constexpr int kGravity = 1;
constexpr int kJumpSpeed = -14;
constexpr int kMaximumFallSpeed = 12;
constexpr UINT_PTR kFrameTimer = 1;

constexpr int ID_CANVAS = 1000;
constexpr int ID_BACKGROUND_LIST = 1001;
constexpr int ID_STATUS = 1002;
constexpr int ID_MAP_LEFT = 1101;
constexpr int ID_MAP_RIGHT = 1102;
constexpr int ID_ERASE = 1103;
constexpr int ID_TEST = 1104;
constexpr int ID_PLATFORM_FIRST = 1201;
constexpr int ID_PLATFORM_LAST = ID_PLATFORM_FIRST + kPlatformCount - 1;

struct MapData
{
	int background;
	int platform[kGridCount][kGridCount];
};

HINSTANCE g_instance;
HWND g_mainWindow;
HWND g_canvas;
HWND g_backgroundList;
HWND g_status;
HWND g_eraseButton;
HWND g_testButton;
HWND g_platformButtons[kPlatformCount];

MapData g_maps[kMapCount] = {};
int g_currentMap = 0;
int g_nextMap = 0;
int g_selectedPlatform = 1;
int g_slideDirection = 0;
int g_slideDistance = 0;
int g_playerX = 25;
int g_playerY = kCanvasSize - kPlayerSize - 25;
int g_playerVelocityY = 0;
int g_lastPaintColumn = -1;
int g_lastPaintRow = -1;

bool g_eraseMode = false;
bool g_testMode = false;
bool g_leftPressed = false;
bool g_rightPressed = false;
bool g_upPressed = false;
bool g_downPressed = false;
bool g_dragging = false;
bool g_dragErase = false;

const wchar_t* kMainClass = L"MapToolMainWindow";
const wchar_t* kCanvasClass = L"MapToolCanvasWindow";
const wchar_t* kBackgroundNames[kBackgroundCount] = {
	L"초원", L"사막", L"설원", L"밤하늘"
};

LRESULT CALLBACK MainWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CanvasWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void InitializeMaps();
void CreateControls(HWND window);
void UpdateUi();
void SelectPlatform(int platform);
void ToggleEraseMode();
void ToggleTestMode();
void PlacePlayerAtOpenCell();
void BeginMapSlide(int direction);
void AdvanceFrame();
void BeginCanvasDrag(HWND window, int x, int y, bool erase);
void ContinueCanvasDrag(int x, int y);
void EndCanvasDrag(HWND window);
void PaintCanvasCell(int x, int y);
bool RectHitsPlatform(int mapIndex, int left, int top, int right, int bottom);
bool PlayerHasSupport();
void TryJump();
void MovePlayerHorizontal(int distance);
void MovePlayerVertical(int distance);
void PaintCanvas(HWND window, HDC targetDc);
void DrawMap(HDC dc, int mapIndex, int originX, bool showGrid);
void DrawBackground(HDC dc, int type, int originX);
void DrawPlatform(HDC dc, int type, const RECT& rect);
void DrawPlatformButton(const DRAWITEMSTRUCT* item);
void DrawPlayer(HDC dc);

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
	g_instance = instance;

	WNDCLASSEX mainClass = {};
	mainClass.cbSize = sizeof(mainClass);
	mainClass.style = CS_HREDRAW | CS_VREDRAW;
	mainClass.lpfnWndProc = MainWindowProc;
	mainClass.hInstance = instance;
	mainClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	mainClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	mainClass.hbrBackground = CreateSolidBrush(RGB(242, 245, 248));
	mainClass.lpszClassName = kMainClass;
	mainClass.hIconSm = mainClass.hIcon;
	RegisterClassEx(&mainClass);

	WNDCLASSEX canvasClass = {};
	canvasClass.cbSize = sizeof(canvasClass);
	canvasClass.style = CS_HREDRAW | CS_VREDRAW;
	canvasClass.lpfnWndProc = CanvasWindowProc;
	canvasClass.hInstance = instance;
	canvasClass.hCursor = LoadCursor(nullptr, IDC_CROSS);
	canvasClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	canvasClass.lpszClassName = kCanvasClass;
	RegisterClassEx(&canvasClass);

	g_mainWindow = CreateWindow(
		kMainClass,
		L"15 x 15 순환 맵 에디터",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1025,
		705,
		nullptr,
		nullptr,
		instance,
		nullptr
	);

	if (g_mainWindow == nullptr)
	{
		return 0;
	}

	ShowWindow(g_mainWindow, showCommand);
	UpdateWindow(g_mainWindow);

	MSG message;
	while (GetMessage(&message, nullptr, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	return static_cast<int>(message.wParam);
}

LRESULT CALLBACK MainWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		InitializeMaps();
		CreateControls(window);
		UpdateUi();
		SetTimer(window, kFrameTimer, 16, nullptr);
		return 0;

	case WM_COMMAND:
	{
		const int id = LOWORD(wParam);

		if (id >= ID_PLATFORM_FIRST && id <= ID_PLATFORM_LAST)
		{
			SelectPlatform(id - ID_PLATFORM_FIRST + 1);
			SetFocus(window);
			return 0;
		}

		switch (id)
		{
		case ID_MAP_LEFT:
			BeginMapSlide(-1);
			break;

		case ID_MAP_RIGHT:
			BeginMapSlide(1);
			break;

		case ID_ERASE:
			ToggleEraseMode();
			break;

		case ID_TEST:
			ToggleTestMode();
			break;

		case ID_BACKGROUND_LIST:
			if (HIWORD(wParam) == LBN_SELCHANGE && !g_testMode)
			{
				const int selection = static_cast<int>(
					SendMessage(g_backgroundList, LB_GETCURSEL, 0, 0)
				);

				if (selection != LB_ERR)
				{
					g_maps[g_currentMap].background = selection;
					InvalidateRect(g_canvas, nullptr, FALSE);
					UpdateUi();
				}
			}
			break;
		}

		SetFocus(window);
		return 0;
	}

	case WM_DRAWITEM:
		if (wParam >= ID_PLATFORM_FIRST && wParam <= ID_PLATFORM_LAST)
		{
			DrawPlatformButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
			return TRUE;
		}
		break;

	case WM_KEYDOWN:
		if (g_testMode)
		{
			if (wParam == VK_LEFT || wParam == 'A')
			{
				g_leftPressed = true;
			}
			else if (wParam == VK_RIGHT || wParam == 'D')
			{
				g_rightPressed = true;
			}
			else if (wParam == VK_UP || wParam == 'W')
			{
				if (!g_upPressed)
				{
					TryJump();
				}
				g_upPressed = true;
			}
			else if (wParam == VK_DOWN || wParam == 'S')
			{
				g_downPressed = true;
			}
		}
		return 0;

	case WM_KEYUP:
		if (wParam == VK_LEFT || wParam == 'A')
		{
			g_leftPressed = false;
		}
		else if (wParam == VK_RIGHT || wParam == 'D')
		{
			g_rightPressed = false;
		}
		else if (wParam == VK_UP || wParam == 'W')
		{
			g_upPressed = false;
		}
		else if (wParam == VK_DOWN || wParam == 'S')
		{
			g_downPressed = false;
		}
		return 0;

	case WM_KILLFOCUS:
		g_leftPressed = false;
		g_rightPressed = false;
		g_upPressed = false;
		g_downPressed = false;
		return 0;

	case WM_TIMER:
		if (wParam == kFrameTimer)
		{
			AdvanceFrame();
		}
		return 0;

	case WM_DESTROY:
		KillTimer(window, kFrameTimer);
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(window, message, wParam, lParam);
}

LRESULT CALLBACK CanvasWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ERASEBKGND:
		return 1;

	case WM_LBUTTONDOWN:
		SetFocus(g_mainWindow);
		BeginCanvasDrag(
			window,
			GET_X_LPARAM(lParam),
			GET_Y_LPARAM(lParam),
			g_eraseMode
		);
		return 0;

	case WM_RBUTTONDOWN:
		SetFocus(g_mainWindow);
		BeginCanvasDrag(
			window,
			GET_X_LPARAM(lParam),
			GET_Y_LPARAM(lParam),
			true
		);
		return 0;

	case WM_MOUSEMOVE:
		if (g_dragging)
		{
			ContinueCanvasDrag(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		return 0;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		EndCanvasDrag(window);
		return 0;

	case WM_CAPTURECHANGED:
		g_dragging = false;
		g_lastPaintColumn = -1;
		g_lastPaintRow = -1;
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC dc = BeginPaint(window, &paint);
		PaintCanvas(window, dc);
		EndPaint(window, &paint);
		return 0;
	}
	}

	return DefWindowProc(window, message, wParam, lParam);
}

void InitializeMaps()
{
	for (int map = 0; map < kMapCount; ++map)
	{
		g_maps[map].background = map;

		for (int row = 0; row < kGridCount; ++row)
		{
			for (int column = 0; column < kGridCount; ++column)
			{
				g_maps[map].platform[row][column] = 0;
			}
		}
	}
}

void CreateControls(HWND window)
{
	g_canvas = CreateWindow(
		kCanvasClass,
		L"",
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		55,
		40,
		kCanvasSize + 2,
		kCanvasSize + 2,
		window,
		reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_CANVAS)),
		g_instance,
		nullptr
	);

	CreateWindow(
		L"button", L"<", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		8, 300, 38, 58, window,
		reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_MAP_LEFT)),
		g_instance, nullptr
	);

	CreateWindow(
		L"button", L">", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		666, 300, 38, 58, window,
		reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_MAP_RIGHT)),
		g_instance, nullptr
	);

	CreateWindow(
		L"static", L"발판 이미지", WS_CHILD | WS_VISIBLE,
		730, 30, 220, 24, window, nullptr, g_instance, nullptr
	);

	for (int i = 0; i < kPlatformCount; ++i)
	{
		const int column = i % 3;
		const int row = i / 3;

		g_platformButtons[i] = CreateWindow(
			L"button",
			L"",
			WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			730 + column * 78,
			60 + row * 66,
			68,
			54,
			window,
			reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_PLATFORM_FIRST + i)),
			g_instance,
			nullptr
		);
	}

	CreateWindow(
		L"static", L"배경 선택", WS_CHILD | WS_VISIBLE,
		730, 200, 220, 24, window, nullptr, g_instance, nullptr
	);

	g_backgroundList = CreateWindow(
		L"listbox",
		L"",
		WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
		730,
		230,
		220,
		125,
		window,
		reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BACKGROUND_LIST)),
		g_instance,
		nullptr
	);

	for (const wchar_t* name : kBackgroundNames)
	{
		SendMessage(g_backgroundList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
	}

	g_eraseButton = CreateWindow(
		L"button",
		L"발판 취소",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		730,
		385,
		220,
		42,
		window,
		reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_ERASE)),
		g_instance,
		nullptr
	);

	g_testButton = CreateWindow(
		L"button",
		L"테스트 시작",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		730,
		450,
		220,
		52,
		window,
		reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_TEST)),
		g_instance,
		nullptr
	);

	g_status = CreateWindow(
		L"static",
		L"",
		WS_CHILD | WS_VISIBLE | SS_CENTER,
		720,
		535,
		240,
		70,
		window,
		reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_STATUS)),
		g_instance,
		nullptr
	);
}

void UpdateUi()
{
	wchar_t text[160];

	if (g_testMode)
	{
		StringCchPrintf(
			text,
			160,
			L"테스트 모드 | 맵 %d / %d\nA/D·←/→ 이동, W·↑ 점프, S·↓ 빠른 낙하",
			g_currentMap + 1,
			kMapCount
		);
	}
	else
	{
		StringCchPrintf(
			text,
			160,
			L"편집 모드 | 맵 %d / %d\n%s",
			g_currentMap + 1,
			kMapCount,
			g_eraseMode ? L"칸을 누르면 발판을 지웁니다." : L"발판 선택 후 그리드를 누르세요."
		);
	}

	SetWindowText(g_status, text);
	SendMessage(
		g_backgroundList,
		LB_SETCURSEL,
		g_maps[g_currentMap].background,
		0
	);
}

void SelectPlatform(int platform)
{
	if (g_testMode)
	{
		return;
	}

	g_selectedPlatform = platform;
	g_eraseMode = false;
	SetWindowText(g_eraseButton, L"발판 취소");

	for (HWND button : g_platformButtons)
	{
		InvalidateRect(button, nullptr, TRUE);
	}

	UpdateUi();
}

void ToggleEraseMode()
{
	if (g_testMode)
	{
		return;
	}

	g_eraseMode = !g_eraseMode;
	SetWindowText(g_eraseButton, g_eraseMode ? L"취소 모드 종료" : L"발판 취소");
	UpdateUi();
}

void ToggleTestMode()
{
	g_testMode = !g_testMode;
	g_leftPressed = false;
	g_rightPressed = false;
	g_upPressed = false;
	g_downPressed = false;
	g_playerVelocityY = 0;
	g_dragging = false;
	g_slideDirection = 0;
	g_slideDistance = 0;

	if (g_testMode)
	{
		PlacePlayerAtOpenCell();
		SetWindowText(g_testButton, L"편집으로 돌아가기");
		EnableWindow(g_backgroundList, FALSE);
		EnableWindow(g_eraseButton, FALSE);

		for (HWND button : g_platformButtons)
		{
			EnableWindow(button, FALSE);
		}
	}
	else
	{
		SetWindowText(g_testButton, L"테스트 시작");
		EnableWindow(g_backgroundList, TRUE);
		EnableWindow(g_eraseButton, TRUE);

		for (HWND button : g_platformButtons)
		{
			EnableWindow(button, TRUE);
		}
	}

	UpdateUi();
	InvalidateRect(g_canvas, nullptr, FALSE);
}

void PlacePlayerAtOpenCell()
{
	for (int row = 0; row < kGridCount; ++row)
	{
		for (int column = 0; column < kGridCount; ++column)
		{
			const int x = column * kCellSize + (kCellSize - kPlayerSize) / 2;
			const int y = row * kCellSize + (kCellSize - kPlayerSize) / 2;

			if (!RectHitsPlatform(
				g_currentMap,
				x,
				y,
				x + kPlayerSize,
				y + kPlayerSize
			))
			{
				g_playerX = x;
				g_playerY = y;
				return;
			}
		}
	}

	g_playerX = 0;
	g_playerY = 0;
}

void BeginMapSlide(int direction)
{
	if (g_slideDirection != 0)
	{
		return;
	}

	g_slideDirection = direction;
	g_slideDistance = 0;
	g_nextMap = (g_currentMap + direction + kMapCount) % kMapCount;
}

void AdvanceFrame()
{
	bool changed = false;

	if (g_slideDirection != 0)
	{
		g_slideDistance += 30;

		if (g_slideDistance >= kCanvasSize)
		{
			g_currentMap = g_nextMap;
			g_slideDirection = 0;
			g_slideDistance = 0;
			UpdateUi();
		}

		changed = true;
	}
	else if (g_testMode)
	{
		if (g_leftPressed != g_rightPressed)
		{
			MovePlayerHorizontal(g_rightPressed ? kMoveSpeed : -kMoveSpeed);
			changed = true;
		}

		g_playerVelocityY += kGravity;
		if (g_downPressed && g_playerVelocityY > 0)
		{
			g_playerVelocityY += 2;
		}
		if (g_playerVelocityY > kMaximumFallSpeed)
		{
			g_playerVelocityY = kMaximumFallSpeed;
		}
		MovePlayerVertical(g_playerVelocityY);
		changed = true;
	}

	if (changed)
	{
		InvalidateRect(g_canvas, nullptr, FALSE);
	}
}

void BeginCanvasDrag(HWND window, int x, int y, bool erase)
{
	if (g_testMode || g_slideDirection != 0)
	{
		return;
	}

	g_dragging = true;
	g_dragErase = erase;
	g_lastPaintColumn = -1;
	g_lastPaintRow = -1;
	SetCapture(window);
	PaintCanvasCell(x, y);
}

void ContinueCanvasDrag(int x, int y)
{
	if (!g_dragging || g_testMode || g_slideDirection != 0)
	{
		return;
	}

	PaintCanvasCell(x, y);
}

void EndCanvasDrag(HWND window)
{
	if (!g_dragging)
	{
		return;
	}

	g_dragging = false;
	g_lastPaintColumn = -1;
	g_lastPaintRow = -1;

	if (GetCapture() == window)
	{
		ReleaseCapture();
	}
}

void PaintCanvasCell(int x, int y)
{
	const int column = x / kCellSize;
	const int row = y / kCellSize;

	if (
		x < 0 || y < 0 ||
		column < 0 || column >= kGridCount ||
		row < 0 || row >= kGridCount ||
		(column == g_lastPaintColumn && row == g_lastPaintRow)
	)
	{
		return;
	}

	g_lastPaintColumn = column;
	g_lastPaintRow = row;
	g_maps[g_currentMap].platform[row][column] =
		g_dragErase ? 0 : g_selectedPlatform;
	InvalidateRect(g_canvas, nullptr, FALSE);
}

bool RectHitsPlatform(int mapIndex, int left, int top, int right, int bottom)
{
	if (
		right <= 0 || left >= kCanvasSize ||
		bottom <= 0 || top >= kCanvasSize
	)
	{
		return false;
	}

	const int clippedLeft = max(0, left);
	const int clippedTop = max(0, top);
	const int clippedRight = min(kCanvasSize, right);
	const int clippedBottom = min(kCanvasSize, bottom);
	const int firstColumn = clippedLeft / kCellSize;
	const int lastColumn = (clippedRight - 1) / kCellSize;
	const int firstRow = clippedTop / kCellSize;
	const int lastRow = (clippedBottom - 1) / kCellSize;

	for (int row = firstRow; row <= lastRow; ++row)
	{
		for (int column = firstColumn; column <= lastColumn; ++column)
		{
			if (g_maps[mapIndex].platform[row][column] != 0)
			{
				return true;
			}
		}
	}

	return false;
}

bool PlayerHasSupport()
{
	if (g_playerY >= kCanvasSize - kPlayerSize)
	{
		return true;
	}

	return RectHitsPlatform(
		g_currentMap,
		g_playerX,
		g_playerY + 1,
		g_playerX + kPlayerSize,
		g_playerY + kPlayerSize + 1
	);
}

void TryJump()
{
	if (g_testMode && PlayerHasSupport())
	{
		g_playerVelocityY = kJumpSpeed;
	}
}

void MovePlayerHorizontal(int distance)
{
	const int direction = distance < 0 ? -1 : 1;
	const int steps = distance < 0 ? -distance : distance;

	for (int step = 0; step < steps; ++step)
	{
		const int nextX = g_playerX + direction;

		if (RectHitsPlatform(
			g_currentMap,
			nextX,
			g_playerY,
			nextX + kPlayerSize,
			g_playerY + kPlayerSize
		))
		{
			return;
		}

		g_playerX = nextX;

		if (g_playerX + kPlayerSize < 0)
		{
			g_currentMap = (g_currentMap - 1 + kMapCount) % kMapCount;
			g_playerX = kCanvasSize - 1;
			UpdateUi();
		}
		else if (g_playerX > kCanvasSize)
		{
			g_currentMap = (g_currentMap + 1) % kMapCount;
			g_playerX = -kPlayerSize + 1;
			UpdateUi();
		}
	}
}

void MovePlayerVertical(int distance)
{
	if (distance == 0)
	{
		return;
	}

	const int direction = distance < 0 ? -1 : 1;
	const int steps = distance < 0 ? -distance : distance;

	for (int step = 0; step < steps; ++step)
	{
		const int nextY = g_playerY + direction;

		if (nextY < 0)
		{
			g_playerY = 0;
			g_playerVelocityY = 0;
			return;
		}

		if (nextY > kCanvasSize - kPlayerSize)
		{
			g_playerY = kCanvasSize - kPlayerSize;
			g_playerVelocityY = 0;
			return;
		}

		if (RectHitsPlatform(
			g_currentMap,
			g_playerX,
			nextY,
			g_playerX + kPlayerSize,
			nextY + kPlayerSize
		))
		{
			g_playerVelocityY = 0;
			return;
		}

		g_playerY = nextY;
	}
}

void PaintCanvas(HWND, HDC targetDc)
{
	HDC memoryDc = CreateCompatibleDC(targetDc);
	HBITMAP bitmap = CreateCompatibleBitmap(targetDc, kCanvasSize, kCanvasSize);
	HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memoryDc, bitmap));

	if (g_slideDirection == 0)
	{
		DrawMap(memoryDc, g_currentMap, 0, !g_testMode);
	}
	else if (g_slideDirection > 0)
	{
		DrawMap(memoryDc, g_currentMap, -g_slideDistance, !g_testMode);
		DrawMap(memoryDc, g_nextMap, kCanvasSize - g_slideDistance, !g_testMode);
	}
	else
	{
		DrawMap(memoryDc, g_currentMap, g_slideDistance, !g_testMode);
		DrawMap(memoryDc, g_nextMap, -kCanvasSize + g_slideDistance, !g_testMode);
	}

	if (g_testMode && g_slideDirection == 0)
	{
		DrawPlayer(memoryDc);
	}

	BitBlt(targetDc, 0, 0, kCanvasSize, kCanvasSize, memoryDc, 0, 0, SRCCOPY);
	SelectObject(memoryDc, oldBitmap);
	DeleteObject(bitmap);
	DeleteDC(memoryDc);
}

void DrawMap(HDC dc, int mapIndex, int originX, bool showGrid)
{
	DrawBackground(dc, g_maps[mapIndex].background, originX);

	for (int row = 0; row < kGridCount; ++row)
	{
		for (int column = 0; column < kGridCount; ++column)
		{
			const int type = g_maps[mapIndex].platform[row][column];

			if (type == 0)
			{
				continue;
			}

			RECT tile = {
				originX + column * kCellSize,
				row * kCellSize,
				originX + (column + 1) * kCellSize,
				(row + 1) * kCellSize
			};
			DrawPlatform(dc, type, tile);
		}
	}

	if (showGrid)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(44, 86, 91));
		HPEN oldPen = static_cast<HPEN>(SelectObject(dc, pen));

		for (int i = 0; i <= kGridCount; ++i)
		{
			const int position = i * kCellSize;
			MoveToEx(dc, originX + position, 0, nullptr);
			LineTo(dc, originX + position, kCanvasSize);
			MoveToEx(dc, originX, position, nullptr);
			LineTo(dc, originX + kCanvasSize, position);
		}

		SelectObject(dc, oldPen);
		DeleteObject(pen);
	}
}

void DrawBackground(HDC dc, int type, int originX)
{
	COLORREF top;
	COLORREF bottom;

	switch (type)
	{
	case 1:
		top = RGB(255, 173, 86);
		bottom = RGB(229, 192, 105);
		break;
	case 2:
		top = RGB(122, 207, 244);
		bottom = RGB(225, 241, 248);
		break;
	case 3:
		top = RGB(29, 39, 86);
		bottom = RGB(88, 66, 131);
		break;
	default:
		top = RGB(91, 211, 225);
		bottom = RGB(112, 202, 124);
		break;
	}

	for (int y = 0; y < kCanvasSize; ++y)
	{
		const int red = GetRValue(top) +
			(GetRValue(bottom) - GetRValue(top)) * y / kCanvasSize;
		const int green = GetGValue(top) +
			(GetGValue(bottom) - GetGValue(top)) * y / kCanvasSize;
		const int blue = GetBValue(top) +
			(GetBValue(bottom) - GetBValue(top)) * y / kCanvasSize;
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(red, green, blue));
		HPEN oldPen = static_cast<HPEN>(SelectObject(dc, pen));
		MoveToEx(dc, originX, y, nullptr);
		LineTo(dc, originX + kCanvasSize, y);
		SelectObject(dc, oldPen);
		DeleteObject(pen);
	}

	if (type == 3)
	{
		HBRUSH starBrush = CreateSolidBrush(RGB(249, 244, 188));
		HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(dc, starBrush));
		for (int i = 0; i < 24; ++i)
		{
			const int x = originX + (i * 97 + 31) % kCanvasSize;
			const int y = (i * 53 + 21) % 330;
			Ellipse(dc, x, y, x + 3, y + 3);
		}
		SelectObject(dc, oldBrush);
		DeleteObject(starBrush);
	}

	POINT hills[7] = {
		{ originX, 470 },
		{ originX + 90, 370 },
		{ originX + 180, 455 },
		{ originX + 300, 330 },
		{ originX + 405, 445 },
		{ originX + 520, 355 },
		{ originX + kCanvasSize, 440 }
	};

	COLORREF hillColor = type == 1 ? RGB(177, 96, 53) :
		type == 2 ? RGB(190, 218, 230) :
		type == 3 ? RGB(52, 50, 94) : RGB(67, 157, 95);
	HBRUSH hillBrush = CreateSolidBrush(hillColor);
	HPEN hillPen = CreatePen(PS_SOLID, 1, hillColor);
	HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(dc, hillBrush));
	HPEN oldPen = static_cast<HPEN>(SelectObject(dc, hillPen));
	Polygon(dc, hills, 7);
	RECT ground = { originX, 430, originX + kCanvasSize, kCanvasSize };
	FillRect(dc, &ground, hillBrush);
	SelectObject(dc, oldPen);
	SelectObject(dc, oldBrush);
	DeleteObject(hillPen);
	DeleteObject(hillBrush);
}

void DrawPlatform(HDC dc, int type, const RECT& rect)
{
	COLORREF body;
	COLORREF top;

	switch (type)
	{
	case 1:
		body = RGB(126, 78, 42);
		top = RGB(67, 190, 58);
		break;
	case 2:
		body = RGB(175, 103, 45);
		top = RGB(239, 169, 73);
		break;
	case 3:
		body = RGB(90, 99, 113);
		top = RGB(183, 193, 204);
		break;
	case 4:
		body = RGB(78, 164, 217);
		top = RGB(211, 248, 255);
		break;
	case 5:
		body = RGB(111, 42, 35);
		top = RGB(255, 105, 24);
		break;
	default:
		body = RGB(102, 62, 149);
		top = RGB(246, 211, 63);
		break;
	}

	HBRUSH bodyBrush = CreateSolidBrush(body);
	FillRect(dc, &rect, bodyBrush);
	DeleteObject(bodyBrush);

	RECT topRect = rect;
	topRect.bottom = topRect.top + max(6, (rect.bottom - rect.top) / 4);
	HBRUSH topBrush = CreateSolidBrush(top);
	FillRect(dc, &topRect, topBrush);
	DeleteObject(topBrush);

	HPEN detailPen = CreatePen(PS_SOLID, 1, RGB(61, 46, 40));
	HPEN oldPen = static_cast<HPEN>(SelectObject(dc, detailPen));
	MoveToEx(dc, rect.left, topRect.bottom, nullptr);
	LineTo(dc, rect.right, topRect.bottom);
	MoveToEx(dc, rect.left + 8, topRect.bottom + 5, nullptr);
	LineTo(dc, rect.left + 15, rect.bottom - 4);
	MoveToEx(dc, rect.right - 8, topRect.bottom + 5, nullptr);
	LineTo(dc, rect.right - 15, rect.bottom - 4);
	SelectObject(dc, oldPen);
	DeleteObject(detailPen);

	FrameRect(dc, &rect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
}

void DrawPlatformButton(const DRAWITEMSTRUCT* item)
{
	RECT rect = item->rcItem;
	const int type = static_cast<int>(item->CtlID) - ID_PLATFORM_FIRST + 1;
	const bool selected = type == g_selectedPlatform && !g_eraseMode;
	const bool pressed = (item->itemState & ODS_SELECTED) != 0;

	HBRUSH background = CreateSolidBrush(
		selected ? RGB(255, 231, 145) :
		pressed ? RGB(210, 216, 222) : RGB(246, 248, 250)
	);
	FillRect(item->hDC, &rect, background);
	DeleteObject(background);

	RECT icon = rect;
	InflateRect(&icon, -10, -9);
	DrawPlatform(item->hDC, type, icon);
	FrameRect(
		item->hDC,
		&rect,
		static_cast<HBRUSH>(GetStockObject(selected ? BLACK_BRUSH : GRAY_BRUSH))
	);
}

void DrawPlayer(HDC dc)
{
	RECT body = {
		g_playerX,
		g_playerY,
		g_playerX + kPlayerSize,
		g_playerY + kPlayerSize
	};

	HBRUSH shadow = CreateSolidBrush(RGB(45, 52, 58));
	HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(dc, shadow));
	Ellipse(dc, body.left - 4, body.bottom - 4, body.right + 4, body.bottom + 4);
	SelectObject(dc, oldBrush);
	DeleteObject(shadow);

	HBRUSH player = CreateSolidBrush(RGB(255, 218, 65));
	oldBrush = static_cast<HBRUSH>(SelectObject(dc, player));
	HPEN outline = CreatePen(PS_SOLID, 2, RGB(48, 53, 61));
	HPEN oldPen = static_cast<HPEN>(SelectObject(dc, outline));
	RoundRect(dc, body.left, body.top, body.right, body.bottom, 8, 8);

	HBRUSH eye = CreateSolidBrush(RGB(28, 33, 38));
	SelectObject(dc, eye);
	Ellipse(dc, body.left + 6, body.top + 7, body.left + 10, body.top + 11);
	Ellipse(dc, body.right - 10, body.top + 7, body.right - 6, body.top + 11);

	SelectObject(dc, oldPen);
	SelectObject(dc, oldBrush);
	DeleteObject(eye);
	DeleteObject(outline);
	DeleteObject(player);
}
