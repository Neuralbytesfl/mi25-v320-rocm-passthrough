#include <hip/hip_runtime.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#define HIP_CHECK(call) do {                                           \
    hipError_t err = (call);                                           \
    if (err != hipSuccess) {                                           \
        std::fprintf(stderr, "HIP error %s:%d: %s\n",                  \
                     __FILE__, __LINE__, hipGetErrorString(err));      \
        return 1;                                                      \
    }                                                                  \
} while (0)

__device__ __forceinline__ uint32_t hash32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

__global__ void init_random(uint8_t *grid, int width, int height, uint32_t seed) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) {
        return;
    }
    uint32_t h = hash32((uint32_t)(y * width + x) ^ seed);
    grid[y * width + x] = (h & 7U) < 2U;
}

__global__ void init_glider(uint8_t *grid, int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) {
        return;
    }
    int cx = width / 2;
    int cy = height / 2;
    bool alive =
        (x == cx + 1 && y == cy) ||
        (x == cx + 2 && y == cy + 1) ||
        (x == cx && y == cy + 2) ||
        (x == cx + 1 && y == cy + 2) ||
        (x == cx + 2 && y == cy + 2);
    grid[y * width + x] = alive;
}

__global__ void life_step(const uint8_t *__restrict__ in,
                          uint8_t *__restrict__ out,
                          int width,
                          int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) {
        return;
    }

    int xm1 = (x == 0) ? width - 1 : x - 1;
    int xp1 = (x == width - 1) ? 0 : x + 1;
    int ym1 = (y == 0) ? height - 1 : y - 1;
    int yp1 = (y == height - 1) ? 0 : y + 1;

    int n =
        in[ym1 * width + xm1] + in[ym1 * width + x] + in[ym1 * width + xp1] +
        in[y * width + xm1]                             + in[y * width + xp1] +
        in[yp1 * width + xm1] + in[yp1 * width + x] + in[yp1 * width + xp1];

    uint8_t alive = in[y * width + x];
    out[y * width + x] = (n == 3 || (alive && n == 2)) ? 1 : 0;
}

static void print_preview(const std::vector<uint8_t> &grid, int width, int height) {
    int shown_w = std::min(width, 96);
    int shown_h = std::min(height, 48);
    for (int y = 0; y < shown_h; ++y) {
        for (int x = 0; x < shown_w; ++x) {
            std::putchar(grid[y * width + x] ? '#' : '.');
        }
        std::putchar('\n');
    }
}

int main(int argc, char **argv) {
    int width = argc > 1 ? std::atoi(argv[1]) : 4096;
    int height = argc > 2 ? std::atoi(argv[2]) : 4096;
    int steps = argc > 3 ? std::atoi(argv[3]) : 500;
    bool glider = argc > 4 && std::string(argv[4]) == "glider";

    hipDeviceProp_t prop{};
    HIP_CHECK(hipGetDeviceProperties(&prop, 0));
    std::printf("Device: %s arch=%s CU=%d VRAM=%.2f GiB\n",
                prop.name, prop.gcnArchName, prop.multiProcessorCount,
                prop.totalGlobalMem / 1024.0 / 1024.0 / 1024.0);
    std::printf("Game of Life: %dx%d, steps=%d, init=%s\n",
                width, height, steps, glider ? "glider" : "random");

    size_t bytes = (size_t)width * (size_t)height;
    uint8_t *a = nullptr;
    uint8_t *b = nullptr;
    HIP_CHECK(hipMalloc(&a, bytes));
    HIP_CHECK(hipMalloc(&b, bytes));

    dim3 block(16, 16);
    dim3 grid((width + block.x - 1) / block.x,
              (height + block.y - 1) / block.y);

    if (glider) {
        init_glider<<<grid, block>>>(a, width, height);
    } else {
        init_random<<<grid, block>>>(a, width, height, 0xC0FFEEU);
    }
    HIP_CHECK(hipMemset(b, 0, bytes));
    HIP_CHECK(hipDeviceSynchronize());

    hipEvent_t start, stop;
    HIP_CHECK(hipEventCreate(&start));
    HIP_CHECK(hipEventCreate(&stop));
    HIP_CHECK(hipEventRecord(start));
    for (int i = 0; i < steps; ++i) {
        life_step<<<grid, block>>>(a, b, width, height);
        std::swap(a, b);
    }
    HIP_CHECK(hipEventRecord(stop));
    HIP_CHECK(hipEventSynchronize(stop));
    HIP_CHECK(hipGetLastError());

    float ms = 0.0f;
    HIP_CHECK(hipEventElapsedTime(&ms, start, stop));
    double cells = (double)width * (double)height * (double)steps;
    std::printf("Kernel time: %.3f ms\n", ms);
    std::printf("Throughput: %.2f billion cell-updates/sec\n", cells / (ms / 1000.0) / 1.0e9);

    std::vector<uint8_t> host(bytes);
    HIP_CHECK(hipMemcpy(host.data(), a, bytes, hipMemcpyDeviceToHost));
    size_t alive = 0;
    for (uint8_t v : host) {
        alive += v != 0;
    }
    std::printf("Alive cells: %zu (%.2f%%)\n", alive, 100.0 * (double)alive / (double)bytes);

    if (width <= 256 && height <= 128) {
        std::puts("Preview:");
        print_preview(host, width, height);
    }

    hipEventDestroy(start);
    hipEventDestroy(stop);
    hipFree(a);
    hipFree(b);
    return 0;
}
