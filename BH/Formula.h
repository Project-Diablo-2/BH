#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

enum class FormulaStatus
{
	OK = 0,
	LEXICAL_ERROR,
	SYNTAX_ERROR,
	MATH_ERROR,
	ARG_COUNT_ERROR
};

enum class FormulaTokenType
{
	NONE,
	NUMBER,
	VARIABLE,
	OP,
	OPEN_P,
	CLOSE_P,
	COMMA,
	T_EOF
};

enum class FormulaOpCode
{
	NONE,
	LITERAL,
	RESOLVER,
	ADD,
	SUB,
	MUL,
	DIV,
	POW,
	EQ,
	NE,
	GT,
	LT,
	GE,
	LE,
	AND,
	OR,
	IF,
	NEGATE,
	NOT,
	LN,
	EXP,
	FLOOR,
	CEIL,
	ROUND,
	MIN,
	MAX,
	MOD,
	AVERAGE,
	SQRT,
	COUNT,
	COUNTIF,
	XOR,
	ABS,
	SIGN,
};

template<typename T>
struct FormulaVarDefinition
{
	int paramCount;
	std::function<float(FormulaStatus& err, T*, const std::vector<int>&)> resolver;

	FormulaVarDefinition(int params, std::function<float(FormulaStatus& err, T*, const std::vector<int>&)> res)
		: paramCount(params),
		resolver(res)
	{}
};

template<typename T>
struct FormulaNode
{
	FormulaOpCode op = FormulaOpCode::NONE;
	float literalValue = 0;
	std::vector<std::unique_ptr<FormulaNode<T>>> children;
	std::function<float(FormulaStatus&, T*)> resolver = nullptr;

	FormulaNode(FormulaOpCode o) : op(o)
	{}
	FormulaNode(float val) : op(FormulaOpCode::LITERAL), literalValue(val)
	{}
};

template<typename T>
class Formula
{
	static float eval(const std::unique_ptr<FormulaNode<T>>& n, T* ctx, FormulaStatus& e);
	static void optimize(std::unique_ptr<FormulaNode<T>>& n, FormulaStatus& e);

	std::unique_ptr<FormulaNode<T>> root;
public:
	static FormulaStatus Compile(const std::string& raw, std::unique_ptr<Formula>& out, const std::unordered_map<std::string, FormulaVarDefinition<T>>& reg);

	Formula(std::unique_ptr<FormulaNode<T>> r) : root(std::move(r))
	{}

	FormulaStatus execute(T* ctx, float& ret) const;

	static bool IsTrue(float f)
	{
		return f != 0.0f;
	}
};

struct FormulaToken
{
	const FormulaTokenType type;
	const std::string value;
};

class FormulaTokenStream
{
	std::vector<FormulaToken> tokens;
	size_t pos = 0;

public:
	void add(FormulaToken t)
	{
		tokens.push_back(t);
	}
	const FormulaToken& peek() const
	{
		static FormulaToken eof_token = { FormulaTokenType::T_EOF, "" };
		return (pos >= tokens.size()) ? eof_token : tokens[pos];
	}
	void advance()
	{
		if (pos < tokens.size()) {
			pos++;
		}
	}
	bool is_at_end() const
	{
		return pos >= tokens.size() || peek().type == FormulaTokenType::T_EOF;
	}
	size_t position() const
	{
		return pos;
	}
};

class FormulaLexer
{
public:
	static FormulaStatus tokenize(const std::string& input, FormulaTokenStream& stream)
	{
		size_t i = 0;
		while (i < input.length()) {
			if (std::isspace(input[i])) {
				i++;
				continue;
			}

			if (i + 1 < input.length()) {
				std::string op2 = input.substr(i, 2);
				if (op2 == "==" || op2 == "!=" || op2 == ">=" || op2 == "<=") {
					stream.add({ FormulaTokenType::OP, op2 });
					i += 2;
					continue;
				}
			}
			if (std::strchr("+-*/^<>!", input[i])) {
				stream.add({ FormulaTokenType::OP, std::string(1, input[i]) });
				i++;
				continue;
			}
			if (input[i] == '(') {
				stream.add({ FormulaTokenType::OPEN_P, "(" });
				i++;
				continue;
			}
			if (input[i] == ')') {
				stream.add({ FormulaTokenType::CLOSE_P, ")" });
				i++;
				continue;
			}
			if (input[i] == ',') {
				stream.add({ FormulaTokenType::COMMA, "," });
				i++;
				continue;
			}

			if (std::isdigit(input[i]) || input[i] == '.') {
				char* end = nullptr;
				std::ignore = strtof(input.c_str() + i, &end);
				size_t len = end - input.c_str() - i;
				if (len == 0) {
					return FormulaStatus::LEXICAL_ERROR;
				}
				stream.add({ FormulaTokenType::NUMBER, input.substr(i, len) });
				i = end - input.c_str();
				continue;
			}

			if (std::isalpha(input[i]) || input[i] == '_') {
				size_t start = i++;
				while (i < input.length() && (std::isalpha(input[i]) || input[i] == '_')) {
					i++;
				}
				stream.add({ FormulaTokenType::VARIABLE, input.substr(start, i - start) });
				continue;
			}
			return FormulaStatus::LEXICAL_ERROR;
		}
		return FormulaStatus::OK;
	}
};

