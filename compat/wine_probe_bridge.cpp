// Local diagnostic adapter: reuse sharing hooks without replacing Wine's GFXT host.
#include <mach-o/dyld.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include "dmn_share.h"
#include "d3dmetal_native.h"
extern "C" void dmn_hooks_after_d3d11_device(void*);
extern "C" void dmn_hooks_after_d3d12_device(void*);
static void *framework_handle;
static char framework_path[1024];
static uint32_t __attribute__((ms_abi)) bridge(void *device,uint32_t api){
 FILE*f=fopen("/Users/YOUR_USER/Games/VR-Tests/SpiderMan2-REALVR/Debug/native-load.log","a");if(f){fprintf(f,"bridge entered device=%p api=%u\n",device,api);fclose(f);}
 if(api==50){Dl_info info={};void*fn=*(void**)device;if(!dladdr(fn,&info)||!info.dli_sname)return 0;
  const char*n=info.dli_sname;bool allowed=(strstr(n,"D3D12Resource")||strstr(n,"D3D12Texture")||strstr(n,"D3D12Buffer"))&&(strstr(n,"SetName")||strstr(n,"SetPrivateData"));
  FILE*f=fopen("/Users/YOUR_USER/Games/VR-Tests/SpiderMan2-REALVR/Debug/native-hook-symbols.log","a");if(f){fprintf(f,"%s allowed=%d\n",n,allowed);fclose(f);}return allowed;
 }
 if(api==11)dmn_hooks_after_d3d11_device(device);
 else if(api==12){
  if(device)dmn_hooks_after_d3d12_device(device);
  else {
   struct Guid {uint32_t a;uint16_t b,c;uint8_t d[8];};
   static const Guid iid={0x189819f1,0x1db6,0x4b57,{0xbe,0x54,0x18,0x21,0x33,0x9b,0x85,0xf7}};
   typedef int32_t(__attribute__((ms_abi))*Create)(void*,uint32_t,const void*,void**);
   if(!framework_handle && framework_path[0])framework_handle=dlopen(framework_path,RTLD_NOW|RTLD_NOLOAD);
   Create create=(Create)(framework_handle?dlsym(framework_handle,"D3D12CreateDevice"):nullptr);
   if(!create){FILE*f=fopen("/Users/YOUR_USER/Games/VR-Tests/SpiderMan2-REALVR/Debug/native-load.log","a");if(f){fprintf(f,"framework path=%s handle=%p error=%s\n",framework_path,framework_handle,dlerror());fclose(f);}return 2;}void*raw=nullptr;int32_t hr=create(nullptr,0xb000,&iid,&raw);if(hr<0||!raw)return 3;
   dmn_hooks_after_d3d12_device(raw);
   typedef uint32_t(__attribute__((ms_abi))*Release)(void*);
   struct QueueDesc {uint32_t type;int32_t priority;uint32_t flags,nodeMask;};QueueDesc qd={};
   static const Guid qid={0x0ec870a6,0x5d7e,0x4c22,{0x8c,0xfc,0x5b,0xaa,0xe0,0x76,0x16,0xed}};
   typedef int32_t(__attribute__((ms_abi))*MakeQueue)(void*,const QueueDesc*,const void*,void**);
   void*q=nullptr;int32_t qhr=((MakeQueue)(*(void***)raw)[8])(raw,&qd,&qid,&q);
   if(q){((Release)(*(void***)q)[2])(q);}else{((Release)(*(void***)raw)[2])(raw);return (uint32_t)qhr;}

   ((Release)(*(void***)raw)[2])(raw);
  }
 }
 else if(api==99)return dmn_shared_handle_close(device);
 else return 1;
 return 0;
}
static void image_added(const struct mach_header *header,intptr_t slide){
 Dl_info info={};if(!dladdr(header,&info)||!info.dli_fname)return;
 if(strstr(info.dli_fname,"D3D")||strstr(info.dli_fname,"d3d")){FILE*f=fopen("/Users/YOUR_USER/Games/VR-Tests/SpiderMan2-REALVR/Debug/native-load.log","a");if(f){fprintf(f,"image %s\n",info.dli_fname);fclose(f);}}
 FILE*map=fopen("/Users/YOUR_USER/Games/VR-Tests/SpiderMan2-REALVR/Debug/native-images.log","a");if(map){fprintf(map,"%p %s\n",header,info.dli_fname);fclose(map);}
 if(!strstr(info.dli_fname,"/D3DMetal.framework/") || strcmp(strrchr(info.dli_fname,'/')+1,"D3DMetal"))return;
 snprintf(framework_path,sizeof(framework_path),"%s",info.dli_fname);
 // This framework loads after Wine initialized the Windows TEB in GS.
 uintptr_t winpid=0;asm volatile("movq %%gs:0x40,%0":"=r"(winpid));
 const char *run=getenv("SM2_NATIVE_RUN");if(!run)return;
 char path[512];snprintf(path,sizeof(path),"/Users/YOUR_USER/Games/VR-Tests/SpiderMan2-REALVR/Debug/native-bridge-%s-%lu.txt",run,(unsigned long)winpid);
 FILE*f=fopen(path,"w");if(f){fprintf(f,"%llx\n",(unsigned long long)(uintptr_t)&bridge);fclose(f);}
}
__attribute__((constructor)) static void initialize_bridge(){
 FILE*f=fopen("/Users/YOUR_USER/Games/VR-Tests/SpiderMan2-REALVR/Debug/native-load.log","a");if(f){fprintf(f,"constructor\n");fclose(f);}
 const char *run=getenv("SM2_NATIVE_RUN");if(run){char p[512];snprintf(p,sizeof(p),"/Users/YOUR_USER/Games/VR-Tests/SpiderMan2-REALVR/Debug/native-bridge-%s.txt",run);FILE*f=fopen(p,"w");if(f){fprintf(f,"%llx\n",(unsigned long long)(uintptr_t)&bridge);fclose(f);}}
 dmn_share_install_swizzles();
 _dyld_register_func_for_add_image(image_added);
}
