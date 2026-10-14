#include "doctest/doctest.h"

#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Formula.h"
#include "LootFilter.h"

// Formula.h is the expression language behind the loot filter's `Formula[KEY]: ...` definitions
// and inline `$f(...)` islands. Expected values follow the PD2 wiki's "Formulas" section
// (https://wiki.projectdiablo2.com/wiki/Formula_Info, transcluded on Item_Filtering), which says
// formulas "closely resemble Excel or Sheets formulas", plus ordinary arithmetic.

namespace {

// A tiny variable context so Formula<T> can be exercised without the game.
struct Ctx {
	float x = 0;
	float y = 0;
	int xReads = 0;
	int yReads = 0;
	int boomReads = 0;
	std::vector<int> lastParams;
};

const std::unordered_map<std::wstring, FormulaVarDefinition<Ctx>>& Defs() {
	static const std::unordered_map<std::wstring, FormulaVarDefinition<Ctx>> defs = {
		{ L"x", { 0, [](FormulaStatus& err, Ctx* c, const std::vector<int>& ids) -> float {
			c->xReads++;
			return c->x;
		} } },
		{ L"y", { 0, [](FormulaStatus& err, Ctx* c, const std::vector<int>& ids) -> float {
			c->yReads++;
			return c->y;
		} } },
		{ L"x_y", { 0, [](FormulaStatus& err, Ctx* c, const std::vector<int>& ids) -> float {
			return 42;
		} } },
		// one integer parameter, echoed back
		{ L"p", { 1, [](FormulaStatus& err, Ctx* c, const std::vector<int>& ids) -> float {
			c->lastParams = ids;
			return (float)ids[0];
		} } },
		// two integer parameters
		{ L"q", { 2, [](FormulaStatus& err, Ctx* c, const std::vector<int>& ids) -> float {
			c->lastParams = ids;
			return (float)(ids[0] * 1000 + ids[1]);
		} } },
		// a variable whose lookup fails at run time
		{ L"boom", { 0, [](FormulaStatus& err, Ctx* c, const std::vector<int>& ids) -> float {
			c->boomReads++;
			err = FormulaStatus::MATH_ERROR;
			return 0;
		} } },
	};
	return defs;
}

// True when `text` does not compile. BH only distinguishes OK from not OK (a rejected definition
// or island is skipped), so the tests do not pin which error status the parser reports.
bool Rejected(const std::wstring& text) {
	std::unique_ptr<Formula<Ctx>> out;
	return Formula<Ctx>::Compile(text, out, Defs()) != FormulaStatus::OK;
}

std::unique_ptr<Formula<Ctx>> MustCompile(const std::wstring& text) {
	std::unique_ptr<Formula<Ctx>> out;
	FormulaStatus st = Formula<Ctx>::Compile(text, out, Defs());
	INFO("formula: ", std::string(text.begin(), text.end()));
	REQUIRE(st == FormulaStatus::OK);
	REQUIRE(out);
	return out;
}

// Compiles and runs `text`; the run must succeed.
float Eval(const std::wstring& text, Ctx& ctx) {
	auto f = MustCompile(text);
	float v = 0;
	INFO("formula: ", std::string(text.begin(), text.end()));
	REQUIRE(f->execute(&ctx, v) == FormulaStatus::OK);
	return v;
}

float Eval(const std::wstring& text) {
	Ctx ctx;
	return Eval(text, ctx);
}

FormulaStatus RunStatus(const std::wstring& text, Ctx& ctx, float& v) {
	auto f = MustCompile(text);
	return f->execute(&ctx, v);
}

// ---- loot filter helpers ----

// The name BH shows for an item the game calls "Axe", as a narrow string so failures print
// readably (the names in these tests are plain ASCII).
std::string Name(support::TestItem& item) {
	std::wstring name = support::NameOf(item, L"Axe");
	std::string out;
	for (wchar_t c : name) {
		out += c < 128 ? (char)c : '?';
	}
	return out;
}

// What BH shows as the item's name when the only rule is `ItemDisplay[]: <action>`.
std::string Show(const std::string& action, support::TestItem& item, const std::string& extraLines = "") {
	support::LoadFilter(extraLines + "ItemDisplay[]: " + action + "\n");
	return Name(item);
}

// How the filter renders the inline formula `$f(expr)` for the item.
std::string Render(const std::string& expr, support::TestItem& item) {
	return Show("<$f(" + expr + ")>", item);
}

}  // namespace

