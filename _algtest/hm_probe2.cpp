// probe2: 用合成图像辨识 Hamming 算法结构（尺寸约束 / 仿射不变性 / 脉冲响应 / 阶跃响应）
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <windows.h>
#include "HmImgProApi.h"

// ---------- 可复现伪随机 ----------
struct LCG { uint64_t s = 88172645463325252ULL; double next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (double)(s >> 11) / 9007199254740992.0; } };

struct Img { int w, h; std::vector<uint16_t> d;
    Img(int w_, int h_, uint16_t v = 0) : w(w_), h(h_), d((size_t)w_ * h_, v) {}
    uint16_t& at(int x, int y) { return d[(size_t)y * w + x]; }
    uint16_t  at(int x, int y) const { return d[(size_t)y * w + x]; }
};

struct Stat { uint32_t mn, mx; double mean, sd; };
static Stat stat(const uint16_t* p, size_t n)
{
    Stat s{ 65535, 0, 0, 0 }; double sum = 0, sum2 = 0;
    for (size_t i = 0; i < n; ++i) { uint32_t v = p[i]; if (v < s.mn) s.mn = v; if (v > s.mx) s.mx = v; sum += v; sum2 += (double)v * v; }
    s.mean = n ? sum / n : 0; double var = n ? sum2 / n - s.mean * s.mean : 0; s.sd = var > 0 ? sqrt(var) : 0; return s;
}
static void prStat(const char* tag, const Stat& s)
{
    printf("[T]   %-22s min=%-6u max=%-6u mean=%-9.2f sd=%-9.2f dyn=%u\n", tag, s.mn, s.mx, s.mean, s.sd, s.mx - s.mn);
}

// 运行一次 pipeline，返回 16 位输出（失败返回空）
static std::vector<uint16_t> run(const Img& in, int bal, int det, int enh, int den)
{
    std::vector<uint16_t> out;
    setRawData((BYTE*)in.d.data(), in.w, in.h);
    setParam(bal, det, enh, den);
    if (!process()) { printf("[T]   !! process() 返回 0\n"); return out; }
    int len = -1; char* p = getProcessedData(len);
    if (!p || len <= 0) return out;
    out.assign((uint16_t*)p, (uint16_t*)p + len / 2);
    return out;
}

// 生成带噪声的平滑斜坡（保证 sd>0）
static Img makeRamp(int w, int h, double lo, double hi, double noiseAmp)
{
    Img im(w, h); LCG r;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        double t = (double)x / (w > 1 ? w - 1 : 1);
        double v = lo + (hi - lo) * t + (r.next() - 0.5) * noiseAmp;
        im.at(x, y) = (uint16_t)std::clamp(v, 0.0, 65535.0);
    }
    return im;
}

