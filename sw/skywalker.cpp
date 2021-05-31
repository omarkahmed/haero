#include "haero/modal_aerosol_config.hpp"
#include "skywalker.hpp"

#include <strings.h>

namespace {

// Provides a reference to a zero value.
skywalker::Real zero_value;

bool is_aerosol(const std::string& param_name) {
  return (param_name.find("aerosols.") != std::string::npos);
}

bool is_number_conc(const std::string& param_name) {
  return (param_name.find("number_conc") != std::string::npos);
}

bool is_gas(const std::string& param_name) {
  return (param_name.find("gases.") != std::string::npos);
}

bool is_atmosphere(const std::string& param_name) {
  return (param_name.find("atmosphere.") != std::string::npos);
}

void parse_aerosol(const haero::ModalAerosolConfig& aero_config,
                   const std::string& param_name, bool& cloudy,
                   int& pop_index) {
  size_t last_dot = param_name.rfind('.');
  size_t penultimate_dot = param_name.rfind('.', last_dot - 1);
  auto mode_name =
      param_name.substr(penultimate_dot + 1, last_dot - penultimate_dot - 1);
  cloudy = (param_name.find("cloudy.") != std::string::npos);
  auto aero_name = param_name.substr(last_dot + 1);
  int mode_index = aero_config.aerosol_mode_index(mode_name, false);
  int aero_index =
      aero_config.aerosol_species_index(mode_index, aero_name, false);
  pop_index = aero_config.population_index(mode_index, aero_index);
}

void parse_number_conc(const haero::ModalAerosolConfig& aero_config,
                       const std::string& param_name, bool& cloudy,
                       int& mode_index) {
  size_t last_dot = param_name.rfind('.');
  size_t penultimate_dot = param_name.rfind('.', last_dot - 1);
  auto mode_name =
      param_name.substr(penultimate_dot + 1, last_dot - penultimate_dot - 1);
  cloudy = (param_name.find("cloudy.") != std::string::npos);
  mode_index = aero_config.aerosol_mode_index(mode_name, false);
}

void parse_gas(const haero::ModalAerosolConfig& aero_config,
               const std::string& param_name, int& gas_index) {
  size_t last_dot = param_name.rfind('.');
  auto gas_name = param_name.substr(last_dot + 1);
  gas_index = aero_config.gas_index(gas_name, false);
}

}  // namespace