namespace FormulaData
{
	// lower precedence ops are evaluated after higher ones
	static std::unordered_map<std::string, std::pair<FormulaOpCode, int>> opTable = {
		{"==", {FormulaOpCode::EQ, 1}},
		{">", {FormulaOpCode::GT, 1}},
		{"<", {FormulaOpCode::LT, 1}},
		{"!=", {FormulaOpCode::NE, 1}},
		{">=", {FormulaOpCode::GE, 1}},
		{"<=", {FormulaOpCode::LE, 1}},
		{"+", {FormulaOpCode::ADD, 2}},
		{"-", {FormulaOpCode::SUB, 2}},
		{"*", {FormulaOpCode::MUL, 3}},
		{"/", {FormulaOpCode::DIV, 3}},
		{"^", {FormulaOpCode::POW, 4}}
	};

	static std::unordered_map<std::string, FormulaOpCode> fnTable = {
		{"if", FormulaOpCode::IF},
		{"and", FormulaOpCode::AND},
		{"or", FormulaOpCode::OR},
		{"ln", FormulaOpCode::LN},
		{"exp", FormulaOpCode::EXP},
		{"floor", FormulaOpCode::FLOOR},
		{"ceil", FormulaOpCode::CEIL},
		{"round", FormulaOpCode::ROUND},
		{"min", FormulaOpCode::MIN},
		{"max", FormulaOpCode::MAX},
		{"mod", FormulaOpCode::MOD},
		{"average", FormulaOpCode::AVERAGE},
		{"sqrt", FormulaOpCode::SQRT},
		{"pow", FormulaOpCode::POW},
		{"count", FormulaOpCode::COUNT},
		{"countif", FormulaOpCode::COUNTIF},
		{"xor", FormulaOpCode::XOR},
		{"abs", FormulaOpCode::ABS},
		{"sign", FormulaOpCode::SIGN},
	};
}

template<typename T>
class FormulaParser
{
	FormulaTokenStream& stream;
	FormulaStatus& err;
	const std::unordered_map<std::string, FormulaVarDefinition<T>>& registry;

	std::unique_ptr<FormulaNode<T>> parseExpression(const int minPrec)
	{
		auto left = parseUnary();
		if (!left) {
			return nullptr;
		}

		size_t lastPos = -1;
		while (lastPos != stream.position()) {
			lastPos = stream.position();
			const FormulaToken& t = stream.peek();
			if (t.type != FormulaTokenType::OP || FormulaData::opTable.find(t.value) == FormulaData::opTable.end()) {
				break;
			}

			auto& opt = FormulaData::opTable.find(t.value)->second;
			const auto& prec = opt.second;
			if (prec < minPrec) {
				break;
			}
			const auto& op = opt.first;

			stream.advance();
			auto n = std::make_unique<FormulaNode<T>>(op);
			n->children.push_back(std::move(left));
			auto right = parseExpression(prec + (op != FormulaOpCode::POW));
			if (!right) {
				err = FormulaStatus::SYNTAX_ERROR;
				return nullptr;
			}
			n->children.push_back(std::move(right));
			left = std::move(n);
		}
		return left;
	}

	std::unique_ptr<FormulaNode<T>> parseUnary()
	{
		if (stream.peek().value == "-") {
			stream.advance();
			auto n = std::make_unique<FormulaNode<T>>(FormulaOpCode::NEGATE);
			auto child = parseUnary();
			if (!child) {
				err = FormulaStatus::SYNTAX_ERROR;
				return nullptr;
			}
			n->children.push_back(std::move(child));
			return n;
		}
		if (stream.peek().value == "+") {
			stream.advance();
			return parseUnary();
		}
		if (stream.peek().value == "!") {
			stream.advance();
			auto n = std::make_unique<FormulaNode<T>>(FormulaOpCode::NOT);
			auto child = parseUnary();
			if (!child) {
				err = FormulaStatus::SYNTAX_ERROR;
				return nullptr;
			}
			n->children.push_back(std::move(child));
			return n;
		}
		return parsePrimary();
	}

