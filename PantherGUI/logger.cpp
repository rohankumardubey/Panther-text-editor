
#include "logger.h"

static Logger* logger = nullptr;

Logger::Logger() {
	file = mmap::OpenFile(std::string(LOG_FILE), PGFileReadWrite);
}

Logger::~Logger() {
	mmap::CloseFile(file);
}

void Logger::_WriteLogMessage(std::string message) {
	message = message + std::string("\n");
	mmap::WriteToFile(file, message.c_str(), message.size());
	mmap::Flush(file);
}

