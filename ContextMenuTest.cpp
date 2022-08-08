// ContextMenuTest.cpp : Defines the entry point for the application.
//

#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"
#include "ContextMenuTest.h"
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

struct params_t {
    float initial_radius = 200.0f;
    float sector_edge_slope = 0.3f;
    float branch_near_edge_offset = 100.0f;
    float branch_far_edge_offset = 100.0f;
    float branch_far_edge_dead_zone = 150.0f;
    float branch_label_length_offset = 300.0f;
    float branch_label_height_offset = 50.0f;
    float leaf_base_offset = 150.0f;
    float leaf_height = 400.0f;
    int min_window_margin = 200;
};
params_t params;
float const display_scale = 0.2f;

float const sqrt_1_2 = 0.70710678f;
float const tan_pi_8 = 0.41421356f;

struct action_t {
    std::wstring name;
};

struct menu_item_t {
    struct submenu_t;
    std::wstring description;
    std::unique_ptr<submenu_t> submenu;
    std::optional<action_t> opt_action;

    static menu_item_t branch(std::wstring descr, menu_item_t ia, menu_item_t ib);
    static menu_item_t leaf(std::wstring descr);
};

struct menu_item_t::submenu_t {
    menu_item_t left;
    menu_item_t right;

    static std::unique_ptr<submenu_t> make(menu_item_t ia, menu_item_t ib) {
        return std::make_unique<submenu_t>(submenu_t{std::move(ia), std::move(ib)});
    }
};

menu_item_t menu_item_t::branch(std::wstring descr, menu_item_t ia, menu_item_t ib) {
    return menu_item_t{std::move(descr) + L"...", submenu_t::make(std::move(ia), std::move(ib))};
}

menu_item_t menu_item_t::leaf(std::wstring descr) {
    std::wstring d2 = descr;
    return menu_item_t{std::move(descr), nullptr, action_t{std::move(d2)}};
}

menu_item_t menu[8] = {
    menu_item_t::branch(
        L"6",
        menu_item_t::branch(
            L"6U",
            menu_item_t::leaf(L"6UL"),
            menu_item_t::leaf(L"6UR")),
        menu_item_t::branch(
            L"6D",
            menu_item_t::leaf(L"6DR"),
            menu_item_t::leaf(L"6DL"))),
    menu_item_t::branch(
        L"3",
        menu_item_t::branch(
            L"3U",
            menu_item_t::branch(
                L"3UL",
                menu_item_t::leaf(L"3ULD"),
                menu_item_t::leaf(L"3ULU")),
            menu_item_t::branch(
                L"3UR",
                menu_item_t::leaf(L"3URU"),
                menu_item_t::leaf(L"3URD"))),
        menu_item_t::branch(
            L"3L",
            menu_item_t::leaf(L"3LD"),
            menu_item_t::leaf(L"3LU"))),
    menu_item_t{L"<legacy here>"},
    menu_item_t::branch(
        L"1",
        menu_item_t::leaf(L"1R"),
        menu_item_t::leaf(L"1U")),
    menu_item_t::branch(
        L"4",
        menu_item_t::branch(
            L"4D",
            menu_item_t::leaf(L"4DR"),
            menu_item_t::leaf(L"4DL")),
        menu_item_t::branch(
            L"4U",
            menu_item_t::leaf(L"4UL"),
            menu_item_t::leaf(L"4UR"))),
    menu_item_t::branch(
        L"7",
        menu_item_t::branch(
            L"7D",
            menu_item_t::leaf(L"7DR"),
            menu_item_t::leaf(L"7DL")),
        menu_item_t::branch(
            L"7R",
            menu_item_t::leaf(L"7RU"),
            menu_item_t::leaf(L"7RD"))),
    menu_item_t{L"<legacy here>"},
    menu_item_t::leaf(L"9"),
};

enum class rotor: uint_fast8_t;

rotor operator~(rotor a) {
    return rotor(-(uint_fast8_t)a);
}

rotor operator+(rotor a, rotor b) {
    return rotor((uint_fast8_t)a + (uint_fast8_t)b);
}

uint_fast8_t operator+(rotor a) {
    return (uint_fast8_t)a & 7;
}

struct vec2 {
    float x, y;

    vec2 rotp90() const {
        return vec2{y, -x};
    }

    vec2 rotm90() const {
        return vec2{-y, x};
    }

