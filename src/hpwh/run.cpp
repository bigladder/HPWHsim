/*
 * Simulate a HPWH model using a test schedule.
 */
#include "HPWH.hh"
#include "Condenser.hh"
#include "hpwh-data-model.hh"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <fmt/format.h>

using std::endl;
using std::ifstream;
using std::string;
#include <CLI/CLI.hpp>

typedef std::vector<double> schedule;

namespace hpwh_cli
{

/// run
static void run(const std::string specType,
                HPWH& hpwh,
                std::string sTestName,
                std::string sOutputDir,
                double airTemp);

CLI::App* add_run(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("run", "Run a schedule");

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
    static std::string fullTestName = "";
    subcommand->add_option("-t,--test", fullTestName, "Test name")->required();

    static std::string outputDir = ".";
    subcommand->add_option("-d,--dir", outputDir, "Output directory");

    static double airTemp = -1000.;
    subcommand->add_option("-a,--air_temp_C", airTemp, "Air temperature (degC)");

    subcommand->callback(
        [&]()
        {
            HPWH hpwh;
            modelName = std::filesystem::path(modelName).stem().string();
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
            run(specType, hpwh, fullTestName, outputDir, airTemp);
        });

    return subcommand;
}

int readSchedule(schedule& scheduleArray, string scheduleFileName, long minutesOfTest);

void run(const std::string specType,
         HPWH& hpwh,
         std::string fullTestName,
         std::string outputDir,
         double airTemp)
{
    HPWH::DRMODES drStatus = HPWH::DR_ALLOW;
    hpwh_presets::MODELS model;

    const double EBALTHRESHOLD = 1.e-6;

    const double soCMinTUse_C = F_TO_C(110.);
    const double soCMains_C = F_TO_C(65.);

    // Schedule stuff
    std::vector<string> scheduleNames;
    std::vector<schedule> allSchedules(7);

    string fileToOpen, fileToOpen2, scheduleName, var1;
    string inputVariableName, firstCol;
    double testVal, newSetpoint, airTemp2, tempDepressThresh, inletH, newTankSize, tot_limit,
        initialTankT_C;
    bool useSoC;
    int i, outputCode;
    long minutesToRun;

    double cumHeatIn[3] = {0, 0, 0};
    double cumHeatOut[3] = {0, 0, 0};

    bool HPWH_doTempDepress;
    int doInvMix, doCondu;

    std::ofstream outputFile;
    std::ofstream yearOutFile;
    ifstream controlFile;

    string strPreamble;

    std::cout << "Testing HPWHsim version " << HPWH::getVersion() << endl;

    if (airTemp > 0.)
    {
        HPWH_doTempDepress = true;
    }
    else
    {
        airTemp = 0;
        HPWH_doTempDepress = false;
    }

    newSetpoint = 0;
    model = static_cast<hpwh_presets::MODELS>(hpwh.getModel());
    if (model == hpwh_presets::MODELS::Sanco83 || model == hpwh_presets::MODELS::Sanco43)
    {
        newSetpoint = (149 - 32) / 1.8;
    }

    std::string sTestName = fullTestName; // remove path prefixes
    if (fullTestName.find("/") != std::string::npos)
    {
        std::size_t iLast = fullTestName.find_last_of("/");
        sTestName = fullTestName.substr(iLast + 1);
    }

    // Use the built-in temperature depression for the lockout test. Set the temp depression of 4C
    // to better try and trigger the lockout and hysteresis conditions
    tempDepressThresh = 4;
    hpwh.setMaxTempDepression(tempDepressThresh);
    hpwh.setDoTempDepression(HPWH_doTempDepress);

    // Read the test control file
    fileToOpen = fullTestName + "/" + "testInfo.txt";
    controlFile.open(fileToOpen.c_str());
    if (!controlFile.is_open())
    {
        std::cout << "Could not open control file " << fileToOpen << "\n";
        exit(1);
    }
    outputCode = 0;
    minutesToRun = 0;
    newSetpoint = 0.;
    initialTankT_C = 0.;
    doCondu = 1;
    doInvMix = 1;
    inletH = 0.;
    newTankSize = 0.;
    tot_limit = 0.;
    useSoC = false;
    bool hasInitialTankTemp = false;

    std::cout << "Running: " << hpwh.name << ", " << specType << ", " << fullTestName << endl;

    while (controlFile >> var1 >> testVal)
    {
        if (var1 == "setpoint")
        { // If a setpoint was specified then override the default
            newSetpoint = testVal;
        }
        else if (var1 == "length_of_test")
        {
            minutesToRun = (int)testVal;
        }
        else if (var1 == "doInversionMixing")
        {
            doInvMix = (testVal > 0.0) ? 1 : 0;
        }
        else if (var1 == "doConduction")
        {
            doCondu = (testVal > 0.0) ? 1 : 0;
        }
        else if (var1 == "inletH")
        {
            inletH = testVal;
        }
        else if (var1 == "tanksize")
        {
            newTankSize = testVal;
        }
        else if (var1 == "tot_limit")
        {
            tot_limit = testVal;
        }
        else if (var1 == "useSoC")
        {
            useSoC = (bool)testVal;
        }
        else if (var1 == "initialTankT_C")
        { // Initialize at this temperature instead of setpoint
            initialTankT_C = testVal;
            hasInitialTankTemp = true;
        }
        else
        {
            std::cout << var1 << " in testInfo.txt is an unrecogized key.\n";
        }
    }

    if (minutesToRun == 0)
    {
        std::cout << "Error, must record length_of_test in testInfo.txt file\n";
        exit(1);
    }

    // ------------------------------------- Read Schedules---------------------------------------
    // //
    scheduleNames.push_back("inletT");
    scheduleNames.push_back("draw");
    scheduleNames.push_back("ambientT");
    scheduleNames.push_back("evaporatorT");
    scheduleNames.push_back("DR");
    scheduleNames.push_back("setpoint");
    scheduleNames.push_back("SoC");

    for (i = 0; (unsigned)i < scheduleNames.size(); i++)
    {
        fileToOpen = fullTestName + "/" + scheduleNames[i] + "schedule.csv";
        outputCode = readSchedule(allSchedules[i], fileToOpen, minutesToRun);
        if (outputCode != 0)
        {
            if (scheduleNames[i] != "setpoint" && scheduleNames[i] != "SoC")
            {
                std::cout << "readSchedule returns an error on " << scheduleNames[i]
                          << " schedule!\n";
                exit(1);
            }
            else
            {
                outputCode = 0;
            }
        }
    }

    if (doInvMix == 0)
    {
        hpwh.setDoInversionMixing(false);
    }

    if (doCondu == 0)
    {
        hpwh.setDoConduction(false);
    }
    if (newSetpoint > 0)
    {
        double maxAllowedSetpointT_C;
        string why;
        if (!allSchedules[5].empty())
        {
            if (hpwh.isNewSetpointPossible(allSchedules[5][0], maxAllowedSetpointT_C, why))
            {
                hpwh.setSetpoint(allSchedules[5][0]);
            }
        }
        else if (newSetpoint > 0)
        {
            if (hpwh.isNewSetpointPossible(newSetpoint, maxAllowedSetpointT_C, why))
            {
                hpwh.setSetpoint(newSetpoint);
            }
        }
        if (hasInitialTankTemp)
            hpwh.setTankToTemperature(initialTankT_C);
        else
            hpwh.resetTankToSetpoint();
    }
    if (inletH > 0)
    {
        hpwh.setInletByFraction(inletH);
    }
    if (newTankSize > 0)
    {
        hpwh.setTankSize(newTankSize, HPWH::UNITS_GAL);
    }
    if (tot_limit > 0)
    {
        hpwh.setTimerLimitTOT(tot_limit);
    }
    if (useSoC)
    {
        if (allSchedules[6].empty())
        {
            std::cout << "If useSoC is true need an SoCschedule.csv file \n";
        }
        hpwh.switchToSoCControls(1., .05, soCMinTUse_C, true, soCMains_C);
    }

    // ----------------------Open the Output Files and Print the Header----------------------------
    // //

    if (minutesToRun > 500000.)
    {
        fileToOpen = outputDir + "/DHW_YRLY_" + specType + ".csv";
        yearOutFile.open(fileToOpen.c_str(), std::ifstream::app);
        if (!yearOutFile.is_open())
        {
            std::cout << "Could not open output file " << fileToOpen << "\n";
            exit(1);
        }
    }
    else
    {

        fileToOpen = outputDir + "/" + sTestName + "_" + specType + "_" + hpwh.name + ".csv";

        outputFile.open(fileToOpen.c_str(), std::ifstream::out);
        if (!outputFile.is_open())
        {
            std::cout << "Could not open output file " << fileToOpen << "\n";
            exit(1);
        }

        hpwh.writeCSVHeading(&outputFile, HPWH::CSVOPT_NONE);
    }

    // ------------------------------------- Simulate --------------------------------------- //
    std::cout << "Now Simulating " << minutesToRun << " Minutes of the Test\n";

    std::vector<double> nodeExtraHeat_W;
    std::vector<double>* vectptr = NULL;
    // Loop over the minutes in the test
    for (i = 0; i < minutesToRun; i++)
    {
        if (HPWH_doTempDepress)
        {
            airTemp2 = F_TO_C(airTemp);
        }
        else
        {
            airTemp2 = allSchedules[2][i];
        }

        double tankHCStart = hpwh.getTankHeatContent_kJ();

        // Process the dr status
        drStatus = static_cast<HPWH::DRMODES>(int(allSchedules[4][i]));

        // Change setpoint if there is a setpoint schedule.
        if (!allSchedules[5].empty() && !hpwh.isSetpointFixed())
        {
            hpwh.setSetpoint(allSchedules[5][i]); // expect this to fail sometimes
        }

        // Change SoC schedule
        if (useSoC)
        {
            hpwh.setTargetSoCFraction(allSchedules[6][i]);
        }

        // Mix down for yearly tests with large compressors
        if (hpwh.getModel() >= 210 && minutesToRun > 500000.)
        {
            // Do a simple mix down of the draw for the cold water temperature
            if (hpwh.getSetpoint() <= 125.)
            {
                allSchedules[1][i] *= (125. - allSchedules[0][i]) /
                                      (hpwh.getTankNodeTemp(hpwh.getNumNodes() - 1, HPWH::UNITS_F) -
                                       allSchedules[0][i]);
            }
        }

        // Run the step
        hpwh.runOneStep(allSchedules[0][i],           // Inlet water temperature (C)
                        GAL_TO_L(allSchedules[1][i]), // Flow in gallons
                        airTemp2,                     // Ambient Temp (C)
                        allSchedules[3][i],           // External Temp (C)
                        drStatus, // DDR Status (now an enum. Fixed for now as allow)
                        1. * GAL_TO_L(allSchedules[1][i]),
                        allSchedules[0][i],
                        vectptr);

        if (!hpwh.isEnergyBalanced(
                GAL_TO_L(allSchedules[1][i]), allSchedules[0][i], tankHCStart, EBALTHRESHOLD))
        {
            std::cout << "WARNING: On minute " << i << " HPWH has an energy balance error.\n";
            exit(1);
        }

        // Check timing
        for (int iHS = 0; iHS < hpwh.getNumHeatSources(); iHS++)
        {
            if (hpwh.getNthHeatSourceRunTime(iHS) > 1)
            {
                std::cout << "ERROR: On minute " << i << " heat source " << iHS << " ran for "
                          << hpwh.getNthHeatSourceRunTime(iHS) << "minutes"
                          << "\n";
                exit(1);
            }
        }
        // Check flow for external MP
        if (hpwh.isCompressorExternalMultipass() == 1)
        {
            double volumeHeated_Gal = hpwh.getExternalVolumeHeated(HPWH::UNITS_GAL);
            double mpFlowVolume_Gal = hpwh.getExternalMPFlowRate(HPWH::UNITS_GPM) *
                                      hpwh.getNthHeatSourceRunTime(hpwh.getCompressorIndex());
            if (fabs(volumeHeated_Gal - mpFlowVolume_Gal) > 0.000001)
            {
                std::cout
                    << "ERROR: Externally heated volumes are inconsistent! Volume Heated [Gal]: "
                    << volumeHeated_Gal << ", mpFlowRate in 1 minute [Gal]: " << mpFlowVolume_Gal
                    << "\n";
                // exit(1);
            }
        }
        // Recording
        if (minutesToRun < 500000.)
        {
            if (HPWH_doTempDepress)
            {
                airTemp2 = hpwh.getLocationTemp_C();
            }

            HPWH::TestData testData;
            testData.time_min = i;
            testData.ambientT_C = airTemp2;
            testData.setpointT_C = hpwh.getSetpoint();
            testData.inletT_C = allSchedules[0][i];
            testData.drawVolume_L = GAL_TO_L(allSchedules[1][i]);
            testData.outletT_C = hpwh.getOutletTemp();

            testData.h_srcIn_kWh = {};
            testData.h_srcOut_kWh = {};
            for (int iHS = 0; iHS < hpwh.getNumHeatSources(); iHS++)
            {
                testData.h_srcIn_kWh.push_back(
                    hpwh.getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KWH));
                testData.h_srcOut_kWh.push_back(
                    hpwh.getNthHeatSourceEnergyOutput(iHS, HPWH::UNITS_KWH));
            }

            testData.thermocoupleT_C = {};
            for (int iTC = 0; iTC < HPWH::TestData::nTCouples; iTC++)
                testData.thermocoupleT_C.push_back(
                    hpwh.getNthSimTcouple(iTC + 1, HPWH::TestData::nTCouples, HPWH::UNITS_C));

            hpwh.writeCSVRow(&outputFile, testData);
        }
        else
        {
            for (int iHS = 0; iHS < hpwh.getNumHeatSources(); iHS++)
            {
                cumHeatIn[iHS] += hpwh.getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KWH) * 1000.;
                cumHeatOut[iHS] += hpwh.getNthHeatSourceEnergyOutput(iHS, HPWH::UNITS_KWH) * 1000.;
            }
        }
    }

    if (minutesToRun > 500000.)
    {
        firstCol = sTestName + "," + hpwh.name;
        yearOutFile << firstCol;
        double totalIn = 0, totalOut = 0;
        for (int iHS = 0; iHS < 3; iHS++)
        {
            yearOutFile << fmt::format(",{:0.0f},{:0.0f}", cumHeatIn[iHS], cumHeatOut[iHS]);
            totalIn += cumHeatIn[iHS];
            totalOut += cumHeatOut[iHS];
        }
        yearOutFile << fmt::format(",{:0.0f},{:0.0f}", totalIn, totalOut);
        for (int iHS = 0; iHS < 3; iHS++)
        {
            yearOutFile << fmt::format(",{:0.2f}", cumHeatOut[iHS] / cumHeatIn[iHS]);
        }
        yearOutFile << fmt::format(",{:0.2f}", totalOut / totalIn) << std::endl;
        yearOutFile.close();
    }
    else
    {
        yearOutFile.close();
    }
    controlFile.close();
}

