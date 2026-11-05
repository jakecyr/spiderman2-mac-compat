/* OpenXR -> XInput bridge for actual user input; included by trace_runtime.c. */
#include "state.h"
#include <math.h>
static struct {
 XrInstance instance; XrSession session; XrActionSet set; int attached,focused;
 HANDLE mapping; SM2QuestState *shared; XINPUT_STATE last;
 PFN_xrStringToPath path; PFN_xrCreateActionSet create_set; PFN_xrCreateAction create_action;
 PFN_xrSuggestInteractionProfileBindings suggest; PFN_xrAttachSessionActionSets attach;
 PFN_xrSyncActions sync; PFN_xrGetActionStateBoolean boolean;
 PFN_xrGetActionStateFloat scalar; PFN_xrGetActionStateVector2f vector;
} qpad;
static struct { const char*name;const char*path;XrActionType type;XrAction action; } qinputs[]={
 {"left_stick","/user/hand/left/input/thumbstick",XR_ACTION_TYPE_VECTOR2F_INPUT},
 {"right_stick","/user/hand/right/input/thumbstick",XR_ACTION_TYPE_VECTOR2F_INPUT},
 {"left_trigger","/user/hand/left/input/trigger/value",XR_ACTION_TYPE_FLOAT_INPUT},
 {"right_trigger","/user/hand/right/input/trigger/value",XR_ACTION_TYPE_FLOAT_INPUT},
 {"left_grip","/user/hand/left/input/squeeze/value",XR_ACTION_TYPE_FLOAT_INPUT},
 {"right_grip","/user/hand/right/input/squeeze/value",XR_ACTION_TYPE_FLOAT_INPUT},
 {"a","/user/hand/right/input/a/click",XR_ACTION_TYPE_BOOLEAN_INPUT},
 {"b","/user/hand/right/input/b/click",XR_ACTION_TYPE_BOOLEAN_INPUT},
 {"x","/user/hand/left/input/x/click",XR_ACTION_TYPE_BOOLEAN_INPUT},
 {"y","/user/hand/left/input/y/click",XR_ACTION_TYPE_BOOLEAN_INPUT},
 {"menu","/user/hand/left/input/menu/click",XR_ACTION_TYPE_BOOLEAN_INPUT},
 {"left_click","/user/hand/left/input/thumbstick/click",XR_ACTION_TYPE_BOOLEAN_INPUT},
 {"right_click","/user/hand/right/input/thumbstick/click",XR_ACTION_TYPE_BOOLEAN_INPUT}
};
static void qpublish(const XINPUT_GAMEPAD*pad,int connected){
 if(!qpad.shared)return;
 if(memcmp(&qpad.last.Gamepad,pad,sizeof(*pad)))qpad.last.dwPacketNumber++;
 qpad.last.Gamepad=*pad;
 InterlockedIncrement(&qpad.shared->sequence);MemoryBarrier();
 qpad.shared->magic=SM2_INPUT_MAGIC;qpad.shared->tick=GetTickCount();qpad.shared->connected=connected;qpad.shared->state=qpad.last;
 MemoryBarrier();InterlockedIncrement(&qpad.shared->sequence);
}
static void qdisconnect(void){XINPUT_GAMEPAD empty={0};qpublish(&empty,0);}
static void qinit(XrInstance instance,XrSession session){
 char flag[8];if(!GetEnvironmentVariableA("SM2_QUEST_GAMEPAD",flag,sizeof(flag))||strcmp(flag,"1"))return;
 qpad.instance=instance;qpad.session=session;qpad.focused=0;qpad.attached=0;
 #define QLOAD(field,name) if(real_get(instance,#name,(PFN_xrVoidFunction*)&qpad.field)<0||!qpad.field){trace("QPAD missing " #name);return;}
 QLOAD(path,xrStringToPath);QLOAD(create_set,xrCreateActionSet);QLOAD(create_action,xrCreateAction);QLOAD(suggest,xrSuggestInteractionProfileBindings);QLOAD(attach,xrAttachSessionActionSets);QLOAD(sync,xrSyncActions);QLOAD(boolean,xrGetActionStateBoolean);QLOAD(scalar,xrGetActionStateFloat);QLOAD(vector,xrGetActionStateVector2f);
 #undef QLOAD
 XrActionSetCreateInfo ci={XR_TYPE_ACTION_SET_CREATE_INFO};strcpy(ci.actionSetName,"sm2_quest_gamepad");strcpy(ci.localizedActionSetName,"Quest Gamepad");
 XrResult r=qpad.create_set(instance,&ci,&qpad.set);if(r<0){trace("QPAD create set %d",r);return;}
 XrActionSuggestedBinding bindings[13];
 for(int i=0;i<13;i++){
  XrActionCreateInfo ac={XR_TYPE_ACTION_CREATE_INFO};strcpy(ac.actionName,qinputs[i].name);strcpy(ac.localizedActionName,qinputs[i].name);ac.actionType=qinputs[i].type;
  if(qpad.create_action(qpad.set,&ac,&qinputs[i].action)<0){qpad.set=XR_NULL_HANDLE;return;}
  bindings[i].action=qinputs[i].action;if(qpad.path(instance,qinputs[i].path,&bindings[i].binding)<0){qpad.set=XR_NULL_HANDLE;return;}
 }
 XrInteractionProfileSuggestedBinding sb={XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};qpad.path(instance,"/interaction_profiles/oculus/touch_controller",&sb.interactionProfile);sb.countSuggestedBindings=13;sb.suggestedBindings=bindings;
 r=qpad.suggest(instance,&sb);trace("QPAD bindings %d",r);if(r<0){qpad.set=XR_NULL_HANDLE;return;}
 qpad.mapping=CreateFileMappingA(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(SM2QuestState),SM2_INPUT_MAPPING);
 if(qpad.mapping)qpad.shared=MapViewOfFile(qpad.mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(SM2QuestState));
 if(qpad.shared){memset(qpad.shared,0,sizeof(*qpad.shared));qdisconnect();}
}
static void qattach_if_needed(XrSession session){
 if(!qpad.set||session!=qpad.session||qpad.attached)return;
 XrSessionActionSetsAttachInfo info={XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};info.countActionSets=1;info.actionSets=&qpad.set;
 XrResult r=qpad.attach(session,&info);qpad.attached=XR_SUCCEEDED(r);trace("QPAD attach %d",r);
}
static SHORT qaxis(float x){if(!isfinite(x))return 0;if(x>1)x=1;if(x< -1)x=-1;return (SHORT)(x*(x<0?32768:32767));}
static BYTE qtrigger(float x){if(!isfinite(x)||x<0)x=0;if(x>1)x=1;return (BYTE)(x*255);}
static void qpoll(XrSession session){
 if(session!=qpad.session||!qpad.attached||!qpad.shared)return;
 if(!qpad.focused){qdisconnect();return;}
 XrActiveActionSet active={qpad.set,XR_NULL_PATH};XrActionsSyncInfo sync={XR_TYPE_ACTIONS_SYNC_INFO};sync.countActiveActionSets=1;sync.activeActionSets=&active;
 if(qpad.sync(session,&sync)!=XR_SUCCESS){qdisconnect();return;}
 XINPUT_GAMEPAD pad={0};int active_count=0;
 for(int i=0;i<13;i++){
  XrActionStateGetInfo info={XR_TYPE_ACTION_STATE_GET_INFO};info.action=qinputs[i].action;
  if(i<2){XrActionStateVector2f s={XR_TYPE_ACTION_STATE_VECTOR2F};if(qpad.vector(session,&info,&s)>=0&&s.isActive){active_count++;if(i==0){pad.sThumbLX=qaxis(s.currentState.x);pad.sThumbLY=qaxis(s.currentState.y);}else{pad.sThumbRX=qaxis(s.currentState.x);pad.sThumbRY=qaxis(s.currentState.y);}}}
  else if(i<6){XrActionStateFloat s={XR_TYPE_ACTION_STATE_FLOAT};if(qpad.scalar(session,&info,&s)>=0&&s.isActive){active_count++;if(i==2)pad.bLeftTrigger=qtrigger(s.currentState);if(i==3)pad.bRightTrigger=qtrigger(s.currentState);if(i==4&&s.currentState>.55f)pad.wButtons|=XINPUT_GAMEPAD_LEFT_SHOULDER;if(i==5&&s.currentState>.55f)pad.wButtons|=XINPUT_GAMEPAD_RIGHT_SHOULDER;}}
  else {static const WORD buttons[]={XINPUT_GAMEPAD_A,XINPUT_GAMEPAD_B,XINPUT_GAMEPAD_X,XINPUT_GAMEPAD_Y,XINPUT_GAMEPAD_START,XINPUT_GAMEPAD_LEFT_THUMB,XINPUT_GAMEPAD_RIGHT_THUMB};XrActionStateBoolean s={XR_TYPE_ACTION_STATE_BOOLEAN};if(qpad.boolean(session,&info,&s)>=0&&s.isActive){active_count++;if(s.currentState)pad.wButtons|=buttons[i-6];}}
 }
 /* Hold left Menu as a D-pad modifier; release it for normal pause/menu use. */
 if((pad.wButtons&XINPUT_GAMEPAD_START)&&(abs(pad.sThumbRX)>16000||abs(pad.sThumbRY)>16000)){
  pad.wButtons&=~XINPUT_GAMEPAD_START;
  if(pad.sThumbRX>16000)pad.wButtons|=XINPUT_GAMEPAD_DPAD_RIGHT;if(pad.sThumbRX< -16000)pad.wButtons|=XINPUT_GAMEPAD_DPAD_LEFT;
  if(pad.sThumbRY>16000)pad.wButtons|=XINPUT_GAMEPAD_DPAD_UP;if(pad.sThumbRY< -16000)pad.wButtons|=XINPUT_GAMEPAD_DPAD_DOWN;pad.sThumbRX=pad.sThumbRY=0;
 }
 qpublish(&pad,active_count>0);
 static DWORD lastlog;if(GetTickCount()-lastlog>1000){lastlog=GetTickCount();trace("QPAD packet=%lu active=%d buttons=%x sticks=%d,%d,%d,%d triggers=%u,%u",qpad.last.dwPacketNumber,active_count,pad.wButtons,pad.sThumbLX,pad.sThumbLY,pad.sThumbRX,pad.sThumbRY,pad.bLeftTrigger,pad.bRightTrigger);}
}
static void qevent(const XrEventDataBuffer*event){
 if(event->type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED){const XrEventDataSessionStateChanged*s=(const void*)event;if(s->session==qpad.session){qpad.focused=s->state==XR_SESSION_STATE_FOCUSED;if(!qpad.focused)qdisconnect();}}
}
static void qdestroy(XrSession session){if(session!=qpad.session)return;qdisconnect();if(qpad.shared)UnmapViewOfFile(qpad.shared);if(qpad.mapping)CloseHandle(qpad.mapping);memset(&qpad,0,sizeof(qpad));}