    vec2& operator+=(vec2 b) {
        x += b.x;
        y += b.y;
        return *this;
    }
};

vec2 operator-(vec2 a) {
    return vec2{-a.x, -a.y};
}

vec2 operator~(vec2 a) {
    return vec2{a.x, -a.y};
}

vec2 operator+(vec2 a, vec2 b) {
    return vec2{a.x + b.x, a.y + b.y};
}

vec2 operator-(vec2 a, vec2 b) {
    return vec2{a.x - b.x, a.y - b.y};
}

vec2 operator*(float a, vec2 b) {
    return vec2{a * b.x, a * b.y};
}

vec2 operator%(rotor a, vec2 b) {
    uint8_t ai = (uint8_t)a;
    float rx = b.x;
    float ry = b.y;
    if (ai & 4) {
        rx = -rx;
        ry = -ry;
    }
    if (ai & 2) {
        float orx = rx;
        float ory = ry;
        rx = -ory;
        ry = orx;
    }
    if (ai & 1) {
        float orx = rx;
        float ory = ry;
        rx = sqrt_1_2 * orx - sqrt_1_2 * ory;
        ry = sqrt_1_2 * orx + sqrt_1_2 * ory;
    }
    return vec2{rx, ry};
}

struct menu_state_t {
    struct branch_t {
        menu_item_t* menu_item_ptr;
        vec2 origin;
        rotor rot;
        float base_slope;
        float top_offset;
        float bot_offset;
        float trigger_offset;
        bool top_active;
        bool bot_active;
    };

    vec2 global_pos;
    std::vector<branch_t> branches;

    void reset() {
        global_pos = vec2{0, 0};
        branches.resize(0);
    }

    void find_sector(rotor& out_rot, vec2& out_pos) {
        int side = 0;
        float x = global_pos.x;
        float y = global_pos.y;
        float a = sqrt_1_2 * (x + y);
        float b = sqrt_1_2 * (- x + y);
        if (x < -b) {
            side = 4;
            x = -x;
            y = -y;
            a = -a;
            b = -b;
        }
        if (x < b) {
            side += 2;
            float ox = x;
            float oy = y;
            float oa = a;
            float ob = b;
            x = oy;
            y = -ox;
            a = ob;
            b = -oa;
        }
        if (x < a) {
            out_rot = rotor(side + 1);
            out_pos = vec2{a, b};
        } else {
            out_rot = rotor(side);
            out_pos = vec2{x, y};
        }
    }

    void apply_delta(vec2 delta) {
        global_pos += delta;
    update:
        if (branches.size() == 0) {
            rotor rot;
            vec2 relpos;
            find_sector(rot, relpos);
            if (relpos.x > params.initial_radius) {
                vec2 origin = rot % vec2{params.initial_radius, 0};
                branches.push_back(branch_t{
                    &menu[+rot],
                    origin,
                    rot,
                    0,
                    params.initial_radius * tan_pi_8,
                    params.initial_radius * tan_pi_8,
                    0,
                    true,
                    true});
                goto update;
            }
        } else {
            branch_t& branch = branches.back();
            vec2 pos = ~branch.rot % (global_pos - branch.origin);
            if (pos.x < pos.y * branch.base_slope) {
                branches.pop_back();
                goto update;
            }
            float trigger_distance = pos.x - pos.y * branch.base_slope;
            if (!branch.top_active) {
                if (trigger_distance > branch.trigger_offset) {
                    branch.top_active = true;
                    float y_distance = branch.top_offset + params.sector_edge_slope * pos.x + pos.y;
                    if (y_distance < branch.top_offset) {
                        branch.top_offset += branch.top_offset - y_distance;
                    }
                }
            }
            if (!branch.bot_active) {
                if (trigger_distance > branch.trigger_offset) {
                    branch.bot_active = true;
                    float y_distance = branch.bot_offset + params.sector_edge_slope * pos.x - pos.y;
                    if (y_distance < branch.bot_offset) {
                        branch.bot_offset += branch.bot_offset - y_distance;
                    }
                }
            }
            if (branch.menu_item_ptr->submenu) {
                if (branch.top_active) {
                    float ylim = branch.top_offset + params.sector_edge_slope * pos.x;
                    if (pos.y < -ylim) {
                        branch.bot_active = true;
                        branches.push_back(branch_t{
                            &branch.menu_item_ptr->submenu->left,
                            branch.origin + branch.rot % vec2{pos.x, -ylim},
                            branch.rot + rotor(6),
                            params.sector_edge_slope,
                            params.branch_near_edge_offset,
                            params.branch_far_edge_offset,
                            params.branch_far_edge_dead_zone,
                            false,
                            false});
                        goto update;
                    }
                }
                if (branch.bot_active) {
                    float ylim = branch.bot_offset + params.sector_edge_slope * pos.x;
                    if (pos.y > ylim) {
                        branch.top_active = true;
                        branches.push_back(branch_t{
                            &branch.menu_item_ptr->submenu->right,
                            branch.origin + branch.rot % vec2{pos.x, ylim},
                            branch.rot + rotor(2),
                            -params.sector_edge_slope,
                            params.branch_far_edge_offset,
                            params.branch_near_edge_offset,
                            params.branch_far_edge_dead_zone,
                            false,
                            false});
                        goto update;
                    }
                }
            }
        }
    }

