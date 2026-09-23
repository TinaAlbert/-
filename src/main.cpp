#define SDL_MAIN_HANDLED
#define NOMINMAX
#include <SDL.h>
#include <SDL_syswm.h>
#include <windows.h>
#include <commdlg.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <tuple>
#include "graphics.hpp"
using namespace std;
using namespace graphics;

bool ChooseRGB(HWND hwnd, uint32_t& target) {
    static COLORREF custom[16] = { 0 };
    CHOOSECOLOR cc{};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hwnd;
    cc.lpCustColors = custom;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    cc.rgbResult = RGB((target >> 16) & 255, (target >> 8) & 255, target & 255);
    if (ChooseColor(&cc)) {
        target = makeColor(GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult));
        return true;
    }
    return false;
}

#define ID_EDIT 200
#define ID_PROMPT 201

struct InputParam {
    char* out; int size;
};

INT_PTR CALLBACK InputDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static InputParam* param = nullptr;
    switch (message) {
    case WM_INITDIALOG:
        param = (InputParam*)lParam;
        if (param && param->out) SetDlgItemTextA(hDlg, ID_EDIT, param->out);
        SetFocus(GetDlgItem(hDlg, ID_EDIT));
        return (INT_PTR)FALSE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            if (param && param->out) GetDlgItemTextA(hDlg, ID_EDIT, param->out, param->size);
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        return (INT_PTR)FALSE;
    }
    return (INT_PTR)FALSE;
}

LPWORD AlignDWord(LPWORD lp) {
    return (LPWORD)(((uintptr_t)lp + 3) & ~3);
}