namespace skywalker {

Real InputData::operator[](const std::string& param_name) const {
  if (is_number_conc(param_name)) {
    bool cloud;
    int mode_index;
    parse_number_conc(aero_config, param_name, cloud, mode_index);
    if (cloud) {
      return cloud_number_concs[mode_index];
    } else {
      return interstitial_number_concs[mode_index];
    }
  } else if (is_aerosol(param_name)) {
    bool cloud;
    int pop_index;
    parse_aerosol(aero_config, param_name, cloud, pop_index);
    if (cloud) {
      return cloud_aero_mmrs[pop_index];
    } else {
      return interstitial_aero_mmrs[pop_index];
    }
  } else if (is_gas(param_name)) {
    int gas_index;
    parse_gas(aero_config, param_name, gas_index);
    return gas_mmrs[gas_index];
  } else if (is_atmosphere(param_name)) {
    if (param_name.find("temperature") != std::string::npos) {
      return temperature;
    } else if (param_name.find("pressure") != std::string::npos) {
      return pressure;
    } else if (param_name.find("relative_humidity") != std::string::npos) {
      return relative_humidity;
    } else if (param_name.find("height") != std::string::npos) {
      return height;
    } else if (param_name.find("hydrostatic_dp") != std::string::npos) {
      return hydrostatic_dp;
    } else if (param_name.find("planetary_boundary_layer_height") !=
               std::string::npos) {
      return planetary_boundary_layer_height;
    } else {
      return 0.0;
    }
  } else {
    return 0.0;
  }
}

Real& InputData::operator[](const std::string& param_name) {
  if (is_number_conc(param_name)) {
    bool cloud;
    int mode_index;
    parse_number_conc(aero_config, param_name, cloud, mode_index);
    if (cloud) {
      if (cloud_number_concs.size() <= mode_index) {
        cloud_number_concs.resize(mode_index + 1);
      }
      return cloud_number_concs[mode_index];
    } else {
      if (interstitial_number_concs.size() <= mode_index) {
        interstitial_number_concs.resize(mode_index + 1);
      }
      return interstitial_number_concs[mode_index];
    }
  } else if (is_aerosol(param_name)) {
    bool cloud;
    int pop_index;
    parse_aerosol(aero_config, param_name, cloud, pop_index);
    if (cloud) {
      if (cloud_aero_mmrs.size() <= pop_index) {
        cloud_aero_mmrs.resize(pop_index + 1);
      }
      return cloud_aero_mmrs[pop_index];
    } else {
      if (interstitial_aero_mmrs.size() <= pop_index) {
        interstitial_aero_mmrs.resize(pop_index + 1);
      }
      return interstitial_aero_mmrs[pop_index];
    }
  } else if (is_gas(param_name)) {
    int gas_index;
    parse_gas(aero_config, param_name, gas_index);
    if (gas_mmrs.size() <= gas_index) {
      gas_mmrs.resize(gas_index + 1);
    }
    return gas_mmrs[gas_index];
  } else if (is_atmosphere(param_name)) {
    if (param_name.find("temperature") != std::string::npos) {
      return temperature;
    } else if (param_name.find("pressure") != std::string::npos) {
      return pressure;
    } else if (param_name.find("relative_humidity") != std::string::npos) {
      return relative_humidity;
    } else if (param_name.find("height") != std::string::npos) {
      return height;
    } else if (param_name.find("hydrostatic_dp") != std::string::npos) {
      return hydrostatic_dp;
    } else if (param_name.find("planetary_boundary_layer_height") !=
               std::string::npos) {
      return planetary_boundary_layer_height;
    } else {
      zero_value = 0.0;
      return zero_value;
    }
  } else {
    zero_value = 0.0;
    return zero_value;
  }
}

Real OutputData::operator[](const std::string& param_name) const {
  if (is_number_conc(param_name)) {
    bool cloud;
    int mode_index;
    parse_number_conc(aero_config, param_name, cloud, mode_index);
    if (cloud) {
      return cloud_number_concs[mode_index];
    } else {
      return interstitial_number_concs[mode_index];
    }
  } else if (is_aerosol(param_name)) {
    bool cloud;
    int pop_index;
    parse_aerosol(aero_config, param_name, cloud, pop_index);
    if (cloud) {
      return cloud_aero_mmrs[pop_index];
    } else {
      return interstitial_aero_mmrs[pop_index];
    }
  } else if (is_gas(param_name)) {
    int gas_index;
    parse_gas(aero_config, param_name, gas_index);
    return gas_mmrs[gas_index];
  } else {
    return 0.0;
  }
}

std::vector<InputData> ParameterWalk::gather_inputs(const std::set<std::string>& excluded_params) const {
  // Count up the number of inputs defined by the parameter walk thingy,
  // excluding those parameters specified.
  size_t num_inputs = 1, num_params = 0;
  for (auto param: ensemble) {
    if (excluded_params.find(param.first) == excluded_params.end()) { // not excluded
      num_inputs *= param.second.size(); // set of parameter values
      num_params++;
    }
  }
  EKAT_REQUIRE_MSG(((num_params >= 1) and (num_params <= 5)),
                   "Invalid number of overridden parameters ("
                       << num_params << ", must be 1-5).");

  // Start from reference data and build a list of inputs corresponding to all
  // the overridden parameters. This involves some ugly index magic based on the
  // number of parameters.
  std::vector<InputData> inputs(num_inputs, ref_input);
  for (size_t l = 0; l < num_inputs; ++l) {
    if (num_params == 1) {
      auto iter = ensemble.begin();
      auto name = iter->first;
      const auto& vals = iter->second;
      inputs[l][name] = vals[l];
    } else if (num_params == 2) {
      auto iter = ensemble.begin();
      auto name1 = iter->first;
      const auto& vals1 = iter->second;
      iter++;
      auto name2 = iter->first;
      const auto& vals2 = iter->second;
      size_t n2 = vals2.size();
      size_t j1 = l / n2;
      size_t j2 = l - n2 * j1;
      inputs[l][name1] = vals1[j1];
      inputs[l][name2] = vals2[j2];
    } else if (num_params == 3) {
      auto iter = ensemble.begin();
      auto name1 = iter->first;
      const auto& vals1 = iter->second;
      iter++;
      auto name2 = iter->first;
      const auto& vals2 = iter->second;
      iter++;
      auto name3 = iter->first;
      const auto& vals3 = iter->second;
      size_t n2 = vals2.size();
      size_t n3 = vals3.size();
      size_t j1 = l / (n2 * n3);
      size_t j2 = (l - n2 * n3 * j1) / n3;
      size_t j3 = l - n2 * n3 * j1 - n3 * j2;
      inputs[l][name1] = vals1[j1];
      inputs[l][name2] = vals2[j2];
      inputs[l][name3] = vals3[j3];
    } else if (num_params == 4) {
      auto iter = ensemble.begin();
      auto name1 = iter->first;
      const auto& vals1 = iter->second;
      iter++;
      auto name2 = iter->first;
      const auto& vals2 = iter->second;
      iter++;
      auto name3 = iter->first;
      const auto& vals3 = iter->second;
      iter++;
      auto name4 = iter->first;
      const auto& vals4 = iter->second;
      size_t n2 = vals2.size();
      size_t n3 = vals3.size();
      size_t n4 = vals4.size();
      size_t j1 = l / (n2 * n3 * n4);
      size_t j2 = (l - n2 * n3 * n4 * j1) / (n3 * n4);
      size_t j3 = (l - n2 * n3 * n4 * j1 - n3 * n4 * j2) / n4;
      size_t j4 = l - n2 * n3 * n4 * j1 - n3 * n4 * j2 - n4 * j3;
      inputs[l][name1] = vals1[j1];
      inputs[l][name2] = vals2[j2];
      inputs[l][name3] = vals3[j3];
      inputs[l][name4] = vals4[j4];
    } else {  // if (num_params == 5)
      auto iter = ensemble.begin();
      auto name1 = iter->first;
      const auto& vals1 = iter->second;
      iter++;
      auto name2 = iter->first;
      const auto& vals2 = iter->second;
      iter++;
      auto name3 = iter->first;
      const auto& vals3 = iter->second;
      iter++;
      auto name4 = iter->first;
      const auto& vals4 = iter->second;
      iter++;
      auto name5 = iter->first;
      const auto& vals5 = iter->second;
      size_t n2 = vals2.size();
      size_t n3 = vals3.size();
      size_t n4 = vals4.size();
      size_t n5 = vals5.size();
      size_t j1 = l / (n2 * n3 * n4 * n5);
      size_t j2 = (l - n2 * n3 * n4 * n5 * j1) / (n3 * n4 * n5);
      size_t j3 = (l - n2 * n3 * n4 * n5 * j1 - n3 * n4 * n5 * j2) / (n4 * n5);
      size_t j4 =
          (l - n2 * n3 * n4 * n5 * j1 - n3 * n4 * n5 * j2 - n4 * n5 * j3) / n5;
      size_t j5 = l - n2 * n3 * n4 * n5 * j1 - n3 * n4 * n5 * j2 -
                  n4 * n5 * j3 - n5 * j4;
      inputs[l][name1] = vals1[j1];
      inputs[l][name2] = vals2[j2];
      inputs[l][name3] = vals3[j3];
      inputs[l][name4] = vals4[j4];
      inputs[l][name5] = vals5[j5];
    }
  }

  return inputs;
}

}  // namespace skywalker

