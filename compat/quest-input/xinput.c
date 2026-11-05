#include "state.h"
#include <string.h>
static HANDLE mapping;
static const SM2QuestState *shared;
static SRWLOCK mapping_lock=SRWLOCK_INIT;
static BOOL enabled=TRUE;
static BOOL snapshot(XINPUT_STATE *out) {
 AcquireSRWLockExclusive(&mapping_lock);
 if(!shared){mapping=OpenFileMappingA(FILE_MAP_READ,FALSE,SM2_INPUT_MAPPING);if(mapping)shared=MapViewOfFile(mapping,FILE_MAP_READ,0,0,sizeof(*shared));if(mapping&&!shared){CloseHandle(mapping);mapping=NULL;}}
 const SM2QuestState*p=shared;
 ReleaseSRWLockExclusive(&mapping_lock);
 if(!p || !enabled)return FALSE;
 for(int tries=0;tries<8;tries++){
  LONG before=p->sequence;MemoryBarrier();if(before&1)continue;
  DWORD magic=p->magic,tick=p->tick,connected=p->connected;XINPUT_STATE s=p->state;
  MemoryBarrier();if(before!=p->sequence)continue;
  if(magic!=SM2_INPUT_MAGIC||!connected||(DWORD)(GetTickCount()-tick)>500)return FALSE;
  *out=s;return TRUE;
 }
 return FALSE;
}
DWORD WINAPI XInputGetState(DWORD index,XINPUT_STATE*out){if(!out)return ERROR_BAD_ARGUMENTS;memset(out,0,sizeof(*out));return index==0&&snapshot(out)?ERROR_SUCCESS:ERROR_DEVICE_NOT_CONNECTED;}
DWORD WINAPI XInputSetState(DWORD index,XINPUT_VIBRATION*v){XINPUT_STATE s;if(!v)return ERROR_BAD_ARGUMENTS;return index==0&&snapshot(&s)?ERROR_SUCCESS:ERROR_DEVICE_NOT_CONNECTED;}
DWORD WINAPI XInputGetCapabilities(DWORD index,DWORD flags,XINPUT_CAPABILITIES*out){XINPUT_STATE s;(void)flags;if(!out)return ERROR_BAD_ARGUMENTS;memset(out,0,sizeof(*out));if(index||!snapshot(&s))return ERROR_DEVICE_NOT_CONNECTED;out->Type=XINPUT_DEVTYPE_GAMEPAD;out->SubType=XINPUT_DEVSUBTYPE_GAMEPAD;out->Gamepad.wButtons=0xf3ff;out->Gamepad.bLeftTrigger=out->Gamepad.bRightTrigger=255;out->Gamepad.sThumbLX=out->Gamepad.sThumbLY=out->Gamepad.sThumbRX=out->Gamepad.sThumbRY=32767;return ERROR_SUCCESS;}
void WINAPI XInputEnable(BOOL value){enabled=value;}
DWORD WINAPI XInputGetBatteryInformation(DWORD index,BYTE type,XINPUT_BATTERY_INFORMATION*out){XINPUT_STATE s;(void)type;if(!out)return ERROR_BAD_ARGUMENTS;memset(out,0,sizeof(*out));if(index||!snapshot(&s))return ERROR_DEVICE_NOT_CONNECTED;out->BatteryType=BATTERY_TYPE_WIRED;out->BatteryLevel=BATTERY_LEVEL_FULL;return ERROR_SUCCESS;}
DWORD WINAPI XInputGetKeystroke(DWORD index,DWORD reserved,PXINPUT_KEYSTROKE out){XINPUT_STATE s;(void)reserved;if(!out)return ERROR_BAD_ARGUMENTS;memset(out,0,sizeof(*out));return index==0&&snapshot(&s)?ERROR_EMPTY:ERROR_DEVICE_NOT_CONNECTED;}
DWORD WINAPI XInputGetAudioDeviceIds(DWORD index,LPWSTR render,UINT*nr,LPWSTR capture,UINT*nc){(void)index;(void)render;(void)capture;if(nr)*nr=0;if(nc)*nc=0;return ERROR_DEVICE_NOT_CONNECTED;}
