#include "pch.h"
#include "Expression.h"

#include <ctype.h>
#include <math.h>
#include <regex>
#include <string.h>

namespace
{
	std::string GetSubExpression(const std::string& expr, size_t startPosition, size_t& endPosition);
	double GetValueToRight(const std::string& expr, size_t startPosition, size_t& endPosition, double x);
	double GetValueToLeft(const std::string& expr, size_t endPosition, size_t& startPosition, double x);
	std::string Reverse(const std::string& s);
}

Expression::Expression(const char* inputExpr, size_t exprLen, const std::shared_ptr<Logger>& loggerIn) :
	logger(loggerIn)
{
	expr = "";
	for (int i = 0; i < exprLen; ++i)
		expr.push_back(inputExpr[i]);
}

Expression::Expression(std::string inputExpr, const std::shared_ptr<Logger>& loggerIn) :
	expr(inputExpr),
	logger(loggerIn)
{ }

Expression::~Expression()
{ }

const std::string& Expression::GetExpr()
{
	return expr;
}


double Expression::Evaluate(double x)
{
	const std::string logMsg = "Evaluate - begun evaluation of: " + expr;
	logger->Log(logMsg);

	const double returnVal = EvalInternal(expr, x);

	const std::string logMsg2 = "Evaluate - result of evaluation: " + std::to_string(returnVal);
	logger->LogEndChunk(logMsg2);

	return returnVal;
}

Expression Expression::Derivative()
{
	const std::string logMsg = "Derivative - begun finding derivative of: " + expr;
	logger->Log(logMsg);

	const std::string derivative = FindDerivativeInternal(expr, "x");

	const std::string logMsg2 = "Derivative - found result to be: " + derivative;
	logger->LogEndChunk(logMsg2);
	return Expression(derivative, logger);
}

std::string Expression::FindDerivativeInternal(const std::string& expr, const std::string& deriveVar)
{
	const auto itemsToLookAt = BreakUpTerms(expr);
	const auto processedItems = ProcessDerivatives(itemsToLookAt, deriveVar);
	const std::string derivative = RecombineTerms(processedItems);
	return derivative;
}

std::vector<std::pair<InterTermOperator, std::string>> Expression::BreakUpTerms(const std::string& expressionToBreak)
{
	// breaks up the expressionToBreak on addition/subtraction for each item not encapsulated in parentheses
	// puts together all terms in parenthesis

	std::vector<std::pair<InterTermOperator, std::string>> terms;
	std::string currentTerm = "";
	InterTermOperator currentOp = InterTermOperator::NONE;
	char lastC = '\0';
	bool isFirst = true;
	size_t nestingLevel = 0;
	for (char c : expressionToBreak)
	{
		if (nestingLevel == 0 && (c == '+' || (c == '-' && lastC != 'E' && !isFirst)))
		{
			terms.push_back(std::make_pair(currentOp, currentTerm));
			currentOp = c == '+' ?
				InterTermOperator::PLUS :
				InterTermOperator::MINUS;
			currentTerm = "";
			isFirst = true;
		}
		else
		{
			if (c == '(') ++nestingLevel;
			else if (c == ')') --nestingLevel;
			currentTerm.push_back(c);
			isFirst = false;
		}

		lastC = c;
	}
	if (!currentTerm.empty()) terms.push_back(std::make_pair(currentOp, currentTerm));

	// Make a note in the log of what are results are
	std::string logMsg = "BreakUpDerivativeTerms - broke-up expression " + expressionToBreak + " into:";
	for (const auto& [op, term] : terms)
	{
		std::string opString;
		switch (op)
		{
		case InterTermOperator::PLUS:
			opString = "+";
			break;
		case InterTermOperator::MINUS:
			opString = "-";
			break;
		default:
			opString = "";
		}
		logMsg += " {" + opString + ", " + term + "}";
	}
	logger->Log(logMsg);
	return terms;
}


