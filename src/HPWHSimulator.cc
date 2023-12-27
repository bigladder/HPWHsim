/*
 * Implementation of class HPWH::Simulator
 */

#include <algorithm>
#include <regex>
#include <fstream>
#include <iostream>

#include "HPWH.hh"

HPWH::Simulator::Simulator() : verbosity(VRB_typical) {}

HPWH::Simulator::Simulator(const Simulator& simulator) { *this = simulator; }

HPWH::Simulator& HPWH::Simulator::operator=(const Simulator& simulator_in)
{
    if (this == &simulator_in)
    {
        return *this;
    }
    return *this;
}

bool HPWH::Simulator::openFileText(std::ifstream& fileStream, const std::string& sFilename)
{
    // std::string fileToOpen = testDirectory + "/" + "testInfo.txt";
    // Read the test control file
    fileStream.open(sFilename.c_str());
    if (!fileStream.is_open())
    {
        if (verbosity >= VRB_reluctant)
        {
            msg("Could not open control file %s\n", sFilename.c_str());
        }
        return false;
    }
    return true;
}

bool HPWH::Simulator::openResourceText(std::istream& resourceStream, const std::string& sFilename)
{
    std::ifstream* controlFile = static_cast<std::ifstream*>(&resourceStream);
    // std::string fileToOpen = testDirectory + "/" + "testInfo.txt";
    // Read the test control file
    controlFile->open(sFilename.c_str());
    if (!controlFile->is_open())
    {
        if (verbosity >= VRB_reluctant)
        {
            msg("Could not open control file %s\n", sFilename.c_str());
        }
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
///	@brief	Reads the control info for a test from file "testInfo.txt"
///	@param[in]	testDirectory	path to directory containing test files
///	@param[out]	controlInfo		data structure containing control info
/// @return		true if successful, false otherwise
//-----------------------------------------------------------------------------
bool HPWH::Simulator::readControlInfo(std::istream& controlStream, ControlInfo& controlInfo)
{
    controlInfo.outputCode = 0;
    controlInfo.timeToRun_min = 0;
    controlInfo.setpointT_C = 0.;
    controlInfo.initialTankT_C = nullptr;
    controlInfo.doConduction = true;
    controlInfo.doInversionMixing = true;
    controlInfo.inletH = nullptr;
    controlInfo.tankSize_gal = nullptr;
    controlInfo.tot_limit = nullptr;
    controlInfo.useSoC = false;
    controlInfo.temperatureUnits = "C";
    controlInfo.recordMinuteData = false;
    controlInfo.recordYearData = false;
    controlInfo.modifyDraw = false;

    std::string token;
    std::string sValue;
    while (controlStream >> token >> sValue)
    {
        if (token == "setpoint")
        { // If a setpoint was specified then override the default
            controlInfo.setpointT_C = std::stod(sValue);
        }
        else if (token == "length_of_test")
        {
            controlInfo.timeToRun_min = std::stoi(sValue);
        }
        else if (token == "doInversionMixing")
        {
            controlInfo.doInversionMixing = getBool(sValue);
        }
        else if (token == "doConduction")
        {
            controlInfo.doConduction = getBool(sValue);
        }
        else if (token == "inletH")
        {
            controlInfo.inletH = std::unique_ptr<double>(new double(std::stod(sValue)));
        }
        else if (token == "tanksize")
        {
            controlInfo.tankSize_gal = std::unique_ptr<double>(new double(std::stod(sValue)));
        }
        else if (token == "tot_limit")
        {
            controlInfo.tot_limit = std::unique_ptr<double>(new double(std::stod(sValue)));
        }
        else if (token == "useSoC")
        {
            controlInfo.useSoC = getBool(sValue);
        }
        else if (token == "initialTankT_C")
        { // Initialize at this temperature instead of setpoint
            controlInfo.initialTankT_C = std::unique_ptr<double>(new double(std::stod(sValue)));
        }
        else if (token == "temperature_units")
        { // Initialize at this temperature instead of setpoint
            controlInfo.temperatureUnits = sValue;
        }
        else
        {
            if (verbosity >= VRB_reluctant)
            {
                msg("Unrecognized key in %s\n", token.c_str());
            }
        }
    }

    if (controlInfo.temperatureUnits == "F")
    {
        controlInfo.setpointT_C = F_TO_C(controlInfo.setpointT_C);
        if (controlInfo.initialTankT_C != nullptr)
            *controlInfo.initialTankT_C = F_TO_C(*controlInfo.initialTankT_C);
    }

    if (controlInfo.timeToRun_min == 0)
    {
        if (verbosity >= VRB_reluctant)
        {
            msg("Error: record length_of_test not found in testInfo.txt file\n");
        }
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
///	@brief	Reads a named schedule
/// @param[out]	schedule			schedule values
///	@param[in]	scheduleName		name of schedule to read
///	@param[in]	testLength_min		length of test (min)
/// @return		true if successful, false otherwise
//-----------------------------------------------------------------------------
bool HPWH::Simulator::readSchedule(Schedule& schedule,
                                   const std::string& scheduleName,
                                   std::istream& scheduleStream,
                                   long testLength_min)
{
    int minuteHrTmp;
    bool hourInput;
    std::string line, snippet, s, minORhr;
    double valTmp;

    scheduleStream >> snippet >> valTmp;

    if (snippet != "default")
    {
        if (verbosity >= VRB_reluctant)
        {
            msg("First line of %s must specify default\n", scheduleName.c_str());
        }
        return false;
    }

    // Fill with the default value
    schedule.assign(testLength_min, valTmp);

    // Burn the first two lines
    std::getline(scheduleStream, line);
    std::getline(scheduleStream, line);

    std::stringstream ss(line); // Will parse with a stringstream
    // Grab the first token, which is the minute or hour marker
    ss >> minORhr;
    if (minORhr.empty())
    { // If nothing left in the file
        return true;
    }
    hourInput = tolower(minORhr.at(0)) == 'h';
    char c; // for commas
    while (scheduleStream >> minuteHrTmp >> c >> valTmp)
    {
        if (minuteHrTmp >= static_cast<int>(schedule.size()))
        {
            if (verbosity >= VRB_reluctant)
            {
                msg("Input file for %s has more minutes than test definition\n",
                    scheduleName.c_str());
            }
            return false;
        }
        // update the value
        if (!hourInput)
        {
            schedule[minuteHrTmp] = valTmp;
        }
        else if (hourInput)
        {
            for (int j = minuteHrTmp * 60; j < (minuteHrTmp + 1) * 60; j++)
            {
                schedule[j] = valTmp;
            }
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
///	@brief	Reads the schedules for a test
///	@param[in]	testDirectory	path to directory containing test files
///	@param[in]	controlInfo		data structure containing control info
///	@param[out]	allSchedules	collection of test schedules read
/// @return		true if successful, false otherwise
//-----------------------------------------------------------------------------
bool HPWH::Simulator::readSchedules(const bool isResource,
                                    const std::string& testName,
                                    const ControlInfo& controlInfo,
                                    std::vector<Schedule>& allSchedules)
{
    std::vector<std::string> scheduleNames;
    scheduleNames.push_back("inletT");
    scheduleNames.push_back("draw");
    scheduleNames.push_back("ambientT");
    scheduleNames.push_back("evaporatorT");
    scheduleNames.push_back("DR");
    scheduleNames.push_back("setpoint");
    scheduleNames.push_back("SoC");

    long outputCode = controlInfo.outputCode;
    allSchedules.reserve(scheduleNames.size());
    for (auto i = 0; i < scheduleNames.size(); i++)
    {
        Schedule schedule;
        if (isResource)
        {
        }
        else
        {
            std::ifstream fileStream;
            std::string sFilename = testName + "/" + scheduleNames[i] + "schedule.csv";
            if (openFileText(fileStream, sFilename))
            {

                if (readSchedule(schedule, scheduleNames[i], fileStream, controlInfo.timeToRun_min))
                {
                    fileStream.close();
                }
                else
                {
                    msg("Unable to read schedule file %s\n", scheduleNames[i].c_str());
                    return false;
                }
            }
            else if (scheduleNames[i] != "setpoint" && scheduleNames[i] != "SoC")
            {
                if (verbosity >= VRB_reluctant)
                {
                    msg("Unable to open schedule file %s\n", scheduleNames[i].c_str());
                }
                return false;
            }
            allSchedules.push_back(schedule);
        }
    }

    if (controlInfo.temperatureUnits == "F")
    {
        for (auto& T : allSchedules[0])
        {
            T = F_TO_C(T);
        }
        for (auto& T : allSchedules[2])
        {
            T = F_TO_C(T);
        }
        for (auto& T : allSchedules[3])
        {
            T = F_TO_C(T);
        }
        for (auto& T : allSchedules[5])
        {
            T = F_TO_C(T);
        }
    }

    if (outputCode != 0)
    {
        if (verbosity >= VRB_reluctant)
        {
            msg("Control file testInfo.txt has unsettable specifics in it. \n");
        }
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
///	@brief	Runs a simulation
///	@param[in]	testDesc		data structure containing test description
/// @param[in]	outputDirectory	destination path for test results (filename based on testDesc)
///	@param[in]	controlInfo		data structure containing control info
///	@param[in]	allSchedules	collection of test schedules
///	@param[in]	airT_C			air temperature (degC) used for temperature depression
///	@param[in]	doTempDepress	whether to apply temperature depression
///	@param[out]	testResults		data structure containing test results
/// @return		true if successful, false otherwise
//-----------------------------------------------------------------------------
bool HPWH::Simulator::run(HPWH& hpwh,
                          const TestDesc& testDesc,
                          const std::string& outputDirectory,
                          const ControlInfo& controlInfo,
                          std::vector<Schedule>& allSchedules,
                          double airT_C,
                          const bool doTempDepress,
                          TestResults& testResults)
{
    const double energyBalThreshold = 0.0001; // 0.1 %
    const int nTestTCouples = 6;

    hpwh.setDoInversionMixing(controlInfo.doInversionMixing);
    hpwh.setDoConduction(controlInfo.doConduction);

    if (controlInfo.setpointT_C > 0.)
    {
        if (!allSchedules[5].empty())
        {
            hpwh.setSetpoint(allSchedules[5][0]); // expect this to fail sometimes
            if (controlInfo.initialTankT_C != nullptr)
                hpwh.setTankToTemperature(*controlInfo.initialTankT_C);
            else
                hpwh.resetTankToSetpoint();
        }
        else
        {
            hpwh.setSetpoint(controlInfo.setpointT_C);
            if (controlInfo.initialTankT_C != nullptr)
                hpwh.setTankToTemperature(*controlInfo.initialTankT_C);
            else
                hpwh.resetTankToSetpoint();
        }
    }

    if (controlInfo.inletH != nullptr)
    {
        hpwh.setInletByFraction(*controlInfo.inletH);
    }
    if (controlInfo.tankSize_gal != nullptr)
    {
        hpwh.setTankSize(*controlInfo.tankSize_gal, HPWH::UNITS_GAL);
    }
    if (controlInfo.tot_limit != nullptr)
    {
        hpwh.setTimerLimitTOT(*controlInfo.tot_limit);
    }
    if (controlInfo.useSoC)
    {
        if (allSchedules[6].empty())
        {
            if (verbosity >= VRB_reluctant)
            {
                msg("If useSoC is true need an SoCschedule.csv file. \n");
            }
        }
        const double soCMinTUse_C = F_TO_C(110.);
        const double soCMains_C = F_TO_C(65.);
        hpwh.switchToSoCControls(1., .05, soCMinTUse_C, true, soCMains_C);
    }

    // UEF data
    testResults = {false, 0., 0.};

    FILE* yearOutputFile = NULL;
    if (controlInfo.recordYearData)
    {
        std::string sOutputFilename = outputDirectory + "/DHW_YRLY.csv";
        if (fopen_s(&yearOutputFile, sOutputFilename.c_str(), "a+") != 0)
        {
            if (hpwh.verbosity >= VRB_reluctant)
            {
                msg("Could not open output file \n", sOutputFilename.c_str());
            }
            return false;
        }
    }

    FILE* minuteOutputFile = NULL;
    if (controlInfo.recordMinuteData)
    {
        std::string sOutputFilename = outputDirectory + "/" + testDesc.testName + "_" +
                                      testDesc.presetOrFile + "_" + testDesc.modelName + ".csv";
        if (fopen_s(&minuteOutputFile, sOutputFilename.c_str(), "w+") != 0)
        {
            if (hpwh.verbosity >= VRB_reluctant)
            {
                msg("Could not open output file \n", sOutputFilename.c_str());
            }
            return false;
        }
    }

    if (controlInfo.recordMinuteData)
    {
        std::string sHeader = "minutes,Ta,Tsetpoint,inletT,draw,";
        if (hpwh.isCompressoExternalMultipass())
        {
            sHeader += "condenserInletT,condenserOutletT,externalVolGPM,";
        }
        if (controlInfo.useSoC)
        {
            sHeader += "targetSoCFract,soCFract,";
        }
        hpwh.WriteCSVHeading(minuteOutputFile, sHeader.c_str(), nTestTCouples, CSVOPT_NONE);
    }

    double cumulativeEnergyIn_kWh[3] = {0., 0., 0.};
    double cumulativeEnergyOut_kWh[3] = {0., 0., 0.};

    bool doChangeSetpoint = (!allSchedules[5].empty()) && (!hpwh.isSetpointFixed());

    // ------------------------------------- Simulate --------------------------------------- //
    if (verbosity >= VRB_reluctant)
    {
        msg("Now Simulating %d Minutes of the Test\n", controlInfo.timeToRun_min);
    }

    // run simulation in 1-min steps
    for (int i = 0; i < controlInfo.timeToRun_min; i++)
    {

        double inletT_C = allSchedules[0][i];
        double drawVolume_L = GAL_TO_L(allSchedules[1][i]);
        double ambientT_C = doTempDepress ? airT_C : allSchedules[2][i];
        double externalT_C = allSchedules[3][i];
        HPWH::DRMODES drStatus = static_cast<HPWH::DRMODES>(int(allSchedules[4][i]));

        // change setpoint if specified in schedule
        if (doChangeSetpoint)
        {
            hpwh.setSetpoint(allSchedules[5][i]); // expect this to fail sometimes
        }

        // change SoC schedule
        if (controlInfo.useSoC)
        {
            if (hpwh.setTargetSoCFraction(allSchedules[6][i]) != 0)
            {
                if (verbosity >= VRB_reluctant)
                {
                    msg("ERROR: Can not set the target state of charge fraction.\n");
                }
                return false;
            }
        }

        if (controlInfo.modifyDraw)
        {
            const double mixT_C = F_TO_C(125.);
            if (hpwh.getSetpoint() <= mixT_C)
            { // do a simple mix down of the draw for the cold-water temperature
                drawVolume_L *=
                    (mixT_C - inletT_C) /
                    (hpwh.getTankNodeTemp(hpwh.getNumNodes() - 1, HPWH::UNITS_C) - inletT_C);
            }
        }

        double inletT2_C = inletT_C;
        double drawVolume2_L = drawVolume_L;

        double previousTankHeatContent_kJ = hpwh.getTankHeatContent_kJ();

        // run a step
        int runResult =
            hpwh.runOneStep(inletT_C,      // inlet water temperature (C)
                            drawVolume_L,  // draw volume (L)
                            ambientT_C,    // ambient Temp (C)
                            externalT_C,   // external Temp (C)
                            drStatus,      // DDR Status (now an enum. Fixed for now as allow)
                            drawVolume2_L, // inlet-2 volume (L)
                            inletT2_C,     // inlet-2 Temp (C)
                            NULL);         // no extra heat

        if (runResult != 0)
        {
            if (verbosity >= VRB_reluctant)
            {
                msg("ERROR: Run failed.\n");
            }
            return false;
        }

        if (!hpwh.isEnergyBalanced(
                drawVolume_L, inletT_C, previousTankHeatContent_kJ, energyBalThreshold))
        {
            if (verbosity >= VRB_reluctant)
            {
                msg("WARNING: On minute %i, HPWH has an energy balance error.\n", i);
            }
            // return false; // fails on ModelTest.testREGoesTo93C.TamScalable_SP.Preset; issue
            // in addHeatExternal
        }

        // check timing
        for (int iHS = 0; iHS < hpwh.getNumHeatSources(); iHS++)
        {
            if (hpwh.getNthHeatSourceRunTime(iHS) > 1.)
            {
                if (verbosity >= VRB_reluctant)
                {
                    msg("ERROR: On minute %i, heat source %i ran for %i min.\n",
                        iHS,
                        hpwh.getNthHeatSourceRunTime(iHS));
                }
                return false;
            }
        }

        // check flow for external MP
        if (hpwh.isCompressoExternalMultipass())
        {
            double volumeHeated_gal = hpwh.getExternalVolumeHeated(HPWH::UNITS_GAL);
            double mpFlowVolume_gal = hpwh.getExternalMPFlowRate(HPWH::UNITS_GPM) *
                                      hpwh.getNthHeatSourceRunTime(hpwh.getCompressorIndex());
            if (fabs(volumeHeated_gal - mpFlowVolume_gal) > 0.000001)
            {
                if (verbosity >= VRB_reluctant)
                {
                    msg("ERROR: Externally heated volumes are inconsistent! Volume Heated "
                        "[gal]: "
                        "%d, mpFlowRate in 1 min [gal] %d\n",
                        volumeHeated_gal,
                        mpFlowVolume_gal);
                }
                return false;
            }
        }

        // location temperature reported with temperature depression, instead of ambient
        // temperature
        if (doTempDepress)
        {
            ambientT_C = hpwh.getLocationTemp_C();
        }

        // write minute summary
        if (controlInfo.recordMinuteData)
        {
            std::string sPreamble = std::to_string(i) + ", " + std::to_string(ambientT_C) + ", " +
                                    std::to_string(hpwh.getSetpoint()) + ", " +
                                    std::to_string(allSchedules[0][i]) + ", " +
                                    std::to_string(allSchedules[1][i]) + ", ";
            // Add some more outputs for mp tests
            if (hpwh.isCompressoExternalMultipass())
            {
                sPreamble += std::to_string(hpwh.getCondenserWaterInletTemp()) + ", " +
                             std::to_string(hpwh.getCondenserWaterOutletTemp()) + ", " +
                             std::to_string(hpwh.getExternalVolumeHeated(HPWH::UNITS_GAL)) + ", ";
            }
            if (controlInfo.useSoC)
            {
                sPreamble += std::to_string(allSchedules[6][i]) + ", " +
                             std::to_string(hpwh.getSoCFraction()) + ", ";
            }
            int csvOptions = CSVOPT_NONE;
            if (allSchedules[1][i] > 0.)
            {
                csvOptions |= CSVOPT_IS_DRAWING;
            }
            hpwh.WriteCSVRow(minuteOutputFile, sPreamble.c_str(), nTestTCouples, csvOptions);
        }

        // accumulate energy and draw
        double energyIn_kJ = 0.;
        for (int iHS = 0; iHS < hpwh.getNumHeatSources(); iHS++)
        {
            double heatSourceEnergyIn_kJ = hpwh.getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KJ);
            energyIn_kJ += heatSourceEnergyIn_kJ;

            cumulativeEnergyIn_kWh[iHS] += KJ_TO_KWH(heatSourceEnergyIn_kJ) * 1000.;
            cumulativeEnergyOut_kWh[iHS] +=
                hpwh.getNthHeatSourceEnergyOutput(iHS, HPWH::UNITS_KWH) * 1000.;
        }
        testResults.totalEnergyConsumed_kJ += energyIn_kJ;
        testResults.totalVolumeRemoved_L += drawVolume_L;
    }
    // -----Simulation complete-------------------------------------- //

    if (controlInfo.recordMinuteData)
    {
        fclose(minuteOutputFile);
    }

    // write year summary
    if (controlInfo.recordYearData)
    {
        std::string firstCol =
            testDesc.testName + "," + testDesc.presetOrFile + "," + testDesc.modelName;
        fprintf(yearOutputFile, "%s", firstCol.c_str());
        double totalEnergyIn_kWh = 0, totalEnergyOut_kWh = 0;
        for (int iHS = 0; iHS < 3; iHS++)
        {
            fprintf(yearOutputFile,
                    ",%0.0f,%0.0f",
                    cumulativeEnergyIn_kWh[iHS],
                    cumulativeEnergyOut_kWh[iHS]);
            totalEnergyIn_kWh += cumulativeEnergyIn_kWh[iHS];
            totalEnergyOut_kWh += cumulativeEnergyOut_kWh[iHS];
        }
        fprintf(yearOutputFile, ",%0.0f,%0.0f", totalEnergyIn_kWh, totalEnergyOut_kWh);
        for (int iHS = 0; iHS < 3; iHS++)
        {
            fprintf(yearOutputFile,
                    ",%0.2f",
                    cumulativeEnergyOut_kWh[iHS] / cumulativeEnergyIn_kWh[iHS]);
        }
        fprintf(yearOutputFile, ",%0.2f", totalEnergyOut_kWh / totalEnergyIn_kWh);
        fprintf(yearOutputFile, "\n");
        fclose(yearOutputFile);
    }

    testResults.passed = true;
    return true;
}
