#include <SDL.h>
#include <cmath>
#include <vector>
#include <cstdint>
#include <iostream>
#include <string>
#include <algorithm>
#include <map>
#include <array>
#include <windows.h>
#include <SDL_syswm.h>
#include <cstdlib>
#include <cstring>

using namespace std;

struct Point {
    int x, y;
};
struct Point3 {
    double x, y, z;
};

int WIDTH = 1024, HEIGHT = 720;
double rotate_step_deg = 10.0;
int move_step = 10;

vector<uint32_t> pixels;

uint32_t makeColor(int r, int g, int b) {
    return (0xFFu << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

void clearBuffer(uint32_t color) {
    std::fill(pixels.begin(), pixels.end(), color);
}

inline void setPixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    pixels[y * WIDTH + x] = color;
}

void drawLine(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        setPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy; x0 += sx;
        }
        if (e2 <= dx) {
            err += dx; y0 += sy;
        }
    }
}

void drawCircle(int cx, int cy, int r, uint32_t color) {
    int x = r, y = 0, err = 0;
    while (x >= y) {
        setPixel(cx + x, cy + y, color);
        setPixel(cx + y, cy + x, color);
        setPixel(cx - y, cy + x, color);
        setPixel(cx - x, cy + y, color);
        setPixel(cx - x, cy - y, color);
        setPixel(cx - y, cy - x, color);
        setPixel(cx + y, cy - x, color);
        setPixel(cx + x, cy - y, color);
        if (err <= 0) {
            y++; err += 2 * y + 1;
        }
        if (err > 0) {
            x--; err -= 2 * x + 1;
        }
    }
}

void drawBezier(const Point& p0, const Point& p1, const Point& p2, const Point& p3, uint32_t color) {
    const int steps = 200;
    double prevX = p0.x, prevY = p0.y;
    for (int i = 1; i <= steps; ++i) {
        double t = (double)i / steps;
        double u = 1 - t;
        double x = u * u * u * p0.x + 3 * u * u * t * p1.x + 3 * u * t * t * p2.x + t * t * t * p3.x;
        double y = u * u * u * p0.y + 3 * u * u * t * p1.y + 3 * u * t * t * p2.y + t * t * t * p3.y;
        drawLine((int)round(prevX), (int)round(prevY), (int)round(x), (int)round(y), color);
        prevX = x; prevY = y;
    }
}

using CharFont = array<array<bool, 5>, 7>;
map<char, CharFont> font5x7;

void initFont() {
    font5x7['0'] = CharFont{ {
        {false,true,true,true,false},
        {true,false,false,false,true},
        {true,false,false,false,true},
        {true,false,false,false,true},
        {true,false,false,false,true},
        {true,false,false,false,true},
        {false,true,true,true,false}
    } };
    font5x7['8'] = CharFont{ {
        {false,true,true,true,false},
        {true,false,false,false,true},
        {true,false,false,false,true},
        {false,true,true,true,false},
        {true,false,false,false,true},
        {true,false,false,false,true},
        {false,true,true,true,false}
    } };
    font5x7['2'] = CharFont{ {
        {false,true,true,true,false},
        {true,false,false,false,true},
        {false,false,false,false,true},
        {false,false,false,true,false},
        {false,false,true,false,false},
        {false,true,false,false,false},
        {true,true,true,true,true}
    } };
    font5x7['7'] = CharFont{ {
        {true,true,true,true,true},
        {false,false,false,false,true},
        {false,false,false,true,false},
        {false,false,true,false,false},
        {false,true,false,false,false},
        {true,false,false,false,false},
        {false,false,false,false,false}
    } };
}

void scanlineFill(const vector<Point>& poly, uint32_t color) {
    if (poly.size() < 3) return;
    int minY = INT32_MAX, maxY = INT32_MIN;
    for (auto& p : poly) {
        minY = min(minY, p.y); maxY = max(maxY, p.y);
    }
    const string pattern = "0827";
    const int charW = 5, charH = 7;
    const int patW = (int)pattern.size() * charW;
    for (int y = minY; y <= maxY; ++y) {
        vector<int> inters;
        for (size_t i = 0; i < poly.size(); ++i) {
            Point p1 = poly[i], p2 = poly[(i + 1) % poly.size()];
            if ((p1.y <= y && p2.y > y) || (p2.y <= y && p1.y > y)) {
                double t = double(y - p1.y) / double(p2.y - p1.y);
                int x = int(round(p1.x + t * (p2.x - p1.x)));
                inters.push_back(x);
            }
        }
        if (inters.empty()) continue;
        sort(inters.begin(), inters.end());
        for (size_t k = 0; k + 1 < inters.size(); k += 2) {
            int x1 = inters[k], x2 = inters[k + 1];
            if (x2 < x1) swap(x1, x2);
            for (int x = x1; x <= x2; ++x) {
                int px = ((x % patW) + patW) % patW;
                int py = ((y % charH) + charH) % charH;
                int charIndex = px / charW;
                int localX = px % charW;
                char ch = pattern[charIndex];
                auto it = font5x7.find(ch);
                if (it != font5x7.end()) {
                    if (it->second[py][localX]) setPixel(x, y, color);
                }
            }
        }
    }
}

