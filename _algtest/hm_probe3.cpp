// probe3: 定量辨识 —— 噪声估计器公式 + 正弦光栅频率响应(MTF)
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <windows.h>
#include "HmImgProApi.h"

struct LCG { uint64_t s = 88172645463325252ULL; double next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (double)(s >> 11) / 9007199254740992.0; } };
// 近似高斯
static double gauss(LCG& r) { double u = std::max(r.next(), 1e-12), v = r.next(); return sqrt(-2 * log(u)) * cos(2 * 3.14159265358979323846 * v); }

static const int N = 256;

static void feedRun(const std::vector<uint16_t>& d, int w, int h, int bal, int det, int enh, int den, std::vector<uint16_t>& out)
{
    out.clear();
    setRawData((BYTE*)d.data(), w, h);
    setParam(bal, det, enh, den);
    process();
    int len = -1; char* p = getProcessedData(len);
    if (p && len > 0) out.assign((uint16_t*)p, (uint16_t*)p + len / 2);
}

// 在中心区域用单频相关法测量振幅与相位
static void measureFreq(const std::vector<uint16_t>& d, int w, int h, double freq, double& amp, double& meanVal)
{
    int m = 64;                    // 边界余量
    double sr = 0, sc = 0; long cnt = 0; double sum = 0;
    for (int y = m; y < h - m; ++y) for (int x = m; x < w - m; ++x) {
        double v = d[(size_t)y * w + x];
        double th = 2 * 3.14159265358979323846 * freq * x;
        sr += v * sin(th); sc += v * cos(th); sum += v; ++cnt;
    }
    amp = 2.0 * sqrt(sr * sr + sc * sc) / cnt;
    meanVal = sum / cnt;
}

int main(int argc, char** argv)
{
    InitiImgPro(false);
    std::vector<uint16_t> out;
    LCG rnd;

    printf("[T] === P1) 纯高斯噪声图：辨识 noise0 估计器与 sd 的关系 ===\n");
    {
        double sigmas[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000 };
        for (double sg : sigmas) {
            std::vector<uint16_t> d((size_t)N * N);
            LCG r; 
            for (auto& v : d) v = (uint16_t)std::clamp(30000 + gauss(r) * sg, 0.0, 65535.0);
            printf("[T] >>> RUN noise_sigma=%.0f\n", sg);
            feedRun(d, N, N, 0, 0, 0, 0, out);
            double ia, im_, oa, om;
            measureFreq(d, N, N, 1.0 / 16, ia, im_);   // 无信号，仅取均值
            // 直接算均值/sd
            double s1 = 0, s2 = 0; for (auto v : d) { s1 += v; s2 += (double)v * v; }
            double mn = s1 / d.size(), sd = sqrt(s2 / d.size() - mn * mn);
            double os1 = 0, os2 = 0; for (auto v : out) { os1 += v; os2 += (double)v * v; }
            double omn = os1 / out.size(), osd = sqrt(os2 / out.size() - omn * omn);
            printf("[T] <<< END noise_sigma=%.0f  in_sd=%.2f out_sd=%.2f 衰减比=%.4f\n", sg, sd, osd, sd > 0 ? osd / sd : 0);
        }
    }

    printf("[T] === P2) 平滑斜坡(无噪声) + 不同梯度：noise0 是否受梯度影响 ===\n");
    {
        double grads[] = { 0, 10, 50, 150, 400 };   // 每像素灰度增量
        for (double g : grads) {
            std::vector<uint16_t> d((size_t)N * N);
            LCG r;
            for (int y = 0; y < N; ++y) for (int x = 0; x < N; ++x) {
                double v = 30000 + g * (x - N / 2) + gauss(r) * 30;   // 极小噪声避免退化
                d[(size_t)y * N + x] = (uint16_t)std::clamp(v, 1.0, 65534.0);
            }
            printf("[T] >>> RUN ramp_grad=%.0f\n", g);
            feedRun(d, N, N, 0, 0, 0, 0, out);
            printf("[T] <<< END ramp_grad=%.0f outsize=%zu\n", g, out.size());
        }
    }

    printf("[T] === P3) 正弦光栅 MTF 扫描（params=0,0,0,0） ===\n");
    {
        double A = 4000, M = 30000;
        int ks[] = { 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128 };
        for (int k : ks) {
            double f = (double)k / N;
            std::vector<uint16_t> d((size_t)N * N);
            for (int y = 0; y < N; ++y) for (int x = 0; x < N; ++x)
                d[(size_t)y * N + x] = (uint16_t)std::clamp(M + A * sin(2 * 3.14159265358979323846 * f * x), 0.0, 65535.0);
            printf("[T] >>> RUN mtf_k=%d\n", k);
            feedRun(d, N, N, 0, 0, 0, 0, out);
            double ai = 0, mi = 0, ao = 0, mo = 0;
            measureFreq(d, N, N, f, ai, mi);
            if (!out.empty()) measureFreq(out, N, N, f, ao, mo);
            printf("[T] <<< END mtf_k=%d freq=%.6f cyc/px (period=%.1f px) amp_in=%.1f amp_out=%.1f GAIN=%.5f mean_in=%.1f mean_out=%.1f\n",
                k, f, 1.0 / f, ai, ao, ai > 0 ? ao / ai : 0, mi, mo);
        }
    }

    printf("[T] === P4) 固定一个频率(period=16px)，扫描四个参数对增益的影响 ===\n");
    {
        double A = 4000, M = 30000, f = 1.0 / 16;
        auto gainAt = [&](int bal, int det, int enh, int den, const char* tag) {
            std::vector<uint16_t> d((size_t)N * N);
            for (int y = 0; y < N; ++y) for (int x = 0; x < N; ++x)
                d[(size_t)y * N + x] = (uint16_t)std::clamp(M + A * sin(2 * 3.14159265358979323846 * f * x), 0.0, 65535.0);
            feedRun(d, N, N, bal, det, enh, den, out);
            double ai = 0, mi = 0, ao = 0, mo = 0;
            measureFreq(d, N, N, f, ai, mi);
            if (!out.empty()) measureFreq(out, N, N, f, ao, mo);
            double s1 = 0, s2 = 0; for (auto v : out) { s1 += v; s2 += (double)v * v; }
            double omn = s1 / out.size(), osd = sqrt(s2 / out.size() - omn * omn);
            printf("[T] %s bal=%3d det=%3d enh=%3d den=%3d -> amp_in=%.1f amp_out=%.1f GAIN=%.5f out_mean=%.1f out_sd=%.1f\n",
                tag, bal, det, enh, den, ai, ao, ai > 0 ? ao / ai : 0, omn, osd);
        };
        gainAt(0, 0, 0, 0, "[T] base ");
        for (int v = 0; v <= 20; v += 2) gainAt(v, 0, 0, 0, "[T] bal  ");
        for (int v = 0; v <= 20; v += 2) gainAt(0, v, 0, 0, "[T] det  ");
        for (int v = 0; v <= 20; v += 2) gainAt(0, 0, v, 0, "[T] enh  ");
        for (int v = 0; v <= 40; v += 5) gainAt(0, 0, 0, v, "[T] den  ");
    }
    return 0;
}