TEST_SUITE("Formula") {

TEST_CASE("number literals parse as decimal reals") {
	CHECK(Eval(L"0") == 0.0f);
	CHECK(Eval(L"42") == 42.0f);
	CHECK(Eval(L"007") == 7.0f);
	CHECK(Eval(L"2.5") == 2.5f);
	CHECK(Eval(L".5") == 0.5f);
	CHECK(Eval(L"5.") == 5.0f);
	CHECK(Eval(L"0.0000001") == doctest::Approx(1e-7).epsilon(1e-6));
}

TEST_CASE("malformed number literals are rejected") {
	// a lone dot is not a number
	CHECK(Rejected(L"."));
	CHECK(Rejected(L"1 + ."));
	// two dots make two adjacent numbers, which is not an expression
	CHECK(Rejected(L"1.2.3"));
	CHECK(Rejected(L"1 2"));
}

TEST_CASE("characters outside the formula language are rejected") {
	// the wiki lists the operators == > < != >= <= + - * / ^ !; '%' is not modulus (MOD() is)
	CHECK(Rejected(L"7 % 3"));
	CHECK(Rejected(L"1 = 1"));
	CHECK(Rejected(L"1 && 1"));
	CHECK(Rejected(L"1 || 0"));
	CHECK(Rejected(L"$1"));
	CHECK(Rejected(L"[1]"));
}

TEST_CASE("multiplication and division bind tighter than addition and subtraction") {
	CHECK(Eval(L"1 + 2 * 3") == 7.0f);
	CHECK(Eval(L"2 * 3 + 1") == 7.0f);
	CHECK(Eval(L"10 - 6 / 2") == 7.0f);
	CHECK(Eval(L"(1 + 2) * 3") == 9.0f);
	CHECK(Eval(L"((((2))))") == 2.0f);
	CHECK(Eval(L"2 * (3 + (4 - 1)) / 3") == 4.0f);
}

TEST_CASE("binary operators of equal precedence group left to right") {
	CHECK(Eval(L"10 - 4 - 3") == 3.0f);
	CHECK(Eval(L"100 / 10 / 5") == 2.0f);
	CHECK(Eval(L"12 / 3 * 2") == 8.0f);
	CHECK(Eval(L"1 - 2 + 3") == 2.0f);
}

TEST_CASE("exponentiation binds tightest and groups right to left") {
	CHECK(Eval(L"2 ^ 10") == 1024.0f);
	CHECK(Eval(L"2 * 3 ^ 2") == 18.0f);
	CHECK(Eval(L"3 ^ 2 * 2") == 18.0f);
	// 2^(3^2) = 2^9, not (2^3)^2 = 64. Right-to-left grouping is deliberate in the parser
	// (parseExpression recurses at the same precedence for '^') and is the usual mathematical
	// convention, unlike Excel/Sheets which group '^' left to right.
	CHECK(Eval(L"2 ^ 3 ^ 2") == 512.0f);
	CHECK(Eval(L"4 ^ 0.5") == 2.0f);
	CHECK(Eval(L"2 ^ -1") == 0.5f);
}

TEST_CASE("unary minus, plus and not apply to the operand that follows") {
	CHECK(Eval(L"-3") == -3.0f);
	CHECK(Eval(L"--3") == 3.0f);
	CHECK(Eval(L"+3") == 3.0f);
	CHECK(Eval(L"-(2 + 3)") == -5.0f);
	CHECK(Eval(L"2 * -3") == -6.0f);
	CHECK(Eval(L"2 - -3") == 5.0f);
	CHECK(Eval(L"-2 + 5") == 3.0f);
	// Excel convention (formulas "closely resemble Excel"): negation binds tighter than ^
	CHECK(Eval(L"-2 ^ 2") == 4.0f);
	CHECK(Eval(L"-(2 ^ 2)") == -4.0f);
}

TEST_CASE("logical not turns zero into 1 and anything else into 0") {
	CHECK(Eval(L"!0") == 1.0f);
	CHECK(Eval(L"!5") == 0.0f);
	CHECK(Eval(L"!-1") == 0.0f);
	CHECK(Eval(L"!0.5") == 0.0f);
	CHECK(Eval(L"!!5") == 1.0f);
	CHECK(Eval(L"!(1 > 2)") == 1.0f);
	// not applies to its operand only: (!0) + 1
	CHECK(Eval(L"!0 + 1") == 2.0f);
}

TEST_CASE("comparisons yield 1 or 0 at their boundaries") {
	CHECK(Eval(L"3 == 3") == 1.0f);
	CHECK(Eval(L"3 == 3.5") == 0.0f);
	CHECK(Eval(L"3 != 3") == 0.0f);
	CHECK(Eval(L"3 != 4") == 1.0f);
	CHECK(Eval(L"3 > 3") == 0.0f);
	CHECK(Eval(L"4 > 3") == 1.0f);
	CHECK(Eval(L"3 < 3") == 0.0f);
	CHECK(Eval(L"2 < 3") == 1.0f);
	CHECK(Eval(L"3 >= 3") == 1.0f);
	CHECK(Eval(L"2.9 >= 3") == 0.0f);
	CHECK(Eval(L"3 <= 3") == 1.0f);
	CHECK(Eval(L"3.1 <= 3") == 0.0f);
	CHECK(Eval(L"-1 < 0") == 1.0f);
}

TEST_CASE("comparisons bind looser than arithmetic and chain left to right") {
	CHECK(Eval(L"1 + 1 == 2") == 1.0f);
	CHECK(Eval(L"2 == 1 + 1") == 1.0f);
	CHECK(Eval(L"2 * 3 > 5") == 1.0f);
	CHECK(Eval(L"2 * 3 > 6") == 0.0f);
	CHECK(Eval(L"10 - 4 >= 2 ^ 3") == 0.0f);
	// (3 > 2) > 1  ->  1 > 1
	CHECK(Eval(L"3 > 2 > 1") == 0.0f);
	// (1 < 2) == 1
	CHECK(Eval(L"1 < 2 == 1") == 1.0f);
	// (2 == 2) == 2  ->  1 == 2
	CHECK(Eval(L"2 == 2 == 2") == 0.0f);
	// a comparison result is an ordinary number
	CHECK(Eval(L"(5 > 3) + (2 > 1) + (0 > 1)") == 2.0f);
}

TEST_CASE("division is real division and follows IEEE rules for zero divisors") {
	CHECK(Eval(L"7 / 2") == 3.5f);
	CHECK(Eval(L"-7 / 2") == -3.5f);
	CHECK(Eval(L"1 / 4") == 0.25f);
	CHECK(Eval(L"1 / 3 * 3") == doctest::Approx(1.0));
	// dividing by zero is not a compile or run error; the output path reports such values as f_err
	CHECK(Eval(L"1 / 0") == std::numeric_limits<float>::infinity());
	CHECK(Eval(L"-1 / 0") == -std::numeric_limits<float>::infinity());
	CHECK(std::isnan(Eval(L"0 / 0")));
}

TEST_CASE("whitespace between tokens is ignored") {
	CHECK(Eval(L"1+2*3") == 7.0f);
	CHECK(Eval(L"  1 +\t2 \r\n* 3  ") == 7.0f);
	CHECK(Eval(L"max ( 1 , 2 )") == 2.0f);
	CHECK(Eval(L"3>=3") == 1.0f);
	// a space splits a two-character operator, and a lone '=' is not an operator
	CHECK(Rejected(L"3 > = 3"));
}

TEST_CASE("function names and variables are case-insensitive") {
	Ctx ctx;
	ctx.x = 4;
	CHECK(Eval(L"MAX(1, 2)") == 2.0f);
	CHECK(Eval(L"Max(1, 2)") == 2.0f);
	CHECK(Eval(L"SQRt(4)") == 2.0f);
	CHECK(Eval(L"X + x", ctx) == 8.0f);
	CHECK(Eval(L"X_Y") == 42.0f);
}

TEST_CASE("empty and incomplete expressions are rejected") {
	CHECK(Rejected(L""));
	CHECK(Rejected(L"   "));
	CHECK(Rejected(L"1 +"));
	CHECK(Rejected(L"* 2"));
	CHECK(Rejected(L"1 * * 2"));
	CHECK(Rejected(L"-"));
	CHECK(Rejected(L"!"));
	CHECK(Rejected(L"1 >"));
	// '!' is only a prefix operator
	CHECK(Rejected(L"1 ! 2"));
	CHECK(Rejected(L"1 ,"));
}

TEST_CASE("unbalanced parentheses are rejected") {
	CHECK(Rejected(L"(1 + 2"));
	CHECK(Rejected(L"1 + 2)"));
	CHECK(Rejected(L"((1)"));
	CHECK(Rejected(L"(1))"));
	CHECK(Rejected(L"()"));
	CHECK(Rejected(L")("));
	CHECK(Rejected(L"max(1, 2"));
	// juxtaposition is not multiplication
	CHECK(Rejected(L"2(3)"));
}

TEST_CASE("IF picks the second argument when the first is non-zero, otherwise the third") {
	CHECK(Eval(L"if(1, 10, 20)") == 10.0f);
	CHECK(Eval(L"if(0, 10, 20)") == 20.0f);
	CHECK(Eval(L"if(-0.5, 10, 20)") == 10.0f);
	CHECK(Eval(L"if(2 > 1, 10, 20)") == 10.0f);
	Ctx ctx;
	ctx.x = 0;
	CHECK(Eval(L"if(x, 10, 20)", ctx) == 20.0f);
	ctx.x = 3;
	CHECK(Eval(L"if(x, 10, 20)", ctx) == 10.0f);
	CHECK(Eval(L"if(x > 2, if(x > 5, 3, 2), 1)", ctx) == 2.0f);
}

TEST_CASE("AND, OR and XOR return 1 or 0 by the truthiness of all arguments") {
	CHECK(Eval(L"and(1, 2, -1)") == 1.0f);
	CHECK(Eval(L"and(1, 0, 1)") == 0.0f);
	CHECK(Eval(L"and(0.5)") == 1.0f);
	CHECK(Eval(L"or(0, 0)") == 0.0f);
	CHECK(Eval(L"or(0, 3)") == 1.0f);
	CHECK(Eval(L"or(-2)") == 1.0f);
	// XOR is true when an odd number of arguments are true
	CHECK(Eval(L"xor(1, 0)") == 1.0f);
	CHECK(Eval(L"xor(1, 1)") == 0.0f);
	CHECK(Eval(L"xor(1, 1, 1)") == 1.0f);
	CHECK(Eval(L"xor(0)") == 0.0f);
	CHECK(Eval(L"xor(5, 0, 0, 7)") == 0.0f);
	Ctx ctx;
	ctx.x = 2;
	ctx.y = 0;
	CHECK(Eval(L"and(x > 1, y == 0)", ctx) == 1.0f);
	CHECK(Eval(L"or(x > 5, y)", ctx) == 0.0f);
}

TEST_CASE("AND, OR and IF do not evaluate arguments they do not need") {
	// the wiki: "AND and OR are short-circuited" (listed under "Minor Details: Tidbits that may
	// or may not change"; if that changes, update these together with the wiki)
	Ctx ctx;
	float v = -1;
	ctx.x = 0;
	CHECK(RunStatus(L"and(x, boom)", ctx, v) == FormulaStatus::OK);
	CHECK(v == 0.0f);
	CHECK(ctx.boomReads == 0);

	ctx.x = 1;
	CHECK(RunStatus(L"or(x, boom)", ctx, v) == FormulaStatus::OK);
	CHECK(v == 1.0f);
	CHECK(ctx.boomReads == 0);

	ctx.x = 1;
	CHECK(RunStatus(L"if(x, 7, boom)", ctx, v) == FormulaStatus::OK);
	CHECK(v == 7.0f);
	ctx.x = 0;
	CHECK(RunStatus(L"if(x, boom, 8)", ctx, v) == FormulaStatus::OK);
	CHECK(v == 8.0f);
	CHECK(ctx.boomReads == 0);

	// arguments that are needed are evaluated, and their errors are reported
	ctx.x = 1;
	CHECK(RunStatus(L"and(x, boom)", ctx, v) == FormulaStatus::MATH_ERROR);
	ctx.x = 0;
	CHECK(RunStatus(L"or(x, boom)", ctx, v) == FormulaStatus::MATH_ERROR);
	CHECK(ctx.boomReads == 2);
}

TEST_CASE("a constant IF condition selects its branch without reading the other one") {
	// the wiki: "constant expressions such as POW(3,3) or IF(TRUE,STAT1,STAT2) are evaluated once"
	// (listed under "Minor Details: Tidbits that may or may not change")
	Ctx ctx;
	float v = -1;
	ctx.x = 4;
	ctx.y = 9;
	CHECK(RunStatus(L"if(1, x, boom)", ctx, v) == FormulaStatus::OK);
	CHECK(v == 4.0f);
	CHECK(RunStatus(L"if(0, boom, y)", ctx, v) == FormulaStatus::OK);
	CHECK(v == 9.0f);
	CHECK(ctx.boomReads == 0);
}

TEST_CASE("rounding functions follow the wiki examples") {
	CHECK(Eval(L"floor(1.5)") == 1.0f);
	CHECK(Eval(L"floor(-1.5)") == -2.0f);
	CHECK(Eval(L"floor(3)") == 3.0f);
	CHECK(Eval(L"ceil(1.49)") == 2.0f);
	CHECK(Eval(L"ceil(-1.5)") == -1.0f);
	CHECK(Eval(L"ceil(3)") == 3.0f);
	CHECK(Eval(L"round(1.49)") == 1.0f);
	CHECK(Eval(L"round(1.5)") == 2.0f);
	// Excel's ROUND rounds halves away from zero (no banker's rounding)
	CHECK(Eval(L"round(2.5)") == 3.0f);
	CHECK(Eval(L"round(-1.5)") == -2.0f);
	CHECK(Eval(L"round(-1.49)") == -1.0f);
}

TEST_CASE("MIN, MAX and AVERAGE take one or more arguments") {
	CHECK(Eval(L"min(3, -1, 2)") == -1.0f);
	CHECK(Eval(L"max(3, -1, 2)") == 3.0f);
	CHECK(Eval(L"min(7)") == 7.0f);
	CHECK(Eval(L"max(-7)") == -7.0f);
	CHECK(Eval(L"max(-3, -2)") == -2.0f);
	CHECK(Eval(L"min(2.5, 2.25)") == 2.25f);
	CHECK(Eval(L"average(8, 8, 8, 8, 8)") == 8.0f);
	CHECK(Eval(L"average(1, 2)") == 1.5f);
	CHECK(Eval(L"average(-4, 4, 3)") == 1.0f);
	CHECK(Eval(L"max(min(3, 5), 1 + 1)") == 3.0f);
	Ctx ctx;
	ctx.x = 60;
	CHECK(Eval(L"min(x, 50) <= 50", ctx) == 1.0f);
	CHECK(Eval(L"min(x, 50)", ctx) == 50.0f);
}

TEST_CASE("MOD keeps the sign of the dividend and works on reals") {
	// the wiki example: MOD(-17,5)==-2
	CHECK(Eval(L"mod(-17, 5)") == -2.0f);
	CHECK(Eval(L"mod(17, 5)") == 2.0f);
	CHECK(Eval(L"mod(17, -5)") == 2.0f);
	CHECK(Eval(L"mod(10, 5)") == 0.0f);
	CHECK(Eval(L"mod(7.5, 2)") == 1.5f);
	CHECK(Eval(L"mod(3, 7)") == 3.0f);
	CHECK(std::isnan(Eval(L"mod(5, 0)")));
}

TEST_CASE("power, root and logarithm functions") {
	CHECK(Eval(L"pow(2, 2)") == 4.0f);
	CHECK(Eval(L"pow(3, 3)") == 27.0f);
	CHECK(Eval(L"pow(9, 0.5)") == 3.0f);
	CHECK(Eval(L"pow(2, -2)") == 0.25f);
	CHECK(Eval(L"pow(5, 0)") == 1.0f);
	CHECK(Eval(L"sqrt(4)") == 2.0f);
	CHECK(Eval(L"sqrt(2)") == doctest::Approx(1.41421356));
	CHECK(std::isnan(Eval(L"sqrt(-1)")));
	CHECK(Eval(L"exp(0)") == 1.0f);
	CHECK(Eval(L"exp(1)") == doctest::Approx(2.718281828));
	CHECK(Eval(L"ln(1)") == 0.0f);
	CHECK(Eval(L"ln(exp(2))") == doctest::Approx(2.0));
	CHECK(Eval(L"ln(0)") == -std::numeric_limits<float>::infinity());
	CHECK(std::isnan(Eval(L"ln(-1)")));
	Ctx ctx;
	ctx.x = 50;
	CHECK(Eval(L"exp(ln(x))", ctx) == doctest::Approx(50.0));
}

TEST_CASE("COUNT counts true arguments and COUNTIF counts matches of the last argument") {
	CHECK(Eval(L"count(1, 0, 1, 1)") == 3.0f);
	CHECK(Eval(L"count(0)") == 0.0f);
	CHECK(Eval(L"count(-2, 0.5, 0)") == 2.0f);
	// the wiki example: COUNTIF(1,0,1)==1 (the last value is the one searched for)
	CHECK(Eval(L"countif(1, 0, 1)") == 1.0f);
	CHECK(Eval(L"countif(2, 2, 2, 2)") == 3.0f);
	CHECK(Eval(L"countif(1, 2)") == 0.0f);
	CHECK(Eval(L"countif(3, 1, 3, 3, 3)") == 3.0f);
	Ctx ctx;
	ctx.x = 3;
	ctx.y = 4;
	CHECK(Eval(L"countif(x, y, 3, x + 1, 4)", ctx) == 2.0f);
}

TEST_CASE("ABS and SIGN") {
	CHECK(Eval(L"abs(-1)") == 1.0f);
	CHECK(Eval(L"abs(2.5)") == 2.5f);
	CHECK(Eval(L"abs(-0.25)") == 0.25f);
	CHECK(Eval(L"abs(0)") == 0.0f);
	CHECK(Eval(L"sign(-1)") == -1.0f);
	CHECK(Eval(L"sign(0)") == 0.0f);
	CHECK(Eval(L"sign(1)") == 1.0f);
	CHECK(Eval(L"sign(-0.001)") == -1.0f);
	CHECK(Eval(L"sign(250)") == 1.0f);
}

TEST_CASE("functions with the wrong number of arguments are rejected") {
	CHECK(Rejected(L"if(1, 2)"));
	CHECK(Rejected(L"if(1, 2, 3, 4)"));
	CHECK(Rejected(L"mod(1)"));
	CHECK(Rejected(L"pow(1, 2, 3)"));
	CHECK(Rejected(L"countif(1)"));
	CHECK(Rejected(L"sqrt(1, 2)"));
	CHECK(Rejected(L"abs()"));
	CHECK(Rejected(L"sign(1, 2)"));
	CHECK(Rejected(L"round(1, 2)"));
	// "All functions require one or more arguments"
	CHECK(Rejected(L"max()"));
	CHECK(Rejected(L"min()"));
	CHECK(Rejected(L"and()"));
	CHECK(Rejected(L"or()"));
	CHECK(Rejected(L"xor()"));
	CHECK(Rejected(L"count()"));
	CHECK(Rejected(L"average()"));
	// an arity error inside a larger expression still rejects the whole formula
	CHECK(Rejected(L"1 + max(2, abs(3, 4))"));
}

TEST_CASE("malformed function calls are rejected") {
	CHECK(Rejected(L"max"));
	CHECK(Rejected(L"max 1"));
	CHECK(Rejected(L"max(1,)"));
	CHECK(Rejected(L"max(,1)"));
	CHECK(Rejected(L"max(1 2)"));
	CHECK(Rejected(L"max(1,,2)"));
	CHECK(Rejected(L"maximum(1)"));
}

TEST_CASE("variables are read from the context on every run") {
	auto f = MustCompile(L"x * (2 + 3) - y");
	Ctx a;
	a.x = 2;
	a.y = 1;
	Ctx b;
	b.x = -3;
	b.y = 0.5f;
	float v = 0;
	REQUIRE(f->execute(&a, v) == FormulaStatus::OK);
	CHECK(v == 9.0f);
	REQUIRE(f->execute(&b, v) == FormulaStatus::OK);
	CHECK(v == -15.5f);
	a.x = 10;
	REQUIRE(f->execute(&a, v) == FormulaStatus::OK);
	CHECK(v == 49.0f);
	CHECK(a.xReads == 2);
}

TEST_CASE("unknown variables are rejected") {
	CHECK(Rejected(L"z"));
	CHECK(Rejected(L"x + z"));
	CHECK(Rejected(L"max(x, nope)"));
	// a name is a whole run of letters and underscores, not a prefix match
	CHECK(Rejected(L"xy"));
	CHECK(Rejected(L"x_"));
	// a variable cannot be called like a function
	CHECK(Rejected(L"x(1)"));
}

TEST_CASE("parameterized variables take integer parameters separated by commas") {
	// wiki: "Parameters are integers separated by a ','", e.g. stat3, MULTI83,2
	Ctx ctx;
	CHECK(Eval(L"p7", ctx) == 7.0f);
	CHECK(Eval(L"P 7", ctx) == 7.0f);
	CHECK(Eval(L"p0", ctx) == 0.0f);
	CHECK(Eval(L"q83,2", ctx) == 83002.0f);
	CHECK(ctx.lastParams == std::vector<int>{ 83, 2 });
	CHECK(Eval(L"q 1 , 5", ctx) == 1005.0f);
	CHECK(Eval(L"p3 + p4 * 2", ctx) == 11.0f);
	// inside a call, the variable consumes exactly its own parameters
	CHECK(Eval(L"max(q1,2, 3)", ctx) == 1002.0f);
	CHECK(Eval(L"min(p9, 4)", ctx) == 4.0f);
	Eval(L"p2147483647", ctx);
	CHECK(ctx.lastParams == std::vector<int>{ 2147483647 });
}

TEST_CASE("parameterized variables reject missing or non-integer parameters") {
	CHECK(Rejected(L"p"));
	CHECK(Rejected(L"p + 1"));
	CHECK(Rejected(L"p(7)"));
	CHECK(Rejected(L"p1.5"));
	CHECK(Rejected(L"p-1"));
	CHECK(Rejected(L"p x"));
	CHECK(Rejected(L"q1"));
	CHECK(Rejected(L"q1 2"));
	CHECK(Rejected(L"q1,"));
	// beyond the int range
	CHECK(Rejected(L"p2147483648"));
	CHECK(Rejected(L"p99999999999"));
}

TEST_CASE("a failing variable makes the whole run fail") {
	Ctx ctx;
	float v = 0;
	CHECK(RunStatus(L"boom", ctx, v) == FormulaStatus::MATH_ERROR);
	CHECK(RunStatus(L"1 + boom", ctx, v) == FormulaStatus::MATH_ERROR);
	CHECK(RunStatus(L"boom * 0", ctx, v) == FormulaStatus::MATH_ERROR);
	CHECK(RunStatus(L"max(1, -boom)", ctx, v) == FormulaStatus::MATH_ERROR);
	CHECK(RunStatus(L"if(boom, 1, 2)", ctx, v) == FormulaStatus::MATH_ERROR);
	ctx.x = 0;
	CHECK(RunStatus(L"if(x, 1, boom)", ctx, v) == FormulaStatus::MATH_ERROR);
	// failing at run time is not a compile problem
	CHECK_FALSE(Rejected(L"boom"));
	// a failure does not stick to the compiled formula
	auto f = MustCompile(L"if(x, boom, 5)");
	ctx.x = 1;
	CHECK(f->execute(&ctx, v) == FormulaStatus::MATH_ERROR);
	ctx.x = 0;
	REQUIRE(f->execute(&ctx, v) == FormulaStatus::OK);
	CHECK(v == 5.0f);
}

TEST_CASE("truthiness is any value other than zero") {
	// the wiki's truthiness table
	CHECK(Formula<Ctx>::IsTrue(1.5f));
	CHECK(Formula<Ctx>::IsTrue(1.0f));
	CHECK_FALSE(Formula<Ctx>::IsTrue(0.0f));
	CHECK(Formula<Ctx>::IsTrue(0.0000001f));
	CHECK(Formula<Ctx>::IsTrue(-1.0f));
	CHECK_FALSE(Formula<Ctx>::IsTrue(-0.0f));
}

}  // TEST_SUITE("Formula")

