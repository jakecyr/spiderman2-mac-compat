#pragma once
#include <windows.h>
#include <xinput.h>
#define SM2_INPUT_MAPPING "Local\\SM2QuestGamepad-v1"
#define SM2_INPUT_MAGIC 0x534d3251u
/* One producer in the OpenXR process; seqlock protects the shared snapshot. */
typedef struct { DWORD magic; volatile LONG sequence; DWORD tick; DWORD connected; XINPUT_STATE state; } SM2QuestState;
