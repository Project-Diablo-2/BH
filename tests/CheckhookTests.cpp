// Drawing::Checkhook (Drawing/Advanced/Checkhook): the checkboxes in BH's windows (e.g. "Autoparty
// Enabled" on the party screen). Clicking the box or its label flips the bound setting when the
// mouse button comes up; the click is swallowed so the game does not see it.
#include <string>
#include <vector>

#include "doctest/doctest.h"

#include "Drawing.h"
#include "FakeEngine.h"

using Drawing::Checkhook;

namespace {

// Hooks register themselves with Hook for the life of the process and cannot be deleted, so
// everything a checkbox points at (its setting, its group) is given storage that is never freed
// (`Leaked<T>()`), and every checkbox is hidden, or its group closed, when its test ends: hidden
// checkboxes are neither drawn nor clicked by Hook::Draw/LeftClick.
template <class T>
T& Leaked() {
	return *new T();
}

// A settings panel the checkbox can sit in.
struct Panel : Drawing::HookGroup {
	unsigned int x = 200, y = 100;
	bool active = true;
	unsigned int GetX() override { return x; }
	unsigned int GetY() override { return y; }
	unsigned int GetXSize() override { return 300; }
	unsigned int GetYSize() override { return 200; }
	bool IsActive() override { return active; }
};

bool Click(Checkhook* box, unsigned int x, unsigned int y) {
	bool down = box->OnLeftClick(false, x, y);
	bool up = box->OnLeftClick(true, x, y);
	CHECK(down == up);
	return up;
}

}  // namespace

TEST_SUITE("Checkhook") {
	TEST_CASE("clicking the box flips the setting when the button comes up, and swallows the click") {
		bool& setting = Leaked<bool>();
		Checkhook* box = new Checkhook(Drawing::InGame, 100, 100, &setting, "Autoparty Enabled");
		CHECK(box->OnLeftClick(false, 105, 105));
		CHECK_FALSE(setting);
		CHECK(box->OnLeftClick(true, 105, 105));
		CHECK(setting);
		CHECK(box->IsChecked());

		Click(box, 105, 105);
		CHECK_FALSE(setting);
		box->SetActive(false);
	}

	TEST_CASE("the label is part of the checkbox; clicks beside it are not taken") {
		bool& setting = Leaked<bool>();
		// 20px for the box and the gap, then the label ("Loot" is 4 characters of 8px).
		Checkhook* box = new Checkhook(Drawing::InGame, 100, 100, &setting, "%s", "Loot");
		CHECK(Click(box, 100 + 20 + 32, 112));
		CHECK(setting);

		CHECK_FALSE(Click(box, 100 + 20 + 33, 105));
		CHECK_FALSE(Click(box, 99, 105));
		CHECK_FALSE(Click(box, 105, 99));
		CHECK_FALSE(Click(box, 105, 113));
		CHECK(setting);
		box->SetActive(false);
	}

	TEST_CASE("a hidden checkbox ignores clicks") {
		bool& setting = Leaked<bool>();
		Checkhook* box = new Checkhook(Drawing::InGame, 100, 100, &setting, "Loot");
		box->SetActive(false);
		CHECK_FALSE(Click(box, 105, 105));
		CHECK_FALSE(setting);
	}

	TEST_CASE("a checkbox in a panel is placed relative to the panel and follows it") {
		bool& setting = Leaked<bool>();
		Panel& panel = Leaked<Panel>();
		Checkhook* box = new Checkhook(&panel, 10, 20, &setting, "Loot");
		CHECK(box->GetX() == 210);
		CHECK(box->GetY() == 120);
		CHECK_FALSE(Click(box, 15, 25));
		CHECK(Click(box, 215, 125));
		CHECK(setting);

		panel.x = 400;
		CHECK_FALSE(Click(box, 215, 125));
		CHECK(Click(box, 415, 125));
		CHECK_FALSE(setting);

		// A closed panel's checkboxes take no clicks.
		panel.active = false;
		CHECK_FALSE(Click(box, 415, 125));
		CHECK_FALSE(setting);
	}
}
