#include "ItemBeams.h"

#include <algorithm>

#include "../../Constants.h"
#include "../../D2Ptrs.h"
#include "ItemDisplay.h"

namespace ItemBeams {

int FlashIntensity(DWORD ageMs) {
	if (ageMs >= kFlashMs)
		return 0;
	// Rounded up: the flash is visible until the full kFlashMs has passed.
	return (int)(((kFlashMs - ageMs) * kMaxIntensity + kFlashMs - 1) / kFlashMs);
}

int PulsePhase(DWORD nowMs) {
	const DWORD half = kPulseMs / 2;
	const DWORD t = nowMs % kPulseMs;
	return (int)((t < half ? t : kPulseMs - t) * 255 / half);
}

int BeamRects(int intensity, int pulse, BeamRect* out, int maxRects) {
	if (intensity <= 0)
		return 0;
	if (intensity > kMaxIntensity)
		intensity = kMaxIntensity;
	const bool strong = intensity * 2 >= kMaxIntensity;
	// A fading beam also gets shorter: full height at full intensity, ~40% just before it vanishes.
	const int height = kBeamHeight * (96 + intensity * 159 / kMaxIntensity) / 255;
	// Soft outer glow 13..17 px wide (breathing with the pulse), narrower while fading; an inner glow
	// half as wide and a 3 px core over it. Overlapping translucent layers brighten towards the middle.
	const int glow = 3 + intensity * (3 + pulse * 2 / 255) / kMaxIntensity;
	const int inner = glow / 2;
	const int core = 1;
	const int segments = 6;

	int n = 0;
	auto add = [&](int x0, int y0, int x1, int y1, int mode) {
		if (n < maxRects)
			out[n++] = BeamRect{ x0, y0, x1, y1, mode };
	};
	auto column = [&](int halfWidth, int taperTo, int fromSegment, int toSegment, int mode) {
		for (int k = fromSegment; k < toSegment; k++) {
			const int hw = halfWidth - (halfWidth - taperTo) * k / (segments - 1);
			add(-hw, -height * (k + 1) / segments, hw + 1, -height * k / segments, mode);
		}
	};
	// Light pool on the ground: a flat ellipse of two rectangles.
	if (strong) {
		add(-glow - 4, -2, glow + 5, 2, kModeTrans25);
		add(-glow, -4, glow + 1, 3, kModeTrans25);
	}
	// Glow columns, tapering towards the top.
	column(glow, 1, 0, segments, kModeTrans25);
	column(inner, 1, 0, segments - 1, kModeTrans25);
	// Core over the lower two thirds: half-opaque at the bottom while strong.
	column(core, core, 0, segments / 2, strong ? kModeTrans50 : kModeTrans25);
	column(core, core, segments / 2, segments - 2, kModeTrans25);
	return n;
}

bool FlashTracker::Observe(DWORD unitId, bool newDrop, DWORD nowMs, DWORD* startMs) {
	auto it = seen.find(unitId);
	if (it == seen.end())
		it = seen.emplace(unitId, Entry{ nowMs, nowMs, newDrop }).first;
	else
		it->second.lastSeen = nowMs;
	if (!it->second.flashes)
		return false;
	*startMs = it->second.firstSeen;
	return true;
}

void FlashTracker::Prune(DWORD nowMs) {
	for (auto it = seen.begin(); it != seen.end();) {
		if (nowMs - it->second.lastSeen > kForgetMs)
			it = seen.erase(it);
		else
			++it;
	}
}

void FlashTracker::Clear() {
	seen.clear();
}

void SelectNearest(std::vector<Candidate>& candidates, size_t cap) {
	if (candidates.size() <= cap)
		return;
	std::nth_element(candidates.begin(), candidates.begin() + cap, candidates.end(),
		[](const Candidate& a, const Candidate& b) { return a.distSq < b.distSq; });
	candidates.resize(cap);
}

namespace {

FlashTracker flashes;
std::vector<Candidate> candidates;
bool drawnThisFrame = false;
DWORD lastPrune = 0;

}  // namespace

void Draw() {
	if (drawnThisFrame)
		return;
	drawnThisFrame = true;

	UnitAny* player = D2CLIENT_GetPlayerUnit();
	if (!player || !player->pAct || !player->pPath || !player->pPath->pRoom1 ||
		player->pPath->pRoom1->pRoom2->pLevel->dwLevelNo == 0)
		return;

	const DWORD now = GetTickCount();
	const long playerX = player->pPath->xPos;
	const long playerY = player->pPath->yPos;
	const long screenW = *p_D2CLIENT_ScreenSizeX;
	const long screenH = *p_D2CLIENT_ScreenSizeY;

	candidates.clear();
	for (Room1* room1 = player->pAct->pRoom1; room1; room1 = room1->pRoomNext) {
		for (UnitAny* unit = room1->pUnitFirst; unit; unit = unit->pListNext) {
			if (unit->dwType != UNIT_ITEM || !unit->pItemPath || !unit->pItemData ||
				(unit->dwMode != ITEM_MODE_ON_GROUND && unit->dwMode != ITEM_MODE_BEING_DROPPED) ||
				(unit->dwFlags2 & UNITFLAGEX_INVISIBLE))
				continue;
			GroundStyle style;
			if (!GetGroundStyle(unit, &style))
				continue;
			int color = UNDEFINED_COLOR;
			int intensity = 0;
			DWORD start;
			if (style.flashColor != UNDEFINED_COLOR &&
				flashes.Observe(unit->dwUnitId, (unit->pItemData->dwFlags & ITEM_NEW) != 0, now, &start)) {
				intensity = FlashIntensity(now - start);
				color = style.flashColor;
			}
			if (intensity == 0 && style.beamColor != UNDEFINED_COLOR) {
				intensity = kMaxIntensity;
				color = style.beamColor;
			}
			if (intensity == 0)
				continue;

			long x = unit->pItemPath->dwPosX;
			long y = unit->pItemPath->dwPosY;
			const long dx = x - playerX;
			const long dy = y - playerY;
			D2COMMON_MapToAbsScreen(&x, &y);
			x -= *p_D2CLIENT_MouseOffsetX;
			y -= *p_D2CLIENT_MouseOffsetY;
			if (x < -16 || x > screenW + 16 || y < 0 || y - kBeamHeight > screenH)
				continue;
			candidates.push_back(Candidate{ dx * dx + dy * dy, x, y, color, intensity });
		}
	}

	SelectNearest(candidates, kMaxBeams);
	const int pulse = PulsePhase(now);
	BeamRect rects[kMaxBeamRects];
	for (const Candidate& c : candidates) {
		const int n = BeamRects(c.intensity, pulse, rects, kMaxBeamRects);
		for (int i = 0; i < n; i++) {
			D2GFX_DrawRectangle(c.screenX + rects[i].x0, c.screenY + rects[i].y0, c.screenX + rects[i].x1,
				c.screenY + rects[i].y1, c.color, rects[i].mode);
		}
	}

	if (now - lastPrune > 1000) {
		flashes.Prune(now);
		lastPrune = now;
	}
}

void EndFrame() {
	drawnThisFrame = false;
}

void Reset() {
	flashes.Clear();
	candidates.clear();
	drawnThisFrame = false;
}

}  // namespace ItemBeams