	std::unique_ptr<FormulaNode<T>> parsePrimary()
	{
		const FormulaToken& t = stream.peek();

		if (t.type == FormulaTokenType::VARIABLE) {
			std::string name = t.value;

			if (FormulaData::fnTable.find(name) != FormulaData::fnTable.end()) {
				return parseFunction(name);
			}

			if (registry.find(name) == registry.end()) {
				err = FormulaStatus::SYNTAX_ERROR;
				return nullptr;
			}
			const auto& def = registry.find(name)->second;
			stream.advance();

			std::vector<int> ids;
			for (int i = 0; i < def.paramCount; ++i) {
				if (i > 0) {
					if (stream.peek().type != FormulaTokenType::COMMA) {
						err = FormulaStatus::SYNTAX_ERROR;
						return nullptr;
					}
					stream.advance();
				}

				if (stream.peek().type != FormulaTokenType::NUMBER) {
					err = FormulaStatus::SYNTAX_ERROR;
					return nullptr;
				}

				const auto& value = stream.peek().value;
				char* end = nullptr;
				errno = 0;
				long val = std::strtol(value.c_str(), &end, 10);
				if (errno == ERANGE || *end) {
					err = FormulaStatus::SYNTAX_ERROR;
					return nullptr;
				}
				if (val > (std::numeric_limits<int>::max)() || val < 0) {
					err = FormulaStatus::SYNTAX_ERROR;
					return nullptr;
				}
				ids.push_back(val);
				stream.advance();
			}

			auto n = std::make_unique<FormulaNode<T>>(FormulaOpCode::RESOLVER);
			auto& res = def.resolver;
			n->resolver = [res, ids](FormulaStatus& err, T* ctx) {
				return res(err, ctx, ids);
			};
			return n;
		}

		if (t.type == FormulaTokenType::NUMBER) {
			char* end = nullptr;
			errno = 0;
			float val = std::strtof(t.value.c_str(), &end);
			if (errno == ERANGE || *end) {
				err = FormulaStatus::SYNTAX_ERROR;
				return nullptr;
			}
			stream.advance();
			return std::make_unique<FormulaNode<T>>(val);
		}

		if (t.type == FormulaTokenType::OPEN_P) {
			stream.advance();
			auto n = parseExpression(0);
			if (stream.peek().type != FormulaTokenType::CLOSE_P) {
				err = FormulaStatus::SYNTAX_ERROR;
				return nullptr;
			}
			stream.advance();
			return n;
		}

		err = FormulaStatus::SYNTAX_ERROR;
		return nullptr;
	}

	std::unique_ptr<FormulaNode<T>> parseFunction(const std::string& name)
	{
		FormulaOpCode code = FormulaData::fnTable.find(name)->second;
		stream.advance();

		auto n = std::make_unique<FormulaNode<T>>(code);

		if (stream.peek().type != FormulaTokenType::OPEN_P) {
			err = FormulaStatus::SYNTAX_ERROR;
			return nullptr;
		}

		stream.advance();
		if (stream.peek().type != FormulaTokenType::CLOSE_P) {
			size_t lastPos = -1;
			while (lastPos != stream.position()) {
				lastPos = stream.position();
				auto arg = parseExpression(0);
				if (!arg) {
					err = FormulaStatus::SYNTAX_ERROR;
					return nullptr;
				}
				n->children.push_back(std::move(arg));

				if (stream.peek().type != FormulaTokenType::COMMA) {
					break;
				}
				stream.advance();
			}
		}
		if (stream.peek().type != FormulaTokenType::CLOSE_P) {
			err = FormulaStatus::SYNTAX_ERROR;
			return nullptr;
		}
		stream.advance();

		size_t count = n->children.size();
		bool ok = false;
		switch (code) {
			case FormulaOpCode::LN:
			case FormulaOpCode::EXP:
			case FormulaOpCode::FLOOR:
			case FormulaOpCode::CEIL:
			case FormulaOpCode::ROUND:
			case FormulaOpCode::SQRT:
			case FormulaOpCode::ABS:
			case FormulaOpCode::SIGN:
			{
				ok = count == 1;
				break;
			}
			case FormulaOpCode::MOD:
			case FormulaOpCode::POW:
			{
				ok = count == 2;
				break;
			}
			case FormulaOpCode::IF:
			{
				ok = count == 3;
				break;
			}
			case FormulaOpCode::AND:
			case FormulaOpCode::OR:
			case FormulaOpCode::MIN:
			case FormulaOpCode::MAX:
			case FormulaOpCode::AVERAGE:
			case FormulaOpCode::XOR:
			{
				ok = count > 0;
				break;
			}
			case FormulaOpCode::COUNT:
			case FormulaOpCode::COUNTIF:
			{
				ok = count > 1;
				break;
			}
		}
		if (!ok) {
			err = FormulaStatus::ARG_COUNT_ERROR;
			return nullptr;
		}

		return n;
	}
public:
	FormulaParser(FormulaTokenStream& ts, FormulaStatus& e, const std::unordered_map<std::string, FormulaVarDefinition<T>>& reg)
		: stream(ts),
		err(e),
		registry(reg)
	{}

