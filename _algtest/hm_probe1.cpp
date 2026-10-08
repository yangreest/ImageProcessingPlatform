// probe1: 探测 Hamming ImgProcCplusplus.dll 的 API 语义（时序 / 参数范围 / 输出长度）
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <windows.h>
#include "HmImgProApi.h"

static int W = 64, H = 64;

static void dumpHead(const char* tag, const uint16_t* p, int n)
{
    printf("  %s first16:", tag);
    for (int i = 0; i < 16 && i < n; ++i) printf(" %u", p[i]);
    printf("\n  %s mid16  :", tag);
    for (int i = n / 2; i < n / 2 + 16 && i < n; ++i) printf(" %u", p[i]);
    uint32_t mn = 65535, mx = 0; double s = 0;
    for (int i = 0; i < n; ++i) { if (p[i] < mn) mn = p[i]; if (p[i] > mx) mx = p[i]; s += p[i]; }
    printf("  %s min=%u max=%u mean=%.2f\n", tag, mn, mx, s / (n ? n : 1));
}

int main()
{
    printf("=== 1) InitiImgPro ===\n");
    printf("  InitiImgPro(false) = %d\n", InitiImgPro(false));

    printf("=== 2) 未设置数据时直接 process ===\n");
    printf("  process() = %d\n", process());
    int L = -1; char* d = getProcessedData(L);
    printf("  getProcessedData -> ptr=%p len=%d\n", (void*)d, L);

    printf("=== 3) setRawData 尺寸接受度（常数图 1000） ===\n");
    auto makeConst = [&](int w, int h, uint16_t v) {
        std::vector<uint16_t> img((size_t)w * h, v);
        return img;
    };
    int sizes[][2] = { {16,16}, {64,64}, {256,256}, {100,100}, {3,5} };
    for (auto& s : sizes) {
        auto img = makeConst(s[0], s[1], 1000);
        int r = setRawData((BYTE*)img.data(), s[0], s[1]);
        int rp = process();
        int len = -1; char* pd = getProcessedData(len);
        printf("  %dx%d: setRawData=%d process=%d outlen=%d (w*h=%d, w*h*2=%d) ptr=%s\n",
            s[0], s[1], r, rp, len, s[0]*s[1], s[0]*s[1]*2, pd ? "ok" : "NULL");
    }

    printf("=== 4) 常数图经过 process 后是否变化（增益/偏置探测） ===\n");
    for (uint16_t v : {0, 1, 100, 1000, 32768, 60000, 65535}) {
        auto img = makeConst(W, H, v);
        setRawData((BYTE*)img.data(), W, H);
        setParam(0, 0, 0, 0);
        process();
        int len = -1; char* pd = getProcessedData(len);
        if (!pd) { printf("  in=%5u -> NULL\n", v); continue; }
        printf("  in=%5u ->", v);
        dumpHead("out", (const uint16_t*)pd, len / 2);
    }

    printf("=== 5) setParam 参数范围接受度 ===\n");
    auto base = makeConst(W, H, 1000);
    setRawData((BYTE*)base.data(), W, H);
    int vals[] = { -5, -1, 0, 1, 5, 10, 20, 50, 100, 255, 1000 };
    for (int v : vals) {
        printf("  setParam(%d,%d,%d,%d) = %d\n", v, v, v, v, setParam(v, v, v, v));
    }
    printf("  单参数扫描（其余为0）:\n");
    for (int v : vals) {
        printf("    balanced=%4d -> %d | detail=%4d -> %d | enhance=%4d -> %d | denoise=%4d -> %d\n",
            v, setParam(v, 0, 0, 0), v, setParam(0, v, 0, 0), v, setParam(0, 0, v, 0), v, setParam(0, 0, 0, v));
    }

    printf("=== 6) getBitmapData vs getProcessedData ===\n");
    setRawData((BYTE*)base.data(), W, H);
    setParam(0, 0, 0, 0);
    process();
    int l16 = -1, l8 = -1;
    char* p16 = getProcessedData(l16);
    unsigned char* p8 = getBitmapData(l8);
    printf("  16bit len=%d  8bit len=%d  (w*h=%d)\n", l16, l8, W * H);
    if (p16) dumpHead("16bit", (const uint16_t*)p16, l16 / 2);
    if (p8) {
        printf("  8bit first16:");
        for (int i = 0; i < 16 && i < l8; ++i) printf(" %u", p8[i]);
        uint32_t mn = 255, mx = 0; double s = 0;
        for (int i = 0; i < l8; ++i) { if (p8[i] < mn) mn = p8[i]; if (p8[i] > mx) mx = p8[i]; s += p8[i]; }
        printf("\n  8bit min=%u max=%u mean=%.2f\n", mn, mx, s / (l8 ? l8 : 1));
    }

    printf("=== 7) setBrightnessContrast 行为 ===\n");
    int bc[][2] = { {0,0}, {100,100}, {1000,1000}, {32768,32768}, {65535,0}, {6452,1977}, {-1,-1}, {100000,100000} };
    for (auto& p : bc) {
        int r = setBrightnessContrast(p[0], p[1]);
        int len = -1; char* pd = getProcessedData(len);
        int l8 = -1; unsigned char* p8 = getBitmapData(l8);
        printf("  setBrightnessContrast(%d,%d)=%d", p[0], p[1], r);
        if (pd) {
            const uint16_t* q = (const uint16_t*)pd;
            uint32_t mn = 65535, mx = 0; double s = 0;
            for (int i = 0; i < len / 2; ++i) { if (q[i] < mn) mn = q[i]; if (q[i] > mx) mx = q[i]; s += q[i]; }
            printf("  16[min=%u max=%u mean=%.1f]", mn, mx, s / (len / 2));
        }
        if (p8) {
            uint32_t mn = 255, mx = 0; double s = 0;
            for (int i = 0; i < l8; ++i) { if (p8[i] < mn) mn = p8[i]; if (p8[i] > mx) mx = p8[i]; s += p8[i]; }
            printf("  8[min=%u max=%u mean=%.1f]", mn, mx, s / l8);
        }
        printf("\n");
    }

    printf("=== 8) 时序：setParam 在 process 之后是否生效；重复 process ===\n");
    setRawData((BYTE*)base.data(), W, H);
    setParam(0, 0, 0, 0); process();
    int la = -1; char* pa = getProcessedData(la);
    std::vector<uint16_t> A; if (pa) A.assign((uint16_t*)pa, (uint16_t*)pa + la / 2);
    process(); process();
    int lb = -1; char* pb = getProcessedData(lb);
    bool same = (A.size() == (size_t)lb / 2) && memcmp(A.data(), pb, A.size() * 2) == 0;
    printf("  重复 process 结果一致: %d\n", same);
    setParam(8, 8, 8, 8);
    int lc = -1; char* pc = getProcessedData(lc);
    bool same2 = pc && (A.size() == (size_t)lc / 2) && memcmp(A.data(), pc, A.size() * 2) == 0;
    printf("  setParam 后不 process，输出是否仍等于旧值: %d (说明 setParam 是否立即生效)\n", same2);

    printf("=== 9) 指针稳定性：两次 getProcessedData 是否同一缓冲 ===\n");
    int l1 = -1, l2 = -1;
    char* q1 = getProcessedData(l1); char* q2 = getProcessedData(l2);
    printf("  q1=%p q2=%p same=%d\n", (void*)q1, (void*)q2, q1 == q2);

    printf("=== 10) GetSDKVersion ===\n");
    SDKVersionInfo vi{};
    printf("  GetSDKVersion=%d  MS=%u.%u LS=%u.%u\n", GetSDKVersion("ImgProcCplusplus.dll", &vi),
        HIWORD(vi.dwFileVersionMS), LOWORD(vi.dwFileVersionMS), HIWORD(vi.dwFileVersionLS), LOWORD(vi.dwFileVersionLS));
    return 0;
}
