#include <core/eos.h>
#include <core/eos_internal.h>
#include "emulator_asm.h"

typedef struct _os_context {
	/* low address */
	int32u_t reg_edi;
	int32u_t reg_esi;
	int32u_t reg_ebp;
	int32u_t reg_esp;
	int32u_t reg_ebx;
	int32u_t reg_edx;
	int32u_t reg_ecx;
	int32u_t reg_eax;
	int32u_t reg_eflags;
	int32u_t reg_eip;
	/* high address */	
} _os_context_t;

void print_context(addr_t context) {
	if(context == NULL) return;
	_os_context_t *ctx = (_os_context_t *)context;
	PRINT("eip  =0x%x\n", ctx->reg_eip);
	//PRINT("eax  =0x%x\n", ctx->reg_eax);
	//PRINT("ecx  =0x%x\n", ctx->reg_ecx);
	//PRINT("edx  =0x%x\n", ctx->reg_edx);
	//PRINT("ebx  =0x%x\n", ctx->reg_ebx);
	PRINT("esp  =0x%x\n", ctx->reg_esp);
	PRINT("ebp  =0x%x\n", ctx->reg_ebp);
	//PRINT("esi  =0x%x\n", ctx->reg_esi);
	//PRINT("edi  =0x%x\n", ctx->reg_edi);
		
}

addr_t _os_create_context(addr_t stack_base, size_t stack_size, void (*entry)(void *), void *arg) {
	int32u_t *stack_top = stack_base + stack_size;
	*(stack_top - 1) = arg; //1.save arg on stack
	*(stack_top - 2) = NULL; //save entry ftn's return addr=NULL on stack
	*(stack_top - 3) = (int32u_t)entry; //2.save context
	*(stack_top - 4) = 1;
	for (int i=5; i<12; i++){
		*(stack_top - i) = NULL;
	}
	
	_os_context_t *ctx = (_os_context_t *)(stack_top - 12);
	//PRINT("eip : %d, eflags : %d\n", ctx->reg_eip, ctx->reg_eflags);
	return (addr_t)(stack_top - 12); //3.return value is the lowest addr

}

void _os_restore_context(addr_t sp) {
	//PRINT("SP : 0x%x\n", sp);
	__asm__ __volatile__(
	"movl %0, %%esp;" //1.move esp to sp+0, input exist -> double %% before register names
	"popa;" //2.old ebp, old eip poped so just pop all regs(from edi to eax)
	"pop _eflags;" //pop and restore _eflags
	"ret;"
	::"m"(sp) //input as sp ptr from memory
	);
	//print_context(sp);
	//__asm__ __volatile(
	//"ret;" //3&4.pop, eip changd to eip: resume_point, control flow changed
	//);
}

addr_t _os_save_context() {
	__asm__ __volatile__(
	"push $resume_point;" //1.saving eip: resume_point
	"push _eflags;" //saving _eflags (maybe _eflags_saved??)
	"mov $0, %eax;" //null after return, no input & output -> single % before register names
	"pusha;" //push regs(from eax to edi)
	"mov %esp, %eax;" // 2.return sp on eax
	"push 0x4(%ebp);" //3.save old eip
	"push (%ebp);" //save old ebp
	"mov %esp, %ebp;" //ebp changed to esp
"resume_point:"
	"leave;"
	"ret;"
	);
}
