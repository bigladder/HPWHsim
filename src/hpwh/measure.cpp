/*
 * Measure the performance metrics of a HPWH model
 */

#include <CLI/CLI.hpp>
#include "HPWH.hh"

namespace hpwh_cli
{

/// measure
static void measure(const std::string& sSpecType,
                    const std::string& sModelName,
                    std::string sOutputDir,
                    bool sSupressOutput,
                    std::string sResultsFilename,
                    std::string sCustomDrawProfile);

CLI::App* add_measure(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("measure", "Measure the metrics for a model");

    static std::string sSpecType = "Preset";
    subcommand->add_option("-s,--spec", sSpecType, "Specification type (Preset, File)");

    static std::string sModelName = "";
    subcommand->add_option("-m,--model", sModelName, "Model name")->required();

    static std::string sOutputDir = ".";
    subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

    static bool noData = false;
    subcommand->add_flag("-n,--no_data", noData, "Suppress data output");

    static std::string sResultsFilename = "";
    subcommand->add_option("-r,--results", sResultsFilename, "Results filename");

    static std::string sCustomDrawProfile = "";
    subcommand->add_option("-p,--profile", sCustomDrawProfile, "Custom draw profile");

    subcommand->callback(
        [&]() {
            measure(
                sSpecType, sModelName, sOutputDir, noData, sResultsFilename, sCustomDrawProfile);
        });

    return subcommand;
}

void measure(const std::string& sSpecType,
             const std::string& sModelName,
             std::string sOutputDir,
             bool sSupressOutput,
             std::string sResultsFilename,
             std::string sCustomDrawProfile)
{
    HPWH::StandardTestSummary standardTestSummary;

    HPWH::StandardTestOptions standardTestOptions;
    standardTestOptions.saveOutput = false;
    standardTestOptions.sOutputFilename = "";
    standardTestOptions.sOutputDirectory = "";
    standardTestOptions.j_results = {};
    standardTestOptions.changeSetpoint = true;
    standardTestOptions.nTestTCouples = 6;
    standardTestOptions.setpointT_C = 51.7;

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

    if (sOutputDir != "")
    {
        standardTestOptions.saveOutput = true;
        standardTestOptions.sOutputDirectory = sOutputDir;
    }
    standardTestOptions.saveOutput = !sSupressOutput;
    bool useCustomDrawProfile = (sCustomDrawProfile != "");

    HPWH hpwh;
    if (sSpecType_mod == "Preset")
    {
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
    else
    {
        std::cout << "Invalid argument, received '" << sSpecType_mod
                  << "', expected 'Preset', 'File', or 'JSON'.\n";
        exit(1);
    }

    if (useCustomDrawProfile)
    {
        bool foundProfile = false;
        sCustomDrawProfile.erase(
            std::remove(sCustomDrawProfile.begin(), sCustomDrawProfile.end(), ' '),
            sCustomDrawProfile.end());
        for (auto& c : sCustomDrawProfile)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        for (const auto& [key, value] : HPWH::FirstHourRating::sDesigMap)
        {
            auto standard_value = value;
            standard_value.erase(std::remove(standard_value.begin(), standard_value.end(), ' '),
                                 standard_value.end());
            for (auto& c : standard_value)
            {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            if (standard_value == sCustomDrawProfile)
            {
                sCustomDrawProfile = value;
                hpwh.customTestOptions.overrideFirstHourRating = true;
                hpwh.customTestOptions.desig = key;
                foundProfile = true;
                break;
            }
        }
        if (!foundProfile)
        {
            std::cout << "Invalid input: Draw profile name not found.\n";
            exit(1);
        }
    }

    standardTestOptions.j_results["spec_type"] = sSpecType_mod;
    standardTestOptions.j_results["model_name"] = sModelName;

    standardTestOptions.sOutputFilename = "test24hr_" + sSpecType_mod + "_" + sModelName + ".csv";

    HPWH::FirstHourRating firstHourRating;
    hpwh.measureMetrics(firstHourRating, standardTestOptions, standardTestSummary);


    if ((sOutputDir != "")&&(sResultsFilename != ""))
    {
        std::ofstream resultsFile;
        std::string sFilepath = sOutputDir + "/" + sResultsFilename + ".json";
        resultsFile.open(sFilepath.c_str(), std::ifstream::out | std::ofstream::trunc);
        if (!resultsFile.is_open())
        {
            std::cout << "Could not open output file " << sResultsFilename << "\n";
            exit(1);
        }
        resultsFile << standardTestOptions.j_results.dump(2);
        resultsFile.close();
    }
    else
    {
        std::cout << "\t24-Hour Test Results:\n";
        if (!standardTestSummary.qualifies)
        {
            std::cout << "\t\tDoes not qualify as consumer water heater.\n";
        }

        std::cout
            << "\t\tRecovery Efficiency: " << standardTestSummary.recoveryEfficiency << "\n";

        std::cout << "\t\tStandby Loss Coefficient (kJ/h degC): "
                                          << standardTestSummary.standbyLossCoefficient_kJperhC << "\n";

        std::cout << "\t\tUEF: " << standardTestSummary.UEF << "\n";

        std::cout
            << "\t\tAverage Inlet Temperature (degC): " << standardTestSummary.avgInletT_C << "\n";

        std::cout
            << "\t\tAverage Outlet Temperature (degC): " << standardTestSummary.avgOutletT_C << "\n";

        std::cout
            << "\t\tTotal Volume Drawn (L): " << standardTestSummary.removedVolume_L << "\n";

        std::cout << "\t\tDaily Water-Heating Energy Consumption (kWh): "
                                          << KJ_TO_KWH(standardTestSummary.waterHeatingEnergy_kJ)
                                          << "\n";

        std::cout
            << "\t\tAdjusted Daily Water-Heating Energy Consumption (kWh): "
            << KJ_TO_KWH(standardTestSummary.adjustedConsumedWaterHeatingEnergy_kJ) << "\n";

        std::cout
            << "\t\tModified Daily Water-Heating Energy Consumption (kWh): "
            << KJ_TO_KWH(standardTestSummary.modifiedConsumedWaterHeatingEnergy_kJ) << "\n";

        std::cout << "\tAnnual Values:\n";
        std::cout
            << "\t\tAnnual Electrical Energy Consumption (kWh): "
            << KJ_TO_KWH(standardTestSummary.annualConsumedElectricalEnergy_kJ) << "\n";

        std::cout << "\t\tAnnual Energy Consumption (kWh): "
                                          << KJ_TO_KWH(standardTestSummary.annualConsumedEnergy_kJ)
                                          << "\n";
    }
}
} // namespace hpwh_cli
