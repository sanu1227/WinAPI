#include <windows.h> //--- 윈도우 헤더 파일
#include <tchar.h>   // TCHAR, LPCTSTR 같은 문자열 자료형을 사용하기 위한 헤더
#include <cstdlib>   // rand, srand, abs 함수 사용을 위한 헤더
#include <ctime>     // time 함수 사용을 위한 헤더

HINSTANCE g_hInst;                                      // 현재 프로그램의 인스턴스 핸들
LPCTSTR lpszClass = L"My Window Class";                 // 윈도우 클래스 이름
LPCTSTR lpszWindowName = L"Window Programming Lab";     // 윈도우 제목
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam); // 메시지 처리 함수 선언

// 보드판 메크로
#define board_size 40        // 40 x 40 보드판
#define cell_size 15         // 한칸의 크기 15
#define start_x 50           // 보드판 시작 x좌표
#define start_y 50           // 보드판 시작 y좌표

// 방향 메크로
#define up 0                 // 위쪽 방향
#define down 1               // 아래쪽 방향
#define right 2              // 오른쪽 방향
#define left 3               // 왼쪽 방향

// 아이템 메크로
#define item 50              // 아이템 갯수
#define item_speed_up 1      // 플레이어 이동 속도를 빠르게 하는 아이템
#define item_speed_down 2    // 플레이어 이동 속도를 느리게 하는 아이템
#define item_size_up 3       // 플레이어 크기를 키우는 아이템
#define item_size_down 4     // 플레이어 크기를 줄이는 아이템

// 타이머 메크로
#define timer_move 0         // 타이머가 작동 중인 상태
#define timer_stop 1         // 타이머가 멈춘 상태

/////////////////////////
// 함수 선언
/////////////////////////
void draw_board(HDC hDC);        // 보드판 그리기 함수
void draw_player(HDC hDC);       // 플레이어 그리기
void move_player(int dx, int dy); // 플레이어 이동 함수
void draw_item(HDC hDC);         // 아이템 그리기 함수
void save_item_xy();             // 랜덤하게 생성된 아이템의 위치를 저장하는 함수
void timer_toggle(HWND hWnd);    // 타이머를 멈추고 시작시키는 함수
void item_effect(int i);         // 아이템 효과를 구현하는 함수
void check_item();               // 아이템과 주인공이 만났는지 확인하는 함수
void mouse_move(LPARAM lParam);  // 마우스 클릭 위치에 따라 이동 방향을 바꾸는 함수

/////////////////////////
// 전역변수
/////////////////////////
int board[board_size][board_size];   // 40x40 보드판 배열, 현재는 맵 정보 저장용으로 준비만 해둠
int player_x = 20;                   // 플레이어 x위치
int player_y = 0;                    // 플레이어 y위치
int player_dir = right;              // 플레이어 이동 방향 변수 - 기본이동은 오른쪽
int timer_index = timer_move;        // 타이머 상태를 나타내는 변수
int timer_speed = 300;               // 타이머 속도를 나타내는 변수, 값이 작을수록 빠르게 이동
HWND g_hWnd;                         // 모든함수에서 hWnd를 사용하기 위한 전역변수
int change_size = 0;                 // 플레이어 크기 변화량을 저장하는 변수

/////////////////////////
// 아이템 구조체
/////////////////////////
typedef struct {        // 아이템 구조체
    int x;              // 아이템 x좌표
    int y;              // 아이템 y좌표
    COLORREF color;     // 아이템 색상
    int effect;         // 아이템 효과 종류
} ITEM;

ITEM items[item];       // 아이템 구조체형 배열 선언

// 보드판 그리기 함수
void draw_board(HDC hDC) {
    for (int i = 0; i <= board_size; i++) { // 세로줄은 칸보다 1개 더 필요하므로 0부터 40까지 반복
        MoveToEx(hDC, start_x + cell_size * i, start_y, NULL); // 세로선 시작점으로 이동
        LineTo(hDC, start_x + cell_size * i, start_y + cell_size * board_size); // 세로선 끝점까지 선 그리기
    }

    for (int i = 0; i <= board_size; i++) { // 가로줄도 칸보다 1개 더 필요하므로 0부터 40까지 반복
        MoveToEx(hDC, start_x, start_y + cell_size * i, NULL); // 가로선 시작점으로 이동
        LineTo(hDC, start_x + cell_size * board_size, start_y + i * cell_size); // 가로선 끝점까지 선 그리기
    }
}

