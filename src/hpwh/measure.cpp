/*
 * Measure the performance metrics of a HPWH model
 */

#include <CLI/CLI.hpp>
#include "HPWH.hh"

namespace hpwh_cli
{

/// measure
static void measure(HPWH& hpwh,
                    std::string sOutputDir,
                    bool sSupressOutput,
                    std::string sResultsFilename,
                    std::string customDrawProfile,
                    std::string sTestConfig);

CLI::App* add_measure(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("measure", "Measure the metrics for a model");

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

    static bool saveTestData = true;
    subcommand->add_flag("-v,--save_data", saveTestData, "Save test data");

    static std::string sResultsFilename = "";
    subcommand->add_option("-r,--results", sResultsFilename, "Results filename");

    static std::string drawProfileName = "";
    subcommand->add_option("-p,--profile", drawProfileName, "Draw profile");

    static std::string sTestConfig = "UEF";
    subcommand->add_option("-c,--config", sTestConfig, "test configuration");

    subcommand->callback(
        [&]()
        {
            HPWH hpwh;
            std::string specType = "Preset";
            if (!modelName.empty())
                hpwh.initPreset(modelName);
            else if (modelNumber != -1)
                hpwh.initPreset(static_cast<hpwh_presets::MODELS>(modelNumber));
            else if (!modelFilename.empty())
            {
                std::ifstream inputFile(modelFilename);
                nlohmann::json j = nlohmann::json::parse(inputFile);
                specType = "JSON";
                modelName = getModelNameFromFilename(modelFilename);
                hpwh.initFromJSON(j, modelName);
            }
            measure(hpwh, sOutputDir, saveTestData, sResultsFilename, drawProfileName, sTestConfig);
        });

    return subcommand;
}

void measure(HPWH& hpwh,
             std::string sOutputDir,
             bool saveTestData,
             std::string sResultsFilename,
             std::string drawProfileName,
             std::string sTestConfig)
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
    std::string results = "";
    auto designation = HPWH::FirstHourRating::Designation::Medium;

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
    // set draw profile
    if (drawProfileName != "")
    {
        bool foundProfile = false;
        if (drawProfileName.length() > 0)
        {
            // make lowercase
            for (auto& c : drawProfileName)
            {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }

            // make first char upper
            drawProfileName[0] =
                static_cast<char>(std::toupper(static_cast<unsigned char>(drawProfileName[0])));
        }
        for (const auto& [key, value] : HPWH::FirstHourRating::DesignationMap)
        {
            if (value == drawProfileName)
            {
                designation = key;
                foundProfile = true;
                results.append(fmt::format("\tCustom Draw Profile: {}\n", drawProfileName));
                break;
            }
        }
        if (!foundProfile)
        {
            hpwh.get_courier()->send_error("Invalid input: Draw profile name not found.");
            exit(1);
        }
    }

    standardTestOptions.j_results["spec_type"] = sSpecType_mod;
    standardTestOptions.j_results["model_name"] = sModelName;
    else
    {
        auto firstHourRating = hpwh.findFirstHourRating();
        designation = firstHourRating.designation;
        results.append(firstHourRating.report());
    }

    // select test configuration
    transform(sTestConfig.begin(),
              sTestConfig.end(),
              sTestConfig.begin(),
              ::toupper); // make uppercase
    HPWH::TestConfiguration testConfiguration = HPWH::testConfiguration_UEF;
    if (sTestConfig == "E50")
        testConfiguration = HPWH::testConfiguration_E50;
    else if (sTestConfig == "E95")
        testConfiguration = HPWH::testConfiguration_E95;

    auto testSummary = hpwh.run24hrTest(testConfiguration, designation, saveTestData);
    if (saveTestData)
    {

    }
    results.append(testSummary.report());

    hpwh.get_courier()->send_info("\n" + results);
    if (sResultsFilename != "")
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

        std::cout << "\t\tRecovery Efficiency: " << standardTestSummary.recoveryEfficiency << "\n";

        std::cout << "\t\tStandby Loss Coefficient (kJ/h degC): "
                  << standardTestSummary.standbyLossCoefficient_kJperhC << "\n";

        std::cout << "\t\tUEF: " << standardTestSummary.UEF << "\n";

        std::cout << "\t\tAverage Inlet Temperature (degC): " << standardTestSummary.avgInletT_C
                  << "\n";

        std::cout << "\t\tAverage Outlet Temperature (degC): " << standardTestSummary.avgOutletT_C
                  << "\n";

        std::cout << "\t\tTotal Volume Drawn (L): " << standardTestSummary.removedVolume_L << "\n";

        std::cout << "\t\tDaily Water-Heating Energy Consumption (kWh): "
                  << KJ_TO_KWH(standardTestSummary.waterHeatingEnergy_kJ) << "\n";

        std::cout << "\t\tAdjusted Daily Water-Heating Energy Consumption (kWh): "
                  << KJ_TO_KWH(standardTestSummary.adjustedConsumedWaterHeatingEnergy_kJ) << "\n";

        std::cout << "\t\tModified Daily Water-Heating Energy Consumption (kWh): "
                  << KJ_TO_KWH(standardTestSummary.modifiedConsumedWaterHeatingEnergy_kJ) << "\n";

        std::cout << "\tAnnual Values:\n";
        std::cout << "\t\tAnnual Electrical Energy Consumption (kWh): "
                  << KJ_TO_KWH(standardTestSummary.annualConsumedElectricalEnergy_kJ) << "\n";

        std::cout << "\t\tAnnual Energy Consumption (kWh): "
                  << KJ_TO_KWH(standardTestSummary.annualConsumedEnergy_kJ) << "\n";
        auto resultsStream = std::make_shared<std::ofstream>();
        if (sOutputDir != "")
            sResultsFilename = sOutputDir + "/" + sResultsFilename;
        resultsStream->open(sResultsFilename.c_str(), std::ofstream::out | std::ofstream::trunc);
        if (resultsStream->is_open())
        {
            *resultsStream << results;
        }
    }
}
} // namespace hpwh_cli
