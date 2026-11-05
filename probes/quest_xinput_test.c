#include "../compat/quest-input/state.h"
#include <stdio.h>
#include <string.h>
#define REQUIRE(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char**argv){
 if(argc!=2)return 2;
 HMODULE dll=LoadLibraryA(argv[1]);REQUIRE(dll);
 DWORD(WINAPI*get)(DWORD,XINPUT_STATE*)=(void*)GetProcAddress(dll,"XInputGetState");REQUIRE(get);
 XINPUT_STATE result;REQUIRE(get(0,&result)==ERROR_DEVICE_NOT_CONNECTED);
 HANDLE h=CreateFileMappingA(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(SM2QuestState),SM2_INPUT_MAPPING);REQUIRE(h);
 SM2QuestState*s=MapViewOfFile(h,FILE_MAP_ALL_ACCESS,0,0,sizeof(*s));REQUIRE(s);
 s->sequence=1;s->magic=SM2_INPUT_MAGIC;s->connected=1;s->tick=GetTickCount();s->state.dwPacketNumber=42;s->state.Gamepad.wButtons=XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_RIGHT_SHOULDER;s->state.Gamepad.sThumbLX=-20000;s->state.Gamepad.bRightTrigger=220;MemoryBarrier();s->sequence=2;
 REQUIRE(get(0,&result)==ERROR_SUCCESS);REQUIRE(result.dwPacketNumber==42);REQUIRE(result.Gamepad.wButtons==(XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_RIGHT_SHOULDER));REQUIRE(result.Gamepad.sThumbLX==-20000);REQUIRE(result.Gamepad.bRightTrigger==220);
 REQUIRE(get(1,&result)==ERROR_DEVICE_NOT_CONNECTED);
 s->sequence=3;REQUIRE(get(0,&result)==ERROR_DEVICE_NOT_CONNECTED);s->sequence=4;
 s->tick=GetTickCount()-1000;REQUIRE(get(0,&result)==ERROR_DEVICE_NOT_CONNECTED);
 s->tick=GetTickCount();s->connected=0;REQUIRE(get(0,&result)==ERROR_DEVICE_NOT_CONNECTED);
 puts("PASS: mapping, buttons/axes, controller index, seqlock and stale/disconnected safety");return 0;
}
