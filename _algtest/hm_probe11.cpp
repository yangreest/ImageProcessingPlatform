// probe11: V1) lo/hi 归一化区间的确定方式(百分位探针)  V2) 真实 raw 上的验证与出图
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <numeric>
#include <cstdlib>
#include <windows.h>
#include <opencv2/opencv.hpp>
#include "HmImgProApi.h"

struct LCG { uint64_t s = 88172645463325252ULL; double next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (double)(s >> 11) / 9007199254740992.0; } };

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

// 输入值 -> 输出均值 的分箱映射表
static void dumpBins(const char* tag, const std::vector<uint16_t>& d, const std::vector<uint16_t>& o, int bw, int maxrow)
{
    std::vector<double> s(65536 / bw + 1, 0); std::vector<uint32_t> c(65536 / bw + 1, 0);
    std::vector<double> si(65536 / bw + 1, 0);
    for (size_t i = 0; i < d.size(); ++i) { int b = d[i] / bw; s[b] += o[i]; si[b] += d[i]; ++c[b]; }
    printf("[T]   --- map %s (bin=%d) ---\n", tag, bw);
    int printed = 0;
    for (size_t b = 0; b < s.size() && printed < maxrow; ++b) {
        if (!c[b]) continue;
        printf("[T]   in=%9.2f n=%8u out=%9.2f delta=%+9.2f\n", si[b] / c[b], c[b], s[b] / c[b], s[b] / c[b] - si[b] / c[b]);
        ++printed;
    }
}

