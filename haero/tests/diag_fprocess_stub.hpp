#ifndef HAERO_DIAG_FPROCESS_STUB_HPP
#define HAERO_DIAG_FPROCESS_STUB_HPP

#include "haero/process.hpp"

namespace haero {

// This diagnostic process implements a simple model of partial pressures in
// aerosol modes and gases using the ideal gas law. It's not intended to be a
// realistic description of a physical process--it's meant to illustrate how
// Fortran-implemented diagnostic processes work.
class DiagFProcessStub : public FDiagnosticProcess {
 public:
  // Constructor (no parameters needed).
  DiagFProcessStub();
};

}  // namespace haero

#endif
