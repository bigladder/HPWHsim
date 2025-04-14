/*
 * Measure the performance metrics of a HPWH model
 */

#include <CLI/CLI.hpp>
#include "HPWH.hh"
#include "hpwh-data-model.h"

namespace hpwh_cli
{

/// measure
static void convert(const std::string& sSpecType,
                    const std::string& sModelName,
                    bool useModelNumber,
                    std::string sOutputDir,
                    std::string sOutputFilename);

CLI::App* add_convert(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("convert", "Convert from legacy format to JSON");

    static std::string sSpecType = "Preset";
    subcommand->add_option("-s,--spec", sSpecType, "Specification type (Preset, File, JSON)");

    static std::string sModelName = "";
    subcommand->add_option("-m,--model", sModelName, "Model name or number")->required();

    static bool use_model_number = false;
    subcommand->add_flag(
        "-n,--number", use_model_number, "Identify by model number, instead of name");

    static std::string sOutputDir = ".";
    subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

    static std::string sOutputFilename = "";
    subcommand->add_option("-f,--filename", sOutputFilename, "Output filename");

    subcommand->callback(
        [&]() { convert(sSpecType, sModelName, use_model_number, sOutputDir, sOutputFilename); });

    return subcommand;
}

void convert(const std::string& sSpecType,
             const std::string& sModelName,
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
    else if (sSpecType_mod == "file")
        sSpecType_mod = "File";
    else if (sSpecType_mod == "json")
        sSpecType_mod = "JSON";

    HPWH hpwh;
    if (sSpecType_mod == "Preset")
    {
        if (useModelNumber)
            hpwh.initPreset(static_cast<HPWH::MODELS>(std::stoi(sModelName)));
        else
            hpwh.initPreset(sModelName);
    }
    else if (sSpecType_mod == "File")
    {
        hpwh.initFromFile(sModelName);
    }
    else if (sSpecType_mod == "JSON")
    {
        hpwh.initFromJSON(sModelName);
    }

    hpwh_data_model::hpwh_sim_input::HPWHSimInput hsi;
    hpwh.to(hsi);

    nlohmann::json j;
    ::to_json(hsi, j);

    std::ofstream outputFile;
    if (sOutputFilename == "")
        sOutputFilename = sModelName + "_" + sSpecType;
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
