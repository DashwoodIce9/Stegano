#include <opencv2/opencv.hpp>
#include <iostream>
#include "SteganoCommon.h"

namespace Stegano {
bool Decode(const std::string& source, const std::string& OutputFilePath) {
	if(verbose) {
		std::cout << "Reading source image\n";
	}
	cv::Mat SourceImage{cv::imread(source, cv::IMREAD_COLOR)};
	if(!SourceImage.data) {
		std::cerr << "Error! Cannot open base image. Please check if the path is correct and if the file is an 8 bit "
					 "color image.\n";
		return false;
	}
	if(verbose) {
		std::cout << "Source image size = [" << SourceImage.rows << " x " << SourceImage.cols << " x " << SourceImage.channels()
				  << "]\n\n";
	}

	const unsigned int AvailableBasePixels{static_cast<unsigned int>(SourceImage.rows * SourceImage.cols - 7)};
	const unsigned int TotalBaseChannels{AvailableBasePixels * 3U + 21U};

	// Reading Trailer
	std::array<unsigned char, 5> trailer{0, 0, 0, 0, 0};
	for(unsigned int ch{21}, i{0}, done{0}; ch > 1; --ch, done += 2) {
		if(done == 8U) {
			done = 0U;
			++i;
		}
		trailer[i] *= PowersOfTwo[2];
		trailer[i] += static_cast<unsigned char>(SourceImage.data[TotalBaseChannels - ch] % 4U);
	}

	// Checking validity of the trailer
	if((trailer[0] ^ trailer[1] ^ trailer[2] ^ trailer[3] ^ SourceImage.data[TotalBaseChannels - 1]) != trailer[4]) {
		std::cerr << "The given image does not have any data embedded using this application\n";
		return false;
	}

	if(verbose) {
		std::cout << "Encoded image found, decoding...\n";
	}

	unsigned int DecodedImageRows{trailer[0]}, DecodedImageColumns{trailer[2]};
	DecodedImageRows *= PowersOfTwo[8];
	DecodedImageRows += trailer[1];
	bool DecodedImageGrayscale = false;
	if(DecodedImageColumns >= PowersOfTwo[7]) {
		DecodedImageColumns -= PowersOfTwo[7];
		DecodedImageGrayscale = true;
	}
	DecodedImageColumns *= PowersOfTwo[8];
	DecodedImageColumns += trailer[3];

	cv::Mat DecodedImage{cv::Mat::zeros(static_cast<int>(DecodedImageRows), static_cast<int>(DecodedImageColumns),
										DecodedImageGrayscale ? CV_8UC1 : CV_8UC3)};

	const unsigned int TotalDecodedImageChannels{DecodedImageRows * DecodedImageColumns * (DecodedImageGrayscale ? 1U : 3U)};
	const unsigned int BitsEncoded{TotalDecodedImageChannels * 8U};
	const unsigned int BitsPerPixel{BitsEncoded / AvailableBasePixels};
	const std::array<unsigned int, 3> bpch{BPCH[BitsPerPixel]};
	const unsigned int stride{(AvailableBasePixels * (BitsPerPixel + 1U) / BitsEncoded) - 1U};
	unsigned char *const DecodedImageData{DecodedImage.data}, *const SourceImageData{SourceImage.data};

	// Extracting Encoded bits
	for(unsigned int i{0}, j{0}, BGR{0}, TransferredBits{0}; i < TotalDecodedImageChannels; ++BGR, ++j) {
		if(BGR == 3U) {
			BGR = 0U;
			j += stride * 3U;
		}
		unsigned int ChannelBits{bpch[BGR]};
		if(ChannelBits) {
			if(TransferredBits + ChannelBits > 8U) {
				unsigned int NextChannelBits{ChannelBits + TransferredBits};
				NextChannelBits -= 8U;
				DecodedImageData[i] *= PowersOfTwo[8U - TransferredBits];
				DecodedImageData[i] += (SourceImageData[j] / PowersOfTwo[NextChannelBits]) % PowersOfTwo[8U - TransferredBits];
				++i;
				DecodedImageData[i] += SourceImageData[j] % PowersOfTwo[NextChannelBits];
				TransferredBits = NextChannelBits;
			}
			else {
				DecodedImageData[i] *= PowersOfTwo[ChannelBits];
				DecodedImageData[i] += SourceImageData[j] % PowersOfTwo[ChannelBits];
				TransferredBits += ChannelBits;
				if(TransferredBits == 8U) {
					TransferredBits = 0U;
					++i;
				}
			}
		}
	}

	if(verbose) {
		std::cout << "Finished decoding\n";
		std::cout << "Saving decoded image\n";
	}

	try {
		cv::imwrite(OutputFilePath, DecodedImage);
		std::cout << "Image saved at - " << OutputFilePath << "\n\n";
	}
	catch(cv::Exception& e) {
		if(e.code == -2) {
			std::cerr << "Error! Cannot save the output file with the given name!\n";
			std::cout << "If this is a privileged directory, please run this application in elevated mode.\n";
			std::cout << "Saving as Decoded.png in the working directory";
			try {
				cv::imwrite("Decoded.png", DecodedImage);
				std::cout << "Image saved at - .\\Decoded.png\n";
			}
			catch(cv::Exception& E) {
				if(E.code == -2) {
					std::cerr << "Error! Cannot save as Decoded.png as well, skipping save step.\n";
				}
			}
		}
	}

	if(showimages) {
#if _WIN32
		ResizeToSmall(DecodedImage, DecodedImage, "Decoded Image");
#endif
		cv::namedWindow("Decoded Image", cv::WINDOW_AUTOSIZE);
		cv::imshow("Decoded Image", DecodedImage);
		cv::waitKey(0);
	}

	return true;
}
}
