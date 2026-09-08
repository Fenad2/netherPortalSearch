#include <iostream>
#include <sstream>
#include <cmath>
#include <limits>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using std::cin;
using std::cout;
using std::string;

static bool readCoord(const char* prompt, long long& x, long long& z) {
    while (true) {
        cout << prompt;
        string line;
        if (!std::getline(cin, line)) return false; // EOF
        // 用 stringstream 解析两个数
        std::stringstream ss(line);
        long long a, b;
        if (ss >> a >> b) {
            x = a; z = b;
            return true;
        }
        cout << "  坐标格式应为两个整数，例如：-739 3915\n";
    }
}

struct SearchBox {
    long long loX, hiX, loZ, hiZ;
};

static long long toSection(long long block) {
    return block >= 0 ? block / 16 : (block - 15) / 16;
}

static SearchBox searchBox(long long cx, long long cz, long long R) {
    SearchBox b;
    b.loX = toSection(cx - R) * 16;   b.hiX = toSection(cx + R) * 16 + 15;
    b.loZ = toSection(cz - R) * 16;   b.hiZ = toSection(cz + R) * 16 + 15;
    return b;
}

static bool inBox(const SearchBox& b, long long x, long long z) {
    return b.loX <= x && x <= b.hiX && b.loZ <= z && z <= b.hiZ;
}

static void nearestInBox(const SearchBox& b, double px, double pz,
                         long long& qx, long long& qz, double& outDistSq) {
    double loX = (double)b.loX, hiX = (double)b.hiX;
    double loZ = (double)b.loZ, hiZ = (double)b.hiZ;
    double projx = std::max(loX, std::min(px, hiX));
    double projz = std::max(loZ, std::min(pz, hiZ));
    long long fx = (long long)std::floor(projx), cx_ = (long long)std::ceil(projx);
    long long fz = (long long)std::floor(projz), cz_ = (long long)std::ceil(projz);
    double bestD = std::numeric_limits<double>::infinity();
    long long bx_ = fx, bz_ = fz;
    const long long xs[2] = { fx, cx_ };
    const long long zs[2] = { fz, cz_ };
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            long long gx = xs[i], gz = zs[j];
            if (b.loX <= gx && gx <= b.hiX && b.loZ <= gz && gz <= b.hiZ) {
                double d = ((double)gx - px) * ((double)gx - px)
                         + ((double)gz - pz) * ((double)gz - pz);
                if (d < bestD) { bestD = d; bx_ = gx; bz_ = gz; }
            }
        }
    }
    qx = bx_; qz = bz_; outDistSq = bestD;
}

static bool solveTwoBlocks(const SearchBox& b1, const SearchBox& b2,
                           double c1x, double c1z,
                           long long& pX, long long& pZ,
                           long long& qX, long long& qZ) {
    double bestDistSq = std::numeric_limits<double>::infinity();
    bool found = false;
    for (long long gx = b1.loX; gx <= b1.hiX; ++gx) {
        for (long long gz = b1.loZ; gz <= b1.hiZ; ++gz) {
            long long nqx, nqz; double nd;
            nearestInBox(b2, (double)gx, (double)gz, nqx, nqz, nd);
            if (nd < bestDistSq) {
                bestDistSq = nd;
                pX = gx; pZ = gz; qX = nqx; qZ = nqz;
                found = true;
            }
        }
    }
    return found;
}

static void emitTwoBlockResult(const SearchBox& b1, const SearchBox& b2,
                               double c1x, double c1z, const char* dstDim) {
    long long pX, pZ, qX, qZ;
    cout << "\n[结果] 单个门块的正方形搜索区域不重叠，改用两块地狱门方块：\n";
    if (!solveTwoBlocks(b1, b2, c1x, c1z, pX, pZ, qX, qZ)) {
        cout << "  连两块也无法找出可用的放置位置（异常情形）。\n";
        return;
    }
    double dist = std::sqrt((double)(pX - qX) * (pX - qX) + (double)(pZ - qZ) * (pZ - qZ));
    cout << "  在目标维度(" << dstDim << ")放置两块门块（两块 Y 值相同）：\n";
    cout << "    第一块 方块坐标(" << pX << ", " << pZ << ") 对齐中心+4 (" << (pX + 4) << ", " << (pZ + 4) << ")  —— 连接边角1\n";
    cout << "    第二块 方块坐标(" << qX << ", " << qZ << ") 对齐中心+4 (" << (qX + 4) << ", " << (qZ + 4) << ")  —— 连接边角2\n";
    cout << "  两块的平面距离 = " << dist << "\n";
}

static int run() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // 让 cmd 按 UTF-8 显示中文
    SetConsoleCP(CP_UTF8);
