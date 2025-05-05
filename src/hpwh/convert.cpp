/*
 * Measure the performance metrics of a HPWH model
 */

#include <CLI/CLI.hpp>
#include "HPWH.hh"
#include "hpwh-data-model.hh"

namespace hpwh_cli
{

/// measure
static void convert(const std::string& sSpecType,
                    const std::string& modelName,
                    bool useModelNumber,
                    std::string sOutputDir,
                    std::string sOutputFilename);

CLI::App* add_convert(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("convert", "Convert from legacy format to JSON");

    static std::string sSpecType = "Preset";
    subcommand->add_option("-s,--spec", sSpecType, "Specification type (Preset, JSON)");

    static std::string modelNameNum = "";
    subcommand->add_option("-m,--model", modelNameNum, "Model name or number")->required();

    static bool use_model_number = false;
    subcommand->add_flag(
        "-n,--number", use_model_number, "Identify by model number, instead of name");

    static std::string sOutputDir = ".";
    subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

    static std::string sOutputFilename = "";
    subcommand->add_option("-f,--filename", sOutputFilename, "Output filename");

    subcommand->callback(
        [&]() { convert(sSpecType, modelNameNum, use_model_number, sOutputDir, sOutputFilename); });

    return subcommand;
}

void convert(const std::string& sSpecType,
             const std::string& modelNameNum,
             bool useModelNumber,
             std::string sOutputDir,
             std::string sOutputFilename)
{
    // process command line arguments
    std::string sSpecType_mod = (sSpecType != "") ? sSpecType : "Preset";
    for (auto& c : sSpecType_mod)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (sSpecType_mod == "preset")
        sSpecType_mod = "Preset";
    else if (sSpecType_mod == "json")
        sSpecType_mod = "JSON";

    auto model_name = modelNameNum;
    if (useModelNumber)
        if (mapNameToPreset(modelName, presetNum))
            model_name = HPWH hpwh;
    if (sSpecType_mod == "Preset")
    {
        if (useModelNumber)
            hpwh.initPreset(static_cast<HPWH::MODELS>(std::stoi(modelNameNum)));
        else
            hpwh.initPreset(modelNameNum);
    }
    else if (sSpecType_mod == "JSON")
    {
        if (useModelNumber)
            hpwh.initFromJSON(static_cast<HPWH::MODELS>(std::stoi(modelNameNum)));
        else
            hpwh.initFromJSON(modelNameNum);
    }

    hpwh_data_model::hpwh_sim_input::HPWHSimInput hsi;
    hpwh.to(hsi);

    nlohmann::json j;
    add_to_json(hsi, j);

    std::ofstream outputFile;
    if (sOutputFilename == "")
        sOutputFilename = modelName + "_" + sSpecType;
    sOutputFilename = sOutputDir + "/" + sOutputFilename + ".json";
    outputFile.open(sOutputFilename.c_str(), std::ifstream::out);
    if (!outputFile.is_open())
    {
        std::cout << "Could not open output file " << sOutputFilename << "\n";
        exit(1);
    }
    outputFile << j.dump(2);
    outputFile.close();
}
} // namespace hpwh_cli
