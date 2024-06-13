/*
 * Process command-line input
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <CLI/CLI.hpp>

namespace hpwh_cli
{
void run(const std::string& sSpecType,
         const std::string& sModelName,
         std::string sTestName,
         std::string sOutputDir,
         double airTemp);

void measure(const std::string& sSpecType,
             const std::string& sModelName,
             std::string sOutputDir,
             std::string sCustomDrawProfile);

void make(const std::string& sSpecType,
          const std::string& sModelName,
          double targetUEF,
          std::string sOutputDir);
} // namespace hpwh_cli

using namespace hpwh_cli;

int main(int argc, char** argv)
{
    CLI::App app {"hpwh"};
    app.require_subcommand(0);

    /// run
    {
        const auto subcommand = app.add_subcommand("run", "Run a schedule");

        static std::string sSpecType = "Preset";
        subcommand->add_option("-s,--spec", sSpecType, "Specification type (Preset, File)");

        static std::string sModelName = "";
        subcommand->add_option("-m,--model", sModelName, "Model name")->required();

        static std::string sTestName = "";
        subcommand->add_option("-t,--test", sTestName, "Test directory name")->required();

        static std::string sOutputDir = ".";
        subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

        static double airTemp = -1000.;
        subcommand->add_option("-a,--air_temp_C", airTemp, "Air temperature (degC)");

        subcommand->callback([&]() { run(sSpecType, sModelName, sTestName, sOutputDir, airTemp); });
    }

    /// measure
    {
        const auto subcommand = app.add_subcommand("measure", "Measure the metrics for a model");

        static std::string sSpecType = "Preset";
        subcommand->add_option("-s,--spec", sSpecType, "Specification type (Preset, File)");

        static std::string sModelName = "";
        subcommand->add_option("-m,--model", sModelName, "Model name")->required();

        static std::string sOutputDir = ".";
        subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

        static std::string sCustomDrawProfile = "";
        subcommand->add_option("-p,--profile", sCustomDrawProfile, "Custom draw profile");

        subcommand->callback([&]()
                             { measure(sSpecType, sModelName, sOutputDir, sCustomDrawProfile); });
    }

    /// make
    {
        const auto subcommand = app.add_subcommand("make", "Make a model with a specified UEF");

        static std::string sSpecType = "Preset";
        subcommand->add_option("-s,--spec", sSpecType, "Specification type (Preset, File)");

        static std::string sModelName = "";
        subcommand->add_option("-m,--model", sModelName, "Model name")->required();

        static double targetUEF = -1.;
        subcommand->add_option("-u,--uef", targetUEF, "target UEF")->required();

        static std::string sOutputDir = ".";
        subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

        subcommand->callback([&]() { make(sSpecType, sModelName, targetUEF, sOutputDir); });

        CLI11_PARSE(app, argc, argv);
    }

    // call with no subcommands is equivalent to subcommand "help"
    if (argc == 1)
    {
        std::cout << "HPWHsim - Heat-Pump Water Heater simulator\n" << std::endl;
        std::cout << app.help() << std::endl;
    }

    return EXIT_SUCCESS;
}