// this function reads the named schedule into the provided array
int readSchedule(schedule& scheduleArray, string scheduleFileName, long minutesOfTest)
{
    int minuteHrTmp;
    bool hourInput;
    string line, snippet, s, minORhr;
    double valTmp;
    ifstream inputFile(scheduleFileName.c_str());
    // open the schedule file provided
    std::cout << "Opening " << scheduleFileName << '\n';

    if (!inputFile.is_open())
    {
        return 1;
    }

    inputFile >> snippet >> valTmp;
    // std::cout << "snippet " << snippet << " valTmp"<< valTmp<<'\n';

    if (snippet != "default")
    {
        std::cout << "First line of " << scheduleFileName << " must specify default\n";
        return 1;
    }
    // std::cout << valTmp << " minutes = " << minutesOfTest << "\n";

    // Fill with the default value
    scheduleArray.assign(minutesOfTest, valTmp);

    // Burn the first two lines
    std::getline(inputFile, line);
    std::getline(inputFile, line);

    std::stringstream ss(line); // Will parse with a stringstream
    // Grab the first token, which is the minute or hour marker
    ss >> minORhr;
    if (minORhr.empty())
    { // If nothing left in the file
        return 0;
    }
    hourInput = tolower(minORhr.at(0)) == 'h';
    char c; // to eat the commas nom nom
    // Read all the exceptions to the default value
    while (inputFile >> minuteHrTmp >> c >> valTmp)
    {

        if (minuteHrTmp >= (int)scheduleArray.size())
        {
            std::cout << "In " << scheduleFileName
                      << " the input file has more minutes than the test was defined with\n";
            return 1;
        }
        // Update the value
        if (!hourInput)
        {
            scheduleArray[minuteHrTmp] = valTmp;
        }
        else if (hourInput)
        {
            for (int j = minuteHrTmp * 60; j < (minuteHrTmp + 1) * 60; j++)
            {
                scheduleArray[j] = valTmp;
                // std::cout << "minute " << j-(minuteHrTmp) * 60 << " of hour" <<
                // (minuteHrTmp)<<"\n";
            }
        }
    }

    inputFile.close();

    return 0;
}

} // namespace hpwh_cli
