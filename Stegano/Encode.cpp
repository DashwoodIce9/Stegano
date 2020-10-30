/* Copyright 2020 Prakhar Agarwal*/

#include "SteganoCommon.h"

/* TODO
** Add functionality to support images with FP16/32 channel types
*/

namespace Stegano {

extern bool expandbase, force, noreduc, nograyscale;

#if _WIN32
extern long DesktopWidth, DesktopHeight;
/**
 * @brief Resizes display images to fit the screen. Resizes only displayed images not the one saved on disk.
 * @param input -> Input image object
 * @param output -> Output image object
 * @param name -> Image name (for verbose logging)
 * @param extrashrinkfactor -> Manual factor by which to shrink the images more than default
 */
void ResizeToSmall(const cv::Mat& input, cv::Mat& output, const std::string& name, double extrashrinkfactor) {
	const double hRatio{extrashrinkfactor * static_cast<double>(DesktopWidth) / static_cast<double>(input.cols)};
	const double vRatio{extrashrinkfactor * static_cast<double>(DesktopHeight) / static_cast<double>(input.rows)};
	auto defaultprecision{std::cout.precision()};
	std::cout.precision(3);
	if(hRatio < 1.0) {
		Stegano::Logger::Verbose(name, " too large to display, constrained ");
		if(hRatio < vRatio) {
			Stegano::Logger::Verbose("horizontally. Shrinking by a factor of ", hRatio, '\n');
			cv::resize(input, output, cv::Size(), hRatio, hRatio, cv::INTER_AREA);
		}
		else {
			Stegano::Logger::Verbose("vertically. Shrinking by a factor of ", vRatio, '\n');
			cv::resize(input, output, cv::Size(), vRatio, vRatio, cv::INTER_AREA);
		}
	}
	else if(vRatio < 1.0) {
		Stegano::Logger::Verbose(name, " too large to display, constrained ", "vertically. Shrinking by a factor of ", vRatio, '\n');
		cv::resize(input, output, cv::Size(), vRatio, vRatio, cv::INTER_AREA);
	}
	std::cout.precision(defaultprecision);
}
#endif

bool Encode(const std::string& base, const std::string& source, const std::string& output) {
	Stegano::Logger::Verbose("Exapnd base = ", expandbase ? "true" : "false", '\n');
	Stegano::Logger::Verbose("No reduction = ", noreduc ? "true" : "false", '\n');
	Stegano::Logger::Verbose("No grayscale = ", nograyscale ? "true" : "false", '\n');
	Stegano::Logger::Verbose("Forced encode = ", force ? "true" : "false", '\n');
	Stegano::Logger::Verbose("Reading base image", '\n');
	cv::Mat BaseImage{cv::imread(base, cv::IMREAD_COLOR)};
	Stegano::Logger::Verbose("Reading source image", '\n');
	cv::Mat SourceImage{cv::imread(source, cv::IMREAD_COLOR)};
	if(!BaseImage.data) {
		Stegano::Logger::Error("Error!", " Cannot open base image.",
							   " Please check if the path is correct and if the file is an 8 bit color image.", '\n');
		return false;
	}
	if(!SourceImage.data) {
		Stegano::Logger::Error("Error!", " Cannot open source image.",
							   " Please check if the path is correct and if the file is an 8 bit color image.", '\n');
		return false;
	}
	if(BaseImage.rows > 65535 || BaseImage.cols > 65535) {
		Stegano::Logger::Error("Error!", " Base image too large.",
							   " Cannot operate on images with dimensions greater than [65536 x 65536].", '\n');
		return false;
	}
	if(SourceImage.rows > 65535 || SourceImage.cols > 65535) {
		Stegano::Logger::Error("Error!", " Source image too large.",
							   " Cannot operate on images with dimensions greater than [65536 x 65536].", '\n');
		return false;
	}

	// Using 7 pixels for the trailer (see definition below)
	unsigned int AvailableBasePixels{static_cast<unsigned int>(BaseImage.rows * BaseImage.cols - 7)};
	unsigned int TotalBaseChannels{AvailableBasePixels * 3U + 21U};
	unsigned int BitsToEncode{static_cast<unsigned int>(SourceImage.rows * SourceImage.cols * 24)};
	unsigned int BitsPerPixel{BitsToEncode / AvailableBasePixels}; // zero indexed for BPCH, add 1 to get actual value

	Stegano::Logger::Verbose("Base image size = [", BaseImage.rows, " x ", BaseImage.cols, " x ", BaseImage.channels(), ']', '\n',
							 "Source image size = [", SourceImage.rows, " x ", SourceImage.cols, " x ", SourceImage.channels(), ']',
							 "\n\n");
	bool overflow{false};

	auto start = std::chrono::high_resolution_clock::now();

	if(BitsPerPixel >= 12U) {
		if(!noreduc) {
			if(expandbase) {
				const unsigned int ExpansionFactor{(BitsToEncode / 12U + 8U) / (static_cast<unsigned int>(BaseImage.rows * BaseImage.cols))
												   + 1U};
				if(ExpansionFactor > 8U) {
					if(!force) {
						Stegano::Logger::Error(
							"Error!", " Cannot encode without significant loss in visual fidelity. Please choose a larger base image",
							'\n');
						return false;
					}
					if(ExpansionFactor > 16U) {
						Stegano::Logger::Log("Base image not large enough, encoding forcefully. Some part of source will be lost", '\n',
											 "Expanding base image area by ", 16, '\n');
						cv::resize(BaseImage, BaseImage, cv::Size(), 4.0, 4.0, cv::INTER_LANCZOS4);
						overflow = true;
					}
					else {
						Stegano::Logger::Log("Forceful encoding, reducing beyond 8x, this may lead to significant loss of quality.", '\n',
											 "Expanding base image area by ", ExpansionFactor, '\n');
						cv::resize(BaseImage, BaseImage, cv::Size(), sqrt(ExpansionFactor), sqrt(ExpansionFactor), cv::INTER_LANCZOS4);
					}
				}
				else {
					Stegano::Logger::Log("Expanding base image area by ", ExpansionFactor, '\n');
					cv::resize(BaseImage, BaseImage, cv::Size(), sqrt(ExpansionFactor), sqrt(ExpansionFactor), cv::INTER_LANCZOS4);
				}
				AvailableBasePixels = static_cast<unsigned int>(BaseImage.rows * BaseImage.cols - 7);
				TotalBaseChannels = AvailableBasePixels * 3U + 21U;
				Stegano::Logger::Verbose("Modified base image size = [", BaseImage.rows, " x ", BaseImage.cols, " x ", BaseImage.channels(),
										 ']', "\n\n");
			}
			else {
				unsigned int ReductionFactor{BitsPerPixel / 12U + 1U};
				if(nograyscale) {
					if(ReductionFactor > 8U) {
						if(!force) {
							Stegano::Logger::Error(
								"Error!", " Cannot encode without significant loss in visual fidelity. Please choose a larger base image",
								'\n');
							return false;
						}
						if(ReductionFactor > 16U) {
							Stegano::Logger::Log("Base image not large enough, encoding forcefully. Some part of source will be lost", '\n',
												 "Reducing source image area by ", 16, '\n');
							cv::resize(SourceImage, SourceImage, cv::Size(), 0.25, 0.25, cv::INTER_AREA);
							overflow = true;
						}
						else {
							Stegano::Logger::Log("Forceful encoding, reducing beyond 8x, this may lead to significant loss of quality.",
												 '\n', "Reducing source image area by ", ReductionFactor, '\n');
							const double ScalingFactor{1.0 / std::sqrt(static_cast<double>(ReductionFactor))};
							cv::resize(SourceImage, SourceImage, cv::Size(), ScalingFactor, ScalingFactor, cv::INTER_AREA);
						}
					}
					else {
						Stegano::Logger::Log("Reducing source image area by ", ReductionFactor, '\n');
						const double ScalingFactor{1.0 / std::sqrt(static_cast<double>(ReductionFactor))};
						cv::resize(SourceImage, SourceImage, cv::Size(), ScalingFactor, ScalingFactor, cv::INTER_AREA);
					}
				}
				else {
					if(ReductionFactor > 24U) {
						if(!force) {
							Stegano::Logger::Error(
								"Error!", " Cannot encode without significant loss in visual fidelity. Please choose a larger base image",
								'\n');
							return false;
						}
						if(ReductionFactor > 48U) {
							Stegano::Logger::Log("Base image not large enough, encoding forcefully. Some part of source will be lost", '\n',
												 "Reducing source image area by ", 16, " and converting to grayscale", '\n');
							cv::cvtColor(SourceImage, SourceImage, cv::COLOR_BGR2GRAY);
							cv::resize(SourceImage, SourceImage, cv::Size(), 0.25, 0.25, cv::INTER_AREA);
							overflow = true;
						}
						else {
							Stegano::Logger::Log("Forceful encoding, reducing beyond 8x, this may lead to significant loss of quality.",
												 '\n', "Reducing source image area by ", ReductionFactor, " and converting to grayscale",
												 '\n');
							cv::cvtColor(SourceImage, SourceImage, cv::COLOR_BGR2GRAY);
							ReductionFactor = ReductionFactor / 3U + 1U;
							const double ScalingFactor = 1.0 / std::sqrt(static_cast<double>(ReductionFactor));
							cv::resize(SourceImage, SourceImage, cv::Size(), ScalingFactor, ScalingFactor, cv::INTER_AREA);
						}
					}
					else {
						if(ReductionFactor > 3U) {
							ReductionFactor = ReductionFactor / 3U + 1U;
							const double ScalingFactor{1.0 / std::sqrt(static_cast<double>(ReductionFactor))};
							Stegano::Logger::Log("Reducing source image area by ", ReductionFactor, " and converting to grayscale", '\n');
							cv::cvtColor(SourceImage, SourceImage, cv::COLOR_BGR2GRAY);
							cv::resize(SourceImage, SourceImage, cv::Size(), ScalingFactor, ScalingFactor, cv::INTER_AREA);
						}
						else {
							Stegano::Logger::Log("Converting source image to grayscale", '\n');
							cv::cvtColor(SourceImage, SourceImage, cv::COLOR_BGR2GRAY);
						}
					}
				}
				BitsToEncode = SourceImage.rows * SourceImage.cols * 8 * SourceImage.channels();
				Stegano::Logger::Verbose("Modified source image size = [", SourceImage.rows, " x ", SourceImage.cols, " x ",
										 SourceImage.channels(), ']', "\n\n");
			}
		}
		else {
			if(!force) {
				Stegano::Logger::Error("Error!",
									   " Image reduction is disabled and base image is not large enough to store the source image", '\n');
				Stegano::Logger::Log("Rerun without \"noreduc\" flag or choose a larger base image", '\n');
				return false;
			}
			overflow = true;
		}
		BitsPerPixel = overflow ? 11U : BitsToEncode / AvailableBasePixels;
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
		cv::waitKey(0);
		cv::destroyAllWindows();
	}

	Stegano::Logger::Verbose("Encoding now...", '\n');

	// const unsigned int RequiredPixels = BitsToEncode / (BitsPerPixel + 1U);
	// Stride between each hiding pixel. Stride = (AvailableBasePixels / RequiredPixels) - 1
	const unsigned int stride{overflow ? 0U : (AvailableBasePixels * (BitsPerPixel + 1U) / BitsToEncode) - 1U};

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
	trailer[4] = static_cast<unsigned char>(trailer[0] ^ trailer[1] ^ trailer[2] ^ trailer[3] ^ BaseImage.data[TotalBaseChannels - 1]);

	// Applying Trailer in 2 bits per channel => 6 pixels and 2 channels for the trailer
	for(unsigned int ch{21}, i{0}, done{0}; ch > 1; --ch, done += 2) {
		if(done == 8U) {
			done = 0U;
			++i;
		}
		BaseImage.data[TotalBaseChannels - ch] -= BaseImage.data[TotalBaseChannels - ch] % PowersOfTwo[2];
		BaseImage.data[TotalBaseChannels - ch] += static_cast<unsigned char>((trailer[i] / PowersOfTwo[6U - done]) % PowersOfTwo[2]);
	}

	const std::array<unsigned int, 3> bpch{BPCH[BitsPerPixel]};
	const unsigned int TotalSourceChannels{static_cast<unsigned int>(SourceImage.rows * SourceImage.cols * SourceImage.channels())};
	unsigned char *const SourceImageData{SourceImage.data}, *const BaseImageData{BaseImage.data};
	for(unsigned int i{0}, j{0}, TransferredBits{0}, BGR{0}; j < TotalSourceChannels && i < TotalBaseChannels - 21U; ++BGR, ++i) {
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
				BaseImageData[i] += (SourceImageData[j] / PowersOfTwo[(8U - ChannelBits) - TransferredBits]) % PowersOfTwo[ChannelBits];
				TransferredBits += ChannelBits;
				if(TransferredBits == 8U) {
					TransferredBits = 0U;
					++j;
				}
			}
		}
	}

	Stegano::Logger::Verbose("Finished encoding", '\n', "Saving encoded image", '\n');

	try {
		cv::imwrite(output, BaseImage,
					std::vector<int>{cv::IMWRITE_PNG_COMPRESSION, 4, cv::IMWRITE_PNG_STRATEGY, cv::IMWRITE_PNG_STRATEGY_FILTERED});
		Stegano::Logger::Log("Image saved at - ", output, '\n');
	}
	catch(cv::Exception& e) {
		if(e.code == -2) {
			Stegano::Logger::Error("Error!", " Cannot save the output file with the given name!", '\n');
			Stegano::Logger::Log("If this is a privileged directory, please run this application in elevated mode.", '\n',
								 "Saving as Encoded.png in the working directory");
			try {
				cv::imwrite("Encoded.png", BaseImage);
				Stegano::Logger::Log("Image saved at - .\\Encoded.png", '\n');
			}
			catch(cv::Exception& E) {
				if(E.code == -2) {
					Stegano::Logger::Error("Error!", " Cannot save as Encoded.png as well, skipping save step.", '\n');
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

	if(!showimages) {
		auto end = std::chrono::high_resolution_clock::now();
		const double timetaken = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000.0;
		Stegano::Logger::Verbose('\n', "Encoding took: ", timetaken, " seconds");
	}

	return true;
}

}
