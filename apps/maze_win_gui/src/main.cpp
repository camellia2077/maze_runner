#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "application/services/maze_generation.h"
#include "application/services/maze_solver.h"
#include "common/kernel_version.hpp"
#include "common/version.hpp"
#include "src/runtime/session_runner.h"
#include "src/state/gui_state_store.h"

namespace {

constexpr const char kWindowClassName[] = "MazeWinGuiShellWindow";
constexpr int kRunButtonId = 1001;
constexpr int kCancelButtonId = 1002;
constexpr int kWidthEditId = 1101;
constexpr int kHeightEditId = 1102;
constexpr int kUnitPixelsEditId = 1103;
constexpr int kStartXEditId = 1104;
constexpr int kStartYEditId = 1105;
constexpr int kEndXEditId = 1106;
constexpr int kEndYEditId = 1107;
constexpr int kGenerationComboId = 1108;
constexpr int kSearchComboId = 1109;

constexpr UINT kMsgFrameUpdated = WM_APP + 1;
constexpr UINT kMsgRunCompleted = WM_APP + 2;

using MazeWinGui::State::RunParams;

struct Controls {
  HWND run_button = nullptr;
  HWND cancel_button = nullptr;
  HWND width_edit = nullptr;
  HWND height_edit = nullptr;
  HWND unit_pixels_edit = nullptr;
  HWND start_x_edit = nullptr;
  HWND start_y_edit = nullptr;
  HWND end_x_edit = nullptr;
  HWND end_y_edit = nullptr;
  HWND generation_combo = nullptr;
  HWND search_combo = nullptr;
};

MazeWinGui::State::GuiStateStore g_state_store;
std::unique_ptr<MazeWinGui::Runtime::SessionRunner> g_session_runner;
Controls g_controls;
HWND g_main_window = nullptr;

void SetStatus(const std::string& status) {
  g_state_store.SetStatus(status);
}

void SetRunButtonsState(bool running) {
  if (g_controls.run_button != nullptr) {
    EnableWindow(g_controls.run_button, running ? FALSE : TRUE);
  }
  if (g_controls.cancel_button != nullptr) {
    EnableWindow(g_controls.cancel_button, running ? TRUE : FALSE);
  }
}

auto JsonEscape(const std::string& input) -> std::string {
  std::string escaped;
  escaped.reserve(input.size());
  for (const char ch : input) {
    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(ch);
        break;
    }
  }
  return escaped;
}

auto GetControlText(HWND control) -> std::string {
  if (control == nullptr) {
    return "";
  }
  const int length = GetWindowTextLengthA(control);
  std::string text(static_cast<size_t>(std::max(0, length)), '\0');
  GetWindowTextA(control, text.data(), length + 1);
  return text;
}

auto GetComboSelectedText(HWND combo) -> std::string {
  if (combo == nullptr) {
    return "";
  }
  const LRESULT index = SendMessageA(combo, CB_GETCURSEL, 0, 0);
  if (index == CB_ERR) {
    return "";
  }
  const LRESULT length =
      SendMessageA(combo, CB_GETLBTEXTLEN, static_cast<WPARAM>(index), 0);
  if (length <= 0) {
    return "";
  }
  std::string text(static_cast<size_t>(length), '\0');
  SendMessageA(combo, CB_GETLBTEXT, static_cast<WPARAM>(index),
               reinterpret_cast<LPARAM>(text.data()));
  return text;
}

auto ParsePositiveIntField(HWND control, const char* name, int& out_value,
                           std::string& error) -> bool {
  const std::string text = GetControlText(control);
  try {
    const int value = std::stoi(text);
    if (value <= 0) {
      error = std::string(name) + " must be > 0.";
      return false;
    }
    out_value = value;
    return true;
  } catch (...) {
    error = std::string(name) + " is not a valid integer.";
    return false;
  }
}

auto ParseNonNegativeIntField(HWND control, const char* name, int& out_value,
                              std::string& error) -> bool {
  const std::string text = GetControlText(control);
  try {
    const int value = std::stoi(text);
    if (value < 0) {
      error = std::string(name) + " must be >= 0.";
      return false;
    }
    out_value = value;
    return true;
  } catch (...) {
    error = std::string(name) + " is not a valid integer.";
    return false;
  }
}

auto TryParseIntFromControl(HWND control, int& out_value) -> bool {
  const std::string text = GetControlText(control);
  try {
    out_value = std::stoi(text);
    return true;
  } catch (...) {
    return false;
  }
}

