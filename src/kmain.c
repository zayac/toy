#include "acpi.h"
#include "addr_map.h"
#include "apic.h"
#include "cpu_info.h"
#include "desc_table.h"
#include "interrupt.h"
#include "page_map.h"
#include "schedule.h"
#include "vga.h"

void kmain(void) {
  init_vga();
  init_desc_tables();
  init_interrupts();
  init_page_map(ADDR_PAGE_MAP, 256 * (1L << 30));
  init_apic();
  init_acpi();
  init_cpu_info();
  init_scheduler();
}