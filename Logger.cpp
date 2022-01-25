#include "pch.h"
#include "Logger.h"

#include <stdlib.h>
#include <iostream>

using namespace std;

Logger::Logger(const string& logNameIn) :
	logName (logNameIn)
{
	if (IS_DEBUG) logfile.open(logName, ofstream::out);
	LogEndChunk("Log started successfully\n");
}
Logger::Logger(const Logger& otherLogger)
{
	logName = otherLogger.logName;
	if (IS_DEBUG) logfile.open(logName, ofstream::app | ofstream::out);
}

Logger::~Logger()
{
	if (IS_DEBUG) logfile.close();
}

void Logger::LogEndChunk(const string& toLog)
{
	LogEndChunk(toLog.c_str());
}

void Logger::LogEndChunk(const char* toLog)
{
	Log(toLog);
	logfile << "\n";
}

void Logger::Log(const string& toLog)
{
	Log(toLog.c_str());
}

void Logger::Log(const char* toLog)
{
	if (IS_DEBUG) logfile << toLog << "\n";
}
