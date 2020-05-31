#include <string>
#include <limits>
#include "SteganoLogger.h"

#if _WIN32
	#define NOMINMAX // to protect from conflict in std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n')
	#include <Windows.h>
#endif

namespace Stegano {
extern bool quiet{false}, verbose{false}, showimages{false}, base{false}, force{false}, noreduc{false}, grayscale{false};
extern int threads{1};

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
			  << "{force | /f | /F} {gray | /g | /G} {base | /b | /B} [{threads | /t | /T} 1...512]"
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
	std::cout << "7) gray (optional) - Prefer conversion to grayscale over dimension shrinking during reduction process."
			  << "\n\n\t";
	std::cout << "8) base (optional, disables => gray) - Apply expanding transformation on the base image if it is not large"
			  << "\n\t\t"
			  << "enough to store the source image. The source will not be shrunk."
			  << "\n\n\t";
	std::cout << "9) threads (optional, default = 1, max = 512) - Enable multithreading with the specified number of threads."
			  << "\n\t\t"
			  << "If repeated, only the last value is considered. e.g. - Stegano.exe decode ..\\Encoded.png threads 8"
			  << "\n\n";
	std::cout << "AUTHOR"
			  << "\n\t"
			  << "DashwoodIce9" << '\n';
}

static bool LoopThroughArgs(const int start, const int& argc, const char** argv, std::string* OutputFilePath) {
	for(int i = start; i < argc; ++i) {
		if(std::string(argv[i]) == "/o" || std::string(argv[i]) == "/O" || std::string(argv[i]) == "output") {
			std::string ext{argv[i + 1]};
			ext = ext.substr(ext.find_last_of('.') + 1);
			if(ext == "png" || ext == "PNG") {
				*OutputFilePath = argv[i + 1];
			}
			else {
				Stegano::Logger::Log('\n', "Given output path does not end in .png, reverting to default.", '\n');
			}
			++i;
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
		else if(std::string(argv[i]) == "/g" || std::string(argv[i]) == "/G" || std::string(argv[i]) == "grayscale") {
			grayscale = true;
		}
		else if(std::string(argv[i]) == "/f" || std::string(argv[i]) == "/F" || std::string(argv[i]) == "force") {
			force = true;
		}
		else if(std::string(argv[i]) == "/nr" || std::string(argv[i]) == "/NR" || std::string(argv[i]) == "noreduc") {
			noreduc = true;
		}
		else if(std::string(argv[i]) == "/t" || std::string(argv[i]) == "/T" || std::string(argv[i]) == "threads") {
			threads = std::stoi(argv[i + 1]);
			if(threads < 1 || threads > 512) {
				Stegano::Logger::Log('\n', "Improper value for threads passed, defaulting to single threaded operation.", '\n');
				threads = 1;
			}
			++i;
		}
		else if(std::string(argv[i]) == "/b" || std::string(argv[i]) == "/B" || std::string(argv[i]) == "base") {
			base = true;
		}
		else {
			return true;
		}
	}
	Stegano::Logger::Log('\n', "\t\tImage Steganography Tool ", "(command line mode)", "\n\n");
	return false;
}

int handler(const int& argc, const char** argv) {
	bool decode{false};
	std::string Base, Source, OutputFilePath{"Encoded.png"};

	if(argc > 1) {
		if(std::string(argv[1]) == "/h" || std::string(argv[1]) == "/H" || std::string(argv[1]) == "help") {
			help();
			return 0;
		}
		if(argc > 2) {
			if(std::string(argv[1]) == "/D" || std::string(argv[1]) == "/d" || std::string(argv[1]) == "decode") {
				decode = true;
				Source = argv[2];
				OutputFilePath = "Decoded.png";
				if(LoopThroughArgs(3, argc, argv, &OutputFilePath)) {
					return 1;
				}
			}
			else if(argc > 3) {
				if(!(std::string(argv[1]) == "/E" || std::string(argv[1]) == "/e" || std::string(argv[1]) == "encode")) {
					return 1;
				}
				Base = argv[2];
				Source = argv[3];
				if(LoopThroughArgs(4, argc, argv, &OutputFilePath)) {
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
			OutputFilePath = "Decoded.png";
			decode = true;
			std::cout << '\n' << "Decode Mode" << '\n';
			std::cout << "Enter the source image path: ";
			std::getline(std::cin, Source);
		}
		else if(in == 'h' || in == 'H') {
			std::cout << "\n\n";
			help();
			return 2;
		}
		else {
			Stegano::Logger::Error('\n', "Invalid input!", '\n');
			return 3;
		}
	}

	std::cout << '\n';

#if _WIN32
	RECT desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	DesktopWidth = desktop.right;
	DesktopHeight = desktop.bottom;
	Stegano::Logger::Verbose("Screen resolution is: [", DesktopWidth, " x ", DesktopHeight, ']', "\n\n");
#endif

	// Add multithreading code

	if(decode ? !Decode(Source, OutputFilePath) : !Encode(Base, Source, OutputFilePath)) {
		return 4;
	}

	return 0;
}
}
