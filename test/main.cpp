/*
 * Process command-line input
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "../vendor/CLI11/include/CLI/CLI.hpp"

void runCommand(const std::string& sSpecType,
                const std::string& sModelName,
                std::string sTestName,
                std::string sOutputDir,
                double airTemp);

void measureCommand(const std::string& sSpecType,
                    const std::string& sModelName,
                    std::string sOutputDir,
                    std::string sCustomDrawProfile);

void makeCommand(const std::string& sSpecType,
                 const std::string& sModelName,
                 double targetUEF,
                 std::string sOutputDir);

int main(int argc, char** argv)
{
    CLI::App app {"hpwh"};
    app.require_subcommand(0);
    {
        auto run = app.add_subcommand("run", "Run a schedule");

        std::string sSpecType;
        run->add_option("-s,--spec_type", sSpecType, "Spec type (Preset, File)");

        std::string sModelName = "";
        run->add_option("-m,--model", sModelName, "Model name")->required();

        std::string sTestName = "";
        run->add_option("-t,--test_name", sTestName, "Test name")->required();

        std::string sOutputDir = ".";
        run->add_option("-d,--dir", sOutputDir, "Output directory");

        double airTemp = -1000.;
        run->add_option("-a,--air_temp_C", airTemp, "Air temperature (degC)");

        run->callback([&]() { runCommand(sSpecType, sModelName, sTestName, sOutputDir, airTemp); });
    }
    {
        auto measure = app.add_subcommand("measure", "Measure the metrics for a model");

        std::string sSpecType;
        measure->add_option("-s,--spec_type", sSpecType, "Spec type (Preset, File)");

        std::string sModelName = "";
        measure->add_option("-m,--model", sModelName, "Model name")->required();

        std::string sOutputDir = ".";
        measure->add_option("-d,--dir", sOutputDir, "Output directory");

        std::string sCustomDrawProfile = "";
        measure->add_option("-p,--profile", sCustomDrawProfile, "Custom draw profile");

        measure->callback(
            [&]() { measureCommand(sSpecType, sModelName, sOutputDir, sCustomDrawProfile); });
    }
    {
        auto make = app.add_subcommand("make", "Make a model with a specified UEF");

        std::string sSpecType;
        make->add_option("-s,--spec_type", sSpecType, "Spec type (Preset, File)");

        std::string sModelName = "";
        make->add_option("-m,--model", sModelName, "Model name")->required();

        double targetUEF = -1.;
        make->add_option("-u,--uef", targetUEF, "target UEF")->required();

        std::string sOutputDir = ".";
        make->add_option("-d,--dir", sOutputDir, "Output directory");

        make->callback([&]() { makeCommand(sSpecType, sModelName, targetUEF, sOutputDir); });
    }
    CLI11_PARSE(app, argc, argv);

    // call with no subcommands is equivalent to subcommand "help"
    if (argc == 1)
    {
        std::cout << "HPWH - Heat-Pump Water Heaters\n" << std::endl;
        std::cout << app.help() << std::endl;
    }

    return EXIT_SUCCESS;
}
