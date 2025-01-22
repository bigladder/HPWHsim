/*
 * Simulate a HPWH model using a test schedule.
 */
#include "HPWH.hh"
#include "Condenser.hh"
#include "hpwh-data-model.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <fmt/format.h>

using std::cout;
using std::endl;
using std::ifstream;
using std::string;
#include <CLI/CLI.hpp>

typedef std::vector<double> schedule;

namespace hpwh_cli
{

/// run
static void run(const std::string& sSpecType,
                const std::string& sModelName,
                std::string sTestName,
                std::string sOutputDir,
                double airTemp);

CLI::App* add_run(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("run", "Run a schedule");

    static std::string sSpecType = "Preset";
    subcommand->add_option("-s,--spec", sSpecType, "Specification type (Preset, File, JSON)");

    static std::string sModelName = "";
    subcommand->add_option("-m,--model", sModelName, "Model name")->required();

    static std::string sTestName = "";
    subcommand->add_option("-t,--test", sTestName, "Test name")->required();

    static std::string sOutputDir = ".";
    subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

    static double airTemp = -1000.;
    subcommand->add_option("-a,--air_temp_C", airTemp, "Air temperature (degC)");

    subcommand->callback([&]() { run(sSpecType, sModelName, sTestName, sOutputDir, airTemp); });

    return subcommand;
}

int readSchedule(schedule& scheduleArray, string scheduleFileName, long minutesOfTest);

void run(const std::string& sSpecType,
         const std::string& sModelName,
         std::string sFullTestName,
         std::string sOutputDir,
         double airTemp)
{
    HPWH hpwh;

    HPWH::DRMODES drStatus = HPWH::DR_ALLOW;
    HPWH::MODELS model;

    const double EBALTHRESHOLD = 1.e-6;

    const int nTestTCouples = 6;

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
    string strHead = "minutes,Ta,Tsetpoint,inletT,draw,";
    string strHeadMP = "condenserInletT,condenserOutletT,externalVolGPM,";
    string strHeadSoC = "targetSoCFract,soCFract,";

    cout << "Testing HPWHsim version " << HPWH::getVersion() << endl;

    if (airTemp > 0.)
    {
        HPWH_doTempDepress = true;
    }
    else
    {
        airTemp = 0;
        HPWH_doTempDepress = false;
    }

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

    // Parse the model
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
        cout << "Invalid argument, received '" << sSpecType_mod
             << "', expected 'Preset', 'File', or 'JSON'.\n";
        exit(1);
    }

    newSetpoint = 0;
    model = static_cast<HPWH::MODELS>(hpwh.getModel());
    if (model == HPWH::MODELS_Sanden80 || model == HPWH::MODELS_Sanden40)
    {
        newSetpoint = (149 - 32) / 1.8;
    }

    std::string sTestName = sFullTestName; // remove path prefixes
    if (sFullTestName.find("/") != std::string::npos)
    {
        std::size_t iLast = sFullTestName.find_last_of("/");
        sTestName = sFullTestName.substr(iLast + 1);
    }

    // Use the built-in temperature depression for the lockout test. Set the temp depression of 4C
    // to better try and trigger the lockout and hysteresis conditions
    tempDepressThresh = 4;
    hpwh.setMaxTempDepression(tempDepressThresh);
    hpwh.setDoTempDepression(HPWH_doTempDepress);