//----------------------------
// Skywalker Fortran bindings
//----------------------------
// The Skywalker Fortran interface is tailored to the needs of the MAM box
// model. At any given time, its design is likely to reflect the needs of a
// handful of legacy MAM-related codes for comparison with Haero. In this
// sense, it's not a "faithful" Fortran representation of the Skywalker C++
// library.

extern "C" {

using Real = skywalker::Real;
using ModalAerosolConfig = haero::ModalAerosolConfig;
using InputData = skywalker::InputData;
using OutputData = skywalker::OutputData;
using EnsembleData = std::pair<std::vector<InputData>, std::vector<OutputData>>;

// This container holds "live" instances of aerosol config metadata.
static std::map<std::string, ModalAerosolConfig>* fortran_aero_configs_ = nullptr;

static void destroy_aero_configs() {
  delete fortran_aero_configs_;
  fortran_aero_configs_ = nullptr;
}

// This container holds "live" instances of Skywalker Fortran ensemble data,
// which is managed by this Fortran bridge.
static std::set<EnsembleData*>* fortran_ensembles_ = nullptr;

static void destroy_ensembles() {
  for (auto ensemble: *fortran_ensembles_) {
    delete ensemble;
  }
  delete fortran_ensembles_;
  fortran_ensembles_ = nullptr;
}

/// Parses the given file, assuming the given named aerosol configuration,
/// returning an opaque pointer to the ensemble data.
/// @param [in] aerosol_config The named aerosol configuration. The only valid
///                            configuration at this time is "mam4".
/// @param [in] filename The name of the YAML file containing ensemble data.
/// @param [in] model_impl The name of the model implementation (typically
///                        "haero" or "mam").
void* sw_load_ensemble(const char* aerosol_config,
                       const char* filename,
                       const char* model_impl) {
  // Construct an aerosol config from the given string.
  if (fortran_aero_configs_ == nullptr) {
    fortran_aero_configs_ = new std::map<std::string, ModalAerosolConfig>();
    atexit(destroy_aero_configs);
  }
  auto config_iter = fortran_aero_configs_->find(aerosol_config);
  ModalAerosolConfig config;
  if (config_iter != fortran_aero_configs_->end()) {
    config = config_iter->second;
  } else {
    if (strcasecmp(aerosol_config, "mam4") == 0) {
      config = haero::create_mam4_modal_aerosol_config();
      fortran_aero_configs_->emplace("mam4", config);
    } else {
      return nullptr; // no dice!
    }
  }

  // Create a ParameterWalk object from the given config and file.
  auto param_walk = skywalker::load_ensemble(config, filename, model_impl);

  // Create an ensemble, allocating storage for output data equal in length
  // to the given input data.
  auto ensemble = new EnsembleData;
  ensemble->first = param_walk.gather_inputs();
  ensemble->second = std::vector<OutputData>(ensemble->first.size(), OutputData(config));
  // Size up the output data arrays to make our life easier down the line.
  for (size_t i = 0; i < ensemble->first.size(); ++i) {
    const auto& input = ensemble->first[i];
    auto output = ensemble->second[i];
    output.interstitial_number_concs.resize(input.interstitial_number_concs.size());
    output.cloud_number_concs.resize(input.cloud_number_concs.size());
    output.interstitial_aero_mmrs.resize(input.interstitial_aero_mmrs.size());
    output.cloud_aero_mmrs.resize(input.cloud_aero_mmrs.size());
    output.gas_mmrs.resize(input.gas_mmrs.size());
  }

  // Track this ensemble, storing its pointer for future reference.
  if (fortran_ensembles_ == nullptr) {
    fortran_ensembles_ = new std::set<EnsembleData*>();
    atexit(destroy_ensembles);
  }
  fortran_ensembles_->emplace(ensemble);
  auto ensemble_ptr = reinterpret_cast<void*>(ensemble);
  return ensemble_ptr;
}

/// Returns the number of inputs (members) for the given ensemble data.
int sw_ensemble_size(void* ensemble) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  return data->first.size();
}

