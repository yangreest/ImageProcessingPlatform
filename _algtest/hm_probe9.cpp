// probe9: 直接抽取 LUT + 定位驱动偏置的全局统计量
//  T1) 256x256 全量程斜坡(index==value) -> 直接得到 f(v) 整条 LUT
//  T2) 子区间斜坡 -> LUT 是否随直方图跨度/中心自相似缩放
//  T3) 背景 sigma=30 + n 个离群像素 V -> 偏置由哪个全局统计量驱动
//  T4) 2/3 级图像的退化条件
//  T5) 块洗牌 vs 像素洗牌 -> 是否存在粗尺度依赖
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <numeric>
#include <cstdlib>
#include <windows.h>
#include "HmImgProApi.h"

struct LCG { uint64_t s = 88172645463325252ULL; double next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (double)(s >> 11) / 9007199254740992.0; } };
static double gauss(LCG& r) { double u = std::max(r.next(), 1e-12), v = r.next(); return sqrt(-2 * log(u)) * cos(2 * 3.14159265358979323846 * v); }

static bool feedRun(std::vector<uint16_t>& d, int w, int h, int bal, int det, int enh, int den, std::vector<uint16_t>& out)
{
    out.clear();
    setRawData((BYTE*)d.data(), w, h); setParam(bal, det, enh, den);
    if (!process()) return false;
    int l = -1; char* p = getProcessedData(l);
    if (!p || l <= 0) return false;
    out.assign((uint16_t*)p, (uint16_t*)p + l / 2);
    return true;
}
static void ms(const std::vector<uint16_t>& v, double& mean, double& sd)
{
    double s = 0, s2 = 0; for (auto x : v) { s += x; s2 += (double)x * x; }
    mean = s / v.size(); double var = s2 / v.size() - mean * mean; sd = var > 0 ? sqrt(var) : 0;
}

