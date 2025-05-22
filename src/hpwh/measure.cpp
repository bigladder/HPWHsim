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

    static std::string specType = "Preset";
    subcommand->add_option("-s,--spec", specType, "Specification type (Preset, JSON)");

    //
    auto model_group = subcommand->add_option_group("model");

    static std::string modelName = "";
    model_group->add_option("-m,--model", modelName, "Model name");

    static int modelNumber = -1;
    model_group->add_option("-n,--number", modelNumber, "Model number");

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

    HPWH hpwh;
    if (modelName != "")
        hpwh.init(specType, modelName);
    else if (modelNumber != -1)
        hpwh.init(specType, static_cast<HPWH::MODELS>(modelNumber));

    subcommand->callback(
        [&]()
        {
            measure(hpwh,
                    sOutputDir,
                    saveTestData,
                    sResultsFilename,
                    drawProfileName,
                    sTestConfig);
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
    std::string results = "";
    auto designation = HPWH::FirstHourRating::Designation::Medium;

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
