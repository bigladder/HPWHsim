#include "HPWH.hh"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include "../vendor/CLI11/include/CLI/CLI.hpp"

void measureCommand(const std::string& sSpecType,
                    const std::string& sModelName,
                    std::string sTestName,
                    std::string sOutputDir,
                    double airTemp);
void makeCommand(const std::string& sSpecType,
                 const std::string& sModelName,
                 double targetUEF,
                 std::string sOutputDir);
void runCommand(const std::string& sSpecType,
                const std::string& sModelName,
                std::string sTestName,
                std::string sOutputDir,
                double airTemp);

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
        std::string tomlFilename;
        auto graph = app.add_subcommand("graph", "Graph a simulation");
        graph->add_option("toml_file", tomlFilename, "TOML filename")->required();

        std::string outputFilename = "graph.dot";
        bool useHtml = true;
        graph->add_option("-o,--out", outputFilename, "Graph output filename");
        graph->add_flag("-s,--simple", useHtml, "Create a simpler graph view");

        graph->callback([&]() { result = graphCommand(tomlFilename, outputFilename, !useHtml); });

        auto checkNetwork = app.add_subcommand("check", "Check network for issues");
        checkNetwork->add_option("toml_input_file", tomlFilename, "TOML input file name")
            ->required();
        checkNetwork->callback([&]() { result = checkNetworkCommand(tomlFilename); });
    }

    {
        std::string tomlFilename;
        std::string tomlOutputFilename = "out.toml";
        bool stripIds = false;
        auto update = app.add_subcommand("update", "Update an ERIN 0.55 file to current");
        update->add_option("toml_input_file", tomlFilename, "TOML input file name")->required();
        update->add_option("toml_output_file", tomlOutputFilename, "TOML output file name");
        update->add_flag(
            "-s,--strip-ids", stripIds, "If specified, strips ids from the input file");
        update->callback(
            [&]() { result = updateTomlInputCommand(tomlFilename, tomlOutputFilename, stripIds); });
    }

    {
        std::string tomlFilename;
        auto packLoads = app.add_subcommand("pack-loads", "Pack loads into a single csv file");
        packLoads->add_option("toml_file", tomlFilename, "TOML filename")->required();

        std::string loadsFilename = "packed-loads.csv";
        packLoads->add_option(
            "-o,--outcsv", loadsFilename, "Packed-loads csv filename; default:packed-loads.csv");

        bool verbose = false;
        packLoads->add_flag("-v,--verbose", verbose, "Verbose output");

        packLoads->callback([&]()
                            { result = packLoadsCommand(tomlFilename, loadsFilename, verbose); });
    }
    CLI11_PARSE(app, argc, argv);

    // call with no subcommands is equivalent to subcommand "help"
    if (argc == 1)
    {
        std::cout << "ERIN - Energy Resilience of Interacting Networks\n"
                  << "Version " << erin::version::version_string << "\n"
                  << std::endl;
        std::cout << app.help() << std::endl;
    }

    return EXIT_SUCCESS;
}
