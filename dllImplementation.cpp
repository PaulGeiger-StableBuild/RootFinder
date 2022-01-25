// Implements the main functions to export

#include "pch.h"
#include "dllImplementation.h"

#include <cmath>
#include "Expression.h"
#include "Logger.h"
#include <memory>

using namespace std;


int dllImplementation::SolveForRoot(const char* expr, size_t exprLen, double initialGuess, int maxSize, double goalErr, double* results)
{

	auto logger = std::make_shared<Logger>("logfile.txt");

	// Uses Newton's Method to calculate the roots.
	// outputs the number of iterations used to solve the system. Stores intermediate results in the results output
	// stops if reaches the maxSize number of iterations or if the absolute error drops reaches goalErr
	// an output of 0 is the signal to the calling funcitons that somethign went wrong
	if (maxSize <= 0 || exprLen == 0)
	{
		logger->Log("Failed to evaluate");
		if (maxSize <= 0)
		{
			logger->Log("Cannot iterate to 0");
		}
		if (exprLen == 0)
		{
			logger->Log("Cannot evaluate nothing");
		}
		return 0;
	}

	int iterNum = 0;
	try
	{
		auto function = Expression(expr, exprLen, logger);


		auto derivative = function.Derivative();
		double xn = initialGuess;
		double funcVal = function.Evaluate(xn);
		results[0] = xn;

		for (iterNum = 1; iterNum <= maxSize && std::fabs(funcVal) > goalErr; ++iterNum)
		{
			// Newton's Method implementation
			// x[n+1] = x[n] - f[x] / f'[x]
			// error == f[x]
			const double derivVal = derivative.Evaluate(xn);
			if (std::fabs(derivVal) < VERY_SMALL_VALUE)
			{
				logger->Log("Derivative found to be zero, exiting");
				return 0; // cannot solve
			}
			xn -= funcVal / derivVal;
			funcVal = function.Evaluate(xn);
			results[iterNum] = xn;
		}
		if (iterNum > maxSize) iterNum = maxSize;
	}
	catch (...)
	{
		// something went wrong. It is possible the inputted expression was incorrect. 
		iterNum = 0;
	}
	return iterNum;
}
