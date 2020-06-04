#include "SteganoThreadedCommon.h"

namespace Stegano {

extern bool base, force, noreduc, nograyscale;

bool ParallelEncode(const std::string& base, const std::string& source, const std::string& output, const bool& expandbase,
					const bool& force, const bool& noreduc, const bool& nograyscale) {
	Stegano::Logger::Verbose("Exapnd base = ", expandbase ? "true" : "false", '\n');
	Stegano::Logger::Verbose("No reduction = ", noreduc ? "true" : "false", '\n');
	Stegano::Logger::Verbose("No grayscale = ", nograyscale ? "true" : "false", '\n');
	Stegano::Logger::Verbose("Forced encode = ", force ? "true" : "false", '\n');

	cv::Mat BaseImage, SourceImage;
	{
		std::thread loadbase([&base, &BaseImage] {
			Stegano::Logger::Verbose("Reading base image", '\n');
			BaseImage = cv::imread(base, cv::IMREAD_COLOR);
		});
		std::thread loadsource([&source, &SourceImage] {
			Stegano::Logger::Verbose("Reading source image", '\n');
			SourceImage = cv::imread(source, cv::IMREAD_COLOR);
		});
		loadbase.join();
		loadsource.join();
	}

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
		}
		BitsPerPixel = overflow ? 11U : BitsToEncode / AvailableBasePixels;
	}

	std::thread display([&BaseImage, &SourceImage] {
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
	});

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

	std::vector<std::thread> thread_pool;
	thread_pool.reserve(threads);
	for(unsigned int k{0}; k < threads; ++k) {
		thread_pool.emplace_back(
			std::thread([k, &stride, &bpch, &BitsPerPixel, &BaseImageData, &SourceImageData, &TotalSourceChannels, &TotalBaseChannels] {
				// i = index pointing to Base Image
				// j = index pointing to Source Image
				// BGR = Active channel (Blue, Green, Red)
				// TransferredBits = Number of bits transferred which do not make up a full byte, see the encoding loop for details
				unsigned int i{0}, j{0}, TransferredBits{0}, BGR{0};
				if(k > 0) {
					// Number of iterations performed before the new loop
					const unsigned int n{((TotalBaseChannels - 21U) / threads) * k};
					i = n;
					// Jump made by a stride in channels
					const unsigned int stridechjump{(stride + 1U) * 3U};
					// Total bits transferred by previous loops
					unsigned int tbt{0};
					if(stride != 0U) {
						// If i points to a channel which will encode bits in it, the following calculation will return a value in [0, 3)
						// If it will not encode bits in it, the value will be >= 3
						BGR = i % stridechjump;
						if(BGR > 2U) {
							// Jumping to the next suitable pixel for encoding
							i += stridechjump - BGR;
							// obviously moving to next fresh pixel implies we are on its first channel, hence BGR = 0
							BGR = 0U;
						}
						// Number of pixels traversed by previous loops = first pixel + number of strides taken
						unsigned int pixelsdone = n / (stridechjump);
						// If fully divisible => first pixel is already counted, else need to add it separately
						if(n % stridechjump != 0U) {
							++pixelsdone;
						}
						// Total bits transferred = (number of pixels) * (bits in each pixel)
						tbt = pixelsdone * (BitsPerPixel + 1U);
					}
					else {
						// No strides, all encoding pixels are together
						BGR = i % 3U;
						tbt = (i / 3U) * (BitsPerPixel + 1U);
					}
					// BGR > 0 => Previous loop encoded extra channels after fully encoded pixels
					if(BGR == 1U) {
						tbt += bpch[0];
					}
					else if(BGR == 2U) {
						tbt += bpch[0] + bpch[1];
					}
					// Number of bytes transferred
					j = tbt / 8U;
					TransferredBits = tbt % 8U;
				}
				for(; j < TotalSourceChannels
					  && i < ((k == threads - 1U) ? (TotalBaseChannels - 21U) : ((TotalBaseChannels - 21U) / threads * (k + 1U)));
					++BGR, ++i) {
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
			}));
	}
	for(unsigned int k{0}; k < threads; ++k) {
		thread_pool[k].join();
	}

	std::thread saveimage([&output, &BaseImage] {
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
	});

	display.join();
	if(showimages) {
#if _WIN32
		ResizeToSmall(BaseImage, BaseImage, "Encoded Image");
#endif
		cv::namedWindow("Encoded Base", cv::WINDOW_AUTOSIZE);
		cv::imshow("Encoded Base", BaseImage);
		cv::waitKey(0);
	}

	saveimage.join();

	if(!showimages) {
		auto end = std::chrono::high_resolution_clock::now();
		const double timetaken = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000.0;
		Stegano::Logger::Verbose('\n', "Encoding took: ", timetaken, " seconds");
	}

	return true;
}

}
