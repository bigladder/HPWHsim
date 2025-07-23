/*
 * Make a HPWH generic model with specified UEF
 */

#include <cmath>
#include "HPWH.hh"
#include "HPWHFitter.hh"
#include <CLI/CLI.hpp>

namespace hpwh_cli
{

/// make
static void make(HPWH& hpwh,
                 double targetEF,
                 std::string sTestConfig,
                 std::string sOutputDir,
                 bool saveTestData,
                 std::string sResultsFilename,
                 std::string drawProfileName,
                 std::vector<HPWH::PerformancePoly>& perfPolySet);

CLI::App* add_make(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("make", "Make a model with a specified EF");

    //
    static std::string specType = "Preset";
    subcommand->add_option("-s,--spec", specType, "Specification type (Preset, JSON, Legacy)");

    auto model_group = subcommand->add_option_group("Model options");

    static std::string modelName = "";
    model_group->add_option("-m,--model", modelName, "Model name");

    static int modelNumber = -1;
    model_group->add_option("-n,--number", modelNumber, "Model number");

    static std::string modelFilename = "";
    model_group->add_option("-f,--filename", modelFilename, "Model filename");

    model_group->required(1);

    //
    static double targetUEF = -1.;
    subcommand->add_option("-e,--ef", targetUEF, "target EF")->required();

    static std::string sTestConfig = "UEF";
    subcommand->add_option("-c,--config", sTestConfig, "test configuration");

    static int tier = 4;
    subcommand->add_option("-t,--tier", tier, "tier 3 or 4")->check(CLI::IsMember({3, 4}));

    static std::string outputDir = ".";
    subcommand->add_option("-d,--dir", outputDir, "Output directory");

    static bool saveTestData = true;
    subcommand->add_flag("-v,--save_data", saveTestData, "Save test data");

    static std::string resultsFilename = "";
    subcommand->add_option("-r,--results", resultsFilename, "Results filename");

    static std::string drawProfileName = "";
    subcommand->add_option("-p,--profile", drawProfileName, "Draw profile");

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
                if (!modelFilename.empty())
                {
                    std::ifstream inputFile(modelFilename + ".json");
                    nlohmann::json j = nlohmann::json::parse(inputFile);
                    modelName = getModelNameFromFilename(modelFilename);
                    hpwh.initFromJSON(j, modelName);
                }
            }
            else if (specType == "Legacy")
            {
                if (!modelName.empty())
                    hpwh.initLegacy(modelName);
                else if (modelNumber != -1)
                    hpwh.initLegacy(static_cast<hpwh_presets::MODELS>(modelNumber));
            }
            auto perfPolySet = (tier == 3) ? HPWH::tier3 : HPWH::tier4;

            make(hpwh,
                 targetUEF,
                 sTestConfig,
                 outputDir,
                 saveTestData,
                 resultsFilename,
                 drawProfileName,
                 perfPolySet);
        });

    return subcommand;
}

void make(HPWH& hpwh,
          double targetEF,
          std::string sTestConfig,
          std::string outputDir,
          bool saveTestData,
          std::string resultsFilename,
          std::string drawProfileName,
          std::vector<HPWH::PerformancePoly>& perfPolySet)
{
    // select test configuration
    transform(sTestConfig.begin(),
              sTestConfig.end(),
              sTestConfig.begin(),
              ::toupper); // make uppercase

    HPWH::TestConfiguration testConfiguration;
    if (sTestConfig == "E50")
        testConfiguration = HPWH::testConfiguration_E50;
    else if (sTestConfig == "E95")
        testConfiguration = HPWH::testConfiguration_E95;
    else
        testConfiguration = HPWH::testConfiguration_UEF;

    nlohmann::json j_results = {};

    auto designation = HPWH::FirstHourRating::Designation::Medium;
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
            remove(drawProfileName.begin(), drawProfileName.end(), ' ');
        }
        for (auto [key, value] : HPWH::FirstHourRating::DesignationMap)
        {
            // make lowercase
            for (auto& c : value)
            {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            // remove spaces
            remove(value.begin(), value.end(), ' ');
            if (value == drawProfileName)
            {
                designation = key;
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
    else
    {
        auto firstHourRating = hpwh.findFirstHourRating();
        designation = firstHourRating.designation;
        j_results["first_hour_rating"] = firstHourRating.report();
    }

    hpwh.makeGenericEF(targetEF, testConfiguration, designation, perfPolySet);

    auto testSummary = hpwh.run24hrTest(testConfiguration, designation, saveTestData);
    j_results["24_hr_test"] = testSummary.report();

    if (saveTestData)
    {
        if ((outputDir != "") && (resultsFilename != ""))
        {
            std::ofstream resultsFile;
            std::string sFilepath = outputDir + "/" + resultsFilename + ".json";
            resultsFile.open(sFilepath.c_str(), std::ifstream::out | std::ofstream::trunc);
            if (!resultsFile.is_open())
            {
                std::cout << "Could not open output file " << resultsFilename << "\n";
                exit(1);
            }
            resultsFile << j_results.dump(2);
            resultsFile.close();
        }
    }
}
} // namespace hpwh_cli
