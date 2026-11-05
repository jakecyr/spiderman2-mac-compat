#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
extern int probe_writable(void *);
extern char probe_begin[],probe_end[],probe_fail[];
static BOOL (WINAPI *real_protect)(LPVOID,SIZE_T,DWORD,PDWORD)=VirtualProtect;
static void logmsg(const char *s,void *p,DWORD x){FILE*f=fopen("C:\\openxr\\protect-trace.log","a");if(f){fprintf(f,"%lu %s %p %lu\n",GetTickCount(),s,p,x);fclose(f);}}
static LONG CALLBACK probe_exception(PEXCEPTION_POINTERS e){
 uintptr_t ip=e->ContextRecord->Rip;
 if(e->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION && ip>=(uintptr_t)probe_begin && ip<(uintptr_t)probe_end){e->ContextRecord->Rip=(DWORD64)probe_fail;return EXCEPTION_CONTINUE_EXECUTION;}
 return EXCEPTION_CONTINUE_SEARCH;
}
static int allow_resource_hook(void*p){
 char run[64];if(!GetEnvironmentVariableA("SM2_NATIVE_RUN",run,sizeof(run)))return 0;
 char path[512];snprintf(path,sizeof(path),"Z:\\Users\\YOUR_USER\\Games\\VR-Tests\\SpiderMan2-REALVR\\Debug\\native-bridge-%s.txt",run);
 FILE*f=fopen(path,"r");unsigned long long a=0;if(f){fscanf(f,"%llx",&a);fclose(f);}if(!a)return 0;
 UINT(*bridge)(void*,UINT)=(void*)(uintptr_t)a;return bridge(p,50)!=0;
}
__declspec(dllexport) BOOL WINAPI compatVirtualProtect(LPVOID p,SIZE_T n,DWORD prot,PDWORD old){
 BOOL ok=real_protect(p,n,prot,old);DWORD err=GetLastError();if(ok)return ok;
 /* ReShade vtable writes only. Never fake protection for code patches or change permissions. */
 if(n==sizeof(void*) && prot==PAGE_READWRITE && old && ((uintptr_t)p%sizeof(void*)==0)){
  MEMORY_BASIC_INFORMATION info={0};
  if(VirtualQuery(p,&info,sizeof(info)) && info.State==MEM_FREE && probe_writable(p) && allow_resource_hook(p)){
   *old=PAGE_READWRITE;logmsg("already-writable native vtable",p,err);return TRUE;
  }
 }
 SetLastError(err);return FALSE;
}
static void patch_module(unsigned char *base){
 IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)base;if(!base||dos->e_magic!=IMAGE_DOS_SIGNATURE)return;
 IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS*)(base+dos->e_lfanew);
 /* Only the installed REAL VR build; compare its known geometry-call instruction. */
 const unsigned char expected[]={0xe8,0xa9,0x9d,0xff,0xff};
 if(nt->OptionalHeader.SizeOfImage<0x279897 || memcmp(base+0x279892,expected,5))return;
 DWORD rva=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;if(!rva)return;
 IMAGE_IMPORT_DESCRIPTOR *imp=(IMAGE_IMPORT_DESCRIPTOR*)(base+rva);
 for(;imp->Name;imp++){
  if(!imp->OriginalFirstThunk)continue;
  IMAGE_THUNK_DATA *names=(IMAGE_THUNK_DATA*)(base+imp->OriginalFirstThunk),*iat=(IMAGE_THUNK_DATA*)(base+imp->FirstThunk);
  for(;names->u1.AddressOfData;names++,iat++){
   if(IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))continue;
   IMAGE_IMPORT_BY_NAME *name=(IMAGE_IMPORT_BY_NAME*)(base+names->u1.AddressOfData);
   if(strcmp((char*)name->Name,"VirtualProtect"))continue;
   DWORD old;if(real_protect(&iat->u1.Function,sizeof(void*),PAGE_READWRITE,&old)){
    iat->u1.Function=(ULONG_PTR)compatVirtualProtect;DWORD unused;real_protect(&iat->u1.Function,sizeof(void*),old,&unused);logmsg("REAL VR VirtualProtect import patched",base,0);
   }
  }
 }
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved){
 if(reason==DLL_PROCESS_ATTACH){
  DisableThreadLibraryCalls(instance);AddVectoredExceptionHandler(1,probe_exception);
  PEB *peb=(PEB*)__readgsqword(0x60);LIST_ENTRY *head=&peb->Ldr->InMemoryOrderModuleList;
  for(LIST_ENTRY *item=head->Flink;item!=head;item=item->Flink){
   LDR_DATA_TABLE_ENTRY *entry=CONTAINING_RECORD(item,LDR_DATA_TABLE_ENTRY,InMemoryOrderLinks);patch_module((unsigned char*)entry->DllBase);
  }
 }
 return TRUE;
}
