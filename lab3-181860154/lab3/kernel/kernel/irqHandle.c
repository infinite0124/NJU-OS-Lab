#include "x86.h"
#include "device.h"

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallPrint(struct TrapFrame *tf);
void syscallFork(struct TrapFrame *tf);
void syscallExec(struct TrapFrame *tf);
void syscallSleep(struct TrapFrame *tf);
void syscallExit(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void timerHandle(struct TrapFrame *tf);
void keyboardHandle(struct TrapFrame *tf);

void irqHandle(struct TrapFrame *tf) { // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));

	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)tf;

	switch(tf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(tf); // return
			break;
		case 0x20:
			timerHandle(tf);         // return or iret
			break;
		case 0x21:
			keyboardHandle(tf);      // return
			break;
		case 0x80:
			syscallHandle(tf);       // return
			break;
		default:assert(0);
	}

	pcb[current].stackTop = tmpStackTop;
}

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case 0:
			syscallWrite(tf);
			break; // for SYS_WRITE
		case 1:
			syscallFork(tf);
			break; // for SYS_FORK
		case 2:
			syscallExec(tf);
			break; // for SYS_EXEC
		case 3:
			syscallSleep(tf);
			break; // for SYS_SLEEP
		case 4:
			syscallExit(tf);
			break; // for SYS_EXIT
		default:break;
	}
}

void timerHandle(struct TrapFrame *tf) {
	// TODO in lab3-ing
	for(int i=0;i<MAX_PCB_NUM;i++)
	{
		if(pcb[i].state==STATE_BLOCKED)
			if((--pcb[i].sleepTime)==0)
				pcb[i].state=STATE_RUNNABLE;
	}
	int pre_cur=current;
	int flag=0;
	if(pcb[current].state==STATE_RUNNING)
	{
		if((++pcb[current].timeCount)>=MAX_TIME_COUNT)
		{
			pcb[current].state=STATE_RUNNABLE;
			pcb[current].timeCount=0;
			flag=1;
		}
	}
	if((pcb[current].state==STATE_BLOCKED)||(pcb[current].state==STATE_DEAD)||flag)
	{
		int i;
		for(i=current+1;i<MAX_PCB_NUM;i++)
		{
			if(pcb[i].state==STATE_RUNNABLE)
			{
				current=i;
				break;
			}
		}
		if(i==MAX_PCB_NUM)
		{
			for(i=1;i<current;i++)
			{
				if(pcb[i].state==STATE_RUNNABLE)
				{
					current=i;
					break;
				}
			}
		}
	}

	if(current==pre_cur)
	{
		if(pcb[current].state==STATE_BLOCKED||pcb[current].state==STATE_DEAD)
			current=0;
	}
	pcb[current].state=STATE_RUNNING;
	//putChar('T');
	//putChar('0'+current);putChar('\n');
	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].stackTop = pcb[current].prevStackTop;
	tss.esp0 = (uint32_t)&(pcb[current].stackTop);
	asm volatile("movl %0, %%esp"::"m"(tmpStackTop)); // switch kernel stack
	asm volatile("popl %gs");
	asm volatile("popl %fs");
	asm volatile("popl %es");
	asm volatile("popl %ds");
	asm volatile("popal");
	asm volatile("addl $8, %esp");
	asm volatile("iret");
}

void keyboardHandle(struct TrapFrame *tf) {
	uint32_t keyCode = getKeyCode();
	if (keyCode == 0)
		return;
	putChar(getChar(keyCode));
	keyBuffer[bufferTail] = keyCode;
	bufferTail = (bufferTail + 1) % MAX_KEYBUFFER_SIZE;
	return;
}

void syscallWrite(struct TrapFrame *tf) {
	switch(tf->ecx) { // file descriptor
		case 0:
			syscallPrint(tf);
			break; // for STD_OUT
		default:break;
	}
}

