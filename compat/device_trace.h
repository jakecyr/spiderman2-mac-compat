static __thread int runtime_swapchain_creation;
static HRESULT (STDMETHODCALLTYPE *native_create_texture)(ID3D11Device*,const D3D11_TEXTURE2D_DESC*,const D3D11_SUBRESOURCE_DATA*,ID3D11Texture2D**);
static HRESULT STDMETHODCALLTYPE share_runtime_texture(ID3D11Device*d,const D3D11_TEXTURE2D_DESC*desc,const D3D11_SUBRESOURCE_DATA*data,ID3D11Texture2D**out){
 D3D11_TEXTURE2D_DESC copy=*desc;
 if(runtime_swapchain_creation && copy.MiscFlags==0){copy.MiscFlags=D3D11_RESOURCE_MISC_SHARED_NTHANDLE;trace("runtime texture shared backing requested %ux%u",copy.Width,copy.Height);}
 return native_create_texture(d,&copy,data,out);
}
static int install_native_sharing(ID3D11Device*d){
 char run[64];if(!GetEnvironmentVariableA("SM2_NATIVE_RUN",run,sizeof(run)))return 0;
 char path[512];snprintf(path,sizeof(path),"Z:\\Users\\YOUR_USER\\Games\\VR-Tests\\SpiderMan2-REALVR\\Debug\\native-bridge-%s.txt",run);
 FILE*f=fopen(path,"r");unsigned long long address=0;if(f){fscanf(f,"%llx",&address);fclose(f);}
 if(!address){trace("native sharing bridge missing");return 1;}
 D3D11_TEXTURE2D_DESC desc={0};desc.Width=desc.Height=desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 ID3D11Texture2D*test=NULL;ID3D11Device*raw=NULL;HRESULT unwrap=d->lpVtbl->CreateTexture2D(d,&desc,NULL,&test);
 if(SUCCEEDED(unwrap)&&test){test->lpVtbl->GetDevice(test,&raw);test->lpVtbl->Release(test);}
 MEMORY_BASIC_INFORMATION info={0};
 if(!raw || !VirtualQuery(raw->lpVtbl,&info,sizeof(info)) || info.State!=MEM_FREE){trace("native unwrap unavailable device=%p raw=%p",d,raw);if(raw)raw->lpVtbl->Release(raw);return 1;}
 trace("native unwrapped device=%p raw=%p",d,raw);d=raw;
 UINT(*bridge)(void*,UINT)=(void*)(uintptr_t)address;
 trace("native D3D11 sharing install result=%u",bridge(d,11));
 if(d->lpVtbl->CreateTexture2D!=share_runtime_texture){native_create_texture=d->lpVtbl->CreateTexture2D;InterlockedExchangePointer((PVOID*)&d->lpVtbl->CreateTexture2D,share_runtime_texture);}
 trace("native D3D12 class hooks result=%u",bridge(NULL,12));
 raw->lpVtbl->Release(raw);
 return 1;
}
typedef struct DeviceRecord { ID3D11Device *device; const ID3D11DeviceVtbl *original; ID3D11DeviceVtbl copy; } DeviceRecord;
static DeviceRecord devices[32]; static unsigned device_count;
static DeviceRecord *record(ID3D11Device *d){for(unsigned i=0;i<device_count;i++)if(devices[i].device==d)return &devices[i];return NULL;}
static HRESULT STDMETHODCALLTYPE query(ID3D11Device *d,REFIID id,void **out){DeviceRecord *p=record(d);HRESULT h=p->original->QueryInterface(d,id,out);trace("D3D11 QI %08lx-%04x-%04x -> %08lx ptr=%p",id->Data1,id->Data2,id->Data3,h,out?*out:0);return h;}
static HRESULT STDMETHODCALLTYPE texture(ID3D11Device *d,const D3D11_TEXTURE2D_DESC *desc,const D3D11_SUBRESOURCE_DATA *data,ID3D11Texture2D **out){DeviceRecord *p=record(d);HRESULT h=p->original->CreateTexture2D(d,desc,data,out);trace("D3D11 CreateTexture2D %ux%u fmt=%u bind=%x misc=%x usage=%u cpu=%x -> %08lx",desc->Width,desc->Height,desc->Format,desc->BindFlags,desc->MiscFlags,desc->Usage,desc->CPUAccessFlags,h);return h;}
static HRESULT STDMETHODCALLTYPE shared(ID3D11Device *d,HANDLE handle,REFIID id,void **out){DeviceRecord *p=record(d);HRESULT h=p->original->OpenSharedResource(d,handle,id,out);trace("D3D11 OpenSharedResource %p -> %08lx",handle,h);return h;}
static HRESULT STDMETHODCALLTYPE format(ID3D11Device *d,DXGI_FORMAT fmt,UINT *support){DeviceRecord *p=record(d);HRESULT h=p->original->CheckFormatSupport(d,fmt,support);trace("D3D11 CheckFormatSupport fmt=%d -> %08lx support=%x",fmt,h,*support);return h;}
static void trace_device(ID3D11Device *d){
 if(!d||record(d)||device_count>=32)return;
 if(install_native_sharing(d))return;
 DeviceRecord *p=&devices[device_count++];p->device=d;p->original=d->lpVtbl;p->copy=*d->lpVtbl;
 p->copy.QueryInterface=query;p->copy.CreateTexture2D=texture;p->copy.OpenSharedResource=shared;p->copy.CheckFormatSupport=format;
 DWORD protection;if(VirtualProtect(d,sizeof(void*),PAGE_READWRITE,&protection)){d->lpVtbl=&p->copy;DWORD unused;VirtualProtect(d,sizeof(void*),protection,&unused);trace("D3D11 trace installed device=%p",d);}else trace("D3D11 trace failed %lu",GetLastError());
}