std::string Expression::RecombineTerms(const std::vector<std::pair<InterTermOperator, std::string>>& brokenExpression)
{
	// puts together all terms in brokenExpression
	std::string completeExpr = "";
	for (const auto& [op, term] : brokenExpression)
	{
		if (term.empty()) continue;
		const bool isNeg = (term.front() == '-');
		switch (op)
		{
		case InterTermOperator::PLUS:
			if (isNeg) completeExpr += term;
			else completeExpr += "+" + term;
			break;
		case InterTermOperator::MINUS:
			if (isNeg) completeExpr += "+" + term.substr(1,term.size()-1); //  /remove the negative sign + double negate
			else completeExpr += "-" + term;
			break;
		default:
			completeExpr = term;
		}
	}
	return completeExpr;
}


std::vector<std::pair<InterTermOperator, std::string>> Expression::ProcessDerivatives(
	const std::vector<std::pair<InterTermOperator, std::string>>& itemsToLookAt, const std::string& deriveVar)
{
	std::vector<std::pair<InterTermOperator, std::string>> processedItems;
	for (const auto& [op, term] : itemsToLookAt)
	{
		std::string logMsg = "ProcessDerivatives - processing " + term + " with respect to " + deriveVar;
		logger->Log(logMsg);

		std::string processedItem = "0";
		bool solved = false;
		const std::string negDeriveVar = "-" + deriveVar;
		if (term.compare(deriveVar) == 0)
		{
			std::string logMsg2 = "ProcessDerivatives - result is 1";
			logger->Log(logMsg2);
			processedItem = "1";
			solved = true;
		}
		if (!solved && term.compare(negDeriveVar) == 0)
		{
			std::string logMsg2 = "ProcessDerivatives - result is -1";
			logger->Log(logMsg2);
			processedItem = "-1";
			solved = true;
		}
		if (!solved && term.find(deriveVar) == std::string::npos)
		{
			std::string logMsg2 = "ProcessDerivatives - result is 0";
			logger->Log(logMsg2);
			processedItem = "0";
			solved = true;
		}
		if (!solved) processedItem = ProcessProductRule(term, deriveVar, solved);
		if (!solved) processedItem = ProcessQuotientRule(term, deriveVar, solved);
		if (!solved) processedItem = ProcessPowers(term, deriveVar, solved);
		if (!solved) processedItem = ProcessSpecialFunctions(term, deriveVar, solved);
		if (!solved) processedItem = ProcessChainRule(term, deriveVar, solved);
		if (!solved)
		{
			// unsolvable case
			std::string errMsg = "ProcessDerivatives - Unable to process derivative for " + term;
			logger->Log(errMsg);
			throw std::invalid_argument(errMsg);
		}
		processedItems.push_back(std::make_pair(op, processedItem));
	}
	return processedItems;
}

std::string Expression::ProcessChainRule(const std::string& term, const std::string& deriveVar, bool& solved)
{
	size_t parenthPos = 0;
	size_t sizeOfTerm = term.size();
	size_t nestingLevel = 0;
	solved = false;
	while (parenthPos < sizeOfTerm)
	{
		const char c = term[parenthPos];
		if (c == '(') ++nestingLevel;
		else if (c == ')') --nestingLevel;

		if (nestingLevel == 1)
		{
			const std::string logMsg = "ProcessChainRule - processing: " + term;
			logger->Log(logMsg);
			std::string processedItem = "0";
			size_t endPosition = parenthPos;
			const std::string subExpr = GetSubExpression(term, parenthPos+1, endPosition);
			const std::string subExprDerivative = FindDerivativeInternal(subExpr, deriveVar);



			const size_t positionToStartReplace = parenthPos;
			const std::string toReplace = "(" + subExpr + ")";
			const std::string subVar = deriveVar + "\'";
			const std::string logMsgReplace = "ProcessChainRule - going to replace from: " + std::to_string(positionToStartReplace) + " for " + std::to_string(toReplace.size());
			logger->Log(logMsgReplace);
			std::string substitutedTerm = term;
			substitutedTerm.replace(positionToStartReplace, toReplace.size(), subVar);

			const std::string substitutedTermDerivative = FindDerivativeInternal(substitutedTerm, subVar);

			// clean-up a bit and put together
			if (substitutedTermDerivative.compare("0") == 0 || subExprDerivative.compare("0") == 0) processedItem = "0";
			else if (substitutedTermDerivative.compare("1") == 0) processedItem = subExprDerivative;
			else if (subExprDerivative.compare("1") == 0) processedItem = substitutedTermDerivative;
			else processedItem = "(" + substitutedTermDerivative + ")*(" + subExprDerivative+")";

			const std::string logMsg2 = "ProcessChainRule - result: " + processedItem;
			logger->Log(logMsg2);

			solved = true;
			return processedItem;
		}
		++parenthPos;
	}
	return "0";
}

