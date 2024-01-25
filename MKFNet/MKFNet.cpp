#include <iostream>
#include "MKFNet.h"
#include "Constants.h"
#include "Insulation.h"
#include "Defaults.h"
#include <MAS.hpp>
#include "MagneticSimulator.h"
#include "InputsWrapper.h"
#include "CoreWrapper.h"
#include "Reluctance.h"
#include "MagnetizingInductance.h"
#include "CoreLosses.h"
#include "CoreTemperature.h"
#include "CoreAdviser.h"
#include "Utils.h"
#include "Settings.h"


MKFNet::MKFNet(){
}

std::string MKFNet::GetMaterial(std::string materialName) {
    auto materialData = OpenMagnetics::find_core_material_by_name(materialName);
    json result;
    to_json(result, materialData);
    return result.dump(4);
}

std::string MKFNet::CalculateCoreLosses(std::string coreData, std::string coilData, std::string inputsData, std::string modelsData) {
    OpenMagnetics::CoreWrapper core(json::parse(coreData));
    OpenMagnetics::CoilWrapper coil(json::parse(coilData));
    OpenMagnetics::InputsWrapper inputs(json::parse(inputsData));
    auto operatingPoint = inputs.get_operating_point(0);
    OpenMagnetics::OperatingPointExcitation excitation = operatingPoint.get_excitations_per_winding()[0];
    double magnetizingInductance = OpenMagnetics::resolve_dimensional_values(inputs.get_design_requirements().get_magnetizing_inductance());
    if (!excitation.get_current()) {
        auto magnetizingCurrent = OpenMagnetics::InputsWrapper::calculate_magnetizing_current(excitation, magnetizingInductance);
        excitation.set_current(magnetizingCurrent);
        operatingPoint.get_mutable_excitations_per_winding()[0] = excitation;
    }

    auto defaults = OpenMagnetics::Defaults();

    std::map<std::string, std::string> models = json::parse(modelsData).get<std::map<std::string, std::string>>();

    auto reluctanceModelName = defaults.reluctanceModelDefault;
    if (models.find("reluctance") != models.end()) {
        std::string modelNameStringUpper = models["reluctance"];
        std::transform(modelNameStringUpper.begin(), modelNameStringUpper.end(), modelNameStringUpper.begin(), ::toupper);
        reluctanceModelName = magic_enum::enum_cast<OpenMagnetics::ReluctanceModels>(modelNameStringUpper).value();
    }
    auto coreLossesModelName = defaults.coreLossesModelDefault;
    if (models.find("coreLosses") != models.end()) {
        std::string modelNameStringUpper = models["coreLosses"];
        std::transform(modelNameStringUpper.begin(), modelNameStringUpper.end(), modelNameStringUpper.begin(), ::toupper);
        coreLossesModelName = magic_enum::enum_cast<OpenMagnetics::CoreLossesModels>(modelNameStringUpper).value();
    }
    auto coreTemperatureModelName = defaults.coreTemperatureModelDefault;
    if (models.find("coreTemperature") != models.end()) {
        std::string modelNameStringUpper = models["coreTemperature"];
        std::transform(modelNameStringUpper.begin(), modelNameStringUpper.end(), modelNameStringUpper.begin(), ::toupper);
        coreTemperatureModelName = magic_enum::enum_cast<OpenMagnetics::CoreTemperatureModels>(modelNameStringUpper).value();
    }

    OpenMagnetics::Magnetic magnetic;
    magnetic.set_core(core);
    magnetic.set_coil(coil);

    OpenMagnetics::MagneticSimulator magneticSimulator;
    magneticSimulator.set_core_losses_model_name(coreLossesModelName);
    magneticSimulator.set_core_temperature_model_name(coreTemperatureModelName);
    magneticSimulator.set_reluctance_model_name(reluctanceModelName);
    auto coreLossesOutput = magneticSimulator.calculate_core_loses(operatingPoint, magnetic);
    json result;
    to_json(result, coreLossesOutput);

    OpenMagnetics::MagnetizingInductance magnetizing_inductance(reluctanceModelName);
    auto magneticFluxDensity = magnetizing_inductance.calculate_inductance_and_magnetic_flux_density(core, coil, &operatingPoint).second;

    result["magneticFluxDensityPeak"] = magneticFluxDensity.get_processed().value().get_peak().value();
    result["magneticFluxDensityAcPeak"] = magneticFluxDensity.get_processed().value().get_peak().value() - magneticFluxDensity.get_processed().value().get_offset();
    result["voltageRms"] = operatingPoint.get_mutable_excitations_per_winding()[0].get_voltage().value().get_processed().value().get_rms().value();
    result["currentRms"] = operatingPoint.get_mutable_excitations_per_winding()[0].get_current().value().get_processed().value().get_rms().value();
    result["apparentPower"] = operatingPoint.get_mutable_excitations_per_winding()[0].get_voltage().value().get_processed().value().get_rms().value() * operatingPoint.get_mutable_excitations_per_winding()[0].get_current().value().get_processed().value().get_rms().value();
    if (coreLossesOutput.get_temperature()) {
        result["maximumCoreTemperature"] = coreLossesOutput.get_temperature().value();
        result["maximumCoreTemperatureRise"] = coreLossesOutput.get_temperature().value() - operatingPoint.get_conditions().get_ambient_temperature();
    }

    return result.dump(4);
}