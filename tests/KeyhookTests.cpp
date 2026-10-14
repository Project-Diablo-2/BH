// Drawing::Keyhook (Drawing/Advanced/Keyhook): a hotkey setting in BH's settings window. It shows
// "<name> <key>"; clicking it starts listening, and the next key released becomes the hotkey (Escape
// clears it).
#include <string>

#include "doctest/doctest.h"

#include "Drawing.h"
#include "FakeEngine.h"

using Drawing::Keyhook;

namespace {

// The settings window a hotkey setting sits in, at the screen origin. Closed when its test ends.
struct Window : Drawing::HookGroup {
	bool open = true;
	unsigned int GetX() override { return 0; }
	unsigned int GetY() override { return 0; }
	unsigned int GetXSize() override { return 800; }
	unsigned int GetYSize() override { return 600; }
	bool IsActive() override { return open; }
};

// A hotkey setting at (100, 50) bound to `key`, in its own settings window. Like every hotkey
// setting BH creates (the settings tabs in Item.cpp), it has no name: it shows just the key, 8px per
// character with the fake font, so "K" covers x 100..108, y 50..60.
//
// Hooks register themselves with Hook for the life of the process and cannot be deleted, so the
// window, the hook and the bound key are deliberately leaked: nothing a hook points at is ever freed.
// When the test ends the window is closed, which makes the hook inert for Hook::LeftClick/KeyClick,
// and window members are never drawn by Hook::Draw.
struct Setting {
	Window* window;
	unsigned int& key;
	Keyhook* hook;
	explicit Setting(unsigned int initialKey) : window(new Window()), key(*new unsigned int(initialKey)) {
		hook = new Keyhook(window, 100, 50, &key, "");
	}
	~Setting() {
		if (Listening()) {
			hook->OnKey(true, static_cast<BYTE>(key), 0);
		}
		window->open = false;
	}
	Keyhook* operator->() {
		return hook;
	}
	bool Click(unsigned int x, unsigned int y) {
		bool down = hook->OnLeftClick(false, x, y);
		hook->OnLeftClick(true, x, y);
		return down;
	}
	// Whether a key press would be taken as the new hotkey (without taking it).
	bool Listening() {
		return hook->OnKey(false, 0, 0);
	}
	// What the setting shows while it is not listening.
	std::wstring Shown() {
		size_t before = fake::Drawn().size();
		hook->OnDraw();
		REQUIRE(fake::Drawn().size() == before + 1);
		return fake::Drawn().back().text;
	}
};

}  // namespace

TEST_SUITE("Keyhook") {
	TEST_CASE("shows the bound key by its name") {
		Setting letter('K');
		CHECK(letter.Shown() == L"K");
		Setting function(VK_F11);
		CHECK(function.Shown() == L"F11");
		Setting numpad(VK_NUMPAD0);
		CHECK(numpad.Shown() == L"Numpad 0");
	}

	TEST_CASE("an unbound hotkey shows as Not Set") {
		Setting setting(0);
		CHECK(setting.Shown() == L"Not Set");
	}

	TEST_CASE("the key is highlighted while the mouse is over the setting") {
		Setting setting('K');
		fake::Var(Var_D2CLIENT_MouseX) = 104;
		fake::Var(Var_D2CLIENT_MouseY) = 55;
		setting.Shown();
		CHECK(fake::Drawn().back().color == Tan);

		fake::Var(Var_D2CLIENT_MouseX) = 10;
		setting.Shown();
		CHECK(fake::Drawn().back().color == Gold);
	}

	TEST_CASE("clicking the setting starts listening; the next key released becomes the hotkey") {
		Setting setting('K');
		CHECK_FALSE(setting.Listening());
		CHECK(setting.Click(104, 55));
		CHECK(setting.Listening());

		// The key press is swallowed; the hotkey changes when the key comes up.
		CHECK(setting->OnKey(false, 'J', 0));
		CHECK(setting.key == 'K');
		CHECK(setting->OnKey(true, 'J', 0));
		CHECK(setting.key == 'J');

		CHECK_FALSE(setting.Listening());
		CHECK_FALSE(setting->OnKey(true, 'L', 0));
		CHECK(setting.key == 'J');
	}

	TEST_CASE("Escape while listening clears the hotkey") {
		Setting setting('K');
		setting.Click(104, 55);
		setting->OnKey(true, VK_ESCAPE, 0);
		CHECK(setting.key == 0);
		CHECK_FALSE(setting.Listening());
	}

	TEST_CASE("clicking again while listening stops listening and keeps the hotkey") {
		Setting setting('K');
		setting.Click(104, 55);
		setting.Click(104, 55);
		CHECK_FALSE(setting.Listening());
		CHECK_FALSE(setting->OnKey(true, 'J', 0));
		CHECK(setting.key == 'K');
	}

	TEST_CASE("clicks outside the shown key are not taken and do not start listening") {
		Setting setting('K');
		CHECK(setting.Click(108, 60));  // the far corner is still the setting...
		setting.Click(108, 60);         // (stop listening again)
		CHECK_FALSE(setting.Click(99, 55));
		CHECK_FALSE(setting.Click(109, 55));
		CHECK_FALSE(setting.Click(104, 49));
		CHECK_FALSE(setting.Click(104, 61));
		CHECK_FALSE(setting.Listening());
	}
}
