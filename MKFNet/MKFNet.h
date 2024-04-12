class MKFNet {
    std::string name;
public:
    MKFNet();

    void LoadDatabases(std::string databasesString);
    std::string LoadMas(std::string key, std::string masString, bool expand);
    std::string LoadMagnetic(std::string key, std::string magneticString, std::string inputsString, bool expand);
    std::string LoadMagnetics(std::string keys, std::string magneticsString, std::string inputsString, bool expand);
    std::string LoadMagneticsFromFile(std::string path, std::string inputsString, bool expand);
    std::string ReadMas(std::string key);
    std::string ReadDatabases(std::string path, bool addInternalData);

    std::string FindCoreMaterialByName(std::string materialName);
    std::string FindCoreShapeByName(std::string shapeName);
    std::string FindWireByName(std::string wireName);
    std::string FindBobbinByName(std::string bobbinName);
    std::string FindInsulationMaterialByName(std::string insulationMaterialName);
    std::string FindWireMaterialByName(std::string wireMaterialName);

    std::string GetCoreMaterialNames();
    std::string GetCoreShapeNames();
    std::string GetWireNames();
    std::string GetBobbinNames();
    std::string GetInsulationMaterialNames();
    std::string GetWireMaterialNames();

    std::string GetCoreMaterials();
    std::string GetCoreShapes();
    std::string GetWires();
    std::string GetBobbins();
    std::string GetInsulationMaterials();
    std::string GetWireMaterials();

    std::string CalculateCoreData(std::string coreDataString, bool includeMaterialData);
    std::string Wind(std::string coilString, size_t repetitions = 1, std::string proportionPerWindingString = "[]", std::string patternString = "[]");
    std::string WindBySections(std::string coilString, size_t repetitions = 1, std::string proportionPerWindingString = "[]", std::string patternString = "[]");
    std::string WindByLayers(std::string coilString);
    std::string WindByTurns(std::string coilString);
    std::string DelimitAndCompact(std::string coilString);

    std::string GetDefaultModels(); 
    std::string CalculateCoreLosses(std::string magneticString, std::string inputsData, std::string modelsData);
    std::string CalculateAdvisedCores(std::string inputsString, std::string weightsString, int maximumNumberResults, bool useOnlyCoresInStock);
    std::string CalculateAdvisedMagnetics(std::string inputsString, int maximumNumberResults);
    std::string CalculateWindingLosses(std::string magneticString, std::string operatingPointString, double temperature);
    std::string CalculateCoreProcessedDescription(std::string coreDataString);
    std::string CalculateCoreGeometricalDescription(std::string coreDataString);
    std::string CalculateCoreGapping(std::string coreDataString);
    std::string CalculateOhmicLosses(std::string coilString, std::string operatingPointString, double temperature);
    std::string CalculateSkinEffectLosses(std::string coilString, std::string windingLossesOutputString, double temperature);
    std::string CalculateSkinEffectLossesPerMeter(std::string wireString, std::string currentString, double temperature, double currentDivider = 1);
    std::string CalculateMagneticFieldStrengthField(std::string operatingPointString, std::string magneticString);
    std::string CalculateProximityEffectLosses(std::string coilString, double temperature, std::string windingLossesOutputString, std::string windingWindowMagneticStrengthFieldOutputString);

    std::string CalculateEffectiveCurrentDensity(std::string magneticString, std::string operatingPointString, double temperature);

    double GetOuterDiameterEnameledRound(double conductingDiameter, int grade = 1, std::string standardString = "IEC_60317");
    double GetOuterDiameterInsulatedRound(double conductingDiameter, int numberLayers, double thicknessLayers, std::string standardString = "IEC_60317");
    double GetOuterDiameterServedLitz(double conductingDiameter, int numberConductors, int grade = 1, int numberLayers= 1, std::string standardString = "IEC_60317");
    double GetOuterDiameterInsulatedLitz(double conductingDiameter, int numberConductors, int numberLayers, double thicknessLayers, int grade = 1, std::string standardString = "IEC_60317");
    double GetConductingAreaRectangular(double conductingWidth, double conductingHeight, std::string standardString = "IEC_60317");
    double GetOuterWidthRectangular(double conductingWidth, int grade = 1, std::string standardString = "IEC_60317");
    double GetOuterHeightRectangular(double conductingHeight, int grade = 1, std::string standardString = "IEC_60317");

    double CalculateCoreMaximumMagneticEnergy(std::string coreDataString, std::string operatingPointString);
    double CalculateRequiredMagneticEnergy(std::string inputsString);

    bool PlotField(std::string magneticString, std::string operatingPointString, std::string outFile);
    bool PlotCore(std::string magneticString, std::string outFile);
    bool PlotSections(std::string magneticString, std::string outFile);
    bool PlotLayers(std::string magneticString, std::string outFile);
    bool PlotTurns(std::string magneticString, std::string outFile);
    std::string GetSettings();
    void SetSettings(std::string settingsString);
    void ResetSettings();
};
