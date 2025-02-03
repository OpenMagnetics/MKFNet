#include <iostream>
#include "MKFNet.h"
#include "Constants.h"
#include "Insulation.h"
#include "Defaults.h"
#include <MAS.hpp>
#include "InputsWrapper.h"
#include "CoreWrapper.h"
#include "Reluctance.h"
#include "MagnetizingInductance.h"
#include "MagneticEnergy.h"
#include "MagneticAdviser.h"
#include "MagneticField.h"
#include "MagneticSimulator.h"
#include "MagneticWrapper.h"
#include "MasWrapper.h"
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

std::map<std::string, OpenMagnetics::MasWrapper> masDatabase;

void MKFNet::LoadDatabases(std::string databasesString) {
    json databasesJson = json::parse(databasesString);
    OpenMagnetics::load_databases(databasesJson, true);
}

std::string MKFNet::ReadDatabases(std::string path, bool addInternalData) {
    try {
        auto masPath = std::filesystem::path{path};
        json data;
        std::string line;
        {
            data["coreMaterials"] = json();
            std::ifstream coreMaterials(masPath.append("core_materials.ndjson"));
            while (getline (coreMaterials, line)) {
                json jf = json::parse(line);
                data["coreMaterials"][jf["name"]] = jf;
            }
        }
        {
            data["coreShapes"] = json();
            std::ifstream coreMaterials(masPath.append("core_shapes.ndjson"));
            while (getline (coreMaterials, line)) {
                json jf = json::parse(line);
                data["coreShapes"][jf["name"]] = jf;
            }
        }
        {
            data["wires"] = json();
            std::ifstream coreMaterials(masPath.append("wires.ndjson"));
            while (getline (coreMaterials, line)) {
                json jf = json::parse(line);
                data["wires"][jf["name"]] = jf;
            }
        }
        {
            data["bobbins"] = json();
            std::ifstream coreMaterials(masPath.append("bobbins.ndjson"));
            while (getline (coreMaterials, line)) {
                json jf = json::parse(line);
                data["bobbins"][jf["name"]] = jf;
            }
        }
        {
            data["insulationMaterials"] = json();
            std::ifstream coreMaterials(masPath.append("insulation_materials.ndjson"));
            while (getline (coreMaterials, line)) {
                json jf = json::parse(line);
                data["insulationMaterials"][jf["name"]] = jf;
            }
        }
        {
            data["wireMaterials"] = json();
            std::ifstream coreMaterials(masPath.append("wire_materials.ndjson"));
            while (getline (coreMaterials, line)) {
                json jf = json::parse(line);
                data["wireMaterials"][jf["name"]] = jf;
            }
        }
        OpenMagnetics::load_databases(data, true, addInternalData);
        return "0";
    }
    catch (const std::exception &exc) {
        return std::string{exc.what()};
    }
}

