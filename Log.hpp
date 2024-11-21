#ifndef LOG_HPP
#define LOG_HPP

#ifdef DEBUG
#include <fstream>
#include <ctime>
#include <iomanip>
#include <string>

extern std::ofstream _logfile;

#define LOG(msg) \
	do { \
		if (_logfile.is_open()) \
		{ \
			std::time_t t = std::time(NULL); \
			char time_buf[80]; \
			std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t)); \
			_logfile << "[" << time_buf << "] " << msg << std::endl; \
		} \
	} while (0)

#else
#define LOG(msg) do {} while(0)
#endif

#endif