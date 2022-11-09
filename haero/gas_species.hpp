#ifndef HAERO_GAS_SPECIES_HPP
#define HAERO_GAS_SPECIES_HPP

#include <haero/haero.hpp>

#include <limits>
#include <map>
#include <string>
#include <vector>

namespace haero {

/// @struct GasSpecies
/// This type represents a gas that participates in one or more aerosol
/// microphysics parameterizations.
struct GasSpecies final {
  /// Molecular weight [kg/mol]
  Real molecular_weight;
};

}  // namespace haero
#endif