std::string Expression::ProcessProductRule(const std::string& term, const std::string& deriveVar, bool& solved)
{
	size_t pos = 0;
	size_t sizeOfTerm = term.size();
	size_t nestingLevel = 0;
	solved = false;
	while (pos < sizeOfTerm)
	{
		const char c = term[pos];
		if (c == '(') ++nestingLevel;
		else if (c == ')') --nestingLevel;

		if (nestingLevel == 0 && c == '*')
		{
			const std::string logMsg = "ProcessProductRule - processing: " + term;
			logger->Log(logMsg);
			std::string processedItem = "0";

			const std::string lTerm = term.substr(0, pos);
			const std::string rTerm = term.substr(pos+1);
			if (lTerm.compare("0") == 0 || rTerm.compare("0") == 0)
			{
				// trivial case
				processedItem = "0";
				solved = true;
				return processedItem;
			}

			const std::string lTermDerivative = FindDerivativeInternal(lTerm, deriveVar);
			const std::string rTermDerivative = FindDerivativeInternal(rTerm, deriveVar);

			// clean-up a bit and put together
			const bool lTD0 = lTermDerivative.compare("0") == 0;
			const bool lTD1 = lTermDerivative.compare("1") == 0;
			const bool rTD0 = rTermDerivative.compare("0") == 0;
			const bool rTD1 = rTermDerivative.compare("1") == 0;
			if (lTD0 && rTD0) processedItem = "0";
			else if (lTD0 && rTD1) processedItem = lTerm;
			else if (lTD1 && rTD0) processedItem = rTerm;
			else if (rTD1 && lTD1) processedItem = lTerm + "+" + rTerm;
			else if (lTD0) processedItem = "(" + lTerm + ")*" + "(" + rTermDerivative + ")";
			else if (rTD0) processedItem = "(" + rTerm + ")*" + "(" + lTermDerivative + ")";
			else processedItem = "(" + lTerm + ")*(" + rTermDerivative + ")+(" + rTerm + ")*(" + lTermDerivative + ")";

			const std::string logMsg2 = "ProcessProductRule - result: " + processedItem;
			logger->Log(logMsg2);

			solved = true;
			return processedItem;
		}
		++pos;
	}
	return "0";
}


std::string Expression::ProcessQuotientRule(const std::string& term, const std::string& deriveVar, bool& solved)
{
	size_t pos = 0;
	size_t sizeOfTerm = term.size();
	size_t nestingLevel = 0;
	solved = false;
	while (pos < sizeOfTerm)
	{
		const char c = term[pos];
		if (c == '(') ++nestingLevel;
		else if (c == ')') --nestingLevel;

		if (nestingLevel == 0 && c == '/')
		{
			const std::string logMsg = "ProcessQuotientRule - processing: " + term;
			logger->Log(logMsg);
			std::string processedItem = "0";

			const std::string lTerm = term.substr(0, pos);
			const std::string rTerm = term.substr(pos+1);
			if (lTerm.compare("0") == 0)
			{
				// trivial case
				processedItem = "0";
				solved = true;
				return processedItem;
			}
			if (rTerm.compare("0") == 0)
			{
				// unsolvable case
				std::string errMsg = "ProcessQuotientRule - Unable to process derivative " + term;
				logger->Log(errMsg);
				throw std::invalid_argument(errMsg);
			}
			const std::string lTermDerivative = FindDerivativeInternal(lTerm, deriveVar);
			const std::string rTermDerivative = FindDerivativeInternal(rTerm, deriveVar);

			// clean-up a bit and put together
			const bool lTD0 = lTermDerivative.compare("0") == 0;
			const bool rTD0 = rTermDerivative.compare("0") == 0;
			const bool rTD1 = rTermDerivative.compare("1") == 0;
			if (lTD0 && rTD0) processedItem = "0";
			else if (lTD0 && rTD1) processedItem = "(" + lTerm + ")/(" + rTerm + ")^2";
			else if (rTD0) processedItem = "(" + lTermDerivative + ")/(" + rTerm + ")";
			else if (lTD0) processedItem = "(" + lTerm + ")*(" + rTermDerivative + ")/(" + rTerm + ")^2";
			else if (rTD1) processedItem = "(" + lTermDerivative + ")/" + rTerm + "-" + lTerm + "/" + rTerm + "^2";
			else processedItem = "(" + lTermDerivative + ")/(" + rTerm + ")-(" + lTerm + ")*(" + rTermDerivative + ")/(" + rTerm + ")^2";

			const std::string logMsg2 = "ProcessQuotientRule - result: " + processedItem;
			logger->Log(logMsg2);

			solved = true;
			return processedItem;
		}
		++pos;
	}
	return "0";
}

