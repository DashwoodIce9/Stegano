#include <iostream>
#include <cstring>

bool Encrypt(const char* base, const char* source);
bool Decrypt(const char* encrypted);

int main(const int argc, const char* argv[]) {
	bool flag = false;
	if(argc > 1) {
		if(!strcmp(argv[1], "D")) {
			std::cout << "Decrypting " << argv[2] << '\n';
			if(!Decrypt(argv[2])) {
				std::cerr << "\nPress any key to abort\n";
				std::cin.get();
				return -1;
			}
		}
		else if(!strcmp(argv[1], "E") && argc == 4) {
			std::cout << "Encrypting " << argv[3] << " in " << argv[2] << '\n';
			if(!Encrypt(argv[2], argv[3])) {
				std::cerr << "\nPress any key to abort\n";
				std::cin.get();
				return -1;
			}
		}
		else {
			flag = true;
		}
	}
	else {
		flag = true;
	}
	if(flag) {
		std::cerr << "Invalid arguments passed! Correct format is -\n\n";
		std::cerr << "For Encryption - \"<executable> E <base_image_path> <source_image_path>\"\n";
		std::cerr << "For Decryption - \"<executable> D <encrypted_image_path>\"\n";
		std::cerr << "\nPress any key to abort\n";
		std::cin.get();
		return 1;
	}
	return 0;
}
