// A doctest listener that calls `reset` before every test case (and before each subcase re-run),
// so no test sees state left behind by another. Register with
//   REGISTER_LISTENER("name", 1, ResetEachTest<&MyReset>);
#pragma once
#include "doctest/doctest.h"

template <void (*reset)()>
struct ResetEachTest : doctest::IReporter {
	ResetEachTest(const doctest::ContextOptions&) {}
	void report_query(const doctest::QueryData&) override {}
	void test_run_start() override {}
	void test_run_end(const doctest::TestRunStats&) override {}
	void test_case_start(const doctest::TestCaseData&) override { reset(); }
	void test_case_reenter(const doctest::TestCaseData&) override { reset(); }
	void test_case_end(const doctest::CurrentTestCaseStats&) override {}
	void test_case_exception(const doctest::TestCaseException&) override {}
	void subcase_start(const doctest::SubcaseSignature&) override {}
	void subcase_end() override {}
	void log_assert(const doctest::AssertData&) override {}
	void log_message(const doctest::MessageData&) override {}
	void test_case_skipped(const doctest::TestCaseData&) override {}
};
