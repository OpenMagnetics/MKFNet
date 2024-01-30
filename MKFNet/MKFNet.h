class MKFNet {
    std::string name;
public:
    MKFNet();
    std::string GetMaterial(std::string materialName);
    std::string GetShape(std::string shapeName);
    std::string CalculateCoreData(std::string coreDataString, bool includeMaterialData);
    std::string Wind(std::string coilString, size_t repetitions = 1, std::string proportionPerWindingString = "[]", std::string patternString = "[]");
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

    bool PlotField(std::string magneticString, std::string operatingPointString, std::string outFile);
    bool PlotCore(std::string magneticString, std::string outFile);
    bool PlotSections(std::string magneticString, std::string outFile);
    bool PlotLayers(std::string magneticString, std::string outFile);
    bool PlotTurns(std::string magneticString, std::string outFile);
    std::string GetSettings();
    void SetSettings(std::string settingsString);
    void ResetSettings();
};
