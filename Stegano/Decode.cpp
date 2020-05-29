#include <opencv2/opencv.hpp>
#include <iostream>
#include <array>

constexpr std::array<std::array<unsigned int, 3>, 12> BPCH{{{0, 0, 1},
															{1, 0, 1},
															{1, 1, 1},
															{1, 1, 2},
															{2, 1, 2},
															{2, 2, 2},
															{2, 2, 3},
															{3, 2, 3},
															{3, 3, 3},
															{3, 3, 4},
															{4, 3, 4},
															{4, 4, 4}}};

constexpr std::array<unsigned int, 5> PowersOfTwo{0x1, 0x2, 0x4, 0x8, 0x10};

bool Decode(const std::string& encoded, const std::string& OutputFilePath, const bool& showimages, const bool& verbose) {
	cv::Mat BaseImage{cv::imread(encoded, cv::IMREAD_COLOR)};
	if(!BaseImage.data) {
		std::cerr << "Error! Cannot open base image. Please check if the path is correct and if the file is an 8 bit "
					 "color image.\n";
		return false;
	}
	std::cout << "Decoding " << encoded << "\n\n";

	const unsigned int AvailableBasePixels{static_cast<unsigned int>(BaseImage.rows * BaseImage.cols - 7)};
	const unsigned int TotalBaseChannels{AvailableBasePixels * 3U + 21U};

	// Reading Trailer
	std::array<unsigned char, 5> trailer;
	for(unsigned int ch{21}, i{0}, done{0}; ch > 1; --ch, done += 2) {
		if(done == 8U) {
			done = 0U;
			++i;
		}
		trailer[i] <<= 2;
		trailer[i] += static_cast<unsigned char>(BaseImage.data[TotalBaseChannels - ch] % 4U);
	}

	// Checking validity of the trailer
	if((trailer[0] ^ trailer[1] ^ trailer[2] ^ trailer[3] ^ BaseImage.data[TotalBaseChannels - 1]) != trailer[4]) {
		std::cerr << "The given image does not have any data embedded using this application\n";
		return false;
	}

	unsigned int HiddenImageRows{trailer[0]}, HiddenImageColumns{trailer[2]};
	HiddenImageRows <<= 8;
	HiddenImageRows += trailer[1];
	bool HiddenImageGrayscale = false;
	if(HiddenImageColumns >= 0x80U) {
		HiddenImageColumns -= 0x80U;
		HiddenImageGrayscale = true;
	}
	HiddenImageColumns <<= 8;
	HiddenImageColumns += trailer[3];

	cv::Mat HiddenImage{cv::Mat::zeros(HiddenImageRows, HiddenImageColumns, HiddenImageGrayscale ? CV_8UC1 : CV_8UC3)};

	const unsigned int TotalHiddenImageChannels{HiddenImageRows * HiddenImageColumns * (HiddenImageGrayscale ? 1U : 3U)};
	const unsigned int BitsEncoded{TotalHiddenImageChannels * 8U};
	const unsigned int BitsPerPixel{BitsEncoded / AvailableBasePixels};
	const std::array<unsigned int, 3> bpch{BPCH[BitsPerPixel]};
	const unsigned int stride{(AvailableBasePixels * (BitsPerPixel + 1U) / BitsEncoded) - 1U};
	unsigned char *const HiddenImageData{HiddenImage.data}, *const BaseImageData{BaseImage.data};

	// Extracting hidden bits
	for(unsigned int i{0}, j{0}, BGR{0}, TransferredBits{0}; i < TotalHiddenImageChannels; ++BGR, ++j) {
		if(BGR == 3U) {
			BGR = 0U;
			j += stride * 3U;
		}
		unsigned int ChannelBits{bpch[BGR]};
		if(ChannelBits) {
			if(TransferredBits + ChannelBits > 8U) {
				unsigned int NextChannelBits{ChannelBits + TransferredBits};
				NextChannelBits -= 8U;
				HiddenImageData[i] <<= (8U - TransferredBits);
				HiddenImageData[i] += (BaseImageData[j] >> NextChannelBits) % PowersOfTwo[8U - TransferredBits];
				++i;
				HiddenImageData[i] += BaseImageData[j] % PowersOfTwo[NextChannelBits];
				TransferredBits = NextChannelBits;
			}
			else {
				HiddenImageData[i] <<= ChannelBits;
				HiddenImageData[i] += BaseImageData[j] % PowersOfTwo[ChannelBits];
				TransferredBits += ChannelBits;
				if(TransferredBits == 8U) {
					TransferredBits = 0U;
					++i;
				}
			}
		}
	}

	cv::imwrite(OutputFilePath, HiddenImage);
	cv::namedWindow("Decoded Image", cv::WINDOW_AUTOSIZE);
	cv::imshow("Decoded Image", HiddenImage);
	cv::waitKey(0);

	return true;
}
