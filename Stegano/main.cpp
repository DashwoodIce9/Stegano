#include <iostream>

namespace Stegano {
int handler(const int& argc, const char** argv);
void hold(); // Hold Screen
}

int main(const int argc, const char** argv) {
	if(int return_code = Stegano::handler(argc, argv)) {
		if(return_code == 1) {
			std::cerr << "Error! Invalid arguments passed!\n\n";
			std::cout << "To encode, run - \"Stegano.exe encrypt <base> <source>\"\n";
			std::cout << "To decode, run - \"Stegano.exe decrypt <encoded>\"\n\n";
			std::cout << "Run - \"Stegano.exe help\" for a full list of commands.\n";
		}

		std::cout << '\n';

		// For interactive mode
		if(argc == 1) {
			Stegano::hold();
		}
		return return_code;
	}

	// For successful run of interactive mode
	if(argc == 1) {
		Stegano::hold();
	}
	std::cout << '\n';

	return 0;
}
