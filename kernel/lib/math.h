#pragma once
#include <stdint.h>

#pragma function(pow)
double pow(double n1, double n2);
uint64_t powi(uint64_t num, uint16_t count);
// double abs(double Val);

#define max(a, b) (a > b ? a : b)