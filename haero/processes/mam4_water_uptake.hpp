#ifndef HAERO_MAM4_WATER_UPTAKE_HPP
#define HAERO_MAM4_WATER_UPTAKE_HPP

#include "haero/process.hpp"

namespace haero {

/// @class Mam4WaterUptake
/// This process implements MAM4's treatment of water uptake, a diagnostic
/// aerosol process that updates the following diagnostic variables:
/// * `aero_water`, the per-mode aerosol water mixing ratio [kg/kg]
/// * `total_aero_water`, the aerosol water mixing ratio summed over modes [kg/kg]
/// * `mean_wet_diameter`, the per-mode geometric mean wet diameter [m]
/// * `wet_density`, the density of wet air [kg / m**3] <-- ???
class Mam4WaterUptake: public DiagnosticProcess {
  public:

  /// Creates a process that simulates water uptake.
  /// @param [in] use_bisection If true, solves the Kohler equation using
  ///                           the bisection method. Otherwise, solves the
  ///                           Kohler equation using an algebraic method.
  explicit Mam4WaterUptake(bool use_bisection = false);

  /// Destructor.
  ~Mam4WaterUptake();

  // Overrides
  void init(const Model& model) override;
  void update(const Model& model, Real t,
              const Prognostics& prognostics,
              Diagnostics& diagnostics) const override;

  private:

  bool use_bisection_;

  // Working arrays (passed to Fortran).
  Real *rh, *maer, *hygro, *naer, *dryvol, *drymass, *dryrad, *rhcrystal,
       *rhdeliques, *specdens_1;
};

}

#endif
