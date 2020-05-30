#include <opencv2/opencv.hpp>
#include <iostream>
#include "SteganoCommon.h"

/* TODO
** Add functionality to support images with FP16/32 channel types
** Add support to encode beyond 8x reduction in image sizes
** Add option to set preference for size reduction or grayscale conversion if bpch > 12
** Multithreading
** Add option for no reduction
** Add option for force encoding, i.e. encode even if base is not large enough. Portion of source will not be encoded
** Add option for converting source/base to grayscale and to enlarge or shrink them by given factors
*/

namespace Stegano {
#if _WIN32
void ResizeToSmall(const cv::Mat& input, cv::Mat& output, const std::string& name) {
	const double hRatio{static_cast<double>(DesktopWidth) / static_cast<double>(input.cols)};
	const double vRatio{static_cast<double>(DesktopHeight) / static_cast<double>(input.rows)};
	std::streamsize defaultprecision{std::cout.precision()};
	std::cout.precision(3);
	if(hRatio < 1.0) {
		if(verbose) {
			std::cout << name << " too large to display, constrained ";
		}
		if(hRatio < vRatio) {
			if(verbose) {
				std::cout << "horizontally. Shrinking by a factor of " << hRatio << '\n';
			}
			cv::resize(input, output, cv::Size(), hRatio, hRatio, cv::INTER_AREA);
		}
		else {
			if(verbose) {
				std::cout << "vertically. Shrinking by a factor of " << vRatio << '\n';
			}
			cv::resize(input, output, cv::Size(), vRatio, vRatio, cv::INTER_AREA);
		}
	}
	else if(vRatio < 1.0) {
		if(verbose) {
			std::cout << name << " too large to display, constrained vertically. Shrinking by a factor of " << vRatio << '\n';
		}
		cv::resize(input, output, cv::Size(), vRatio, vRatio, cv::INTER_AREA);
	}
	std::cout.precision(defaultprecision);
}
#endif

bool Encode(const std::string& base, const std::string& source, const std::string& OutputFilePath) {
	if(verbose) {
		std::cout << "Reading base image\n";
	}
	cv::Mat BaseImage{cv::imread(base, cv::IMREAD_COLOR)};
	if(verbose) {
		std::cout << "Reading source image\n";
	}
	cv::Mat SourceImage{cv::imread(source, cv::IMREAD_COLOR)};
	if(!BaseImage.data) {
		std::cerr << "Error! Cannot open base image. Please check if the path is correct and if the file is an 8 bit "
					 "color image.\n";
		return false;
	}
	if(!SourceImage.data) {
		std::cerr << "Error! Cannot open source image. Please check if the path is correct and if the file is an 8 bit "
					 "color image.\n";
		return false;
	}

	// Using 7 pixels for the trailer (see definition below)
	const unsigned int AvailableBasePixels{static_cast<unsigned int>(BaseImage.rows * BaseImage.cols - 7)};
	const unsigned int TotalBaseChannels{AvailableBasePixels * 3U + 21U};
	unsigned int BitsToEncode{static_cast<unsigned int>(SourceImage.rows * SourceImage.cols * 24)};
	unsigned int BitsPerPixel{BitsToEncode / AvailableBasePixels}; // zero indexed for BPCH, add 1 to get actual value
	if(verbose) {
		std::cout << "Base image size = [" << BaseImage.rows << " x " << BaseImage.cols << " x " << BaseImage.channels() << ']'
				  << '\n';
		std::cout << "Source image size = [" << SourceImage.rows << " x " << SourceImage.cols << " x " << SourceImage.channels()
				  << ']' << "\n\n";
	}

	if(BitsPerPixel >= 12) {
		if(BitsPerPixel >= 288) { // Not reducing beyond 8x and grayscale conversion (for now)
			std::cerr << "Cannot encode without significant loss in visual fidelity. Please choose a larger base image\n";
			return false;
		}
		std::cout << "Base image not large enough, reducing source image\n";
		if(BitsPerPixel < 24) {
			std::cout << "Reducing source image area by 2\n";
			cv::resize(SourceImage, SourceImage, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
		}
		else if(BitsPerPixel < 36) {
			std::cout << "Converting source image to grayscale\n";
			cv::cvtColor(SourceImage, SourceImage, cv::COLOR_BGR2GRAY);
		}
		else if(BitsPerPixel < 48) {
			std::cout << "Reducing source image area by 4\n";
			cv::resize(SourceImage, SourceImage, cv::Size(), 0.25, 0.25, cv::INTER_AREA);
		}
		else if(BitsPerPixel < 72) {
			std::cout << "Reducing source image area by 2 and converting to grayscale\n";
			cv::cvtColor(SourceImage, SourceImage, cv::COLOR_BGR2GRAY);
			cv::resize(SourceImage, SourceImage, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
		}
		else if(BitsPerPixel < 96) {
			std::cout << "Reducing source image area by 8\n";
			cv::resize(SourceImage, SourceImage, cv::Size(), 0.125, 0.125, cv::INTER_AREA);
		}
		else if(BitsPerPixel < 144) {
			std::cout << "Reducing source image area by 4 and converting to grayscale\n";
			cv::cvtColor(SourceImage, SourceImage, cv::COLOR_BGR2GRAY);
			cv::resize(SourceImage, SourceImage, cv::Size(), 0.25, 0.25, cv::INTER_AREA);
		}
		else if(BitsPerPixel < 288) {
			std::cout << "Reducing source image area by 8 and converting to grayscale\n";
			cv::cvtColor(SourceImage, SourceImage, cv::COLOR_BGR2GRAY);
			cv::resize(SourceImage, SourceImage, cv::Size(), 0.125, 0.125, cv::INTER_AREA);
		}
		BitsToEncode = SourceImage.rows * SourceImage.cols * 8 * SourceImage.channels();
		if(verbose) {
			std::cout << "Modified source image size = [" << SourceImage.rows << " x " << SourceImage.cols << " x "
					  << SourceImage.channels() << ']' << "\n\n";
		}
		BitsPerPixel = BitsToEncode / AvailableBasePixels;
	}

	if(showimages) {
		cv::Mat BaseCopy{BaseImage}, SourceCopy{SourceImage};
#if _WIN32
		ResizeToSmall(BaseImage, BaseCopy, "Base Image");
		ResizeToSmall(SourceImage, SourceCopy, "Source Image");
#endif
		cv::namedWindow("Base", cv::WINDOW_AUTOSIZE);
		cv::namedWindow("Source", cv::WINDOW_AUTOSIZE);
		cv::imshow("Source", SourceCopy);
		cv::imshow("Base", BaseCopy);
	}

	if(verbose) {
		std::cout << "Encoding now...\n";
	}

	// const unsigned int RequiredPixels = BitsToEncode / (BitsPerPixel + 1U);
	// Stride between each hiding pixel. Stride = (AvailableBasePixels / RequiredPixels) - 1
	const unsigned int stride{(AvailableBasePixels * (BitsPerPixel + 1U) / BitsToEncode) - 1U};

	/* Trailer Config (1 pixel + 2 channels of another pixel = 40 bits)
	** First 16 bits = number of rows in SourceImage
	** Next bit = SourceImage == Grayscale_Image ? 1 : 0
	** Next 15 bits = number of cols in SourceImage
	** Next 8 bits = as they were
	** Next 8 bits = checksum (see below)
	*/
	std::array<unsigned char, 5> trailer{0, 0, 0, 0, 0};
	trailer[0] = static_cast<unsigned char>((SourceImage.rows / PowersOfTwo[8]) % PowersOfTwo[8]); // bits 0-7
	trailer[1] = static_cast<unsigned char>(SourceImage.rows % PowersOfTwo[8]);					   // bits 8-15
	trailer[2] = static_cast<unsigned char>((SourceImage.channels() == 1 ? PowersOfTwo[7] : 0)
											+ ((SourceImage.cols / PowersOfTwo[8]) % PowersOfTwo[7])); // bits 16-23
	trailer[3] = static_cast<unsigned char>(SourceImage.cols % PowersOfTwo[8]);						   // bits 24-31

	/* Checksum config
	** Checksum is the last channel of 2nd pixel used by the trailer
	** Checksum = XOR(trailer in 8 bit chunks, last channel in BaseImage)
	*/
	trailer[4]
		= static_cast<unsigned char>(trailer[0] ^ trailer[1] ^ trailer[2] ^ trailer[3] ^ BaseImage.data[TotalBaseChannels - 1]);

	// Applying Trailer in 2 bits per channel => 6 pixels and 2 channels for the trailer
	for(unsigned int ch{21}, i{0}, done{0}; ch > 1; --ch, done += 2) {
		if(done == 8U) {
			done = 0U;
			++i;
		}
		BaseImage.data[TotalBaseChannels - ch] -= BaseImage.data[TotalBaseChannels - ch] % PowersOfTwo[2];
		BaseImage.data[TotalBaseChannels - ch]
			+= static_cast<unsigned char>((trailer[i] / PowersOfTwo[6U - done]) % PowersOfTwo[2]);
	}

	const std::array<unsigned int, 3> bpch{BPCH[BitsPerPixel]};
	const unsigned int TotalSourceImageChannels{
		static_cast<unsigned int>(SourceImage.rows * SourceImage.cols * SourceImage.channels())};
	unsigned char *const SourceImageData{SourceImage.data}, *const BaseImageData{BaseImage.data};
	for(unsigned int i{0}, j{0}, TransferredBits{0}, BGR{0}; j < TotalSourceImageChannels; ++BGR, ++i) {
		if(BGR == 3U) {
			BGR = 0U;
			i += stride * 3U;
		}
		unsigned int ChannelBits{bpch[BGR]};
		if(ChannelBits) {
			BaseImageData[i] -= BaseImageData[i] % PowersOfTwo[ChannelBits];
			if(TransferredBits + ChannelBits > 8U) {
				unsigned int NextChannelBits{ChannelBits + TransferredBits};
				NextChannelBits -= 8U;
				BaseImageData[i] += ((SourceImageData[j] % PowersOfTwo[8U - TransferredBits]) * PowersOfTwo[NextChannelBits]);
				++j;
				BaseImageData[i] += (SourceImageData[j] / PowersOfTwo[8U - NextChannelBits]) % PowersOfTwo[NextChannelBits];
				TransferredBits = NextChannelBits;
			}
			else {
				BaseImageData[i]
					+= (SourceImageData[j] / PowersOfTwo[(8U - ChannelBits) - TransferredBits]) % PowersOfTwo[ChannelBits];
				TransferredBits += ChannelBits;
				if(TransferredBits == 8U) {
					TransferredBits = 0U;
					++j;
				}
			}
		}
	}

	if(verbose) {
		std::cout << "Finished encoding\n";
		std::cout << "Saving encoded image\n";
	}

	try {
		cv::imwrite(OutputFilePath, BaseImage);
		std::cout << "Image saved at - " << OutputFilePath << "\n";
	}
	catch(cv::Exception& e) {
		if(e.code == -2) {
			std::cerr << "Error! Cannot save the output file with the given name!\n";
			std::cout << "If this is a privileged directory, please run this application in elevated mode.\n";
			std::cout << "Saving as Encoded.png in the working directory";
			try {
				cv::imwrite("Encoded.png", BaseImage);
				std::cout << "Image saved at - .\\Encoded.png\n\n";
			}
			catch(cv::Exception& E) {
				if(E.code == -2) {
					std::cerr << "Error! Cannot save as Encoded.png as well, skipping save step.\n\n";
				}
			}
		}
	}

	if(showimages) {
#if _WIN32
		ResizeToSmall(BaseImage, BaseImage, "Encoded Image");
#endif
		cv::namedWindow("Encoded Base", cv::WINDOW_AUTOSIZE);
		cv::imshow("Encoded Base", BaseImage);
		cv::waitKey(0);
	}

	return true;
}
}
