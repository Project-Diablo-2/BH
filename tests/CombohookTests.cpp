// Drawing::Combohook (Drawing/Advanced/Combohook): the drop-down lists in BH's settings, such as the
// loot filter level. Clicking the closed box opens the list below it; clicking an entry selects it
// and closes the list; clicking anywhere else closes it without changing the selection.
#include <string>
#include <vector>

#include "doctest/doctest.h"

#include "Drawing.h"
#include "FakeEngine.h"

using Drawing::Combohook;

namespace {

// The settings window a drop-down sits in, at the screen origin. Closed when its test ends.
struct Window : Drawing::HookGroup {
	bool open = true;
	unsigned int GetX() override { return 0; }
	unsigned int GetY() override { return 0; }
	unsigned int GetXSize() override { return 800; }
	unsigned int GetYSize() override { return 600; }
	bool IsActive() override { return open; }
};

// A drop-down at (100, 100), 200px wide, in font 0 (10px high): the closed box covers y 100..113
// and entry n of the open list covers y 114 + 14n .. 128 + 14n.
//
// Hooks register themselves with Hook for the life of the process and cannot be deleted, so the
// window, the hook and the bound index are deliberately leaked: nothing a hook points at is ever
// freed. When the test ends the window is closed, which makes the hook inert for Hook::LeftClick,
// and window members are never drawn by Hook::Draw.
struct Dropdown {
	Window* window;
	unsigned int& selected;
	Combohook* hook;
	explicit Dropdown(unsigned int initial = 1) : window(new Window()), selected(*new unsigned int(initial)) {
		hook = new Combohook(window, 100, 100, 200, &selected, { "0 - Show All", "1 - Standard", "2 - Strict" });
	}
	~Dropdown() {
		window->open = false;
	}
	// Press and release at (x, y); returns whether the release was taken.
	bool Click(unsigned int x, unsigned int y) {
		hook->OnLeftClick(false, x, y);
		return hook->OnLeftClick(true, x, y);
	}
	bool Open() {
		return Click(150, 105);
	}
	static unsigned int EntryY(unsigned int n) {
		return 114 + 14 * n + 7;
	}
};

}  // namespace

TEST_SUITE("Combohook") {
	TEST_CASE("clicking the closed box opens the list; picking an entry selects it and closes the list") {
		Dropdown list(1);
		CHECK(list.Open());
		CHECK(list.Click(150, Dropdown::EntryY(2)));
		CHECK(list.selected == 2);

		// Closed again: the same spot is below the box now, so the click is not taken.
		CHECK_FALSE(list.Click(150, Dropdown::EntryY(2)));
		CHECK(list.selected == 2);
	}

	TEST_CASE("each entry of the open list selects its own index") {
		for (unsigned int n = 0; n < 3; n++) {
			CAPTURE(n);
			Dropdown list(1);
			list.Open();
			CHECK(list.Click(150, Dropdown::EntryY(n)));
			CHECK(list.selected == n);
		}
	}

	TEST_CASE("the list opens and the entry is chosen when the mouse button comes up") {
		Dropdown list(1);
		CHECK(list.hook->OnLeftClick(false, 150, 105));
		CHECK(list.hook->OnLeftClick(true, 150, 105));
		CHECK(list.hook->OnLeftClick(false, 150, Dropdown::EntryY(0)));
		CHECK(list.selected == 1);
		CHECK(list.hook->OnLeftClick(true, 150, Dropdown::EntryY(0)));
		CHECK(list.selected == 0);
	}

	TEST_CASE("clicking outside the open list closes it and leaves the selection") {
		Dropdown list(1);
		list.Open();
		CHECK_FALSE(list.Click(400, Dropdown::EntryY(0)));
		CHECK(list.selected == 1);
		// Closed: the entries no longer react.
		CHECK_FALSE(list.Click(150, Dropdown::EntryY(0)));
		CHECK(list.selected == 1);
	}

	TEST_CASE("clicking the box of the open list closes it and leaves the selection") {
		Dropdown list(1);
		list.Open();
		CHECK(list.Click(150, 105));
		CHECK(list.selected == 1);
		CHECK_FALSE(list.Click(150, Dropdown::EntryY(0)));
		CHECK(list.selected == 1);
	}

	TEST_CASE("clicks around the closed box are not taken") {
		Dropdown list(1);
		CHECK_FALSE(list.Click(99, 105));
		CHECK_FALSE(list.Click(150, 99));
		CHECK_FALSE(list.Click(150, 114));
		CHECK_FALSE(list.Click(306, 105));
		// ...and do not open the list.
		CHECK_FALSE(list.Click(150, Dropdown::EntryY(0)));
		CHECK(list.selected == 1);
	}
}
