#include "pixbench/utils/utils.h"
#include <random>
#include <cstdint>


std::random_device g_random_device;
std::mt19937 g_random_gen;
std::uniform_int_distribution<uint32_t> g_random_dis;
void PrepareRandomGenerator() {
    g_random_gen = std::mt19937(g_random_device());
}

uint32_t GenerateRandomUInt32() {
    return g_random_dis(g_random_gen);
}
