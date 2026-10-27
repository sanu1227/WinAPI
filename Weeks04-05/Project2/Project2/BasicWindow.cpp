#include <windows.h>                         // 윈도우 API 사용을 위한 헤더
#include <tchar.h>                           // TCHAR, LPCTSTR 등을 사용하기 위한 헤더


HINSTANCE g_hInst;                           // 프로그램 인스턴스 핸들
LPCTSTR lpszClass = L"My Window Class";      // 윈도우 클래스 이름
LPCTSTR lpszWindowName = L"Window Programming Lab"; // 창 제목

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); // 메시지 처리 함수 선언

constexpr int MaxLines = 10;
constexpr int MaxColumns = 30;

static void HandleCharacter(WPARAM character, TCHAR (&text)[MaxLines][MaxColumns + 1],
    int (&lengths)[MaxLines], int& currentLine, int& currentColumn)
{
    if (character >= 32 && character <= 126) {
        if (currentColumn < MaxColumns) {
            text[currentLine][currentColumn++] = static_cast<TCHAR>(character);
            text[currentLine][currentColumn] = _T('\0');
            lengths[currentLine] = currentColumn;
        }
    }
    else if (character == VK_BACK) {
        if (currentColumn > 0) {
            text[currentLine][--currentColumn] = _T('\0');
            lengths[currentLine] = currentColumn;
        }
        else if (currentLine > 0) {
            currentColumn = lengths[--currentLine];
        }
    }
    else if (character == VK_RETURN && currentLine + 1 < MaxLines) {
        currentColumn = lengths[++currentLine];
    }
}

/* ---------------- WinMain ---------------- */

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow) // 프로그램 시작 함수
{
    HWND hWnd;                                // 창 핸들 선언
    MSG Message;                              // 메시지 구조체 선언
    WNDCLASSEX WndClass;                      // 윈도우 클래스 구조체 선언

    g_hInst = hInstance;                      // 인스턴스 핸들 저장

    WndClass.cbSize = sizeof(WndClass);       // 구조체 크기 설정
    WndClass.style = CS_HREDRAW | CS_VREDRAW; // 창 크기 변경 시 다시 그리기
    WndClass.lpfnWndProc = WndProc;           // 메시지 처리 함수 등록
    WndClass.cbClsExtra = 0;                  // 추가 클래스 메모리 없음
    WndClass.cbWndExtra = 0;                  // 추가 윈도우 메모리 없음
    WndClass.hInstance = hInstance;           // 인스턴스 저장
    WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION); // 기본 아이콘 사용
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);   // 기본 커서 사용
    WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); // 흰 배경 사용
    WndClass.lpszMenuName = NULL;             // 메뉴 없음
    WndClass.lpszClassName = lpszClass;       // 클래스 이름 설정
    WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION); // 작은 아이콘 설정

    RegisterClassEx(&WndClass);               // 윈도우 클래스 등록

    hWnd = CreateWindow(                      // 실제 창 생성
        lpszClass,                            // 클래스 이름
        lpszWindowName,                       // 창 제목
        WS_OVERLAPPEDWINDOW,                  // 기본 윈도우 스타일
        0, 0, 800, 600,                       // 위치와 크기
        NULL, NULL, hInstance, NULL          // 부모, 메뉴, 인스턴스, 추가 데이터
    );

    ShowWindow(hWnd, nCmdShow);               // 창 보이기
    UpdateWindow(hWnd);                       // 즉시 다시 그리기

    while (GetMessage(&Message, NULL, 0, 0)) { // 메시지 루프
        TranslateMessage(&Message);           // 키보드 메시지 변환
        DispatchMessage(&Message);            // WndProc으로 메시지 전달
    }

    return (int)Message.wParam;               // 종료 코드 반환
}

/* ---------------- WndProc ---------------- */

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) // 메시지 처리 함수 정의
{
    PAINTSTRUCT ps;                           // WM_PAINT용 구조체
    HDC hDC;                                  // 디바이스 컨텍스트
    SIZE size{};                             // 문자열 픽셀 길이 저장용 구조체
    static TCHAR str[MaxLines][MaxColumns + 1]; // 10줄, 줄당 30글자와 널 종료 문자
    static int len[MaxLines];                 // 각 줄의 길이
    static int currentColumn;                // 현재 캐럿이 위치한 칸(가로)
    static int currentLine;                  // 현재 캐럿이 위치한 줄(세로)

    switch (uMsg) {                           // 메시지 종류에 따라 분기

    case WM_CREATE:                           // 창이 처음 생성될 때
        CreateCaret(hWnd, NULL, 5, 15);       // 캐럿 생성
        ShowCaret(hWnd);                      // 캐럿 표시
        break;                                // 처리 종료

    case WM_KEYDOWN:                          // 기능키, 방향키 같은 특수키 처리

        InvalidateRect(hWnd, NULL, TRUE);     // 화면 다시 그리기
        break;                                // WM_KEYDOWN 기본 종료

    case WM_CHAR:                             // ASCII 문자, Enter, Backspace 처리
        HandleCharacter(wParam, str, len, currentLine, currentColumn);
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;

    case WM_PAINT: {                          // 화면 다시 그리기 메시지
        hDC = BeginPaint(hWnd, &ps);          // 그리기 시작
        TEXTMETRIC metrics{};
        GetTextMetrics(hDC, &metrics);
        const int lineHeight = metrics.tmHeight + 4;
        for (int line = 0; line < MaxLines; ++line) {
            TextOut(hDC, 0, line * lineHeight, str[line], len[line]);
        }
        GetTextExtentPoint32(hDC, str[currentLine], currentColumn, &size);
        SetCaretPos(size.cx, currentLine * lineHeight); // 현재 줄 끝에 캐럿 표시
        EndPaint(hWnd, &ps);                  // 그리기 종료
        return 0;
    }

    case WM_DESTROY:                          // 창이 닫힐 때
        HideCaret(hWnd);                      // 캐럿 숨기기
        DestroyCaret();                       // 캐럿 제거
        PostQuitMessage(0);                   // 프로그램 종료 메시지 전달
        return 0;                             // 처리 완료 반환
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam); // 처리하지 않은 메시지는 기본 처리
}