void solidFill(const vector<Point>& poly, uint32_t color) {
    if (poly.size() < 3) return;
    int minY = INT32_MAX, maxY = INT32_MIN;
    for (auto& p : poly) {
        minY = min(minY, p.y); maxY = max(maxY, p.y);
    }
    for (int y = minY; y <= maxY; ++y) {
        vector<int> inters;
        for (size_t i = 0; i < poly.size(); ++i) {
            Point p1 = poly[i], p2 = poly[(i + 1) % poly.size()];
            if ((p1.y <= y && p2.y > y) || (p2.y <= y && p1.y > y)) {
                double t = double(y - p1.y) / double(p2.y - p1.y);
                int x = int(round(p1.x + t * (p2.x - p1.x)));
                inters.push_back(x);
            }
        }
        if (inters.empty()) continue;
        sort(inters.begin(), inters.end());
        for (size_t k = 0; k + 1 < inters.size(); k += 2) {
            int x1 = inters[k], x2 = inters[k + 1];
            if (x2 < x1) swap(x1, x2);
            for (int x = x1; x <= x2; ++x) setPixel(x, y, color);
        }
    }
}

void fillRect(int cx, int cy, int size, uint32_t color) {
    int half = size / 2;
    for (int dy = -half; dy <= half; ++dy)
        for (int dx = -half; dx <= half; ++dx)
            setPixel(cx + dx, cy + dy, color);
}


struct Cube {
    vector<Point3> v;
    vector<array<int, 4>> faces;
    array<uint32_t, 6> faceColors;
    Cube() {
        double s = 100.0;
        v = { {-s,-s,-s},{s,-s,-s},{s,s,-s},{-s,s,-s}, {-s,-s,s},{s,-s,s},{s,s,s},{-s,s,s} };
        faces = {
            array<int,4>{0,1,2,3},
            array<int,4>{4,7,6,5},
            array<int,4>{0,4,5,1},
            array<int,4>{1,5,6,2},
            array<int,4>{2,6,7,3},
            array<int,4>{3,7,4,0}
        };
        faceColors = { makeColor(255,0,0), makeColor(0,255,0), makeColor(0,0,255),
                       makeColor(255,255,0), makeColor(0,255,255), makeColor(255,0,255) };
    }
    void translate(double dx, double dy, double dz) {
        for (auto& p : v) {
            p.x += dx; p.y += dy; p.z += dz;
        }
    }
    void rotate(double rx, double ry, double rz) {
        for (auto& p : v) {
            double y = p.y * cos(rx) - p.z * sin(rx);
            double z = p.y * sin(rx) + p.z * cos(rx);
            p.y = y; p.z = z;
            double x = p.x * cos(ry) + p.z * sin(ry);
            z = -p.x * sin(ry) + p.z * cos(ry);
            p.x = x; p.z = z;
            x = p.x * cos(rz) - p.y * sin(rz);
            y = p.x * sin(rz) + p.y * cos(rz);
            p.x = x; p.y = y;
        }
    }
    static Point3 sub(const Point3& a, const Point3& b) {
        return Point3{ a.x - b.x, a.y - b.y, a.z - b.z };
    }