std::string Expression::ProcessPowers(const std::string& term, const std::string& deriveVar, bool& solved)
{
	size_t pos = 0;
	size_t sizeOfTerm = term.size();
	size_t nestingLevel = 0;
	solved = false;
	while (pos < sizeOfTerm)
	{
		const char c = term[pos];
		if (c == '(') ++nestingLevel;
		else if (c == ')') --nestingLevel;

		if (nestingLevel == 0 && c == '^')
		{
			const std::string logMsg = "ProcessPowers - processing: " + term;
			logger->Log(logMsg);
			std::string processedItem = "0";

			const std::string lTerm = term.substr(0, pos);
			const std::string rTerm = term.substr(pos+1);
			if (lTerm.compare("0") == 0 || rTerm.compare("0") == 0)
			{
				// trivial case
				processedItem = "0";
				solved = true;
				return processedItem;
			}
			const std::string lTermDerivative = FindDerivativeInternal(lTerm, deriveVar);
			const std::string rTermDerivative = FindDerivativeInternal(rTerm, deriveVar);

			// clean-up a bit and put together
			const bool lTD0 = lTermDerivative.compare("0") == 0;
			const bool lTD1 = lTermDerivative.compare("1") == 0;
			const bool rTD0 = rTermDerivative.compare("0") == 0;
			const bool rTD1 = rTermDerivative.compare("1") == 0;
			if (lTD0 && rTD0) processedItem = "0"; // trivial case
			else if (lTD0)
			{
				// exp
				if (lTerm.compare("e") == 0 && rTD1) processedItem = "e^" + rTerm + "";
				else if (rTD1) processedItem = "(" + lTerm + ")^(" + rTerm + ")*" + "ln(" + lTerm + ")";
				else if (lTerm.compare("e") == 0)  processedItem =lTerm + "^(" + rTerm + ")*(" + rTermDerivative + ")";
				else processedItem = "(" + lTerm + ")^(" + rTerm + ")*(" + "ln(" + lTerm + ")*(" + rTermDerivative + "))";
			}
			else if (rTD0)
			{
				// polynomial
				const double rTermVal = EvalInternal(rTerm, 0);
				const std::string rTermValS = std::to_string(rTermVal);
				const double rTermPowVal = rTermVal - 1.0;
				const std::string rTermPowValS = std::to_string(rTermPowVal);
				if (lTD1) processedItem = rTermValS + "*(" + lTerm + ")^" + rTermPowValS;
				else processedItem = "(" + rTermValS + ")*(" + lTermDerivative + ")*(" + lTerm + ")^(" + rTermPowValS +")";
			}
			else
			{
				// complex case
				if (lTD1 && rTD1) processedItem = "(" + lTerm + ")^(" + rTerm + "-1)*((" + rTerm + ")+(" + lTerm + ")*ln(" + lTerm + "))";
				else if (lTD1) processedItem = "(" + lTerm + ")^(" + rTerm + "-1)*((" + rTerm+ ")+(" + lTerm + ")*ln(" + lTerm + ")*(" + rTermDerivative + "))";
				else if (rTD1) processedItem = "(" + lTerm + ")^(" + rTerm + "-1)*((" + rTerm + ")*(" + lTermDerivative + ")+(" + lTerm + ")*ln(" + lTerm + "))";
				else processedItem = "(" + lTerm + ")^(" + rTerm + "-1)*((" + rTerm + ")*(" + lTermDerivative + ")+(" + lTerm + ")*ln(" + lTerm + ")*(" + rTermDerivative + "))";
			}

			const std::string logMsg2 = "ProcessPowers - result: " + processedItem;
			logger->Log(logMsg2);

			solved = true;
			return processedItem;
		}
		++pos;
	}
	return "0";
}

