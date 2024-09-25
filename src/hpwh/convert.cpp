/*
 * Measure the performance metrics of a HPWH model
 */

#include <CLI/CLI.hpp>
#include "HPWH.hh"
#include "data-model.h"

namespace hpwh_cli
{

/// measure
static void
convert(const std::string& sSpecType, const std::string& sModelName, std::string sOutputDir);

CLI::App* add_convert(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("measure", "Measure the metrics for a model");

    static std::string sSpecType = "Preset";
    subcommand->add_option("-s,--spec", sSpecType, "Specification type (Preset, File)");

    static std::string sModelName = "";
    subcommand->add_option("-m,--model", sModelName, "Model name")->required();

    static std::string sOutputDir = ".";
    subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

    subcommand->callback([&]() { convert(sSpecType, sModelName, sOutputDir); });

    return subcommand;
}

void convert(const std::string& sSpecType, const std::string& sModelName, std::string sOutputDir)
{

    // process command line arguments
    std::string sPresetOrFile = (sSpecType != "") ? sSpecType : "Preset";
    if (sOutputDir != "")
    {
    }

    for (auto& c : sPresetOrFile)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (sPresetOrFile.length() > 0)
    {
        sPresetOrFile[0] =
            static_cast<char>(std::toupper(static_cast<unsigned char>(sPresetOrFile[0])));
    }

    HPWH hpwh;
    if (sPresetOrFile == "Preset")
    {
        hpwh.initPreset(sModelName);
    }
    else
    {
        std::string inputFile = sModelName + ".txt";
        hpwh.initFromFile(inputFile);
    }

    data_model::rsintegratedwaterheater_ns::RSINTEGRATEDWATERHEATER rswh;
    hpwh.to(rswh);

    nlohmann::json j;
    HPWH::to_json(rswh, j);
}
} // namespace hpwh_cli
