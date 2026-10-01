#pragma once
// Light beams above ground items (%BEAM-XX% permanent, %FLASH-XX% for ~2 s after the item drops).
#include <Windows.h>

#include <unordered_map>
#include <vector>

namespace ItemBeams {

const DWORD kFlashMs = 2000;   // a %FLASH% beam fades out over this long after the drop
const DWORD kPulseMs = 1600;   // period of the slow pulse of every beam
const int kMaxBeams = 16;      // at most this many beams (the ones nearest the player)
const int kBeamHeight = 150;   // pixels above the item's ground point, at full intensity
const int kMaxIntensity = 255;

// D2GFX_DrawRectangle draw modes the beam uses.
const int kModeTrans25 = 0;
const int kModeTrans50 = 1;

// Intensity (0 = not drawn .. kMaxIntensity) of a %FLASH% beam `ageMs` after its item dropped.
int FlashIntensity(DWORD ageMs);

// Pulse phase (0 .. 255, triangle wave over kPulseMs) at time nowMs.
int PulsePhase(DWORD nowMs);

// One rectangle of a beam, in pixels relative to the item's ground point (y grows downwards):
// x0 <= x < x1, y0 <= y < y1, drawn with D2GFX_DrawRectangle draw mode `mode`.
struct BeamRect {
	int x0;
	int y0;
	int x1;
	int y1;
	int mode;
};
const int kMaxBeamRects = 20;

// The rectangles of one beam at the given intensity and pulse phase, back to front (glow first, the
// core over it); writes at most maxRects, returns the count (0 when intensity is 0).
int BeamRects(int intensity, int pulse, BeamRect* out, int maxRects);

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
};

// Keeps the `cap` candidates nearest the player (ties: any of them); order is unspecified.
void SelectNearest(std::vector<Candidate>& candidates, size_t cap);

// Draws the beams of the visible ground items. Runs at most once per frame: the first call draws,
// later calls do nothing until EndFrame. Called from BH's draw hook (MapNotify::OnDraw); a hook that
// runs before the ground labels may call it first so the labels end up over the beams.
void Draw();
// End of BH's frame: the next Draw draws again.
void EndFrame();
// New game: forget every item.
void Reset();

}  // namespace ItemBeams
