#pragma once
// Automap item marker shapes (%ICON-<shape>%, IconShape in ItemDisplay.h).
//
// A marker of size s (8 %BORDER%, 6 %MAP%, 4 %DOT%, 2 %PX%) covers the same s x s box as today's
// square: pixels (cx + dx, cy + dy) with -s/2 <= dx, dy < s/2, where (cx, cy) is the item's automap
// position. Every shape is rasterised inside that box, so the layers stay concentric.
namespace MarkerShapes {

// Shapes other than the square are never drawn smaller than this: a marker whose largest layer is
// smaller (a lone %DOT% or %PX%) grows all its layers by the difference, so the shape stays readable.
const int kMinShapeSize = 6;

// One row of a rasterised shape: pixels cx + x0 .. cx + x1 - 1 on row cy + dy.
struct Span {
	int dy;
	int x0;
	int x1;
};

// Whether the pixel (dx, dy) relative to the marker centre belongs to the shape of size `size`
// (an even number of pixels). Unknown shapes are squares.
bool Contains(int shape, int size, int dx, int dy);

// The shape as row spans, top to bottom, left to right; writes at most maxSpans, returns the count.
int Rasterize(int shape, int size, Span* out, int maxSpans);

// The size a layer of nominal size `size` is drawn at, when the marker's largest layer is
// `largestSize` (see kMinShapeSize). Squares keep their size.
int DrawSize(int shape, int size, int largestSize);

// Draws an item's automap marker layers (palette colours; UNDEFINED_COLOR = layer not set) in a
// non-square shape, largest first, centred on (cx, cy).
void DrawMarker(int cx, int cy, int shape, int borderColor, int mapColor, int dotColor, int pxColor);

}  // namespace MarkerShapes