auto ClampInt(int value, int low, int high) -> int {
  if (value < low) {
    return low;
  }
  if (value > high) {
    return high;
  }
  return value;
}

void AutoAdjustCoordinatesForBounds(bool align_end_to_max) {
  int width = 0;
  int height = 0;
  if (!TryParseIntFromControl(g_controls.width_edit, width) ||
      !TryParseIntFromControl(g_controls.height_edit, height) || width <= 0 ||
      height <= 0) {
    return;
  }

  int start_x = 0;
  if (TryParseIntFromControl(g_controls.start_x_edit, start_x)) {
    const int clamped_x = ClampInt(start_x, 0, width - 1);
    if (clamped_x != start_x) {
      SetWindowTextA(g_controls.start_x_edit, std::to_string(clamped_x).c_str());
    }
  }

  int start_y = 0;
  if (TryParseIntFromControl(g_controls.start_y_edit, start_y)) {
    const int clamped_y = ClampInt(start_y, 0, height - 1);
    if (clamped_y != start_y) {
      SetWindowTextA(g_controls.start_y_edit, std::to_string(clamped_y).c_str());
    }
  }

  int end_x = 0;
  if (TryParseIntFromControl(g_controls.end_x_edit, end_x)) {
    const int clamped_x =
        align_end_to_max ? (width - 1) : ClampInt(end_x, 0, width - 1);
    if (clamped_x != end_x) {
      SetWindowTextA(g_controls.end_x_edit, std::to_string(clamped_x).c_str());
    }
  }

  int end_y = 0;
  if (TryParseIntFromControl(g_controls.end_y_edit, end_y)) {
    const int clamped_y =
        align_end_to_max ? (height - 1) : ClampInt(end_y, 0, height - 1);
    if (clamped_y != end_y) {
      SetWindowTextA(g_controls.end_y_edit, std::to_string(clamped_y).c_str());
    }
  }
}

auto BuildRequestJsonFromControls(std::string& out_json_request,
                                  std::string& out_label, std::string& error,
                                  RunParams& out_params) -> bool {
  int width = 0;
  int height = 0;
  int unit_pixels = 0;
  int start_x = 0;
  int start_y = 0;
  int end_x = 0;
  int end_y = 0;

  if (!ParsePositiveIntField(g_controls.width_edit, "width", width, error) ||
      !ParsePositiveIntField(g_controls.height_edit, "height", height, error) ||
      !ParsePositiveIntField(g_controls.unit_pixels_edit, "unit_pixels",
                             unit_pixels, error) ||
      !ParseNonNegativeIntField(g_controls.start_x_edit, "start_x", start_x,
                                error) ||
      !ParseNonNegativeIntField(g_controls.start_y_edit, "start_y", start_y,
                                error) ||
      !ParseNonNegativeIntField(g_controls.end_x_edit, "end_x", end_x, error) ||
      !ParseNonNegativeIntField(g_controls.end_y_edit, "end_y", end_y, error)) {
    return false;
  }

  if (start_x >= width || end_x >= width || start_y >= height || end_y >= height) {
    error = "start/end must be inside maze bounds.";
    return false;
  }

  const std::string generation = GetComboSelectedText(g_controls.generation_combo);
  const std::string search = GetComboSelectedText(g_controls.search_combo);
  if (generation.empty()) {
    error = "generation algorithm is required.";
    return false;
  }
  if (search.empty()) {
    error = "search algorithm is required.";
    return false;
  }

  MazeGeneration::MazeAlgorithmType generation_type;
  if (!MazeGeneration::try_parse_algorithm(generation, generation_type)) {
    error = "Unknown generation algorithm: " + generation;
    return false;
  }
  MazeSolver::SolverAlgorithmType search_type;
  if (!MazeSolver::TryParseAlgorithm(search, search_type)) {
    error = "Unknown search algorithm: " + search;
    return false;
  }
  const std::string normalized_generation =
      MazeGeneration::algorithm_name(generation_type);
  const std::string normalized_search = MazeSolver::AlgorithmName(search_type);

  std::ostringstream oss;
  oss << "{"
      << "\"schema_version\":1,"
      << "\"request\":{"
      << "\"maze\":{"
      << "\"width\":" << width << ","
      << "\"height\":" << height << ","
      << "\"start\":{\"x\":" << start_x << ",\"y\":" << start_y << "},"
      << "\"end\":{\"x\":" << end_x << ",\"y\":" << end_y << "}"
      << "},"
      << "\"algorithms\":{"
      << "\"generation\":\"" << JsonEscape(normalized_generation) << "\","
      << "\"search\":\"" << JsonEscape(normalized_search) << "\""
      << "},"
      << "\"visualization\":{"
      << "\"unit_pixels\":" << unit_pixels
      << "},"
      << "\"stream\":{"
      << "\"emit_stride\":1,"
      << "\"emit_progress\":1"
      << "}"
      << "}";

  out_json_request = oss.str();
  out_label = normalized_generation + " + " + normalized_search;
  out_params = {.width = width,
                .height = height,
                .unit_pixels = unit_pixels,
                .start_x = start_x,
                .start_y = start_y,
                .end_x = end_x,
                .end_y = end_y};
  return true;
}

