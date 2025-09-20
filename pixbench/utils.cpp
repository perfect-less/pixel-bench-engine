#include "pixbench/utils/utils.h"
#include <random>
#include <sys/types.h>


std::random_device g_random_device;
std::mt19937 g_random_gen;
std::uniform_int_distribution<u_int32_t> g_random_dis;
void PrepareRandomGenerator() {
    g_random_gen = std::mt19937(g_random_device());
}

u_int32_t GenerateRandomUInt32() {
    return g_random_dis(g_random_gen);
}