// 주인공 그리기 함수
void draw_player(HDC hDC) {
    HBRUSH hBrush, oldBrush; // 새 브러시와 기존 브러시를 저장할 변수

    hBrush = CreateSolidBrush(RGB(255, 0, 0)); // 빨간색 브러시 생성
    oldBrush = (HBRUSH)SelectObject(hDC, hBrush); // 현재 DC에 빨간색 브러시 선택, 기존 브러시는 저장

    Ellipse(
        hDC,
        start_x + cell_size * player_x - change_size,              // 원의 왼쪽 좌표
        start_y + cell_size * player_y - change_size,              // 원의 위쪽 좌표
        start_x + cell_size * (player_x + 1) + change_size,        // 원의 오른쪽 좌표
        start_y + cell_size * (player_y + 1) + change_size         // 원의 아래쪽 좌표
    );

    SelectObject(hDC, oldBrush); // 기존 브러시로 복구
    DeleteObject(hBrush);        // 새로 만든 브러시 삭제
}

// 주인공 이동 함수
void move_player(int dx, int dy) {
    player_x += dx; // x방향 이동값을 플레이어 x위치에 더함
    player_y += dy; // y방향 이동값을 플레이어 y위치에 더함

    if (player_x < 0) { // 왼쪽 벽을 벗어난 경우
        player_x = 0; // x좌표를 왼쪽 끝으로 고정
        if (timer_index == timer_move) { // 자동 이동 중일 때만
            player_y++; // 아래로 한 칸 이동
        }
        player_dir = right; // 방향을 오른쪽으로 변경
    }

    if (player_x >= board_size) { // 오른쪽 벽을 벗어난 경우
        player_x = board_size - 1; // x좌표를 오른쪽 끝으로 고정
        if (timer_index == timer_move) { // 자동 이동 중일 때만
            player_y++; // 아래로 한 칸 이동
        }
        player_dir = left; // 방향을 왼쪽으로 변경
    }

    if (player_y < 0) { // 위쪽 벽을 벗어난 경우
        player_y = 0; // y좌표를 위쪽 끝으로 고정
        if (timer_index == timer_move) { // 자동 이동 중일 때만
            player_x++; // 오른쪽으로 한 칸 이동
        }
        player_dir = down; // 방향을 아래쪽으로 변경
    }

    if (player_y >= board_size) { // 아래쪽 벽을 벗어난 경우
        player_y = board_size - 1; // y좌표를 아래쪽 끝으로 고정
        if (timer_index == timer_move) { // 자동 이동 중일 때만
            player_x++; // 오른쪽으로 한 칸 이동
        }
        player_dir = up; // 방향을 위쪽으로 변경
    }

    check_item(); // 이동 후 현재 위치에 아이템이 있는지 확인
}

// 랜덤 아이템 생성
void save_item_xy() {
    srand((unsigned int)time(NULL)); // 현재 시간을 기준으로 랜덤값 초기화

    for (int i = 0; i < item; i++) { // 아이템 개수만큼 반복
        items[i].x = rand() % board_size; // 아이템 x좌표를 0~39 사이 랜덤으로 설정
        items[i].y = rand() % board_size; // 아이템 y좌표를 0~39 사이 랜덤으로 설정
        items[i].color = RGB(rand() % 256, rand() % 256, rand() % 256); // 아이템 색상을 랜덤으로 설정
        items[i].effect = rand() % 4 + 1; // 아이템 효과를 1~4 중 하나로 랜덤 설정
    }
}

