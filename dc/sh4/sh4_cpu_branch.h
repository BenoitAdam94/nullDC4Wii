//Specialised version for recompiler ;)
#ifdef SH4_REC
#include "sh4_cpu_branch_rec.h"
#else

//braf <REG_N>
sh4op(i0000_nnnn_0010_0011)
{
	u32 n = GetN(op);
	u32 newpc = r[n] + next_pc + 2;//
	ExecuteDelayslot();	//WARN : r[n] can change here
	next_pc = newpc;
}
//bsrf <REG_N>
 sh4op(i0000_nnnn_0000_0011)
{
	u32 n = GetN(op);
	u32 newpc = r[n] + next_pc +2;
	pr = next_pc + 2;		   //after delayslot
	ExecuteDelayslot();	//WARN : pr and r[n] can change here
	next_pc = newpc;
}


 //rte
 sh4op(i0000_0000_0010_1011)
{
	//iNimp("rte");
	//sr.SetFull(ssr);
	u32 newpc = spc;
	ExecuteDelayslot_RTE();
	next_pc = newpc;
	if (UpdateSR())
	{
		//FIXME olny if interrupts got on .. :P
		UpdateINTC();
	}
}


//rts
 sh4op(i0000_0000_0000_1011)
{
	u32 newpc=pr;
	ExecuteDelayslot();	//WARN : pr can change here
	next_pc=newpc;
}

u32 branch_target_s8(u32 op)
{
	return GetSImm8(op)*2 + 2 + next_pc;
}
// bf <bdisp8>
 sh4op(i1000_1011_iiii_iiii)
{//ToDo : Check Me [26/4/05]  | Check DELAY SLOT [28/1/06]
	if (sr.T==0)
	{
		//direct jump
		next_pc = branch_target_s8(op);
	}
}


// bf.s <bdisp8>
 sh4op(i1000_1111_iiii_iiii)
{
	if (sr.T==0)
	{
		//delay 1 instruction
		u32 newpc=branch_target_s8(op);
		ExecuteDelayslot();
		next_pc = newpc;
	}
}


// bt <bdisp8>
 sh4op(i1000_1001_iiii_iiii)
{
	if (sr.T != 0)
	{
		//direct jump
		next_pc = branch_target_s8(op);
	}
}


// bt.s <bdisp8>
 sh4op(i1000_1101_iiii_iiii)
{
	if (sr.T != 0)
	{
		//delay 1 instruction
		u32 newpc=branch_target_s8(op);
		ExecuteDelayslot();
		next_pc = newpc;
	}
}

u32 branch_target_s12(u32 op)
{
	return GetSImm12(op)*2 + 2 + next_pc;
}

// bra <bdisp12>
sh4op(i1010_iiii_iiii_iiii)
{
	u32 newpc = branch_target_s12(op);//(u32) ((  ((s16)((GetImm12(op))<<4)) >>3)  + pc + 4);//(s16<<4,>>4(-1*2))
	ExecuteDelayslot();
	next_pc=newpc;
}

// bsr <bdisp12>
sh4op(i1011_iiii_iiii_iiii)
{
	pr = next_pc + 2;					//return after delayslot
	u32 newpc = branch_target_s12(op);
	ExecuteDelayslot();
	next_pc=newpc;
}

// trapa #<imm>
sh4op(i1100_0011_iiii_iiii)
{
	//printf("trapa 0x%X\n",(GetImm8(op) << 2));
	CCN_TRA = (GetImm8(op) << 2);
	Do_Exeption(next_pc,0x160,0x100);
}

//jmp @<REG_N>
 sh4op(i0100_nnnn_0010_1011)
{
	u32 n = GetN(op);

	u32 newpc=r[n];
	ExecuteDelayslot();	//r[n] can change here
	next_pc=newpc;
}

//jsr @<REG_N>
 sh4op(i0100_nnnn_0000_1011)
{
	u32 n = GetN(op);

	pr = next_pc + 2;	//return after delayslot
	u32 newpc= r[n];
	ExecuteDelayslot();	//r[n]/pr can change here
	next_pc=newpc;
}

//sleep
 sh4op(i0000_0000_0001_1011)
{
	//iNimp("Sleep");
	//just wait for an Interrupt

	sh4_sleeping=true;
	int i=0,s=1;

	while (!UpdateSystem())//448
	{
		if (i++>1000)
		{
			s=0;
			break;
		}
	}
	//if not Interrupted , we must rexecute the sleep
	if (s==0)
		next_pc-=2;// re execute sleep

	sh4_sleeping=false;
}
#endif
