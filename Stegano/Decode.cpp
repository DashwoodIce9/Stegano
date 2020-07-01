/* Copyright 2020 Prakhar Agarwal*/

#include "SteganoCommon.h"

namespace Stegano {

bool Decode(const std::string& source, const std::string& output) {
	Stegano::Logger::Verbose("Reading source image", '\n');

	cv::Mat SourceImage{cv::imread(source, cv::IMREAD_COLOR)};
	if(!SourceImage.data) {
		Stegano::Logger::Error("Error!", " Cannot open base image.",
							   " Please check if the path is correct and if the file is an 8 bit color image.", '\n');
		return false;
	}
	Stegano::Logger::Verbose("Source image size = [", SourceImage.rows, " x ", SourceImage.cols, " x ", SourceImage.channels(), ']',
							 "\n\n");

	const unsigned int AvailableBasePixels{static_cast<unsigned int>(SourceImage.rows * SourceImage.cols - 7)};
	const unsigned int TotalSourceChannels{AvailableBasePixels * 3U + 21U};

	// Reading Trailer
	std::array<unsigned char, 5> trailer{0, 0, 0, 0, 0};
	for(unsigned int ch{21}, i{0}, done{0}; ch > 1; --ch, done += 2) {
		if(done == 8U) {
			done = 0U;
			++i;
		}
		trailer[i] *= PowersOfTwo[2];
		trailer[i] += static_cast<unsigned char>(SourceImage.data[TotalSourceChannels - ch] % 4U);
	}

	// Checking validity of the trailer
	if((trailer[0] ^ trailer[1] ^ trailer[2] ^ trailer[3] ^ SourceImage.data[TotalSourceChannels - 1]) != trailer[4]) {
		Stegano::Logger::Error("Error!", " The given image does not have any data embedded using this application", '\n');
		return false;
	}

	if(showimages) {
		cv::Mat SourceCopy{SourceImage};
#if _WIN32
		ResizeToSmall(SourceImage, SourceCopy, "Source Image");
#endif
		cv::namedWindow("Source", cv::WINDOW_AUTOSIZE);
		cv::imshow("Source", SourceCopy);
		cv::waitKey(0);
		cv::destroyAllWindows();
	}

	auto start = std::chrono::high_resolution_clock::now();

	Stegano::Logger::Verbose("Encoded image found, decoding...", '\n');

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
	unsigned int BitsPerPixel{BitsEncoded / AvailableBasePixels};
	unsigned int stride{(AvailableBasePixels * (BitsPerPixel + 1U) / BitsEncoded) - 1U};
	if(BitsPerPixel >= 12U) {
		BitsPerPixel = 11U;
		stride = 0U;
	}
	const std::array<unsigned int, 3> bpch{BPCH[BitsPerPixel]};
	unsigned char *const DecodedImageData{DecodedImage.data}, *const SourceImageData{SourceImage.data};

	// Extracting Encoded bits
	for(unsigned int i{0}, j{0}, TransferredBits{0}, BGR{0}; i < TotalDecodedImageChannels && j < TotalSourceChannels - 21U; ++BGR, ++j) {
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

	Stegano::Logger::Verbose("Finished decoding", '\n', "Saving decoded image", '\n');

	try {
		cv::imwrite(output, DecodedImage,
					std::vector<int>{cv::IMWRITE_PNG_COMPRESSION, 4, cv::IMWRITE_PNG_STRATEGY, cv::IMWRITE_PNG_STRATEGY_FILTERED});
		Stegano::Logger::Log("Image saved at - ", output, '\n');
	}
	catch(cv::Exception& e) {
		if(e.code == -2) {
			Stegano::Logger::Error("Error!", " Cannot save the output file with the given name!", '\n');
			Stegano::Logger::Log("If this is a privileged directory, please run this application in elevated mode.", '\n',
								 "Saving as Decoded.png in the working directory");
			try {
				cv::imwrite("Decoded.png", DecodedImage);
				Stegano::Logger::Log("Image saved at - .\\Decoded.png", '\n');
			}
			catch(cv::Exception& E) {
				if(E.code == -2) {
					Stegano::Logger::Error("Error!", " Cannot save as Decoded.png as well, skipping save step.", '\n');
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

	if(!showimages) {
		auto end = std::chrono::high_resolution_clock::now();
		const double timetaken = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000.0;
		Stegano::Logger::Verbose('\n', "Decoding took: ", timetaken, " seconds");
	}

	return true;
}

}
