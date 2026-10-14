// AsyncDrawBuffer / DrawDirective (AsyncDrawBuffer.cpp): the double-buffered draw list the automap
// item markers are drawn from (MapNotify: a synchronous DrawDirective with maxGhost 5, whose director
// pushes one top-layer draw call per marker). A frame is built in the back buffer and shown only after
// a swap; the directive rebuilds its frame only every few game frames (redrawing the last one in
// between, at most `maxGhost` times), or straight away after forceUpdate() (MapNotify forces one when
// the player changes act).
#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "doctest/doctest.h"

#include "AsyncDrawBuffer.h"

namespace {

// Draw calls that record what they drew.
struct Canvas {
	std::vector<std::string> drawn;
	std::function<void()> Draw(const std::string& what) {
		return [this, what]() { drawn.push_back(what); };
	}
};

// Runs `frames` game frames of a synchronous DrawDirective whose director builds frame number
// 1, 2, 3... each time it runs. Returns what was on screen each frame ("" = nothing).
std::vector<std::string> Frames(DrawDirective& directive, int frames, std::vector<int> forceBeforeFrame = {}) {
	Canvas canvas;
	int built = 0;
	std::vector<std::string> shown;
	for (int frame = 1; frame <= frames; frame++) {
		if (std::find(forceBeforeFrame.begin(), forceBeforeFrame.end(), frame) != forceBeforeFrame.end()) {
			directive.forceUpdate();
		}
		canvas.drawn.clear();
		directive.draw([&](AsyncDrawBuffer& buffer) {
			built++;
			buffer.push_top_layer(canvas.Draw(std::to_string(built)));
		});
		shown.push_back(canvas.drawn.empty() ? "" : canvas.drawn[0]);
	}
	return shown;
}

std::vector<std::string> S(std::initializer_list<const char*> values) {
	return std::vector<std::string>(values.begin(), values.end());
}

}  // namespace

TEST_SUITE("AsyncDrawBuffer") {
	TEST_CASE("draw calls appear only after the buffers are swapped, in the order they were pushed") {
		Canvas canvas;
		AsyncDrawBuffer buffer;
		buffer.push_top_layer(canvas.Draw("a"));
		buffer.push_top_layer(canvas.Draw("b"));
		buffer.drawAll();
		CHECK(canvas.drawn.empty());

		buffer.swapBuffers();
		buffer.drawAll();
		CHECK(canvas.drawn == S({ "a", "b" }));
	}

	TEST_CASE("the shown frame is drawn again every time until the next swap") {
		Canvas canvas;
		AsyncDrawBuffer buffer;
		buffer.push_top_layer(canvas.Draw("a"));
		buffer.swapBuffers();
		buffer.push_top_layer(canvas.Draw("b"));  // the next frame, still being built
		buffer.drawAll();
		buffer.drawAll();
		CHECK(canvas.drawn == S({ "a", "a" }));
	}

	TEST_CASE("clear discards the frame being built, not the one shown") {
		Canvas canvas;
		AsyncDrawBuffer buffer;
		buffer.push_top_layer(canvas.Draw("shown"));
		buffer.swapBuffers();
		buffer.push_top_layer(canvas.Draw("discarded"));
		buffer.clear();
		buffer.drawAll();
		CHECK(canvas.drawn == S({ "shown" }));

		canvas.drawn.clear();
		buffer.swapBuffers();
		buffer.drawAll();
		CHECK(canvas.drawn.empty());
	}

	TEST_CASE("each rebuild replaces the previous frame instead of adding to it") {
		DrawDirective directive(true, 5);
		Canvas canvas;
		for (int frame = 0; frame < 3; frame++) {
			canvas.drawn.clear();
			directive.forceUpdate();
			directive.draw([&](AsyncDrawBuffer& buffer) {
				buffer.push_top_layer(canvas.Draw("marker"));
			});
		}
		CHECK(canvas.drawn == S({ "marker" }));
	}

	TEST_CASE("the automap directive rebuilds its frame every 6 frames and redraws it in between") {
		DrawDirective directive(true, 5);  // as MapNotify's automap markers
		std::vector<std::string> shown = Frames(directive, 25);
		// The first frame is built within maxGhost + 2 frames...
		auto first = std::find(shown.begin(), shown.end(), "1");
		REQUIRE(first != shown.end());
		CHECK(first - shown.begin() <= 6);
		// ...and from then on each frame is shown 6 times (built once, redrawn maxGhost times).
		REQUIRE(shown.end() - first >= 18);
		std::vector<std::string> steady(first, first + 18);
		CHECK(steady == S({ "1", "1", "1", "1", "1", "1", "2", "2", "2", "2", "2", "2", "3", "3", "3", "3", "3", "3" }));
	}

	TEST_CASE("forceUpdate rebuilds on the very next frame") {
		DrawDirective directive(true, 5);
		// Force the first frame and again on frame 3, well before the 6-frame refresh.
		std::vector<std::string> shown = Frames(directive, 5, { 1, 3 });
		CHECK(shown == S({ "1", "1", "2", "2", "2" }));
	}
}
