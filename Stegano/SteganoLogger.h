#pragma once

#include <iostream>

namespace Stegano {

extern bool quiet, verbose;

class Logger {
	Logger() {
	}

	template <typename Arg>
	static void Print(const Arg& arg) {
		std::cout << arg;
	}
	template <typename Arg, typename... Args>
	static void Print(const Arg& arg, const Args&... args) {
		std::cout << arg;
		Print(args...);
	}

public:

	template <typename... Args>
	static void Log(const Args&... args) {
		if(verbose || !quiet) {
			Print(args...);
		}
	}

	template <typename... Args>
	static void Verbose(const Args&... args) {
		if(verbose) {
			Print(args...);
		}
	}

	template <typename Arg>
	static void Error(const Arg& arg) {
		std::cerr << arg;
	}
	template <typename Arg, typename... Args>
	static void Error(const Arg& arg, const Args&... args) {
		std::cerr << arg;
		Error(args...);
	}
};

}