OpenMagnetics::MagneticWrapper expandMagnetic(OpenMagnetics::MagneticWrapper magnetic) {
    auto core = magnetic.get_core();
    auto coil = magnetic.get_coil();
    auto coreMaterial = core.resolve_material();
    core.get_mutable_functional_description().set_material(coreMaterial);

    if (!core.get_processed_description()) {
        core.process_data();
    }

    if (core.get_functional_description().get_gapping().size() > 0 && !core.get_functional_description().get_gapping()[0].get_area()) {
        core.process_gap();
    }

    if (!coil.get_turns_description()) {
        for (size_t windingIndex = 0; windingIndex < coil.get_functional_description().size(); windingIndex++) {
            coil.resolve_wire(windingIndex);
        }
        for (size_t windingIndex = 0; windingIndex < coil.get_functional_description().size(); windingIndex++)
        {
            auto wire = coil.get_wires()[windingIndex];
            if (wire.get_type() == OpenMagnetics::WireType::FOIL) {
                if (!wire.get_conducting_height())
                {
                    auto bobbin = coil.resolve_bobbin();
                    OpenMagnetics::DimensionWithTolerance aux;
                    aux.set_nominal(bobbin.get_processed_description()->get_winding_windows()[0].get_height().value() * 0.8);
                    wire.set_conducting_height(aux);
                }
                if (!wire.get_outer_height())
                {
                    wire.set_outer_height(wire.get_conducting_height().value());
                }
                if (!wire.get_outer_width())
                {
                    wire.set_outer_width(wire.get_conducting_width().value());
                }
            }
            if (wire.get_type() == OpenMagnetics::WireType::RECTANGULAR) {
                if (!wire.get_outer_height())
                {
                    OpenMagnetics::DimensionWithTolerance aux;
                    aux.set_nominal(OpenMagnetics::WireWrapper::get_outer_height_rectangular(OpenMagnetics::resolve_dimensional_values(wire.get_conducting_height().value())));
                    wire.set_outer_height(aux);
                }
                if (!wire.get_outer_width())
                {
                    OpenMagnetics::DimensionWithTolerance aux;
                    aux.set_nominal(OpenMagnetics::WireWrapper::get_outer_height_rectangular(OpenMagnetics::resolve_dimensional_values(wire.get_conducting_width().value())));
                    wire.set_outer_width(aux);
                }
            }
            if (wire.get_type() == OpenMagnetics::WireType::ROUND) {
                if (!wire.get_outer_diameter())
                {
                    auto coating = wire.resolve_coating();
                    if (coating->get_type() == OpenMagnetics::InsulationWireCoatingType::ENAMELLED)
                    {
                        OpenMagnetics::DimensionWithTolerance aux;
                        aux.set_nominal(OpenMagnetics::WireWrapper::get_outer_diameter_round(OpenMagnetics::resolve_dimensional_values(OpenMagnetics::resolve_dimensional_values(wire.get_conducting_diameter().value()))));
                        wire.set_outer_diameter(aux);
                    }
                    
                    if (coating->get_type() == OpenMagnetics::InsulationWireCoatingType::INSULATED)
                    {
                        int numberLayers = coating->get_number_layers().value();
                        int thicknessLayers = coating->get_thickness_layers().value();
                        OpenMagnetics::DimensionWithTolerance aux;
                        aux.set_nominal(OpenMagnetics::WireWrapper::get_outer_diameter_round(OpenMagnetics::resolve_dimensional_values(wire.get_conducting_diameter().value()), numberLayers, thicknessLayers));
                        wire.set_outer_diameter(aux);
                    }
                }
            }
            if (wire.get_type() == OpenMagnetics::WireType::LITZ) {
                if (!wire.get_outer_diameter())
                {
                    auto coating = wire.resolve_coating();
                    auto strand = wire.resolve_strand();
                    if (coating->get_type() == OpenMagnetics::InsulationWireCoatingType::SERVED)
                    {
                        OpenMagnetics::DimensionWithTolerance aux;
                        aux.set_nominal(OpenMagnetics::WireWrapper::get_outer_diameter_served_litz(OpenMagnetics::resolve_dimensional_values(strand.get_conducting_diameter()), wire.get_number_conductors().value()));
                        wire.set_outer_diameter(aux);
                    }
                    
                    if (coating->get_type() == OpenMagnetics::InsulationWireCoatingType::INSULATED)
                    {
                        int numberLayers = coating->get_number_layers().value();
                        int thicknessLayers = coating->get_thickness_layers().value();
                        OpenMagnetics::DimensionWithTolerance aux;
                        aux.set_nominal(OpenMagnetics::WireWrapper::get_outer_diameter_insulated_litz(OpenMagnetics::resolve_dimensional_values(strand.get_conducting_diameter()), wire.get_number_conductors().value(), numberLayers, thicknessLayers));
                    }
                }
            }
            coil.get_mutable_functional_description()[windingIndex].set_wire(wire);
        }
        if (!coil.get_sections_description())
        {
            coil.wind();
        }
        else {
            if (!coil.get_layers_description())
            {
                coil.wind_by_layers();
            }
            if (!coil.get_turns_description())
            {
                coil.wind_by_turns();
                coil.delimit_and_compact();
            }
        }
    }
    magnetic.set_core(core);
    magnetic.set_coil(coil);

    return magnetic;
}

std::string MKFNet::LoadMas(std::string key, std::string masString, bool expand) {
    try {
        json masJson = json::parse(masString);
        OpenMagnetics::MasWrapper mas(masJson);
        if (expand) {
            mas.get_mutable_magnetic() = expandMagnetic(mas.get_mutable_magnetic());
        }
        masDatabase[key] = mas;
        return std::to_string(masDatabase.size());
    }
    catch (const std::exception &exc) {
        return std::string{exc.what()};
    }
}

std::string MKFNet::LoadMagnetic(std::string key, std::string magneticString, std::string inputsString, bool expand) {
    try {
        OpenMagnetics::MagneticWrapper magnetic(json::parse(magneticString));
        OpenMagnetics::InputsWrapper inputs(json::parse(inputsString));
        if (expand) {
            magnetic = expandMagnetic(magnetic);
        }
        OpenMagnetics::MasWrapper mas;
        mas.set_magnetic(magnetic);
        mas.set_inputs(inputs);
        masDatabase[key] = mas;
        return std::to_string(masDatabase.size());
    }
    catch (const std::exception &exc) {
        return std::string{exc.what()};
    }
}


std::string MKFNet::LoadMagnetics(std::string keys, std::string magneticsString, std::string inputsString, bool expand) {
    try {
        json magneticJsons = json::parse(magneticsString);
        json keysJson = json::parse(keys);
        OpenMagnetics::InputsWrapper inputs(json::parse(inputsString));
        for (size_t magneticIndex = 0; magneticIndex < magneticJsons.size(); magneticIndex++) {
            OpenMagnetics::MagneticWrapper magnetic(magneticJsons[magneticIndex]);
            if (expand) {
                magnetic = expandMagnetic(magnetic);
            }
            OpenMagnetics::MasWrapper mas;
            mas.set_magnetic(magnetic);
            mas.set_inputs(inputs);
            masDatabase[keysJson[magneticIndex]] = mas;
        }
        return std::to_string(masDatabase.size());
    }
    catch (const std::exception &exc) {
        return std::string{exc.what()};
    }
}

