#include <cstdint>

#include "config.h"
#include "multiboot.h"
#include "x86_64.h"

using namespace toy::kernel;
using namespace toy::x86_64;

const multiboot_uint32_t kMultibootHeaderFlags = MULTIBOOT_MEMORY_INFO;

__attribute__((section(".mbh"), used))
const struct multiboot_header __multiboot_header = {
  MULTIBOOT_HEADER_MAGIC, kMultibootHeaderFlags,
  -(MULTIBOOT_HEADER_MAGIC + kMultibootHeaderFlags)
};

uint32_t __multiboot_info = 0;

__attribute__((aligned(16)))
uint8_t __bsp_boot_stack[kBspBootStackSize] = {};

namespace {

__attribute__((used))
void BStart32() {
  __asm__(R"!!!(

.text

.global __bstart32
__bstart32:

  movl %0, %%esp
  movl %%ebx, __multiboot_info
  movb $0xFF, %%al
  outb %%al, $0xA1
  outb %%al, $0x21 # disable IRQs
  movl %%cr4, %%edx # enable SSE
  orl %1, %%edx
  movl %%edx, %%cr4
  call __boot32

.global __halt
__halt:

  hlt
  jmp __halt

  )!!!" : : "i"(__bsp_boot_stack+kBspBootStackSize), "i"(CR4RegBit::kOSFXSR));
}

}

extern "C" void __boot32() {
  *(char*)0xB8000 = '!';

}
