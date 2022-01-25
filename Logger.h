#pragma once

#include <fstream>
#include <string>
class Logger

{
public:
	Logger(const std::string& logName);
	Logger(const Logger& otherLogger);
	~Logger();

	void LogEndChunk(const std::string& toLog);
	void LogEndChunk(const char* toLog);

	void Log(const std::string& toLog);
	void Log(const char* toLog);

private:
	std::ofstream logfile;
	std::string logName;
};

