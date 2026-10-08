// probe8: 决定性实验
//  S0) 像素随机置换不变性 -> 是否严格"逐点 LUT + 全局直方图驱动"
//  S1) 棋盘/脉冲/细线 -> 是否存在空间滤波
//  S2) 量化级数 K 扫描 -> 双峰透传的真正原因
//  S3) 直方图宽度扫描 -> tone curve 的 slope/offset 如何随直方图变化
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
    const int NN = 512;
    const size_t NP = (size_t)NN * NN;

    // ================= S0) 置换不变性 =================
    printf("[T] === S0) 像素随机置换：判定是否严格逐点 LUT ===\n");
    {
        // 构造一张既有结构又有噪声、直方图偏斜的图
        std::vector<uint16_t> d(NP); LCG r;
        for (int y = 0; y < NN; ++y) for (int x = 0; x < NN; ++x) {
            double base = 8000 + 22000 * exp(-((x - 256.0) * (x - 256.0) + (y - 256.0) * (y - 256.0)) / (2 * 90.0 * 90.0));
            base += 6000 * sin(2 * 3.14159265358979323846 * x / 37.0);
            d[(size_t)y * NN + x] = (uint16_t)std::clamp(base + gauss(r) * 400.0, 0.0, 65535.0);
        }
        std::vector<size_t> perm(NP); std::iota(perm.begin(), perm.end(), 0u);
        for (size_t i = NP - 1; i > 0; --i) { size_t j = (size_t)(r.next() * (i + 1)); if (j > i) j = i; std::swap(perm[i], perm[j]); }
        std::vector<uint16_t> d2(NP);
        for (size_t i = 0; i < NP; ++i) d2[i] = d[perm[i]];

        printf("[T] >>> RUN s0_orig\n");
        bool ok1 = feedRun(d, NN, NN, 0, 0, 0, 0, out);
        printf("[T] <<< END s0 ok=%d\n", (int)ok1);
        printf("[T] >>> RUN s0_perm\n");
        bool ok2 = feedRun(d2, NN, NN, 0, 0, 0, 0, out2);
        printf("[T] <<< END s0 ok=%d\n", (int)ok2);
        if (ok1 && ok2) {
            long exact = 0, nearN = 0, badN = 0; double se = 0; int mx = 0;
            for (size_t i = 0; i < NP; ++i) {
                int a = out[perm[i]], b = out2[i]; int df = a - b; if (df < 0) df = -df;
                if (df == 0) ++exact; else if (df <= 2) ++nearN; else ++badN;
                se += (double)df * df; if (df > mx) mx = df;
            }
            printf("[T]   逐点一致: exact=%ld (%.3f%%)  |diff|<=2:%ld  |diff|>2:%ld  rms=%.3f max=%d\n",
                exact, 100.0 * exact / NP, nearN, badN, sqrt(se / NP), mx);
            // 同时给出直方图是否被保持
            std::vector<uint32_t> h1(65536, 0), h2(65536, 0);
            for (auto v : out) ++h1[v]; for (auto v : out2) ++h2[v];
            long hdiff = 0; for (int i = 0; i < 65536; ++i) hdiff += labs((long)h1[i] - (long)h2[i]);
            printf("[T]   输出直方图 L1 差异 = %ld (0 表示完全相同的灰度集合)\n", hdiff);
        }
    }

    // ================= S1) 空间滤波存在性 =================
    printf("[T] === S1) 棋盘/细线/脉冲：是否存在空间滤波 ===\n");
    {
        // (a) 棋盘 (Nyquist 频率)
        for (int A : { 1000, 4000, 12000 }) {
            std::vector<uint16_t> d(NP);
            for (int y = 0; y < NN; ++y) for (int x = 0; x < NN; ++x)
                d[(size_t)y * NN + x] = (uint16_t)std::clamp(30000.0 + A * (((x + y) & 1) ? 1.0 : -1.0), 0.0, 65535.0);
            printf("[T] >>> RUN s1_check%d\n", A);
            if (!feedRun(d, NN, NN, 0, 0, 0, 0, out)) { printf("[T] <<< END FAIL\n"); continue; }
            printf("[T] <<< END ok\n");
            // 用与棋盘同相的相关求幅度
            double sc = 0, sm = 0; long n = 0;
            for (int y = 128; y < 384; ++y) for (int x = 128; x < 384; ++x) {
                double v = out[(size_t)y * NN + x]; sc += v * (((x + y) & 1) ? 1.0 : -1.0); sm += v; ++n;
            }
            printf("[T]   checker A=%5d  amp_in=%d  amp_out=%.1f  GAIN=%.5f  mean_in=30000 mean_out=%.1f d_mean=%+.1f\n",
                A, A, sc / n, sc / n / A, sm / n, sm / n - 30000);
        }
        // (b) 脉冲响应：低噪背景 + 单点 +8000
        {
            std::vector<uint16_t> d(NP); LCG r;
            for (auto& v : d) v = (uint16_t)std::clamp(30000 + gauss(r) * 30.0, 0.0, 65535.0);
            const int cy = 256, cx = 256;
            d[(size_t)cy * NN + cx] += 8000;
            printf("[T] >>> RUN s1_impulse\n");
            if (feedRun(d, NN, NN, 0, 0, 0, 0, out)) {
                printf("[T] <<< END ok\n");
                // 背景基准（远离脉冲）
                double bg = 0; long nb = 0;
                for (int y = 380; y < 420; ++y) for (int x = 380; x < 420; ++x) { bg += out[(size_t)y * NN + x]; ++nb; }
                bg /= nb;
                printf("[T]   背景输出均值=%.2f (输入 30000)\n", bg);
                printf("[T]   dy dx   in      out     out-bg\n");
                for (int dy = -3; dy <= 3; ++dy) for (int dx = -3; dx <= 3; ++dx) {
                    double vi = d[(size_t)(cy + dy) * NN + cx + dx], vo = out[(size_t)(cy + dy) * NN + cx + dx];
                    printf("[T]   %2d %2d %7.0f %8.1f %+9.1f\n", dy, dx, vi, vo, vo - bg);
                }
            }
            else printf("[T] <<< END FAIL\n");
        }
        // (c) 1px 竖线
        {
            std::vector<uint16_t> d(NP); LCG r;
            for (auto& v : d) v = (uint16_t)std::clamp(30000 + gauss(r) * 30.0, 0.0, 65535.0);
            for (int y = 0; y < NN; ++y) d[(size_t)y * NN + 256] = 38000;
            printf("[T] >>> RUN s1_line\n");
            if (feedRun(d, NN, NN, 0, 0, 0, 0, out)) {
                printf("[T] <<< END ok\n");
                double col[7] = { 0 }, cnt = 0;
                for (int y = 64; y < 448; ++y) for (int k = -3; k <= 3; ++k) { col[k + 3] += out[(size_t)y * NN + 256 + k]; }
                cnt = 384;
                printf("[T]   1px竖线(输入38000, 背景30000) 输出列均值 dx=-3..3: ");
                for (int k = 0; k < 7; ++k) printf("%.1f ", col[k] / cnt);
                printf("\n");
            }
            else printf("[T] <<< END FAIL\n");
        }
    }

    // ================= S2) 量化级数 K =================
    printf("[T] === S2) 量化级数 K 扫描 (sigma=300, L=30000) ===\n");
    printf("[T]     K   levels  in_mean   in_sd   out_mean   offset  offset/sd  sd_gain  max|diff|  nDistinctOut\n");
    for (int K : { 2, 3, 4, 5, 8, 16, 32, 64, 256, 4096 }) {
        std::vector<uint16_t> d(NP); LCG r;
        for (auto& v : d) {
            double g = gauss(r) * 300.0;
            // 量化到 K 级，级距 = 6*300/(K-1)（覆盖 ±3sigma）
            double step = (K > 1) ? (6.0 * 300.0 / (K - 1)) : 1.0;
            double q = (K > 1) ? round(g / step) * step : g;
            v = (uint16_t)std::clamp(30000 + q, 0.0, 65535.0);
        }
        double im = 0, isd = 0; ms(d, im, isd);
        std::vector<uint32_t> hi(65536, 0); for (auto v : d) ++hi[v];
        int nd = 0; for (int i = 0; i < 65536; ++i) if (hi[i]) ++nd;
        printf("[T] >>> RUN s2_K%d\n", K);
        if (!feedRun(d, NN, NN, 0, 0, 0, 0, out)) { printf("[T] <<< END FAIL\n"); continue; }
        printf("[T] <<< END ok\n");
        double om = 0, osd = 0; ms(out, om, osd);
        int mxd = 0; std::vector<uint32_t> ho(65536, 0);
        for (size_t i = 0; i < NP; ++i) { int df = (int)out[i] - (int)d[i]; if (df < 0) df = -df; if (df > mxd) mxd = df; ++ho[out[i]]; }
        int ndo = 0; for (int i = 0; i < 65536; ++i) if (ho[i]) ++ndo;
        printf("[T]   %5d %7d %9.2f %7.2f %10.2f %+9.2f %10.4f %8.4f %9d %12d\n",
            K, nd, im, isd, om, om - im, (om - im) / isd, osd / isd, mxd, ndo);
    }

    // ================= S2b) 双峰非对称 =================
    printf("[T] === S2b) 双峰非对称 p/(1-p)，gap=600, L=30000 ===\n");
    for (double p : { 0.5, 0.6, 0.7, 0.8, 0.9, 0.95 }) {
        std::vector<uint16_t> d(NP); LCG r;
        for (auto& v : d) { double g = (r.next() < p) ? -300.0 : 300.0; v = (uint16_t)std::clamp(30000 + g, 0.0, 65535.0); }
        double im = 0, isd = 0; ms(d, im, isd);
        printf("[T] >>> RUN s2b_p%.2f\n", p);
        if (!feedRun(d, NN, NN, 0, 0, 0, 0, out)) { printf("[T] <<< END FAIL\n"); continue; }
        printf("[T] <<< END ok\n");
        double om = 0, osd = 0; ms(out, om, osd);
        int mxd = 0; for (size_t i = 0; i < NP; ++i) { int df = (int)out[i] - (int)d[i]; if (df < 0) df = -df; if (df > mxd) mxd = df; }
        printf("[T]   p=%.2f in_mean=%.2f in_sd=%.2f out_mean=%.2f offset=%+.2f sd_gain=%.4f max|diff|=%d\n",
            p, im, isd, om, om - im, osd / isd, mxd);
    }

    // ================= S3) 直方图宽度扫描 =================
    printf("[T] === S3) 高斯直方图 L=30000，扫描 sigma_hist -> slope/offset ===\n");
    printf("[T]   sigma_hist  in_mean   in_sd   out_mean   offset  offset/sd   sd_gain(=slope)\n");
    for (double s : { 30.0, 100.0, 300.0, 1000.0, 3000.0, 6000.0, 10000.0, 16000.0 }) {
        std::vector<uint16_t> d(NP); LCG r;
        for (auto& v : d) v = (uint16_t)std::clamp(30000 + gauss(r) * s, 0.0, 65535.0);
        double im = 0, isd = 0; ms(d, im, isd);
        printf("[T] >>> RUN s3_%.0f\n", s);
        if (!feedRun(d, NN, NN, 0, 0, 0, 0, out)) { printf("[T] <<< END FAIL\n"); continue; }
        printf("[T] <<< END ok\n");
        double om = 0, osd = 0; ms(out, om, osd);
        printf("[T]   %10.0f %9.2f %7.2f %10.2f %+9.2f %10.4f %10.4f\n", s, im, isd, om, om - im, (om - im) / isd, osd / isd);
    }
    return 0;
}