TEST_SUITE("Formula loot filter") {

TEST_CASE("a Formula definition is referenced as FORMULA<KEY> in conditions") {
	support::LoadFilter(
		"Formula[STRONG]: STAT0 >= 20\n"
		"ItemDisplay[FORMULASTRONG]: Strong %NAME%\n");
	support::TestItem at("hax", ITEM_QUALITY_UNIQUE);
	at.Stat(STAT_STRENGTH, 20);
	support::TestItem below("hax", ITEM_QUALITY_UNIQUE);
	below.Stat(STAT_STRENGTH, 19);
	CHECK(Name(at) == "Strong Axe");
	CHECK(Name(below) == "Axe");
	CHECK(support::Matches(L"FORMULASTRONG", at));
	CHECK_FALSE(support::Matches(L"FORMULASTRONG", below));
	CHECK(support::Matches(L"!FORMULASTRONG", below));
}

TEST_CASE("a formula condition compares the real value, not a truncated integer") {
	support::LoadFilter("Formula[HALF]: STAT0 / 2\n");
	support::TestItem odd("hax", ITEM_QUALITY_RARE);
	odd.Stat(STAT_STRENGTH, 11);  // 5.5
	support::TestItem even("hax", ITEM_QUALITY_RARE);
	even.Stat(STAT_STRENGTH, 10);  // 5
	CHECK(support::Matches(L"FORMULAHALF>5", odd));
	CHECK_FALSE(support::Matches(L"FORMULAHALF>5", even));
	CHECK_FALSE(support::Matches(L"FORMULAHALF=5", odd));
	CHECK(support::Matches(L"FORMULAHALF=5", even));
	CHECK(support::Matches(L"FORMULAHALF<6", odd));
	CHECK_FALSE(support::Matches(L"FORMULAHALF<5", even));
	CHECK(support::Matches(L"FORMULAHALF~5-6", odd));
	CHECK(support::Matches(L"FORMULAHALF~5-6", even));
	CHECK_FALSE(support::Matches(L"FORMULAHALF~6-9", odd));
	// used alone it is a truthiness test: 5.5 != 0
	CHECK(support::Matches(L"FORMULAHALF", odd));
	support::TestItem none("hax", ITEM_QUALITY_RARE);
	CHECK_FALSE(support::Matches(L"FORMULAHALF", none));
}

TEST_CASE("formula conditions combine with other conditions") {
	support::LoadFilter("Formula[RES]: min(STAT39, STAT43) >= 30\n");
	support::TestItem ring("rin", ITEM_QUALITY_RARE);
	ring.Stat(STAT_FIRERESIST, 30).Stat(STAT_COLDRESIST, 35);
	CHECK(support::Matches(L"rin RARE FORMULARES", ring));
	CHECK_FALSE(support::Matches(L"rin UNI FORMULARES", ring));
	CHECK(support::Matches(L"UNI OR FORMULARES", ring));
	CHECK_FALSE(support::Matches(L"rin !FORMULARES", ring));
}

TEST_CASE("a formula that fails at run time never matches") {
	// SK999 is past the last skill id, which the SK variable reports as a math error
	support::LoadFilter("Formula[BAD]: SK999 + 1\n");
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK_FALSE(support::Matches(L"FORMULABAD", item));
	CHECK_FALSE(support::Matches(L"FORMULABAD<5", item));
	CHECK_FALSE(support::Matches(L"FORMULABAD=0", item));
	CHECK_FALSE(support::Matches(L"FORMULABAD~0-100", item));
}

TEST_CASE("a mixed-case formula reference in a condition still refers to the formula" * doctest::should_fail()) {
	// BUG: the wiki says "an input reference only requires the first letter (F) to be capitalized
	// the rest is case-insensitive" (listed under "Minor Details: Tidbits that may or may not
	// change"), but BuildConditions looks the key up verbatim in formulaMap, whose keys are
	// upper-cased. "Formulastrong" is not found and no condition is added, so the rule matches
	// items the formula rejects.
	support::LoadFilter("Formula[STRONG]: STAT0 >= 20\n");
	support::TestItem weak("hax", ITEM_QUALITY_UNIQUE);
	weak.Stat(STAT_STRENGTH, 5);
	CHECK_FALSE(support::Matches(L"Formulastrong", weak));
	CHECK_FALSE(support::Matches(L"FORMULAstrong", weak));
}

TEST_CASE("a formula's value is shown with %FORMULA<KEY>% in the item name") {
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.Stat(STAT_STRENGTH, 15).Stat(STAT_DEXTERITY, 10);
	CHECK(Show("%NAME% (%FORMULASUM%)", item, "Formula[SUM]: STAT0 + STAT2\n") == "Axe (25)");
	CHECK(Show("%FORMULA_A%/%FORMULA_B%", item, "Formula[_A]: STAT0 / 2\nFormula[_B]: STAT2 * 3\n") == "7.5/30");
	// wiki: "an output reference is always case-insensitive" (a "Minor Details" tidbit that may change)
	CHECK(Show("%formulasum%", item, "Formula[SUM]: STAT0 + STAT2\n") == "25");
	CHECK(Show("%FormulaSum%", item, "Formula[SUM]: STAT0 + STAT2\n") == "25");
}

TEST_CASE("formula output shows 0-2 decimals, dropping trailing zeros") {
	// the wiki's "Item Output" table
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(Render("1.49", item) == "<1.49>");
	CHECK(Render("1.495", item) == "<1.5>");
	CHECK(Render("1.494", item) == "<1.49>");
	CHECK(Render("1.5", item) == "<1.5>");
	CHECK(Render("TRUE", item) == "<1>");
	CHECK(Render("FALSE", item) == "<0>");
	CHECK(Render("0", item) == "<0>");
	CHECK(Render(".0000001", item) == "<0>");
	CHECK(Render("2/3", item) == "<0.67>");
	CHECK(Render("100", item) == "<100>");
	CHECK(Render("100.5", item) == "<100.5>");
	CHECK(Render("1.1", item) == "<1.1>");
	CHECK(Render("0.05", item) == "<0.05>");
	CHECK(Render("-7/2", item) == "<-3.5>");
	CHECK(Render("-42", item) == "<-42>");
}

TEST_CASE("a negative formula value that rounds to zero is shown as 0" * doctest::should_fail()) {
	// BUG: ReplaceBindFormula prints with "%.2f" and strips ".00", so any negative value that
	// rounds to zero ("-0.00") is shown as "-0". The wiki's "Item Output" table renders values that
	// round to zero (.0000001, 0, FALSE) as 0. An ordinary filter hits this: negating a stat the
	// item does not have, $f(-STAT5), evaluates to -0 and is shown as "-0".
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(Render("-STAT5", item) == "<0>");
	CHECK(Render("-.0000001", item) == "<0>");
}

TEST_CASE("formula output is f_err for math errors and values beyond 2^31") {
	// wiki: "If a math error occurs or the value is too large to be rendered, the text is f_err.
	// The maximum absolute value of a renderable result is 2^31 (2147483648)."
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(Render("1/0", item) == "<f_err>");
	CHECK(Render("-1/0", item) == "<f_err>");
	CHECK(Render("0/0", item) == "<f_err>");
	CHECK(Render("sqrt(-1)", item) == "<f_err>");
	CHECK(Render("ln(0)", item) == "<f_err>");
	CHECK(Render("SK999", item) == "<f_err>");
	CHECK(Render("2^31", item) == "<2147483648>");
	CHECK(Render("-(2^31)", item) == "<-2147483648>");
	// 2^31 + 256 is the next float above 2^31
	CHECK(Render("2^31 + 256", item) == "<f_err>");
	CHECK(Render("-(2^31) - 256", item) == "<f_err>");
	CHECK(Render("2^40", item) == "<f_err>");
}

TEST_CASE("Formula definitions that do not compile are skipped") {
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.Stat(STAT_STRENGTH, 5);
	// the reference is left as unknown text
	CHECK(Show("[%FORMULABAD%]", item, "Formula[BAD]: STAT0 +\n") == "[%FORMULABAD%]");
	CHECK(Show("[%FORMULABAD%]", item, "Formula[BAD]: nosuchvar\n") == "[%FORMULABAD%]");
	CHECK(Show("[%FORMULABAD%]", item, "Formula[BAD]: max()\n") == "[%FORMULABAD%]");
	// one broken definition does not disturb the others
	CHECK(Show("%FORMULAGOOD%", item, "Formula[BAD]: (\nFormula[GOOD]: STAT0 * 2\n") == "10");
}

TEST_CASE("reloading the filter forgets formulas that are no longer defined") {
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(Show("[%FORMULAX%]", item, "Formula[X]: 7\n") == "[7]");
	CHECK(Show("[%FORMULAX%]", item) == "[%FORMULAX%]");
	support::LoadFilter("Formula[X]: 1\n");
	CHECK(support::Matches(L"FORMULAX", item));
	support::LoadFilter("Formula[X]: 0\n");
	CHECK_FALSE(support::Matches(L"FORMULAX", item));
}

TEST_CASE("Formula definitions cannot use aliases") {
	// wiki: "Aliases cannot be used in the definition."
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.Stat(STAT_STRENGTH, 8);
	CHECK(Show("[%FORMULAX%]", item, "Alias[HALFSTR]: STAT0/2\nFormula[X]: HALFSTR\n") == "[%FORMULAX%]");
}

TEST_CASE("inline $f() in a condition works as a boolean and as a value comparison") {
	// the wiki's "Item Input" examples
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.Stat(1, 10).Stat(STAT_DEXTERITY, 11);  // STAT1 + STAT2 = 21
	support::LoadFilter("ItemDisplay[$f(STAT1+STAT2>20)]: yes\n");
	CHECK(Name(item) == "yes");
	support::LoadFilter("ItemDisplay[$f(STAT1+STAT2>21)]: yes\n");
	CHECK(Name(item) == "Axe");
	support::LoadFilter("ItemDisplay[$f(STAT1+STAT2)>20]: yes\n");
	CHECK(Name(item) == "yes");
	support::LoadFilter("ItemDisplay[$f(STAT1+STAT2)>21]: yes\n");
	CHECK(Name(item) == "Axe");
	// spaces inside the island do not split the condition
	support::LoadFilter("ItemDisplay[UNI $f( STAT1 + STAT2 == 21 ) hax]: yes\n");
	CHECK(Name(item) == "yes");
	support::LoadFilter("ItemDisplay[!$f(STAT1 + STAT2 == 21)]: yes\n");
	CHECK(Name(item) == "Axe");
}

TEST_CASE("inline $f() can be one term of an add condition") {
	// wiki: ItemDisplay[$f(STAT1)+STAT2>15] is an add condition; the formula's real value is added
	support::LoadFilter("ItemDisplay[$f(STAT0/2)+STAT2>15]: yes\n");
	support::TestItem over("hax", ITEM_QUALITY_UNIQUE);
	over.Stat(STAT_STRENGTH, 11).Stat(STAT_DEXTERITY, 10);  // 5.5 + 10 = 15.5
	support::TestItem under("hax", ITEM_QUALITY_UNIQUE);
	under.Stat(STAT_STRENGTH, 11).Stat(STAT_DEXTERITY, 9);  // 5.5 + 9 = 14.5
	support::TestItem exact("hax", ITEM_QUALITY_UNIQUE);
	exact.Stat(STAT_STRENGTH, 10).Stat(STAT_DEXTERITY, 10);  // 5 + 10 = 15, not > 15
	CHECK(Name(over) == "yes");
	CHECK(Name(under) == "Axe");
	CHECK(Name(exact) == "Axe");
}

TEST_CASE("a named formula can be one term of an add condition") {
	support::LoadFilter("Formula[Q]: STAT0 / 4\n");
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.Stat(STAT_STRENGTH, 10).Stat(STAT_DEXTERITY, 8);  // 2.5 + 8 = 10.5
	CHECK(support::Matches(L"FORMULAQ+STAT2>10", item));
	CHECK_FALSE(support::Matches(L"FORMULAQ+STAT2>11", item));
	CHECK(support::Matches(L"STAT2+FORMULAQ<11", item));
}

TEST_CASE("inline $f() in an action is replaced by the formula's value") {
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.Stat(STAT_STRENGTH, 20).Stat(STAT_DEXTERITY, 7);
	CHECK(Show("%NAME% [$f(STAT0 * 2)]", item) == "Axe [40]");
	CHECK(Show("$f(STAT0 + 1) and $f(STAT2/2)", item) == "21 and 3.5");
	// parentheses inside the island are matched
	CHECK(Show("[$f(max(STAT0, STAT2) * (1 + 1))]", item) == "[40]");
	CHECK(Show("[$f((STAT0))]", item) == "[20]");
	// text right after the island stays
	CHECK(Show("$f(1)$f(2)x", item) == "12x");
}

TEST_CASE("inline $f() that does not compile or is not closed stays as text") {
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(Show("[$f(1 +)]", item) == "[$f(1 +)]");
	CHECK(Show("[$f(nosuchvar)]", item) == "[$f(nosuchvar)]");
	CHECK(Show("[$f(1 + 2]", item) == "[$f(1 + 2]");
	// a broken island does not stop later islands from working
	CHECK(Show("$f(max()) $f(2*3)", item) == "$f(max()) 6");
	// wiki: "The 'f' must be lowercase"
	CHECK(Show("[$F(1+1)]", item) == "[$F(1+1)]");
}

TEST_CASE("inline $f() can use aliases") {
	// wiki: aliases can be used in an inline definition
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.Stat(STAT_STRENGTH, 13);
	support::LoadFilter("Alias[HALFSTR]: STAT0/2\nItemDisplay[$f(HALFSTR)>6]: [$f(%HALFSTR%)]\n");
	CHECK(Name(item) == "[6.5]");
	item.Stat(STAT_STRENGTH, 12);
	ResetCaches();  // BH caches names per item until something invalidates them
	CHECK(Name(item) == "Axe");
}

TEST_CASE("many inline formulas in one filter each keep their own value") {
	std::string action;
	std::string expected;
	for (int i = 1; i <= 10; i++) {
		action += "$f(" + std::to_string(i) + ")-";
		expected += std::to_string(i) + "-";
	}
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(Show(action, item) == expected);

	// 40 rules with two islands each: more islands than letters A-Z, so the generated
	// names must not repeat
	std::string filter;
	for (int i = 1; i <= 40; i++) {
		filter += "ItemDisplay[$f(STAT0==" + std::to_string(i) + ")]: v$f(STAT0*100+" + std::to_string(i) + ")\n";
	}
	support::LoadFilter(filter);
	support::TestItem a("hax", ITEM_QUALITY_UNIQUE);
	a.Stat(STAT_STRENGTH, 3);
	support::TestItem b("hax", ITEM_QUALITY_UNIQUE);
	b.Stat(STAT_STRENGTH, 14);  // 27th and 28th islands
	support::TestItem c("hax", ITEM_QUALITY_UNIQUE);
	c.Stat(STAT_STRENGTH, 40);
	support::TestItem d("hax", ITEM_QUALITY_UNIQUE);
	d.Stat(STAT_STRENGTH, 41);
	CHECK(Name(a) == "v303");
	CHECK(Name(b) == "v1414");
	CHECK(Name(c) == "v4040");
	CHECK(Name(d) == "Axe");
}

TEST_CASE("STAT and MULTI read the item's stats, with life and mana in whole points") {
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.Stat(STAT_VITALITY, 12)
		.Stat(STAT_MAXHP, 40 * 256)  // the game stores life in 1/256ths
		.Stat(STAT_CLASSSKILLS, 3, 2)  // +3 necromancer skills
		.Stat(STAT_CLASSSKILLS, 1, 4);  // +1 barbarian skills
	CHECK(Render("STAT3", item) == "<12>");
	CHECK(Render("stat 3", item) == "<12>");
	CHECK(Render("STAT7", item) == "<40>");
	CHECK(Render("LIFE", item) == "<40>");
	CHECK(Render("MULTI83,2", item) == "<3>");
	CHECK(Render("MULTI83,4", item) == "<1>");
	CHECK(Render("MULTI83,0", item) == "<0>");
	CHECK(Render("STAT5", item) == "<0>");
}

TEST_CASE("CHARSTAT, CLVL and DIFF read the player and the game, not the item") {
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.Stat(STAT_STRENGTH, 50).Stat(STAT_LEVEL, 3);
	fake::SetStat(&fake::Player(), STAT_STRENGTH, 120);
	fake::SetStat(&fake::Player(), STAT_LEVEL, 85);
	fake::SetDifficulty(2);
	CHECK(Render("CHARSTAT0", item) == "<120>");
	CHECK(Render("STAT0", item) == "<50>");
	CHECK(Render("CLVL", item) == "<85>");
	CHECK(Render("DIFF", item) == "<2>");
	fake::SetDifficulty(0);
	CHECK(Render("DIFF", item) == "<0>");
	// a level-dependent condition
	support::LoadFilter("ItemDisplay[$f(CLVL >= 80 + DIFF*2)]: late\n");
	fake::SetDifficulty(2);
	CHECK(Name(item) == "late");
	fake::SetStat(&fake::Player(), STAT_LEVEL, 83);
	ResetCaches();
	CHECK(Name(item) == "Axe");
}

TEST_CASE("item property variables: quality, flags and item level") {
	support::TestItem uni("hax", ITEM_QUALITY_UNIQUE);
	uni.ItemLevel(87).Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	support::TestItem rare("hax", ITEM_QUALITY_RARE);
	rare.ItemLevel(12).Flags(0);
	CHECK(Render("UNI*100 + RARE*10 + ETH*2 + ID", uni) == "<103>");
	CHECK(Render("UNI*100 + RARE*10 + ETH*2 + ID", rare) == "<10>");
	CHECK(Render("ILVL", uni) == "<87>");
	CHECK(Render("ILVL / 2", rare) == "<6>");
	CHECK(Render("TRUE + TRUE + FALSE", rare) == "<2>");
}

TEST_CASE("named-stat variables read the matching item stats") {
	support::TestItem item("rin", ITEM_QUALITY_RARE);
	item.Stat(STAT_FIRERESIST, 30)
		.Stat(STAT_LIGHTNINGRESIST, 20)
		.Stat(STAT_COLDRESIST, 25)
		.Stat(STAT_POISONRESIST, 15)
		.Stat(STAT_FASTERCAST, 10)
		.Stat(STAT_MAGICFIND, 24)
		.Stat(STAT_ALLSKILLS, 2)
		.Stat(STAT_SOCKETS, 1);
	CHECK(Render("FRES + LRES + CRES + PRES", item) == "<90>");
	// RES is all resistances: the smallest of the four
	CHECK(Render("RES", item) == "<15>");
	CHECK(Render("FCR", item) == "<10>");
	CHECK(Render("MFIND", item) == "<24>");
	CHECK(Render("ALLSK", item) == "<2>");
	CHECK(Render("SOCK", item) == "<1>");
	CHECK(Render("SOCKETS", item) == "<1>");
}

TEST_CASE("RES is 0 unless the item has every resistance") {
	support::TestItem item("rin", ITEM_QUALITY_RARE);
	item.Stat(STAT_FIRERESIST, 30).Stat(STAT_LIGHTNINGRESIST, 20).Stat(STAT_COLDRESIST, 25);
	CHECK(Render("RES", item) == "<0>");
}

TEST_CASE("skill variables read single, oskill and class skill bonuses") {
	support::TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.Stat(STAT_SINGLESKILL, 3, 54)  // +3 Teleport
		.Stat(STAT_NONCLASSSKILL, 1, 54)  // +1 Teleport (oskill)
		.Stat(STAT_CLASSSKILLS, 2, 1);  // +2 sorceress skills
	CHECK(Render("SK54", item) == "<3>");
	CHECK(Render("OS54", item) == "<1>");
	CHECK(Render("SK53", item) == "<0>");
	CHECK(Render("CLSK1", item) == "<2>");
	CHECK(Render("CLSK6", item) == "<0>");
	// class ids run 0 (Amazon) to 6 (Assassin)
	CHECK(Render("CLSK7", item) == "<f_err>");
	CHECK(Render("OS999", item) == "<f_err>");
}

}  // TEST_SUITE("Formula loot filter")
