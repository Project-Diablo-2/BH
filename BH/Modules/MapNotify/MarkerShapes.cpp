#include "MarkerShapes.h"

#include <cstdlib>

#include "../../D2Ptrs.h"
#include "../../Drawing/Hook.h"
#include "../Item/ItemDisplay.h"

namespace MarkerShapes {

namespace {

// Hand-drawn stars (a 5-point star polygon does not survive rasterisation at these sizes).
// Row strings, top to bottom; '#' = pixel set.
const char* const kStar8[] = {
	"...##...",
	"...##...",
	"########",
	".######.",
	"..####..",
	"..####..",
	".##..##.",
	".#....#.",
};
const char* const kStar6[] = {
	"..##..",
	"######",
	".####.",
	".####.",
	".#..#.",
	"#....#",
};
const char* const kStar4[] = {
	".##.",
	"####",
	".##.",
	"#..#",
};

const char* const* StarRows(int size) {
	switch (size) {
	case 8: return kStar8;
	case 6: return kStar6;
	case 4: return kStar4;
	default: return nullptr;
	}
}

}  // namespace

bool Contains(int shape, int size, int dx, int dy) {
	// Doubled coordinates of the pixel centre relative to the box centre: the box spans
	// -size < U, V < size, so every test below stays in integers.
	const int U = 2 * dx + 1;
	const int V = 2 * dy + 1;
	if (size <= 0 || U <= -size || U >= size || V <= -size || V >= size)
		return false;
	const int au = std::abs(U);
	const int av = std::abs(V);
	switch (shape) {
	case ICON_CIRCLE:
		return U * U + V * V <= size * size;
	case ICON_DIAMOND:
		return au + av <= size;
	case ICON_TRIANGLE:
		// Points up: the half width grows linearly from the top row to the full box at the bottom.
		return 2 * au <= V + size + 2;
	case ICON_CROSS: {
		// A plus with arms a quarter of the box thick (at least 2 pixels).
		const int arm = size > 4 ? size : 4;
		return 4 * au <= arm || 4 * av <= arm;
	}
	case ICON_STAR: {
		const char* const* rows = StarRows(size);
		if (!rows)  // 2 px: no room for a star, a dot is the best a 2x2 box can do
			return true;
		return rows[dy + size / 2][dx + size / 2] == '#';
	}
	default:
		return true;
	}
}

int Rasterize(int shape, int size, Span* out, int maxSpans) {
	int n = 0;
	const int half = size / 2;
	for (int dy = -half; dy < half; dy++) {
		int dx = -half;
		while (dx < half) {
			if (!Contains(shape, size, dx, dy)) {
				dx++;
				continue;
			}
			const int x0 = dx;
			while (dx < half && Contains(shape, size, dx, dy))
				dx++;
			if (n == maxSpans)
				return n;
			out[n++] = Span{ dy, x0, dx };
		}
	}
	return n;
}

int DrawSize(int shape, int size, int largestSize) {
	if (shape == ICON_SQUARE || largestSize >= kMinShapeSize)
		return size;
	return size + (kMinShapeSize - largestSize);
}

void DrawMarker(int cx, int cy, int shape, int borderColor, int mapColor, int dotColor, int pxColor) {
	struct Layer {
		int size;
		int color;
	};
	const Layer layers[] = { { 8, borderColor }, { 6, mapColor }, { 4, dotColor }, { 2, pxColor } };
	int largest = 0;
	for (const Layer& layer : layers) {
		if (layer.color != UNDEFINED_COLOR) {
			largest = layer.size;
			break;
		}
	}
	Span spans[16];
	for (const Layer& layer : layers) {
		if (layer.color == UNDEFINED_COLOR)
			continue;
		const int n = Rasterize(shape, DrawSize(shape, layer.size, largest), spans, 16);
		for (int i = 0; i < n; i++) {
			D2GFX_DrawRectangle(cx + spans[i].x0, cy + spans[i].dy, cx + spans[i].x1, cy + spans[i].dy + 1,
				layer.color, Drawing::BTHighlight);
		}
	}
}

}  // namespace MarkerShapes