std::string Expression::ProcessSpecialFunctions(const std::string& term, const std::string& deriveVar, bool& solved)
{			
	// at this point, if "solved" is to be true, the function should be of form "[function-escape](f(x))"
	// only sin should start with s
	// only cos should start with c
	// only tan should start with t
	// only ln should start with l
	if (term.size() < 2)
	{
		return "0";
	}
	const char c = term[0];
	const char c2 = term[1];
	std::string processedItem = "0";
	size_t offset = 0;
	const bool isNeg = c == '-';
	const char cStar = isNeg ? c2 : c;
	if (isNeg) offset += 1;
	switch (cStar)
	{
	case 's':
		solved = true;
		offset += 3;
		break;
	case 'c':
		solved = true;
		offset += 3;
		break;
	case 't':
		solved = true;
		offset += 3;
		break;
	case 'l':
		solved = true;
		offset += 2;
		break;
	default:
		solved = false;
	}
	if (!solved) return processedItem;

	const std::string logMsg = "ProcessSpecialFunctions - " + term;
	logger->Log(logMsg);
	const std::string fn = term.substr(offset);
	const std::string fnDerivative = FindDerivativeInternal(fn, deriveVar);

	if (fnDerivative.compare("0") == 0)
	{
		// trivial case
		processedItem = "0";
		return processedItem;
	}
	const bool fnD1 = fnDerivative.compare("1") == 0;

	switch (cStar)
	{
	case 's':
		processedItem = "cos" + fn;
		if (!fnD1) processedItem = "(" + fnDerivative + ")*" + processedItem;
		break;
	case 'c':
		processedItem = "-sin" + fn;
		if (!fnD1) processedItem = "(" + fnDerivative + ")*" + processedItem;
		break;
	case 't':
		processedItem = "(" + fnDerivative + ")/(cos" + fn + ")^2";
		break;
	case 'l':
		processedItem = "(" + fnDerivative + ")/(" + fn + ")";
		break;
	}
	if (isNeg) processedItem = "-1.0*" + processedItem;
	return processedItem;

}

size_t Expression::EvalSubExpression(std::string& expr, size_t startPosition, double x)
{
	// Replaces the sub expressions starting at startPosition with the value evaluated at x
	// Outputs the end position of the last digit in the new substring in the modified expr
	// startPosition = position after parenthesis
	// endPosition = position of end parenthesis
	if (startPosition >= expr.size())
	{
		return startPosition;
	}
	size_t endPosition = startPosition;
	const std::string subExpr = GetSubExpression(expr, startPosition, endPosition);

	const std::string logMsgStart = "EvalSubExpression - beginning evaluation of sub expression: " + subExpr;
	logger->Log(logMsgStart);

	const double subResult = EvalInternal(subExpr, x);
	const std::string subResultS = std::to_string(subResult);

	const size_t positionToStartReplace = startPosition - 1; // begins after opening parenthesis, still need to remove opening parenthesis.
	const size_t totalSizeToReplace = endPosition - startPosition + 2; // include parentheses pair

	// Make note in log about what is being done
	const std::string logMsg = "EvalSubExpression - replaced sub expression: " + subExpr + " with " + subResultS;
	logger->Log(logMsg);

	// Make note in log and replace the expression
	std::string logMsg2 = "Before: " + expr;
	expr.replace(positionToStartReplace, totalSizeToReplace, subResultS);
	logMsg2 += " After: " + expr;
	logger->Log(logMsg2);

	// new position = position of last digit in new substring
	return positionToStartReplace + subResultS.size();
}
	
