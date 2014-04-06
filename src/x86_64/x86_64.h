#ifndef __X86_64_H_
#define __X86_64_H_

#include <cstdint>

namespace toy {
namespace x86_64 {

enum class CR4RegBit : uint32_t {
  kVME = 1 << 0,
  kPVI = 1 << 1,
  kTSD = 1 << 2,
  kDE = 1 << 3,
  kPSE = 1 << 4,
  kPAE = 1 << 5,
  kMCE = 1 << 6,
  kPGE = 1 << 7,
  kPCE = 1 << 8,
  kOSFXSR = 1 << 9,
  kOSXMMEXCPT = 1 << 10,
  kVMXE = 10 << 13,
  kSMXE = 10 << 14,
  kPCIDE = 1 << 17,
  kOSXSAVE = 1 << 18,
  kSMEP = 1 << 20,
  kSMAP = 1 << 21
};

}
}

#endif  // __X86_64_H_
