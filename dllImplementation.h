// Defines the main functions to export
#pragma once

namespace dllImplementation
{
	int SolveForRoot(const char* expr, size_t exprLen, double initialGuess, int maxSize, double goalErr, double* results);
};
