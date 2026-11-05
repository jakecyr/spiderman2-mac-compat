from pathlib import Path
import re

root = Path(__file__).resolve().parent
header = (root / 'openxr.h').read_text()
# Core API plus the extensions used in the observed session. No behavior changes.
names = set(re.findall(r'PFN_(xr\w+)\)', header.split('// XR_VERSION_1_1')[0]))
names.update(['xrGetD3D11GraphicsRequirementsKHR', 'xrGetD3D12GraphicsRequirementsKHR',
              'xrGetDisplayRefreshRateFB', 'xrEnumerateDisplayRefreshRatesFB',
              'xrConvertWin32PerformanceCounterToTimeKHR', 'xrConvertTimeToWin32PerformanceCounterKHR'])
allheaders = header + (root / 'openxr_platform.h').read_text()
decls = {}
for match in re.finditer(r'typedef XrResult \(XRAPI_PTR \*PFN_(xr\w+)\)\((.*?)\);', allheaders, re.S):
    if match[1] in names and match[1] != 'xrGetInstanceProcAddr':
        decls[match[1]] = re.sub(r'\s+', ' ', match[2])

extras = {
 'xrEndFrame': 'trace("SUBMITTED layers=%u result=%d",frameEndInfo->layerCount,r);trace_mod_frames();',
 'xrCreateSession': 'if(createInfo->next && ((XrBaseInStructure*)createInfo->next)->type==XR_TYPE_GRAPHICS_BINDING_D3D11_KHR) trace_device(((XrGraphicsBindingD3D11KHR*)createInfo->next)->device);',
 'xrGetSystemProperties': 'if(r>=0) trace("SYSTEM name=%s maxWidth=%u maxHeight=%u layers=%u", properties->systemName, properties->graphicsProperties.maxSwapchainImageWidth, properties->graphicsProperties.maxSwapchainImageHeight, properties->graphicsProperties.maxLayerCount);',
 'xrEnumerateViewConfigurationViews': 'if(r>=0 && views && viewCapacityInput) for(unsigned i=0;i<*viewCountOutput && i<viewCapacityInput;i++) trace("VIEW %u rec=%ux%u max=%ux%u samples=%u/%u type=%d",i,views[i].recommendedImageRectWidth,views[i].recommendedImageRectHeight,views[i].maxImageRectWidth,views[i].maxImageRectHeight,views[i].recommendedSwapchainSampleCount,views[i].maxSwapchainSampleCount,views[i].type);',
 'xrEnumerateSwapchainFormats': 'if(r>=0 && formats && formatCapacityInput) for(unsigned i=0;i<*formatCountOutput && i<formatCapacityInput;i++) trace("FORMAT %lld",(long long)formats[i]);',
 'xrGetViewConfigurationProperties': 'if(r>=0) trace("VIEWPROPS type=%d fovMutable=%u",configurationProperties->viewConfigurationType,configurationProperties->fovMutable);',
 'xrCreateSwapchain': 'trace("SWAPCHAIN %ux%u fmt=%lld usage=%llu samples=%u arrays=%u",createInfo->width,createInfo->height,(long long)createInfo->format,(unsigned long long)createInfo->usageFlags,createInfo->sampleCount,createInfo->arraySize);',
 'xrGetDisplayRefreshRateFB': 'if(r>=0)trace("REFRESH %f",*displayRefreshRate);',
 'xrLocateViews': 'if(r>=0)trace("LOCATE flags=%llu count=%u",(unsigned long long)viewState->viewStateFlags,*viewCountOutput);',
 'xrWaitFrame': 'if(r>=0)trace("FRAME time=%lld period=%lld render=%u",(long long)frameState->predictedDisplayTime,(long long)frameState->predictedDisplayPeriod,frameState->shouldRender);',
}
source = r'''
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_GRAPHICS_API_D3D12
#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "openxr.h"
#include "openxr_platform.h"
#include "openxr_loader_negotiation.h"
static PFN_xrGetInstanceProcAddr real_get;
static void trace(const char *fmt, ...) {
 FILE *f=fopen("C:\\openxr\\trace.log","a"); if(!f)return;
 fprintf(f,"%lu/%lu ",GetTickCount(),GetCurrentThreadId());
 va_list ap;va_start(ap,fmt);vfprintf(f,fmt,ap);va_end(ap);fputc('\n',f);fclose(f);
}
'''
source += r'''
static LONG CALLBACK crash_trace(EXCEPTION_POINTERS* e){
 DWORD code=e->ExceptionRecord->ExceptionCode;
 if(code==EXCEPTION_ACCESS_VIOLATION || code==EXCEPTION_ILLEGAL_INSTRUCTION || code==0xc0000409){
  trace("EXCEPTION code=%lx rip=%p addr=%p access=%llu rcx=%p rdx=%p r8=%p r9=%p",code,(void*)e->ContextRecord->Rip,(void*)e->ExceptionRecord->ExceptionInformation[1],(unsigned long long)e->ExceptionRecord->ExceptionInformation[0],(void*)e->ContextRecord->Rcx,(void*)e->ContextRecord->Rdx,(void*)e->ContextRecord->R8,(void*)e->ContextRecord->R9);
  void*frames[32];USHORT count=CaptureStackBackTrace(0,32,frames,NULL);
  for(USHORT i=0;i<count;i++){HMODULE m=NULL;char path[MAX_PATH]={0};GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCSTR)frames[i],&m);if(m)GetModuleFileNameA(m,path,sizeof(path));trace("STACK %u %p %s +%llx",i,frames[i],path,(unsigned long long)((uintptr_t)frames[i]-(uintptr_t)m));}
 }
 return EXCEPTION_CONTINUE_SEARCH;
}
'''
source += '\n#include \"device_trace.h\"\n#include \"mod_trace.h\"\n#include \"quest-input/openxr_gamepad.h\"\n'
extras['xrCreateSession'] += 'if(r>=0)qinit(instance,*session);'
extras['xrPollEvent'] = 'if(r==XR_SUCCESS)qevent(eventData);'
extras['xrDestroySession'] = 'qdestroy(session);'
for name, args in decls.items():
    params = [re.search(r'(\w+)(?:\[[^]]*\])?$', part.strip())[1] for part in args.split(',')]
    source += f'static PFN_{name} real_{name};\n'
    source += f'static XrResult XRAPI_CALL wrap_{name}({args}) {{\n'
    if name == 'xrCreateSwapchain': source += 'runtime_swapchain_creation++;\n'
    if name == 'xrBeginSession': source += 'qattach_if_needed(session);\n'
    if name == 'xrEndFrame': source += 'qpoll(session);\n'
    if name == 'xrAttachSessionActionSets':
        source += 'XrSessionActionSetsAttachInfo augmented=*attachInfo;XrActionSet sets[64];if(qpad.set && session==qpad.session && !qpad.attached && attachInfo->countActionSets<63){memcpy(sets,attachInfo->actionSets,attachInfo->countActionSets*sizeof(XrActionSet));sets[attachInfo->countActionSets]=qpad.set;augmented.countActionSets++;augmented.actionSets=sets;attachInfo=&augmented;}\n'
    source += f'trace("CALL {name}"); XrResult r=real_{name}({", ".join(params)}); trace("RETURN {name} %d",r);\n'
    if name == 'xrCreateSwapchain': source += 'runtime_swapchain_creation--;\n'
    if name == 'xrAttachSessionActionSets': source += 'if(r>=0 && qpad.set && session==qpad.session)qpad.attached=1;\n'
    source += extras.get(name, '') + '\nreturn r;\n}\n'
