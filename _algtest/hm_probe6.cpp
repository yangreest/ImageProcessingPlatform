// probe6: 判别色调映射的来源 —— 噪声偏置 / 直方图形状 / 空间排布 / 图像尺寸
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
    setRawData((BYTE*)d.data(), w, h);
    setParam(bal, det, enh, den);
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
// 在矩形区域取输出均值
static double roiMean(const std::vector<uint16_t>& o, int w, int x0, int y0, int ww, int hh)
{
    double s = 0; long n = 0;
    for (int y = y0; y < y0 + hh; ++y) for (int x = x0; x < x0 + ww; ++x) { s += o[(size_t)y * w + x]; ++n; }
    return n ? s / n : 0;
}

int main()
{
    InitiImgPro(false);
    std::vector<uint16_t> out;

    // ---------- Q1: ISO 偏置是否随噪声幅度变化 ----------
    printf("[T] === Q1) 单灰阶 L=30000，扫描噪声 sigma：偏置来源 ===\n");
    printf("[T]     sigma    in_mean    out_mean    offset    out_sd    sd_gain\n");
    for (double sg : { 1.0, 10.0, 30.0, 100.0, 300.0, 1000.0, 3000.0, 8000.0 }) {
        const int NN = 512;
        std::vector<uint16_t> d((size_t)NN * NN); LCG r;
        for (auto& v : d) v = (uint16_t)std::clamp(30000 + gauss(r) * sg, 0.0, 65535.0);
        double im = 0, isd = 0; ms(d, im, isd);
        printf("[T] >>> RUN q1_%.0f\n", sg);
        if (!feedRun(d, NN, NN, 0, 0, 0, 0, out)) { printf("[T] <<< END q1 FAIL\n"); continue; }
        printf("[T] <<< END q1 ok\n");
        double om = 0, osd = 0; ms(out, om, osd);
        printf("[T]   %8.1f %10.2f %11.2f %+9.2f %9.2f %9.4f\n", sg, im, om, om - im, osd, isd > 0 ? osd / isd : 0);
    }

    // ---------- Q2: 空间排布是否影响映射（同一直方图，不同排布） ----------
    printf("[T] === Q2) 同一直方图(50%%@10000, 50%%@40000)不同空间排布 ===\n");
    {
        const int NN = 512;
        auto mk = [&](int mode) {
            std::vector<uint16_t> d((size_t)NN * NN); LCG r;
            for (int y = 0; y < NN; ++y) for (int x = 0; x < NN; ++x) {
                bool dark;
                switch (mode) {
                case 0: dark = (x < NN / 2); break;                       // 左右两半
                case 1: dark = ((x / 16) % 2) == 0; break;                 // 16px 竖条纹
                case 2: dark = (((x / 8) + (y / 8)) % 2) == 0; break;      // 8px 棋盘
                case 3: dark = (r.next() < 0.5); break;                    // 逐像素随机
                case 4: dark = (y < NN / 2); break;                        // 上下两半
                default: dark = true;
                }
                double L = dark ? 10000.0 : 40000.0;
                d[(size_t)y * NN + x] = (uint16_t)std::clamp(L + gauss(r) * 100.0, 0.0, 65535.0);
            }
            return d;
        };
        const char* names[] = { "左右两半", "16px竖条纹", "8px棋盘", "逐像素随机", "上下两半" };
        for (int m = 0; m < 5; ++m) {
            auto d = mk(m);
            printf("[T] >>> RUN q2_%d\n", m);
            if (!feedRun(d, NN, NN, 0, 0, 0, 0, out)) { printf("[T] <<< END q2 FAIL\n"); continue; }
            printf("[T] <<< END q2 ok\n");
            double om = 0, osd = 0; ms(out, om, osd);
            // 统计输出直方图双峰位置（近似 f(10000), f(40000)）
            std::vector<uint32_t> hist(65536, 0);
            for (auto v : out) ++hist[v];
            // 找两个峰：分别在 [0,25000) 与 [25000,65536) 内取众数邻域均值
            auto peak = [&](int lo, int hi) {
                int best = lo; uint32_t bc = 0;
                for (int i = lo; i < hi; ++i) if (hist[i] > bc) { bc = hist[i]; best = i; }
                double s = 0; long n = 0;
                for (int i = std::max(lo, best - 300); i <= std::min(hi - 1, best + 300); ++i) { s += (double)i * hist[i]; n += hist[i]; }
                return n ? s / n : (double)best;
            };
            double p1 = peak(0, 25000), p2 = peak(25000, 65536);
            printf("[T]   %-14s out_mean=%.1f out_sd=%.1f  f(10000)=%.1f (delta%+.1f)  f(40000)=%.1f (delta%+.1f)\n",
                names[m], om, osd, p1, p1 - 10000, p2, p2 - 40000);
        }
    }

    // ---------- Q3: 图像尺寸是否影响映射 ----------
    printf("[T] === Q3) 单灰阶 L=30000 sigma=300，不同图像尺寸 ===\n");
    for (int NN : { 64, 128, 256, 512, 1024, 2048 }) {
        std::vector<uint16_t> d((size_t)NN * NN); LCG r;
        for (auto& v : d) v = (uint16_t)std::clamp(30000 + gauss(r) * 300.0, 0.0, 65535.0);
        double im = 0, isd = 0; ms(d, im, isd);
        printf("[T] >>> RUN q3_%d\n", NN);
        if (!feedRun(d, NN, NN, 0, 0, 0, 0, out)) { printf("[T] <<< END q3 FAIL\n"); continue; }
        printf("[T] <<< END q3 ok\n");
        double om = 0, osd = 0; ms(out, om, osd);
        printf("[T]   %5dx%-5d in_mean=%.2f out_mean=%.2f offset=%+.2f sd_gain=%.4f\n", NN, NN, im, om, om - im, osd / isd);
    }

    // ---------- Q4: 密集阶梯（64 级等面积）→ 高分辨率色调曲线 ----------
    printf("[T] === Q4) 64 级等面积阶梯 (512x2048)：密集 f(v) ===\n");
    {
        const int WW = 512, HH = 2048, B = 64, BH = HH / B;
        std::vector<uint16_t> d((size_t)WW * HH); LCG r;
        std::vector<double> lev(B);
        for (int b = 0; b < B; ++b) {
            double L = 800.0 + b * (64000.0 - 800.0) / (B - 1); lev[b] = L;
            for (int y = b * BH; y < (b + 1) * BH; ++y) for (int x = 0; x < WW; ++x)
                d[(size_t)y * WW + x] = (uint16_t)std::clamp(L + gauss(r) * 60.0, 0.0, 65535.0);
        }
        for (auto& c : std::vector<std::array<int,4>>{ {0,0,0,0}, {0,0,10,0}, {0,0,20,0}, {10,0,0,0}, {20,0,0,0} }) {
            printf("[T] >>> RUN q4_%d_%d_%d_%d\n", c[0], c[1], c[2], c[3]);
            if (!feedRun(d, WW, HH, c[0], c[1], c[2], c[3], out)) { printf("[T] <<< END q4 FAIL\n"); continue; }
            printf("[T] <<< END q4 ok\n");
            printf("[T] --- cfg bal=%d det=%d enh=%d den=%d ---\n", c[0], c[1], c[2], c[3]);
            for (int b = 0; b < B; ++b) {
                double o = roiMean(out, WW, 64, b * BH + 8, WW - 128, BH - 16);
                double i = roiMean(d, WW, 64, b * BH + 8, WW - 128, BH - 16);
                printf("[T]   b=%2d in=%8.1f out=%9.2f delta=%+9.2f ratio=%.4f cdf=%.4f\n",
                    b, i, o, o - i, o / i, (b + 0.5) / B);
            }
        }
    }

    // ---------- Q5: 直方图形状（同样 16 级，面积均匀 vs 递减） ----------
    printf("[T] === Q5) 直方图形状对 f(v) 的影响（16 级） ===\n");
    {
        const int WW = 512;
        for (int shape = 0; shape < 2; ++shape) {
            // shape 0: 等面积; shape 1: 面积随级别递减
            std::vector<int> rows(16);
            int tot = 0;
            for (int b = 0; b < 16; ++b) { rows[b] = shape == 0 ? 1 : (17 - b); tot += rows[b]; }
            int unit = 96;                       // 每份行数
            int HH = tot * unit;
            std::vector<uint16_t> d((size_t)WW * HH); LCG r;
            std::vector<double> lev(16), y0(16), y1(16);
            int y = 0;
            for (int b = 0; b < 16; ++b) {
                double L = 800.0 + b * (64000.0 - 800.0) / 15.0; lev[b] = L;
                y0[b] = y; y1[b] = y + rows[b] * unit;
                for (int yy = y0[b]; yy < y1[b]; ++yy) for (int x = 0; x < WW; ++x)
                    d[(size_t)yy * WW + x] = (uint16_t)std::clamp(L + gauss(r) * 60.0, 0.0, 65535.0);
                y = y1[b];
            }
            printf("[T] >>> RUN q5_shape%d\n", shape);
            if (!feedRun(d, WW, HH, 0, 0, 0, 0, out)) { printf("[T] <<< END q5 FAIL\n"); continue; }
            printf("[T] <<< END q5 ok\n");
            // 计算各级面积占比（累积 CDF）
            double cdf = 0;
            printf("[T] --- shape=%d (%s) HH=%d ---\n", shape, shape == 0 ? "等面积" : "面积递减", HH);
            for (int b = 0; b < 16; ++b) {
                cdf += (double)rows[b] / tot;
                int cy = (int)((y0[b] + y1[b]) / 2) - 8;
                double o = roiMean(out, WW, 64, cy, WW - 128, 16);
                double i = roiMean(d, WW, 64, cy, WW - 128, 16);
                printf("[T]   b=%2d in=%8.1f out=%9.2f delta=%+9.2f area_frac=%.4f cdf=%.4f\n",
                    b, i, o, o - i, (double)rows[b] / tot, cdf);
            }
        }
    }
    return 0;
}