int main()
{
    InitiImgPro(false);
    std::vector<uint16_t> out;

    // ================= V1) 百分位探针 =================
    printf("[T] === V1) 斜坡[10000,50000] 512x512 + n 个 0/65535 离群点 ===\n");
    {
        const int R = 512; const size_t NP = (size_t)R * R;
        const int LO = 10000, HI = 50000;
        std::vector<uint16_t> ramp(NP);
        for (size_t i = 0; i < NP; ++i) ramp[i] = (uint16_t)(LO + (double)i * (HI - LO) / (NP - 1));
        // 基准（无离群点）
        printf("[T] >>> RUN v1_base\n");
        if (feedRun(ramp, R, R, 0, 0, 0, 0, out)) {
            printf("[T] <<< END ok\n");
            // 用值定位
            auto at = [&](int v) { size_t idx = (size_t)std::llround((double)(v - LO) * (NP - 1) / (HI - LO)); return out[std::min(idx, NP - 1)]; };
            printf("[T]   base: ");
            for (int v = LO; v <= HI; v += 2500) printf("f(%d)=%u ", v, (int)at(v));
            printf("\n");
        }
        else printf("[T] <<< END FAIL\n");

        for (int n : { 1, 8, 64, 512, 4096, 26214, 65536 }) {
            std::vector<uint16_t> d = ramp;
            // 前 n 个像素置 0，接着 n 个置 65535（分散放置避免空间聚集）
            for (int i = 0; i < n; ++i) d[(size_t)i * 37 % NP] = 0;
            for (int i = 0; i < n; ++i) d[((size_t)i * 91 + 13) % NP] = 65535;
            double frac = 100.0 * n / NP;
            printf("[T] >>> RUN v1_n%d\n", n);
            if (!feedRun(d, R, R, 0, 0, 0, 0, out)) { printf("[T] <<< END FAIL n=%d\n", n); continue; }
            printf("[T] <<< END ok\n");
            auto at = [&](int v) { size_t idx = (size_t)std::llround((double)(v - LO) * (NP - 1) / (HI - LO)); return out[std::min(idx, NP - 1)]; };
            // f(0) / f(65535) 从离群像素读取
            double s0 = 0, s1 = 0; long c0 = 0, c1 = 0;
            for (size_t i = 0; i < NP; ++i) { if (d[i] == 0) { s0 += out[i]; ++c0; } else if (d[i] == 65535) { s1 += out[i]; ++c1; } }
            printf("[T]   n=%6d (%.4f%%) f(0)=%.1f f(65535)=%.1f | ", n, frac, c0 ? s0 / c0 : -1, c1 ? s1 / c1 : -1);
            for (int v = LO; v <= HI; v += 5000) printf("f(%d)=%u ", v, (int)at(v));
            printf("\n");
        }
    }

    // ================= V2) 真实 raw =================
    printf("[T] === V2) 真实 raw 101XL1-1.raw (2560x3072) ===\n");
    {
        const int W = 2560, H = 3072; const size_t NP = (size_t)W * H;
        std::vector<uint16_t> d(NP);
        FILE* fp = fopen("d:\\GitHub\\ImageProcessingPlatform\\101XL1-1.raw", "rb");
        if (!fp) { printf("[T]   无法打开 raw\n"); }
        else {
            size_t rd = fread(d.data(), 2, NP, fp); fclose(fp);
            printf("[T]   读入 %zu 个像素\n", rd);
            uint32_t mn = 65535, mx = 0; double s = 0, s2 = 0;
            std::vector<uint32_t> hist(65536, 0);
            for (auto v : d) { if (v < mn) mn = v; if (v > mx) mx = v; s += v; s2 += (double)v * v; ++hist[v]; }
            double mean = s / NP, sd = sqrt(s2 / NP - mean * mean);
            // 百分位
            std::vector<double> cum(65536); double acc = 0;
            for (int i = 0; i < 65536; ++i) { acc += hist[i]; cum[i] = acc; }
            auto pct = [&](double p) { double tgt = p * NP; for (int i = 0; i < 65536; ++i) if (cum[i] >= tgt) return i; return 65535; };
            printf("[T]   in: min=%u max=%u mean=%.1f sd=%.1f  p0.1=%d p1=%d p50=%d p99=%d p99.9=%d\n",
                mn, mx, mean, sd, pct(0.001), pct(0.01), pct(0.5), pct(0.99), pct(0.999));

            std::vector<std::array<int,4>> cfgs = { {0,0,0,0}, {0,10,0,0}, {0,0,10,0}, {0,0,0,15}, {5,7,5,3}, {0,20,20,0} };
            for (auto& c : cfgs) {
                printf("[T] >>> RUN v2_%d_%d_%d_%d\n", c[0], c[1], c[2], c[3]);
                if (!feedRun(d, W, H, c[0], c[1], c[2], c[3], out)) { printf("[T] <<< END FAIL\n"); continue; }
                printf("[T] <<< END ok\n");
                uint32_t omn = 65535, omx = 0; double os = 0, os2 = 0;
                std::vector<uint32_t> oh(65536, 0);
                for (auto v : out) { if (v < omn) omn = v; if (v > omx) omx = v; os += v; os2 += (double)v * v; ++oh[v]; }
                double om = os / NP, osd = sqrt(os2 / NP - om * om);
                std::vector<double> ocum(65536); double a2 = 0;
                for (int i = 0; i < 65536; ++i) { a2 += oh[i]; ocum[i] = a2; }
                auto opct = [&](double p) { double tgt = p * NP; for (int i = 0; i < 65536; ++i) if (ocum[i] >= tgt) return i; return 65535; };
                printf("[T]   cfg(%2d,%2d,%2d,%2d) out: min=%u max=%u mean=%.1f sd=%.1f sd_gain=%.4f off=%+.1f | p0.1=%d p1=%d p50=%d p99=%d p99.9=%d\n",
                    c[0], c[1], c[2], c[3], omn, omx, om, osd, osd / sd, om - mean,
                    opct(0.001), opct(0.01), opct(0.5), opct(0.99), opct(0.999));
                dumpBins("real", d, out, 512, 40);

                // 存图：按各自 p0.1/p99.9 开窗
                char nm[256];
                auto save = [&](const std::vector<uint16_t>& src, int lo, int hi, const char* tag) {
                    cv::Mat m(H, W, CV_16UC1, (void*)src.data()), m8;
                    double a = 255.0 / std::max(1, hi - lo);
                    m.convertTo(m8, CV_8UC1, a, -lo * a);
                    snprintf(nm, sizeof nm, "real_%s_%d_%d_%d_%d.png", tag, c[0], c[1], c[2], c[3]);
                    cv::imwrite(nm, m8);
                    cv::Mat small; cv::resize(m8, small, cv::Size(W / 4, H / 4), 0, 0, cv::INTER_AREA);
                    snprintf(nm, sizeof nm, "prev_%s_%d_%d_%d_%d.png", tag, c[0], c[1], c[2], c[3]);
                    cv::imwrite(nm, small);
                };
                save(d, pct(0.001), pct(0.999), "in");
                save(out, opct(0.001), opct(0.999), "out");
                // 同一窗口下对比（用输入的窗）
                save(out, pct(0.001), pct(0.999), "outw");
            }
        }
    }
    return 0;
}