void StartRunAsync() {
  std::string request_json;
  std::string request_label;
  std::string parse_error;
  RunParams run_params;
  if (!BuildRequestJsonFromControls(request_json, request_label, parse_error,
                                    run_params)) {
    SetStatus(std::string("Invalid params: ") + parse_error);
    InvalidateRect(g_main_window, nullptr, TRUE);
    return;
  }

  if (g_session_runner == nullptr) {
    SetStatus("Session runner is not initialized.");
    InvalidateRect(g_main_window, nullptr, TRUE);
    return;
  }

  if (!g_session_runner->Start(run_params, std::move(request_json),
                               std::move(request_label))) {
    InvalidateRect(g_main_window, nullptr, TRUE);
    return;
  }

  SetRunButtonsState(true);
  InvalidateRect(g_main_window, nullptr, TRUE);
}

void RequestCancel() {
  if (g_session_runner == nullptr) {
    SetStatus("Session runner is not initialized.");
    InvalidateRect(g_main_window, nullptr, TRUE);
    return;
  }
  g_session_runner->RequestCancel();
  InvalidateRect(g_main_window, nullptr, TRUE);
}

void DrawFrame(HDC hdc, int x, int y, int target_width, int target_height,
               const std::vector<unsigned char>& bgra, int frame_width,
               int frame_height) {
  if (bgra.empty() || frame_width <= 0 || frame_height <= 0) {
    return;
  }

  BITMAPINFO bitmap_info = {};
  bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bitmap_info.bmiHeader.biWidth = frame_width;
  bitmap_info.bmiHeader.biHeight = -frame_height;
  bitmap_info.bmiHeader.biPlanes = 1;
  bitmap_info.bmiHeader.biBitCount = 32;
  bitmap_info.bmiHeader.biCompression = BI_RGB;

  StretchDIBits(hdc, x, y, target_width, target_height, 0, 0, frame_width,
                frame_height, bgra.data(), &bitmap_info, DIB_RGB_COLORS,
                SRCCOPY);
}

HWND CreateLabeledEdit(HWND parent, const char* label, int label_x, int label_y,
                       int edit_x, int edit_y, int edit_w, int edit_h, int id,
                       const char* initial_text) {
  CreateWindowA("STATIC", label, WS_VISIBLE | WS_CHILD, label_x, label_y, 110, 20,
                parent, nullptr, nullptr, nullptr);
  HWND edit = CreateWindowA("EDIT", initial_text,
                            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                            edit_x, edit_y, edit_w, edit_h, parent,
                            reinterpret_cast<HMENU>(id), nullptr, nullptr);
  return edit;
}