	std::unique_ptr<FormulaNode<T>> parse()
	{
		auto res = parseExpression(0);
		if (!stream.is_at_end()) {
			err = FormulaStatus::SYNTAX_ERROR;
		}
		return res;
	}
};

template<typename T>
float Formula<T>::eval(const std::unique_ptr<FormulaNode<T>>& n, T* ctx, FormulaStatus& e)
{
	if (e != FormulaStatus::OK || !n) {
		return 0;
	}

	switch (n->op) {
		case FormulaOpCode::LITERAL:
		{
			return n->literalValue;
		}
		case FormulaOpCode::RESOLVER:
		{
			return n->resolver(e, ctx);
		}
		case FormulaOpCode::NEGATE:
		{
			return -eval(n->children[0], ctx, e);
		}
		case FormulaOpCode::NOT:
		{
			return !Formula<T>::IsTrue(eval(n->children[0], ctx, e));
		}
		case FormulaOpCode::ADD:
		{
			return eval(n->children[0], ctx, e) + eval(n->children[1], ctx, e);
		}
		case FormulaOpCode::SUB:
		{
			return eval(n->children[0], ctx, e) - eval(n->children[1], ctx, e);
		}
		case FormulaOpCode::MUL:
		{
			return eval(n->children[0], ctx, e) * eval(n->children[1], ctx, e);
		}
		case FormulaOpCode::DIV:
		{
			return eval(n->children[0], ctx, e) / eval(n->children[1], ctx, e);
		}
		case FormulaOpCode::POW:
		{
			return std::pow(eval(n->children[0], ctx, e), eval(n->children[1], ctx, e));
		}
		case FormulaOpCode::EQ:
		{
			return eval(n->children[0], ctx, e) == eval(n->children[1], ctx, e);
		}
		case FormulaOpCode::NE:
		{
			return eval(n->children[0], ctx, e) != eval(n->children[1], ctx, e);
		}
		case FormulaOpCode::GT:
		{
			return eval(n->children[0], ctx, e) > eval(n->children[1], ctx, e);
		}
		case FormulaOpCode::LT:
		{
			return eval(n->children[0], ctx, e) < eval(n->children[1], ctx, e);
		}
		case FormulaOpCode::GE:
		{
			return eval(n->children[0], ctx, e) >= eval(n->children[1], ctx, e);
		}
		case FormulaOpCode::LE:
		{
			return eval(n->children[0], ctx, e) <= eval(n->children[1], ctx, e);
		}
		case FormulaOpCode::IF:
		{
			return Formula<T>::IsTrue(eval(n->children[0], ctx, e)) ? eval(n->children[1], ctx, e) : eval(n->children[2], ctx, e);
		}
		case FormulaOpCode::AND:
		{
			for (auto& c : n->children) {
				if (!Formula<T>::IsTrue(eval(c, ctx, e))) {
					return 0;
				}
			}
			return 1;
		}
		case FormulaOpCode::OR:
		{
			for (auto& c : n->children) {
				if (Formula<T>::IsTrue(eval(c, ctx, e))) {
					return 1;
				}
			}
			return 0;
		}
		case FormulaOpCode::LN:
		{
			return std::log(eval(n->children[0], ctx, e));
		}
		case FormulaOpCode::EXP:
		{
			return std::exp(eval(n->children[0], ctx, e));
		}
		case FormulaOpCode::FLOOR:
		{
			return std::floor(eval(n->children[0], ctx, e));
		}
		case FormulaOpCode::CEIL:
		{
			return std::ceil(eval(n->children[0], ctx, e));
		}
		case FormulaOpCode::ROUND:
		{
			return std::round(eval(n->children[0], ctx, e));
		}
		case FormulaOpCode::MIN:
		{
			float res = (std::numeric_limits<float>::max)();
			for (auto& c : n->children) {
				float v = eval(c, ctx, e);
				res = (std::min)(res, v);
			}
			return res;
		}
		case FormulaOpCode::MAX:
		{
			float res = std::numeric_limits<float>::lowest();
			for (auto& c : n->children) {
				float v = eval(c, ctx, e);
				res = (std::max)(res, v);
			}
			return res;
		}
		case FormulaOpCode::MOD:
		{
			return std::fmodf(eval(n->children[0], ctx, e), eval(n->children[1], ctx, e));
		}
		case FormulaOpCode::AVERAGE:
		{
			float res = 0.0;
			for (auto& c : n->children) {
				res += eval(c, ctx, e);
			}
			return res / n->children.size();
		}
		case FormulaOpCode::SQRT:
		{
			return std::sqrt(eval(n->children[0], ctx, e));
		}
		case FormulaOpCode::XOR:
		{
			bool res = false;
			for (auto& c : n->children) {
				res ^= Formula<T>::IsTrue(eval(c, ctx, e));
			}
			return res;
		}
		case FormulaOpCode::COUNT:
		{
			size_t res = 0;
			for (auto& c : n->children) {
				res += Formula<T>::IsTrue(eval(c, ctx, e));
			}
			return res;
		}
		case FormulaOpCode::COUNTIF:
		{
			size_t res = 0;
			float lastEl = eval(n->children.back(), ctx, e);
			for (size_t i = 0; i < n->children.size() - 1; ++i) {
				const auto& c = n->children[i];
				res += eval(c, ctx, e) == lastEl;
			}
			return res;
		}
		case FormulaOpCode::ABS:
		{
			return fabsf(eval(n->children[0], ctx, e));
		}
		case FormulaOpCode::SIGN:
		{
			const float v = eval(n->children[0], ctx, e);
			return (0 < v) - (v < 0);
		}
		default:
		{
			return 0;
		}
	}
}

