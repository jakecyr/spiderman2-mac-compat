#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include "openxr.h"
#include "openxr_platform.h"
#include "openxr_loader_negotiation.h"
int main(){
 HMODULE dll=LoadLibraryA("C:\\openxr\\trace_runtime.dll");
 if(!dll){printf("Load failed %lu\n",GetLastError());return 1;}
 auto negotiate=(PFN_xrNegotiateLoaderRuntimeInterface)GetProcAddress(dll,"xrNegotiateLoaderRuntimeInterface");
 XrNegotiateLoaderInfo li={XR_LOADER_INTERFACE_STRUCT_LOADER_INFO,1,sizeof(li),1,1,XR_MAKE_VERSION(1,0,0),XR_MAKE_VERSION(1,1,0)};
 XrNegotiateRuntimeRequest rr={XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST,1,sizeof(rr)};
 printf("negotiate %d\n",negotiate(&li,&rr));
 auto get=rr.getInstanceProcAddr; XrInstance inst=XR_NULL_HANDLE;
 #define LOAD(n) PFN_##n n=nullptr; get(inst,#n,(PFN_xrVoidFunction*)&n)
 #define CHECK(x) do{XrResult r=(x);printf("%s = %d\n",#x,r);fflush(stdout);if(r<0)return 2;}while(0)
 LOAD(xrCreateInstance);
 const char *exts[]={"XR_KHR_D3D11_enable"};
 XrInstanceCreateInfo ci={XR_TYPE_INSTANCE_CREATE_INFO}; strcpy(ci.applicationInfo.applicationName,"Quest input diagnostic");ci.applicationInfo.apiVersion=XR_MAKE_VERSION(1,0,0);ci.enabledExtensionCount=1;ci.enabledExtensionNames=exts;
 CHECK(xrCreateInstance(&ci,&inst));
 LOAD(xrGetSystem); LOAD(xrGetSystemProperties);LOAD(xrGetD3D11GraphicsRequirementsKHR);LOAD(xrCreateSession);LOAD(xrBeginSession);LOAD(xrEnumerateViewConfigurationViews);LOAD(xrEnumerateSwapchainFormats);LOAD(xrCreateSwapchain);LOAD(xrDestroySession);LOAD(xrDestroyInstance);LOAD(xrWaitFrame);LOAD(xrBeginFrame);LOAD(xrEndFrame);LOAD(xrCreateReferenceSpace);LOAD(xrLocateViews);LOAD(xrGetViewConfigurationProperties);
 XrSystemGetInfo gi={XR_TYPE_SYSTEM_GET_INFO};gi.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;XrSystemId sys;CHECK(xrGetSystem(inst,&gi,&sys));
 XrSystemProperties props={XR_TYPE_SYSTEM_PROPERTIES}; CHECK(xrGetSystemProperties(inst,sys,&props));
 XrGraphicsRequirementsD3D11KHR req={XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};CHECK(xrGetD3D11GraphicsRequirementsKHR(inst,sys,&req));
 ID3D11Device *dev=nullptr;ID3D11DeviceContext *context=nullptr;D3D_FEATURE_LEVEL level;
 HRESULT hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&dev,&level,&context);printf("D3D11CreateDevice %lx level %x\n",hr,level);if(FAILED(hr))return 3;
 XrGraphicsBindingD3D11KHR binding={XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};binding.device=dev;
 XrSessionCreateInfo sci={XR_TYPE_SESSION_CREATE_INFO};sci.next=&binding;sci.systemId=sys;XrSession session;CHECK(xrCreateSession(inst,&sci,&session));

 LOAD(xrStringToPath);LOAD(xrCreateActionSet);LOAD(xrCreateAction);LOAD(xrSuggestInteractionProfileBindings);LOAD(xrAttachSessionActionSets);LOAD(xrSyncActions);LOAD(xrGetActionStateBoolean);LOAD(xrGetActionStateFloat);LOAD(xrGetActionStateVector2f);
 XrActionSetCreateInfo asci={XR_TYPE_ACTION_SET_CREATE_INFO};strcpy(asci.actionSetName,"quest_gamepad");strcpy(asci.localizedActionSetName,"Quest Gamepad");XrActionSet aset;CHECK(xrCreateActionSet(inst,&asci,&aset));
 struct Entry {const char* name;const char* path;XrActionType type;XrAction action;};
 Entry inputs[]={
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
 {"right_click","/user/hand/right/input/thumbstick/click",XR_ACTION_TYPE_BOOLEAN_INPUT}};
 XrActionSuggestedBinding bindings[13];int bindcount=0;
 for(auto &input:inputs){XrActionCreateInfo aci={XR_TYPE_ACTION_CREATE_INFO};strcpy(aci.actionName,input.name);strcpy(aci.localizedActionName,input.name);aci.actionType=input.type;CHECK(xrCreateAction(aset,&aci,&input.action));XrPath path;CHECK(xrStringToPath(inst,input.path,&path));bindings[bindcount++]={input.action,path};}
 XrPath profile;CHECK(xrStringToPath(inst,"/interaction_profiles/oculus/touch_controller",&profile));
 XrInteractionProfileSuggestedBinding suggested={XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};suggested.interactionProfile=profile;suggested.countSuggestedBindings=bindcount;suggested.suggestedBindings=bindings;CHECK(xrSuggestInteractionProfileBindings(inst,&suggested));
 XrSessionActionSetsAttachInfo attach={XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};attach.countActionSets=1;attach.actionSets=&aset;CHECK(xrAttachSessionActionSets(session,&attach));

 XrSessionBeginInfo bi={XR_TYPE_SESSION_BEGIN_INFO};bi.primaryViewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;CHECK(xrBeginSession(session,&bi));
 XrViewConfigurationProperties vp={XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};CHECK(xrGetViewConfigurationProperties(inst,sys,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,&vp));
 unsigned count=0;CHECK(xrEnumerateViewConfigurationViews(inst,sys,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,0,&count,nullptr));
 XrViewConfigurationView views[2]={{XR_TYPE_VIEW_CONFIGURATION_VIEW},{XR_TYPE_VIEW_CONFIGURATION_VIEW}};CHECK(xrEnumerateViewConfigurationViews(inst,sys,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,2,&count,views));
 int64_t fmts[32];CHECK(xrEnumerateSwapchainFormats(session,32,&count,fmts));
 XrSwapchainCreateInfo sw={XR_TYPE_SWAPCHAIN_CREATE_INFO};sw.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_SAMPLED_BIT;sw.format=DXGI_FORMAT_R8G8B8A8_UNORM;sw.sampleCount=1;sw.width=512;sw.height=512;sw.faceCount=1;sw.arraySize=1;sw.mipCount=1;XrSwapchain swap;CHECK(xrCreateSwapchain(session,&sw,&swap));
 LOAD(xrEnumerateSwapchainImages);LOAD(xrAcquireSwapchainImage);LOAD(xrWaitSwapchainImage);LOAD(xrReleaseSwapchainImage);
 XrSwapchainImageD3D11KHR images[8];for(auto &i:images){i={XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR};}
 CHECK(xrEnumerateSwapchainImages(swap,8,&count,(XrSwapchainImageBaseHeader*)images));
 XrReferenceSpaceCreateInfo ref={XR_TYPE_REFERENCE_SPACE_CREATE_INFO};ref.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_LOCAL;ref.poseInReferenceSpace.orientation.w=1;XrSpace space;CHECK(xrCreateReferenceSpace(session,&ref,&space));
 for(int frame=0;frame<600;frame++){
  XrActiveActionSet active={aset,XR_NULL_PATH};XrActionsSyncInfo sync={XR_TYPE_ACTIONS_SYNC_INFO};sync.countActiveActionSets=1;sync.activeActionSets=&active;XrResult synced=xrSyncActions(session,&sync);
  if(frame%10==0){printf("INPUT frame=%d sync=%d",frame,synced);
   for(auto &input:inputs){XrActionStateGetInfo info={XR_TYPE_ACTION_STATE_GET_INFO};info.action=input.action;
    if(input.type==XR_ACTION_TYPE_BOOLEAN_INPUT){XrActionStateBoolean st={XR_TYPE_ACTION_STATE_BOOLEAN};XrResult r=xrGetActionStateBoolean(session,&info,&st);printf(" %s=%u(active%u,r%d)",input.name,st.currentState,st.isActive,r);}
    else if(input.type==XR_ACTION_TYPE_FLOAT_INPUT){XrActionStateFloat st={XR_TYPE_ACTION_STATE_FLOAT};XrResult r=xrGetActionStateFloat(session,&info,&st);printf(" %s=%.2f(active%u,r%d)",input.name,st.currentState,st.isActive,r);}
    else {XrActionStateVector2f st={XR_TYPE_ACTION_STATE_VECTOR2F};XrResult r=xrGetActionStateVector2f(session,&info,&st);printf(" %s=%.2f,%.2f(active%u,r%d)",input.name,st.currentState.x,st.currentState.y,st.isActive,r);}}
   printf("\n");fflush(stdout);}

  XrFrameWaitInfo wi={XR_TYPE_FRAME_WAIT_INFO};XrFrameState fs={XR_TYPE_FRAME_STATE};CHECK(xrWaitFrame(session,&wi,&fs));
  XrFrameBeginInfo begin={XR_TYPE_FRAME_BEGIN_INFO};CHECK(xrBeginFrame(session,&begin));
  XrSwapchainImageAcquireInfo ac={XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};unsigned index;CHECK(xrAcquireSwapchainImage(swap,&ac,&index));
  XrSwapchainImageWaitInfo wait={XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=XR_INFINITE_DURATION;CHECK(xrWaitSwapchainImage(swap,&wait));
  ID3D11RenderTargetView *rtv=nullptr;HRESULT h=dev->CreateRenderTargetView(images[index].texture,nullptr,&rtv);if(FAILED(h)){printf("RTV failed %lx\n",h);return 4;}
  float color[]={0.05f,0.15f,0.25f,1.0f};context->ClearRenderTargetView(rtv,color);context->Flush();rtv->Release();
  XrSwapchainImageReleaseInfo release={XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};CHECK(xrReleaseSwapchainImage(swap,&release));
  XrViewLocateInfo loc={XR_TYPE_VIEW_LOCATE_INFO};loc.viewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;loc.displayTime=fs.predictedDisplayTime;loc.space=space;XrViewState vs={XR_TYPE_VIEW_STATE};XrView eye[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};unsigned n;CHECK(xrLocateViews(session,&loc,&vs,2,&n,eye));
  XrCompositionLayerProjectionView pv[2];for(int i=0;i<2;i++){pv[i]={XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};pv[i].pose=eye[i].pose;pv[i].fov=eye[i].fov;pv[i].subImage.swapchain=swap;pv[i].subImage.imageRect.extent={512,512};}
  XrCompositionLayerProjection layer={XR_TYPE_COMPOSITION_LAYER_PROJECTION};layer.space=space;layer.viewCount=2;layer.views=pv;const XrCompositionLayerBaseHeader *layers[]={(const XrCompositionLayerBaseHeader*)&layer};
  XrFrameEndInfo end={XR_TYPE_FRAME_END_INFO};end.displayTime=fs.predictedDisplayTime;end.environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE;end.layerCount=1;end.layers=layers;CHECK(xrEndFrame(session,&end));Sleep(50);
 }
 CHECK(xrDestroySession(session));CHECK(xrDestroyInstance(inst));return 0;
}
