// Copyright (C) 2021-2022 Fuwn
// SPDX-License-Identifier: GPL-3.0-only

/**
 * @file   tray.cc
 * @author Fuwn
 * @date   2021. August. 18.
 */

#include <soyuz/tray.hh>

#include <fmt/format.h>
#include <sstream>

#include <soyuz/library.hh>
#include <soyuz/resource.hh>
#include <soyuz/soyuz.hh>

UINT WM_TASKBAR =   0;
HWND window;
HMENU menu;
NOTIFYICONDATA data;
TCHAR tip[64] =     TEXT(WINDOW_TRAY_NAME);
char class_name[] = WINDOW_TRAY_NAME;
std::vector<soyuz::log_t> logs;

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (message == WM_TASKBAR && !IsWindowVisible(window)) {
    minimize(); return 0;
  }

  switch (message) {
    case WM_ACTIVATE:
      Shell_NotifyIcon(NIM_ADD, &data);
      break;

    case WM_PAINT: {
      PAINTSTRUCT ps;
      RECT rect;
      HDC hdc = BeginPaint(window, &ps);

      GetClientRect(hwnd, &rect);

      int height = 5;
      for (soyuz::log_t &i : logs) {
        SetTextColor(hdc, i.to_coloref());
        TextOut(hdc, 5, height, i.value.c_str(), (int)strlen(i.value.c_str()));
        height += 20;
      }

      EndPaint(window, &ps);
    } break;

    case WM_CREATE:
      ShowWindow(window, SW_HIDE);
      menu = CreatePopupMenu();
      AppendMenu(menu, MF_STRING, ID_TRAY_EXIT, TEXT("Exit Soyuz"));

      break;

    case WM_SYSCOMMAND:
      switch(wParam & 0xFFF0) {
        case SC_MINIMIZE:
        case SC_CLOSE:
          minimize(); return 0; // break;
      } break;

    case WM_SYSICON: {
      switch (wParam) {
        case ID_TRAY_APP_ICON:
          SetForegroundWindow(window);
          break;

        default: ;
      }

      if (lParam == WM_LBUTTONUP) {
        restore();
      } else if (lParam == WM_RBUTTONDOWN) {
        POINT curPoint;
        GetCursorPos(&curPoint);
        SetForegroundWindow(window);

        UINT clicked = TrackPopupMenu(
          menu,
          TPM_RETURNCMD | TPM_NONOTIFY,
          curPoint.x,
          curPoint.y,
          0,
          hwnd,
          nullptr
        );

        SendMessage(hwnd, WM_NULL, 0, 0);
        if (clicked == ID_TRAY_EXIT) {
          Shell_NotifyIcon(NIM_DELETE, &data);
          PostQuitMessage(0);
        }
      }
    } break;

    case WM_NCHITTEST: {
      LRESULT uHitTest = DefWindowProc(hwnd, WM_NCHITTEST, wParam, lParam);
      if (uHitTest == HTCLIENT) {
        return HTCAPTION;
      } else {
        return uHitTest;
      }
    }

    case WM_CLOSE:
      minimize(); return 0; // break;

    case WM_DESTROY:
      PostQuitMessage (0); break;

    default: ;
  }

  return DefWindowProc(hwnd, message, wParam, lParam);
}

void minimize() { ShowWindow(window, SW_HIDE); }
void restore() { ShowWindow(window, SW_SHOW); }

void InitNotifyIconData() {
  memset(&data, 0, sizeof(NOTIFYICONDATA));

  data.cbSize = sizeof(NOTIFYICONDATA);
  data.hWnd = window;
  data.uID = ID_TRAY_APP_ICON;
  data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  data.uCallbackMessage = WM_SYSICON;
  data.hIcon = (HICON)LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(ICO1));
  strncpy_s(data.szTip, tip, sizeof(tip));
}

namespace soyuz {

auto log(const std::string &message) -> void {
  if (logs.size() == 16) { logs.erase(logs.begin()); }

  std::string to_log = fmt::format("[{}] {}", current_date_time(), message);

  logs.emplace_back(log_t(log_level::info, to_log)); write_log_file(to_log);
  RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}

auto log(log_level level, const std::string &message) -> void {
  if (logs.size() == 16) { logs.erase(logs.begin()); }

  std::string to_log = fmt::format("[{}] {}", current_date_time(), message);

  logs.emplace_back(log_t(level, to_log)); write_log_file(to_log);
  RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}

}
