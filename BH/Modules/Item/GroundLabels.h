#pragma once
#include <Windows.h>

// Loot filter label styles on ground items (%BG%, %OPACITY%, %FRAME%, %SIZE%).
//
// The engine collects the visible ground item labels into an array and then draws each one with
// D2Win #10013 (box + text); it measures each label with D2Win #10177 first and stacks the boxes
// itself. Vanilla does this in D2Client 0x58FB0, PD2 in its own rewrite of that function inside
// ProjectDiablo.dll; both keep the same entry layout and call the same two D2Win functions. Their
// entries are hooked: a call whose text is a label entry's name gets the item's GroundStyle (font
// for the measurement, colour/opacity/frame/font for the draw). Every other call, and every label
// whose item has no style, goes to the original function with the original arguments.
namespace GroundLabels {

// Hooks D2Win #10013 and #10177 (1.13c only). On failure the hooks stay off and *why names the
// reason (the first code site that differs from the expected bytes).
bool Install(const char** why);
void Uninstall();

// Called once per label pass, right before the first ground item label is drawn, so that what it
// draws (item beams) ends up below the labels. The label pass only runs while ground items are
// shown (Alt / "always show items"). nullptr clears it.
void SetPreLabelCallback(void (*callback)());

// Entry hook of a function: `jmp hook` over its first whole instructions, which move to a
// trampoline (followed by a jmp back). An entry another module already detoured (`jmp rel32` plus
// int3 padding, as PD2's D2GL does to D2Win #10177) matches when the bytes after the padding do;
// the trampoline is then that jmp, re-aimed from the trampoline's address at the same destination,
// so the other module's hook keeps running behind ours.
struct DetourPlan {
	int stolen;            // entry bytes the jmp (+ int3 padding) replaces
	BYTE trampoline[16];   // code to place at the trampoline address
	int trampolineLen;
	ULONGLONG hookCode;    // the first 8 entry bytes with the jmp to the hook
};

// `code` holds at least max(expectedLen, 8) bytes read at address `entry`; `expected` are the
// function's original first bytes, of which the first `prologue` (>= 5) are whole,
// position-independent instructions. False when the code is not that function.
bool PlanDetour(const BYTE* code, DWORD entry, const BYTE* expected, int expectedLen, int prologue, DWORD trampoline,
	DWORD hook, DetourPlan* plan);

}
