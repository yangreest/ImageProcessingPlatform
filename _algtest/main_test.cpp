// 临时验证程序：统计 raw 直方图/ROI，并渲染多种窗宽窗位变体用于调参。
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <opencv2/opencv.hpp>
#include "ImageProcessByOpenCV.h"

// harness 不链接 ImageProcessBase.cpp（其依赖 Hamming SDK），在此提供参数结构体构造定义
CImageProcessParam1::CImageProcessParam1() { m_nBalanced = 0; m_nDetail = 7; m_nEnhance = 0; m_nDenoise = 0; }
CImageProcessParam2::CImageProcessParam2() { m_wMaxBright = 0; m_wMinBright = 0; }

static const int W = 2560, H = 3072;

static void printPercentiles(const std::vector<uint8_t>& raw)
{
    std::vector<uint32_t> hist(65536, 0);
    size_t n = raw.size() / 2;
    for (size_t i = 0; i < n; ++i)
    {
        uint16_t v = (uint16_t)raw[2 * i] | ((uint16_t)raw[2 * i + 1] << 8);
        ++hist[v];
    }
    double pcts[] = { 0.0, 0.01, 0.1, 1, 10, 50, 90, 99, 99.9, 99.99, 100.0 };
    uint64_t cum = 0; int pi = 0;
    for (int i = 0; i < 65536 && pi < 11; ++i)
    {
        cum += hist[i];
        while (pi < 11 && (double)cum >= n * pcts[pi] / 100.0) { printf("pct %6.2f%% = %d\n", pcts[pi], i); ++pi; }
    }
}

static void printRoi(const char* name, const std::vector<uint8_t>& raw, int x, int y, int w, int h)
{
    double sum = 0, sum2 = 0; long cnt = 0; double mn = 1e9, mx = -1e9;
    for (int j = y; j < y + h; ++j)
        for (int i = x; i < x + w; ++i)
        {
            double v = (uint16_t)raw[2 * (j * W + i)] | ((uint16_t)raw[2 * (j * W + i) + 1] << 8);
            sum += v; sum2 += v * v; ++cnt;
            if (v < mn) mn = v; if (v > mx) mx = v;
        }
    double mean = sum / cnt;
    double sd = sqrt(sum2 / cnt - mean * mean);
    printf("%s: mean=%.0f sd=%.0f min=%.0f max=%.0f\n", name, mean, sd, mn, mx);
}

static void printRoi8(const char* name, const std::vector<uint8_t>& img8, int x, int y, int w, int h)
{
    double sum = 0, sum2 = 0; long cnt = 0; double mn = 1e9, mx = -1e9;
    for (int j = y; j < y + h; ++j)
        for (int i = x; i < x + w; ++i)
        {
            double v = img8[j * W + i];
            sum += v; sum2 += v * v; ++cnt;
            if (v < mn) mn = v; if (v > mx) mx = v;
        }
    double mean = sum / cnt; double sd = sqrt(sum2 / cnt - mean * mean);
    printf("  out8 %s: mean=%.1f sd=%.1f min=%.0f max=%.0f\n", name, mean, sd, mn, mx);
}

int main(int argc, char** argv)
{
    const char* rawPath = argc > 1 ? argv[1] : "d:/GitHub/ImageProcessingPlatform/101XL1-1.raw";
    std::ifstream f(rawPath, std::ios::binary);
    if (!f) { printf("cannot open raw\n"); return 1; }
    std::vector<uint8_t> raw((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if ((int)raw.size() != W * H * 2) { printf("size mismatch: %zu\n", raw.size()); return 1; }

    printPercentiles(raw);
    // 背景区 / 金属杆区 / 金属内部空腔附近（坐标按 2560x3072 估）
    printRoi("background", raw, 500, 1100, 200, 200);
    printRoi("metal_rod  ", raw, 1750, 2000, 120, 200);
    printRoi("metal_head ", raw, 1000, 2300, 200, 200);

    CImageProcessByOpenCV algo;
    CImageProcessParam1 p1; p1.m_nDenoise = 33; p1.m_nEnhance = 10;
    std::vector<uint8_t> temp;
    if (!algo.ImageProcess1(W, H, raw.data(), &p1, &temp)) { printf("ImageProcess1 fail\n"); return 1; }

    int nMax = 0, nMin = 0;
    algo.calculateMaxandMinBright(raw, nMax, nMin);
    printf("auto(percentile) window: max=%d min=%d\n", nMax, nMin);

    // 真实 min/max（旧算法窗口）
    uint16_t tmin = 65535, tmax = 0;
    for (size_t i = 0; i < raw.size() / 2; ++i)
    {
        uint16_t v = (uint16_t)raw[2 * i] | ((uint16_t)raw[2 * i + 1] << 8);
        if (v < tmin) tmin = v; if (v > tmax) tmax = v;
    }
    printf("true min/max: %d / %d\n", tmin, tmax);

    auto render = [&](uint16_t wmax, uint16_t wmin, const char* path) {
        CImageProcessParam2 p2; p2.m_wMaxBright = wmax; p2.m_wMinBright = wmin;
        std::vector<uint8_t> out8;
        if (algo.BrightAndContrastProcess(W, H, (uint16_t*)temp.data(), &p2, &out8))
        {
            cv::imwrite(path, cv::Mat(H, W, CV_8UC1, out8.data())); printf("saved %s\n", path);
            printRoi8("metal_head", out8, 1000, 2300, 200, 200);
            printRoi8("metal_rod ", out8, 1750, 2000, 120, 200);
            printRoi8("background", out8, 500, 1100, 200, 200);
        }
    };
    render((uint16_t)nMax, (uint16_t)nMin, "d:/GitHub/ImageProcessingPlatform/_algtest/v_percentile.png");
    render(tmax, tmin, "d:/GitHub/ImageProcessingPlatform/_algtest/v_fullrange.png");
    return 0;
}
