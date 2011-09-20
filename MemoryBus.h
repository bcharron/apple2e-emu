#include <vector>
#include <stdint.h>
#include "MemoryRegion.h"

typedef struct soft_switches_struct {
	bool altCharset;
	bool text;
	bool mixed;
	bool page2;
	bool hires;
	bool text80Col;
	bool text80Store;
	bool vbl;
} soft_switches_t;

/*
 * The memory bus is a linked list of all regions in the system: RAM, ROMs, screen memory, etc.
 */
class MemoryBus
{
public:
	MemoryBus(unsigned int size);
	void addRegion(MemoryRegion *region);
	uint8_t read(uint16_t offset);
	void write(uint16_t offset, uint8_t byte);
	unsigned int getSize(void);
	uint8_t readSoftSwitch(uint16_t offset);
	void writeSoftSwitch(uint16_t offset, uint8_t val);

protected:
	MemoryRegion* findRegion(uint16_t offset);
	std::vector<MemoryRegion*> regions;
	MemoryRegion *ram;
	unsigned int memorySize;
	soft_switches_t softSwitches;
};
