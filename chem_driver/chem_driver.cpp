#include "chem_driver.hpp"

namespace haero {
namespace chem_driver {

namespace defaults {
static constexpr Real tbeg = 0.0;
static constexpr Real tend = 1.0;
static constexpr Real dt = 1.0e-8;
static constexpr Real dtmin = 1.0e-8;
static constexpr Real dtmax = 1.0e-1;
static constexpr int max_num_newton_iterations = 100;
static constexpr int num_time_iterations_per_interval = 10;
static constexpr int jacobian_interval = 1;

static constexpr Real max_time_iterations = 1.0e3;
static constexpr Real atol_newton = 1.0e-10;
static constexpr Real rtol_newton = 1.0e-6;
static constexpr Real atol_time = 1.0e-12;
static constexpr Real tol_time = 1.0e-4;

static constexpr int nbatch = 1;
static constexpr bool verbose = false;
static constexpr int team_size = -1;
static constexpr int vector_size = -1;
static const std::string outputfile = "chem.dat";
}  // namespace defaults

// anonymous namespace to hold this YamlException class
namespace {
// This exception class stores information about errors encountered in reading
// data from a YAML file.
class YamlException : public std::exception {
 public:
  // Constructs an exception containing the given descriptive message.
  YamlException(const std::string& message) : _message(message) {}

  // Constructs an exception containing the given formatting string and
  // C-style variadic arguments (a la printf).
  YamlException(const char* fmt, ...);

  const char* what() const throw() { return _message.c_str(); }