    // Read the test control file
    fileToOpen = sFullTestName + "/" + "testInfo.txt";
    controlFile.open(fileToOpen.c_str());
    if (!controlFile.is_open())
    {
        cout << "Could not open control file " << fileToOpen << "\n";
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
    cout << "Running: " << sModelName << ", " << sSpecType_mod << ", " << sFullTestName << endl;

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
            cout << var1 << " in testInfo.txt is an unrecogized key.\n";
        }
    }

    if (minutesToRun == 0)
    {
        cout << "Error, must record length_of_test in testInfo.txt file\n";
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
        fileToOpen = sFullTestName + "/" + scheduleNames[i] + "schedule.csv";
        outputCode = readSchedule(allSchedules[i], fileToOpen, minutesToRun);
        if (outputCode != 0)
        {
            if (scheduleNames[i] != "setpoint" && scheduleNames[i] != "SoC")
            {
                cout << "readSchedule returns an error on " << scheduleNames[i] << " schedule!\n";
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
            cout << "If useSoC is true need an SoCschedule.csv file \n";
        }
        hpwh.switchToSoCControls(1., .05, soCMinTUse_C, true, soCMains_C);
    }

    // ----------------------Open the Output Files and Print the Header----------------------------
    // //

    if (minutesToRun > 500000.)
    {
        fileToOpen = sOutputDir + "/DHW_YRLY.csv";
        yearOutFile.open(fileToOpen.c_str(), std::ifstream::app);
        if (!yearOutFile.is_open())
        {
            cout << "Could not open output file " << fileToOpen << "\n";
            exit(1);
        }
    }
    else
    {

        fileToOpen = sOutputDir + "/" + sTestName + "_" + sSpecType_mod + "_" + sModelName + ".csv";

        outputFile.open(fileToOpen.c_str(), std::ifstream::out);
        if (!outputFile.is_open())
        {
            cout << "Could not open output file " << fileToOpen << "\n";
            exit(1);
        }

        string header = strHead;
        if (hpwh.isCompressorExternalMultipass() == 1)
        {
            header += strHeadMP;
        }
        if (useSoC)
        {
            header += strHeadSoC;
        }
        int csvOptions = HPWH::CSVOPT_NONE;
        hpwh.WriteCSVHeading(outputFile, header.c_str(), nTestTCouples, csvOptions);
    }

    // ------------------------------------- Simulate --------------------------------------- //
    cout << "Now Simulating " << minutesToRun << " Minutes of the Test\n";

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
            cout << "WARNING: On minute " << i << " HPWH has an energy balance error.\n";
            exit(1);
        }

        // Check timing
        for (int iHS = 0; iHS < hpwh.getNumHeatSources(); iHS++)
        {
            if (hpwh.getNthHeatSourceRunTime(iHS) > 1)
            {
                cout << "ERROR: On minute " << i << " heat source " << iHS << " ran for "
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
                cout << "ERROR: Externally heated volumes are inconsistent! Volume Heated [Gal]: "
                     << volumeHeated_Gal << ", mpFlowRate in 1 minute [Gal]: " << mpFlowVolume_Gal
                     << "\n";
                // exit(1);
            }
        }
        // Recording
        if (minutesToRun < 500000.)
        {
            // Copy current status into the output file
            if (HPWH_doTempDepress)
            {
                airTemp2 = hpwh.getLocationTemp_C();
            }
            strPreamble = std::to_string(i) + ", " + std::to_string(airTemp2) + ", " +
                          std::to_string(hpwh.getSetpoint()) + ", " +
                          std::to_string(allSchedules[0][i]) + ", " +
                          std::to_string(allSchedules[1][i]) + ", ";
            // Add some more outputs for mp tests
            if (hpwh.isCompressorExternalMultipass() == 1)
            {
                strPreamble += std::to_string(hpwh.getCondenserWaterInletTemp()) + ", " +
                               std::to_string(hpwh.getCondenserWaterOutletTemp()) + ", " +
                               std::to_string(hpwh.getExternalVolumeHeated(HPWH::UNITS_GAL)) + ", ";
            }
            if (useSoC)
            {
                strPreamble += std::to_string(allSchedules[6][i]) + ", " +
                               std::to_string(hpwh.getSoCFraction()) + ", ";
            }
            int csvOptions = HPWH::CSVOPT_NONE;
            if (allSchedules[1][i] > 0.)
            {
                csvOptions |= HPWH::CSVOPT_IS_DRAWING;
            }
            hpwh.WriteCSVRow(outputFile, strPreamble.c_str(), nTestTCouples, csvOptions);
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

        firstCol = sTestName + "," + sSpecType_mod + "," + sModelName;
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
    cout << "Opening " << scheduleFileName << '\n';

    if (!inputFile.is_open())
    {
        return 1;
    }

    inputFile >> snippet >> valTmp;
    // cout << "snippet " << snippet << " valTmp"<< valTmp<<'\n';

    if (snippet != "default")
    {
        cout << "First line of " << scheduleFileName << " must specify default\n";
        return 1;
    }
    // cout << valTmp << " minutes = " << minutesOfTest << "\n";

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
            cout << "In " << scheduleFileName
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
                // cout << "minute " << j-(minuteHrTmp) * 60 << " of hour" << (minuteHrTmp)<<"\n";
            }
        }
    }

    inputFile.close();

    return 0;
}

} // namespace hpwh_cli
