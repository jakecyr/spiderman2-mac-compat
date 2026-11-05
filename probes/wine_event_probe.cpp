#include <windows.h>
#include <d3d11_4.h>
#include <d3d12.h>
#include <stdio.h>
#define CHECK(x) do {HRESULT check_result=(x);printf(#x " = %lx\n",check_result);if(FAILED(check_result))return 1;}while(0)
int main(){
 ID3D11Device*d11=nullptr;D3D_FEATURE_LEVEL level;CHECK(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d11,&level,nullptr));
 char run[64],path[512];GetEnvironmentVariableA("SM2_NATIVE_RUN",run,sizeof(run));sprintf(path,"Z:\\Users\\YOUR_USER\\Games\\VR-Tests\\SpiderMan2-REALVR\\Debug\\native-bridge-%s.txt",run);FILE*f=fopen(path,"r");unsigned long long a=0;if(f){fscanf(f,"%llx",&a);fclose(f);}if(!a)return 2;
 auto bridge=(UINT(*)(void*,UINT))(uintptr_t)a;bridge(d11,11);
 ID3D12Device*d12=nullptr;CHECK(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,__uuidof(ID3D12Device),(void**)&d12));bridge(d12,12);
 ID3D11Device5*dev5=nullptr;CHECK(d11->QueryInterface(__uuidof(ID3D11Device5),(void**)&dev5));
 ID3D11Fence*producer=nullptr;CHECK(dev5->CreateFence(0,D3D11_FENCE_FLAG_SHARED,__uuidof(ID3D11Fence),(void**)&producer));HANDLE h=nullptr;CHECK(producer->CreateSharedHandle(nullptr,GENERIC_ALL,nullptr,&h));
 ID3D12Fence*consumer=nullptr;CHECK(d12->OpenSharedHandle(h,__uuidof(ID3D12Fence),(void**)&consumer));
 D3D12_COMMAND_QUEUE_DESC qd={};ID3D12CommandQueue*q=nullptr;CHECK(d12->CreateCommandQueue(&qd,__uuidof(ID3D12CommandQueue),(void**)&q));CHECK(q->Signal(consumer,1));
 for(int i=0;i<200 && producer->GetCompletedValue()<1;i++)Sleep(10);
 auto value=producer->GetCompletedValue();printf("Reverse GPU signal producer=%llu consumer=%llu\n",value,consumer->GetCompletedValue());
 HANDLE event=CreateEventA(nullptr,TRUE,FALSE,nullptr);
 CHECK(consumer->SetEventOnCompletion(1,event));
 DWORD wait=WaitForSingleObject(event,2000);printf("Wine event wait=%lu\n",wait);CloseHandle(event);
 return value==1 && wait==WAIT_OBJECT_0?0:3;
}
