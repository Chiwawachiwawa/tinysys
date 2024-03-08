#ifdef CAT_WINDOWS
#include "SDL.h"
#else
#include <SDL2/SDL.h>
#endif
#include <stdio.h>
#include "emulator.h"

bool CEmulator::Reset(const char* romFile)
{
    m_clock.Reset();
	m_cpu.SetMem(&m_mem);

    if (m_rombin)
        delete []m_rombin;

    if (romFile)
    {
        FILE *fp = fopen(romFile, "rb");
        if (!fp)
            return false;
        fseek(fp, 0, SEEK_END);
        fpos_t fpos;
        fgetpos(fp, &fpos);
        size_t filesize = (size_t)fpos;
        fseek(fp, 0, SEEK_SET);

        m_rombin = new uint8_t[filesize];
        m_romsize = (uint32_t)filesize;
        fread(m_rombin, 1, filesize, fp);
        fclose(fp);
    }

    m_mem.CopyROM(m_cpu.m_resetvector, m_rombin, m_romsize);

    return true;
}

bool CEmulator::Step()
{
    m_clock.Step();

    // Wire up the clocks to each device
    m_mem.Tick(m_clock);
	m_cpu.Tick(m_clock);

    return false;
}
