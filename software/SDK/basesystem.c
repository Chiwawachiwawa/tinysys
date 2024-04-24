#include "basesystem.h"
#include "encoding.h"

// Utilities

uint64_t E32ReadTime()
{
   uint32_t clockhigh, clocklow, tmp;
   asm volatile(
      "1:\n"
      "rdtimeh %0\n"
      "rdtime %1\n"
      "rdtimeh %2\n"
      "bne %0, %2, 1b\n"
      : "=&r" (clockhigh), "=&r" (clocklow), "=&r" (tmp)
   );

   uint64_t now = ((uint64_t)(clockhigh)<<32) | clocklow;
   return now;
}

uint64_t E32ReadCycles()
{
   uint32_t cyclehigh, cyclelow, tmp;
   asm volatile(
      "1:\n"
      "rdcycleh %0\n"
      "rdcycle %1\n"
      "rdcycleh %2\n"
      "bne %0, %2, 1b\n"
      : "=&r" (cyclehigh), "=&r" (cyclelow), "=&r" (tmp)
   );

   uint64_t now = ((uint64_t)(cyclehigh)<<32) | cyclelow;
   return now;
}

void E32Sleep(uint64_t ticks)
{
   // Start time is now in ticks
   uint64_t tstart = E32ReadTime();
   // End time is now plus ms in ticks
   uint64_t tend = tstart + ticks;
   while (E32ReadTime() < tend) { }
}

uint64_t E32ReadRetiredInstructions()
{
   uint32_t retihigh, retilow;

   asm (
      "rdinstreth %0;"
      "rdinstret %1;"
      : "=&r" (retihigh), "=&r" (retilow)
   );

   uint64_t reti = ((uint64_t)(retihigh)<<32) | retilow;

   return reti;
}

uint32_t ClockToMs(uint64_t clk)
{
   return (uint32_t)(clk / ONE_MILLISECOND_IN_TICKS);
}

uint32_t ClockToUs(uint64_t clk)
{
   return (uint32_t)(clk / ONE_MICROSECOND_IN_TICKS);
}

void ClockMsToHMS(uint32_t ms, uint32_t *hours, uint32_t *minutes, uint32_t *seconds)
{
   *hours = ms / 3600000;
   *minutes = (ms % 3600000) / 60000;
   *seconds = ((ms % 360000) % 60000) / 1000;
}

void E32SetTimeCompare(const uint64_t future)
{
   // NOTE: ALWAYS set high word first to avoid misfires outside timer interrupt
   swap_csr(0x801, ((future&0xFFFFFFFF00000000)>>32));         // CSR_TIMECMPHI
   swap_csr(0x800, ((uint32_t)(future&0x00000000FFFFFFFF)));   // CSR_TIMECMPLO
}

void E32WriteMemMappedCSR(uint32_t _hart, uint32_t _csr, uint32_t _value)
{
	uint32_t csrbase[] = {DEVICE_CSR0, DEVICE_CSR1};
	*(uint32_t*)(csrbase[_hart] + (_csr<<2)) = _value;
}

uint32_t E32ReadMemMappedCSR(uint32_t _hart, uint32_t _csr)
{
	uint32_t csrbase[] = {DEVICE_CSR0, DEVICE_CSR1};
	return *(uint32_t*)(csrbase[_hart] + (_csr<<2));
}

void __attribute__((aligned(16))) __attribute__((naked)) resetISR()
{
	asm volatile(
		"fence.i;"				// Clear I$
		"csrw 0xFEF, 0x0;"		// Clear cpu reset request
		"csrr s0, mscratch;"	// Grab address from scratch register
		"jalr s0;");			// Jump to reset vector
}

void __attribute__((aligned(64), noinline)) voidThread1()
{
	// Boot sequence for CPU#1
	asm volatile(
		"fence.i;"				// Invalidate I$
		"csrw mstatus,0;"		// Disable all interrupts (mstatus:mie=0)
		"li sp, 0x0FFDFEF0;"	// User CPU#1 stack
		"mv s0, sp;"			// Set frame pointer to current stack pointer
		"infloopcpu1:"
		"wfi;"					// Halt core
		"j infloopcpu1;" );
}

void __attribute__((aligned(64), noinline)) voidThread0()
{
	// Boot sequence for CPU#1
	asm volatile(
		"fence.i;"				// Invalidate I$
		"csrw mstatus,0;"		// Disable all interrupts (mstatus:mie=0)
		"li sp, 0x0FFDFFF0;"	// User CPU#0 stack
		"mv s0, sp;"			// Set frame pointer to current stack pointer
		"infloopcpu0:"
		"wfi;"					// Halt core
		"j infloopcpu0;" );
}

void E32SetupCPU(uint32_t hartid, void *workerThread)
{
	// Call with null workerthread pointer to use default thread on this core
	E32WriteMemMappedCSR(hartid, CSR_MSCRATCH, workerThread ? (uint32_t)workerThread : (hartid==0 ? (uint32_t)voidThread0 : (uint32_t)voidThread1));
	E32WriteMemMappedCSR(hartid, CSR_MTVEC, (uint32_t)resetISR);
	E32WriteMemMappedCSR(hartid, CSR_MIE, MIP_MEIP);
	E32WriteMemMappedCSR(hartid, CSR_MSTATUS, MSTATUS_MIE);
}

void E32ResetCPU(uint32_t hartid)
{
	E32WriteMemMappedCSR(hartid, 0xFEF, 0x1);
}