int main()
{
    InitiImgPro(false);

    printf("[T] === A) 尺寸接受度（含噪声斜坡，非恒定） ===\n");
    int sizes[][2] = { {8,8},{16,16},{32,32},{33,17},{64,64},{100,100},{127,65},{128,128},{256,256},{512,512} };
    for (auto& s : sizes) {
        Img im = makeRamp(s[0], s[1], 2000, 40000, 600);
        auto o = run(im, 0, 0, 0, 0);
        printf("[T]   %4dx%-4d out elems=%zu (w*h=%d) %s\n", s[0], s[1], o.size(), s[0]*s[1], o.empty() ? "FAIL" : "OK");
    }

    printf("[T] === B) 仿射不变性：输入线性变换后输出是否一致（判定是否先做全局归一化） ===\n");
    {
        Img a = makeRamp(256, 256, 2000, 40000, 600);
        Img b(a.w, a.h);
        for (size_t i = 0; i < a.d.size(); ++i) b.d[i] = (uint16_t)std::clamp(a.d[i] * 0.25 + 500.0, 0.0, 65535.0);
        Img c(a.w, a.h);
        for (size_t i = 0; i < a.d.size(); ++i) c.d[i] = (uint16_t)std::clamp(a.d[i] * 1.5 - 3000.0, 0.0, 65535.0);
        auto oa = run(a, 0, 0, 0, 0); Stat sa = stat(a.d.data(), a.d.size()); prStat("in A", sa);
        auto ob = run(b, 0, 0, 0, 0); Stat sb = stat(b.d.data(), b.d.size()); prStat("in B=0.25A+500", sb);
        auto oc = run(c, 0, 0, 0, 0); Stat sc = stat(c.d.data(), c.d.size()); prStat("in C=1.5A-3000", sc);
        if (!oa.empty()) prStat("out A", stat(oa.data(), oa.size()));
        if (!ob.empty()) prStat("out B", stat(ob.data(), ob.size()));
        if (!oc.empty()) prStat("out C", stat(oc.data(), oc.size()));
        auto cmp = [](const std::vector<uint16_t>& x, const std::vector<uint16_t>& y, const char* tag) {
            if (x.empty() || y.empty() || x.size() != y.size()) { printf("[T]   %s: 无法比较\n", tag); return; }
            double mad = 0; uint32_t mxd = 0; int ndiff = 0;
            for (size_t i = 0; i < x.size(); ++i) { int d = abs((int)x[i] - (int)y[i]); mad += d; if ((uint32_t)d > mxd) mxd = d; if (d) ++ndiff; }
            printf("[T]   %s: 逐点相同=%d 不同点数=%zu 平均绝对差=%.3f 最大绝对差=%u\n", tag, ndiff == 0, (size_t)ndiff, mad / x.size(), mxd);
        };
        cmp(oa, ob, "outA vs outB");
        cmp(oa, oc, "outA vs outC");
    }

    printf("[T] === C) 输出动态范围是否与输入动态范围无关 ===\n");
    {
        double ranges[][2] = { {10000,10500},{5000,20000},{1000,60000},{30000,35000},{0,65535} };
        for (auto& rr : ranges) {
            Img im = makeRamp(256, 256, rr[0], rr[1], (rr[1] - rr[0]) * 0.02 + 50);
            auto o = run(im, 0, 0, 0, 0);
            printf("[T]   in[%.0f,%.0f]", rr[0], rr[1]);
            if (o.empty()) { printf(" -> FAIL\n"); continue; }
            Stat si = stat(im.d.data(), im.d.size()), so = stat(o.data(), o.size());
            printf(" in(min=%u,max=%u,sd=%.1f) -> out(min=%u,max=%u,mean=%.1f,sd=%.1f)\n", si.mn, si.mx, si.sd, so.mn, so.mx, so.mean, so.sd);
        }
    }

    printf("[T] === D) 脉冲响应（辨识滤波核形状）：斜坡背景 + 单点 +20000 ===\n");
    {
        int N = 129;
        Img im = makeRamp(N, N, 8000, 12000, 200);
        int cx = N / 2, cy = N / 2;
        im.at(cx, cy) = (uint16_t)std::min(65535.0, (double)im.at(cx, cy) + 20000);
        auto o0 = run(im, 0, 0, 0, 0);
        im.at(cx, cy) = (uint16_t)(im.at(cx, cy) - 20000); // 恢复为无脉冲
        auto ob = run(im, 0, 0, 0, 0);
        if (o0.empty() || ob.empty()) { printf("[T]   脉冲实验失败\n"); }
        else {
            printf("[T]   行 y=%d 上 (有脉冲 - 无脉冲) 的差值剖面：\n", cy);
            for (int x = cx - 12; x <= cx + 12; ++x) {
                int d = (int)o0[(size_t)cy * N + x] - (int)ob[(size_t)cy * N + x];
                printf("[T]     dx=%+3d  diff=%+7d %s\n", x - cx, d, d == 0 ? "" : "*");
            }
            printf("[T]   列 x=%d 上差值剖面：\n", cx);
            for (int y = cy - 12; y <= cy + 12; ++y) {
                int d = (int)o0[(size_t)y * N + cx] - (int)ob[(size_t)y * N + cx];
                printf("[T]     dy=%+3d  diff=%+7d %s\n", y - cy, d, d == 0 ? "" : "*");
            }
        }
    }

    printf("[T] === E) 阶跃响应（是否有过冲/欠冲 → 判定锐化类型） ===\n");
    {
        int N = 129, cx = N / 2;
        Img im(N, N); LCG r;
        for (int y = 0; y < N; ++y) for (int x = 0; x < N; ++x)
            im.at(x, y) = (uint16_t)std::clamp((x < cx ? 10000.0 : 30000.0) + (r.next() - 0.5) * 300, 0.0, 65535.0);
        auto o = run(im, 0, 0, 0, 0);
        if (o.empty()) printf("[T]   阶跃实验失败\n");
        else {
            int y = N / 2;
            printf("[T]   输入 / 输出 在阶跃附近（x=cx=%d）：\n", cx);
            for (int x = cx - 10; x <= cx + 10; ++x)
                printf("[T]     x=%3d in=%6u out=%6u\n", x, im.at(x, y), o[(size_t)y * N + x]);
        }
    }
    return 0;
}
