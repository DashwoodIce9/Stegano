#include <iostream>
#include <cstring>
#include <limits>

bool Encode(std::string base, std::string source, std::string OutputFilePath, bool showimages, bool verbose);
bool Decode(std::string encoded, std::string OutputFilePath, bool showimages, bool verbose);

bool output{false}, verbose{false}, multithreading{false}, showimages{false};
std::string OutputFilePath{"Encoded.png"};
unsigned int threads{0};

void help() {
	std::cout << "------------------------------------------------------ Help "
				 "------------------------------------------------------\n\n";
	std::cout << "NAME\n\tImage Steganography Tool\n\n";
	std::cout << "SYNTAX\n\tStegano.exe {{help | /h | /H} | {{encode | /e | /E} <base> <source>} | {{decode | /d | /D}"
				 "<encoded>}}\n\t{{output | /o | /O} <path>} {verbose | /v | /V} {show | /s | /S} {{threads | /t | /T} "
				 "1...512}\n\n";
	std::cout << "DESCRIPTION\n\tSteganography is the practice of concealing messages or information within other"
				 "non-secret text or data.\n\tThis application hides images within images.\n\n\t";
	std::cout << "help (optional) - Display the help text and terminate.\n\n\t";
	std::cout << "encode - Hides source file inside base.\n\t\te.g. - Stegano.exe ..\\Base.png ..\\Source.png\n\n\t";
	std::cout << "decode - Decodes the encoded file (must be a losslessly encoded file like .png or .tif).\n\t\t"
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

bool LoopThroughArgs(int start, const int& argc, const char* argv[]) {
	for(int i = start; i < argc; ++i) {
		if(!output && (!strcmp(argv[i], "/o") || !strcmp(argv[i], "/O") || !strcmp(argv[i], "output"))) {
			output = true;
			OutputFilePath = argv[i + 1];
			++i;
		}
		else if(!verbose && (!strcmp(argv[i], "/v") || !strcmp(argv[i], "/V") || !strcmp(argv[i], "verbose"))) {
			verbose = true;
		}
		else if(!showimages && (!strcmp(argv[i], "/s") || !strcmp(argv[i], "/S") || !strcmp(argv[i], "show"))) {
			showimages = true;
		}
		else if(!multithreading && (!strcmp(argv[i], "/t") || !strcmp(argv[i], "/T") || !strcmp(argv[i], "threads"))) {
			multithreading = true;
			threads = static_cast<unsigned int>(atoi(argv[i + 1]));
			++i;
		}
		else {
			return true;
		}
	}
	return false;
}

int handler(const int& argc, const char* argv[]) {
	bool decode{false};
	std::string Base, Source;
	if(argc > 1) {
		if(!strcmp(argv[1], "/h") || !strcmp(argv[1], "/H") || !strcmp(argv[1], "help")) {
			help();
			return 0;
		}
		else if(argc > 2) {
			if(!strcmp(argv[1], "/D") || !strcmp(argv[1], "/d") || !strcmp(argv[1], "decode")) {
				decode = true;
				Base = argv[2];
				OutputFilePath = "Decoded.png";
				if(LoopThroughArgs(3, argc, argv)) {
					return 1;
				}
			}
			else if(argc > 3) {
				if(strcmp(argv[1], "/E") && strcmp(argv[1], "/e") && strcmp(argv[1], "encode")) {
					return 1;
				}
				Base = argv[2];
				Source = argv[3];
				if(LoopThroughArgs(4, argc, argv)) {
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
			std::cin >> Base;
			std::cout << "Enter the Source Image path: ";
			std::cin >> Source;
		}
		else if(in == 'd' || in == 'D') {
			OutputFilePath = "Decoded.png";
			decode = true;
			std::cout << "\nDecode Mode\n";
			std::cout << "Enter the encoded image path: ";
			std::cin >> Base;
		}
		else if(in == 'h' || in == 'H') {
			std::cout << "\n\n";
			help();
			return 2;
		}
		else {
			std::cout << "\nInvalid input!\n";
			return 3;
		}
	}

	std::cout << '\n';

	if(decode ? !Decode(Base, OutputFilePath, showimages, verbose) : !Encode(Base, Source, OutputFilePath, showimages, verbose)) {
		return 4;
	}

	return 0;
}
