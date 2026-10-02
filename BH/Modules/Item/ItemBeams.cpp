#include "ItemBeams.h"

#include <algorithm>

#include "../../Constants.h"
#include "../../D2Ptrs.h"
#include "GroundLabels.h"
#include "ItemDisplay.h"

namespace ItemBeams {

int FlashIntensity(DWORD ageMs) {
	if (ageMs >= kFlashMs)
		return 0;
	// Rounded up: the flash is visible until the full kFlashMs has passed.
	return (int)(((kFlashMs - ageMs) * kMaxIntensity + kFlashMs - 1) / kFlashMs);
}

int SparkY(DWORD nowMs, DWORD seed, int k, int height) {
	if (height <= 0)
		return 0;
	const DWORD risen = (DWORD)(((unsigned long long)nowMs * kRisePxPerSec) / 1000) + seed * 53 + k * (height / 2);
	return -(int)(risen % (DWORD)height) - 1;
}

int BeamRects(int intensity, DWORD nowMs, DWORD seed, BeamRect* out, int maxRects) {
	if (intensity <= 0)
		return 0;
	if (intensity > kMaxIntensity)
		intensity = kMaxIntensity;
	const bool strong = intensity * 2 >= kMaxIntensity;
	// A fading beam also gets shorter (full height at full intensity, ~40% just before it vanishes)
	// and narrower.
	const int height = kBeamHeight * (96 + intensity * 159 / kMaxIntensity) / 255;
	const int outer = 2 + 4 * intensity / kMaxIntensity;  // half widths: 13 px at full intensity
	const int inner = 1 + 2 * intensity / kMaxIntensity;

	int n = 0;
	auto add = [&](int x0, int y0, int x1, int y1, int mode, bool white) {
		if (n < maxRects && x0 < x1 && y0 < y1)
			out[n++] = BeamRect{ x0, y0, x1, y1, mode, white };
	};
	// A layer of 1 px columns: column dx reaches h(dx) = top * (1 - |dx| / (halfWidth + 1)) above the
	// foot (a needle), its last kDitherRows rows drawn every other row.
	auto layer = [&](int halfWidth, int top, int mode, bool white) {
		const int w1 = halfWidth + 1;
		for (int dx = -halfWidth; dx <= halfWidth; dx++) {
			const int h = top * (w1 - (dx < 0 ? -dx : dx)) / w1;
			if (h <= 0)
				continue;
			const int solidTop = h > kDitherRows ? -h + kDitherRows : -h;
			add(dx, solidTop, dx + 1, 0, mode, white);
			for (int y = -h + ((dx & 1) ? 1 : 0); y < solidTop; y += 2)
				add(dx, y, dx + 1, y + 1, mode, white);
		}
	};
	layer(outer, height, kModeTrans25, false);
	layer(inner, height, kModeTrans25, false);
	layer(1, height, strong ? kModeTrans50 : kModeTrans25, false);
	if (strong)
		layer(0, height * 70 / 100, kModeTrans25, true);  // faint white centre line
	// Rising sparks in the core.
	for (int k = 0; k < 2; k++) {
		const int y = SparkY(nowMs, seed, k, height);
		const int top = (std::max)(y - kSparkLength, -height);
		add(0, top, 1, y, kModeTrans25, true);
	}
	return n;
}

long BeamFootY(long groundX, long groundY, const LabelBox* label) {
	if (label && label->left <= groundX && groundX < label->right && label->top < label->bottom)
		return (label->top + label->bottom) / 2;
	return groundY - kFootAboveGround;
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

ScreenSpan VisibleSpan(DWORD covered, long screenWidth) {
	const long half = screenWidth / 2;
	switch (covered) {
	case 0: return ScreenSpan{ 0, screenWidth };
	case 1: return ScreenSpan{ 0, half };            // right panel open
	case 2: return ScreenSpan{ half, screenWidth };  // left panel open
	default: return ScreenSpan{ 0, 0 };              // both
	}
}

ScreenPoint GroundToScreen(long absX, long absY, long mouseOffsetX, long mouseOffsetY, long viewShiftX) {
	return ScreenPoint{ absX - mouseOffsetX + viewShiftX, absY - mouseOffsetY };
}

bool ClipToSpan(const ScreenSpan& span, long* x0, long* x1) {
	if (*x0 < span.x0)
		*x0 = span.x0;
	if (*x1 > span.x1)
		*x1 = span.x1;
	return *x0 < *x1;
}

namespace {

FlashTracker flashes;
std::vector<Candidate> candidates;
bool drawnThisFrame = false;
DWORD frameNow = 0;
ScreenSpan frameSpan = { 0, 0 };
DWORD lastPrune = 0;

// UIs that hide the game world, or that BH already hides the ground labels for (Item.cpp
// PermShowItemsPatch1); the engine draws no ground labels with the NPC dialog open either.
bool WorldHiddenByUI() {
	return D2CLIENT_GetUIState(UI_ESCMENU_MAIN) || D2CLIENT_GetUIState(UI_HOTKEY_CONFIG) ||
		D2CLIENT_GetUIState(UI_HELP_MENU) || D2CLIENT_GetUIState(UI_NPCMENU) ||
		D2CLIENT_GetUIState(UI_MINISKILL) || *p_D2CLIENT_GoldDialog;
}

// This frame's beams (once per frame): the 16 nearest beam items in the visible part of the screen.
// False when there is no game to draw.
bool Collect() {
	UnitAny* player = D2CLIENT_GetPlayerUnit();
	if (!player || !player->pAct || !player->pPath || !player->pPath->pRoom1 ||
		player->pPath->pRoom1->pRoom2->pLevel->dwLevelNo == 0)
		return false;

	const DWORD now = GetTickCount();
	frameNow = now;
	const long playerX = player->pPath->xPos;
	const long playerY = player->pPath->yPos;
	const long screenH = *p_D2CLIENT_ScreenSizeY;
	// Items are still observed while the world is hidden (a drop under a panel must not flash later).
	const ScreenSpan span = WorldHiddenByUI() ? ScreenSpan{ 0, 0 } :
		VisibleSpan(*p_D2CLIENT_ScreenCovered, (long)*p_D2CLIENT_ScreenSizeX);
	frameSpan = span;

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
			const ScreenPoint p = GroundToScreen(x, y, *p_D2CLIENT_MouseOffsetX, *p_D2CLIENT_MouseOffsetY,
				*p_D2CLIENT_ViewShiftX);
			// The foot: behind the item's label (the box it is drawn in this frame, while the labels are
			// being drawn), else where its label would sit.
			GroundLabels::Box drawn;
			LabelBox box;
			const LabelBox* label = nullptr;
			if (GroundLabels::LabelBoxOf(unit, &drawn)) {
				box = LabelBox{ drawn.left, drawn.top, drawn.right, drawn.bottom };
				label = &box;
			}
			const long footY = BeamFootY(p.x, p.y, label);
			// Off screen or behind a panel: not a candidate, so the cap keeps the visible beams.
			if (span.x0 >= span.x1 || p.x < span.x0 - 16 || p.x >= span.x1 + 16 || footY < 0 ||
				footY - kBeamHeight > screenH)
				continue;
			candidates.push_back(Candidate{ dx * dx + dy * dy, p.x, footY, color, intensity, unit->dwUnitId });
		}
	}
	SelectNearest(candidates, kMaxBeams);

	if (now - lastPrune > 1000) {
		flashes.Prune(now);
		lastPrune = now;
	}
	return true;
}

void DrawBeams() {
	BeamRect r[kMaxBeamRects];
	for (const Candidate& c : candidates) {
		const int n = BeamRects(c.intensity, frameNow, c.seed, r, kMaxBeamRects);
		for (int i = 0; i < n; i++) {
			long x0 = c.screenX + r[i].x0;
			long x1 = c.screenX + r[i].x1;
			if (ClipToSpan(frameSpan, &x0, &x1))
				D2GFX_DrawRectangle(x0, c.screenY + r[i].y0, x1, c.screenY + r[i].y1, r[i].white ? kWhite : c.color,
					r[i].mode);
		}
	}
}

}  // namespace

void Draw() {
	if (drawnThisFrame)
		return;
	drawnThisFrame = true;
	if (Collect())
		DrawBeams();
}

void EndFrame() {
	drawnThisFrame = false;
}

void Reset() {
	flashes.Clear();
	candidates.clear();
	EndFrame();
}

}  // namespace ItemBeams