 private:
  std::string _message;
};
YamlException::YamlException(const char* fmt, ...) {
  char ss[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(ss, 255, fmt, args);
  va_end(args);
  _message.assign(ss);
}
};  // end anonymous namespace

// write current state to file
// the fields written to file are: iteration, t, dt, Density, Pressure,
// Temperature, "concentration(s)" (in the relevant, specified units)
void static write_state(const ordinal_type iter, const Real_1d_view_host _t,
                        const Real_1d_view_host _dt,
                        const Real_2d_view_host _state_at_i, FILE* fout_) {
  // loop over batches
  for (size_t sp = 0; sp < _state_at_i.extent(0); sp++) {
    fmt::print(fout_, "{:d} \t {:15.10e} \t  {:15.10e} \t ", iter, _t(sp),
               _dt(sp));
    // loop over species concs
    for (ordinal_type k = 0, kend = _state_at_i.extent(1); k < kend; ++k)
      fmt::print(fout_, "{:15.10e} \t", _state_at_i(sp, k));
    fmt::print(fout_, "\n");
  }
};

// print current state to screen
// the fields printed to screen are: current time, elapsed time, Density,
// Pressure, Temperature, "concentration(s)" (in the relevant, specified units)
void static print_state(const time_advance_type _tadv, const Real _t,
                        const Real_1d_view_host _state_at_i) {
  fmt::print(stdout, "{:e} {:e} {:e} {:e} {:e}", _t, _t - _tadv._tbeg,
             _state_at_i(0), _state_at_i(1), _state_at_i(2));
  // loop over species concs
  for (ordinal_type k = 3, kend = _state_at_i.extent(0); k < kend; ++k)
    fmt::print(stdout, " {:e}", _state_at_i(k));
  fmt::print(stdout, "\n");
};

// ChemSolver constructor: initializes all the required views on device and
// sets some kokkos-related parameters
ChemSolver::ChemSolver(std::string input_file) {
  // parse the tchem section of the input yaml
  parse_tchem_inputs_(input_file);
  // read the parameters from file
  solver_params_.set_params(input_file, verbose_);

  // make sure that the execution spaces are the same
  static_assert(std::is_same<TChem::exec_space(), ExecutionSpace()>::value,
                "TChem and Haero are using different execution spaces");

  TChem::exec_space::print_configuration(std::cout, verbose_);
  TChem::host_exec_space::print_configuration(std::cout, verbose_);
  using device_type = typename Tines::UseThisDevice<ExecutionSpace>::type;

  // construct the kinetic model data object and its const version
  kmd_ = TChem::KineticModelData(input_file);
  kmcd_ = TChem::createNCAR_KineticModelConstData<device_type>(kmd_);

  // this is the number of species and second dimension in the 2d state view
  const ordinal_type stateVecDim = TChem::Impl::getStateVectorSize(kmcd_.nSpec);

  if (verbose_) {
    fmt::print(stdout, "Number of Species {:d} \n", kmcd_.nSpec);
    fmt::print(stdout, "Number of Reactions {:d} \n", kmcd_.nReac);
  }
  const auto speciesNamesHost = Kokkos::create_mirror_view(kmcd_.speciesNames);
  Kokkos::deep_copy(speciesNamesHost, kmcd_.speciesNames);

  fout_ = fopen(solver_params_.outputfile.c_str(), "w");
  // read initial condition from yaml file
  TChem::AtmChemistry::setScenarioConditions(input_file, speciesNamesHost,
                                             kmcd_.nSpec, state_host_, nbatch_);
  state_ = Real_2d_view("StateVector Devices", nbatch_, stateVecDim);

  Kokkos::deep_copy(state_, state_host_);

  const auto exec_space_instance = TChem::exec_space();

  // kokkos team policy
  policy_ = policy_type(exec_space_instance, nbatch_, Kokkos::AUTO());

  // fancier version with nonzero vector size
  if (team_size_ > 0 && vector_size_ > 0) {
    policy_ =
        policy_type(exec_space_instance, nbatch_, team_size_, vector_size_);
  }

  // set scratch memory size for kokkos teams
  const ordinal_type level = 1;
  ordinal_type per_team_extent(0);
  per_team_extent = TChem::AtmosphericChemistry::getWorkSpaceSize(kmcd_);
  const ordinal_type per_team_scratch =
      TChem::Scratch<Real_1d_view>::shmem_size(per_team_extent);
  policy_.set_scratch_size(level, Kokkos::PerTeam(per_team_scratch));
}

// time integrator that takes tbeg and tend as arguments--used in the unit tests
// in order to time step externally
// Note: this and the following version of this function are nearly identical,
// and only commented here
void ChemSolver::time_integrate(const Real& tbeg, const Real& tend) {
  Real_1d_view t("time", nbatch_);
  Kokkos::deep_copy(t, tbeg);
  Real_1d_view dt("delta time", nbatch_);
  Kokkos::deep_copy(dt, solver_params_.tadv_default._dtmin);

  Real_1d_view_host t_host;
  Real_1d_view_host dt_host;

  t_host = Real_1d_view_host("time host", nbatch_);
  dt_host = Real_1d_view_host("dt host", nbatch_);

  ordinal_type number_of_equations = 0;

  using device_type = typename Tines::UseThisDevice<ExecutionSpace>::type;
  using problem_type =
      TChem::Impl::AtmosphericChemistry_Problem<Real, device_type>;
  number_of_equations = problem_type::getNumberOfTimeODEs(kmcd_);

  Real_2d_view tol_time("tol time", number_of_equations, 2);
  Real_1d_view tol_newton("tol newton", 2);

  Real_2d_view fac("fac", nbatch_, number_of_equations);

  // copy tolerances to device
  {
    auto tol_time_host = Kokkos::create_mirror_view(tol_time);
    auto tol_newton_host = Kokkos::create_mirror_view(tol_newton);

    for (ordinal_type i = 0, iend = tol_time.extent(0); i < iend; ++i) {
      tol_time_host(i, 0) = solver_params_.atol_time;
      tol_time_host(i, 1) = solver_params_.tol_time;
    }
    tol_newton_host(0) = solver_params_.atol_newton;
    tol_newton_host(1) = solver_params_.rtol_newton;

    Kokkos::deep_copy(tol_time, tol_time_host);
    Kokkos::deep_copy(tol_newton, tol_newton_host);
  }

  // set tbeg and tend in tadv_default from the function arguments, rather than
  // the ones read from file, then deep copy to device
  solver_params_.tadv_default._tbeg = tbeg;
  solver_params_.tadv_default._tend = tend;
  time_advance_type_1d_view tadv("tadv", nbatch_);
  Kokkos::deep_copy(tadv, solver_params_.tadv_default);

  // setup current timestep subviews and mirrors
  const auto tadv_at_i = Kokkos::subview(tadv, 0);
  const auto t_at_i = Kokkos::subview(t, 0);
  const auto state_at_i = Kokkos::subview(state_, 0, Kokkos::ALL());

  auto tadv_at_i_host = Kokkos::create_mirror_view(tadv_at_i);
  auto t_at_i_host = Kokkos::create_mirror_view(t_at_i);
  auto state_at_i_host = Kokkos::create_mirror_view(state_at_i);

  // print initial state info to screen, if enabled
  if (print_qoi_) {
    Kokkos::deep_copy(tadv_at_i_host, tadv_at_i);
    Kokkos::deep_copy(t_at_i_host, t_at_i);
    Kokkos::deep_copy(state_at_i_host, state_at_i);
    print_state(tadv_at_i_host(), t_at_i_host(), state_at_i_host);
  }

  Kokkos::deep_copy(dt_host, dt);
  Kokkos::deep_copy(t_host, t);

  // write the initial state information, along with header, to file
  fmt::print(fout_, "{:s} \t {:s} \t {:s} \t ", "iter", "t", "dt");
  fmt::print(fout_, "{:s} \t {:s} \t {:s} \t", "Density[kg/m3]",
             "Pressure[Pascal]", "Temperature[K]");
  const auto speciesNamesHost = Kokkos::create_mirror_view(kmcd_.speciesNames);
  Kokkos::deep_copy(speciesNamesHost, kmcd_.speciesNames);
  for (ordinal_type k = 0; k < kmcd_.nSpec; k++) {
    fmt::print(fout_, "{:s} \t", &speciesNamesHost(k, 0));
  }
  fmt::print(fout_, "\n");
  // write the initial condition to file (index = -1 => pre-time-stepping)
  static constexpr int init_condition_iteration = -1;
  write_state(init_condition_iteration, t_host, dt_host, state_host_, fout_);

  Real tsum = 0;
  ordinal_type iter = 0;
  // begin time stepping
  // note that this stops according to the tend passed to the function
  for (; iter < solver_params_.max_time_iterations && tsum <= tend * 0.9999;
       ++iter) {
    // this is where the magic happens
    TChem::AtmosphericChemistry::runDeviceBatch(
        policy_, tol_newton, tol_time, fac, tadv, state_, t, dt, state_, kmcd_);

    // print current state info to screen, if enabled
    if (print_qoi_) {
      Kokkos::deep_copy(tadv_at_i_host, tadv_at_i);
      Kokkos::deep_copy(t_at_i_host, t_at_i);
      Kokkos::deep_copy(state_at_i_host, state_at_i);
      print_state(tadv_at_i_host(), t_at_i_host(), state_at_i_host);
    }

    Kokkos::deep_copy(dt_host, dt);
    Kokkos::deep_copy(t_host, t);
    Kokkos::deep_copy(state_host_, state_);

    // write current state info to file
    write_state(iter, t_host, dt_host, state_host_, fout_);

    // carry over time and dt computed in this step
    Real tsum = 0;
    Kokkos::parallel_reduce(
        nbatch_,
        KOKKOS_LAMBDA(const ordinal_type& i, Real& update) {
          tadv(i)._tbeg = t(i);
          tadv(i)._dt = dt(i);
          update += t(i);
        },
        tsum);
    Kokkos::fence();
    tsum /= nbatch_;
  }  // end for

  if (print_qoi_) {
    Kokkos::deep_copy(state_host_, state_);
    for (int i = 0; i < nbatch_; ++i) {
      fmt::print(stdout, "Devices:: Solution sample No {:d}\n", i);
      auto state_at_i = Kokkos::subview(state_host_, i, Kokkos::ALL());
      for (int k = 0; k < state_at_i.extent(0); ++k) {
        fmt::print(stdout, " {:e}", state_at_i(k));
      }
      fmt::print(stdout, "\n");
    }
  }
}

// time integrator that gets tbeg and tend from yaml input (stored in
// solver_params_)
void ChemSolver::time_integrate() {
  Real_1d_view t("time", nbatch_);
  Kokkos::deep_copy(t, solver_params_.tadv_default._tbeg);
  Real_1d_view dt("delta time", nbatch_);
  Kokkos::deep_copy(dt, solver_params_.tadv_default._dtmin);

  Real_1d_view_host t_host;
  Real_1d_view_host dt_host;

  t_host = Real_1d_view_host("time host", nbatch_);
  dt_host = Real_1d_view_host("dt host", nbatch_);

  ordinal_type number_of_equations = 0;

  using device_type = typename Tines::UseThisDevice<ExecutionSpace>::type;
  using problem_type =
      TChem::Impl::AtmosphericChemistry_Problem<Real, device_type>;
  number_of_equations = problem_type::getNumberOfTimeODEs(kmcd_);

  Real_2d_view tol_time("tol time", number_of_equations, 2);
  Real_1d_view tol_newton("tol newton", 2);

  Real_2d_view fac("fac", nbatch_, number_of_equations);

  {
    auto tol_time_host = Kokkos::create_mirror_view(tol_time);
    auto tol_newton_host = Kokkos::create_mirror_view(tol_newton);

    for (ordinal_type i = 0, iend = tol_time.extent(0); i < iend; ++i) {
      tol_time_host(i, 0) = solver_params_.atol_time;
      tol_time_host(i, 1) = solver_params_.tol_time;
    }
    tol_newton_host(0) = solver_params_.atol_newton;
    tol_newton_host(1) = solver_params_.rtol_newton;

    Kokkos::deep_copy(tol_time, tol_time_host);
    Kokkos::deep_copy(tol_newton, tol_newton_host);
  }

  time_advance_type_1d_view tadv("tadv", nbatch_);
  Kokkos::deep_copy(tadv, solver_params_.tadv_default);

  const auto tadv_at_i = Kokkos::subview(tadv, 0);
  const auto t_at_i = Kokkos::subview(t, 0);
  const auto state_at_i = Kokkos::subview(state_, 0, Kokkos::ALL());

  auto tadv_at_i_host = Kokkos::create_mirror_view(tadv_at_i);
  auto t_at_i_host = Kokkos::create_mirror_view(t_at_i);
  auto state_at_i_host = Kokkos::create_mirror_view(state_at_i);

  ordinal_type iter = 0;
  if (print_qoi_) {
    Kokkos::deep_copy(tadv_at_i_host, tadv_at_i);
    Kokkos::deep_copy(t_at_i_host, t_at_i);
    Kokkos::deep_copy(state_at_i_host, state_at_i);
    print_state(tadv_at_i_host(), t_at_i_host(), state_at_i_host);
  }

  Kokkos::deep_copy(dt_host, dt);
  Kokkos::deep_copy(t_host, t);

  fmt::print(fout_, "{:s} \t {:s} \t {:s} \t ", "iter", "t", "dt");
  fmt::print(fout_, "{:s} \t {:s} \t {:s} \t", "Density[kg/m3]",
             "Pressure[Pascal]", "Temperature[K]");

  const auto speciesNamesHost = Kokkos::create_mirror_view(kmcd_.speciesNames);
  Kokkos::deep_copy(speciesNamesHost, kmcd_.speciesNames);

  for (ordinal_type k = 0; k < kmcd_.nSpec; k++) {
    fmt::print(fout_, "{:s} \t", &speciesNamesHost(k, 0));
  }
  fmt::print(fout_, "\n");
  // write the initial condition to file (index = -1 => pre-time-stepping)
  static constexpr std::size_t init_condition_iteration = -1;
  write_state(init_condition_iteration, t_host, dt_host, state_host_, fout_);

  Real tsum = 0;
  for (; iter < solver_params_.max_time_iterations &&
         tsum <= solver_params_.tadv_default._tend * 0.9999;
       ++iter) {
    TChem::AtmosphericChemistry::runDeviceBatch(
        policy_, tol_newton, tol_time, fac, tadv, state_, t, dt, state_, kmcd_);

    if (print_qoi_) {
      Kokkos::deep_copy(tadv_at_i_host, tadv_at_i);
      Kokkos::deep_copy(t_at_i_host, t_at_i);
      Kokkos::deep_copy(state_at_i_host, state_at_i);
      print_state(tadv_at_i_host(), t_at_i_host(), state_at_i_host);
    }

    Kokkos::deep_copy(dt_host, dt);
    Kokkos::deep_copy(t_host, t);
    Kokkos::deep_copy(state_host_, state_);

    write_state(iter, t_host, dt_host, state_host_, fout_);

    Real tsum = 0;
    Kokkos::parallel_reduce(
        nbatch_,
        KOKKOS_LAMBDA(const ordinal_type& i, Real& update) {
          tadv(i)._tbeg = t(i);
          tadv(i)._dt = dt(i);
          update += t(i);
        },
        tsum);
    Kokkos::fence();
    tsum /= nbatch_;
  }  // end for

  if (print_qoi_) {
    Kokkos::deep_copy(state_host_, state_);
    for (int i = 0; i < nbatch_; ++i) {
      fmt::print(stdout, "Devices:: Solution sample No {:d}\n", i);
      auto state_at_i = Kokkos::subview(state_host_, i, Kokkos::ALL());
      for (int k = 0; k < state_at_i.extent(0); ++k) {
        fmt::print(stdout, " {:e}", state_at_i(k));
      }
      fmt::print(stdout, "\n");
    }
  }
}

// get the TChem-specific inputs from the input yaml or use defaults
void ChemSolver::parse_tchem_inputs_(const std::string& input_file) {
  auto root = YAML::LoadFile(input_file);
  if (root["tchem"] and root["tchem"].IsMap()) {
    auto node = root["tchem"];
    if (not node["nbatch"]) {
      throw YamlException(
          "problem specific entry does not specify number "
          "of batches (nbatch).");
    } else if (not node["verbose"]) {
      throw YamlException(
          "problem specific entry has no verbose boolean (verbose).");
    } else if (not node["team_size"]) {
      throw YamlException(
          "problem specific entry has no team_size entry (team_size).");
    } else if (not node["vector_size"]) {
      throw YamlException(
          "problem specific entry has no vector_size entry (vector_size).");
    } else if (not node["print_qoi"]) {
      throw YamlException(
          "problem specific entry has no print_qoi boolean (verbose).");
    } else {
      nbatch_ = node["nbatch"].as<int>();
      verbose_ = node["verbose"].as<bool>();
      team_size_ = node["team_size"].as<int>();
      vector_size_ = node["vector_size"].as<int>();
      print_qoi_ = node["print_qoi"].as<bool>();
    }
  } else {
    fmt::print(stdout,
               "No tchem section was found--using default values: verbose = "
               "false, nbatch_ = 1.\n");
    nbatch_ = defaults::nbatch;
    verbose_ = defaults::verbose;
    team_size_ = defaults::team_size;
    vector_size_ = defaults::vector_size;
  }
}

// function to read the chemistry input yaml file (or set defaults) and
// construct a SolverParams struct from what is found there
void SolverParams::set_params(const std::string& filename,
                              const bool& verbose_) {
  auto root = YAML::LoadFile(filename);
  if (root["solver_parameters"] and root["solver_parameters"].IsMap()) {
    auto node = root["solver_parameters"];
    const std::vector<std::string> required_nodes{
        "dtmin",
        "dtmax",
        "tbeg",
        "tend",
        "num_time_iterations_per_interval",
        "max_time_iterations",
        "max_newton_iterations",
        "atol_newton",
        "rtol_newton",
        "atol_time",
        "tol_time",
        "jacobian_interval",
        "outputfile"};
    for (const auto& req_node : required_nodes) {
      if (not node[req_node]) {
        throw YamlException([&]() -> std::string {
          std::stringstream ss;
          ss << "solver_parameters contains no " << req_node << " entry.";
          return ss.str();
        }());
      }
    }
    // initialize tchem's tadv struct
    tadv_default._tbeg = node["tbeg"].as<Real>();
    tadv_default._tend = node["tend"].as<Real>();
    tadv_default._dt = node["dtmin"].as<Real>();
    tadv_default._dtmin = node["dtmin"].as<Real>();
    tadv_default._dtmax = node["dtmax"].as<Real>();
    tadv_default._max_num_newton_iterations =
        node["max_newton_iterations"].as<int>();
    tadv_default._num_time_iterations_per_interval =
        node["num_time_iterations_per_interval"].as<int>();
    tadv_default._jacobian_interval = node["jacobian_interval"].as<int>();

    // initialize the other solver params that don't go in the above struct
    max_time_iterations = node["max_time_iterations"].as<int>();
    atol_newton = node["atol_newton"].as<Real>();
    rtol_newton = node["rtol_newton"].as<Real>();
    atol_time = node["atol_time"].as<Real>();
    tol_time = node["tol_time"].as<Real>();

    outputfile = node["outputfile"].as<std::string>();
  } else {  // defaults
    tadv_default._tbeg = defaults::tbeg;
    tadv_default._tend = defaults::tend;
    tadv_default._dt = defaults::dt;
    tadv_default._dtmin = defaults::dtmin;
    tadv_default._dtmax = defaults::dtmax;
    tadv_default._max_num_newton_iterations = defaults::max_num_newton_iterations;
    tadv_default._num_time_iterations_per_interval = defaults::num_time_iterations_per_interval;
    tadv_default._jacobian_interval = defaults::jacobian_interval;

    max_time_iterations = defaults::max_time_iterations;
    atol_newton = defaults::atol_newton;
    rtol_newton = defaults::rtol_newton;
    atol_time = defaults::atol_time;
    tol_time = defaults::tol_time;
    outputfile = defaults::outputfile;
    if (verbose_) {
      fmt::print(stdout,
                 "No solver_parameters section was found--using defaults\n");
    }
  }
}

// ChemSolver destructor
ChemSolver::~ChemSolver() { fclose(fout_); }

}  // end namespace chem_driver
}  // end namespace haero
