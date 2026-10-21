// probe4: 色调曲线(wedge) + 核支撑范围(locality) + setBrightnessContrast 语义
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <windows.h>
#include "HmImgProApi.h"

struct LCG { uint64_t s = 88172645463325252ULL; double next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (double)(s >> 11) / 9007199254740992.0; } };
static double gauss(LCG& r) { double u = std::max(r.next(), 1e-12), v = r.next(); return sqrt(-2 * log(u)) * cos(2 * 3.14159265358979323846 * v); }

static bool feedRun(const std::vector<uint16_t>& d, int w, int h, int bal, int det, int enh, int den,
                    std::vector<uint16_t>& out16, std::vector<uint8_t>& out8)
{
    out16.clear(); out8.clear();
    setRawData((BYTE*)d.data(), w, h);
    setParam(bal, det, enh, den);
    if (!process()) return false;
    int l16 = -1; char* p16 = getProcessedData(l16);
    if (p16 && l16 > 0) out16.assign((uint16_t*)p16, (uint16_t*)p16 + l16 / 2);
    int l8 = -1; unsigned char* p8 = getBitmapData(l8);
    if (p8 && l8 > 0) out8.assign(p8, p8 + l8);
    return !out16.empty();
}

int main()
{
    InitiImgPro(false);
    std::vector<uint16_t> o16; std::vector<uint8_t> o8;

    // ---------------- E1: 色调曲线 ----------------
    printf("[T] === E1) 色调曲线：16 级灰阶楔（512x1024，每级 64 行，含 sigma=30 噪声） ===\n");
    {
        const int WW = 512, HH = 1024, BANDS = 16, BH = HH / BANDS;
        std::vector<uint16_t> d((size_t)WW * HH);
        std::vector<double> levelIn(BANDS);
        LCG r;
        for (int b = 0; b < BANDS; ++b) {
            double L = 500.0 + b * (63500.0 - 500.0) / (BANDS - 1);
            levelIn[b] = L;
            for (int y = b * BH; y < (b + 1) * BH; ++y)
                for (int x = 0; x < WW; ++x)
                    d[(size_t)y * WW + x] = (uint16_t)std::clamp(L + gauss(r) * 30.0, 0.0, 65535.0);
        }
        int cfgs[][4] = { {0,0,0,0}, {20,0,0,0}, {0,0,20,0}, {0,20,0,0}, {0,0,0,30}, {5,7,5,3} };
        for (auto& c : cfgs) {
            printf("[T] >>> RUN wedge_%d_%d_%d_%d\n", c[0], c[1], c[2], c[3]);
            if (!feedRun(d, WW, HH, c[0], c[1], c[2], c[3], o16, o8)) { printf("[T] <<< END wedge FAIL\n"); continue; }
            printf("[T] <<< END wedge_%d_%d_%d_%d\n", c[0], c[1], c[2], c[3]);
            printf("[T]   cfg(bal=%d,det=%d,enh=%d,den=%d) 灰阶映射：\n", c[0], c[1], c[2], c[3]);
            for (int b = 0; b < BANDS; ++b) {
                double s = 0, s2 = 0; long n = 0;
                double s8 = 0; long n8 = 0;
                for (int y = b * BH + 26; y < b * BH + 38; ++y)
                    for (int x = 128; x < 384; ++x) {
                        double v = o16[(size_t)y * WW + x]; s += v; s2 += v * v; ++n;
                        if (!o8.empty()) { s8 += o8[(size_t)y * WW + x]; ++n8; }
                    }
                double m = s / n, sd = sqrt(s2 / n - m * m);
                printf("[T]     in=%8.1f -> out16=%9.2f (sd=%6.2f, delta=%+8.2f, ratio=%.4f)\n",
                    levelIn[b], m, sd, m - levelIn[b], m / levelIn[b]);
                if (n8) printf("[T]        out8=%7.3f   out8*257=%9.1f  out16/257=%9.3f\n", s8 / n8, s8 / n8 * 257.0, m / 257.0);
            }
        }
    }

    // ---------------- E2: 核支撑范围 ----------------
    printf("[T] === E2) 核支撑范围：噪声背景(sigma=1500) + 单点 +8000，测差值扩散半径 ===\n");
    {
        const int NN = 256, cx = NN / 2, cy = NN / 2;
        std::vector<uint16_t> base((size_t)NN * NN);
        LCG r;
        for (auto& v : base) v = (uint16_t)std::clamp(30000 + gauss(r) * 1500.0, 0.0, 65535.0);
        std::vector<uint16_t> imp = base;
        imp[(size_t)cy * NN + cx] = (uint16_t)std::min(65535.0, (double)imp[(size_t)cy * NN + cx] + 8000.0);
        std::vector<uint16_t> ob, oi;
        printf("[T] >>> RUN locality_base\n"); feedRun(base, NN, NN, 0, 0, 0, 0, ob, o8); printf("[T] <<< END locality_base\n");
        printf("[T] >>> RUN locality_imp\n");  feedRun(imp,  NN, NN, 0, 0, 0, 0, oi, o8); printf("[T] <<< END locality_imp\n");
        if (ob.size() == oi.size() && !ob.empty()) {
            printf("[T]   按到中心的切比雪夫距离分层统计 |diff|：\n");
            for (int rad = 0; rad <= 40; ++rad) {
                double s = 0, mx = 0; long n = 0;
                for (int y = cy - rad; y <= cy + rad; ++y) for (int x = cx - rad; x <= cx + rad; ++x) {
                    if (x < 0 || y < 0 || x >= NN || y >= NN) continue;
                    if (std::max(abs(x - cx), abs(y - cy)) != rad) continue;
                    double dv = fabs((double)oi[(size_t)y * NN + x] - ob[(size_t)y * NN + x]);
                    s += dv; if (dv > mx) mx = dv; ++n;
                }
                if (n) printf("[T]     r=%2d  n=%4ld  mean|diff|=%9.3f  max|diff|=%9.1f\n", rad, n, s / n, mx);
            }
            printf("[T]   中心点 diff=%d\n", (int)oi[(size_t)cy * NN + cx] - (int)ob[(size_t)cy * NN + cx]);
        }
    }

    // ---------------- E3: setBrightnessContrast 语义 ----------------
    printf("[T] === E3) setBrightnessContrast 对 16bit/8bit 输出的影响 ===\n");
    {
        const int WW = 512, HH = 1024, BANDS = 16, BH = HH / BANDS;
        std::vector<uint16_t> d((size_t)WW * HH); LCG r;
        for (int b = 0; b < BANDS; ++b) {
            double L = 500.0 + b * (63500.0 - 500.0) / (BANDS - 1);
            for (int y = b * BH; y < (b + 1) * BH; ++y) for (int x = 0; x < WW; ++x)
                d[(size_t)y * WW + x] = (uint16_t)std::clamp(L + gauss(r) * 30.0, 0.0, 65535.0);
        }
        setRawData((BYTE*)d.data(), WW, HH);
        setParam(0, 0, 0, 0);
        process();
        int bcs[][2] = { {0,0}, {1000,1000}, {30000,30000}, {6452,1977}, {65535,0}, {20000,40000}, {40000,20000} };
        for (auto& bc : bcs) {
            printf("[T] >>> RUN bc_%d_%d\n", bc[0], bc[1]);
            int rr = setBrightnessContrast(bc[0], bc[1]);
            int l16 = -1; char* p16 = getProcessedData(l16);
            int l8 = -1; unsigned char* p8 = getBitmapData(l8);
            printf("[T] <<< END bc_%d_%d ret=%d l16=%d l8=%d\n", bc[0], bc[1], rr, l16, l8);
            if (!p16 || !p8) { printf("[T]   NULL\n"); continue; }
            // 在若干灰阶带中心取样
            printf("[T]   brightness=%d contrast=%d\n", bc[0], bc[1]);
            for (int b = 0; b < BANDS; b += 3) {
                int y = b * BH + 32, x = 256;
                printf("[T]     band%2d in=%6u out16=%6u out8=%3u  (out16/257=%.2f)\n",
                    b, d[(size_t)y * WW + x], ((uint16_t*)p16)[(size_t)y * WW + x], p8[(size_t)y * WW + x],
                    ((uint16_t*)p16)[(size_t)y * WW + x] / 257.0);
            }
        }
    }
    return 0;
}