    void draw(double focal) {
        int cx = WIDTH / 2, cy = HEIGHT / 2;
        const double EPS = 1e-6;

                 vector<Point> proj(v.size());
        vector<double> zcam(v.size(), 0.0);
        vector<bool> ok(v.size(), false);
        for (size_t i = 0; i < v.size(); ++i) {
            double zc = v[i].z + focal;              zcam[i] = zc;
            if (zc <= EPS) {
                ok[i] = false; continue;
            }              double s = focal / zc;
            proj[i].x = int(round(v[i].x * s)) + cx;
            proj[i].y = int(round(v[i].y * s)) + cy;
            ok[i] = true;
        }

                 vector<pair<double, size_t>> faceOrder;          for (size_t fi = 0; fi < faces.size(); ++fi) {
            bool allOK = true;
            double avgDepth = 0.0;
            for (int k = 0; k < 4; ++k) {
                int idx = faces[fi][k];
                if (!ok[idx]) {
                    allOK = false; break;
                }
                avgDepth += zcam[idx];
            }
            if (!allOK) continue;              avgDepth /= 4.0;
            faceOrder.emplace_back(avgDepth, fi);
        }

                 sort(faceOrder.begin(), faceOrder.end(), [](const auto& a, const auto& b) {
            return a.first > b.first;              });

                 for (auto& ord : faceOrder) {
            size_t fi = ord.second;
            vector<Point> facePoly(4);
            for (int k = 0; k < 4; ++k) facePoly[k] = proj[faces[fi][k]];
            solidFill(facePoly, faceColors[fi]);
            for (int k = 0; k < 4; ++k) {
                Point p1 = facePoly[k], p2 = facePoly[(k + 1) % 4];
                drawLine(p1.x, p1.y, p2.x, p2.y, makeColor(0, 0, 0));
            }
        }
    }
};




                                                                                     
uint32_t lineColor = makeColor(0, 0, 0);
uint32_t fillColor = makeColor(200, 200, 255);
uint32_t boundaryColor = makeColor(0, 0, 0);

 bool ChooseRGB(HWND hwnd, uint32_t& target) {
    static COLORREF custom[16] = { 0 };
    CHOOSECOLOR cc = { 0 };
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hwnd;
    cc.lpCustColors = custom;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    cc.rgbResult = RGB(GetRValue(target), GetGValue(target), GetBValue(target));
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
    int nchar = MultiByteToWideChar(CP_ACP, 0, title, -1, NULL, 0);
    if (nchar > 0) MultiByteToWideChar(CP_ACP, 0, title, -1, lpwsz, nchar);
    lpw = (LPWORD)(lpwsz + nchar);

         lpw = AlignDWord(lpw);
    LPDLGITEMTEMPLATE lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 20; lpdit->y = 50; lpdit->cx = 50; lpdit->cy = 14; lpdit->id = IDOK;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0080;      lpwsz = (LPWSTR)lpw; nchar = MultiByteToWideChar(CP_ACP, 0, "OK", -1, NULL, 0);
    if (nchar > 0) MultiByteToWideChar(CP_ACP, 0, "OK", -1, lpwsz, nchar);
    lpw = (LPWORD)(lpwsz + nchar); *lpw++ = 0;

         lpw = AlignDWord(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 120; lpdit->y = 50; lpdit->cx = 50; lpdit->cy = 14; lpdit->id = IDCANCEL;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0080; lpwsz = (LPWSTR)lpw; nchar = MultiByteToWideChar(CP_ACP, 0, "Cancel", -1, NULL, 0);
    if (nchar > 0) MultiByteToWideChar(CP_ACP, 0, "Cancel", -1, lpwsz, nchar);
    lpw = (LPWORD)(lpwsz + nchar); *lpw++ = 0;

         lpw = AlignDWord(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | SS_LEFT;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 10; lpdit->y = 10; lpdit->cx = 180; lpdit->cy = 14; lpdit->id = ID_PROMPT;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0082;      lpwsz = (LPWSTR)lpw; nchar = MultiByteToWideChar(CP_ACP, 0, prompt, -1, NULL, 0);
    if (nchar > 0) MultiByteToWideChar(CP_ACP, 0, prompt, -1, lpwsz, nchar);
    lpw = (LPWORD)(lpwsz + nchar); *lpw++ = 0;

         lpw = AlignDWord(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | WS_TABSTOP;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 10; lpdit->y = 30; lpdit->cx = 180; lpdit->cy = 14; lpdit->id = ID_EDIT;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0081;      lpwsz = (LPWSTR)lpw; nchar = MultiByteToWideChar(CP_ACP, 0, default_text, -1, NULL, 0);
    if (nchar > 0) MultiByteToWideChar(CP_ACP, 0, default_text, -1, lpwsz, nchar);
    lpw = (LPWORD)(lpwsz + nchar); *lpw++ = 0;

         InputParam param = { out_buf, buf_size };
    INT_PTR ret = DialogBoxIndirectParamA(hinst, (LPCDLGTEMPLATE)lpdt, hwndOwner, InputDlgProc, (LPARAM)&param);

    GlobalUnlock(hgbl);
    GlobalFree(hgbl);
    return ret == IDOK;
}

int main(int argc, char** argv) {
    cout << "按键指南:" << endl;
    cout << "g: 图形绘制模式 (1:矩形 2:圆形 3:设置颜色)" << endl;
    cout << "a: 区域填充模式 (1:多边形 2:设置颜色)" << endl;
    cout << "t: 三维变换模式 (1:重置立方体 2:x平移 3:y平移 4:z平移 5:x旋转 6:y旋转 7:z旋转 8:设置步长/角度)" << endl;
    cout << "   在三维子模式下按 = (或 小键盘 +) 做正向，按 - (或 小键盘 -) 做反向。" << endl;
    cout << "b: 绘制曲线模式 (1:Bezier)" << endl;
    cout << "k: 清当前组" << endl;
    cout << "Esc: 退出" << endl;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        cerr << "初始化失败: " << SDL_GetError() << endl; return 1;
    }
    SDL_Window* win = SDL_CreateWindow("CGWORK0827", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!win) {
        cerr << "窗口失败: " << SDL_GetError() << endl; SDL_Quit(); return 1;
    }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        cerr << "渲染失败: " << SDL_GetError() << endl; SDL_DestroyWindow(win); SDL_Quit(); return 1;
    }
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    pixels.assign(WIDTH * HEIGHT, makeColor(255, 255, 255));
    initFont();

    SDL_SysWMinfo info; SDL_VERSION(&info.version); SDL_GetWindowWMInfo(win, &info);
    HWND hwnd = info.info.win.window;

    bool running = true; bool mouseDown = false; Point start{ 0,0 }, curr{ 0,0 };
    double cubeFocal = 500.0; int main_mode = 0, sub_mode = 0;
    vector<pair<Point, Point>> graphic_lines; vector<tuple<int, int, int>> graphic_circles;
    vector<vector<Point>> area_polygons; vector<Point> area_currPoly;
    vector<Point> bezier_stored; vector<Point> bezier_controls; Cube cube;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                WIDTH = e.window.data1; HEIGHT = e.window.data2;
                if (tex) SDL_DestroyTexture(tex);
                tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
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
                    main_mode = 1; sub_mode = 0; cout << "进入图形绘制模式" << endl;
                }
                else if (k == SDLK_a) {
                    main_mode = 2; sub_mode = 0; cout << "进入区域填充模式" << endl;
                }
                else if (k == SDLK_t) {
                    main_mode = 3; sub_mode = 0; cout << "进入三维变换模式" << endl;
                }
                else if (k == SDLK_b) {
                    main_mode = 4; sub_mode = 0; cout << "进入曲线绘制模式" << endl;
                }
                else if (k == SDLK_k) {
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
                        if (InputBox(GetModuleHandle(NULL), hwnd, "输入平移步长", "设置数据", buf, buf, 64)) move_step = atoi(buf);
                        memset(buf, 0, sizeof(buf)); sprintf(buf, "%.1f", rotate_step_deg);
                        if (InputBox(GetModuleHandle(NULL), hwnd, "输入旋转度数(度)", "设置数据", buf, buf, 64)) rotate_step_deg = atof(buf);
                        cout << "设置已保存: move_step=" << move_step << " rotate_step_deg=" << rotate_step_deg << endl;
                    }
                                         if (sub_mode >= 2 && sub_mode <= 7) {
                        double step = 0.0; bool is_translate = sub_mode <= 4;
                        if (k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS) step = is_translate ? move_step : rotate_step_deg * M_PI / 180.0;
                        else if (k == SDLK_MINUS || k == SDLK_KP_MINUS) step = is_translate ? -move_step : -rotate_step_deg * M_PI / 180.0;
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
                for (size_t i = 0; i < poly.size(); ++i) {
                    Point p1 = poly[i], p2 = poly[(i + 1) % poly.size()]; drawLine(p1.x, p1.y, p2.x, p2.y, boundaryColor);
                }
                scanlineFill(poly, fillColor);
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
                drawLine(bezier_stored[i].x, bezier_stored[i].y, bezier_stored[i + 1].x, bezier_stored[i + 1].y, lineColor);
                drawLine(bezier_stored[i + 1].x, bezier_stored[i + 1].y, bezier_stored[i + 2].x, bezier_stored[i + 2].y, lineColor);
                drawLine(bezier_stored[i + 2].x, bezier_stored[i + 2].y, bezier_stored[i + 3].x, bezier_stored[i + 3].y, lineColor);
                for (int j = 0; j < 4; ++j) fillRect(bezier_stored[i + j].x, bezier_stored[i + j].y, 5, makeColor(0, 0, 0));
            }
            if (!bezier_controls.empty()) {
                for (size_t i = 0; i + 1 < bezier_controls.size(); ++i) drawLine(bezier_controls[i].x, bezier_controls[i].y, bezier_controls[i + 1].x, bezier_controls[i + 1].y, lineColor);
                drawLine(bezier_controls.back().x, bezier_controls.back().y, curr.x, curr.y, lineColor);
            }
        }

        SDL_UpdateTexture(tex, NULL, pixels.data(), WIDTH * sizeof(uint32_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    if (tex) SDL_DestroyTexture(tex);
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