std::string MKFNet::LoadMagneticsFromFile(std::string path, std::string inputsString, bool expand) {
    try {
        std::ifstream in(path);
        std::vector<std::vector<double>> fields;
        size_t number_read_rows = 0;
        OpenMagnetics::InputsWrapper inputs(json::parse(inputsString));

        if (in) {
            std::string line;

            while (getline(in, line)) {
                std::stringstream sep(line);
                std::string field;

                std::vector<std::string> row_data;

                while (getline(sep, field, ';')) {
                    row_data.push_back(field);
                }

                OpenMagnetics::MagneticWrapper magnetic(json::parse(row_data[1]));
                OpenMagnetics::MagneticManufacturerInfo manufacturerInfo;
                manufacturerInfo.set_name("Wuerth Elektronik");
                manufacturerInfo.set_reference(row_data[0]);
                magnetic.set_manufacturer_info(manufacturerInfo);
                if (expand) {
                    magnetic = expandMagnetic(magnetic);
                }
                OpenMagnetics::MasWrapper mas;
                mas.set_magnetic(magnetic);
                mas.set_inputs(inputs);
                masDatabase[row_data[0]] = mas;
            }
        }
        return std::to_string(masDatabase.size());
    }
    catch (const std::exception &exc) {
        return std::string{exc.what()};
    }

    return "Ea";
}

std::string MKFNet::ReadMas(std::string key) {
    json result;
    to_json(result, masDatabase[key]);
    return result.dump(4);
}

