
#pragma warning( push)
#pragma warning( disable : 28251)

#ifndef UNICODE
#define UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

// voidtools library
#include "Everything.h"

#include <string>
#include <vector>

constexpr wchar_t WIN_CLASS_NAME[] = L"fast-search-palette";
constexpr int EDIT_CONTROL_HEIGHT = 25;
constexpr int LIST_CONTROL_HEIGHT = 17;
constexpr int MAX_SEARCH_RESULTS = 25;

static int win_width = 300;
static int win_height = 25;

struct search_result_t
{
    std::wstring name;
    std::wstring path;
};
static std::vector <search_result_t> search_results;

struct {
    HWND hwnd = nullptr;
    uint64_t id = 65535;
} edit_control;

struct {
    HWND hwnd = nullptr;
    uint64_t id = 65536;
} list_control;

void open_file_or_folder(const std::wstring& path, const std::wstring& name)
{
    if (GetKeyState(VK_CONTROL) & 0x8000) // ctrl is pressed
    {
        // open containing folder:
        ShellExecute(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
    else
    {
        // open the file directly:
        std::wstring full_path = path + L"\\" + name;
        ShellExecute(nullptr, L"open", full_path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

LRESULT CALLBACK edit_control_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR subclass_id, DWORD_PTR ref_data)
{
    // EDIT CONTROL
    switch (msg)
    {
        case WM_KEYDOWN: {
            if (w_param == VK_ESCAPE)
            {
                PostQuitMessage(0);
                return 0;
            }
            else if (w_param == VK_RETURN)
            {
                LRESULT list_count = SendMessage(list_control.hwnd, LB_GETCOUNT, 0, 0);
                if (list_count > 0)
                {
                    open_file_or_folder(search_results[0].path, search_results[0].name);
                    PostQuitMessage(0);
                }
            }
            else if (w_param == VK_DOWN)
            {
                LRESULT list_count = SendMessage(list_control.hwnd, LB_GETCOUNT, 0, 0);
                if (list_count > 0)
                {
                    SendMessage(list_control.hwnd, LB_SETCURSEL, 0, 0);
                    SetFocus(list_control.hwnd);
                }
                return 0;
            }
            return 0; // Add return to prevent fallthrough
        }
        // [[fallthrough]]; // Not needed, as return is present
    }
    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

LRESULT CALLBACK list_control_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR subclass_id, DWORD_PTR ref_data)
{
    // LIST CONTROL
    switch (msg)
    {
        case WM_CHAR:
        {
            SendMessage(hwnd, LB_SETCURSEL, -1, 0); // deselect
			SetWindowText(edit_control.hwnd, (LPCWSTR)&w_param);
			SendMessage(edit_control.hwnd, EM_SETSEL, 1, 1); // move cursor to end
			SetFocus(edit_control.hwnd);
        }
        return 0;
        case WM_KEYDOWN:
        {
            if (w_param == VK_ESCAPE)
            {
                PostQuitMessage(0);
            }
            else if (w_param == VK_RETURN)
            {
                LRESULT current_selection = SendMessage(hwnd, LB_GETCURSEL, 0, 0);
                if (current_selection != LB_ERR && current_selection >= 0 && current_selection < (LRESULT)search_results.size())
                {
					open_file_or_folder(search_results[current_selection].path, search_results[current_selection].name);
					PostQuitMessage(0);
                }
			}
            else if (w_param == VK_UP)
            {
	    		LRESULT current_selection = SendMessage(hwnd, LB_GETCURSEL, 0, 0);
                LRESULT count = SendMessage(hwnd, LB_GETCOUNT, 0, 0);

                if (current_selection == 0 || current_selection == LB_ERR)
                {
					SendMessage(hwnd, LB_SETCURSEL, -1, 0); // deselect
                    SetFocus(edit_control.hwnd);
                }
                else
                {
                    SendMessage(hwnd, LB_SETCURSEL, current_selection-1, 0); 
                }
            }
            else if (w_param == VK_DOWN)
            {
                LRESULT current_selection = SendMessage(hwnd, LB_GETCURSEL, 0, 0);
	    		LRESULT count = SendMessage(hwnd, LB_GETCOUNT, 0, 0);
                if (current_selection+1 < count || current_selection != LB_ERR)
                    SendMessage(hwnd, LB_SETCURSEL, current_selection + 1, 0);
	    		
            }
            return 0;
        }
    }
    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg)
    {
    case WM_CREATE: {
        // create edit control
        edit_control.hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, win_width, EDIT_CONTROL_HEIGHT, hwnd, (HMENU)edit_control.id, GetModuleHandle(nullptr), nullptr);
		SetWindowSubclass(edit_control.hwnd, edit_control_proc, NULL, NULL);
		SetFocus(edit_control.hwnd);

        // create list control
        list_control.hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
			0, EDIT_CONTROL_HEIGHT, win_width, win_height - EDIT_CONTROL_HEIGHT, hwnd, (HMENU)list_control.id, GetModuleHandle(nullptr), nullptr);
		SetWindowSubclass(list_control.hwnd, list_control_proc, NULL, NULL);
    }
	return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
        EndPaint(hwnd, &ps);
	}
    return 0;
    case WM_DESTROY: {
        PostQuitMessage(0);
    }
    return 0;
    case WM_KEYDOWN: {
        if (w_param == VK_ESCAPE)
            PostQuitMessage(0);
        return 0; // Add return to prevent fallthrough
    }
    case WM_COMMAND: {
        if (HIWORD(w_param) == EN_CHANGE && LOWORD(w_param) == edit_control.id)
        {
            // text changed in edit control
            int length = GetWindowTextLength(edit_control.hwnd);
            std::wstring text(length + 1, L'\0');
            GetWindowText(edit_control.hwnd, &text[0], length + 1);
            //text.resize(length);
            // search
            Everything_SetSearch(text.c_str());
            
            Everything_Query    (TRUE);
            // clear listbox
            SendMessage(list_control.hwnd, LB_RESETCONTENT, 0, 0);
            // fill listbox
            int num_results = Everything_GetNumResults();
			win_height = min((num_results * LIST_CONTROL_HEIGHT) + EDIT_CONTROL_HEIGHT, 600);
			SetWindowPos(hwnd, nullptr, 0, 0, win_width, win_height, SWP_NOMOVE | SWP_NOZORDER);
            SetWindowPos(list_control.hwnd, nullptr, 0, EDIT_CONTROL_HEIGHT, win_width, win_height - EDIT_CONTROL_HEIGHT, SWP_NOMOVE | SWP_NOZORDER);

			search_results.clear();
            for (int i = 0; i < num_results; i++)
            {
				search_result_t result;

				result.name = Everything_GetResultFileName(i);
                if (Everything_IsFolderResult(i))
                {
                    LPCWSTR str = Everything_GetResultExtension(i);
                    if (str != nullptr) result.name += str;
                }

				result.path = Everything_GetResultPath(i);

				SendMessage(list_control.hwnd, LB_ADDSTRING, 0, (LPARAM)result.name.c_str());
				search_results.push_back(result);
            }
        }
    }
    return 0;
    }
    return DefWindowProc(hwnd, msg, w_param, l_param);
}

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR cmd_line, int cmd_show)
{   
    InitCommonControls();

	// init everything
    Everything_SetRegex(TRUE);
    Everything_SetSort(EVERYTHING_SORT_NAME_ASCENDING);
    Everything_SetMax(MAX_SEARCH_RESULTS);

    // create window class
    WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hInstance = instance;
	wc.lpfnWndProc = window_proc;
    wc.lpszClassName = WIN_CLASS_NAME;
    
    if (RegisterClassEx(&wc) == 0)
        return EXIT_FAILURE;

    // create HWND
	HWND hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        WIN_CLASS_NAME,
        L"test",
        WS_POPUP | WS_VISIBLE,
        0, 0, win_width, win_height,
        nullptr,
        nullptr,
        instance,
		nullptr);


    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return EXIT_SUCCESS;
}

#pragma warning( pop)