size_t Expression::EvalSpecialFunction(std::string& expr, size_t startPositionOfFunction, size_t startPositionOfValue, std::function<double (double)> func, double x)
{
	// Replaces the expression starting at startPositionOfFunction with the value evaluated at x, using the inputted func
	// Outputs the end position of the last digit in the new substring in the modified expr
	// startPositionOfFunction = position of the first character of the function keyword
	// startPositionOfValue = position of the first digit

	size_t endPosition = startPositionOfValue;
	const double value = GetValueToRight(expr, startPositionOfValue, endPosition, x);
	const double evaluatedValue = func(value);
	const std::string evaluatedValueS = std::to_string(evaluatedValue);

	// Make note in log and replace the expression
	std::string logMsg = "EvalSpecialFunction - Before: " + expr;
	const size_t totalSizeToReplace = endPosition - startPositionOfFunction + 1;
	expr.replace(startPositionOfFunction, totalSizeToReplace, evaluatedValueS);
	logMsg += " After: " + expr;
	logger->Log(logMsg);

	return startPositionOfFunction + evaluatedValueS.size() -1;
}

void Expression::EvalSubExpressions(std::string& expr, double x)
{
	size_t position = 0;
	size_t sizeOfExpression = expr.size();
	while (position < sizeOfExpression)
	{
		const char c = expr[position];
		if (c == '(')
		{
			position = EvalSubExpression(expr, position + 1, x);
			sizeOfExpression = expr.size();
		}
		++position;
	}
}

void Expression::EvalSpecialFunctions(std::string& expr, double x)
{
	const static auto sinLambda = [](double x) { return std::sin(x); };
	const static auto cosLambda = [](double x) { return std::cos(x); };
	const static auto tanLambda = [](double x) { return std::tan(x); };
	const static auto lnLambda = [](double x) { return std::log(x); };
	const static auto negSinLambda = [](double x) { return -1.0 * std::sin(x); };
	const static auto negCosLambda = [](double x) { return -1.0 * std::cos(x); };
	const static auto negTanLambda = [](double x) { return -1.0 * std::tan(x); };
	const static auto negLnLambda = [](double x) { return -1.0 * std::log(x); };
	
	size_t position = 0;
	size_t sizeOfExpression = expr.size();
	while (position < sizeOfExpression)
	{
		// at this point:
		// only sin should start with s
		// only cos should start with c
		// only tan should start with t
		// only ln should start with l
		// all values in parenthesis of functions are evaluated.
		const char c = expr[position];
		const bool hasNextC = position != sizeOfExpression;
		const char nextC = hasNextC ?
			expr[position + 1] :
			c; // default, always check hasNextC

		const bool isNeg = c == '-';
		const char cStar = isNeg && hasNextC ? nextC : c;

		switch (cStar)
		{
		case 's':
			if (isNeg) position = EvalSpecialFunction(expr, position, position + 4, negSinLambda, x);
			else position = EvalSpecialFunction(expr, position, position + 3 , sinLambda, x);
			sizeOfExpression = expr.size();
			break;
		case 'c':
			if (isNeg) position = EvalSpecialFunction(expr, position, position + 4, negCosLambda, x);
			else position = EvalSpecialFunction(expr, position, position + 3, cosLambda, x);
			sizeOfExpression = expr.size();
			break;
		case 't':
			if (isNeg) position = EvalSpecialFunction(expr, position, position + 4, negTanLambda, x);
			else position = EvalSpecialFunction(expr, position, position + 3, tanLambda, x);
			sizeOfExpression = expr.size();
			break;
		case 'l':
			if (isNeg) position = EvalSpecialFunction(expr, position, position + 3, negLnLambda, x);
			else position = EvalSpecialFunction(expr, position, position + 2, lnLambda, x);
			sizeOfExpression = expr.size();
			break;
		}
		++position;
	}
}

