// Copyright (c) 2021, National Technology & Engineering Solutions of Sandia,
// LLC (NTESS). Copyright (c) 2022, Battelle Memorial Institute
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HAERO_ATMOSPHERE_HPP
#define HAERO_ATMOSPHERE_HPP

#include <haero/haero.hpp>

namespace haero {

/// @class Atmosphere
/// This type stores atmospheric state variables inherited from a host model.
class Atmosphere final {
public:
  /// Default constructor.
  /// CAUTION: only useful for creating placeholders in views!
  Atmosphere() = default;

  /// Creates an Atmosphere that stores a column of data with the given number
  /// of vertical levels and the given planetary boundary height.
  /// @param [in] num_levels the number of vertical levels per column stored by
  ///                        the state
  /// @param [in] pblh The column-specific planetary boundary height [m],
  ///                  computed by the host model
  Atmosphere(int num_levels, Real pblh);

  /// Creates an Atmosphere that stores unmanaged views of atmospheric column
  /// data owned and managed by the atmosphere host model.
  /// @param [in] num_levels the number of vertical levels per column stored by
  ///                        the state
  /// @param [in] temp A view of temperature column data [K] managed by the host
  ///                  model
  /// @param [in] press A view of total pressure column data [Pa] managed by the
  ///                   host model
  /// @param [in] qv A view of water vapor mass mixing ratio column data
  ///                [kg vapor/kg dry air] managed by the host model
  /// @param [in] ht A view of height column data [m] on level interfaces,
  ///                managed by the host model
  /// @param [in] pdel The hydrostatic "pressure thickness" defined as the
  ///                  difference in hydrostatic pressure levels at interfaces
  ///                  bounding each vertical level [Pa]
  /// @param [in] pblh The column-specific planetary boundary height [m],
  ///                  computed by the host model
  Atmosphere(int num_levels, const ColumnView temp, const ColumnView press,
             const ColumnView qv, const ColumnView ht, const ColumnView pdel,
             const ColumnView could_f, const ColumnView uv_ice_nuc, Real pblh);

  // Copy construction and assignment are supported for moving data between
  // host and device, and for populating multi-column views.
  Atmosphere(const Atmosphere &) = default;
  Atmosphere &operator=(const Atmosphere &) = default;

  /// Destructor.
  KOKKOS_FUNCTION
  ~Atmosphere() {}

  // Views.
  ColumnView temperature;
  ColumnView pressure;
  ColumnView vapor_mixing_ratio;
  ColumnView height;
  ColumnView hydrostatic_dp;
  ColumnView cloud_fraction;
  ColumnView updraft_vel_ice_nucleation;

  // Planetary boundary height.
  Real planetary_boundary_height;

  /// Returns the number of vertical levels per column in the system.
  KOKKOS_INLINE_FUNCTION
  int num_levels() const { return num_levels_; }

  /// Sets the planetary boundary height [m].
  KOKKOS_INLINE_FUNCTION
  void set_planetary_boundary_height(Real pblh) {
    planetary_boundary_height = pblh;
  }

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
              (vapor_mixing_ratio(k) < 0)) {
            violation = 1;
          }
        },
        violations);
    return (violations == 0);
  }

private:
  // Number of vertical levels.
  int num_levels_;
};

} // namespace haero

#endif