// 랜덤 아이템 그리기 함수
void draw_item(HDC hDC) {
    for (int i = 0; i < item; i++) { // 모든 아이템을 반복하면서 그림
        HBRUSH hBrush, oldBrush; // 아이템 색상 브러시와 기존 브러시 저장 변수

        hBrush = CreateSolidBrush(items[i].color); // 아이템 색상으로 브러시 생성
        oldBrush = (HBRUSH)SelectObject(hDC, hBrush); // 새 브러시를 선택하고 기존 브러시 저장

        Rectangle(
            hDC,
            start_x + cell_size * items[i].x,             // 아이템 왼쪽 좌표
            start_y + cell_size * items[i].y,             // 아이템 위쪽 좌표
            start_x + cell_size * (items[i].x + 1),       // 아이템 오른쪽 좌표
            start_y + cell_size * (items[i].y + 1)        // 아이템 아래쪽 좌표
        );

        SelectObject(hDC, oldBrush); // 기존 브러시로 복구
        DeleteObject(hBrush);        // 생성한 브러시 삭제
    }
}

// 타이머 stop, start 함수
void timer_toggle(HWND hWnd) {
    if (timer_index == timer_move) { // 현재 자동 이동 중이면
        KillTimer(hWnd, 1); // 1번 타이머 정지
        timer_index = timer_stop; // 상태를 정지 상태로 변경
    }
    else if (timer_index == timer_stop) { // 현재 정지 상태이면
        SetTimer(hWnd, 1, timer_speed, NULL); // 1번 타이머를 현재 속도로 다시 시작
        timer_index = timer_move; // 상태를 이동 상태로 변경
    }
}

// 플레이어가 아이템을 먹었는지 확인하는 함수
void check_item() {
    for (int i = 0; i < item; i++) { // 모든 아이템 검사
        if (items[i].x == player_x && items[i].y == player_y) { // 아이템 위치와 플레이어 위치가 같으면
            item_effect(i); // 해당 아이템 효과 실행

            items[i].x = 1000; // 먹은 아이템을 보드 밖으로 이동시켜 안 보이게 함
            items[i].y = 1000; // 먹은 아이템을 보드 밖으로 이동시켜 다시 충돌하지 않게 함

            break; // 한 번에 하나의 아이템만 먹고 반복 종료
        }
    }
}

// 아이템 효과 함수
void item_effect(int i) {
    if (items[i].effect == item_speed_down) { // 속도 감소 아이템이면
        timer_speed += 100; // 타이머 간격 증가, 즉 이동 속도 느려짐

        if (timer_speed > 1000) { // 너무 느려지지 않도록 최대값 제한
            timer_speed = 1000; // 최대 타이머 간격을 1000으로 고정
        }

        KillTimer(g_hWnd, 1); // 기존 타이머 제거
        SetTimer(g_hWnd, 1, timer_speed, NULL); // 변경된 속도로 타이머 재설정
    }
    else if (items[i].effect == item_speed_up) { // 속도 증가 아이템이면
        timer_speed -= 100; // 타이머 간격 감소, 즉 이동 속도 빨라짐

        if (timer_speed < 50) { // 너무 빨라지지 않도록 최소값 제한
            timer_speed = 50; // 최소 타이머 간격을 50으로 고정
        }

        KillTimer(g_hWnd, 1); // 기존 타이머 제거
        SetTimer(g_hWnd, 1, timer_speed, NULL); // 변경된 속도로 타이머 재설정
    }
    else if (items[i].effect == item_size_up) { // 크기 증가 아이템이면
        change_size += 3; // 플레이어 크기 증가량을 늘림
        InvalidateRect(g_hWnd, NULL, TRUE); // 화면을 다시 그리도록 요청

        if (change_size > 10) { // 너무 커지지 않도록 최대 크기 제한
            change_size = 10; // 최대 크기 변화량을 10으로 고정
        }
    }
    else if (items[i].effect == item_size_down) { // 크기 감소 아이템이면
        change_size -= 3; // 플레이어 크기 증가량을 줄임
        InvalidateRect(g_hWnd, NULL, TRUE); // 화면을 다시 그리도록 요청

        if (change_size < -5) { // 너무 작아지지 않도록 최소 크기 제한
            change_size = -5; // 최소 크기 변화량을 -5로 고정
        }
    }
}

