// probe7: 偏置机制判别(分布形状) + 暗部地板 + 正确 MTF + noise0 估计器公式
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
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
// 噪声分布：0=高斯, 1=均匀, 2=双峰(±s), 3=拉普拉斯
static double noiseOf(int kind, double s, LCG& r)
{
    switch (kind) {
    case 1: return (r.next() * 2 - 1) * s * sqrt(3.0);
    case 2: return (r.next() < 0.5 ? -s : s);
    case 3: { double u = r.next() - 0.5; return -s * sqrt(0.5) * (u < 0 ? log(1 + 2 * u) : log(1 - 2 * u)) * 0.92; }
    default: return gauss(r) * s;
    }
}

int main()
{
    InitiImgPro(false);
    std::vector<uint16_t> out;
    const int NN = 512;

    // ---------- R1: 偏置是否只依赖 sigma（还是依赖 E|x|） ----------
    printf("[T] === R1) L=30000，不同噪声分布（同 sigma）→ 偏置判别 ===\n");
    printf("[T]   kind      sigma   in_mean   in_sd   in_E|x|   out_mean   offset  offset/sigma  offset/E|x|  sd_gain\n");
    const char* kn[] = { "gauss", "uniform", "bimodal", "laplace" };
    for (int kind = 0; kind < 4; ++kind)
        for (double sg : { 300.0, 1000.0, 3000.0 }) {
            std::vector<uint16_t> d((size_t)NN * NN); LCG r;
            for (auto& v : d) v = (uint16_t)std::clamp(30000 + noiseOf(kind, sg, r), 0.0, 65535.0);
            double im = 0, isd = 0; ms(d, im, isd);
            double ea = 0; for (auto x : d) ea += fabs((double)x - im); ea /= d.size();
            printf("[T] >>> RUN r1_%d_%.0f\n", kind, sg);
            if (!feedRun(d, NN, NN, 0, 0, 0, 0, out)) { printf("[T] <<< END r1 FAIL\n"); continue; }
            printf("[T] <<< END r1 ok\n");
            double om = 0, osd = 0; ms(out, om, osd);
            printf("[T]   %-8s %8.0f %9.2f %7.2f %9.2f %10.2f %+9.2f %10.4f %10.4f %9.4f\n",
                kn[kind], sg, im, isd, ea, om, om - im, (om - im) / sg, (om - im) / ea, osd / isd);
        }

    // ---------- R2: 暗部地板 ----------
    printf("[T] === R2) 暗部密集阶梯（32 级，100..6000）+ 输出极值 ===\n");
    {
        const int WW = 512, B = 32, BH = 64, HH = B * BH;
        std::vector<uint16_t> d((size_t)WW * HH); LCG r;
        for (int b = 0; b < B; ++b) {
            double L = 100.0 + b * (6000.0 - 100.0) / (B - 1);
            for (int y = b * BH; y < (b + 1) * BH; ++y) for (int x = 0; x < WW; ++x)
                d[(size_t)y * WW + x] = (uint16_t)std::clamp(L + gauss(r) * 30.0, 0.0, 65535.0);
        }
        printf("[T] >>> RUN r2\n");
        if (feedRun(d, WW, HH, 0, 0, 0, 0, out)) {
            printf("[T] <<< END r2 ok\n");
            uint32_t omn = 65535, omx = 0; for (auto v : out) { if (v < omn) omn = v; if (v > omx) omx = v; }
            printf("[T]   输出全图 min=%u max=%u\n", omn, omx);
            for (int b = 0; b < B; ++b) {
                double si = 0, so = 0; long n = 0; uint32_t lo = 65535, hi = 0;
                for (int y = b * BH + 24; y < b * BH + 40; ++y) for (int x = 64; x < 448; ++x) {
                    double vi = d[(size_t)y * WW + x], vo = out[(size_t)y * WW + x];
                    si += vi; so += vo; ++n; if (vo < lo) lo = (uint32_t)vo; if (vo > hi) hi = (uint32_t)vo;
                }
                printf("[T]   b=%2d in=%8.1f out=%9.2f delta=%+9.2f  outROI[min=%u max=%u]\n", b, si / n, so / n, (so - si) / n, lo, hi);
            }
        }
        else printf("[T] <<< END r2 FAIL\n");
    }

    // ---------- R3: 正确 MTF ----------
    printf("[T] === R3) MTF：512x512 正弦光栅，分析窗 [128,384)^2 含整数周期 ===\n");
    {
        const double A = 4000, M = 30000;
        auto measure = [&](const std::vector<uint16_t>& d, double f, double& amp, double& mean) {
            double sr = 0, sc = 0, sm = 0; long n = 0;
            for (int y = 128; y < 384; ++y) for (int x = 128; x < 384; ++x) {
                double v = d[(size_t)y * NN + x];
                double th = 2 * 3.14159265358979323846 * f * x;
                sr += v * sin(th); sc += v * cos(th); sm += v; ++n;
            }
            amp = 2.0 * sqrt(sr * sr + sc * sc) / n; mean = sm / n;
        };
        std::vector<std::array<int,4>> cfgs = { {0,0,0,0}, {10,0,0,0}, {0,10,0,0}, {0,0,10,0}, {0,0,0,20}, {5,7,5,3} };
        int ks[] = { 2,4,8,16,32,48,64,96,128,192,256 };
        for (auto& c : cfgs) {
            printf("[T] --- cfg bal=%d det=%d enh=%d den=%d ---\n", c[0], c[1], c[2], c[3]);
            printf("[T]     period   amp_in   amp_out    GAIN    mean_in  mean_out  d_mean\n");
            for (int k : ks) {
                double f = (double)k / NN;
                std::vector<uint16_t> d((size_t)NN * NN); LCG r;
                for (int y = 0; y < NN; ++y) for (int x = 0; x < NN; ++x)
                    d[(size_t)y * NN + x] = (uint16_t)std::clamp(M + A * sin(2 * 3.14159265358979323846 * f * x) + gauss(r) * 30.0, 0.0, 65535.0);
                printf("[T] >>> RUN r3_%d_%d_%d_%d_k%d\n", c[0], c[1], c[2], c[3], k);
                if (!feedRun(d, NN, NN, c[0], c[1], c[2], c[3], out)) { printf("[T] <<< END r3 FAIL\n"); continue; }
                printf("[T] <<< END r3 ok\n");
                double ai = 0, mi = 0, ao = 0, mo = 0;
                measure(d, f, ai, mi); measure(out, f, ao, mo);
                printf("[T]   %8.2f %8.1f %9.1f %8.5f %9.1f %9.1f %+8.1f\n", NN / (double)k, ai, ao, ai > 1 ? ao / ai : 0, mi, mo, mo - mi);
            }
        }
    }
    return 0;
}
