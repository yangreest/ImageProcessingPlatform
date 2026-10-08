// probe10: 逐点抽取 f(v)，弄清 lo/hi 归一化区间如何确定 + 参数如何改变曲线
//  U1) 同一直方图(高斯 sigma=30@30000) 在 有/无离群点 下的 f(v) 逐点对比
//  U2) 均匀斜坡 LUT 在 6 组参数下的归一化曲线 u(t)（自相似坐标）
//  U3) balanced 在偏斜直方图上的作用
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

// 逐点映射表：按输入值分桶统计输出均值
static void dumpMap(const char* tag, const std::vector<uint16_t>& d, const std::vector<uint16_t>& o, int step)
{
    std::vector<double> s(65536, 0); std::vector<uint32_t> c(65536, 0);
    uint32_t lo = 65535, hi = 0;
    for (size_t i = 0; i < d.size(); ++i) { s[d[i]] += o[i]; ++c[d[i]]; if (d[i] < lo) lo = d[i]; if (d[i] > hi) hi = d[i]; }
    printf("[T]   --- %s  in[min=%u max=%u] ---\n", tag, lo, hi);
    printf("[T]       v      n     f(v)     delta\n");
    for (uint32_t v = lo; v <= hi; v += step) {
        double sn = 0; long cn = 0;
        for (uint32_t k = v; k < v + (uint32_t)step && k <= hi; ++k) { sn += s[k]; cn += c[k]; }
        if (cn > 0) {
            double vm = 0, cc = 0; for (uint32_t k = v; k < v + (uint32_t)step && k <= hi; ++k) { vm += k * c[k]; cc += c[k]; }
            vm /= cc; double fm = sn / cn;
            printf("[T]   %7u %6ld %9.2f %+9.2f\n", v, cn, fm, fm - vm);
        }
    }
}

