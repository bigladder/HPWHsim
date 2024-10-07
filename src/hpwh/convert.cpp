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
    const auto subcommand = app.add_subcommand("convert", "Convert from legacy format to JSON");

    static std::string sSpecType = "Preset";
    subcommand->add_option("-s,--spec", sSpecType, "Specification type (Preset, File, JSON)");

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
    std::string sSpecType_mod = (sSpecType != "") ? sSpecType : "Preset";
    if (sOutputDir != "")
    {
    }

    for (auto& c : sSpecType_mod)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (sSpecType_mod.length() > 0)
    {
        sSpecType_mod[0] =
            static_cast<char>(std::toupper(static_cast<unsigned char>(sSpecType_mod[0])));
    }

    HPWH hpwh;
    if (sSpecType_mod == "Preset")
    {
        hpwh.initPreset(sModelName);
    }
    else if (sSpecType_mod == "File")
    {
        std::string inputFile = sModelName + ".txt";
        hpwh.initFromFile(inputFile);
    }
    else if (sSpecType_mod == "Json")
    {
        auto inputFile = sModelName + ".json";

        std::ifstream inputFILE(inputFile);
        nlohmann::json j = nlohmann::json::parse(inputFILE);

        HPWH::fromProto(j);
        //std::cout << j.dump(2) << std::endl;

        data_model::init(hpwh.get_courier());
        data_model::rsintegratedwaterheater_ns::RSINTEGRATEDWATERHEATER rswh;
        data_model::rsintegratedwaterheater_ns::from_json(j, rswh);

        hpwh.from(rswh);
    }

    data_model::rsintegratedwaterheater_ns::RSINTEGRATEDWATERHEATER rswh;
    hpwh.to(rswh);

    nlohmann::json j;
    HPWH::to_json(rswh, j);

    std::cout << j.dump(2) << std::endl;

    std::ofstream outputFile;
    std::string outputFilename = sOutputDir + "/" + sModelName + "_" + sSpecType + ".json";
    outputFile.open(outputFilename.c_str(), std::ifstream::out);
    if (!outputFile.is_open())
    {
        std::cout << "Could not open output file " << outputFilename << "\n";
        exit(1);
    }
    outputFile << j.dump(2);
    outputFile.close();
}
} // namespace hpwh_cli