#endif
    cout << "============================================================\n";
    cout << "  地狱门对门工具\n";
    cout << "============================================================\n\n";

    long long aX, aZ, bX, bZ;
    if (!readCoord("输入边角1坐标(x z): ", aX, aZ)) return 0;
    if (!readCoord("输入边角2坐标(x z): ", bX, bZ)) return 0;

    // 目标维度：这些坐标传送过去之后是哪个维度
    int targetDim = -1;
    while (targetDim != 0 && targetDim != 1) {
        cout << "这些坐标传送过去是哪个维度? (0=主世界  1=地狱): ";
        string line;
        if (!std::getline(cin, line)) return 0;
        std::stringstream ss(line);
        int v;
        if (ss >> v && (v == 0 || v == 1)) targetDim = v;
        else cout << "  请输入 0 或 1\n";
    }

    const double scale = (targetDim == 0) ? 8.0 : (1.0 / 8.0);
    const long long R  = (targetDim == 0) ? 128 : 16;
    const double R2    = (double)R * (double)R;

    const char* srcDim = (targetDim == 0) ? "下界" : "主世界";
    const char* dstDim = (targetDim == 0) ? "主世界" : "下界";

    cout << "\n  源维度: " << srcDim << "   对映目标维度: " << dstDim
         << "   缩放: x" << scale << "   搜索半径: " << R << "\n";

    double c1x = (double)aX * scale, c1z = (double)aZ * scale;
    double c2x = (double)bX * scale, c2z = (double)bZ * scale;
    cout << "  边角1 -> 目标(缩放后)(" << c1x << ", " << c1z << ")\n";
    cout << "  边角2 -> 目标(缩放后)(" << c2x << ", " << c2z << ")\n";

    long long c1iX = (long long)std::floor(c1x), c1iZ = (long long)std::floor(c1z);
    long long c2iX = (long long)std::floor(c2x), c2iZ = (long long)std::floor(c2z);
    cout << "  边角1 -> 目标搜索中心(" << c1iX << ", " << c1iZ << ")\n";
    cout << "  边角2 -> 目标搜索中心(" << c2iX << ", " << c2iZ << ")\n";
    cout << "  搜索区域: 以搜索中心为基准 ±半径" << R << " 并按区块(16)对齐的正方形\n";

    SearchBox b1 = searchBox(c1iX, c1iZ, R);
    SearchBox b2 = searchBox(c2iX, c2iZ, R);
    cout << "  边角1搜索方形: x[" << b1.loX << ", " << b1.hiX << "]  z["
         << b1.loZ << ", " << b1.hiZ << "]\n";
    cout << "  边角2搜索方形: x[" << b2.loX << ", " << b2.hiX << "]  z["
         << b2.loZ << ", " << b2.hiZ << "]\n";

    long long loX = std::max(b1.loX, b2.loX);
    long long hiX = std::min(b1.hiX, b2.hiX);
    long long loZ = std::max(b1.loZ, b2.loZ);
    long long hiZ = std::min(b1.hiZ, b2.hiZ);

    if (loX > hiX || loZ > hiZ) {
        cout << "  两个搜索正方形在目标维度没有公共区域，单个门块无法同时被两边搜到。\n";
        emitTwoBlockResult(b1, b2, c1x, c1z, dstDim);
        return 0;
    }

    bool found = false;
    long long bestX = 0, bestZ = 0;
    double bestScore = std::numeric_limits<double>::infinity();
    long long hitCount = 0;

    for (long long gx = loX; gx <= hiX; ++gx) {
        for (long long gz = loZ; gz <= hiZ; ++gz) {
            if (inBox(b1, gx, gz) && inBox(b2, gx, gz)) {
                ++hitCount;
                double dx1 = (double)(gx - c1iX), dz1 = (double)(gz - c1iZ);
                double dx2 = (double)(gx - c2iX), dz2 = (double)(gz - c2iZ);
                double score = std::max(dx1*dx1 + dz1*dz1, dx2*dx2 + dz2*dz2);
                if (score < bestScore) {
                    bestScore = score;
                    bestX = gx; bestZ = gz;
                    found = true;
                }
            }
        }
    }

    if (!found) {
        emitTwoBlockResult(b1, b2, c1x, c1z, dstDim);
        return 0;
    }

    cout << "\n[结果] 可以用单个地狱门方块连接！\n";
    cout << "  推荐方块坐标(目标维度 " << dstDim << "):\n";
    cout << "    (" << bestX << ", " << bestZ << ")\n";
    cout << "  对齐方块中心(坐标 x、z 各+4)最终放置坐标:\n";
    cout << "    (" << (bestX + 4) << ", " << (bestZ + 4) << ")\n";
    cout << "  同时命中两边搜索方形的可选格点数量: " << hitCount << "\n";

    return 0;
}

int main() {
    int code = run();
    std::cout << "\n按回车键退出...";
    std::string line;
    std::getline(cin, line);
    return code;
}