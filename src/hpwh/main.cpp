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
CLI::App* add_run(CLI::App& app);
CLI::App* add_measure(CLI::App& app);
CLI::App* add_make(CLI::App& app);
} // namespace hpwh_cli

using namespace hpwh_cli;

int main(int argc, char** argv)
{
    CLI::App app {"hpwh"};
    app.require_subcommand(0);

    add_run(app);
    add_measure(app);
    add_make(app);

    CLI11_PARSE(app, argc, argv);

    // call with no subcommands is equivalent to subcommand "help"
    if (argc == 1)
    {
        std::cout << "HPWHsim - Heat-Pump Water Heater simulator\n" << std::endl;
        std::cout << app.help() << std::endl;
    }

    return EXIT_SUCCESS;
}