void Expression::EvalPowers(std::string& expr, double x)
{
	// evaluates all powers, left-to-right
	size_t nestingLevel = 0;

	size_t position = 0;
	size_t sizeOfExpression = expr.size();
	while (position < sizeOfExpression)
	{
		const char c = expr[position];
		if (c == '^')
		{
			size_t startPosition = position;
			size_t endPosition = position;
			const double valueToLeft = GetValueToLeft(expr, position - 1, startPosition, x);
			const double valueToRight = GetValueToRight(expr, position + 1, endPosition, x);
			const double evaluatedValue = std::pow(valueToLeft, valueToRight);
			const std::string evaluatedValueS = std::to_string(evaluatedValue);

			const std::string logMsgLR = "EvalPowers - Left: " + std::to_string(valueToLeft) + " - " + std::to_string(startPosition) + " - Right: " + std::to_string(valueToRight) + " - " + std::to_string(endPosition);
			logger->Log(logMsgLR);

			// Make note in log and replace the expression
			std::string logMsg = "EvalPowers - Before: " + expr;
			const size_t totalSizeToReplace = endPosition - startPosition + 1;
			expr.replace(startPosition, totalSizeToReplace, evaluatedValueS);
			logMsg += " After: "+ expr;
			logger->Log(logMsg);

			position = startPosition + evaluatedValueS.size() - 1;
			sizeOfExpression = expr.size();
		}
		++position;
	}
}

void Expression::EvalMultiplcation(std::string& expr, double x)
{
	// evaluates all multiplaction and division operations, left-to-right
	size_t nestingLevel = 0;

	size_t position = 0;
	size_t sizeOfExpression = expr.size();
	while (position < sizeOfExpression)
	{
		const char c = expr[position];
		if (c == '*' || c == '/')
		{
			size_t startPosition = position;
			size_t endPosition = position;
			const double valueToLeft = GetValueToLeft(expr, position - 1, startPosition, x);
			const double valueToRight = GetValueToRight(expr, position + 1, endPosition, x);

			const std::string logMsgLR = "EvalMultiplcation - Left: " + std::to_string(valueToLeft) + " - " + std::to_string(startPosition) + " - Right: " + std::to_string(valueToRight) + " - " + std::to_string(endPosition);
			logger->Log(logMsgLR);

			const double evaluatedValue = c == '/' ?
				valueToLeft / valueToRight :
				valueToLeft * valueToRight;
			const std::string evaluatedValueS = std::to_string(evaluatedValue);

			// Make note in log and replace the expression
			std::string logMsg = "EvalMultiplcation - Before: " + expr;
			const size_t totalSizeToReplace = endPosition - startPosition + 1;
			expr.replace(startPosition, totalSizeToReplace, evaluatedValueS);
			auto pos = expr.find("+-");
			if (pos != std::string::npos) expr.replace(pos, 2, "-");
			pos = expr.find("--");
			if (pos != std::string::npos) expr.replace(pos, 2, "+");
			logMsg += " After: " + expr;
			logger->Log(logMsg);

			position = startPosition + evaluatedValueS.size() - 1;
			sizeOfExpression = expr.size();
		}
		++position;
	}

}

void Expression::EvalAddition(std::string& expr, double x)
{
	// evaluates all addition and subtraction operations, left-to-right
	size_t nestingLevel = 0;

	size_t position = 0;
	size_t sizeOfExpression = expr.size();
	while (position < sizeOfExpression)
	{
		const char c = expr[position];
		if (c == '+' || (c == '-' && position != 0))
		{
			size_t startPosition = position;
			size_t endPosition = position;
			const double valueToLeft = GetValueToLeft(expr, position - 1, startPosition, x);
			const double valueToRight = GetValueToRight(expr, position + 1, endPosition, x);


			const std::string logMsgLR = "EvalPowers - Left: " + std::to_string(valueToLeft) + " - " + std::to_string(startPosition) + " - Right: " + std::to_string(valueToRight) + " - " + std::to_string(endPosition);
			logger->Log(logMsgLR);

			const double evaluatedValue = c == '+' ?
				valueToLeft + valueToRight :
				valueToLeft - valueToRight;
			const std::string evaluatedValueS = std::to_string(evaluatedValue);

			// Make note in log and replace the expression
			std::string logMsg = "EvalAddition - Before: " + expr;
			const size_t totalSizeToReplace = endPosition - startPosition + 1;
			expr.replace(startPosition, totalSizeToReplace, evaluatedValueS);
			logMsg += " After: " + expr;
			logger->Log(logMsg);

			position = startPosition + evaluatedValueS.size() - 1;

			sizeOfExpression = expr.size();
		}
		++position;
	}
}

