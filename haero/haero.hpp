#ifndef HAERO_HPP
#define HAERO_HPP

#include "haero/haero_config.hpp"

namespace haero {

/// MemorySpace refers to the memory space on the device.
#ifdef KOKKOS_ENABLE_CUDA
typedef Kokkos::CudaSpace MemorySpace;
#else
typedef Kokkos::HostSpace MemorySpace;
#endif

/// Helpers for pack/array indexing
using PackInfo = ekat::PackInfo<HAERO_PACK_SIZE>;

// Returns haero's version string.
const char* version();

// Returns haero's git revision hash, or "unknown" if not found.
const char* revision();

// Returns true iff this build has changes that weren't committed to the git
// repo.
bool has_uncommitted_changes();

}  // end namespace haero

#endif