    menu_item_t* selected_leaf_item() {
        if (branches.size() == 0) {
            return nullptr;
        } else {
            return branches.back().menu_item_ptr;
        }
    }
};

vec2 text_delta_table[8] = {
    vec2{ 0.0f, -0.5f},
    vec2{ 0.0f,  0.0f},
    vec2{-0.5f,  0.0f},
    vec2{-1.0f,  0.0f},
    vec2{-1.0f, -0.5f},
    vec2{-1.0f, -1.0f},
    vec2{-0.5f, -1.0f},
    vec2{ 0.0f, -1.0f},
};

menu_state_t menu_state;
enum class Mode {
    Disabled,
    Pressed,
    PressedAgain,
    Clicked,
} mode;
POINT center_point;
action_t* last_selected_action;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HBRUSH hbrush_background;
HPEN hpen_geometry_passive;
HPEN hpen_geometry_active;
HPEN hpen_current;
HFONT hfont_selection;
HFONT hfont_label;
COLORREF color_label_waiting = RGB(0, 0, 0);
COLORREF color_label_selected = RGB(255, 0, 0);
COLORREF color_label_disabled = RGB(192, 192, 192);

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void SetCursorWindowPos(HWND hwnd, int x, int y) {
    POINT pos;
    pos.x = x;
    pos.y = y;
    ClientToScreen(hwnd, &pos);
    SetCursorPos(pos.x, pos.y);
}