/// Fetches array sizes for members in the given ensemble.
void sw_ensemble_get_array_sizes(void* ensemble, int* num_modes,
                                 int* num_populations, int* num_gases) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  EKAT_REQUIRE(not data->first.empty());

  // Get the aerosol configuration from the first ensemble member.
  const auto& config = data->first[0].aero_config;

  // Read off the data.
  *num_modes = config.num_modes();
  *num_populations = config.num_aerosol_populations;
  *num_gases = config.num_gases();
}

/// Fetches the number of aerosols present in each mode, which can be used
/// to map between population and aerosol indices. The output array is sized
/// to store the number of aerosols in each mode.
void sw_ensemble_get_modal_aerosol_sizes(void* ensemble, int* aerosols_per_mode) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  EKAT_REQUIRE(not data->first.empty());

  // Get the aerosol configuration from the first ensemble member.
  const auto& config = data->first[0].aero_config;

  // Fetch the numbers of species per mode.
  for (int m = 0; m < config.num_modes(); ++m) {
    const auto mode_species = config.aerosol_species_for_mode(m);
    aerosols_per_mode[m] = int(mode_species.size());
  }
}

/// Fetches an opaque pointer to the ith set of input data from the given
/// ensemble.
void* sw_ensemble_input(void* ensemble, int i) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  EKAT_REQUIRE(i >= 0);
  EKAT_REQUIRE(i < data->first.size());
  InputData* input = &(data->first[i]);
  return reinterpret_cast<void*>(input);
}

/// Fetches timestepping data from the given ensemble input data pointer.
void sw_input_get_timestepping(void* input, Real* dt, Real* total_time) {
  auto inp = reinterpret_cast<InputData*>(input);
  *dt = inp->dt;
  *total_time = inp->total_time;
}

