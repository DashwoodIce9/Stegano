#include <iostream>
#include <string>
#include <limits>

#if _WIN32
	#define NOMINMAX // to protect from conflict in std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n')
	#include <Windows.h>
#endif

namespace Stegano {
extern bool verbose{false}, showimages{false};
extern unsigned int threads{0};

#if _WIN32
extern long DesktopWidth{0}, DesktopHeight{0};
#endif

bool Encode(const std::string& base, const std::string& source, const std::string& OutputFilePath);
bool Decode(const std::string& source, const std::string& OutputFilePath);

void hold() {
	std::cout << "Press enter/return to exit";
	std::cin.get();
}

static void help() {
	std::cout << "------------------------------------------------------ Help "
				 "------------------------------------------------------\n\n";
	std::cout << "NAME\n\tImage Steganography Tool\n\n";
	std::cout << "SYNTAX\n\tStegano.exe {{help | /h | /H} | {{encode | /e | /E} <base> <source>} | {{decode | /d | /D}"
				 "<source>}}\n\t{{output | /o | /O} <path>} {verbose | /v | /V} {show | /s | /S} {{threads | /t | /T} "
				 "1...512}\n\n";
	std::cout << "DESCRIPTION\n\tSteganography is the practice of concealing messages or information within other"
				 "non-secret text or data.\n\tThis application hides images within images.\n\n\t";
	std::cout << "help (optional) - Display the help text and terminate.\n\n\t";
	std::cout << "encode - Hides source file inside base.\n\t\te.g. - Stegano.exe ..\\Base.png ..\\Source.png\n\n\t";
	std::cout << "decode - Decodes the data embedded in source file (must be a losslessly encoded file like .png or .tif).\n\t\t"
				 "e.g. - Stegano.exe ..\\Encoded.png\n\n\t";
	std::cout << "output (optional, default = Encoded.png) - Sets the output image path. Must end with .png extension.\n\t\t"
				 "If the directory requires privileged access, the application must be run with elevated permissions.\n\t\t"
				 "e.g. - Stegano.exe encrypt ..\\Base.png ..\\Source.png output ..\\Output.png\n\n\t";
	std::cout << "verbose (optional) - Turns on verbose mode\n\n\t";
	std::cout << "show (optional) - Display the source image(s) and the output image.\n\n\t";
	std::cout << "threads (optional, default = 1) - Enable multithreaded operation with the specified number of threads.\n\t\t"
				 "e.g. - Stegano.exe decode ..\\Encoded.png threads 8\n\n";
	std::cout << "AUTHOR\n\tDashwoodIce9\n\n";
}

static bool LoopThroughArgs(const int start, const int& argc, const char** argv, std::string* OutputFilePath,
							bool* multithreading) {
	bool output{false};
	for(int i = start; i < argc; ++i) {
		if(!output && (std::string(argv[i]) == "/o" || std::string(argv[i]) == "/O" || std::string(argv[i]) == "output")) {
			output = true;
			*OutputFilePath = argv[i + 1];
			++i;
		}
		else if(!verbose && (std::string(argv[i]) == "/v" || std::string(argv[i]) == "/V" || std::string(argv[i]) == "verbose")) {
			verbose = true;
		}
		else if(!showimages && (std::string(argv[i]) == "/s" || std::string(argv[i]) == "/S" || std::string(argv[i]) == "show")) {
			showimages = true;
		}
		else if(!(*multithreading)
				&& (std::string(argv[i]) == "/t" || std::string(argv[i]) == "/T" || std::string(argv[i]) == "threads")) {
			*multithreading = true;
			threads = static_cast<unsigned int>(std::stoi(argv[i + 1]));
			if(threads == 0 || threads > 512) {
				std::cout << "\nImproper value for threads passed, defaulting to single threaded operation.\n";
				*multithreading = false;
			}
			++i;
		}
		else {
			return true;
		}
	}
	return false;
}

int handler(const int& argc, const char** argv) {
	bool decode{false}, multithreading{false};
	std::string Base, Source, OutputFilePath{"Encoded.png"};

	if(argc > 1) {
		std::cout << "\n\t\tImage Steganography Tool (command line mode)\n\n";
		if(std::string(argv[1]) == "/h" || std::string(argv[1]) == "/H" || std::string(argv[1]) == "help") {
			help();
			return 0;
		}
		if(argc > 2) {
			if(std::string(argv[1]) == "/D" || std::string(argv[1]) == "/d" || std::string(argv[1]) == "decode") {
				decode = true;
				Source = argv[2];
				OutputFilePath = "Decoded.png";
				if(LoopThroughArgs(3, argc, argv, &OutputFilePath, &multithreading)) {
					return 1;
				}
			}
			else if(argc > 3) {
				if(!(std::string(argv[1]) == "/E" || std::string(argv[1]) == "/e" || std::string(argv[1]) == "encode")) {
					return 1;
				}
				Base = argv[2];
				Source = argv[3];
				if(LoopThroughArgs(4, argc, argv, &OutputFilePath, &multithreading)) {
					return 1;
				}
			}
			else {
				return 1;
			}
		}
		else {
			return 1;
		}
	}
	else {
		std::cout << "\n\t\tImage Steganography Tool (interactive mode)\n\n";
		std::cout << "Press -\nE - for Encoding\nD - for Decoding\nH - for Help\n\nInput: ";
		unsigned char in;
		std::cin >> in;
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		if(in == 'e' || in == 'E') {
			std::cout << "\nEncode Mode\n";
			std::cout << "Enter the Base Image path: ";
			std::getline(std::cin, Base);
			std::cout << "Enter the Source Image path: ";
			std::getline(std::cin, Source);
		}
		else if(in == 'd' || in == 'D') {
			OutputFilePath = "Decoded.png";
			decode = true;
			std::cout << "\nDecode Mode\n";
			std::cout << "Enter the source image path: ";
			std::getline(std::cin, Source);
		}
		else if(in == 'h' || in == 'H') {
			std::cout << "\n\n";
			help();
			return 2;
		}
		else {
			std::cerr << "\nInvalid input!\n";
			return 3;
		}
	}

	std::cout << '\n';

#if _WIN32
	RECT desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	DesktopWidth = desktop.right;
	DesktopHeight = desktop.bottom;
	if(verbose) {
		std::cout << "Screen resolution is: [" << DesktopWidth << " x " << DesktopHeight << "]\n\n";
	}
#endif

	// Add multithreading code

	if(decode ? !Decode(Source, OutputFilePath) : !Encode(Base, Source, OutputFilePath)) {
		return 4;
	}

	return 0;
}
}
