#include <iostream>

int handler(const int& argc, const char* argv[]);

int main(const int argc, const char* argv[]) {
	if(int return_code = handler(argc, argv)) {
		if(return_code == 1) {
			std::cerr << "Error! Invalid arguments passed!\n\n";
			std::cout << "To encode, run - \"Stegano.exe encrypt <base> <source>\"\n";
			std::cout << "To decode, run - \"Stegano.exe decrypt <encoded>\"\n\n";
			std::cout << "Run - \"Stegano.exe help\" for a full list of commands.\n";
		}
		if(argc == 1) {
			if(return_code != 2 && return_code != 3) {
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			}
			std::cerr << "\nPress enter/return to terminate";
			std::cin.get();
		}
		std::cout << '\n';
		return return_code;
	}
	else {
		return 0;
	}
}
