// Copyright (c) 2021, National Technology & Engineering Solutions of Sandia,
// LLC (NTESS). Copyright (c) 2022, Battelle Memorial Institute
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HAERO_ATMOSPHERE_HPP
#define HAERO_ATMOSPHERE_HPP

#include <haero/haero.hpp>

#include <ekat/ekat_assert.hpp>

namespace haero {

/// @class Atmosphere
/// This type stores atmospheric state variables inherited from a host model.
class Atmosphere final {
  // number of vertical levels
  int num_levels_;

public:
  /// Constructs an Atmosphere object holding the state for a single atmospheric
  /// column with the given planetary boundary layer height.
  /// All views must be set manually elsewhere or provided by a host model.
  Atmosphere(const ColumnView T, const ColumnView p, const ColumnView qv,
    const ColumnView qc, const ColumnView nqc, const ColumnView qi,
    const ColumnView nqi, const ColumnView z, const ColumnView hdp,
    const ColumnView cf, const ColumnView w, const Real pblh) :
      num_levels_(T.extent(0)),
      temperature(T),
      pressure(p),
      vapor_mixing_ratio(qv),
      liquid_mixing_ratio(qc),
      cloud_liquid_number_mixing_ratio(nqc),
      ice_mixing_ratio(qi),
      cloud_ice_number_mixing_ratio(nqi),
      height(z),
      hydrostatic_dp(hdp),
      cloud_fraction(cf),
      updraft_vel_ice_nucleation(w),
      planetary_boundary_layer_height(pblh) {

        EKAT_ASSERT(T.extent(0) > 0);

        EKAT_ASSERT(
          T.extent(0) == p.extent(0) &&
          T.extent(0) == qv.extent(0) &&
          T.extent(0) == qc.extent(0) &&
          T.extent(0) == nqc.extent(0) &&
          T.extent(0) == qi.extent(0) &&
          T.extent(0) == nqi.extent(0) &&
          T.extent(0) == z.extent(0) &&
          T.extent(0) == hdp.extent(0) &&
          T.extent(0) == cf.extent(0) &&
          T.extent(0) == w.extent(0));

        EKAT_ASSERT(pblh >= 0.0);
      }

  Atmosphere() = delete;

  // these are supported for initializing containers of Atmospheres
  Atmosphere(const Atmosphere &rhs) = default;
  Atmosphere &operator=(const Atmosphere &rhs) = default;

  /// destructor, valid on both host and device
  KOKKOS_FUNCTION
  ~Atmosphere() {}

  // views storing atmospheric state data for a single vertical column

  /// temperature [K]
  ConstColumnView temperature;

  /// pressure [Pa]
  ConstColumnView pressure;

  /// water vapor mass mixing ratio [kg vapor/kg dry air]
  ConstColumnView vapor_mixing_ratio;

  /// liquid water mass mixing ratio [kg vapor/kg dry air]
  ConstColumnView liquid_mixing_ratio;

  /// grid box averaged cloud liquid number mixing ratio [#/kg dry air]
  ConstColumnView cloud_liquid_number_mixing_ratio;

  /// ice water mass mixing ratio [kg vapor/kg dry air]
  ConstColumnView ice_mixing_ratio;

  // grid box averaged cloud ice number mixing ratio [#/kg dry air]
  ConstColumnView cloud_ice_number_mixing_ratio;

  /// height at the midpoint of each vertical level [m]
  ConstColumnView height;

  /// hydroѕtatic "pressure thickness" defined as the difference in hydrostatic
  /// pressure levels between the interfaces bounding a vertical level [Pa]
  ConstColumnView hydrostatic_dp;

  /// cloud fraction [-]
  ConstColumnView cloud_fraction;

  /// vertical updraft velocity used for ice nucleation [m/s]
  ConstColumnView updraft_vel_ice_nucleation;

  // column-specific planetary boundary layer height [m]
  Real planetary_boundary_layer_height;

  /// returns the number of vertical levels per column in the system
  KOKKOS_INLINE_FUNCTION
  int num_levels() const { return num_levels_; }

  /// Returns true iff all atmospheric quantities are nonnegative, using the
  /// given thread team to parallelize the check.
  KOKKOS_INLINE_FUNCTION
  bool quantities_nonnegative(const ThreadTeam &team) const {
    const int nk = num_levels();
    int violations = 0;
    Kokkos::parallel_reduce(
        Kokkos::TeamThreadRange(team, nk),
        KOKKOS_CLASS_LAMBDA(int k, int &violation) {
          if ((temperature(k) < 0) || (pressure(k) < 0) ||
              (vapor_mixing_ratio(k) < 0) || (liquid_mixing_ratio(k) < 0) ||
              (ice_mixing_ratio(k) < 0) ||
              (cloud_liquid_number_mixing_ratio(k) < 0) ||
              (cloud_ice_number_mixing_ratio(k) < 0)) {
            violation = 1;
          }
        },
        violations);
    return (violations == 0);
  }
};

} // namespace haero

#endif