void syscallPrint(struct TrapFrame *tf) {
	int sel = tf->ds; //TODO segment selector for user data, need further modification/
	char *str = (char *)tf->edx;
	int size = tf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
		if (character == '\n') {
			displayRow++;
			displayCol = 0;
			if (displayRow == 25){
				displayRow = 24;
				displayCol = 0;
				scrollScreen();
			}
		}
		else {
			data = character | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
			displayCol++;
			if (displayCol == 80){
				displayRow++;
				displayCol = 0;
				if (displayRow == 25){
					displayRow = 24;
					displayCol = 0;
					scrollScreen();
				}
			}
		}
	}
	
	updateCursor(displayRow, displayCol);
	//TODO take care of return value
}

void syscallFork(struct TrapFrame *tf) {
	// TODO in lab3-ing
	//putChar('F');
	//putChar('0'+current);putChar('\n');
	int pos=0;
	for(int i=0;i<MAX_PCB_NUM;i++)
	{
		if(pcb[i].state==STATE_DEAD)
		{
			pos=i;
			break;
		}
	}
	if(pos==0)//fail
	{
		pcb[current].regs.eax=-1;
		return;
	}

	enableInterrupt();
	//copy data and code
	for(int i=0;i<0x100000;i++)
	{
		*((uint8_t *)((pos+1)*0x100000+i))=*((uint8_t *)((current+1)*0x100000+i));
		asm volatile("int $0x20"); //XXX Testing irqTimer during syscall
	}
	disableInterrupt();

	//set pcb
	for(int i=0;i<sizeof(ProcessTable);i++)
		*((uint8_t *)&pcb[pos]+i)=*((uint8_t *)&pcb[current]+i);
	//*((uint8_t *)(&pcb[pos]+i))=*((uint8_t *)(&pcb[current]+i));
	pcb[pos].stackTop=(uint32_t)&(pcb[pos].regs);
	pcb[pos].prevStackTop=(uint32_t)&(pcb[pos].stackTop);
	pcb[pos].state=STATE_RUNNABLE;
	pcb[pos].timeCount=0;
	pcb[pos].sleepTime=0;
	pcb[pos].pid=pos;
	pcb[pos].regs.gs=USEL(2*pos+2);
	pcb[pos].regs.fs=USEL(2*pos+2);
	pcb[pos].regs.es=USEL(2*pos+2);
	pcb[pos].regs.ds=USEL(2*pos+2);
	pcb[pos].regs.cs=USEL(2*pos+1);
	pcb[pos].regs.ss=USEL(2*pos+2);
	pcb[pos].regs.eax=0;
	pcb[current].regs.eax=pos;
}

void syscallExec(struct TrapFrame *tf) {
	// TODO in lab3-ing
	// hint: ret = loadElf(tmp, (current + 1) * 0x100000, &entry);
	int sel = tf->ds; //TODO segment selector for user data, need further modification/
	char *str = (char *)tf->edx;
	int i = 0;
	char character = 1;
	char tmp[100];
	uint32_t entry;
	asm volatile("movw %0, %%es"::"m"(sel));
	while(character!=0){
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
		tmp[i]=character;
		//putChar(tmp[i]);
		i++;
		assert(i<100);
	}
	int ret=loadElf(tmp,(current+1)*0x100000,&entry);
	if(ret==-1)
		pcb[current].regs.eax=-1;
	else
		tf->eip=entry;
}

void syscallSleep(struct TrapFrame *tf) {
	// TODO in lab3-ing
	assert((int)(tf->ecx)>0);
	pcb[current].sleepTime=(int)(tf->ecx);
	//putChar('0'+(uint32_t)tf->ecx);
	pcb[current].state=STATE_BLOCKED;
	asm volatile("int $0x20");
}

void syscallExit(struct TrapFrame *tf) {
	// TODO in lab3-ing
	pcb[current].state=STATE_DEAD;
	asm volatile("int $0x20");
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}
