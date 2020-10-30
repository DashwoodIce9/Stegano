/* Copyright 2020 Prakhar Agarwal*/

#pragma once

#include <array>
#include <tuple>
#include <opencv2/opencv.hpp>
#include "SteganoLogger.h"

namespace Stegano {
// BPCH = Bits per channel
constexpr std::array<std::array<unsigned int, 3>, 12> BPCH{
	{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {1, 1, 2}, {2, 1, 2}, {2, 2, 2}, {2, 2, 3}, {3, 2, 3}, {3, 3, 3}, {3, 3, 4}, {4, 3, 4}, {4, 4, 4}}};

constexpr std::array<unsigned int, 9> PowersOfTwo{0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100};

extern bool showimages;

#if _WIN32
inline void ResizeToSmall(const cv::Mat& input, cv::Mat& output, const std::string& name, double extrashrinkfactor = 1.0);
#endif
}
