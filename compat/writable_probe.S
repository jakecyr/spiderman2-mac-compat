.text
.globl probe_writable,probe_begin,probe_end,probe_fail
probe_writable:
 xor %eax,%eax
 xor %edx,%edx
probe_begin:
 lock cmpxchgq %rdx,(%rcx)
probe_end:
 mov $1,%eax
 ret
probe_fail:
 xor %eax,%eax
 ret
