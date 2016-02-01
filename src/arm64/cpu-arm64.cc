// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// CPU specific code for arm independent of OS goes here.

#if V8_TARGET_ARCH_ARM64

#include "src/arm64/utils-arm64.h"
#include "src/assembler.h"

namespace v8 {
namespace internal {

void CpuFeatures::FlushICache(void* address, size_t length) {
#ifdef V8_HOST_ARCH_ARM64
  // The code below assumes user space cache operations are allowed. The goal
  // of this routine is to make sure the code generated is visible to the I
  // side of the CPU.

  uintptr_t start = reinterpret_cast<uintptr_t>(address);
  // Sizes will be used to generate a mask big enough to cover a pointer.
  uintptr_t dsize = CpuFeatures::dcache_line_size();
  uintptr_t isize = CpuFeatures::icache_line_size();
  // Cache line sizes are always a power of 2.
  DCHECK(CountSetBits(dsize, 64) == 1);
  DCHECK(CountSetBits(isize, 64) == 1);
  uintptr_t dstart = start & ~(dsize - 1);
  uintptr_t istart = start & ~(isize - 1);
  uintptr_t end = start + length;

  __asm__ __volatile__ (  // NOLINT
    // Clean every line of the D cache containing the target data.
    "0:                                \n\t"
    // dc      : Data Cache maintenance
    //    c    : Clean
    //     va  : by (Virtual) Address
    //       u : to the point of Unification
    // The point of unification for a processor is the point by which the
    // instruction and data caches are guaranteed to see the same copy of a
    // memory location. See ARM DDI 0406B page B2-12 for more information.
    "dc   cvau, %[dline]                \n\t"
    "add  %[dline], %[dline], %[dsize]  \n\t"
    "cmp  %[dline], %[end]              \n\t"
    "b.lt 0b                            \n\t"
    // Barrier to make sure the effect of the code above is visible to the rest
    // of the world.
    // dsb    : Data Synchronisation Barrier
    //    ish : Inner SHareable domain
    // The point of unification for an Inner Shareable shareability domain is
    // the point by which the instruction and data caches of all the processors
    // in that Inner Shareable shareability domain are guaranteed to see the
    // same copy of a memory location.  See ARM DDI 0406B page B2-12 for more
    // information.
    "dsb  ish                           \n\t"
    // Invalidate every line of the I cache containing the target data.
    "1:                                 \n\t"
    // ic      : instruction cache maintenance
    //    i    : invalidate
    //     va  : by address
    //       u : to the point of unification
    "ic   ivau, %[iline]                \n\t"
    "add  %[iline], %[iline], %[isize]  \n\t"
    "cmp  %[iline], %[end]              \n\t"
    "b.lt 1b                            \n\t"
    // Barrier to make sure the effect of the code above is visible to the rest
    // of the world.
    "dsb  ish                           \n\t"
    // Barrier to ensure any prefetching which happened before this code is
    // discarded.
    // isb : Instruction Synchronisation Barrier
    "isb                                \n\t"
    : [dline] "+r" (dstart),
      [iline] "+r" (istart)
    : [dsize] "r"  (dsize),
      [isize] "r"  (isize),
      [end]   "r"  (end)
    // This code does not write to memory but without the dependency gcc might
    // move this code before the code is generated.
    : "cc", "memory"
  );  // NOLINT
#endif  // V8_HOST_ARCH_ARM64
}

}  // namespace internal
}  // namespace v8

#endif  // V8_TARGET_ARCH_ARM64