bool InputBox(HINSTANCE hinst, HWND hwndOwner, const char* prompt, const char* title, const char* default_text, char* out_buf, int buf_size) {
    HGLOBAL hgbl = GlobalAlloc(GMEM_ZEROINIT, 4096);
    if (!hgbl) return false;
    LPDLGTEMPLATE lpdt = (LPDLGTEMPLATE)GlobalLock(hgbl);
    if (!lpdt) {
        GlobalFree(hgbl); return false;
    }

    lpdt->style = WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION;
    lpdt->cdit = 4;
    lpdt->x = 10; lpdt->y = 10;
    lpdt->cx = 200; lpdt->cy = 80;

    LPWORD lpw = (LPWORD)(lpdt + 1);
    *lpw++ = 0;      *lpw++ = 0;
    LPWSTR lpwsz = (LPWSTR)lpw;
    int nchar = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    if (nchar > 0) MultiByteToWideChar(CP_UTF8, 0, title, -1, lpwsz, nchar);
    lpw = (LPWORD)(lpwsz + nchar);

    lpw = AlignDWord(lpw);
    LPDLGITEMTEMPLATE lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 20; lpdit->y = 50; lpdit->cx = 50; lpdit->cy = 14; lpdit->id = IDOK;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0080;      lpwsz = (LPWSTR)lpw; nchar = MultiByteToWideChar(CP_UTF8, 0, "OK", -1, NULL, 0);
    if (nchar > 0) MultiByteToWideChar(CP_UTF8, 0, "OK", -1, lpwsz, nchar);
    lpw = (LPWORD)(lpwsz + nchar); *lpw++ = 0;

    lpw = AlignDWord(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 120; lpdit->y = 50; lpdit->cx = 50; lpdit->cy = 14; lpdit->id = IDCANCEL;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0080; lpwsz = (LPWSTR)lpw; nchar = MultiByteToWideChar(CP_UTF8, 0, "Cancel", -1, NULL, 0);
    if (nchar > 0) MultiByteToWideChar(CP_UTF8, 0, "Cancel", -1, lpwsz, nchar);
    lpw = (LPWORD)(lpwsz + nchar); *lpw++ = 0;

    lpw = AlignDWord(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | SS_LEFT;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 10; lpdit->y = 10; lpdit->cx = 180; lpdit->cy = 14; lpdit->id = ID_PROMPT;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0082;      lpwsz = (LPWSTR)lpw; nchar = MultiByteToWideChar(CP_UTF8, 0, prompt, -1, NULL, 0);
    if (nchar > 0) MultiByteToWideChar(CP_UTF8, 0, prompt, -1, lpwsz, nchar);
    lpw = (LPWORD)(lpwsz + nchar); *lpw++ = 0;

    lpw = AlignDWord(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | WS_TABSTOP;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 10; lpdit->y = 30; lpdit->cx = 180; lpdit->cy = 14; lpdit->id = ID_EDIT;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0081;      lpwsz = (LPWSTR)lpw; nchar = MultiByteToWideChar(CP_UTF8, 0, default_text, -1, NULL, 0);
    if (nchar > 0) MultiByteToWideChar(CP_UTF8, 0, default_text, -1, lpwsz, nchar);
    lpw = (LPWORD)(lpwsz + nchar); *lpw++ = 0;

    InputParam param = { out_buf, buf_size };
    INT_PTR ret = DialogBoxIndirectParamA(hinst, (LPCDLGTEMPLATE)lpdt, hwndOwner, InputDlgProc, (LPARAM)&param);

    GlobalUnlock(hgbl);
    GlobalFree(hgbl);
    return ret == IDOK;
}

bool parsePositive(const char* text, double maximum, double& result) {
    char* end = nullptr;
    const double value = strtod(text, &end);
    if (end == text || *end != '\0' || !isfinite(value) || value <= 0 || value > maximum) return false;
    result = value;
    return true;
}

bool saveCanvas(const char* path) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
        pixels.data(), WIDTH, HEIGHT, 32, WIDTH * sizeof(uint32_t), SDL_PIXELFORMAT_ARGB8888);
    if (!surface) return false;
    const bool saved = SDL_SaveBMP(surface, path) == 0;
    SDL_FreeSurface(surface);
    return saved;
}

void updateTitle(SDL_Window* window, int mode, int submode) {
    const char* modes[] = {"Select G / A / T / B", "Shapes", "Pattern fill", "Cube transforms", "Bezier"};
    const char* shapes[] = {"1 rectangle | 2 circle | 3 color", "Drag to draw rectangle", "Drag from center for circle", "Color selected; press 1 or 2"};
    const char* area[] = {"1 polygon | 2 colors", "Left: add vertex | Right: close polygon", "Colors selected; press 1"};
    const char* cubeModes[] = {"1 reset | 2-4 translate | 5-7 rotate | 8 steps", "Cube reset", "Translate X", "Translate Y", "Translate Z", "Rotate X (world origin)", "Rotate Y (world origin)", "Rotate Z (world origin)", "Steps updated; select 2-7"};
    string detail;
    if (mode == 1) detail = shapes[submode];
    else if (mode == 2) detail = area[submode];
    else if (mode == 3) detail = string(cubeModes[submode]) + " | +/- apply";
    else if (mode == 4) detail = "1: select tool | Click four control points";
    const string title = string("Graphics Algorithms Lab | ") + modes[mode] + " | " + detail + " | F1 demo | F2 save BMP | K clear";
    SDL_SetWindowTitle(window, title.c_str());
}

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    SDL_SetMainReady();
    bool smokeTest = false;
    string exportPath;
    int initialScene = 1;
    for (int i = 1; i < argc; ++i) {
        const string arg = argv[i];
        if (arg == "--smoke-test") smokeTest = true;
        else if (arg == "--export-demo" && i + 1 < argc) exportPath = argv[++i];
        else if (arg == "--scene" && i + 1 < argc) {
            const string value = argv[++i];
            if (value.size() != 1 || value[0] < '1' || value[0] > '4') return 2;
            initialScene = value[0] - '0';
        }
        else {
            cout << "Usage: graphics-lab [--scene 1..4] [--export-demo output.bmp] [--smoke-test]\n";
            return arg == "--help" ? 0 : 2;
        }
    }
    const bool automated = smokeTest || !exportPath.empty();
    cout << "按键指南:" << endl;
    cout << "g: 图形绘制模式 (1:矩形 2:圆形 3:设置颜色)" << endl;
    cout << "a: 区域填充模式 (1:多边形 2:设置颜色)" << endl;
    cout << "t: 三维变换模式 (1:重置立方体 2:x平移 3:y平移 4:z平移 5:x旋转 6:y旋转 7:z旋转 8:设置步长/角度)" << endl;
    cout << "   在三维子模式下按 = (或 小键盘 +) 做正向，按 - (或 小键盘 -) 做反向。" << endl;
    cout << "b: 绘制曲线模式 (1:Bezier)" << endl;
    cout << "k: 清当前组" << endl;
    cout << "F1: 加载当前模式示例；F2: 保存画布到工作目录的 canvas.bmp" << endl;
    cout << "Esc: 退出" << endl;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        cerr << "初始化失败: " << SDL_GetError() << endl; return 1;
    }
    SDL_Window* win = SDL_CreateWindow("Graphics Algorithms Lab", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, automated ? SDL_WINDOW_HIDDEN : SDL_WINDOW_RESIZABLE);
    if (!win) {
        cerr << "窗口失败: " << SDL_GetError() << endl; SDL_Quit(); return 1;
    }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) {
        cerr << "渲染失败: " << SDL_GetError() << endl; SDL_DestroyWindow(win); SDL_Quit(); return 1;
    }
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!tex) {
        cerr << "Texture creation failed: " << SDL_GetError() << endl;
        SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit(); return 1;
    }
    pixels.assign(WIDTH * HEIGHT, makeColor(255, 255, 255));
    initFont();

    SDL_SysWMinfo info; SDL_VERSION(&info.version); const bool hasNativeWindow = SDL_GetWindowWMInfo(win, &info) == SDL_TRUE;
    HWND hwnd = hasNativeWindow ? info.info.win.window : nullptr;

    bool running = true; bool mouseDown = false; Point start{ 0,0 }, curr{ 0,0 };
    double cubeFocal = 500.0; int main_mode = initialScene, sub_mode = 1;
    vector<pair<Point, Point>> graphic_lines; vector<tuple<int, int, int>> graphic_circles;
    vector<vector<Point>> area_polygons; vector<Point> area_currPoly;
    vector<Point> bezier_stored; vector<Point> bezier_controls; Cube cube;

    auto loadDemo = [&]() {
        mouseDown = false;
        sub_mode = 1;
                if (main_mode == 1) {
            graphic_lines = {{{160,180},{460,180}}, {{460,180},{460,480}}, {{460,480},{160,480}}, {{160,480},{160,180}}};
            graphic_circles = {{690,330,150}, {690,330,85}};
        } else if (main_mode == 2) {
            area_currPoly.clear();
            area_polygons = {{{180,160},{470,200},{470,350},{780,350},{780,560},{180,560}}};
        } else if (main_mode == 3) {
            cube = Cube(); cube.rotate(0.4, 0.6, 0.15);
        } else if (main_mode == 4) {
            bezier_controls.clear();
            bezier_stored = {{140,540},{330,100},{660,620},{880,180}};
        }
    };
    if (automated) loadDemo();
    updateTitle(win, main_mode, sub_mode);
    int exitCode = 0;
    int frames = 0;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                if (e.window.data1 <= 0 || e.window.data2 <= 0) continue;
                WIDTH = e.window.data1; HEIGHT = e.window.data2;
                if (tex) SDL_DestroyTexture(tex);
                tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
                if (!tex) { cerr << SDL_GetError() << endl; running = false; exitCode = 1; break; }
                pixels.assign(WIDTH * HEIGHT, makeColor(255, 255, 255));
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    mouseDown = true; start.x = e.button.x; start.y = e.button.y; curr = start;
                    if (main_mode == 2 && sub_mode == 1) area_currPoly.push_back({ start.x, start.y });
                    else if (main_mode == 4 && sub_mode == 1) {
                        bezier_controls.push_back({ start.x, start.y });
                        if (bezier_controls.size() == 4) {
                            bezier_stored.insert(bezier_stored.end(), bezier_controls.begin(), bezier_controls.end()); bezier_controls.clear();
                        }
                    }
                }
                else if (e.button.button == SDL_BUTTON_RIGHT) {
                    if (main_mode == 2 && sub_mode == 1 && area_currPoly.size() >= 3) {
                        area_polygons.push_back(area_currPoly); area_currPoly.clear();
                    }
                }
            }
            else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
                if (!mouseDown) continue;
                mouseDown = false; Point end{ e.button.x, e.button.y };
                if (main_mode == 1) {
                    if (sub_mode == 1) {
                        int x1 = min(start.x, end.x), y1 = min(start.y, end.y);
                        int w = abs(start.x - end.x), h = abs(start.y - end.y);
                        graphic_lines.push_back({ {x1,y1},{x1 + w,y1} });
                        graphic_lines.push_back({ {x1 + w,y1},{x1 + w,y1 + h} });
                        graphic_lines.push_back({ {x1 + w,y1 + h},{x1,y1 + h} });
                        graphic_lines.push_back({ {x1,y1 + h},{x1,y1} });
                    }
                    else if (sub_mode == 2) {
                        int dx = end.x - start.x, dy = end.y - start.y;
                        int r = (int)round(sqrt(dx * dx + dy * dy)); graphic_circles.emplace_back(start.x, start.y, r);
                    }
                }
            }
            else if (e.type == SDL_MOUSEMOTION) {
                curr.x = e.motion.x; curr.y = e.motion.y;
            }
            else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_ESCAPE) {
                    running = false; break;
                }
                else if (k == SDLK_g) {
                    mouseDown = false;
                    main_mode = 1; sub_mode = 0; cout << "进入图形绘制模式" << endl;
                }
                else if (k == SDLK_a) {
                    mouseDown = false;
                    main_mode = 2; sub_mode = 0; cout << "进入区域填充模式" << endl;
                }
                else if (k == SDLK_t) {
                    mouseDown = false;
                    main_mode = 3; sub_mode = 0; cout << "进入三维变换模式" << endl;
                }
                else if (k == SDLK_b) {
                    mouseDown = false;
                    main_mode = 4; sub_mode = 0; cout << "进入曲线绘制模式" << endl;
                }
                else if (k == SDLK_F1) { loadDemo(); }
                else if (k == SDLK_F2) {
                    if (saveCanvas("canvas.bmp")) cout << "Saved canvas.bmp in the working directory" << endl;
                    else cerr << "Save failed: " << SDL_GetError() << endl;
                }
                else if (k == SDLK_k) {
                    mouseDown = false;
                    if (main_mode == 3) cube = Cube();
                if (main_mode == 1) {
                        graphic_lines.clear(); graphic_circles.clear();
                    }
                    else if (main_mode == 2) {
                        area_polygons.clear(); area_currPoly.clear();
                    }
                    else if (main_mode == 4) {
                        bezier_stored.clear(); bezier_controls.clear();
                    } cout << "已清除当前组" << endl;
                }

                if (main_mode == 1) {
                    if (k == SDLK_1) {
                        sub_mode = 1; cout << "图形模式: 矩形" << endl;
                    }
                    else if (k == SDLK_2) {
                        sub_mode = 2; cout << "图形模式: 圆形" << endl;
                    }
                    else if (k == SDLK_3) {
                        sub_mode = 3; ChooseRGB(hwnd, lineColor); cout << "线颜色设置完成" << endl;
                    }
                }
                else if (main_mode == 2) {
                    if (k == SDLK_1) {
                        sub_mode = 1; cout << "区域填充: 多边形" << endl;
                    }
                    else if (k == SDLK_2) {
                        sub_mode = 2; ChooseRGB(hwnd, fillColor); ChooseRGB(hwnd, boundaryColor); cout << "填充与边界颜色设置完成" << endl;
                    }
                }
                else if (main_mode == 3) {
                    if (k == SDLK_1) {
                        sub_mode = 1; cube = Cube(); cout << "立方体重置" << endl;
                    }
                    else if (k >= SDLK_2 && k <= SDLK_7) {
                        sub_mode = (int)(k - SDLK_0); cout << "三维子模式: " << sub_mode << endl;
                    }
                    else if (k == SDLK_8) {
                        sub_mode = 8;
                        char buf[64] = { 0 }; sprintf(buf, "%d", move_step);
                        if (InputBox(GetModuleHandle(NULL), hwnd, "平移步长 (整数 1-1000)", "设置数据", buf, buf, 64)) {
                            double value;
                            if (parsePositive(buf, 1000, value) && floor(value) == value) move_step = int(value);
                            else cerr << "Invalid translation step; previous value retained." << endl;
                        }
                        memset(buf, 0, sizeof(buf)); sprintf(buf, "%.1f", rotate_step_deg);
                        if (InputBox(GetModuleHandle(NULL), hwnd, "旋转角度 (0-360，不能为 0)", "设置数据", buf, buf, 64)) {
                            double value;
                            if (parsePositive(buf, 360, value)) rotate_step_deg = value;
                            else cerr << "Invalid rotation step; previous value retained." << endl;
                        }
                        cout << "设置已保存: move_step=" << move_step << " rotate_step_deg=" << rotate_step_deg << endl;
                    }
                    if (sub_mode >= 2 && sub_mode <= 7) {
                        double step = 0.0; bool is_translate = sub_mode <= 4;
                        if (k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS) step = is_translate ? move_step : rotate_step_deg * PI / 180.0;
                        else if (k == SDLK_MINUS || k == SDLK_KP_MINUS) step = is_translate ? -move_step : -rotate_step_deg * PI / 180.0;
                        if (step != 0.0) {
                            if (sub_mode == 2) cube.translate(step, 0, 0);
                            else if (sub_mode == 3) cube.translate(0, step, 0);
                            else if (sub_mode == 4) cube.translate(0, 0, step);
                            else if (sub_mode == 5) cube.rotate(step, 0, 0);
                            else if (sub_mode == 6) cube.rotate(0, step, 0);
                            else if (sub_mode == 7) cube.rotate(0, 0, step);
                        }
                    }
                }
                else if (main_mode == 4) {
                    if (k == SDLK_1) {
                        sub_mode = 1; cout << "Bezier 模式" << endl;
                    }
                }
            }
        }

        clearBuffer(makeColor(255, 255, 255));
                if (main_mode == 1) {
            for (auto& ln : graphic_lines) drawLine(ln.first.x, ln.first.y, ln.second.x, ln.second.y, lineColor);
            for (auto& c : graphic_circles) drawCircle(get<0>(c), get<1>(c), get<2>(c), lineColor);
            if (mouseDown) {
                if (sub_mode == 1) {
                    int x1 = min(start.x, curr.x), y1 = min(start.y, curr.y);
                    int w = abs(start.x - curr.x), h = abs(start.y - curr.y);
                    drawLine(x1, y1, x1 + w, y1, lineColor);
                    drawLine(x1 + w, y1, x1 + w, y1 + h, lineColor);
                    drawLine(x1 + w, y1 + h, x1, y1 + h, lineColor);
                    drawLine(x1, y1 + h, x1, y1, lineColor);
                }
                else if (sub_mode == 2) {
                    int dx = curr.x - start.x, dy = curr.y - start.y;
                    int r = (int)round(sqrt(dx * dx + dy * dy)); drawCircle(start.x, start.y, r, lineColor);
                }
            }
        }
        else if (main_mode == 2) {
            for (auto& poly : area_polygons) {
                scanlineFill(poly, fillColor);
                for (size_t i = 0; i < poly.size(); ++i) {
                    Point p1 = poly[i], p2 = poly[(i + 1) % poly.size()]; drawLine(p1.x, p1.y, p2.x, p2.y, boundaryColor);
                }
            }
            if (!area_currPoly.empty()) {
                for (size_t i = 0; i + 1 < area_currPoly.size(); ++i) drawLine(area_currPoly[i].x, area_currPoly[i].y, area_currPoly[i + 1].x, area_currPoly[i + 1].y, boundaryColor);
                drawLine(area_currPoly.back().x, area_currPoly.back().y, curr.x, curr.y, boundaryColor);
            }
        }
        else if (main_mode == 3) {
            cube.draw(cubeFocal);
        }
        else if (main_mode == 4) {
            for (size_t i = 0; i + 3 < bezier_stored.size(); i += 4) {
                drawBezier(bezier_stored[i], bezier_stored[i + 1], bezier_stored[i + 2], bezier_stored[i + 3], lineColor);
                const uint32_t controlColor = makeColor(170, 170, 170);
                drawLine(bezier_stored[i].x, bezier_stored[i].y, bezier_stored[i + 1].x, bezier_stored[i + 1].y, controlColor);
                drawLine(bezier_stored[i + 1].x, bezier_stored[i + 1].y, bezier_stored[i + 2].x, bezier_stored[i + 2].y, controlColor);
                drawLine(bezier_stored[i + 2].x, bezier_stored[i + 2].y, bezier_stored[i + 3].x, bezier_stored[i + 3].y, controlColor);
                for (int j = 0; j < 4; ++j) fillRect(bezier_stored[i + j].x, bezier_stored[i + j].y, 5, makeColor(0, 0, 0));
            }
            if (!bezier_controls.empty()) {
                for (size_t i = 0; i + 1 < bezier_controls.size(); ++i) drawLine(bezier_controls[i].x, bezier_controls[i].y, bezier_controls[i + 1].x, bezier_controls[i + 1].y, lineColor);
                drawLine(bezier_controls.back().x, bezier_controls.back().y, curr.x, curr.y, lineColor);
            }
        }

        if (!running) break;
        updateTitle(win, main_mode, sub_mode);
        if (SDL_UpdateTexture(tex, NULL, pixels.data(), WIDTH * sizeof(uint32_t)) != 0 ||
            SDL_RenderClear(ren) != 0 || SDL_RenderCopy(ren, tex, NULL, NULL) != 0) {
            cerr << "Render failed: " << SDL_GetError() << endl;
            exitCode = 1; break;
        }
        SDL_RenderPresent(ren);
        ++frames;
        if (automated && frames == 2) {
            if (!exportPath.empty() && !saveCanvas(exportPath.c_str())) {
                cerr << "Export failed: " << SDL_GetError() << endl; exitCode = 1;
            }
            running = false;
        }
        SDL_Delay(16);
    }

    if (tex) SDL_DestroyTexture(tex);
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);
    SDL_Quit();
    return exitCode;
}