double Expression::EvalInternal(const std::string& expr, double x)
{
	// solve the given expression, recursively using order-of-operations
	if (expr.empty())
	{
		std::string errMsg = "EvalInternal - Received an invalid entry " + expr;
		logger->Log(errMsg);
		throw std::invalid_argument(errMsg);
	}
	if (expr.compare("x") == 0)
	{
		return x;
	}
	if (expr.compare("-x") == 0)
	{
		return -x;
	}


	std::string exprModified = expr;

	// solve expressions inside parenthesis, evaluate sub expr and replace with values
	EvalSubExpressions(exprModified, x);

	// find sin, cos, tan. ReplaceVals
	EvalSpecialFunctions(exprModified, x);

	// find powers from left to right
	EvalPowers(exprModified, x);

	// find multiply/divide
	EvalMultiplcation(exprModified, x);

	// find addition/subtraction
	EvalAddition(exprModified, x);

	// if the final solution is just "x", a final replacement is required
	if (exprModified.compare("x") == 0)
		exprModified = std::to_string(x);

	return std::stod(exprModified);
}

namespace
{

	double GetValueToRight(const std::string& expr, size_t startPosition, size_t& endPosition, double x)
	{
		// Return result from a double substring starting at startPosition
		// endPosition is the digit the function last looks at
		bool hasDecimal = false;
		std::string returnVal = "";

		endPosition = startPosition;
		size_t sizeOfExpression = expr.size();
		while (endPosition < sizeOfExpression)
		{
			const char c = expr[endPosition];
			switch (c)
			{
			case 'x':
				// this is the variable x
				if (!returnVal.empty()) return std::stod(returnVal) * x; // assume multiplcation was intended in this case
				return x;
			case 'e':
				if (returnVal.empty())  return M_E; // assume this is the constant "e" and not intended for scientific notation 
				break;
			case 'E':
			case '.':
				break;
			default:
				if (!std::isdigit(c) && !(c == '-' && (returnVal.empty() || returnVal.back() == 'e')))
				{
					--endPosition; // went to far
					return std::stod(returnVal);
				}
			}

			returnVal.push_back(c);
			++endPosition;
		}

		--endPosition; // went to far
		return std::stod(returnVal);
	}

	double GetValueToLeft(const std::string& expr, size_t endPosition, size_t& startPosition, double x)
	{
		// Return result from a double substring starting at endPosition and going left
		// startPosition is the digit the function last looks at
		bool hasDecimal = false;
		std::string returnVal = "";

		startPosition = endPosition;
		size_t sizeOfExpression = expr.size();
		while (startPosition >= 0)
		{
			const char c = expr[startPosition];
			const bool hasNoNextVal = startPosition == 0;
			const char nextVal = hasNoNextVal ?
				c : // default, should always check hasNoNextVal
				expr[startPosition - 1];
			switch (c)
			{
			case 'x':
				// this is the variable x
				if (!returnVal.empty()) return std::stod(Reverse(returnVal)) * x; // assume multiplcation was intended in this case
				return x;
			case 'e':
				if (hasNoNextVal || !std::isdigit(nextVal)) return M_E; // assume this is the constant "e" and not intended for scientific notation 
				break;
			case 'E':
			case '.':
				break;
			default:
				if (!std::isdigit(c) && !(c == '-' && (hasNoNextVal || nextVal == 'e')))
				{
					++startPosition;
					return std::stod(Reverse(returnVal));
				}
			}

			returnVal.push_back(c);

			if (hasNoNextVal) return std::stod(Reverse(returnVal));
			--startPosition;
		}
		return 0.0;
	}

	std::string GetSubExpression(const std::string& expr, size_t startPosition, size_t& endPosition)
	{
		// startPosition = position after parenthesis
		// endPosition = position of end parenthesis
		size_t nestingLevel = 1;

		std::string returnVal = "";
		returnVal.reserve(expr.size()); // is a maximum

		endPosition = startPosition;
		size_t sizeOfExpression = expr.size();
		while (endPosition < sizeOfExpression)
		{
			const char c = expr[endPosition];
			if (c == '(') ++nestingLevel;
			else if (c == ')') --nestingLevel;

			if (nestingLevel == 0) return returnVal;
			returnVal.push_back(c);
			++endPosition;
		}
		return returnVal;
	}

	std::string Reverse(const std::string& s)
	{
		const std::string rev(s.rbegin(), s.rend());
		return rev;
	}
}