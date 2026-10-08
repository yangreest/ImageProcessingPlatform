// probe5: 决定性实验 —— 单灰阶隔离(纯逐点色调曲线) + 阶跃扩散函数ESF(窗口半径) + 全局耦合范围
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <windows.h>
#include "HmImgProApi.h"

struct LCG { uint64_t s = 88172645463325252ULL; double next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (double)(s >> 11) / 9007199254740992.0; } };
static double gauss(LCG& r) { double u = std::max(r.next(), 1e-12), v = r.next(); return sqrt(-2 * log(u)) * cos(2 * 3.14159265358979323846 * v); }

static bool feedRun(const std::vector<uint16_t>& d, int w, int h, int bal, int det, int enh, int den, std::vector<uint16_t>& out)
{
    out.clear();
    setRawData((BYTE*)d.data(), w, h);
    setParam(bal, det, enh, den);
    if (!process()) return false;
    int l = -1; char* p = getProcessedData(l);
    if (!p || l <= 0) return false;
    out.assign((uint16_t*)p, (uint16_t*)p + l / 2);
    return true;
}
static void statOf(const std::vector<uint16_t>& v, double& mean, double& sd)
{
    double s = 0, s2 = 0;
    for (auto x : v) { s += x; s2 += (double)x * x; }
    mean = s / v.size();
    double var = s2 / v.size() - mean * mean;
    sd = var > 0 ? sqrt(var) : 0;
}

int main()
{
    InitiImgPro(false);
    std::vector<uint16_t> out;
    const int NN = 512;
    const double SG = 300.0;

    // ---------- ISO: 单一灰阶 + 噪声，无任何空间结构 ----------
    printf("[T] === ISO) 单灰阶图(512x512, sigma=%.0f)：纯逐点色调曲线辨识 ===\n", SG);
    int cfgs[][4] = { {0,0,0,0}, {20,0,0,0}, {0,20,0,0}, {0,0,20,0}, {0,0,0,30}, {10,10,10,10} };
    for (auto& c : cfgs) {
        printf("[T] --- cfg bal=%d det=%d enh=%d den=%d ---\n", c[0], c[1], c[2], c[3]);
        printf("[T]      L_in     out_mean    delta      slope_est   out_sd    sd_gain\n");
        double prevL = 0, prevO = 0; int idx = 0;
        for (int b = 0; b < 16; ++b) {
            double L = 1000.0 + b * (63000.0 - 1000.0) / 15.0;
            std::vector<uint16_t> d((size_t)NN * NN);
            LCG r;
            for (auto& v : d) v = (uint16_t)std::clamp(L + gauss(r) * SG, 0.0, 65535.0);
            printf("[T] >>> RUN iso_%d_%d_%d_%d_L%.0f\n", c[0], c[1], c[2], c[3], L);
            if (!feedRun(d, NN, NN, c[0], c[1], c[2], c[3], out)) { printf("[T] <<< END iso FAIL L=%.0f\n", L); continue; }
            printf("[T] <<< END iso ok\n");
            double m = 0, sd = 0; statOf(out, m, sd);
            double slope = idx > 0 ? (m - prevO) / (L - prevL) : 0.0;
            printf("[T]   %9.1f %11.2f %+10.2f %10.4f %9.2f %9.4f\n", L, m, m - L, idx > 0 ? slope : 0.0, sd, sd / SG);
            prevL = L; prevO = m; ++idx;
        }
    }

    // ---------- ESF: 阶跃扩散函数 ----------
    printf("[T] === ESF) 阶跃 10000|40000 (512x512, sigma=%.0f)：窗口半径与过冲 ===\n", SG);
    for (auto& c : cfgs) {
        std::vector<uint16_t> d((size_t)NN * NN);
        LCG r;
        for (int y = 0; y < NN; ++y) for (int x = 0; x < NN; ++x)
            d[(size_t)y * NN + x] = (uint16_t)std::clamp((x < NN / 2 ? 10000.0 : 40000.0) + gauss(r) * SG, 0.0, 65535.0);
        printf("[T] >>> RUN esf_%d_%d_%d_%d\n", c[0], c[1], c[2], c[3]);
        if (!feedRun(d, NN, NN, c[0], c[1], c[2], c[3], out)) { printf("[T] <<< END esf FAIL\n"); continue; }
        printf("[T] <<< END esf ok\n");
        printf("[T] --- cfg bal=%d det=%d enh=%d den=%d : 行均值剖面(x) ---\n", c[0], c[1], c[2], c[3]);
        for (int x = 0; x < NN; x += 4) {
            double si = 0, so = 0; int n = 0;
            for (int y = 128; y < 384; ++y) { si += d[(size_t)y * NN + x]; so += out[(size_t)y * NN + x]; ++n; }
            printf("[T]   x=%3d (d=%+4d)  in=%8.1f  out=%8.1f  delta=%+9.1f\n", x, x - NN / 2, si / n, so / n, so / n - si / n);
        }
    }

    // ---------- 全局耦合范围 ----------
    printf("[T] === GLB) 单点脉冲在 512x512 噪声图中的影响半径（验证是否全图耦合） ===\n");
    {
        const int MM = 512, cx = MM / 2, cy = MM / 2;
        std::vector<uint16_t> base((size_t)MM * MM); LCG r;
        for (auto& v : base) v = (uint16_t)std::clamp(30000 + gauss(r) * 1500.0, 0.0, 65535.0);
        std::vector<uint16_t> imp = base;
        imp[(size_t)cy * MM + cx] = (uint16_t)std::min(65535.0, (double)imp[(size_t)cy * MM + cx] + 8000.0);
        std::vector<uint16_t> ob, oi;
        printf("[T] >>> RUN glb_base\n"); feedRun(base, MM, MM, 0, 0, 0, 0, ob); printf("[T] <<< END glb_base\n");
        printf("[T] >>> RUN glb_imp\n");  feedRun(imp,  MM, MM, 0, 0, 0, 0, oi);  printf("[T] <<< END glb_imp\n");
        if (!ob.empty() && ob.size() == oi.size()) {
            for (int rad : {0,1,2,3,5,8,12,20,32,50,80,120,180,240}) {
                double s = 0, mx = 0; long n = 0; int nz = 0;
                for (int y = std::max(0, cy - rad); y <= std::min(MM - 1, cy + rad); ++y)
                    for (int x = std::max(0, cx - rad); x <= std::min(MM - 1, cx + rad); ++x) {
                        if (std::max(abs(x - cx), abs(y - cy)) != rad) continue;
                        double dv = fabs((double)oi[(size_t)y * MM + x] - ob[(size_t)y * MM + x]);
                        s += dv; if (dv > mx) mx = dv; if (dv != 0) ++nz; ++n;
                    }
                if (n) printf("[T]   r=%3d n=%4ld mean|diff|=%8.3f max=%8.1f 非零点=%ld\n", rad, n, s / n, mx, nz);
            }
            printf("[T]   中心 diff=%d (输入脉冲 +8000)\n", (int)oi[(size_t)cy * MM + cx] - (int)ob[(size_t)cy * MM + cx]);
        }
    }
    return 0;
}
