#pragma once

#include "Logger.h"
#include <functional>
#include <string>
#include <utility>
#include <vector>

enum class InterTermOperator
{
	NONE,
	PLUS,
	MINUS
};

class Expression
{
public:
	Expression() = delete;
	Expression(const char* inputExpr, size_t exprLen, const std::shared_ptr<Logger>& loggerIn);
	Expression(std::string inputExpr, const std::shared_ptr<Logger>& loggerIn);
	~Expression();

	double Evaluate(double x);

	const std::string& GetExpr();

	Expression Derivative();
private:
	size_t EvalSubExpression(std::string& expr, size_t startPosition, double x);
	size_t EvalSpecialFunction(std::string& expr, size_t startPositionOfFunction, size_t startPositionOfValue, std::function<double(double)> func, double x);
	void EvalSubExpressions(std::string& expr, double x);
	void EvalSpecialFunctions(std::string& expr, double x);
	void EvalPowers(std::string& expr, double x);
	void EvalMultiplcation(std::string& expr, double x);
	void EvalAddition(std::string& expr, double x);
	double EvalInternal(const std::string& expr, double x);

	std::string FindDerivativeInternal(const std::string& expr, const std::string& derivVar);
	std::vector<std::pair<InterTermOperator, std::string>> BreakUpTerms(const std::string& expressionToBreak);
	std::string RecombineTerms(const std::vector<std::pair<InterTermOperator, std::string>>& expressionToBreak);
	std::vector<std::pair<InterTermOperator, std::string>> ProcessDerivatives(
		const std::vector<std::pair<InterTermOperator, std::string>>& itemsToLookAt, const std::string& derivVar);
	std::string ProcessChainRule(const std::string& term, const std::string& derivVar, bool& solved);
	std::string ProcessProductRule(const std::string& term, const std::string& derivVar, bool& solved);
	std::string ProcessQuotientRule(const std::string& term, const std::string& derivVar, bool& solved);
	std::string ProcessPowers(const std::string& term, const std::string& derivVar, bool& solved);
	std::string ProcessSpecialFunctions(const std::string& term, const std::string& derivVar, bool& solved);

	std::string expr;
	std::shared_ptr<Logger> logger;
};