std::string MKFNet::GetCoreMaterials() {
    try {
        auto materials = OpenMagnetics::get_materials(std::nullopt);
        json result = json::array();
        for (auto elem: materials) {
            json aux;
            OpenMagnetics::to_json(aux, elem);
            result.push_back(aux);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}
std::string MKFNet::GetCoreShapes() {
    try {
        auto shapes = OpenMagnetics::get_shapes(true);
        json result = json::array();
        for (auto elem : shapes) {
            json aux;
            OpenMagnetics::to_json(aux, elem);
            result.push_back(aux);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}
std::string MKFNet::GetWires() {
    try {
        auto wires = OpenMagnetics::get_wires();
        json result = json::array();
        for (auto elem : wires) {
            json aux;
            OpenMagnetics::to_json(aux, elem);
            result.push_back(aux);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}
std::string MKFNet::GetBobbins() {
    try {
        auto bobbins = OpenMagnetics::get_bobbins();
        json result = json::array();
        for (auto elem : bobbins) {
            json aux;
            OpenMagnetics::to_json(aux, elem);
            result.push_back(aux);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}
std::string MKFNet::GetInsulationMaterials() {
    try {
        auto insulationMaterials = OpenMagnetics::get_insulation_materials();
        json result = json::array();
        for (auto elem : insulationMaterials) {
            json aux;
            OpenMagnetics::to_json(aux, elem);
            result.push_back(aux);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}
std::string MKFNet::GetWireMaterials() {
    try {
        auto wireMaterials = OpenMagnetics::get_wire_materials();
        json result = json::array();
        for (auto elem : wireMaterials) {
            json aux;
            OpenMagnetics::to_json(aux, elem);
            result.push_back(aux);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::GetCoreMaterialNames() {
    try {
        auto materialNames = OpenMagnetics::get_material_names(std::nullopt);
        json result = json::array();
        for (auto elem : materialNames) {
            result.push_back(elem);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::GetCoreShapeNames() {
    try {
        auto shapeNames = OpenMagnetics::get_shape_names();
        json result = json::array();
        for (auto elem : shapeNames) {
            result.push_back(elem);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::GetWireNames() {
    try {
        auto wireNames = OpenMagnetics::get_wire_names();
        json result = json::array();
        for (auto elem : wireNames) {
            result.push_back(elem);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::GetBobbinNames() {
    try {
        auto bobbinNames = OpenMagnetics::get_bobbin_names();
        json result = json::array();
        for (auto elem : bobbinNames) {
            result.push_back(elem);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::GetInsulationMaterialNames() {
    try {
        auto insulationMaterialNames = OpenMagnetics::get_insulation_material_names();
        json result = json::array();
        for (auto elem : insulationMaterialNames) {
            result.push_back(elem);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::GetWireMaterialNames() {
    try {
        auto wireMaterialNames = OpenMagnetics::get_wire_material_names();
        json result = json::array();
        for (auto elem : wireMaterialNames) {
            result.push_back(elem);
        }
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::FindCoreMaterialByName(std::string materialName) {
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

std::string MKFNet::FindCoreShapeByName(std::string shapeName) {
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

std::string MKFNet::FindWireByName(std::string wireName) {
    try {
        auto wireData = OpenMagnetics::find_wire_by_name(wireName);
        json result;
        to_json(result, wireData);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::FindBobbinByName(std::string bobbinName) {
    try {
        auto bobbinData = OpenMagnetics::find_bobbin_by_name(bobbinName);
        json result;
        to_json(result, bobbinData);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::FindInsulationMaterialByName(std::string insulationMaterialName) {
    try {
        auto insulationMaterialData = OpenMagnetics::find_insulation_material_by_name(insulationMaterialName);
        json result;
        to_json(result, insulationMaterialData);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::FindWireMaterialByName(std::string wireMaterialName) {
    try {
        auto wireMaterialData = OpenMagnetics::find_wire_material_by_name(wireMaterialName);
        json result;
        to_json(result, wireMaterialData);
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

std::string MKFNet::WindBySections(std::string coilString, size_t repetitions, std::string proportionPerWindingString, std::string patternString) {
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
                coil.wind_by_sections(proportionPerWinding, pattern, repetitions);
            }
            else if (repetitions > 0) {
                coil.wind_by_sections(repetitions);
            }
            else {
                coil.wind_by_sections();
            }
        }
        else {
            if (pattern.size() > 0 && repetitions > 0) {
                coil.wind_by_sections(pattern, repetitions);
            }
            else if (repetitions > 0) {
                coil.wind_by_sections(repetitions);
            }
            else {
                coil.wind_by_sections();
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

std::string MKFNet::WindByLayers(std::string coilString) {
    try {
        auto coilJson = json::parse(coilString);

        auto coilFunctionalDescription = std::vector<OpenMagnetics::CoilFunctionalDescription>(coilJson["functionalDescription"]);
        auto coilSectionsDescription = std::vector<OpenMagnetics::Section>(coilJson["sectionsDescription"]);
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
        coil.set_sections_description(coilSectionsDescription);
        coil.wind_by_layers();

        json result;
        to_json(result, coil);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::WindByTurns(std::string coilString) {
    try {
        auto coilJson = json::parse(coilString);

        auto coilFunctionalDescription = std::vector<OpenMagnetics::CoilFunctionalDescription>(coilJson["functionalDescription"]);
        auto coilSectionsDescription = std::vector<OpenMagnetics::Section>(coilJson["sectionsDescription"]);
        auto coilLayersDescription = std::vector<OpenMagnetics::Layer>(coilJson["layersDescription"]);
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
        coil.set_sections_description(coilSectionsDescription);
        coil.set_layers_description(coilLayersDescription);
        coil.wind_by_turns();

        json result;
        to_json(result, coil);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}


std::string MKFNet::DelimitAndCompact(std::string coilString) {
    try {
        auto coilJson = json::parse(coilString);

        auto coilFunctionalDescription = std::vector<OpenMagnetics::CoilFunctionalDescription>(coilJson["functionalDescription"]);
        auto coilSectionsDescription = std::vector<OpenMagnetics::Section>(coilJson["sectionsDescription"]);
        auto coilLayersDescription = std::vector<OpenMagnetics::Layer>(coilJson["layersDescription"]);
        auto coilTurnsDescription = std::vector<OpenMagnetics::Turn>(coilJson["turnsDescription"]);
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
        coil.set_sections_description(coilSectionsDescription);
        coil.set_layers_description(coilLayersDescription);
        coil.set_turns_description(coilTurnsDescription);
        coil.delimit_and_compact();

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
        OpenMagnetics::MagneticWrapper magnetic;
        OpenMagnetics::InputsWrapper inputs;
        OpenMagnetics::OperatingPoint operatingPoint;
        if (magneticString.starts_with("{")) {
            magnetic = OpenMagnetics::MagneticWrapper(json::parse(magneticString));
        }
        else {
            magnetic = masDatabase[magneticString].get_magnetic();
        }
        if (inputsString.starts_with("{")) {
            inputs = OpenMagnetics::InputsWrapper(json::parse(inputsString));
            operatingPoint = inputs.get_operating_point(0);
        }
        else {
            size_t operatingPointIndex = stoi(inputsString);
            inputs = masDatabase[magneticString].get_inputs();
            operatingPoint = masDatabase[magneticString].get_inputs().get_operating_points()[operatingPointIndex];
        }

        OpenMagnetics::CoreWrapper core = magnetic.get_core();
        OpenMagnetics::CoilWrapper coil = magnetic.get_coil();
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
        auto coreLossesOutput = magneticSimulator.calculate_core_losses(operatingPoint, magnetic);
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

        OpenMagnetics::CoreAdviser coreAdviser;
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
        for (auto& [masMagnetic, scoring] : masMagnetics) {
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

std::string MKFNet::CalculateWindingLosses(std::string magneticString, std::string operatingPointString, double temperature, double windingLossesHarmonicAmplitudeThreshold) {
    try {
        auto settings = OpenMagnetics::Settings::GetInstance();
        OpenMagnetics::MagneticWrapper magnetic;
        OpenMagnetics::OperatingPoint operatingPoint;
        if (magneticString.starts_with("{")) {
            magnetic = OpenMagnetics::MagneticWrapper(json::parse(magneticString));
        }
        else {
            magnetic = masDatabase[magneticString].get_magnetic();
        }
        if (operatingPointString.starts_with("{")) {
            operatingPoint = OpenMagnetics::OperatingPoint(json::parse(operatingPointString));
        }
        else {
            size_t operatingPointIndex = stoi(operatingPointString);
            operatingPoint = masDatabase[magneticString].get_inputs().get_operating_points()[operatingPointIndex];
        }

        auto windingLossesModel = OpenMagnetics::WindingLosses(); 
        settings->set_harmonic_amplitude_threshold(windingLossesHarmonicAmplitudeThreshold);
        auto windingLossesOutput = windingLossesModel.calculate_losses(magnetic, operatingPoint, temperature);

        json result;
        to_json(result, windingLossesOutput);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}

std::string MKFNet::CalculateEffectiveCurrentDensity(std::string magneticString, std::string operatingPointString, double temperature) {
    try {

        OpenMagnetics::MagneticWrapper magnetic;
        OpenMagnetics::OperatingPoint operatingPoint;
        if (magneticString.starts_with("{")) {
            magnetic = OpenMagnetics::MagneticWrapper(json::parse(magneticString));
        }
        else {
            magnetic = masDatabase[magneticString].get_magnetic();
        }
        if (operatingPointString.starts_with("{")) {
            operatingPoint = OpenMagnetics::OperatingPoint(json::parse(operatingPointString));
        }
        else {
            size_t operatingPointIndex = stoi(operatingPointString);
            operatingPoint = masDatabase[magneticString].get_inputs().get_operating_points()[operatingPointIndex];
        }

        auto wires = magnetic.get_mutable_coil().get_wires();
        std::vector<double> effectiveCurrentDensityPerWire;
        double frequency = 1;
        json result = json::array();
        for (size_t windingIndex = 0; windingIndex < wires.size(); windingIndex++) {
            auto wire = wires[windingIndex];
            double rms = operatingPoint.get_mutable_excitations_per_winding()[windingIndex].get_current().value().get_processed().value().get_rms().value();
            // effectiveCurrentDensityPerWire.push_back(calculate_effective_current_density(rms, frequency, temperature));
            double effectiveCurrentDensity = wire.calculate_effective_current_density(rms, frequency, temperature);
            result.push_back(std::to_string(effectiveCurrentDensity));
        }
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

double MKFNet::GetOuterDiameterEnameledRound(double conductingDiameter, int grade, std::string standardString) {
    try {
        OpenMagnetics::WireStandard standard;
        if (bool(magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString))) {
            standard = magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString).value();
        }
        else {
            from_json(standardString, standard);
        }

        auto outerDiameter = OpenMagnetics::WireWrapper::get_outer_diameter_round(conductingDiameter, grade, standard);

        return outerDiameter;
    }
    catch (const std::exception &exc) {
        return -1;
    }
}
double MKFNet::GetOuterDiameterInsulatedRound(double conductingDiameter, int numberLayers, double thicknessLayers, std::string standardString){
    try {
        OpenMagnetics::WireStandard standard;
        if (bool(magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString))) {
            standard = magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString).value();
        }
        else {
            from_json(standardString, standard);
        }

        auto outerDiameter = OpenMagnetics::WireWrapper::get_outer_diameter_round(conductingDiameter, numberLayers, thicknessLayers, standard);

        return outerDiameter;
    }
    catch (const std::exception &exc) {
        return -1;
    }
}
double MKFNet::GetOuterDiameterServedLitz(double conductingDiameter, int numberConductors, int grade, int numberLayers, std::string standardString){
    try {
        OpenMagnetics::WireStandard standard;
        if (bool(magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString))) {
            standard = magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString).value();
        }
        else {
            from_json(standardString, standard);
        }

        auto outerDiameter = OpenMagnetics::WireWrapper::get_outer_diameter_served_litz(conductingDiameter, numberConductors, grade, numberLayers, standard);

        return outerDiameter;
    }
    catch (const std::exception &exc) {
        return -1;
    }
}
double MKFNet::GetOuterDiameterInsulatedLitz(double conductingDiameter, int numberConductors, int numberLayers, double thicknessLayers, int grade, std::string standardString){
    try {
        OpenMagnetics::WireStandard standard;
        if (bool(magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString))) {
            standard = magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString).value();
        }
        else {
            from_json(standardString, standard);
        }

        auto outerDiameter = OpenMagnetics::WireWrapper::get_outer_diameter_insulated_litz(conductingDiameter, numberConductors, numberLayers, thicknessLayers, grade, standard);

        return outerDiameter;
    }
    catch (const std::exception &exc) {
        return -1;
    }
}
double MKFNet::GetConductingAreaRectangular(double conductingWidth, double conductingHeight, std::string standardString){
    try {
        OpenMagnetics::WireStandard standard;
        if (bool(magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString))) {
            standard = magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString).value();
        }
        else {
            from_json(standardString, standard);
        }

        auto outerDiameter = OpenMagnetics::WireWrapper::get_conducting_area_rectangular(conductingWidth, conductingHeight, standard);

        return outerDiameter;
    }
    catch (const std::exception &exc) {
        return -1;
    }
}

double MKFNet::GetOuterWidthRectangular(double conductingWidth, int grade, std::string standardString){
    try {
        OpenMagnetics::WireStandard standard;
        if (bool(magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString))) {
            standard = magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString).value();
        }
        else {
            from_json(standardString, standard);
        }

        auto outerDiameter = OpenMagnetics::WireWrapper::get_outer_width_rectangular(conductingWidth, grade, standard);

        return outerDiameter;
    }
    catch (const std::exception &exc) {
        return -1;
    }
}

double MKFNet::GetOuterHeightRectangular(double conductingHeight, int grade, std::string standardString){
    try {
        OpenMagnetics::WireStandard standard;
        if (bool(magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString))) {
            standard = magic_enum::enum_cast<OpenMagnetics::WireStandard>(standardString).value();
        }
        else {
            from_json(standardString, standard);
        }

        auto outerDiameter = OpenMagnetics::WireWrapper::get_outer_height_rectangular(conductingHeight, grade, standard);

        return outerDiameter;
    }
    catch (const std::exception &exc) {
        return -1;
    }
}

double MKFNet::CalculateCoreMaximumMagneticEnergy(std::string coreDataString, std::string operatingPointString){
    try {
        OpenMagnetics::CoreWrapper core;
        OpenMagnetics::OperatingPoint operatingPoint;
        if (coreDataString.starts_with("{")) {
            core = OpenMagnetics::CoreWrapper(json::parse(coreDataString), false, false, false);
            if (!core.get_processed_description()) {
                core.process_data();
                core.process_gap();
            }
        }
        else {
            core = masDatabase[coreDataString].get_magnetic().get_core();
        }
        if (operatingPointString.starts_with("{")) {
            operatingPoint = OpenMagnetics::OperatingPoint(json::parse(operatingPointString));
        }
        else {
            size_t operatingPointIndex = stoi(operatingPointString);
            operatingPoint = masDatabase[coreDataString].get_inputs().get_operating_points()[operatingPointIndex];
        }
        auto magneticEnergy = OpenMagnetics::MagneticEnergy({});

        double coreMaximumMagneticEnergy;
        if (operatingPoint.get_excitations_per_winding().size() == 0) {
            coreMaximumMagneticEnergy = magneticEnergy.calculate_core_maximum_magnetic_energy(core, nullptr);
        }
        else {
            coreMaximumMagneticEnergy = magneticEnergy.calculate_core_maximum_magnetic_energy(core, &operatingPoint);
        }
        return coreMaximumMagneticEnergy;
    }
    catch (const std::exception &exc) {
        std::cout << "Exception: " + std::string{exc.what()} << std::endl;
        return -1;
    }
}

double MKFNet::CalculateRequiredMagneticEnergy(std::string inputsString){
    try {
        OpenMagnetics::InputsWrapper inputs(json::parse(inputsString));
        auto magneticEnergy = OpenMagnetics::MagneticEnergy({});
        auto requiredMagneticEnergy = magneticEnergy.calculate_required_magnetic_energy(inputs);
        return OpenMagnetics::resolve_dimensional_values(requiredMagneticEnergy);
    }
    catch (const std::exception &exc) {
        std::cout << "Exception: " + std::string{exc.what()} << std::endl;
        return false;
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
    try {
        auto settings = OpenMagnetics::Settings::GetInstance();
        json settingsJson = json::parse(settingsString);
        settings->set_coil_allow_margin_tape(settingsJson["coilAllowMarginTape"] == 1);
        settings->set_coil_allow_insulated_wire(settingsJson["coilAllowInsulatedWire"] == 1);
        settings->set_coil_fill_sections_with_margin_tape(settingsJson["coilFillSectionsWithMarginTape"] == 1);
        settings->set_coil_wind_even_if_not_fit(settingsJson["coilWindEvenIfNotFit"] == 1);
        settings->set_coil_delimit_and_compact(settingsJson["coilDelimitAndCompact"] == 1);
        settings->set_coil_try_rewind(settingsJson["coilTryRewind"] == 1);
        settings->set_painter_mode(settingsJson["painterMode"]);
        settings->set_use_only_cores_in_stock(settingsJson["useOnlyCoresInStock"] == 1);
        settings->set_painter_number_points_x(settingsJson["painterNumberPointsX"]);
        settings->set_painter_number_points_y(settingsJson["painterNumberPointsY"]);
        settings->set_painter_logarithmic_scale(settingsJson["painterLogarithmicScale"] == 1);
        settings->set_painter_include_fringing(settingsJson["painterIncludeFringing"] == 1);
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
        settings->set_magnetic_field_include_fringing(settingsJson["magneticFieldIncludeFringing"] == 1);
        settings->set_magnetic_field_mirroring_dimension(settingsJson["magneticFieldMirroringDimension"] == 1);
    }
    catch (const std::exception &exc) {
        std::cout << std::string{exc.what()} << std::endl;
    }
}

void MKFNet::ResetSettings() {
    auto settings = OpenMagnetics::Settings::GetInstance();
    settings->reset();
}



std::string MKFNet::CalculateInductanceAndMagneticFluxDensity(std::string coreData, std::string coilData, std::string operatingPointData, std::string modelsData){
    try {
        OpenMagnetics::CoreWrapper core(json::parse(coreData));
        OpenMagnetics::CoilWrapper coil(json::parse(coilData));
        OpenMagnetics::OperatingPoint operatingPoint(json::parse(operatingPointData));

        std::map<std::string, std::string> models = json::parse(modelsData).get<std::map<std::string, std::string>>();

        auto reluctanceModelName = OpenMagnetics::Defaults().reluctanceModelDefault;
        if (models.find("reluctance") != models.end()) {
            std::string modelNameStringUpper = models["reluctance"];
            std::transform(modelNameStringUpper.begin(), modelNameStringUpper.end(), modelNameStringUpper.begin(), ::toupper);
            reluctanceModelName = magic_enum::enum_cast<OpenMagnetics::ReluctanceModels>(modelNameStringUpper).value();
        }

        OpenMagnetics::MagnetizingInductance magnetizing_inductance(reluctanceModelName);
        auto magnetizingInductanceAndMagneticFluxDensity = magnetizing_inductance.calculate_inductance_and_magnetic_flux_density(core, coil, &operatingPoint);

        json result = json::array();
        json magnetizingInductanceJson;
        json magneticFluxDensityJson;
        to_json(magnetizingInductanceJson, magnetizingInductanceAndMagneticFluxDensity.first);
        to_json(magneticFluxDensityJson, magnetizingInductanceAndMagneticFluxDensity.second);
        result.push_back(magnetizingInductanceJson);
        result.push_back(magneticFluxDensityJson);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}


std::string MKFNet::CalculateInductanceFromNumberTurnsAndGapping(std::string coreData, std::string coilData, std::string operatingPointData, std::string modelsData){
    try {
        OpenMagnetics::CoreWrapper core(json::parse(coreData));
        OpenMagnetics::CoilWrapper coil(json::parse(coilData));

        std::map<std::string, std::string> models = json::parse(modelsData).get<std::map<std::string, std::string>>();

        auto reluctanceModelName = OpenMagnetics::Defaults().reluctanceModelDefault;
        if (models.find("reluctance") != models.end()) {
            std::string modelNameStringUpper = models["reluctance"];
            std::transform(modelNameStringUpper.begin(), modelNameStringUpper.end(), modelNameStringUpper.begin(), ::toupper);
            reluctanceModelName = magic_enum::enum_cast<OpenMagnetics::ReluctanceModels>(modelNameStringUpper).value();
        }

        OpenMagnetics::MagnetizingInductance magnetizing_inductance(reluctanceModelName);

        OpenMagnetics::MagnetizingInductanceOutput magnetizingInductanceOutput;
        if (operatingPointData != "null") {
            OpenMagnetics::OperatingPoint operatingPoint(json::parse(operatingPointData));
            if (operatingPoint.get_excitations_per_winding().size() == 0) {
                magnetizingInductanceOutput = magnetizing_inductance.calculate_inductance_from_number_turns_and_gapping(core, coil, nullptr);
            }
            else {
                magnetizingInductanceOutput = magnetizing_inductance.calculate_inductance_from_number_turns_and_gapping(core, coil, &operatingPoint);
            }
        }
        else {
            magnetizingInductanceOutput = magnetizing_inductance.calculate_inductance_from_number_turns_and_gapping(core, coil, nullptr);
        }

        json result;
        to_json(result, magnetizingInductanceOutput);
        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}


int MKFNet::CalculateNumberTurnsFromGappingAndInductance(std::string coreData, std::string inputsData, std::string modelsData){
    try {
        OpenMagnetics::CoreWrapper core(json::parse(coreData));
        OpenMagnetics::InputsWrapper inputs(json::parse(inputsData));

        std::map<std::string, std::string> models = json::parse(modelsData).get<std::map<std::string, std::string>>();
        
        auto reluctanceModelName = OpenMagnetics::Defaults().reluctanceModelDefault;
        if (models.find("reluctance") != models.end()) {
            std::string modelNameStringUpper = models["reluctance"];
            std::transform(modelNameStringUpper.begin(), modelNameStringUpper.end(), modelNameStringUpper.begin(), ::toupper);
            reluctanceModelName = magic_enum::enum_cast<OpenMagnetics::ReluctanceModels>(modelNameStringUpper).value();
        }

        OpenMagnetics::MagnetizingInductance magnetizing_inductance(reluctanceModelName);
        int numberTurns = magnetizing_inductance.calculate_number_turns_from_gapping_and_inductance(core, &inputs);

        return numberTurns;
    }
    catch (const std::exception &exc) {
        return -1;
    }
}


std::string MKFNet::CalculateGappingFromNumberTurnsAndInductance(std::string coreData, std::string coilData, std::string inputsData, std::string gappingTypeString, int decimals, std::string modelsData){
    try {
        OpenMagnetics::CoreWrapper core(json::parse(coreData));
        OpenMagnetics::CoilWrapper coil(json::parse(coilData));
        json inputsJson = json::parse(inputsData);
        OpenMagnetics::InputsWrapper inputs;
        OpenMagnetics::from_json(inputsJson, inputs);

        std::transform(gappingTypeString.begin(), gappingTypeString.end(), gappingTypeString.begin(), ::toupper);

        std::map<std::string, std::string> models = json::parse(modelsData).get<std::map<std::string, std::string>>();
        OpenMagnetics::GappingType gappingType = magic_enum::enum_cast<OpenMagnetics::GappingType>(gappingTypeString).value();
        
        auto reluctanceModelName = OpenMagnetics::Defaults().reluctanceModelDefault;
        if (models.find("reluctance") != models.end()) {
            std::string modelNameStringUpper = models["reluctance"];
            std::transform(modelNameStringUpper.begin(), modelNameStringUpper.end(), modelNameStringUpper.begin(), ::toupper);
            reluctanceModelName = magic_enum::enum_cast<OpenMagnetics::ReluctanceModels>(modelNameStringUpper).value();
        }

        OpenMagnetics::MagnetizingInductance magnetizing_inductance(reluctanceModelName);
        std::vector<OpenMagnetics::CoreGap> gapping = magnetizing_inductance.calculate_gapping_from_number_turns_and_inductance(core,
                                                                                                           coil,
                                                                                                           &inputs,
                                                                                                           gappingType,
                                                                                                           decimals);

        core.set_processed_description(std::nullopt);
        core.set_geometrical_description(std::nullopt);
        core.get_mutable_functional_description().set_gapping(gapping);
        core.process_data();
        core.process_gap();
        auto geometricalDescription = core.create_geometrical_description();
        core.set_geometrical_description(geometricalDescription);

        json result = json::array();
        for (auto gap : core.get_functional_description().get_gapping()) {
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



std::string MKFNet::Simulate(std::string inputsString, std::string magneticString, std::string modelsData){
    try {
        OpenMagnetics::MagneticWrapper magnetic(json::parse(magneticString));
        OpenMagnetics::InputsWrapper inputs(json::parse(inputsString));

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

        OpenMagnetics::MagneticSimulator magneticSimulator;

        magneticSimulator.set_core_losses_model_name(coreLossesModelName);
        magneticSimulator.set_core_temperature_model_name(coreTemperatureModelName);
        magneticSimulator.set_reluctance_model_name(reluctanceModelName);
        auto mas = magneticSimulator.simulate(inputs, magnetic);

        json result;
        to_json(result, mas);

        return result.dump(4);
    }
    catch (const std::exception &exc) {
        return "Exception: " + std::string{exc.what()};
    }
}


std::string MKFNet::CalculateProcessed(std::string harmonicsString, std::string waveformString) {
    OpenMagnetics::Waveform waveform;
    OpenMagnetics::Harmonics harmonics;
    OpenMagnetics::from_json(json::parse(waveformString), waveform);
    OpenMagnetics::from_json(json::parse(harmonicsString), harmonics);

    auto processed = OpenMagnetics::InputsWrapper::calculate_processed_data(harmonics, waveform, true);

    json result;
    to_json(result, processed);
    return result.dump(4);
}


std::string MKFNet::CalculateHarmonics(std::string waveformString, double frequency) {
    OpenMagnetics::Waveform waveform;
    OpenMagnetics::from_json(json::parse(waveformString), waveform);

    auto sampledCurrentWaveform = OpenMagnetics::InputsWrapper::calculate_sampled_waveform(waveform, frequency);
    auto harmonics = OpenMagnetics::InputsWrapper::calculate_harmonics_data(sampledCurrentWaveform, frequency);

    json result;
    to_json(result, harmonics);
    return result.dump(4);
}