source += '''
static XrResult XRAPI_CALL getproc(XrInstance instance,const char *name,PFN_xrVoidFunction *fn) {
 XrResult r=real_get(instance,name,fn);
 if(r<0 || !*fn) return r;
'''
for name in decls:
    source += f'if(!strcmp(name,"{name}")) {{ real_{name}=(PFN_{name})*fn;*fn=(PFN_xrVoidFunction)wrap_{name};return r;}}\n'
source += r'''
 return r;
}
__declspec(dllexport) XrResult XRAPI_CALL xrNegotiateLoaderRuntimeInterface(const XrNegotiateLoaderInfo *info,XrNegotiateRuntimeRequest *request) {
 static int handler_set;if(!handler_set){AddVectoredExceptionHandler(1,crash_trace);handler_set=1;}
 HMODULE dll=LoadLibraryA("C:\\openxr\\moltenvr_openxr.dll");
 if(!dll){trace("LOAD FAILED %lu",GetLastError());return XR_ERROR_INITIALIZATION_FAILED;}
 PFN_xrNegotiateLoaderRuntimeInterface negotiate=(PFN_xrNegotiateLoaderRuntimeInterface)GetProcAddress(dll,"xrNegotiateLoaderRuntimeInterface");
 XrResult r=negotiate(info,request);trace("NEGOTIATE %d",r);
 if(r>=0){real_get=request->getInstanceProcAddr;request->getInstanceProcAddr=getproc;trace_mod();}
 return r;
}
'''
(root / 'trace_runtime.c').write_text(source)
print('Generated', len(decls), 'wrappers')
