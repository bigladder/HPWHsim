/*
 * Measure the performance metrics of a HPWH model
 */

#include <CLI/CLI.hpp>
#include "HPWH.hh"
#include "hpwh-data-model.hh"

namespace hpwh_cli
{

/// measure
void convert(const std::string& specType,
             HPWH& hpwh,
             std::string sOutputDir,
             std::string sOutputFilename);

CLI::App* add_convert(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("convert", "Convert to JSON");

    //
    auto model_group = subcommand->add_option_group("model");

    static std::string modelName = "";
    model_group->add_option("-m,--model", modelName, "Model name");

    static int modelNumber = -1;
    model_group->add_option("-n,--number", modelNumber, "Model number");

    static std::string modelFilename = "";
    model_group->add_option("-f,--filename", modelFilename, "Model filename");

    model_group->required(1);

    //
    static std::string sOutputDir = ".";
    subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

    static std::string sOutputFilename = "";
    subcommand->add_option("-f,--filename", sOutputFilename, "Output filename");

    subcommand->callback(
        [&]()
        {
            HPWH hpwh;
            std::string specType = "Preset";
            if (modelName != "")
                hpwh.initPreset(modelName);
            else if (modelNumber != -1)
                hpwh.initPreset(static_cast<HPWH::MODELS>(modelNumber));
            else if (modelFilename != "")
            {
                specType = "JSON";
                hpwh.initFromJSON(modelFilename);
            }
            convert(specType, hpwh, sOutputDir, sOutputFilename);
        });

    return subcommand;
}

void convert(const std::string& specType,
             HPWH& hpwh,
             std::string sOutputDir,
             std::string sOutputFilename)
{
    hpwh_data_model::hpwh_sim_input::HPWHSimInput hsi;
    hpwh.to(hsi);

    nlohmann::json j;
    hpwh_data_model::hpwh_sim_input::to_json(j, hsi);

    if (sOutputFilename == "")
        sOutputFilename = hpwh.name + "_" + specType + ".json";

    if (sOutputDir != "")
        sOutputFilename = sOutputDir + "/" + sOutputFilename;

    std::ofstream outputFile;
    outputFile.open(sOutputFilename.c_str(), std::ifstream::out);
    if (!outputFile.is_open())
    {
        hpwh.get_courier()->send_error(
            fmt::format("Could not open output file {}\n", sOutputFilename));
    }
    outputFile << j.dump(2);
    outputFile.close();
}

} // namespace hpwh_cli
