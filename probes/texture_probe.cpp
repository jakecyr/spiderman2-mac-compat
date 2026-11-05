#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_2.h>
#include <stdio.h>
int main(){
 ID3D11Device *d=nullptr;D3D_FEATURE_LEVEL level;HRESULT h=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,&level,nullptr);
 printf("D3D11CreateDevice=%lx\n",h);if(FAILED(h))return 1;
 UINT(*native_bridge)(void*,UINT)=nullptr;
 ID3D12Device *d12=nullptr;
 char run[64];if(GetEnvironmentVariableA("SM2_NATIVE_RUN",run,sizeof(run))){
  char path[512];sprintf(path,"Z:\\Users\\YOUR_USER\\Games\\VR-Tests\\SpiderMan2-REALVR\\Debug\\native-bridge-%s.txt",run);FILE*f=fopen(path,"r");unsigned long long address=0;if(f){fscanf(f,"%llx",&address);fclose(f);}printf("native bridge pid=%lu found=%d\n",GetCurrentProcessId(),address!=0);if(!address)return 5;
  auto bridge=(UINT(*)(void*,UINT))(uintptr_t)address;printf("native bridge result=%u\n",bridge(d,11));native_bridge=bridge;
 h=D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,__uuidof(ID3D12Device),(void**)&d12);printf("D3D12CreateDevice=%lx\n",h);if(d12)bridge(d12,12);
 }
 void **vt=*(void***)d;MEMORY_BASIC_INFORMATION mbi={};SIZE_T queried=VirtualQuery(vt,&mbi,sizeof(mbi));DWORD old=0;BOOL prot=VirtualProtect(vt,sizeof(void*),PAGE_READWRITE,&old);DWORD err=GetLastError();printf("VTABLE %p VirtualQuery=%llu state=%lx protect=%lx type=%lx VirtualProtect=%d error=%lu old=%lx\n",vt,(unsigned long long)queried,mbi.State,mbi.Protect,mbi.Type,prot,err,old);if(prot){DWORD tmp;VirtualProtect(vt,sizeof(void*),old,&tmp);}
 void *fn=vt[0];*(void* volatile*)vt=fn;printf("VTABLE same-value direct write succeeded\n");
 for(UINT flags: {0u,2u,256u,2048u,2304u}){
  D3D11_TEXTURE2D_DESC desc={};desc.Width=1024;desc.Height=1024;desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;desc.MiscFlags=flags;
  ID3D11Texture2D *t=nullptr;h=d->CreateTexture2D(&desc,nullptr,&t);printf("CreateTexture2D misc=%x HRESULT=%lx\n",flags,h);
  if(t){IDXGIResource *res=nullptr;h=t->QueryInterface(__uuidof(IDXGIResource),(void**)&res);printf(" IDXGIResource=%lx",h);if(res){HANDLE handle=nullptr;h=res->GetSharedHandle(&handle);printf(" GetSharedHandle=%lx handle=%p",h,handle);if(SUCCEEDED(h)){ID3D11Texture2D *other=nullptr;h=d->OpenSharedResource(handle,__uuidof(ID3D11Texture2D),(void**)&other);printf(" OpenSharedResource=%lx",h);if(other)other->Release();}res->Release();}printf("\n");
   IDXGIResource1 *r1=nullptr;h=t->QueryInterface(__uuidof(IDXGIResource1),(void**)&r1);printf(" IDXGIResource1=%lx",h);if(r1){HANDLE handle=nullptr;h=r1->CreateSharedHandle(nullptr,DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE,nullptr,&handle);printf(" CreateSharedHandle=%lx handle=%p",h,handle);if(handle && d12){ID3D12Resource* imported=nullptr;HRESULT x=d12->OpenSharedHandle(handle,__uuidof(ID3D12Resource),(void**)&imported);printf(" D3D12OpenSharedHandle=%lx",x);if(imported){void**vt=*(void***)imported;printf(" NameHookAllowed=%u PrivateHookAllowed=%u",native_bridge(vt+6,50),native_bridge(vt+4,50));auto rd=imported->GetDesc();printf(" size=%llux%u",rd.Width,rd.Height);imported->Release();}}if(handle && native_bridge)native_bridge(handle,99);r1->Release();}printf("\n");t->Release();}
 }
 if(d12)d12->Release();d->Release();return 0;
}