[[noreturn]] void winapi_failure() {
    DWORD errCode = GetLastError();
    LPWSTR buffer = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,
        errCode,
        0,
        (LPWSTR)&buffer,
        0,
        nullptr);
    MessageBoxW(
        0,
        buffer,
        L"bad",
        MB_OK | MB_ICONERROR | MB_TASKMODAL);
    DebugBreak();
    ExitProcess(0);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    RAWINPUTDEVICE rid[1];

    rid[0].usUsagePage = 0x01;
    rid[0].usUsage = 0x02;
    rid[0].dwFlags = 0;
    rid[0].hwndTarget = 0;

    if (RegisterRawInputDevices(rid, sizeof(rid) / sizeof(rid[0]), sizeof(rid[0])) == FALSE) {
        winapi_failure();
    }

    AllocConsole();

    (void)freopen("CON", "w", stdout);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CONTEXTMENUTEST, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    hbrush_background = CreateSolidBrush(RGB(255, 255, 255));
    hpen_geometry_passive = CreatePen(PS_SOLID, 0, RGB(128, 128, 128));
    hpen_geometry_active = CreatePen(PS_SOLID, 0, RGB(255, 0, 0));
    hpen_current = CreatePen(PS_SOLID, 0, RGB(0, 128, 0));
    hfont_selection = CreateFontW(
        20,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        FIXED_PITCH | FF_DONTCARE,
        nullptr);
    hfont_label = CreateFontW(
        14,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        VARIABLE_PITCH | FF_MODERN,
        L"Verdana");

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CONTEXTMENUTEST));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CONTEXTMENUTEST));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CONTEXTMENUTEST);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wparam);
        // Parse the menu selections:
        switch (wmId) {
        case IDM_ABOUT:
        {
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
            return 0;
        }
        case IDM_EXIT:
        {
            DestroyWindow(hwnd);
            return 0;
        }
        case IDM_CANCEL:
        {
            if (mode != Mode::Disabled) {
                mode = Mode::Disabled;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        default:
        {
            return DefWindowProc(hwnd, message, wparam, lparam);
        }
        }
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC window_dc = BeginPaint(hwnd, &ps);
        RECT client_rect;
        if (!GetClientRect(hwnd, &client_rect)) {
            winapi_failure();
        }
        int cx = client_rect.right - client_rect.left;
        int cy = client_rect.bottom - client_rect.top;
        HDC dc = CreateCompatibleDC(window_dc);
        if (!dc) {
            winapi_failure();
        }
        HBITMAP bm = CreateCompatibleBitmap(window_dc, cx, cy);
        if (!bm) {
            winapi_failure();
        }
        SelectObject(dc, bm);
        RECT fill_rect{0, 0, cx, cy};
        FillRect(dc, &fill_rect, hbrush_background);
        auto to_screen = [&](vec2 a, int& ax, int& ay) {
            ax = center_point.x + (int)(display_scale * a.x);
            ay = center_point.y + (int)(display_scale * a.y);
        };
        auto line = [&](vec2 a, vec2 b) {
            int ax, ay, bx, by;
            to_screen(a, ax, ay);
            to_screen(b, bx, by);
            MoveToEx(dc, ax, ay, nullptr);
            LineTo(dc, bx, by);
        };
        SetTextAlign(dc, TA_LEFT | TA_TOP);
        auto text = [&](vec2 base, vec2 delta, std::wstring const& str) {
            SIZE tsize;
            GetTextExtentPoint32W(dc, str.data(), (int)str.size(), &tsize);
            delta.x *= (float)tsize.cx;
            delta.y *= (float)tsize.cy;
            vec2 a = base;
            int ax, ay;
            to_screen(a, ax, ay);
            ax += (int)delta.x;
            ay += (int)delta.y;
            TextOutW(dc, ax, ay, str.data(), (int)str.size());
        };
        if (mode != Mode::Disabled) {
            SelectObject(dc, hfont_label);
            if (menu_state.branches.size() == 0) {
                SelectObject(dc, hpen_geometry_active);
            } else {
                SelectObject(dc, hpen_geometry_passive);
            }
            for (int i = 0; i < 8; ++i) {
                vec2 p = params.initial_radius * vec2{1, tan_pi_8};
                vec2 gpa = rotor(i) % p;
                vec2 gpb = rotor(i) % ~p;
                line(gpa, gpb);
                vec2 t = vec2{params.initial_radius + params.branch_label_height_offset, 0};
                vec2 gt = rotor(i) % t;
                COLORREF t_color = color_label_waiting;
                if (menu_state.branches.size() >= 1) {
                    if (+menu_state.branches[0].rot == i) {
                        t_color = color_label_selected;
                    } else {
                        t_color = color_label_disabled;
                    }
                }
                if (t_color != color_label_selected) {
                    SetTextColor(dc, t_color);
                    text(gt, text_delta_table[i], menu[i].description);
                }
            }
            for (size_t i = 0; i < menu_state.branches.size(); ++i) {
                menu_state_t::branch_t& branch = menu_state.branches[i];
                bool is_active = i == menu_state.branches.size() - 1;
                if (branch.menu_item_ptr->submenu) {
                    menu_item_t::submenu_t& submenu = *branch.menu_item_ptr->submenu;
                    /*
                        px = left_slope * py

                        py = base_width + sector_slope * px
                        py = base_width / (1 - sector_slope * base_slope)

                        py = - base_width - sector_slope * px
                        py = - base_width / (1 + sector_slope * base_slope)



                        px = base_slope * py + trigger

                        py = bot_offset + sector_slope * px
                        py = (bot_offset + sector_slope * trigger) / (1 - sector_slope * base_slope)

                        py = - top_offset - sector_slope * px
                        py = - (top_offset + sector_slope * trigger) / (1 + sector_slope * base_slope)
                    */
                    vec2 pos = ~branch.rot % (menu_state.global_pos - branch.origin);
                    float bot_offset = branch.bot_offset;
                    if (!branch.bot_active) {
                        float y_distance = bot_offset + params.sector_edge_slope * pos.x - pos.y;
                        if (y_distance < bot_offset) {
                            bot_offset += bot_offset - y_distance;
                        }
                    }
                    float top_offset = branch.top_offset;
                    if (!branch.top_active) {
                        float y_distance = top_offset + params.sector_edge_slope * pos.x + pos.y;
                        if (y_distance < top_offset) {
                            top_offset += top_offset - y_distance;
                        }
                    }
                    float a = bot_offset / (1 - branch.base_slope * params.sector_edge_slope);
                    float b = -top_offset / (1 + branch.base_slope * params.sector_edge_slope);
                    vec2 pa = vec2{branch.base_slope * a, a};
                    vec2 pb = vec2{branch.base_slope * b, b};
                    vec2 qa = vec2{0, bot_offset} + (10 * params.initial_radius) * vec2 { 1, params.sector_edge_slope };
                    vec2 qb = vec2{0, -top_offset} + (10 * params.initial_radius) * vec2 { 1, -params.sector_edge_slope };
                    vec2 gpa = branch.origin + branch.rot % pa;
                    vec2 gpb = branch.origin + branch.rot % pb;
                    vec2 gqa = branch.origin + branch.rot % qa;
                    vec2 gqb = branch.origin + branch.rot % qb;
                    if (is_active) {
                        SelectObject(dc, hpen_geometry_active);
                    }
                    line(gpa, gpb);
                    if (is_active && branch.bot_active) {
                        SelectObject(dc, hpen_geometry_active);
                    } else {
                        SelectObject(dc, hpen_geometry_passive);
                    }
                    line(gqa, gpa);
                    if (is_active && branch.top_active) {
                        SelectObject(dc, hpen_geometry_active);
                    } else {
                        SelectObject(dc, hpen_geometry_passive);
                    }
                    line(gpb, gqb);
                    if (is_active && (!branch.top_active || !branch.bot_active)) {
                        float at = (bot_offset + params.sector_edge_slope * branch.trigger_offset) / (1 - branch.base_slope * params.sector_edge_slope);
                        float bt = (-top_offset - params.sector_edge_slope * branch.trigger_offset) / (1 + branch.base_slope * params.sector_edge_slope);
                        vec2 pat = vec2{branch.base_slope * at + branch.trigger_offset, at};
                        vec2 pbt = vec2{branch.base_slope * bt + branch.trigger_offset, bt};
                        vec2 gpat = branch.origin + branch.rot % pat;
                        vec2 gpbt = branch.origin + branch.rot % pbt;
                        SelectObject(dc, hpen_geometry_active);
                        line(gpat, gpbt);
                    }
                    vec2 tdx = params.branch_label_height_offset * vec2{branch.base_slope, 1};
                    vec2 tdya = params.branch_label_length_offset * vec2{1, params.sector_edge_slope};
                    vec2 tdyb = params.branch_label_length_offset * vec2{1, -params.sector_edge_slope};
                    vec2 ta = pa + tdx + tdya;
                    vec2 tb = pb - tdx + tdyb;
                    vec2 gta = branch.origin + branch.rot % ta;
                    vec2 gtb = branch.origin + branch.rot % tb;
                    COLORREF color_ta = color_label_disabled;
                    COLORREF color_tb = color_label_disabled;
                    if (menu_state.branches.size() > (i + 1)) {
                        rotor delta_rot = ~menu_state.branches[i].rot + menu_state.branches[i + 1].rot;
                        if (+delta_rot == 2) {
                            color_ta = color_label_selected;
                        } else if (+delta_rot == 6) {
                            color_tb = color_label_selected;
                        }
                    } else {
                        color_ta = color_label_waiting;
                        color_tb = color_label_waiting;
                    }
                    if (color_ta != color_label_selected) {
                        vec2 tadelta = text_delta_table[+(menu_state.branches[i].rot + (rotor)1)];
                        SetTextColor(dc, color_ta);
                        text(gta, tadelta, submenu.right.description);
                    }
                    if (color_tb != color_label_selected) {
                        vec2 tbdelta = text_delta_table[+(menu_state.branches[i].rot + (rotor)7)];
                        SetTextColor(dc, color_tb);
                        text(gtb, tbdelta, submenu.left.description);
                    }
                } else {
                    float a = params.leaf_base_offset / (1 - branch.base_slope * params.sector_edge_slope);
                    float b = -params.leaf_base_offset / (1 + branch.base_slope * params.sector_edge_slope);
                    vec2 pa = vec2{branch.base_slope * a, a};
                    vec2 pb = vec2{branch.base_slope * b, b};
                    vec2 q = vec2{params.leaf_height, 0};
                    vec2 gpa = branch.origin + branch.rot % pa;
                    vec2 gpb = branch.origin + branch.rot % pb;
                    vec2 gq = branch.origin + branch.rot % q;
                    SelectObject(dc, hpen_geometry_active);
                    line(gpa, gpb);
                    line(gpb, gq);
                    line(gq, gpa);
                }
                vec2 t = vec2{params.branch_label_height_offset, 0};
                vec2 gt = branch.origin + branch.rot % t;
                vec2 tdelta = text_delta_table[+menu_state.branches[i].rot];
                SetTextColor(dc, color_label_selected);
                text(gt, tdelta, branch.menu_item_ptr->description);
            }
            SelectObject(dc, hpen_current);
            if (menu_state.branches.size() == 0) {
                line(vec2{0,0}, menu_state.global_pos);
            } else {
                line(vec2{0,0}, menu_state.branches[0].origin);
                for (size_t i = 0; i < menu_state.branches.size() - 1; ++i) {
                    line(menu_state.branches[i].origin, menu_state.branches[i + 1].origin);
                }
                line(menu_state.branches.back().origin, menu_state.global_pos);
            }
        }
        if (last_selected_action) {
            SelectObject(dc, hfont_selection);
            SetTextColor(dc, color_label_disabled);
            TextOutW(dc, 20, 20, last_selected_action->name.data(), (int)last_selected_action->name.size());
        }
        BitBlt(window_dc, 0, 0, cx, cy, dc, 0, 0, SRCCOPY);
        if (!DeleteObject(bm)) {
            winapi_failure();
        }
        if (!DeleteDC(dc)) {
            winapi_failure();
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_RBUTTONDOWN:
    {
        SetCapture(hwnd);
        if (mode == Mode::Clicked) {
            mode = Mode::PressedAgain;
        } else {
            center_point.x = GET_X_LPARAM(lparam);
            center_point.y = GET_Y_LPARAM(lparam);
            RECT client_rect;
            if (!GetClientRect(hwnd, &client_rect)) {
                winapi_failure();
            }
            int cx = client_rect.right - client_rect.left;
            int cy = client_rect.bottom - client_rect.top;
            if (center_point.x < params.min_window_margin) {
                center_point.x = params.min_window_margin;
            } else if (center_point.x > cx - params.min_window_margin) {
                center_point.x = cx - params.min_window_margin;
            }
            if (center_point.y < params.min_window_margin) {
                center_point.y = params.min_window_margin;
            } else if (center_point.y > cy - params.min_window_margin) {
                center_point.y = cy - params.min_window_margin;
            }
            menu_state.reset();

            SetCursorWindowPos(hwnd, center_point.x, center_point.y);

            mode = Mode::Pressed;

            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_RBUTTONUP:
    {
        ReleaseCapture();
        menu_item_t* item_ptr = menu_state.selected_leaf_item();
        if (item_ptr) {
            if (item_ptr->opt_action.has_value()) {
                last_selected_action = &*item_ptr->opt_action;
                wprintf(L"%s\n", last_selected_action->name.c_str());
            } else {
                last_selected_action = nullptr;
            }
            mode = Mode::Disabled;
        } else {
            if (mode == Mode::PressedAgain) {
                mode = Mode::Disabled;
            } else {
                mode = Mode::Clicked;
            }
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        SetCapture(hwnd);
        if (mode == Mode::Clicked) {
            mode = Mode::PressedAgain;
        }
        return 0;
    }
    case WM_LBUTTONUP:
    {
        ReleaseCapture();
        if (mode == Mode::PressedAgain) {
            menu_item_t* item_ptr = menu_state.selected_leaf_item();
            if (item_ptr) {
                if (item_ptr->opt_action.has_value()) {
                    last_selected_action = &*item_ptr->opt_action;
                    wprintf(L"%s\n", last_selected_action->name.c_str());
                } else {
                    last_selected_action = nullptr;
                }
            }
            mode = Mode::Disabled;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        if (mode != Mode::Disabled) {
            int dx = GET_X_LPARAM(lparam) - center_point.x;
            int dy = GET_Y_LPARAM(lparam) - center_point.y;

            if (dx != 0 || dy != 0) {
                menu_state.apply_delta(vec2{dx / display_scale, dy / display_scale});
                // menu_state.apply_delta(vec2{(float)dx, (float)dy});

                SetCursorWindowPos(hwnd, center_point.x, center_point.y);

                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }

        return 0;
    }
    default:
    {
        return DefWindowProc(hwnd, message, wparam, lparam);
    }
    }
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
