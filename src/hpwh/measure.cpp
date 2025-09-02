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
    static std::string specType = "Preset";
    subcommand->add_option("-s,--spec", specType, "Specification type (Preset, JSON, Legacy)");

    auto model_group = subcommand->add_option_group("Model options");

    static std::string modelName = "";
    model_group->add_option("-m,--model", modelName, "Model name");

    static int modelNumber = -1;
    model_group->add_option("-n,--number", modelNumber, "Model number");

    static std::string modelFilepath = "";
    model_group->add_option("-f,--filepath", modelFilepath, "Model filepath");

    model_group->required(1);

    //
    static std::string outputDir = ".";
    subcommand->add_option("-d,--dir", outputDir, "Output directory");

    static bool saveTestData = true;
    subcommand->add_flag("-v,--save_data", saveTestData, "Save test data");

    static std::string resultsFilename = "";
    subcommand->add_option("-r,--results", resultsFilename, "Results filename");

    static std::string drawProfileName = "";
    subcommand->add_option("-p,--profile", drawProfileName, "Draw profile");

    static std::string testConfig = "UEF";
    subcommand->add_option("-c,--config", testConfig, "test configuration");

    subcommand->callback(
        [&]()
        {
            HPWH hpwh;
            if (specType == "Preset")
            {
                if (!modelName.empty())
                    hpwh.initPreset(modelName);
                else if (modelNumber != -1)
                    hpwh.initPreset(static_cast<hpwh_presets::MODELS>(modelNumber));
            }
            else if (specType == "JSON")
            {
                if (!modelName.empty())
                    modelFilepath = "./models_json/" + modelName + ".json";
                else if (!modelFilepath.empty())
                    modelName = getModelNameFromFilepath(modelFilepath);

                std::ifstream inputFile;
                inputFile.open(modelFilepath.c_str(), std::ifstream::in);
                if (!inputFile.is_open())
                {
                    hpwh.get_courier()->send_error(
                        fmt::format("Could not open input file {}\n", modelFilepath));
                }
                nlohmann::json j = nlohmann::json::parse(inputFile);
                hpwh.initFromJSON(j, modelName);
            }
            else if (specType == "Legacy")
            {
                if (!modelName.empty())
                    hpwh.initLegacy(modelName);
                else if (modelNumber != -1)
                    hpwh.initLegacy(static_cast<hpwh_presets::MODELS>(modelNumber));
            }
            measure(hpwh, outputDir, saveTestData, resultsFilename, drawProfileName, testConfig);
        });

    return subcommand;
}

void measure(HPWH& hpwh,
             std::string outputDir,
             bool saveTestData,
             std::string resultsFilename,
             std::string drawProfileName,
             std::string testConfig)
{
    auto designation = HPWH::FirstHourRating::Designation::Medium;

    nlohmann::json j_results = {};
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
            // remove spaces
            drawProfileName.erase(remove(drawProfileName.begin(), drawProfileName.end(), ' '),
                                  drawProfileName.end());
        }
        for (auto [key, value] : HPWH::FirstHourRating::DesignationMap)
        {
            // make lowercase
            for (auto& c : value)
            {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            // remove spaces
            value.erase(remove(value.begin(), value.end(), ' '), value.end());
            if (value == drawProfileName)
            {
                designation = key;
                foundProfile = true;
                break;
            }
        }
        if (!foundProfile)
        {
            hpwh.get_courier()->send_error("Invalid input: Draw profile name not found.");
        }
    }
    else
    {
        auto firstHourRating = hpwh.findFirstHourRating();
        designation = firstHourRating.designation;
        j_results["first_hour_rating"] = firstHourRating.report();
    }

    // select test configuration
    transform(testConfig.begin(),
              testConfig.end(),
              testConfig.begin(),
              ::toupper); // make uppercase
    HPWH::TestConfiguration testConfiguration = HPWH::testConfiguration_UEF;
    if (testConfig == "E50")
        testConfiguration = HPWH::testConfiguration_E50;
    else if (testConfig == "E95")
        testConfiguration = HPWH::testConfiguration_E95;

    auto testSummary = hpwh.run24hrTest(testConfiguration, designation, saveTestData);

    j_results["24_hr_test"] = testSummary.report();
    if ((outputDir != "") && (resultsFilename != ""))
    {
        resultsFilename = std::filesystem::path(resultsFilename).stem().string();
        std::string resultsFilepath = outputDir + "/" + resultsFilename + ".json";
        std::ofstream resultsFile;
        resultsFile.open(resultsFilepath.c_str(), std::ifstream::out | std::ofstream::trunc);
        if (!resultsFile.is_open())
        {
            std::cout << "Could not open output file " << resultsFilename << "\n";
            exit(1);
        }
        resultsFile << j_results.dump(2);
        resultsFile.close();
    }

    if (saveTestData)
    {
        std::ofstream outputFile;
        std::string filepath = "test24hrEF_" + hpwh.name + ".csv";
        if (outputDir != "")
            filepath = outputDir + "/" + filepath;
        outputFile.open(filepath.c_str(), std::ifstream::out);
        if (!outputFile.is_open())
        {
            std::cout << "Could not open output file " << filepath << "\n";
            exit(1);
        }
        hpwh.writeCSVHeading(&outputFile);
        for (auto& testData : testSummary.testDataSet)
            hpwh.writeCSVRow(&outputFile, testData);
    }
}
} // namespace hpwh_cli