int main()
{
    InitiImgPro(false);
    std::vector<uint16_t> out;
    const int NN = 512; const size_t NP = (size_t)NN * NN;

    // ================= U1 =================
    printf("[T] === U1) 高斯 sigma=30 @30000，加不同数量离群点 -> 逐点 f(v) ===\n");
    std::vector<uint16_t> base(NP); LCG r0;
    for (auto& v : base) v = (uint16_t)std::clamp(30000 + gauss(r0) * 30.0, 0.0, 65535.0);
    {
        uint32_t lo = 65535, hi = 0; for (auto v : base) { if (v < lo) lo = v; if (v > hi) hi = v; }
        printf("[T]   背景 min=%u max=%u width=%u\n", lo, hi, hi - lo);
    }
    struct Case { const char* name; int V; int n; };
    std::vector<Case> cases = { {"pure", 0, 0}, {"V65000_n1", 65000, 1}, {"V65000_n26", 65000, 26},
                                {"V65000_n262", 65000, 262}, {"V65000_n2621", 65000, 2621},
                                {"V38000_n1", 38000, 1}, {"V31000_n1", 31000, 1},
                                {"V500_n1", 500, 1}, {"V500_n262", 500, 262} };
    for (auto& cs : cases) {
        std::vector<uint16_t> d = base;
        for (int i = 0; i < cs.n; ++i) d[(size_t)i * 97 % NP] = (uint16_t)cs.V;
        printf("[T] >>> RUN u1_%s\n", cs.name);
        if (!feedRun(d, NN, NN, 0, 0, 0, 0, out)) { printf("[T] <<< END FAIL\n"); continue; }
        printf("[T] <<< END ok\n");
        uint32_t omx = 0, omn = 65535; for (auto v : out) { if (v > omx) omx = v; if (v < omn) omn = v; }
        printf("[T]   out[min=%u max=%u] span=%u\n", omn, omx, omx - omn);
        dumpMap(cs.name, d, out, 20);
    }

    // ================= U2) 参数对归一化曲线的影响 =================
    printf("[T] === U2) 均匀斜坡 [8000,56000] 256x256，参数扫描 -> u(t)=(f-lo)/w ===\n");
    {
        const int R = 256; const int LO = 8000, HI = 56000; const double W = HI - LO;
        std::vector<uint16_t> d((size_t)R * R);
        for (int i = 0; i < R * R; ++i) d[i] = (uint16_t)(LO + (double)i * W / (R * R - 1));
        std::vector<std::array<int,4>> cfgs = {
            {0,0,0,0}, {5,0,0,0}, {10,0,0,0}, {20,0,0,0},
            {0,5,0,0}, {0,10,0,0}, {0,20,0,0},
            {0,0,5,0}, {0,0,10,0}, {0,0,20,0},
            {0,0,0,5}, {0,0,0,10}, {0,0,0,20} };
        // 采样 t 位置
        std::vector<double> ts = { 0.0,0.02,0.05,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,0.95,0.98,1.0 };
        printf("[T]   cfg(bal,det,enh,den)  ");
        for (double t : ts) printf("t=%.2f  ", t);
        printf("\n");
        for (auto& c : cfgs) {
            printf("[T] >>> RUN u2_%d_%d_%d_%d\n", c[0], c[1], c[2], c[3]);
            if (!feedRun(d, R, R, c[0], c[1], c[2], c[3], out)) { printf("[T] <<< END FAIL\n"); continue; }
            printf("[T] <<< END ok\n");
            printf("[T]   (%2d,%2d,%2d,%2d)  ", c[0], c[1], c[2], c[3]);
            for (double t : ts) {
                int idx = (int)(t * (R * R - 1)); double v = d[idx];
                printf("%7.4f ", (out[idx] - LO) / W);
            }
            printf("\n");
        }
        printf("[T]   identity      ");
        for (double t : ts) printf("%7.4f ", t);
        printf("\n");
    }

    // ================= U3) balanced 在偏斜直方图 =================
    printf("[T] === U3) 偏斜直方图（幂律/双峰群），扫描 balanced ===\n");
    {
        const int R = 512; const size_t NPP = (size_t)R * R;
        for (int shape = 0; shape < 3; ++shape) {
            std::vector<uint16_t> d(NPP); LCG r;
            for (size_t i = 0; i < NPP; ++i) {
                double u = r.next(), v;
                switch (shape) {
                case 0: v = 60000 * pow(u, 3.0); break;                  // 重暗部
                case 1: v = 5000 + 55000 * pow(u, 0.3333); break;        // 重亮部
                default: {                                                 // 双群体
                    bool a = (u < 0.7); double uu = a ? u / 0.7 : (u - 0.7) / 0.3;
                    v = a ? 8000 + 6000 * uu : 40000 + 12000 * uu; break; }
                }
                d[i] = (uint16_t)std::clamp(v + gauss(r) * 60.0, 0.0, 65535.0);
            }
            double im = 0, s = 0, s2 = 0; for (auto x : d) { s += x; s2 += (double)x * x; } im = s / NPP;
            double isd = sqrt(s2 / NPP - im * im);
            const char* sn[] = { "重暗部(x^3)", "重亮部(x^1/3)", "双群体7:3" };
            for (int bal : { 0, 5, 10, 20, 40 }) {
                printf("[T] >>> RUN u3_s%d_b%d\n", shape, bal);
                if (!feedRun(d, R, R, bal, 0, 0, 0, out)) { printf("[T] <<< END FAIL\n"); continue; }
                printf("[T] <<< END ok\n");
                double om = 0, os = 0, os2 = 0; for (auto x : out) { os += x; os2 += (double)x * x; } om = os / NPP;
                double osd = sqrt(os2 / NPP - om * om);
                // 输出百分位
                std::vector<uint16_t> cp = out; std::nth_element(cp.begin(), cp.begin() + NPP / 100, cp.end());
                double p1 = cp[NPP / 100];
                std::nth_element(cp.begin(), cp.begin() + NPP / 2, cp.end()); double p50 = cp[NPP / 2];
                std::nth_element(cp.begin(), cp.begin() + NPP * 99 / 100, cp.end()); double p99 = cp[NPP * 99 / 100];
                printf("[T]   %-14s bal=%2d in_mean=%.1f in_sd=%.1f | out_mean=%.1f out_sd=%.1f sd_gain=%.4f off=%+.1f | p1=%.0f p50=%.0f p99=%.0f\n",
                    sn[shape], bal, im, isd, om, osd, osd / isd, om - im, p1, p50, p99);
            }
        }
    }
    return 0;
}
