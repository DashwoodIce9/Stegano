/* Copyright 2020 Prakhar Agarwal*/

#include <string>
#include <limits>
#include <thread>
#include <chrono>
#include "SteganoLogger.h"

#if _WIN32
	#define NOMINMAX // to protect from conflict in std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n')
	#include <Windows.h>
#endif

namespace Stegano {
bool quiet{false}, verbose{false}, showimages{false}, expandbase{false}, force{false}, noreduc{false}, nograyscale{false};
unsigned int threads{1U};

#if _WIN32
long DesktopWidth{0}, DesktopHeight{0};
#endif

/**
 * @brief Encodes source image in base
 * @param base -> Base image path
 * @param source -> Source image path
 * @param output -> Output image path
 * @param expandbase -> Expands base, does not shrink source during reduction phase
 * @param force -> Force encode even if base is not large enough
 * @param noreduc -> Skip reduction phase
 * @param nograyscale -> Disables conversion to grayscale during reduction phase
 * @return true => Success
 */
inline bool Encode(const std::string& base, const std::string& source, const std::string& output);
/**
 * @brief Decodes the hidden image in source image
 * @param source -> Source image path
 * @param output -> Output image path
 * @return true => Success
 */
inline bool Decode(const std::string& source, const std::string& output);
inline bool ParallelEncode(const std::string& base, const std::string& source, const std::string& output);
inline bool ParallelDecode(const std::string& source, const std::string& output);

// Hold Screen
static inline void hold() {
	std::cout << "Press enter/return to exit";
	std::cin.get();
}

// Display help
static inline void help() {
	std::cout << '\n';
	std::cout << "------------------------------------------------------ Help ------------------------------------------------------"
			  << "\n\n";
	std::cout << "NAME"
			  << "\n\t"
			  << "Image Steganography Tool"
			  << "\n\n";
	std::cout << "SYNTAX"
			  << "\n\t"
			  << "Stegano.exe {help | /h | /H} | {[{encode | /e | /E} <base> <source>] | [{decode | /d | /D} <source>]}"
			  << "\n\t"
			  << "[{output | /o | /O} <path>] {quiet | /q | /Q} {verbose | /v | /V} {show | /s | /S} {noreduc | /nr | /NR}"
			  << "\n\t"
			  << "{force | /f | /F} {nogray | /ng | /NG} {base | /b | /B} [{threads | /t | /T} 1...512]"
			  << "\n\n";
	std::cout << "DESCRIPTION"
			  << "\n\t"
			  << "Steganography is the practice of concealing messages or information within other non-secret text or data."
			  << "\n\t"
			  << "The advantage of steganography over cryptography is that the secret message does not attract attention to"
			  << "\n\t"
			  << "itself. It's the benefit of hiding in plain sight. This application hides images within images."
			  << "\n\n\t";
	std::cout << "------------------------------------------------- Commands ------------------------------------------------"
			  << "\n\n\t";
	std::cout << "a) help (optional) - Display the help text and terminate. "
			  << "\n\n\t";
	std::cout << "b) encode - Hides the source image within the base image."
			  << "\n\t\t"
			  << "e.g. - Stegano.exe encode ..\\Base.png ..\\Source.png"
			  << "\n\n\t";
	std::cout << "c) decode - Decodes the hidden image within the source image (must be a losslessly encoded image)."
			  << "\n\t\t"
			  << "e.g. - Stegano.exe decode ..\\Encoded.png"
			  << "\n\n\t";
	std::cout << "------------------------------------------------- Flags --------------------------------------------------"
			  << "\n\n\t";
	std::cout << "1) output (optional, default = Encoded.png / Decoded.png) - Sets the output image path. Must end with .png"
			  << "\n\t\t"
			  << "extension. e.g. - Stegano.exe encode ..\\Base.png ..\\Source.png output ..\\Output.png"
			  << "\n\n\t";
	std::cout << "2) quiet (optional) - No output to console except for error messages."
			  << "\n\n\t";
	std::cout << "3) verbose (optional, disables => quiet) - Turns on verbose logging."
			  << "\n\n\t";
	std::cout << "4) show (optional) - Display the source and output images. If the images are too large to be displayed,"
			  << "\n\t\t"
			  << "they will be shrunk to fit the screen. The output image is not shrunk."
			  << "\n\n\t";
	std::cout << "5) noreduc (optional, disables => base) - Disables reduction of the source and expansion of the base image"
			  << "\n\t\t"
			  << "during encoding. Throws error if the base is not large enough and \"force\" is not set."
			  << "\n\n\t";
	std::cout << "6) force (optional) - Force encode even if the base image cannot store the source image completely."
			  << "\n\t\t"
			  << "A portion of source will be lost in this process."
			  << "\n\n\t";
	std::cout << "7) nogray (optional) - Disables grayscale conversion during reduction process."
			  << "\n\n\t";
	std::cout << "8) base (optional, disables => gray) - Apply expanding transformation on the base image if it is not large"
			  << "\n\t\t"
			  << "enough to store the source image. The source will not be shrunk."
			  << "\n\n\t";
	std::cout << "9) threads (optional, default = 1, max = 512) - Enable multithreading with the specified number of threads."
			  << "\n\t\t"
			  << "Should be the last argument passed. e.g. - Stegano.exe decode ..\\Encoded.png threads 8"
			  << "\n\n";
	std::cout << "AUTHOR"
			  << "\n\t"
			  << "DashwoodIce9" << '\n';
}

// Display invalid arguments error
static inline void invalidargs() {
	Stegano::Logger::Error("Error!", " Invalid arguments passed!", "\n\n");
	Stegano::Logger::Log("To encode, run - \"Stegano.exe encode <base> <source>\"", '\n',
						 "To decode, run - \"Stegano.exe decode <source>\"", "\n\n",
						 "Run - \"Stegano.exe help\" for a full list of commands.", '\n');
}

/**
 * @brief Loops through the command line arguments passed and sets the global variables controlling program execution
 * @param start -> Position to start the loop from, different for Encode and Decode modes
 * @param argc -> Argument count
 * @param argv -> Argument vector
 * @param output -> Sets output file path string
 * @param expandbase -> Sets expandbase boolean
 * @param force -> Sets force boolean
 * @param noreduc -> Sets noreduc boolean
 * @param nograyscale -> Sets nograyscale boolean
 * @return true => Success
 */
static inline bool LoopThroughArgs(const int start, const int& argc, const char** argv, std::string* output) {
	for(int i = start; i < argc; ++i) {
		if(std::string(argv[i]) == "/o" || std::string(argv[i]) == "/O" || std::string(argv[i]) == "output") {
			++i;
			if(i < argc) {
				std::string ext{argv[i]};
				ext = ext.substr(ext.find_last_of('.') + 1);
				if(ext == "png" || ext == "PNG") {
					*output = argv[i];
				}
				else {
					Stegano::Logger::Log('\n', "Given output path - \"", argv[i], "\" does not end in .png, reverting to default.", '\n');
				}
			}
			else {
				Stegano::Logger::Log('\n', "Output file path not found", '\n');
				return false;
			}
		}
		else if(std::string(argv[i]) == "/q" || std::string(argv[i]) == "/Q" || std::string(argv[i]) == "quiet") {
			quiet = true;
		}
		else if(std::string(argv[i]) == "/v" || std::string(argv[i]) == "/V" || std::string(argv[i]) == "verbose") {
			verbose = true;
		}
		else if(std::string(argv[i]) == "/s" || std::string(argv[i]) == "/S" || std::string(argv[i]) == "show") {
			showimages = true;
		}
		else if(std::string(argv[i]) == "/ng" || std::string(argv[i]) == "/NG" || std::string(argv[i]) == "nograyscale") {
			nograyscale = true;
		}
		else if(std::string(argv[i]) == "/f" || std::string(argv[i]) == "/F" || std::string(argv[i]) == "force") {
			force = true;
		}
		else if(std::string(argv[i]) == "/nr" || std::string(argv[i]) == "/NR" || std::string(argv[i]) == "noreduc") {
			noreduc = true;
		}
		else if(std::string(argv[i]) == "/t" || std::string(argv[i]) == "/T" || std::string(argv[i]) == "threads") {
			++i;
			if(i < argc) {
				try {
					threads = static_cast<unsigned int>(std::stoi(argv[i]));
				}
				catch(...) {
					Stegano::Logger::Log('\n', "Improper value for threads passed, defaulting to single threaded operation.", '\n');
					threads = 1;
				}
				if(threads < 1U || threads > 512U) {
					Stegano::Logger::Log('\n', "Improper value for threads passed, defaulting to single threaded operation.", '\n');
					threads = 1;
				}
			}
			else {
				threads = 0U;
			}
		}
		else if(std::string(argv[i]) == "/b" || std::string(argv[i]) == "/B" || std::string(argv[i]) == "base") {
			expandbase = true;
		}
		else {
			return false;
		}
	}
	return true;
}

/**
 * @brief Handler of program execution, decides mode (interactive, command line), deduces flags, calls Encode() or Decode()
 * @param argc -> Argument count
 * @param argv -> Argument vector
 * @return true => Success
 */
static inline bool handler(const int& argc, const char** argv) {
	bool decode{false};
	std::string Base, Source, output{"Encoded.png"};

	if(argc > 1) {
		if(std::string(argv[1]) == "/h" || std::string(argv[1]) == "/H" || std::string(argv[1]) == "help") {
			help();
			return true;
		}
		if(argc > 2) {
			Stegano::Logger::Log('\n', "\t\tImage Steganography Tool ", "(command line mode)", "\n\n");
			if(std::string(argv[1]) == "/D" || std::string(argv[1]) == "/d" || std::string(argv[1]) == "decode") {
				decode = true;
				Source = argv[2];
				output = "Decoded.png";
				if(!LoopThroughArgs(3, argc, argv, &output)) {
					invalidargs();
					return false;
				}
			}
			else if(argc > 3) {
				if(!(std::string(argv[1]) == "/E" || std::string(argv[1]) == "/e" || std::string(argv[1]) == "encode")) {
					invalidargs();
					return false;
				}
				Base = argv[2];
				Source = argv[3];
				if(!LoopThroughArgs(4, argc, argv, &output)) {
					invalidargs();
					return false;
				}
			}
			else {
				invalidargs();
				return false;
			}
		}
		else {
			invalidargs();
			return false;
		}
	}
	else {
		std::cout << '\n'
				  << "\t\tImage Steganography Tool "
				  << "(interactive mode)"
				  << "\n\n";
		std::cout << "Press -" << '\n'
				  << "E - to Encode" << '\n'
				  << "D - to Decode" << '\n'
				  << "H - to view Help page"
				  << "\n\n"
				  << "Input: ";
		unsigned char in;
		std::cin >> in;
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		if(in == 'e' || in == 'E') {
			std::cout << '\n' << "Encode Mode" << '\n';
			std::cout << "Enter the Base Image path: ";
			std::getline(std::cin, Base);
			std::cout << "Enter the Source Image path: ";
			std::getline(std::cin, Source);
		}
		else if(in == 'd' || in == 'D') {
			output = "Decoded.png";
			decode = true;
			std::cout << '\n' << "Decode Mode" << '\n';
			std::cout << "Enter the source image path: ";
			std::getline(std::cin, Source);
		}
		else if(in == 'h' || in == 'H') {
			std::cout << "\n\n";
			help();
			return true;
		}
		else {
			Stegano::Logger::Error('\n', "Invalid input!", '\n');
			return false;
		}
	}

	std::cout << '\n';

#if _WIN32
	RECT desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	DesktopWidth = desktop.right;
	DesktopHeight = desktop.bottom;
	Stegano::Logger::Verbose("Screen resolution is: [", DesktopWidth, " x ", DesktopHeight, ']', '\n');
#endif

	Stegano::Logger::Verbose("Output file path = ", output, '\n');
	showimages ? Stegano::Logger::Verbose("Show Images = ", "true", '\n') : Stegano::Logger::Verbose("Show Images = ", "false", '\n');

	if(threads == 0U) {
		threads = std::thread::hardware_concurrency();
	}

	if(threads == 1U) {
		if(decode ? !Decode(Source, output) : !Encode(Base, Source, output)) {
			return false;
		}
	}
	else {
		if(threads > std::thread::hardware_concurrency()) {
			threads = std::thread::hardware_concurrency();
			Stegano::Logger::Log('\n',
								 "Entered value of threads greater than supported by the platform. Setting thread count to maximum value "
								 "this platform allows.",
								 '\n');
		}
		Stegano::Logger::Verbose("Thread count = ", threads, "\n\n");
		if(decode ? !ParallelDecode(Source, output) : !ParallelEncode(Base, Source, output)) {
			return false;
		}
	}

	return true;
}

/**
 * @brief Calls handler(), finishes program execution
 * @param argc -> Argument count
 * @param argv -> Argument vector
 * @return 0 => Successful execution, 1 => Failure in execution
 */
int run(const int& argc, const char** argv) {
	auto start = std::chrono::steady_clock::now();
	if(!handler(argc, argv)) {
		if(argc == 1) {
			hold();
		}
		return 1;
	}

	// For successful run of interactive mode
	if(argc == 1) {
		hold();
	}
	else {
		std::cout << '\n';
	}
	if(!showimages) {
		auto end = std::chrono::steady_clock::now();
		const double timetaken = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000.0;
		Stegano::Logger::Verbose("Total execution took: ", timetaken, " seconds", "\n\n");
	}
	return 0;
}
}