int main()
{
    InitiImgPro(false);
    std::vector<uint16_t> out, out2;

    // ================= T1) 全量程斜坡 -> LUT =================
    printf("[T] === T1) 256x256 全量程斜坡 f(v)=out[v] ===\n");
    {
        const int R = 256;
        std::vector<uint16_t> d((size_t)R * R);
        for (int i = 0; i < R * R; ++i) d[i] = (uint16_t)i;
        printf("[T] >>> RUN t1_ramp\n");
        if (feedRun(d, R, R, 0, 0, 0, 0, out)) {
            printf("[T] <<< END ok\n");
            printf("[T]      v      f(v)    delta    slope(local)\n");
            int prev = -1; double pv = 0;
            for (int v = 0; v < 65536; v += 1024) {
                double f = out[v];
                double sl = (prev >= 0) ? (f - pv) / (v - prev) : 0;
                printf("[T]   %6d %9.0f %+9.0f %10.4f\n", v, f, f - v, sl);
                prev = v; pv = f;
            }
            // 单调性与整体统计
            long nonmono = 0; int mn = 65535, mx = 0;
            for (int v = 1; v < 65536; ++v) { if (out[v] < out[v - 1]) ++nonmono; }
            for (auto x : out) { if (x < mn) mn = x; if (x > mx) mx = x; }
            printf("[T]   LUT 非单调点数=%ld  输出 min=%d max=%d  f(0)=%d f(65535)=%d\n", nonmono, mn, mx, out[0], out[65535]);
        }
        else printf("[T] <<< END FAIL\n");
    }

    // ================= T2) 子区间斜坡 =================
    printf("[T] === T2) 子区间斜坡 [lo,hi] -> f 的自相似性 ===\n");
    {
        const int R = 256;
        std::vector<std::array<int,2>> rng = { {0,65535}, {16384,49151}, {24000,36000}, {28000,32000}, {29500,30500}, {0,32767}, {32768,65535} };
        for (auto& rg : rng) {
            std::vector<uint16_t> d((size_t)R * R);
            for (int i = 0; i < R * R; ++i)
                d[i] = (uint16_t)(rg[0] + (double)i * (rg[1] - rg[0]) / (R * R - 1));
            printf("[T] >>> RUN t2_%d_%d\n", rg[0], rg[1]);
            if (!feedRun(d, R, R, 0, 0, 0, 0, out)) { printf("[T] <<< END FAIL\n"); continue; }
            printf("[T] <<< END ok\n");
            // 归一化输入 t=(v-lo)/(hi-lo)，输出归一化 (f-mid)/(hi-lo)
            double lo = rg[0], hi = rg[1], w = hi - lo;
            printf("[T]   range=[%d,%d] width=%.0f\n", rg[0], rg[1], w);
            printf("[T]     t      v        f(v)     (f-v)/w    df/dv\n");
            double pf = 0; int pi = 0;
            for (int k = 0; k <= 10; ++k) {
                int idx = (int)((double)k / 10 * (R * R - 1));
                double v = d[idx], f = out[idx];
                double sl = (k > 0) ? (f - pf) / (v - pi) : 0;
                printf("[T]   %5.2f %8.0f %9.0f %+10.4f %8.4f\n", k / 10.0, v, f, (f - v) / w, sl);
                pf = f; pi = (int)v;
            }
        }
    }

    // ================= T3) 离群像素驱动 =================
    printf("[T] === T3) 背景 30000+sigma30，加 n 个像素值 V -> 背景偏置 ===\n");
    printf("[T]      V     n   bg_in_mean  bg_out_mean   offset   out@V     noise0seeLog\n");
    {
        const int NN = 512; const size_t NP = (size_t)NN * NN;
        std::vector<uint16_t> base(NP); LCG r0;
        for (auto& v : base) v = (uint16_t)std::clamp(30000 + gauss(r0) * 30.0, 0.0, 65535.0);
        double bm = 0, bsd = 0; ms(base, bm, bsd);
        for (int V : { 30200, 31000, 34000, 38000, 45000, 55000, 65000 }) {
            for (int n : { 1, 26, 262, 2621 }) {
                std::vector<uint16_t> d = base;
                for (int i = 0; i < n; ++i) d[(size_t)i * 97 % NP] = (uint16_t)V;
                printf("[T] >>> RUN t3_V%d_n%d\n", V, n);
                if (!feedRun(d, NN, NN, 0, 0, 0, 0, out)) { printf("[T] <<< END FAIL\n"); continue; }
                printf("[T] <<< END ok\n");
                double s = 0; long c = 0;
                for (size_t i = 0; i < NP; ++i) { if (d[i] != (uint16_t)V) { s += out[i]; ++c; } }
                double bgm = s / c;
                printf("[T]   %6d %5d %11.2f %12.2f %+9.2f %8d\n", V, n, bm, bgm, bgm - bm, (int)out[0]);
            }
        }
    }

    // ================= T4) 少级图像退化 =================
    printf("[T] === T4) K 级等间距图像（无噪声）-> 何时恒等 ===\n");
    printf("[T]   K   gap    lo      hi      out_lo  out_hi   maxdiff  identity?\n");
    {
        const int NN = 256; const size_t NP = (size_t)NN * NN;
        for (int K : { 2, 3, 4, 5, 8 }) {
            for (int gap : { 10, 100, 600, 3000, 15000 }) {
                std::vector<uint16_t> d(NP); LCG r;
                int lo = 30000 - (K - 1) * gap / 2;
                for (size_t i = 0; i < NP; ++i) {
                    int k = (int)(r.next() * K); if (k >= K) k = K - 1;
                    d[i] = (uint16_t)std::clamp(lo + k * gap, 0, 65535);
                }
                printf("[T] >>> RUN t4_K%d_g%d\n", K, gap);
                if (!feedRun(d, NN, NN, 0, 0, 0, 0, out)) { printf("[T] <<< END FAIL\n"); continue; }
                printf("[T] <<< END ok\n");
                int mxd = 0; for (size_t i = 0; i < NP; ++i) { int df = (int)out[i] - (int)d[i]; if (df < 0) df = -df; if (df > mxd) mxd = df; }
                printf("[T]   %2d %6d %7d %7d %7d %7d %8d %9s\n", K, gap, lo, lo + (K - 1) * gap,
                    (int)out[0], (int)out[NP - 1], mxd, mxd == 0 ? "YES" : "no");
            }
        }
    }

    // ================= T5) 洗牌尺度依赖 =================
    printf("[T] === T5) 原图 vs 8x8块洗牌 vs 像素洗牌 ===\n");
    {
        const int NN = 512; const size_t NP = (size_t)NN * NN;
        std::vector<uint16_t> d(NP); LCG r;
        for (int y = 0; y < NN; ++y) for (int x = 0; x < NN; ++x) {
            double base = 8000 + 22000 * exp(-((x - 256.0) * (x - 256.0) + (y - 256.0) * (y - 256.0)) / (2 * 90.0 * 90.0));
            d[(size_t)y * NN + x] = (uint16_t)std::clamp(base + gauss(r) * 400.0, 0.0, 65535.0);
        }
        // 8x8 块洗牌
        std::vector<uint16_t> db(NP);
        {
            int nb = NN / 8; std::vector<int> idx(nb * nb); std::iota(idx.begin(), idx.end(), 0);
            for (int i = (int)idx.size() - 1; i > 0; --i) { int j = (int)(r.next() * (i + 1)); if (j > i) j = i; std::swap(idx[i], idx[j]); }
            for (int b = 0; b < nb * nb; ++b) {
                int sy = (idx[b] / nb) * 8, sx = (idx[b] % nb) * 8, dy = (b / nb) * 8, dx = (b % nb) * 8;
                for (int j = 0; j < 8; ++j) for (int i = 0; i < 8; ++i)
                    db[(size_t)(dy + j) * NN + dx + i] = d[(size_t)(sy + j) * NN + sx + i];
            }
        }
        // 像素洗牌
        std::vector<size_t> perm(NP); std::iota(perm.begin(), perm.end(), 0u);
        for (size_t i = NP - 1; i > 0; --i) { size_t j = (size_t)(r.next() * (i + 1)); if (j > i) j = i; std::swap(perm[i], perm[j]); }
        std::vector<uint16_t> dp(NP); for (size_t i = 0; i < NP; ++i) dp[i] = d[perm[i]];

        const char* nm[] = { "orig", "block8", "pixel" };
        std::vector<uint16_t>* imgs[] = { &d, &db, &dp };
        std::vector<std::vector<uint16_t>> res(3);
        for (int t = 0; t < 3; ++t) {
            printf("[T] >>> RUN t5_%s\n", nm[t]);
            if (!feedRun(*imgs[t], NN, NN, 0, 0, 0, 0, res[t])) { printf("[T] <<< END FAIL\n"); continue; }
            printf("[T] <<< END ok\n");
        }
        // 对同一输入值 v，统计三种排布下输出的均值（检验 LUT 是否随排布变化）
        printf("[T]     v    out_orig   out_blk8   out_pix     d(bo)     d(po)\n");
        for (int v = 2000; v <= 30000; v += 2000) {
            double a = 0, b = 0, c = 0; long na = 0, nb = 0, nc = 0;
            for (size_t i = 0; i < NP; ++i) {
                if (d[i] >= (uint16_t)v && d[i] < (uint16_t)(v + 2000)) {
                    a += res[0][i]; ++na; b += res[1][i]; ++nb; c += res[2][i]; ++nc;
                }
            }
            if (na) printf("[T]   %5d %10.2f %10.2f %10.2f %+9.2f %+9.2f\n", v, a / na, b / nb, c / nc, b / nb - a / na, c / nc - a / na);
        }
    }
    return 0;
}
