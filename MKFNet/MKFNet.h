class MKFNet {
    std::string name;
public:
    MKFNet();
    std::string GetMaterial(std::string materialName);
    std::string CalculateCoreData(std::string coreDataString, bool includeMaterialData);
    std::string Wind(std::string coilString, std::string proportionPerWindingString = "[]", std::string patternString = "[]", size_t repetitions = 1);
    std::string GetDefaultModels(); 
    std::string CalculateCoreLosses(std::string magneticString, std::string inputsData, std::string modelsData);
    std::string CalculateAdvisedCores(std::string inputsString, std::string weightsString, int maximumNumberResults, bool useOnlyCoresInStock);
    std::string CalculateAdvisedMagnetics(std::string inputsString, int maximumNumberResults);
    std::string CalculateWindingLosses(std::string magneticString, std::string operatingPointString, double temperature);
    bool PlotField(std::string magneticString, std::string operatingPointString, std::string outFile);
    bool PlotCore(std::string magneticString, std::string outFile);
    bool PlotSections(std::string magneticString, std::string outFile);
    bool PlotLayers(std::string magneticString, std::string outFile);
    bool PlotTurns(std::string magneticString, std::string outFile);
    std::string GetSettings();
    void SetSettings(std::string settingsString);
    void ResetSettings();
};
