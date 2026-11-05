/* Version-specific instrumentation: verified call-site bytes only; no altered return values. */
typedef unsigned char (*GeometryFn)(unsigned, const float*, unsigned long long, const float*, void*, void*, float*);
static GeometryFn original_geometry;
static unsigned char* diagnostic_owner;
static unsigned char geometry_trace(unsigned mode,const float *params,unsigned long long dims,const float *fov,void *a,void *b,float *c){
 diagnostic_owner=(unsigned char*)fov-0xbc;
 trace("MOD GEOMETRY mode=%u dimensions=%ux%u params=%f,%f,%f,%f fov=%f,%f,%f,%f / %f,%f,%f,%f",mode,(unsigned)dims,(unsigned)(dims>>32),params[0],params[1],params[2],params[3],fov[0],fov[1],fov[2],fov[3],fov[4],fov[5],fov[6],fov[7]);
 FILE *cfg=fopen("C:\\openxr\\geometry_fix.txt","r");unsigned width=0,height=0;
 if(cfg){fscanf(cfg,"%u %u",&width,&height);fclose(cfg);}
 if(!dims && width>=256 && width<=4096 && height>=256 && height<=4096){
  unsigned char *owner=(unsigned char*)fov-0xbc;
  unsigned long long *owner_size=(unsigned long long*)(owner+0x18);
  if(*owner_size==dims){dims=((unsigned long long)height<<32)|width;*owner_size=dims;trace("MOD EXPERIMENT supplied missing image size %ux%u",width,height);}
 }
 if(width>=256 && width<=4096 && height>=256 && height<=4096){
  unsigned char*owner=(unsigned char*)fov-0xbc;unsigned long long*hud=(unsigned long long*)(owner+0x20);
  if(!*hud){*hud=((unsigned long long)height<<32)|width;trace("MOD EXPERIMENT supplied missing HUD size %ux%u",width,height);}
 }
 unsigned char result=original_geometry(mode,params,dims,fov,a,b,c);trace("MOD GEOMETRY result=%u",result);return result;
}
static void trace_mod(void){
 unsigned char *base=(unsigned char*)GetModuleHandleA("dxgi.dll");if(!base)return;
 IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)base;if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return;
 IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS*)(base+dos->e_lfanew);if(nt->OptionalHeader.SizeOfImage<0x279897){trace("MOD dxgi is not the expected mod");return;}
 unsigned char *site=base+0x279892;
 const unsigned char expected[]={0xe8,0xa9,0x9d,0xff,0xff};
 if(memcmp(site,expected,5)){trace("MOD trace not installed: callsite differs");return;}
 intptr_t delta=(unsigned char*)&geometry_trace-(site+5);
 if(delta>INT32_MAX||delta<INT32_MIN){trace("MOD trace not installed: branch range");return;}
 DWORD old;if(!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&old)){trace("MOD trace protect failed");return;}
 original_geometry=(GeometryFn)(base+0x273640);int32_t d=(int32_t)delta;memcpy(site+1,&d,4);DWORD dummy;VirtualProtect(site,5,old,&dummy);FlushInstructionCache(GetCurrentProcess(),site,5);trace("MOD geometry tracing installed");
}

static void trace_mod_frames(void){
 static unsigned tick;if(!diagnostic_owner || (++tick%30))return;
 for(unsigned eye=0;eye<2;eye++){
  unsigned char*b=diagnostic_owner+0x1f8+eye*0x660;
  trace("MOD RING eye=%u count=%u read=%u pending=%u slots=%lld,%lld,%lld",eye,*(unsigned*)(b+4),*(unsigned*)(b+0x4f0),*(unsigned*)(b+0x4f4),*(long long*)(b+0x10),*(long long*)(b+0xe0),*(long long*)(b+0x1b0));
  void*fence=*(void**)(b+0x10+0x58);if(fence){unsigned long long(STDMETHODCALLTYPE*completed)(void*)=(void*)(*(void***)fence)[8];trace("MOD FENCE eye=%u fence=%p completed=%llu target=%llu",eye,fence,completed(fence),*(unsigned long long*)(b+0x10+0x70));}

 }
}
