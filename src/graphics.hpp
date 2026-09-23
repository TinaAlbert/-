#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace graphics {
using namespace std;
constexpr double PI = 3.14159265358979323846;

struct Point {
    int x, y;
};
struct Point3 {
    double x, y, z;
};

inline int WIDTH = 1024, HEIGHT = 720;
inline double rotate_step_deg = 10.0;
inline int move_step = 10;

inline vector<uint32_t> pixels;

inline uint32_t makeColor(int r, int g, int b) {
    return (0xFFu << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

inline void clearBuffer(uint32_t color) {
    std::fill(pixels.begin(), pixels.end(), color);
}

inline void setPixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    pixels[y * WIDTH + x] = color;
}

// Liang-Barsky clipping bounds rasterization work to the viewport.
inline void drawLine(int x0, int y0, int x1, int y1, uint32_t color) {
    double t0 = 0.0, t1 = 1.0;
    const double deltaX = double(x1) - x0, deltaY = double(y1) - y0;
    auto clip = [&](double p, double q) {
        if (p == 0.0) return q >= 0.0;
        const double r = q / p;
        if (p < 0.0) { if (r > t1) return false; t0 = max(t0, r); }
        else { if (r < t0) return false; t1 = min(t1, r); }
        return true;
    };
    if (!clip(-deltaX, x0) || !clip(deltaX, WIDTH - 1.0 - x0) ||
        !clip(-deltaY, y0) || !clip(deltaY, HEIGHT - 1.0 - y0)) return;
    x1 = int(round(x0 + t1 * deltaX)); y1 = int(round(y0 + t1 * deltaY));
    x0 = int(round(x0 + t0 * deltaX)); y0 = int(round(y0 + t0 * deltaY));
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

inline void drawCircle(int cx, int cy, int r, uint32_t color) {
    if (r < 0) return;
    int x = r, y = 0, decision = 1 - r;
    while (x >= y) {
        setPixel(cx + x, cy + y, color); setPixel(cx + y, cy + x, color);
        setPixel(cx - y, cy + x, color); setPixel(cx - x, cy + y, color);
        setPixel(cx - x, cy - y, color); setPixel(cx - y, cy - x, color);
        setPixel(cx + y, cy - x, color); setPixel(cx + x, cy - y, color);
        ++y;
        if (decision < 0) decision += 2 * y + 1;
        else { --x; decision += 2 * (y - x) + 1; }
    }
}

// Cubic Bernstein basis, sampled as 200 connected segments.
inline void drawBezier(const Point& p0, const Point& p1, const Point& p2, const Point& p3, uint32_t color) {
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
inline map<char, CharFont> font5x7;

inline void initFont() {
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

// Even-odd scanline fill with a repeating 5x7 bitmap pattern.
inline void scanlineFill(const vector<Point>& poly, uint32_t color) {
    if (poly.size() < 3) return;
    int minY = INT32_MAX, maxY = INT32_MIN;
    for (auto& p : poly) {
        minY = min(minY, p.y); maxY = max(maxY, p.y);
    }
    const string pattern = "0827";
    const int charW = 5, charH = 7;
    const int patW = (int)pattern.size() * charW;
    for (int y = max(0, minY); y <= min(HEIGHT - 1, maxY); ++y) {
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
            for (int x = max(0, x1); x <= min(WIDTH - 1, x2); ++x) {
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

inline void solidFill(const vector<Point>& poly, uint32_t color) {
    if (poly.size() < 3) return;
    int minY = INT32_MAX, maxY = INT32_MIN;
    for (auto& p : poly) {
        minY = min(minY, p.y); maxY = max(maxY, p.y);
    }
    for (int y = max(0, minY); y <= min(HEIGHT - 1, maxY); ++y) {
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
            for (int x = max(0, x1); x <= min(WIDTH - 1, x2); ++x) setPixel(x, y, color);
        }
    }
}

inline void fillRect(int cx, int cy, int size, uint32_t color) {
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
        const double EPS = 1.0; // Faces crossing the near plane are skipped, not clipped.

        vector<Point> proj(v.size());
        vector<double> zcam(v.size(), 0.0);
        vector<bool> ok(v.size(), false);
        for (size_t i = 0; i < v.size(); ++i) {
            double zc = v[i].z + focal;
            zcam[i] = zc;
            if (zc <= EPS) {
                ok[i] = false; continue;
            }
            double s = focal / zc;
            proj[i].x = int(round(clamp(v[i].x * s, -1000000.0, 1000000.0))) + cx;
            proj[i].y = int(round(clamp(v[i].y * s, -1000000.0, 1000000.0))) + cy;
            ok[i] = true;
        }

        // Painter's algorithm: draw complete faces from far to near.
        vector<pair<double, size_t>> faceOrder;
        for (size_t fi = 0; fi < faces.size(); ++fi) {
            bool allOK = true;
            double avgDepth = 0.0;
            for (int k = 0; k < 4; ++k) {
                int idx = faces[fi][k];
                if (!ok[idx]) {
                    allOK = false; break;
                }
                avgDepth += zcam[idx];
            }
            if (!allOK) continue;
            avgDepth /= 4.0;
            faceOrder.emplace_back(avgDepth, fi);
        }

        sort(faceOrder.begin(), faceOrder.end(), [](const auto& a, const auto& b) {
            return a.first > b.first;
        });

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

inline uint32_t lineColor = makeColor(0, 0, 0);
inline uint32_t fillColor = makeColor(200, 200, 255);
inline uint32_t boundaryColor = makeColor(0, 0, 0);

} // namespace graphics
