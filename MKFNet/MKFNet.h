class MKFNet {
    std::string name;
public:
    MKFNet();
    std::string GetMaterial(std::string materialName);
    std::string CalculateCoreLosses(std::string coreData, std::string coilData, std::string inputsData, std::string modelsData);
};