// 마우스로 이동방향을 변경 하는 함수
void mouse_move(LPARAM lParam) {
    int mouse_x = LOWORD(lParam); // 마우스 클릭 x좌표 추출
    int mouse_y = HIWORD(lParam); // 마우스 클릭 y좌표 추출

    int mx = (mouse_x - start_x) / cell_size; // 마우스 x좌표를 보드 칸 x좌표로 변환
    int my = (mouse_y - start_y) / cell_size; // 마우스 y좌표를 보드 칸 y좌표로 변환

    if (mx >= 0 && mx < board_size && my >= 0 && my < board_size) { // 클릭 위치가 보드 안이면
        int dx = mx - player_x; // 클릭한 칸과 플레이어 사이의 x방향 차이
        int dy = my - player_y; // 클릭한 칸과 플레이어 사이의 y방향 차이

        if (abs(dx) > abs(dy)) { // x방향 차이가 y방향 차이보다 크면 좌우 이동
            if (dx > 0) { // 클릭 위치가 플레이어보다 오른쪽이면
                player_dir = right; // 오른쪽으로 이동 방향 변경
            }
            else { // 클릭 위치가 플레이어보다 왼쪽이면
                player_dir = left; // 왼쪽으로 이동 방향 변경
            }
        }
        else { // y방향 차이가 더 크거나 같으면 상하 이동
            if (dy > 0) { // 클릭 위치가 플레이어보다 아래쪽이면
                player_dir = down; // 아래쪽으로 이동 방향 변경
            }
            else { // 클릭 위치가 플레이어보다 위쪽이면
                player_dir = up; // 위쪽으로 이동 방향 변경
            }
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    HWND hWnd; // 생성된 윈도우를 저장할 핸들
    MSG Message; // 윈도우 메시지를 저장할 구조체
    WNDCLASSEX WndClass; // 윈도우 클래스 정보를 저장할 구조체

    g_hInst = hInstance; // 전역 인스턴스 핸들에 현재 인스턴스 저장

    WndClass.cbSize = sizeof(WndClass); // 구조체 크기 설정
    WndClass.style = CS_HREDRAW | CS_VREDRAW; // 가로/세로 크기 변경 시 다시 그리기
    WndClass.lpfnWndProc = (WNDPROC)WndProc; // 메시지 처리 함수 등록
    WndClass.cbClsExtra = 0; // 클래스 여분 메모리 사용 안 함
    WndClass.cbWndExtra = 0; // 윈도우 여분 메모리 사용 안 함
    WndClass.hInstance = hInstance; // 현재 프로그램 인스턴스 등록
    WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION); // 기본 아이콘 사용
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW); // 기본 화살표 커서 사용
    WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); // 배경색을 흰색으로 설정
    WndClass.lpszMenuName = NULL; // 메뉴 사용 안 함
    WndClass.lpszClassName = lpszClass; // 윈도우 클래스 이름 지정
    WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION); // 작은 아이콘도 기본 아이콘 사용

    RegisterClassEx(&WndClass); // 윈도우 클래스 등록

    hWnd = CreateWindow(
        lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW,
        0, 0, 1200, 900,
        NULL, (HMENU)NULL, hInstance, NULL
    ); // 실제 윈도우 생성

    ShowWindow(hWnd, nCmdShow); // 윈도우를 화면에 표시
    UpdateWindow(hWnd); // WM_PAINT 메시지를 발생시켜 초기 화면 그림

    while (GetMessage(&Message, 0, 0, 0)) { // 메시지 큐에서 메시지를 계속 가져옴
        TranslateMessage(&Message); // 키보드 입력 메시지를 문자 메시지로 변환
        DispatchMessage(&Message); // 메시지를 WndProc으로 전달
    }

    return Message.wParam; // 프로그램 종료 코드 반환
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps; // 그리기 정보를 저장하는 구조체
    HDC hDC; // 그림을 그릴 때 사용하는 디바이스 컨텍스트

    //--- 메시지 처리하기
    switch (uMsg) {
    case WM_CREATE: // 윈도우가 처음 생성될 때 실행
        save_item_xy(); // 아이템 위치, 색상, 효과를 랜덤으로 생성
        g_hWnd = hWnd; // 다른 함수에서도 윈도우 핸들을 사용할 수 있도록 저장
        SetTimer(hWnd, 1, timer_speed, NULL); // 1번 타이머를 timer_speed 간격으로 시작
        break; // WM_CREATE 처리 종료

    case WM_LBUTTONDOWN: // 마우스 왼쪽 버튼을 눌렀을 때 실행
        mouse_move(lParam); // 클릭 위치에 따라 플레이어 이동 방향 변경
        InvalidateRect(hWnd, NULL, TRUE); // 화면 다시 그리기 요청
        break; // WM_LBUTTONDOWN 처리 종료

    case WM_KEYDOWN: // 키보드 키를 눌렀을 때 실행
        if (wParam == 'W') // W키를 누르면
            move_player(0, -1); // 플레이어를 위로 한 칸 이동

        if (wParam == 'A') // A키를 누르면
            move_player(-1, 0); // 플레이어를 왼쪽으로 한 칸 이동

        if (wParam == 'S') // S키를 누르면
            move_player(0, 1); // 플레이어를 아래로 한 칸 이동

        if (wParam == 'D') // D키를 누르면
            move_player(1, 0); // 플레이어를 오른쪽으로 한 칸 이동

        if (wParam == VK_UP) // 방향키 위를 누르면
            player_dir = up; // 자동 이동 방향을 위로 변경

        if (wParam == VK_DOWN) // 방향키 아래를 누르면
            player_dir = down; // 자동 이동 방향을 아래로 변경

        if (wParam == VK_RIGHT) // 방향키 오른쪽을 누르면
            player_dir = right; // 자동 이동 방향을 오른쪽으로 변경

        if (wParam == VK_LEFT) // 방향키 왼쪽을 누르면
            player_dir = left; // 자동 이동 방향을 왼쪽으로 변경

        if (wParam == 'P') { // P키를 누르면
            timer_toggle(hWnd); // 자동 이동 정지 또는 다시 시작
        }

        if (wParam == VK_PRIOR) { // Page Up 키를 누르면
            timer_speed -= 100; // 타이머 간격을 줄여 속도를 빠르게 함

            if (timer_speed < 0) { // 속도가 너무 빨라지는 것을 방지
                timer_speed = 10; // 최소 타이머 간격을 10으로 설정
            }

            KillTimer(hWnd, 1); // 기존 타이머 제거
            SetTimer(hWnd, 1, timer_speed, NULL); // 변경된 속도로 타이머 재설정
        }

        if (wParam == VK_NEXT) { // Page Down 키를 누르면
            timer_speed += 100; // 타이머 간격을 늘려 속도를 느리게 함

            if (timer_speed > 1000) { // 속도가 너무 느려지는 것을 방지
                timer_speed = 1000; // 최대 타이머 간격을 1000으로 설정
            }

            KillTimer(hWnd, 1); // 기존 타이머 제거
            SetTimer(hWnd, 1, timer_speed, NULL); // 변경된 속도로 타이머 재설정
        }

        InvalidateRect(hWnd, NULL, TRUE); // 키 입력 처리 후 화면 다시 그리기 요청
        break; // WM_KEYDOWN 처리 종료

    case WM_PAINT: // 화면을 다시 그려야 할 때 실행
        hDC = BeginPaint(hWnd, &ps); // 그리기 시작

        draw_board(hDC); // 보드판 그리기
        draw_player(hDC); // 플레이어 그리기
        draw_item(hDC); // 아이템 그리기

        EndPaint(hWnd, &ps); // 그리기 종료
        break; // WM_PAINT 처리 종료

    case WM_TIMER: // 타이머가 정해진 시간마다 실행
        if (player_dir == up) // 현재 방향이 위쪽이면
            move_player(0, -1); // 위로 이동

        if (player_dir == down) // 현재 방향이 아래쪽이면
            move_player(0, 1); // 아래로 이동

        if (player_dir == right) // 현재 방향이 오른쪽이면
            move_player(1, 0); // 오른쪽으로 이동

        if (player_dir == left) // 현재 방향이 왼쪽이면
            move_player(-1, 0); // 왼쪽으로 이동

        InvalidateRect(hWnd, NULL, TRUE); // 이동 후 화면 다시 그리기 요청
        break; // WM_TIMER 처리 종료

    case WM_DESTROY: // 윈도우가 닫힐 때 실행
        KillTimer(hWnd, 1); // 타이머 제거
        PostQuitMessage(0); // 프로그램 종료 메시지 발생
        break; // WM_DESTROY 처리 종료
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam); //--- 위의 세 메시지 외의 나머지 메시지는 OS로
}