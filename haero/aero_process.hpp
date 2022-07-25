#ifndef HAERO_AERO_PROCESS_HPP
#define HAERO_AERO_PROCESS_HPP

#include "haero/atmosphere.hpp"

#include <memory>
#include <type_traits>

namespace haero {

/// @class AeroProcess
/// This type defines the interface for a specific process in the aerosol
/// lifecycle, backed by a specific implementation, the structure of which is
/// defined by a specific "aerosol configuration".
template <typename AerosolConfig, typename AerosolProcessImpl>
class AeroProcess final {
 public:

  // Types derived from template parameters.
  using AeroConfig    = AerosolConfig;
  using Prognostics   = typename AerosolConfig::Prognostics;
  using Diagnostics   = typename AerosolConfig::Diagnostics;
  using Tendencies    = typename AerosolConfig::Tendencies;
  using Impl          = AerosolProcessImpl;
  using ProcessConfig = typename Impl::Config;

  // Tendencies type must be the same as that for Prognostics.
  static_assert(std::is_same<Tendencies, Prognostics>::value,
    "Tendencies and Prognostics types must be identical!");

  /// Constructs an instance of an aerosol process with the given name,
  /// associated with the given aerosol configuration.
  /// @param [in] aero_config The aerosol configuration for this process
  /// @param [in] process_config Any process-specific information required by
  ///                            this process's implementation.
  explicit AeroProcess(const AeroConfig& aero_config,
                       const ProcessConfig& process_config = ProcessConfig())
      : name_(), aero_config_(aero_config), process_config_(process_config),
        impl_() {
    // Set the name of this process.
    name_ = impl_.name();
    // Pass the configuration data to the implementation to initialize it.
    impl_.init(aero_config_, process_config_);
  }

  /// Destructor.
  KOKKOS_INLINE_FUNCTION ~AeroProcess() {}

  // Copy construction is required for host -> device dispatches
  KOKKOS_INLINE_FUNCTION
  AeroProcess(const AeroProcess&) = default;

  /// Default constructor is disabled.
  AeroProcess() = delete;

  // Deep copies are not allowed.
  AeroProcess& operator=(const AeroProcess&) = delete;

  //------------------------------------------------------------------------
  //                          Accessors (host only)
  //------------------------------------------------------------------------

  /// On host: returns the name of this process.
  std::string name() const { return name_; }

  /// On host: returns the aerosol configuration (metadata) associated with
  /// this process.
  const AeroConfig& aero_config() const {
    return aero_config_;
  }

  /// On host: returns any process-specific configuration data.
  const ProcessConfig& process_config() const {
    return process_config_;
  }

  //------------------------------------------------------------------------
  //                            Public Interface
  //------------------------------------------------------------------------

  /// On host or device: Validates input aerosol and atmosphere data, returning
  /// true if all data is physically consistent (whatever that means), and false
  /// if not.
  /// @param [in] team The Kokkos team used to run this process in a parallel
  ///                  dispatch.
  /// @param [in] atmosphere Atmosphere state variables with which to validate.
  /// @param [in] prognostics A collection of aerosol prognostic variables to be
  ///                         validated.
  KOKKOS_INLINE_FUNCTION
  bool validate(const TeamType& team,
                const Atmosphere& atmosphere,
                const Prognostics& prognostics) const {
    return impl_.validate(aero_config_, team, atmosphere, prognostics);
  }

  /// On host or device: runs the aerosol process at a given time with the given
  /// data.
  /// @param [in]    team The Kokkos team used to run this process in a parallel
  ///                     dispatch.
  /// @param [in]    t The simulation time at which this process is being
  ///                  invoked (in seconds).
  /// @param [in]    dt The simulation time interval ("timestep size") over
  ///                   which this process occurs.
  /// @param [in]    atmosphere The atmosphere state variables used by this
  ///                           process.
  /// @param [in]    prognostics An array containing aerosol tracer data to be
  ///                            evolved.
  /// @param [inout] diagnostics An array that can store aerosol diagnostic
  ///                            data computed or updated by this process.
  /// @param [out]   tendencies An array analogous to prognostics that
  ///                           stores computed tendencies.
  KOKKOS_INLINE_FUNCTION
  void compute_tendencies(const TeamType& team, Real t, Real dt,
                          const Atmosphere& atmosphere,
                          const Prognostics& prognostics,
                          const Diagnostics& diagnostics,
                          const Tendencies& tendencies) const {
    impl_.compute_tendencies(aero_config_, team, t, dt, atmosphere, prognostics,
                             diagnostics, tendencies);
  }

 private:
  std::string   name_;
  AeroConfig    aero_config_;
  ProcessConfig process_config_;
  Impl          impl_;
};

}  // namespace haero

#endif
