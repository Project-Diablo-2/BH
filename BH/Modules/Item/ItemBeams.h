#pragma once
// Light beams above ground items (%BEAM-XX% permanent, %FLASH-XX% for ~2 s after the item drops).
#include <Windows.h>

#include <unordered_map>
#include <vector>

namespace ItemBeams {

const DWORD kFlashMs = 2000;   // a %FLASH% beam fades out over this long after the drop
const int kMaxBeams = 16;      // at most this many beams (the ones nearest the player)
const int kBeamHeight = 150;   // pixels above the item's ground point, at full intensity
const int kMaxIntensity = 255;

// D2GFX_DrawRectangle draw modes the beam uses.
const int kModeTrans25 = 0;
const int kModeTrans50 = 1;

// Intensity (0 = not drawn .. kMaxIntensity) of a %FLASH% beam `ageMs` after its item dropped.
int FlashIntensity(DWORD ageMs);

// The beam's look, all in 8-bit palette blends (D2GFX_DrawRectangle has no additive mode on the
// DirectDraw renderer: modes 0/1/2 blend 25/50/75 % of the colour, 7/8 are opaque, 3 draws nothing,
// 4/6 black; measured in game). Light is built from stacked translucent layers of 1 px columns: an
// outer glow, an inner glow and a core of the beam colour, a white-hot centre line, a light pool and
// a white flare where the beam meets the ground. Each column is cut to its own height (a shallow
// parabola across the beam, so the sides end lower than the middle) and ends in a dithered tail (every
// other row), so neither the sides nor the top show bands. Two short bright sparks rise slowly
// through the core and the ground pool flickers gently.
const int kWhite = 0xFF;          // palette index of the white-hot parts (white in every act palette)
const DWORD kRisePxPerSec = 45;   // speed of the sparks rising in the core
const int kSparkLength = 12;
const int kDitherRows = 8;        // rows of the dithered end of each column
// The part of a beam within this many pixels above the ground point is drawn with the world, before
// ground items and units, so the item (and anyone standing in front) covers the beam's foot; the rest
// is drawn over the world (see Draw).
const int kBaseHeight = 40;

// Gentle flicker of the ground pool at time nowMs (-64 .. 64, smooth: two slow sines, out of phase
// per item via seed).
int Flicker(DWORD nowMs, DWORD seed);

// y (pixels above the ground point, negative) of the bottom of rising spark k (0 or 1) of a beam of
// the given height at time nowMs: moves up at kRisePxPerSec, wraps around at the top.
int SparkY(DWORD nowMs, DWORD seed, int k, int height);

// One rectangle of a beam, in pixels relative to the item's ground point (y grows downwards):
// x0 <= x < x1, y0 <= y < y1, drawn with D2GFX_DrawRectangle draw mode `mode` in the beam's colour,
// or in kWhite when `white`.
struct BeamRect {
	int x0;
	int y0;
	int x1;
	int y1;
	int mode;
	bool white;
};
const int kMaxBeamRects = 192;

// The rectangles of one beam at the given intensity at time nowMs (seed: per item, so beams do not
// move in step), back to front; writes at most maxRects, returns the count (0 when intensity is 0).
int BeamRects(int intensity, DWORD nowMs, DWORD seed, BeamRect* out, int maxRects);

// The parts of the rectangles at or below splitY (lower = true: y >= splitY) or above it (y <
// splitY), in order; rectangles crossing the line are cut. Writes at most maxOut, returns the count.
int SplitRects(const BeamRect* in, int n, int splitY, bool lower, BeamRect* out, int maxOut);

// Which items get a %FLASH%: one that dropped while the player watched (the client marks fresh
// drops ITEM_NEW), not one that was already lying there when its room came into view. The flash
// starts when the item is first seen.
class FlashTracker {
public:
	// Records the item (first sighting or still there). Returns true and sets *startMs to the flash
	// start when the item flashes (its age decides whether the flash is still visible).
	bool Observe(DWORD unitId, bool newDrop, DWORD nowMs, DWORD* startMs);
	// Forgets items not observed for kForgetMs (picked up, out of range) so the table stays small.
	void Prune(DWORD nowMs);
	void Clear();
	size_t Size() const { return seen.size(); }

	static const DWORD kForgetMs = 30000;

private:
	struct Entry {
		DWORD firstSeen;
		DWORD lastSeen;
		bool flashes;
	};
	std::unordered_map<DWORD, Entry> seen;
};

// A beam candidate: its distance from the player (squared, map units) and what to draw.
struct Candidate {
	long distSq;
	long screenX;
	long screenY;
	int color;
	int intensity;
	DWORD seed;
};

// Keeps the `cap` candidates nearest the player (ties: any of them); order is unspecified.
void SelectNearest(std::vector<Candidate>& candidates, size_t cap);

// The screen columns x0 <= x < x1 where the game world is visible, by the engine's own rule for ground
// labels (D2Client 0x58FB0 / GameDraw 0xC3D56): side panels cover half the screen each. `covered` is
// D2Client's ScreenCovered (0 none, 1 right panel open, 2 left panel open, 3 both); `screenWidth`
// the game's screen width (D2Client ScreenSizeX, e.g. 800 or 1068). Empty (x0 == x1) when both sides
// are covered.
struct ScreenSpan {
	long x0;
	long x1;
};
ScreenSpan VisibleSpan(DWORD covered, long screenWidth);

// Screen position of a ground point whose absolute screen coordinates (D2COMMON_MapToAbsScreen) are
// (absX, absY), placed the way D2Client places ground labels (0x5912D): minus the view's mouse offset
// (D2Client MouseOffsetX/Y), plus the horizontal shift of the world view while one side panel is open
// (D2Client ViewShiftX, set with ScreenCovered at 0x3FF90: -width/4 with a right panel, +width/4 with
// a left one, 0 with none or both).
struct ScreenPoint {
	long x;
	long y;
};
ScreenPoint GroundToScreen(long absX, long absY, long mouseOffsetX, long mouseOffsetY, long viewShiftX);

// Clips the horizontal extent x0 <= x < x1 of a rectangle to the span; false when nothing is left.
bool ClipToSpan(const ScreenSpan& span, long* x0, long* x1);

// Draws the beams of the visible ground items, in two passes per frame:
// - DrawWorldPass, from the world draw (after the floor, D2Client 0x8B235, before the ground items
//   and units; see InstallWorldPass): the foot of every beam (kBaseHeight), under the items.
// - Draw: everything else, over the world. Runs at most once per frame (until EndFrame), from BH's
//   draw hook (MapNotify::OnDraw) or, first, from the hook before the ground labels so the labels
//   end up over the beams. Without a world pass this frame (hook not installed) it draws whole beams.
// Beams never cover UI: they are clipped to VisibleSpan and not drawn at all while a full-screen UI
// hides the world (game menu, hotkey config, help, NPC dialog, skill picker, gold dialog).
void Draw();
void DrawWorldPass();
// Hooks the world draw for DrawWorldPass (1.13c, byte-checked: the site must be a call). False with
// the reason when it cannot; beams then draw whole in Draw.
bool InstallWorldPass(const char** why);
void UninstallWorldPass();
// End of BH's frame: the next Draw draws again.
void EndFrame();
// New game: forget every item.
void Reset();

}  // namespace ItemBeams