template<typename T>
void Formula<T>::optimize(std::unique_ptr<FormulaNode<T>>& n, FormulaStatus& err)
{
	if (!n || err != FormulaStatus::OK) {
		return;
	}
	for (auto& child : n->children) {
		optimize(child, err);
	}

	bool allLiterals = !n->children.empty();
	for (const auto& c : n->children) {
		if (c->op != FormulaOpCode::LITERAL) {
			allLiterals = false;
		}
	}

	if (allLiterals && n->op != FormulaOpCode::RESOLVER && n->op != FormulaOpCode::LITERAL) {
		float val = eval(n, nullptr, err);
		if (err != FormulaStatus::OK) {
			err = FormulaStatus::MATH_ERROR;
			return;
		}
		n->children.clear();
		n->op = FormulaOpCode::LITERAL;
		n->literalValue = val;
	}
	else if (n->op == FormulaOpCode::IF && n->children[0]->op == FormulaOpCode::LITERAL) {
		if (Formula<T>::IsTrue(n->children[0]->literalValue)) {
			n = std::move(n->children[1]);
		}
		else {
			n = std::move(n->children[2]);
		}
	}
	else if (n->op == FormulaOpCode::AND) {
		for (auto& c : n->children) {
			if (c->op == FormulaOpCode::LITERAL && !Formula<T>::IsTrue(c->literalValue)) {
				n = std::move(c);
				n->literalValue = 0.0f;
				break;
			}
		}
	}
	else if (n->op == FormulaOpCode::OR) {
		for (auto& c : n->children) {
			if (c->op == FormulaOpCode::LITERAL && Formula<T>::IsTrue(c->literalValue)) {
				n = std::move(c);
				n->literalValue = 1.0f;
				break;
			}
		}
	}
}

template<typename T>
FormulaStatus Formula<T>::Compile(const std::string& raw, std::unique_ptr<Formula<T>>& out, const std::unordered_map<std::string, FormulaVarDefinition<T>>& reg)
{
	FormulaTokenStream ts;
	std::string input = raw;
	std::transform(input.begin(), input.end(), input.begin(), tolower);

	FormulaStatus err = FormulaLexer::tokenize(input, ts);
	if (err != FormulaStatus::OK) {
		return err;
	}

	FormulaParser<T> p(ts, err, reg);
	auto rootNode = p.parse();

	if (err != FormulaStatus::OK || !rootNode) {
		return (err == FormulaStatus::OK) ? FormulaStatus::SYNTAX_ERROR : err;
	}

	optimize(rootNode, err);

	out = std::make_unique<Formula>(std::move(rootNode));
	return FormulaStatus::OK;
}

template<typename T>
FormulaStatus Formula<T>::execute(T* ctx, float& ret) const
{
	FormulaStatus e = FormulaStatus::OK;
	ret = eval(root, ctx, e);
	return e;
}
