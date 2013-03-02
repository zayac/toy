#include "acpi.h"
#include "cpu_info.h"

#define CPUS_MAX 32
#define VENDOR_LEN 12

static int vendor, num, bsp;
static struct cpu_desc info[CPUS_MAX];

int get_cpu_vendor(void) {
  if (vendor)
    return vendor;
  else {
    uint32_t buf[4] = { };
    asm("xorl %%eax, %%eax\ncpuid" :
        "=b"(buf[0]), "=d"(buf[1]), "=c"(buf[2]) : : "eax");
    if (!memcmp(buf, "GenuineIntel", VENDOR_LEN))
      vendor = CPU_VENDOR_INTEL;
    else if (!memcmp(buf, "AuthenticAMD", VENDOR_LEN))
      vendor = CPU_VENDOR_AMD;
    else
      vendor = CPU_VENDOR_UNKNOWN;
    LOG_DEBUG("vendor %d (%s)", vendor, buf);
    return vendor;
  }
}

int get_cpus(void) {
  return num;
}

int get_cpu_bsp(void) {
  return bsp;
}

const struct cpu_desc *get_cpu_info(void) {
  return info;
}

static bool get_chip_multithreading(int *chip_threads_max) {
  uint32_t ebx, edx;
  asm("movl $1, %%eax\ncpuid" : "=b"(ebx), "=d"(edx) : : "eax", "ecx");
  bool support = !!(edx & (1 << 28));
  *chip_threads_max = INT_BITS(ebx, 16, 23);
  LOG_DEBUG("support: %s, chip_threads_max: %d", bool_str(support),
            *chip_threads_max);
  return support;
}

static void get_thread_core_bits_intel(uint32_t cpuid_func_max,
                                       uint32_t cpuid_ex_func_max,
                                       int chip_threads_max, int *thread_bits,
                                       int *core_bits) {
  uint32_t eax;
  if (cpuid_func_max >= 0xB) {
    asm("movl $0xB, %%eax\nxorl %%ecx, %%ecx\ncpuid" :
        "=a"(eax) : : "ebx", "ecx", "edx");
    *thread_bits = INT_BITS(eax, 0, 4);
    asm("movl $0xB, %%eax\nmovl $1, %%ecx\ncpuid" :
        "=a"(eax) : : "ebx", "ecx", "edx");
    *core_bits = INT_BITS(eax, 0, 4) - *thread_bits;
  }
  else if (cpuid_func_max >= 0x4) {
    asm("movl $0x4, %%eax\nxorl %%ecx, %%ecx\ncpuid" :
        "=a"(eax) : : "ebx", "ecx", "edx");
    int core_index_max = INT_BITS(eax, 26, 31);
    *core_bits = core_index_max ? bsr(core_index_max) + 1 : 0;
    int chip_thread_bits = bsr(chip_threads_max - 1) + 1;
    *thread_bits = chip_thread_bits - *core_bits;
  }
  else
    *core_bits = 0, *thread_bits = bsr(chip_threads_max - 1) + 1;
}

static void get_thread_core_bits_amd(uint32_t cpuid_func_max,
                                     uint32_t cpuid_ex_func_max,
                                     int chip_threads_max, int *thread_bits,
                                     int *core_bits) {
  if (cpuid_ex_func_max >= 0x80000008) {
    uint32_t ecx;
    asm("movl $0x80000008, %%eax\ncpuid" : "=c"(ecx) : : "eax", "ebx", "edx");
    *core_bits = INT_BITS(ecx, 12, 15);
    if (!*core_bits) {
      int core_index_max = INT_BITS(ecx, 0, 7);
      *core_bits = core_index_max ? bsr(core_index_max) + 1 : 0;
    }
    int chip_thread_bits = bsr(chip_threads_max - 1) + 1;
    *thread_bits = chip_thread_bits - *core_bits;
  }
  else
    *core_bits = bsr(chip_threads_max - 1) + 1, *thread_bits = 0;
}

static void get_thread_core_bits(int *thread_bits, int *core_bits) {
  uint32_t cpuid_func_max, cpuid_ex_func_max;
  asm("xorl %%eax, %%eax\ncpuid" :
      "=a"(cpuid_func_max) : : "ebx", "ecx", "edx");
  asm("movl $0x80000000, %%eax\ncpuid" :
      "=a"(cpuid_ex_func_max) : : "ebx", "ecx", "edx");

  int chip_threads_max;
  if (get_cpu_vendor() != CPU_VENDOR_UNKNOWN &&
      get_chip_multithreading(&chip_threads_max)) {
    if (get_cpu_vendor() == CPU_VENDOR_INTEL)
      get_thread_core_bits_intel(cpuid_func_max, cpuid_ex_func_max,
                                 chip_threads_max, thread_bits, core_bits);
    else
      get_thread_core_bits_amd(cpuid_func_max, cpuid_ex_func_max,
                               chip_threads_max, thread_bits, core_bits);
    LOG_DEBUG("thread_bits: %d, core_bits: %d", *thread_bits, *core_bits);
  }
  else
    *thread_bits = *core_bits = 0;
}

void fill_cpu_info(int thread_bits, int core_bits) {
  int i = 0;
  struct acpi_madt_lapic *mentry = NULL;
  while (get_next_acpi_entry(get_acpi_madt(), &mentry, ACPI_MADT_LAPIC_TYPE))
    if (mentry->enabled) {
      info[i].apic_id = mentry->apic_id;
      info[i].thread = mentry->apic_id & ((1 << thread_bits) - 1);
      info[i].core = (mentry->apic_id >> thread_bits) & ((1 << core_bits) - 1);
      info[i].chip = mentry->apic_id >> (thread_bits + core_bits);

      struct acpi_srat_lapic *sentry = NULL;
      while (get_next_acpi_entry(get_acpi_srat(), &sentry,
                                 ACPI_SRAT_LAPIC_TYPE))
        if (sentry->enabled && sentry->apic_id == mentry->apic_id)
          info[i].domain = sentry->prox_domain0 +
            ((uint32_t)sentry->prox_domain1 << 8) +
            ((uint32_t)sentry->prox_domain2 << 24);

      LOG_DEBUG("CPU: apic_id: %X, thread: %d, core: %d, chip: %d, domain: %X",
                info[i].apic_id, info[i].thread, info[i].core, info[i].chip,
                info[i].domain);
      i++;
    }

  num = i;
}

void init_cpu_info(void) {
  int thread_bits, core_bits;
  get_thread_core_bits(&thread_bits, &core_bits);
  fill_cpu_info(thread_bits, core_bits);
  LOG_DEBUG("done");
}