/// Fetches atmosphere data from the given ensemble input data pointer.
void sw_input_get_atmosphere(void* input, Real* temperature, Real* pressure,
                             Real* relative_humidity, Real* height,
                             Real* hydrostatic_dp,
                             Real* planetary_boundary_layer_height) {
  auto inp = reinterpret_cast<InputData*>(input);
  *temperature = inp->temperature;
  *pressure = inp->pressure;
  *relative_humidity = inp->relative_humidity;
  *height = inp->height;
  *hydrostatic_dp = inp->hydrostatic_dp;
  *planetary_boundary_layer_height = inp->planetary_boundary_layer_height;
}

/// Fetches aerosol data from the given ensemble input data pointer. All output
/// arguments are arrays that are properly sized to store aerosol data.
void sw_input_get_aerosols(void* input, Real* interstitial_number_concs,
                           Real* cloud_number_concs, Real* interstitial_aero_mmrs,
                           Real* cloud_aero_mmrs) {
  auto inp = reinterpret_cast<InputData*>(input);
  std::copy(inp->interstitial_number_concs.begin(), inp->interstitial_number_concs.end(),
            interstitial_number_concs);
  std::copy(inp->cloud_number_concs.begin(), inp->cloud_number_concs.end(),
            cloud_number_concs);
  std::copy(inp->interstitial_aero_mmrs.begin(), inp->interstitial_aero_mmrs.end(),
            interstitial_aero_mmrs);
  std::copy(inp->cloud_aero_mmrs.begin(), inp->cloud_aero_mmrs.end(),
            cloud_aero_mmrs);
}

/// Fetches gas data from the given ensemble input data pointer. The output
/// argument is an array properly sized to store gas mass mixing ratios.
void sw_input_get_gases(void* input, Real* gas_mmrs) {
  auto inp = reinterpret_cast<InputData*>(input);
  std::copy(inp->gas_mmrs.begin(), inp->gas_mmrs.end(), gas_mmrs);
}

/// Fetches an opaque pointer to the ith set of output data from the given
/// ensemble.
void* sw_ensemble_output(void* ensemble, int i) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  EKAT_REQUIRE(i >= 0);
  EKAT_REQUIRE(i < data->first.size());
  OutputData* output = &(data->second[i]);
  return reinterpret_cast<void*>(output);
}

/// Sets aerosol data for the given ensemble output data pointer.
void sw_output_set_aerosols(void* output, Real* interstitial_number_concs,
                            Real* cloud_number_concs, Real* interstitial_aero_mmrs,
                            Real* cloud_aero_mmrs) {
  auto outp = reinterpret_cast<OutputData*>(output);
  size_t num_modes = outp->interstitial_number_concs.size();
  size_t num_pops = outp->interstitial_aero_mmrs.size();
  std::copy(interstitial_number_concs, interstitial_number_concs + num_modes,
            outp->interstitial_number_concs.begin());
  std::copy(cloud_number_concs, cloud_number_concs + num_modes,
            outp->cloud_number_concs.begin());
  std::copy(interstitial_aero_mmrs, interstitial_aero_mmrs + num_pops,
            outp->interstitial_aero_mmrs.begin());
  std::copy(cloud_aero_mmrs, cloud_aero_mmrs + num_pops,
            outp->cloud_aero_mmrs.begin());
}

/// Sets gas data for the given ensemble output data pointer.
void sw_output_set_gases(void* output, Real* gas_mmrs) {
  auto outp = reinterpret_cast<OutputData*>(output);
  size_t num_gases = outp->gas_mmrs.size();
  std::copy(gas_mmrs, gas_mmrs + num_gases, outp->gas_mmrs.begin());
}

// Writes out a Python module containing input and output data for the
// ǥiven ensemble to the given filename.
void sw_ensemble_write_py_module(void* ensemble, const char* filename) {
  auto data = reinterpret_cast<EnsembleData*>(ensemble);
  skywalker::write_py_module(data->first, data->second, filename);
}

/// Frees all memory associated with the ensemble, including input and output
/// data.
void sw_enѕemble_free(void* ensemble) {
  if (fortran_ensembles_ != nullptr) {
    auto data = reinterpret_cast<EnsembleData*>(ensemble);
    auto iter = fortran_ensembles_->find(data);
    if (iter != fortran_ensembles_->end()) {
      fortran_ensembles_->erase(iter);
      delete data;
    }
  }
}

}
