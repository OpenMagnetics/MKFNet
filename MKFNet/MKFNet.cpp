#include <iostream>
#include "MKFNet.h"
#include "Constants.h"
#include "Insulation.h"
#include "Defaults.h"
#include <MAS.hpp>
#include "MagneticSimulator.h"
#include "InputsWrapper.h"
#include "CoreWrapper.h"
#include "MagneticWrapper.h"
#include "Reluctance.h"
#include "MagnetizingInductance.h"
#include "MagneticAdviser.h"
#include "MagneticField.h"
#include "WindingSkinEffectLosses.h"
#include "WindingProximityEffectLosses.h"
#include "WindingOhmicLosses.h"
#include "CoreLosses.h"
#include "CoreTemperature.h"
#include "CoreAdviser.h"
#include "Utils.h"
#include "Settings.h"
#include "Painter.h"
#include <vector>

MKFNet::MKFNet(){
}

std::string MKFNet::GetMaterial(std::string materialName) {
    try {
        auto materialData = OpenMagnetics::find_core_material_by_name(materialName);
        json result;
        to_json(result, materialData);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::GetShape(std::string shapeName) {
    try {
        auto shapeData = OpenMagnetics::find_core_shape_by_name(shapeName);
        json result;
        to_json(result, shapeData);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateCoreData(std::string coreDataString, bool includeMaterialData){
    try {
        OpenMagnetics::CoreWrapper core(json::parse(coreDataString), includeMaterialData);
        json result;
        to_json(result, core);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateCoreProcessedDescription(std::string coreDataString){
    try {
        OpenMagnetics::CoreWrapper core(json::parse(coreDataString), false, false, false);
        core.process_data();
        json result;
        to_json(result, core.get_processed_description().value());
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateCoreGeometricalDescription(std::string coreDataString){
    try {
        OpenMagnetics::CoreWrapper core(json::parse(coreDataString), false, false, false);
        auto geometricalDescription = core.create_geometrical_description().value();
        json result = json::array();
        for (auto& elem : geometricalDescription) {
            json aux;
            to_json(aux, elem);
            result.push_back(aux);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateCoreGapping(std::string coreDataString){
    try {
        OpenMagnetics::CoreWrapper core(json::parse(coreDataString), false, false, false);
        core.process_gap();
        json result = json::array();
        for (auto& gap : core.get_functional_description().get_gapping()) {
            json aux;
            to_json(aux, gap);
            result.push_back(aux);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::Wind(std::string coilString, size_t repetitions, std::string proportionPerWindingString, std::string patternString) {
    try {
        auto coilJson = json::parse(coilString);

        std::vector<double> proportionPerWinding = json::parse(proportionPerWindingString);
        std::vector<size_t> pattern = json::parse(patternString);
        auto coilFunctionalDescription = std::vector<OpenMagnetics::CoilFunctionalDescription>(coilJson["functionalDescription"]);
        OpenMagnetics::CoilWrapper coil;
        if (coilJson.contains("_interleavingLevel")) {
            coil.set_interleaving_level(coilJson["_interleavingLevel"]);
        }
        if (coilJson.contains("_windingOrientation")) {
            coil.set_winding_orientation(coilJson["_windingOrientation"]);
        }
        if (coilJson.contains("_layersOrientation")) {
            coil.set_layers_orientation(coilJson["_layersOrientation"]);
        }
        if (coilJson.contains("_turnsAlignment")) {
            coil.set_turns_alignment(coilJson["_turnsAlignment"]);
        }
        if (coilJson.contains("_sectionAlignment")) {
            coil.set_section_alignment(coilJson["_sectionAlignment"]);
        }

        coil.set_bobbin(coilJson["bobbin"]);
        coil.set_functional_description(coilFunctionalDescription);
        if (proportionPerWinding.size() == coilFunctionalDescription.size()) {
            if (pattern.size() > 0 && repetitions > 0) {
                coil.wind(proportionPerWinding, pattern, repetitions);
            }
            else if (repetitions > 0) {
                coil.wind(repetitions);
            }
            else {
                coil.wind();
            }
        }
        else {
            if (pattern.size() > 0 && repetitions > 0) {
                coil.wind(pattern, repetitions);
            }
            else if (repetitions > 0) {
                coil.wind(repetitions);
            }
            else {
                coil.wind();
            }
        }

        json result;
        to_json(result, coil);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::GetDefaultModels() {
    try {
        json models;
        auto reluctanceModelName = magic_enum::enum_name(OpenMagnetics::Defaults().reluctanceModelDefault);
        models["reluctance"] = reluctanceModelName;
        auto coreLossesModelName = magic_enum::enum_name(OpenMagnetics::Defaults().coreLossesModelDefault);
        models["coreLosses"] = coreLossesModelName;
        auto coreTemperatureModelName = magic_enum::enum_name(OpenMagnetics::Defaults().coreTemperatureModelDefault);
        models["coreTemperature"] = coreTemperatureModelName;

        return models.dump();
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateCoreLosses(std::string magneticString, std::string inputsString, std::string modelsString) {
    try {
        OpenMagnetics::MagneticWrapper magnetic(json::parse(magneticString));
        OpenMagnetics::CoreWrapper core = magnetic.get_core();
        OpenMagnetics::CoilWrapper coil = magnetic.get_coil();
        OpenMagnetics::InputsWrapper inputs(json::parse(inputsString));
        auto operatingPoint = inputs.get_operating_point(0);
        OpenMagnetics::OperatingPointExcitation excitation = operatingPoint.get_excitations_per_winding()[0];
        double magnetizingInductance = OpenMagnetics::resolve_dimensional_values(inputs.get_design_requirements().get_magnetizing_inductance());
        if (!excitation.get_current()) {
            auto magnetizingCurrent = OpenMagnetics::InputsWrapper::calculate_magnetizing_current(excitation, magnetizingInductance);
            excitation.set_current(magnetizingCurrent);
            operatingPoint.get_mutable_excitations_per_winding()[0] = excitation;
        }

        auto defaults = OpenMagnetics::Defaults();

        std::map<std::string, std::string> models = json::parse(modelsString).get<std::map<std::string, std::string>>();

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
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateAdvisedCores(std::string inputsString, std::string weightsString, int maximumNumberResults, bool useOnlyCoresInStock){
    try {
        OpenMagnetics::InputsWrapper inputs(json::parse(inputsString));
        std::map<std::string, double> weightsKeysString = json::parse(weightsString);
        std::map<OpenMagnetics::CoreAdviser::CoreAdviserFilters, double> weights;

        for (auto const& pair : weightsKeysString) {
            weights[magic_enum::enum_cast<OpenMagnetics::CoreAdviser::CoreAdviserFilters>(pair.first).value()] = pair.second;
        }
        weights[OpenMagnetics::CoreAdviser::CoreAdviserFilters::AREA_PRODUCT] = 1;
        weights[OpenMagnetics::CoreAdviser::CoreAdviserFilters::ENERGY_STORED] = 1;
        weights[OpenMagnetics::CoreAdviser::CoreAdviserFilters::COST] = 1;
        weights[OpenMagnetics::CoreAdviser::CoreAdviserFilters::EFFICIENCY] = 1;
        weights[OpenMagnetics::CoreAdviser::CoreAdviserFilters::DIMENSIONS] = 1;

        auto settings = OpenMagnetics::Settings::GetInstance();
        settings->set_use_only_cores_in_stock(useOnlyCoresInStock);

        OpenMagnetics::CoreAdviser coreAdviser(false);
        auto masMagnetics = coreAdviser.get_advised_core(inputs, weights, maximumNumberResults);

        json results = json::array();
        for (auto& masMagnetic : masMagnetics) {
            json aux;
            to_json(aux, masMagnetic.first);
            results.push_back(aux);
        }
        settings->reset();

        return results.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateAdvisedMagnetics(std::string inputsString, int maximumNumberResults){
    try {
        OpenMagnetics::InputsWrapper inputs(json::parse(inputsString));

        OpenMagnetics::MagneticAdviser magneticAdviser;
        auto masMagnetics = magneticAdviser.get_advised_magnetic(inputs, maximumNumberResults);

        json results = json::array();
        for (auto& masMagnetic : masMagnetics) {
            json aux;
            to_json(aux, masMagnetic);
            results.push_back(aux);
        }

        return results.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateWindingLosses(std::string magneticString, std::string operatingPointString, double temperature) {
    try {
        OpenMagnetics::MagneticWrapper magnetic(json::parse(magneticString));
        OpenMagnetics::OperatingPoint operatingPoint(json::parse(operatingPointString));

        auto windingLossesOutput = OpenMagnetics::WindingLosses().calculate_losses(magnetic, operatingPoint, temperature);

        json result;
        to_json(result, windingLossesOutput);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateOhmicLosses(std::string coilString, std::string operatingPointString, double temperature) {
    try {
        OpenMagnetics::CoilWrapper coil(json::parse(coilString));
        OpenMagnetics::OperatingPoint operatingPoint(json::parse(operatingPointString));

        auto windingLossesOutput = OpenMagnetics::WindingOhmicLosses::calculate_ohmic_losses(coil, operatingPoint, temperature);

        json result;
        to_json(result, windingLossesOutput);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateMagneticFieldStrengthField(std::string operatingPointString, std::string magneticString) {
    try {
        OpenMagnetics::MagneticWrapper magnetic(json::parse(magneticString));
        OpenMagnetics::OperatingPoint operatingPoint(json::parse(operatingPointString));
        OpenMagnetics::MagneticField magneticField;

        auto windingWindowMagneticStrengthFieldOutput = magneticField.calculate_magnetic_field_strength_field(operatingPoint, magnetic);

        json result;
        to_json(result, windingWindowMagneticStrengthFieldOutput);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateProximityEffectLosses(std::string coilString, double temperature, std::string windingLossesOutputString, std::string windingWindowMagneticStrengthFieldOutputString) {
    try {
        OpenMagnetics::CoilWrapper coil(json::parse(coilString));
        OpenMagnetics::WindingLossesOutput windingLossesOutput(json::parse(windingLossesOutputString));
        OpenMagnetics::WindingWindowMagneticStrengthFieldOutput windingWindowMagneticStrengthFieldOutput(json::parse(windingWindowMagneticStrengthFieldOutputString));

        auto windingLossesOutputOutput = OpenMagnetics::WindingProximityEffectLosses::calculate_proximity_effect_losses(coil, temperature, windingLossesOutput, windingWindowMagneticStrengthFieldOutput);

        json result;
        to_json(result, windingLossesOutputOutput);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateSkinEffectLosses(std::string coilString, std::string windingLossesOutputString, double temperature) {
    try {
        OpenMagnetics::CoilWrapper coil(json::parse(coilString));
        OpenMagnetics::WindingLossesOutput windingLossesOutput(json::parse(windingLossesOutputString));

        auto windingLossesOutputOutput = OpenMagnetics::WindingSkinEffectLosses::calculate_skin_effect_losses(coil, temperature, windingLossesOutput);
        json result;
        to_json(result, windingLossesOutputOutput);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateSkinEffectLossesPerMeter(std::string wireString, std::string currentString, double temperature, double currentDivider) {
    try {
        OpenMagnetics::WireWrapper wire(json::parse(wireString));
        OpenMagnetics::SignalDescriptor current(json::parse(currentString));

        auto skinEffectLossesPerMeter = OpenMagnetics::WindingSkinEffectLosses::calculate_skin_effect_losses_per_meter(wire, current, temperature, currentDivider);

        json result = skinEffectLossesPerMeter;
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

bool MKFNet::PlotCore(std::string magneticString, std::string outFile) {
    try {
        OpenMagnetics::MagneticWrapper magnetic(json::parse(magneticString));
        OpenMagnetics::Painter painter(outFile);
        painter.paint_core(magnetic);
        painter.paint_bobbin(magnetic);
        painter.export_svg();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool MKFNet::PlotSections(std::string magneticString, std::string outFile) {
    try {
        OpenMagnetics::MagneticWrapper magnetic(json::parse(magneticString));
        OpenMagnetics::Painter painter(outFile);
        painter.paint_core(magnetic);
        painter.paint_bobbin(magnetic);
        painter.paint_coil_sections(magnetic);
        painter.export_svg();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool MKFNet::PlotLayers(std::string magneticString, std::string outFile) {
    try {
        OpenMagnetics::MagneticWrapper magnetic(json::parse(magneticString));
        OpenMagnetics::Painter painter(outFile);
        painter.paint_core(magnetic);
        painter.paint_bobbin(magnetic);
        painter.paint_coil_layers(magnetic);
        painter.export_svg();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool MKFNet::PlotTurns(std::string magneticString, std::string outFile) {
    try {
        OpenMagnetics::MagneticWrapper magnetic(json::parse(magneticString));
        OpenMagnetics::Painter painter(outFile);
        painter.paint_core(magnetic);
        painter.paint_bobbin(magnetic);
        painter.paint_coil_turns(magnetic);
        painter.export_svg();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool MKFNet::PlotField(std::string magneticString, std::string operatingPointString, std::string outFile) {
    try {
        auto settings = OpenMagnetics::Settings::GetInstance();
        OpenMagnetics::MagneticWrapper magnetic(json::parse(magneticString));
        OpenMagnetics::OperatingPoint operatingPoint(json::parse(operatingPointString));
        OpenMagnetics::Painter painter(outFile);
        painter.paint_magnetic_field(operatingPoint, magnetic);
        painter.paint_core(magnetic);
        painter.paint_bobbin(magnetic);
        painter.paint_coil_turns(magnetic);
        painter.export_svg();
        return true;
    }
    catch (...) {
        return false;
    }
}

std::string MKFNet::GetSettings() {
    try {
        auto settings = OpenMagnetics::Settings::GetInstance();
        json settingsJson;
        settingsJson["coilAllowMarginTape"] = settings->get_coil_allow_margin_tape();
        settingsJson["coilAllowInsulatedWire"] = settings->get_coil_allow_insulated_wire();
        settingsJson["coilFillSectionsWithMarginTape"] = settings->get_coil_fill_sections_with_margin_tape();
        settingsJson["coilWindEvenIfNotFit"] = settings->get_coil_wind_even_if_not_fit();
        settingsJson["coilDelimitAndCompact"] = settings->get_coil_delimit_and_compact();
        settingsJson["coilTryRewind"] = settings->get_coil_try_rewind();
        settingsJson["useOnlyCoresInStock"] = settings->get_use_only_cores_in_stock();
        settingsJson["painterNumberPointsX"] = settings->get_painter_number_points_x();
        settingsJson["painterNumberPointsY"] = settings->get_painter_number_points_y();
        settingsJson["painterMode"] = settings->get_painter_mode();
        settingsJson["painterLogarithmicScale"] = settings->get_painter_logarithmic_scale();
        settingsJson["painterIncludeFringing"] = settings->get_painter_include_fringing();
        if (settings->get_painter_maximum_value_colorbar()) {
            settingsJson["painterMaximumValueColorbar"] = settings->get_painter_maximum_value_colorbar();
        }
        if (settings->get_painter_minimum_value_colorbar()) {
            settingsJson["painterMinimumValueColorbar"] = settings->get_painter_minimum_value_colorbar();
        }
        settingsJson["painterColorFerrite"] = settings->get_painter_color_ferrite();
        settingsJson["painterColorBobbin"] = settings->get_painter_color_bobbin();
        settingsJson["painterColorCopper"] = settings->get_painter_color_copper();
        settingsJson["painterColorInsulation"] = settings->get_painter_color_insulation();
        settingsJson["painterColorMargin"] = settings->get_painter_color_margin();
        settingsJson["painterMirroringDimension"] = settings->get_painter_mirroring_dimension();
        settingsJson["magneticFieldNumberPointsX"] = settings->get_magnetic_field_number_points_x();
        settingsJson["magneticFieldNumberPointsY"] = settings->get_magnetic_field_number_points_y();
        settingsJson["magneticFieldIncludeFringing"] = settings->get_magnetic_field_include_fringing();
        settingsJson["magneticFieldMirroringDimension"] = settings->get_magnetic_field_mirroring_dimension();
        return settingsJson.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

void MKFNet::SetSettings(std::string settingsString) {
    auto settings = OpenMagnetics::Settings::GetInstance();
    json settingsJson = json::parse(settingsString);
    settings->set_coil_allow_margin_tape(settingsJson["coilAllowMarginTape"]);
    settings->set_coil_allow_insulated_wire(settingsJson["coilAllowInsulatedWire"]);
    settings->set_coil_fill_sections_with_margin_tape(settingsJson["coilFillSectionsWithMarginTape"]);
    settings->set_coil_wind_even_if_not_fit(settingsJson["coilWindEvenIfNotFit"]);
    settings->set_coil_delimit_and_compact(settingsJson["coilDelimitAndCompact"]);
    settings->set_coil_try_rewind(settingsJson["coilTryRewind"]);
    settings->set_painter_mode(settingsJson["painterMode"]);
    settings->set_use_only_cores_in_stock(settingsJson["useOnlyCoresInStock"]);
    settings->set_painter_number_points_x(settingsJson["painterNumberPointsX"]);
    settings->set_painter_number_points_y(settingsJson["painterNumberPointsY"]);
    settings->set_painter_logarithmic_scale(settingsJson["painterLogarithmicScale"]);
    settings->set_painter_include_fringing(settingsJson["painterIncludeFringing"]);
    if (settingsJson.contains("painterMaximumValueColorbar")) {
        settings->set_painter_maximum_value_colorbar(settingsJson["painterMaximumValueColorbar"]);
    }
    if (settingsJson.contains("painterMinimumValueColorbar")) {
        settings->set_painter_minimum_value_colorbar(settingsJson["painterMinimumValueColorbar"]);
    }
    settings->set_painter_color_ferrite(settingsJson["painterColorFerrite"]);
    settings->set_painter_color_bobbin(settingsJson["painterColorBobbin"]);
    settings->set_painter_color_copper(settingsJson["painterColorCopper"]);
    settings->set_painter_color_insulation(settingsJson["painterColorInsulation"]);
    settings->set_painter_color_margin(settingsJson["painterColorMargin"]);
    settings->set_painter_mirroring_dimension(settingsJson["painterMirroringDimension"]);
    settings->set_magnetic_field_number_points_x(settingsJson["magneticFieldNumberPointsX"]);
    settings->set_magnetic_field_number_points_y(settingsJson["magneticFieldNumberPointsY"]);
    settings->set_magnetic_field_include_fringing(settingsJson["magneticFieldIncludeFringing"]);
    settings->set_magnetic_field_mirroring_dimension(settingsJson["magneticFieldMirroringDimension"]);
}

void MKFNet::ResetSettings() {
    auto settings = OpenMagnetics::Settings::GetInstance();
    settings->reset();
}