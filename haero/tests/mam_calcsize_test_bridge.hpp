// This file is autogenerated. DO NOT EDIT DIRECTLY.
#ifndef HAERO_mam_calcsize_TEST_BRIDGE_HPP
#define HAERO_mam_calcsize_TEST_BRIDGE_HPP

#include "haero/haero.hpp"
#include "haero/model.hpp"

using namespace haero;

extern "C" {

// Fortran subroutines that implement this process.

extern Real run_bridge(const ModalAerosolConfig& modal_aerosol_config, Real t,
                       Real dt, const Prognostics& prognostics,
                       const Atmosphere& atmosphere,
                       const Diagnostics& diagnostics, Tendencies& tendencies);

}  // extern "C"

#endif
