---
layout: post
title:  "C++11 raw strings and inline assembly"
---
I wrote basic Waf scripts for building a kernel skeleton, ISO-image creation and running QEMU emulator. Waf is pretty convenient and intuitive. The only problem I coped with trying to link my kernel was the following error: *Node X is created more than once*. Thus I didn't succeed to use *cprogram* or *cxxprogram* for this task, so I used a generic task generator with a given rule. Will not explain this stuff with a greater detail - it's not noteworthy.

Let me show you how did I employ C++11 raw string literals while writing inline assembly:

{% highlight c++ %}

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
{% endhighlight %}

As you can see, the raw strings support enables me to write assembly just as I would wrote it in dedicated s-file: no semicolons, no extra quotes. Very useful.

**Note:** to use immediate operands in the inline assembly I wrap the code by a dummy function called *BStart32*. It's not supposed to be called and it's not visible outside the module (anonymous namespace).