void CreateParameterControls(HWND hwnd) {
  const int left_col_x = 12;
  const int right_col_x = 220;
  const int label_w = 100;
  const int edit_w = 90;
  const int row_h = 26;
  int y = 12;

  g_controls.width_edit =
      CreateLabeledEdit(hwnd, "Width", left_col_x, y + 4, left_col_x + label_w, y,
                        edit_w, 22, kWidthEditId, "8");
  g_controls.height_edit =
      CreateLabeledEdit(hwnd, "Height", right_col_x, y + 4, right_col_x + label_w,
                        y, edit_w, 22, kHeightEditId, "8");
  y += row_h;

  g_controls.unit_pixels_edit = CreateLabeledEdit(
      hwnd, "UnitPixels", left_col_x, y + 4, left_col_x + label_w, y, edit_w, 22,
      kUnitPixelsEditId, "20");
  g_controls.start_x_edit =
      CreateLabeledEdit(hwnd, "Start X", right_col_x, y + 4, right_col_x + label_w,
                        y, edit_w, 22, kStartXEditId, "0");
  y += row_h;

  g_controls.start_y_edit =
      CreateLabeledEdit(hwnd, "Start Y", left_col_x, y + 4, left_col_x + label_w, y,
                        edit_w, 22, kStartYEditId, "0");
  g_controls.end_x_edit =
      CreateLabeledEdit(hwnd, "End X", right_col_x, y + 4, right_col_x + label_w, y,
                        edit_w, 22, kEndXEditId, "7");
  y += row_h;

  g_controls.end_y_edit = CreateLabeledEdit(hwnd, "End Y", left_col_x, y + 4,
                                            left_col_x + label_w, y, edit_w, 22,
                                            kEndYEditId, "7");

  CreateWindowA("STATIC", "Generation", WS_VISIBLE | WS_CHILD, right_col_x, y + 4,
                110, 20, hwnd, nullptr, nullptr, nullptr);
  g_controls.generation_combo = CreateWindowA(
      "COMBOBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | CBS_DROPDOWNLIST,
      right_col_x + label_w, y, edit_w + 80, 220, hwnd,
      reinterpret_cast<HMENU>(kGenerationComboId), nullptr, nullptr);
  SendMessageA(g_controls.generation_combo, CB_ADDSTRING, 0,
               reinterpret_cast<LPARAM>("DFS"));
  SendMessageA(g_controls.generation_combo, CB_ADDSTRING, 0,
               reinterpret_cast<LPARAM>("Prims"));
  SendMessageA(g_controls.generation_combo, CB_ADDSTRING, 0,
               reinterpret_cast<LPARAM>("Kruskal"));
  SendMessageA(g_controls.generation_combo, CB_ADDSTRING, 0,
               reinterpret_cast<LPARAM>("Recursive Division"));
  SendMessageA(g_controls.generation_combo, CB_ADDSTRING, 0,
               reinterpret_cast<LPARAM>("Growing Tree"));
  SendMessageA(g_controls.generation_combo, CB_SETCURSEL, 0, 0);
  y += row_h;

  CreateWindowA("STATIC", "Search", WS_VISIBLE | WS_CHILD, left_col_x, y + 4, 110, 20,
                hwnd, nullptr, nullptr, nullptr);
  g_controls.search_combo = CreateWindowA(
      "COMBOBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | CBS_DROPDOWNLIST,
      left_col_x + label_w, y, edit_w + 80, 220, hwnd,
      reinterpret_cast<HMENU>(kSearchComboId), nullptr, nullptr);
  SendMessageA(g_controls.search_combo, CB_ADDSTRING, 0,
               reinterpret_cast<LPARAM>("BFS"));
  SendMessageA(g_controls.search_combo, CB_ADDSTRING, 0,
               reinterpret_cast<LPARAM>("DFS"));
  SendMessageA(g_controls.search_combo, CB_ADDSTRING, 0,
               reinterpret_cast<LPARAM>("ASTAR"));
  SendMessageA(g_controls.search_combo, CB_ADDSTRING, 0,
               reinterpret_cast<LPARAM>("Dijkstra"));
  SendMessageA(g_controls.search_combo, CB_ADDSTRING, 0,
               reinterpret_cast<LPARAM>("Greedy Best-First"));
  SendMessageA(g_controls.search_combo, CB_SETCURSEL, 0, 0);

  g_controls.run_button = CreateWindowA(
      "BUTTON", "Run JSON", WS_TABSTOP | WS_VISIBLE | WS_CHILD, right_col_x, y + 4,
      100, 28, hwnd, reinterpret_cast<HMENU>(kRunButtonId), nullptr, nullptr);
  g_controls.cancel_button = CreateWindowA(
      "BUTTON", "Cancel", WS_TABSTOP | WS_VISIBLE | WS_CHILD, right_col_x + 110, y + 4,
      100, 28, hwnd, reinterpret_cast<HMENU>(kCancelButtonId), nullptr,
      nullptr);

  AutoAdjustCoordinatesForBounds(true);
  SetRunButtonsState(false);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM w_param,
                         LPARAM l_param) {
  switch (message) {
    case WM_CREATE: {
      g_main_window = hwnd;
      CreateParameterControls(hwnd);
      g_session_runner = std::make_unique<MazeWinGui::Runtime::SessionRunner>(
          g_state_store,
          []() {
            if (g_main_window != nullptr) {
              PostMessage(g_main_window, kMsgFrameUpdated, 0, 0);
            }
          },
          []() {
            if (g_main_window != nullptr) {
              PostMessage(g_main_window, kMsgRunCompleted, 0, 0);
            }
          });
      return 0;
    }
    case WM_COMMAND: {
      const int control_id = LOWORD(w_param);
      const int notify_code = HIWORD(w_param);
      if ((control_id == kWidthEditId || control_id == kHeightEditId) &&
          notify_code == EN_CHANGE) {
        AutoAdjustCoordinatesForBounds(true);
        return 0;
      }
      if ((control_id == kStartXEditId || control_id == kStartYEditId ||
           control_id == kEndXEditId || control_id == kEndYEditId) &&
          notify_code == EN_KILLFOCUS) {
        AutoAdjustCoordinatesForBounds(false);
        return 0;
      }
      if (control_id == kRunButtonId) {
        StartRunAsync();
      } else if (control_id == kCancelButtonId) {
        RequestCancel();
      }
      return 0;
    }
    case kMsgFrameUpdated:
      InvalidateRect(hwnd, nullptr, FALSE);
      return 0;
    case kMsgRunCompleted:
      SetRunButtonsState(false);
      InvalidateRect(hwnd, nullptr, TRUE);
      return 0;
    case WM_PAINT: {
      const MazeWinGui::State::PaintSnapshot snapshot =
          g_state_store.GetPaintSnapshot();

      PAINTSTRUCT paint_struct;
      HDC hdc = BeginPaint(hwnd, &paint_struct);
      RECT rect;
      GetClientRect(hwnd, &rect);

      RECT text_rect = rect;
      text_rect.left = 12;
      text_rect.top = 190;
      text_rect.right = 460;

      char text_buffer[4096];
      std::snprintf(
          text_buffer, sizeof(text_buffer),
          "Status: %s\r\n"
          "Running: %s\r\n"
          "CancelRequested: %s\r\n"
          "ProgressFrameCount: %d\r\n"
          "EventCount: %d\r\n"
          "HasTopologySnapshot: %s\r\n"
          "LastFrame: %dx%d channels=%d\r\n"
          "SummaryJson: %s\r\n",
          snapshot.status.c_str(), snapshot.running ? "true" : "false",
          snapshot.cancel_requested ? "true" : "false", snapshot.frame_count,
          snapshot.callback_count,
          snapshot.has_topology_snapshot ? "true" : "false", snapshot.last_width,
          snapshot.last_height, snapshot.last_channels,
          snapshot.summary_json.c_str());
      DrawTextA(hdc, text_buffer, -1, &text_rect, DT_LEFT | DT_TOP | DT_WORDBREAK);

      RECT frame_rect = rect;
      frame_rect.left = 470;
      frame_rect.top = 12;
      frame_rect.right -= 12;
      frame_rect.bottom -= 12;
      const int target_width =
          std::max(1, static_cast<int>(frame_rect.right - frame_rect.left));
      const int target_height =
          std::max(1, static_cast<int>(frame_rect.bottom - frame_rect.top));
      DrawFrame(hdc, frame_rect.left, frame_rect.top, target_width, target_height,
                snapshot.latest_bgra, snapshot.last_width, snapshot.last_height);

      EndPaint(hwnd, &paint_struct);
      return 0;
    }
    case WM_DESTROY: {
      g_session_runner.reset();
      PostQuitMessage(0);
      return 0;
    }
    default:
      break;
  }
  return DefWindowProc(hwnd, message, w_param, l_param);
}

}  // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance,
                   LPSTR command_line, int command_show) {
  (void)previous_instance;
  (void)command_line;

  WNDCLASSA window_class = {};
  window_class.lpfnWndProc = WndProc;
  window_class.hInstance = instance;
  window_class.lpszClassName = kWindowClassName;
  window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);

  if (RegisterClassA(&window_class) == 0) {
    return 1;
  }

  std::ostringstream window_title;
  window_title << "Maze Windows GUI Shell"
               << " (gui=" << MazeWinGui::kGuiVersion
               << ", kernel=" << MazeKernel::kKernelVersion << ")";
  HWND window = CreateWindowA(kWindowClassName, window_title.str().c_str(),
                              WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                              1280, 760, nullptr, nullptr, instance, nullptr);
  if (window == nullptr) {
    return 1;
  }

  ShowWindow(window, command_show);
  UpdateWindow(window);

  MSG message = {};
  while (GetMessage(&message, nullptr, 0, 0) > 0) {
    TranslateMessage(&message);
    DispatchMessage(&message);
  }
  return static_cast<int>(message.wParam);
}
