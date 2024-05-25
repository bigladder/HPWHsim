/*
Copyright (c) 2014-2016 Ecotope Inc.
All rights reserved.



Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:



* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.



* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.



* Neither the name of the copyright holders nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission from the copyright holders.



THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "HPWHsim.hh"
#include <btwxt/btwxt.h>
#include <fmt/format.h>

#include <stdarg.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include "hpwh_data_model.h"

using std::cout;
using std::endl;
using std::string;

const double HPWH::DENSITYWATER_kgperL = 0.995; /// mass density of water
const double HPWH::KWATER_WpermC = 0.62;        /// thermal conductivity of water
const double HPWH::CPWATER_kJperkgC = 4.180;    /// specific heat capcity of water

const double HPWH::TOL_MINVALUE = 0.0001;
const float HPWH::UNINITIALIZED_LOCATIONTEMP = -500.f;

const double HPWH::MAXOUTLET_R134A = F_TO_C(160.);
const double HPWH::MAXOUTLET_R410A = F_TO_C(140.);
const double HPWH::MAXOUTLET_R744 = F_TO_C(190.);
const double HPWH::MINSINGLEPASSLIFT = dF_TO_dC(15.);

std::unordered_map<HPWH::FirstHourRating::Desig, std::size_t> HPWH::firstDrawClusterSizes = {
    {HPWH::FirstHourRating::Desig::VerySmall, 5},
    {HPWH::FirstHourRating::Desig::Low, 3},
    {HPWH::FirstHourRating::Desig::Medium, 3},
    {HPWH::FirstHourRating::Desig::High, 4}};

std::unordered_map<HPWH::FirstHourRating::Desig, HPWH::DrawPattern> HPWH::drawPatterns = {
    {HPWH::FirstHourRating::Desig::VerySmall,
     {{HM_TO_MIN(0, 00), 7.6, 3.8},
      {HM_TO_MIN(1, 00), 3.8, 3.8},
      {HM_TO_MIN(1, 05), 1.9, 3.8},
      {HM_TO_MIN(1, 10), 1.9, 3.8},
      {HM_TO_MIN(1, 15), 1.9, 3.8},
      {HM_TO_MIN(8, 00), 3.8, 3.8},
      {HM_TO_MIN(8, 15), 7.6, 3.8},
      {HM_TO_MIN(9, 00), 5.7, 3.8},
      {HM_TO_MIN(9, 15), 3.8, 3.8}}},

    {HPWH::FirstHourRating::Desig::Low,
     {{HM_TO_MIN(0, 00), 56.8, 6.4},
      {HM_TO_MIN(0, 30), 7.6, 3.8},
      {HM_TO_MIN(1, 00), 3.8, 3.8},
      {HM_TO_MIN(10, 30), 22.7, 6.4},
      {HM_TO_MIN(11, 30), 15.1, 6.4},
      {HM_TO_MIN(12, 00), 3.8, 3.8},
      {HM_TO_MIN(12, 45), 3.8, 3.8},
      {HM_TO_MIN(12, 50), 3.8, 3.8},
      {HM_TO_MIN(16, 15), 7.6, 3.8},
      {HM_TO_MIN(16, 45), 7.6, 6.4},
      {HM_TO_MIN(17, 00), 11.4, 6.4}}},

    {HPWH::FirstHourRating::Desig::Medium,
     {{HM_TO_MIN(0, 00), 56.8, 6.4},
      {HM_TO_MIN(0, 30), 7.6, 3.8},
      {HM_TO_MIN(1, 40), 34.1, 6.4},
      {HM_TO_MIN(10, 30), 34.1, 6.4},
      {HM_TO_MIN(11, 30), 18.9, 6.4},
      {HM_TO_MIN(12, 00), 3.8, 3.8},
      {HM_TO_MIN(12, 45), 3.8, 3.8},
      {HM_TO_MIN(12, 50), 3.8, 3.8},
      {HM_TO_MIN(16, 00), 3.8, 3.8},
      {HM_TO_MIN(16, 15), 7.6, 3.8},
      {HM_TO_MIN(16, 45), 7.6, 6.4},
      {HM_TO_MIN(17, 00), 26.5, 6.4}}},

    {HPWH::FirstHourRating::Desig::High,
     {{HM_TO_MIN(0, 00), 102, 11.4},
      {HM_TO_MIN(0, 30), 7.6, 3.8},
      {HM_TO_MIN(0, 40), 3.8, 3.8},
      {HM_TO_MIN(1, 40), 34.1, 6.4},
      {HM_TO_MIN(10, 30), 56.8, 11.4},
      {HM_TO_MIN(11, 30), 18.9, 6.4},
      {HM_TO_MIN(12, 00), 3.8, 3.8},
      {HM_TO_MIN(12, 45), 3.8, 3.8},
      {HM_TO_MIN(12, 50), 3.8, 3.8},
      {HM_TO_MIN(16, 00), 7.6, 3.8},
      {HM_TO_MIN(16, 15), 7.6, 3.8},
      {HM_TO_MIN(16, 30), 7.6, 6.4},
      {HM_TO_MIN(16, 45), 7.6, 6.4},
      {HM_TO_MIN(17, 00), 53.0, 11.4}}}};

const int HPWH::HPWH_ABORT = -274000;

void HPWH::setMinutesPerStep(const double minutesPerStep_in)
{
    minutesPerStep = minutesPerStep_in;
    secondsPerStep = sec_per_min * minutesPerStep;
    hoursPerStep = minutesPerStep / min_per_hr;
}

// public HPWH functions
HPWH::HPWH(const std::shared_ptr<Courier::Courier>& courier) : Sender("HPWH", courier)
{
    tank = std::make_shared<Tank>(this, courier);
    setAllDefaults();
}

HPWH::HPWH(const HPWH& hpwh) : Sender("HPWH", hpwh.courier) { *this = hpwh; }

void HPWH::setAllDefaults()
{
    tank->setAllDefaults();
    heatSources.clear();

    isHeating = false;
    setpointFixed = false;

    member_inletT_C = HPWH_ABORT;
    currentSoCFraction = 1.;
    doTempDepression = false;
    locationTemperature_C = UNINITIALIZED_LOCATIONTEMP;

    fittingsUA_kJperHrC = 0.;
    prevDRstatus = DR_ALLOW;
    timerLimitTOT = 60.;
    timerTOT = 0.;
    usesSoCLogic = false;
    setMinutesPerStep(1.0);
}

HPWH& HPWH::operator=(const HPWH& hpwh)
{
    if (this == &hpwh)
    {
        return *this;
    }

    Sender::operator=(hpwh);
    isHeating = hpwh.isHeating;

    tank = hpwh.tank;
    tank->hpwh = this;

    heatSources = hpwh.heatSources;
    for (auto& heatSource : heatSources)
    {
        heatSource.hpwh = this;
    }

    fittingsUA_kJperHrC = hpwh.fittingsUA_kJperHrC;

    setpoint_C = hpwh.setpoint_C;

    condenserInlet_C = hpwh.condenserInlet_C;
    condenserOutlet_C = hpwh.condenserOutlet_C;
    externalVolumeHeated_L = hpwh.externalVolumeHeated_L;
    energyRemovedFromEnvironment_kWh = hpwh.energyRemovedFromEnvironment_kWh;
    standbyLosses_kWh = hpwh.standbyLosses_kWh;

    doTempDepression = hpwh.doTempDepression;

    locationTemperature_C = hpwh.locationTemperature_C;

    prevDRstatus = hpwh.prevDRstatus;
    timerLimitTOT = hpwh.timerLimitTOT;

    usesSoCLogic = hpwh.usesSoCLogic;

    return *this;
}

HPWH::~HPWH() {}

int HPWH::runOneStep(double drawVolume_L,
                     double tankAmbientT_C,
                     double heatSourceAmbientT_C,
                     DRMODES DRstatus,
                     double inletVol2_L,
                     double inletT2_C,
                     std::vector<double>* extraHeatDist_W)
{
    // returns 0 on successful completion, HPWH_ABORT on failure

    // check for errors
    if (doTempDepression && (minutesPerStep != 1))
    {
        send_error("minutesPerStep must equal one for temperature depression to work.");
    }

    if ((DRstatus & (DR_TOO | DR_TOT)))
    {
        send_warning(
            "DR_TOO | DR_TOT use conflicting logic sets. The logic will follow a DR_TOT scheme.");
    }

    // reset the output variables
    tank->setOutletT_C(0.);
    condenserInlet_C = 0.;
    condenserOutlet_C = 0.;
    externalVolumeHeated_L = 0.;
    energyRemovedFromEnvironment_kWh = 0.;
    standbyLosses_kWh = 0.;
    tank->standbyLosses_kJ = 0.;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].runtime_min = 0;
        heatSources[i].energyInput_kWh = 0.;
        heatSources[i].energyOutput_kWh = 0.;
    }
    extraEnergyInput_kWh = 0.;

    // if you are doing temp. depression, set tank and heatSource ambient temps
    // to the tracked locationTemperature
    double temperatureGoal = tankAmbientT_C;
    if (doTempDepression)
    {
        if (locationTemperature_C == UNINITIALIZED_LOCATIONTEMP)
        {
            locationTemperature_C = tankAmbientT_C;
        }
        tankAmbientT_C = locationTemperature_C;
        heatSourceAmbientT_C = locationTemperature_C;
    }

    // process draws and standby losses
    tank->updateNodes(drawVolume_L, member_inletT_C, tankAmbientT_C, inletVol2_L, inletT2_C);

    updateSoCIfNecessary();

    // First Logic DR checks //////////////////////////////////////////////////////////////////

    // If the DR signal includes a top off but the previous signal did not, then top it off!
    if ((DRstatus & DR_LOC) != 0 && (DRstatus & DR_LOR) != 0)
    {
        turnAllHeatSourcesOff(); // turns off isheating
    }
    else
    { // do normal check
        if (((DRstatus & DR_TOO) != 0 || (DRstatus & DR_TOT) != 0) && timerTOT == 0)
        {

            // turn on the compressor and last resistance element.
            if (hasACompressor())
            {
                heatSources[compressorIndex].engageHeatSource(DRstatus);
            }
            if (lowestElementIndex >= 0)
            {
                heatSources[lowestElementIndex].engageHeatSource(DRstatus);
            }
        }

        // do HeatSource choice
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (isHeating)
            {
                // check if anything that is on needs to turn off (generally for lowT cutoffs)
                // things that just turn on later this step are checked for this in shouldHeat
                if (heatSources[i].isEngaged() && heatSources[i].shutsOff())
                {
                    heatSources[i].disengageHeatSource();
                    // check if the backup heat source would have to shut off too
                    if ((heatSources[i].backupHeatSource != NULL) &&
                        !heatSources[i].backupHeatSource->shutsOff())
                    {
                        // and if not, go ahead and turn it on
                        heatSources[i].backupHeatSource->engageHeatSource(DRstatus);
                    }
                }

                // if there's a priority HeatSource (e.g. upper resistor) and it needs to
                // come on, then turn  off and start it up
                if (heatSources[i].isVIP)
                {
                    if (heatSources[i].shouldHeat())
                    {
                        if (shouldDRLockOut(heatSources[i].typeOfHeatSource, DRstatus))
                        {
                            if (hasACompressor())
                            {
                                heatSources[compressorIndex].engageHeatSource(DRstatus);
                                break;
                            }
                        }
                        else
                        {
                            turnAllHeatSourcesOff();
                            heatSources[i].engageHeatSource(DRstatus);
                            // stop looking if the VIP needs to run
                            break;
                        }
                    }
                }
            }
            // if nothing is currently on, then check if something should come on
            else /* (isHeating == false) */
            {
                if (heatSources[i].shouldHeat())
                {
                    heatSources[i].engageHeatSource(DRstatus);
                    // engaging a heat source sets isHeating to true, so this will only trigger once
                }
            }

        } // end loop over heat sources

        // do heating logic
        double minutesToRun = minutesPerStep;
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            // check/apply lock-outs
            if (shouldDRLockOut(heatSources[i].typeOfHeatSource, DRstatus))
            {
                heatSources[i].lockOutHeatSource();
            }
            else
            {
                // locks or unlocks the heat source
                heatSources[i].toLockOrUnlock(heatSourceAmbientT_C);
            }
            if (heatSources[i].isLockedOut() && heatSources[i].backupHeatSource == NULL)
            {
                heatSources[i].disengageHeatSource();
                send_warning("lock-out triggered, but no backupHeatSource defined. Simulation will "
                             "continue with the heat source locked-out.");
            }

            // going through in order, check if the heat source is on
            if (heatSources[i].isEngaged())
            {

                HeatSource* heatSourcePtr;
                if (heatSources[i].isLockedOut() && heatSources[i].backupHeatSource != NULL)
                {

                    // Check that the backup isn't locked out too or already engaged then it will
                    // heat on its own.
                    if (heatSources[i].backupHeatSource->toLockOrUnlock(heatSourceAmbientT_C) ||
                        shouldDRLockOut(heatSources[i].backupHeatSource->typeOfHeatSource,
                                        DRstatus) || //){
                        heatSources[i].backupHeatSource->isEngaged())
                    {
                        continue;
                    }
                    // Don't turn the backup electric resistance heat source on if the VIP
                    // resistance element is on .
                    else if (VIPIndex >= 0 && heatSources[VIPIndex].isOn &&
                             heatSources[i].backupHeatSource->isAResistance())
                    {
                        continue;
                    }
                    else
                    {
                        heatSourcePtr = heatSources[i].backupHeatSource;
                    }
                }
                else
                {
                    heatSourcePtr = &heatSources[i];
                }

                addHeatParent(heatSourcePtr, heatSourceAmbientT_C, minutesToRun);

                // if it finished early. i.e. shuts off early like if the heatsource met setpoint or
                // maxed out
                if (heatSourcePtr->runtime_min < minutesToRun)
                {
                    // subtract time it ran and turn it off
                    minutesToRun -= heatSourcePtr->runtime_min;
                    heatSources[i].disengageHeatSource();
                    // and if there's a heat source that follows this heat source (regardless of
                    // lockout) that's able to come on,
                    if ((heatSources[i].followedByHeatSource != NULL) &&
                        !heatSources[i].followedByHeatSource->shutsOff())
                    {
                        // turn it on
                        heatSources[i].followedByHeatSource->engageHeatSource(DRstatus);
                    }
                    // or if there heat source can't produce hotter water (i.e. it's maxed out) and
                    // the tank still isn't at setpoint. the compressor should get locked out when
                    // the maxedOut is true but have to run the resistance first during this
                    // timestep to make sure tank is above the max temperature for the compressor.
                    else if (heatSources[i].maxedOut() && heatSources[i].backupHeatSource != NULL)
                    {

                        // Check that the backup isn't locked out or already engaged then it will
                        // heat or already heated on its own.
                        if (!heatSources[i].backupHeatSource->toLockOrUnlock(
                                heatSourceAmbientT_C) && // If not locked out
                            !shouldDRLockOut(heatSources[i].backupHeatSource->typeOfHeatSource,
                                             DRstatus) && // and not DR locked out
                            !heatSources[i].backupHeatSource->isEngaged())
                        { // and not already engaged

                            HeatSource* backupHeatSourcePtr = heatSources[i].backupHeatSource;
                            // turn it on
                            backupHeatSourcePtr->engageHeatSource(DRstatus);
                            // add heat if it hasn't heated up this whole minute already
                            if (backupHeatSourcePtr->runtime_min >= 0.)
                            {
                                addHeatParent(backupHeatSourcePtr,
                                              heatSourceAmbientT_C,
                                              minutesToRun - backupHeatSourcePtr->runtime_min);
                            }
                        }
                    }
                }
            } // heat source not engaged
        }     // end while iHS heat source
    }
    if (areAllHeatSourcesOff())
    {
        isHeating = false;
    }
    // If there's extra user defined heat to add -> Add extra heat!
    if ((extraHeatDist_W != NULL) && (extraHeatDist_W->size() != 0))
    {
        addExtraHeat(*extraHeatDist_W);
        updateSoCIfNecessary();
    }

    // track the depressed local temperature
    if (doTempDepression)
    {
        bool compressorRan = false;
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i].isEngaged() && !heatSources[i].isLockedOut() &&
                heatSources[i].depressesTemperature)
            {
                compressorRan = true;
            }
        }

        if (compressorRan)
        {
            temperatureGoal -= maxDepression_C; // hardcoded 4.5 degree total drop - from
                                                // experimental data. Changed to an input
        }
        else
        {
            // otherwise, do nothing, we're going back to ambient
        }

        // shrink the gap by the same percentage every minute - that gives us
        // exponential behavior the percentage was determined by a fit to
        // experimental data - 9.4 minute half life and 4.5 degree total drop
        // minus-equals is important, and fits with the order of locationTemperature
        // and temperatureGoal, so as to not use fabs() and conditional tests
        locationTemperature_C -= (locationTemperature_C - temperatureGoal) * (1 - 0.9289);
    }

    // settle outputs

    // outletTemp_C and standbyLosses_kWh are taken care of in updateTankTemps
    standbyLosses_kWh = KJ_TO_KWH(tank->standbyLosses_kJ);

    // sum energyRemovedFromEnvironment_kWh for each heat source;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        energyRemovedFromEnvironment_kWh +=
            (heatSources[i].energyOutput_kWh - heatSources[i].energyInput_kWh);
    }

    tank->checkForInversion();

    // Handle DR timer
    prevDRstatus = DRstatus;
    // DR check for TOT to increase timer.
    timerTOT += minutesPerStep;
    // Restart the time if we're over the limit or the command is not a top off.
    if ((DRstatus & DR_TOT) != 0 && timerTOT >= timerLimitTOT)
    {
        resetTopOffTimer();
    }
    else if ((DRstatus & DR_TOO) == 0 && (DRstatus & DR_TOT) == 0)
    {
        resetTopOffTimer();
    }

    return 0; // successful completion of the step returns 0
} // end runOneStep

int HPWH::runNSteps(int N,
                    double* inletT_C,
                    double* drawVolume_L,
                    double* tankAmbientT_C,
                    double* heatSourceAmbientT_C,
                    DRMODES* DRstatus)
{
    // returns 0 on successful completion, HPWH_ABORT on failure

    // these are all the accumulating variables we'll need
    double energyRemovedFromEnvironment_kWh_SUM = 0;
    double standbyLosses_kWh_SUM = 0;
    double outletTemp_C_AVG = 0;
    double totalDrawVolume_L = 0;
    std::vector<double> heatSources_runTimes_SUM(getNumHeatSources());
    std::vector<double> heatSources_energyInputs_SUM(getNumHeatSources());
    std::vector<double> heatSources_energyOutputs_SUM(getNumHeatSources());

    // run the sim one step at a time, accumulating the outputs as you go
    for (int i = 0; i < N; i++)
    {

        runOneStep(
            inletT_C[i], drawVolume_L[i], tankAmbientT_C[i], heatSourceAmbientT_C[i], DRstatus[i]);

        energyRemovedFromEnvironment_kWh_SUM += energyRemovedFromEnvironment_kWh;
        standbyLosses_kWh_SUM += standbyLosses_kWh;

        outletTemp_C_AVG += tank->getOutletT_C() * drawVolume_L[i];
        totalDrawVolume_L += drawVolume_L[i];

        for (int j = 0; j < getNumHeatSources(); j++)
        {
            heatSources_runTimes_SUM[j] += getNthHeatSourceRunTime(j);
            heatSources_energyInputs_SUM[j] += getNthHeatSourceEnergyInput(j);
            heatSources_energyOutputs_SUM[j] += getNthHeatSourceEnergyOutput(j);
        }
    }
    // finish weighted avg. of outlet temp by dividing by the total drawn volume
    outletTemp_C_AVG /= totalDrawVolume_L;

    // now, reassign all accumulated values to their original spots
    energyRemovedFromEnvironment_kWh = energyRemovedFromEnvironment_kWh_SUM;
    standbyLosses_kWh = standbyLosses_kWh_SUM;
    tank->setOutletT_C(outletTemp_C_AVG);

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].runtime_min = heatSources_runTimes_SUM[i];
        heatSources[i].energyInput_kWh = heatSources_energyInputs_SUM[i];
        heatSources[i].energyOutput_kWh = heatSources_energyOutputs_SUM[i];
    }

    return 0;
}

void HPWH::addHeatParent(HeatSource* heatSourcePtr,
                         double heatSourceAmbientT_C,
                         double minutesToRun)
{

    double tempSetpoint_C = -273.15;

    // Check the air temprature and setpoint against maxOut_at_LowT
    if (heatSourcePtr->isACompressor())
    {
        if (heatSourceAmbientT_C <= heatSourcePtr->maxOut_at_LowT.airT_C &&
            setpoint_C >= heatSourcePtr->maxOut_at_LowT.outT_C)
        {
            tempSetpoint_C = setpoint_C; // Store setpoint
            setSetpoint(heatSourcePtr->maxOut_at_LowT
                            .outT_C); // Reset to new setpoint as this is used in the add heat calc
        }
    }
    // and add heat if it is
    heatSourcePtr->addHeat(heatSourceAmbientT_C, minutesToRun);

    // Change the setpoint back to what it was pre-compressor depression
    if (tempSetpoint_C > -273.15)
    {
        setSetpoint(tempSetpoint_C);
    }
}

// public members to write to CSV file
int HPWH::WriteCSVHeading(std::ofstream& outFILE,
                          const char* preamble,
                          int nTCouples,
                          int options) const
{

    bool doIP = (options & CSVOPT_IPUNITS) != 0;

    outFILE << preamble;

    outFILE << "DRstatus";

    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        outFILE << fmt::format(",h_src{}In (Wh),h_src{}Out (Wh)", iHS + 1, iHS + 1);
    }

    for (int iTC = 0; iTC < nTCouples; iTC++)
    {
        outFILE << fmt::format(",tcouple{} ({})", iTC + 1, doIP ? "F" : "C");
    }

    outFILE << fmt::format(",toutlet ({})", doIP ? "F" : "C") << std::endl;

    return 0;
}

int HPWH::WriteCSVRow(std::ofstream& outFILE,
                      const char* preamble,
                      int nTCouples,
                      int options) const
{

    bool doIP = (options & CSVOPT_IPUNITS) != 0;

    outFILE << preamble;

    outFILE << prevDRstatus;

    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        outFILE << fmt::format(",{:0.2f},{:0.2f}",
                               getNthHeatSourceEnergyInput(iHS, UNITS_KWH) * 1000.,
                               getNthHeatSourceEnergyOutput(iHS, UNITS_KWH) * 1000.);
    }

    for (int iTC = 0; iTC < nTCouples; iTC++)
    {
        outFILE << fmt::format(",{:0.2f}",
                               getNthSimTcouple(iTC + 1, nTCouples, doIP ? UNITS_F : UNITS_C));
    }

    if (options & HPWH::CSVOPT_IS_DRAWING)
    {
        outFILE << fmt::format(",{:0.2f}",
                               doIP ? C_TO_F(tank->getOutletT_C()) : tank->getOutletT_C());
    }
    else
    {
        outFILE << ",";
    }

    outFILE << std::endl;

    return 0;
}

int HPWH::writeRowAsCSV(std::ofstream& outFILE,
                        OutputData& outputData,
                        const CSVOPTIONS& options /* = CSVOPTIONS::CSVOPT_NONE */) const
{
    bool doIP = (options & CSVOPT_IPUNITS) != 0;

    //
    outFILE << fmt::format("{}", outputData.time_min);
    outFILE << fmt::format(",{:0.2f}",
                           doIP ? C_TO_F(outputData.ambientT_C) : outputData.ambientT_C);
    outFILE << fmt::format(",{:0.2f}",
                           doIP ? C_TO_F(outputData.setpointT_C) : outputData.setpointT_C);
    outFILE << fmt::format(",{:0.2f}", doIP ? C_TO_F(outputData.inletT_C) : outputData.inletT_C);
    outFILE << fmt::format(",{:0.2f}",
                           doIP ? L_TO_GAL(outputData.drawVolume_L) : outputData.drawVolume_L);
    outFILE << fmt::format(",{}", static_cast<int>(outputData.drMode));

    //
    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        outFILE << fmt::format(",{:0.2f},{:0.2f}",
                               outputData.h_srcIn_kWh[iHS] * 1000.,
                               outputData.h_srcOut_kWh[iHS] * 1000.);
    }

    //
    for (auto thermocoupleT_C : outputData.thermocoupleT_C)
    {
        outFILE << fmt::format(",{:0.2f}", doIP ? C_TO_F(thermocoupleT_C) : thermocoupleT_C);
    }

    //
    if (outputData.drawVolume_L > 0.)
    {
        outFILE << fmt::format(",{:0.2f}",
                               doIP ? C_TO_F(outputData.outletT_C) : outputData.outletT_C);
    }
    else
    {
        outFILE << ",";
    }

    outFILE << std::endl;
    return 0;
}

bool HPWH::isSetpointFixed() const { return setpointFixed; }

int HPWH::setSetpoint(double newSetpoint, UNITS units /*=UNITS_C*/)
{

    double newSetpoint_C, temp;
    string why;
    if (units == UNITS_C)
    {
        newSetpoint_C = newSetpoint;
    }
    else if (units == UNITS_F)
    {
        newSetpoint_C = F_TO_C(newSetpoint);
    }
    else
    {
        send_warning(fmt::format("Incorrect unit specification for setSetpoint.", 0));
        return HPWH_ABORT;
    }
    if (!isNewSetpointPossible(newSetpoint_C, temp, why))
    {
        send_warning(fmt::format(
            fmt::format("Unwilling to set this setpoint for the currently selected model, "
                        "max setpoint is {} C. {}",
                        temp,
                        why.c_str())));
        return HPWH_ABORT;
    }

    setpoint_C = newSetpoint_C;

    return 0;
}

double HPWH::getSetpoint(UNITS units /*=UNITS_C*/) const
{
    if (units == UNITS_C)
    {
        return setpoint_C;
    }
    else if (units == UNITS_F)
    {
        return C_TO_F(setpoint_C);
    }
    else
    {
        send_warning(fmt::format("Incorrect unit specification for getSetpoint."));
        return HPWH_ABORT;
    }
}

double HPWH::getMaxCompressorSetpoint(UNITS units /*=UNITS_C*/) const
{

    if (!hasACompressor())
    {
        send_warning("Unit does not have a compressor.");
        return HPWH_ABORT;
    }

    double returnVal = heatSources[compressorIndex].maxSetpoint_C;

    if (units == UNITS_F)
    {
        returnVal = C_TO_F(returnVal);
    }
    else if (units != UNITS_C)
    {
        send_warning("Incorrect unit specification for getMaxCompressorSetpoint.");
        return HPWH_ABORT;
    }
    return returnVal;
}

bool HPWH::isNewSetpointPossible(double newSetpoint,
                                 double& maxAllowedSetpoint,
                                 string& why,
                                 UNITS units /*=UNITS_C*/) const
{
    double newSetpoint_C;
    double maxAllowedSetpoint_C = -273.15;
    if (units == UNITS_C)
    {
        newSetpoint_C = newSetpoint;
    }
    else if (units == UNITS_F)
    {
        newSetpoint_C = F_TO_C(newSetpoint);
    }
    else
    {
        send_warning("Incorrect unit specification for isNewSetpointPossible.");
        return false;
    }
    bool returnVal = false;

    if (isSetpointFixed())
    {
        returnVal = (newSetpoint == setpoint_C);
        maxAllowedSetpoint_C = setpoint_C;
        if (!returnVal)
        {
            why = "The set point is fixed for the currently selected model.";
        }
    }
    else
    {

        if (hasACompressor())
        { // If there's a compressor lets check the new setpoint against the compressor's max
          // setpoint

            maxAllowedSetpoint_C =
                heatSources[compressorIndex].maxSetpoint_C -
                heatSources[compressorIndex].secondaryHeatExchanger.hotSideTemperatureOffset_dC;

            if (newSetpoint_C > maxAllowedSetpoint_C && lowestElementIndex == -1)
            {
                why = "The compressor cannot meet the setpoint temperature and there is no "
                      "resistance backup.";
                returnVal = false;
            }
            else
            {
                returnVal = true;
            }
        }
        if (lowestElementIndex >= 0)
        { // If there's a resistance element lets check the new setpoint against the its max
          // setpoint
            maxAllowedSetpoint_C = heatSources[lowestElementIndex].maxSetpoint_C;

            if (newSetpoint_C > maxAllowedSetpoint_C)
            {
                why = "The resistance elements cannot produce water this hot.";
                returnVal = false;
            }
            else
            {
                returnVal = true;
            }
        }
        else if (lowestElementIndex == -1 && !hasACompressor())
        { // There are no heat sources here!
            if (model == MODELS_StorageTank)
            {
                returnVal = true; // The one pass the storage tank doesn't have any heating elements
                                  // so sure change the setpoint it does nothing!
            }
            else
            {
                why = "There aren't any heat sources to check the new setpoint against!";
                returnVal = false;
            }
        }
    }

    if (units == UNITS_C)
    {
        maxAllowedSetpoint = maxAllowedSetpoint_C;
    }
    else if (units == UNITS_F)
    {
        maxAllowedSetpoint = C_TO_F(maxAllowedSetpoint_C);
    }
    return returnVal;
}

double HPWH::getSoCFraction() const { return currentSoCFraction; }

double HPWH::calcSoCFraction(double tMains_C, double tMinUseful_C, double tMax_C) const
{
    return tank->calcSoCFraction(tMains_C, tMinUseful_C, tMax_C);
}

double HPWH::calcSoCFraction(double tMains_C, double tMinUseful_C) const
{
    return calcSoCFraction(tMains_C, tMinUseful_C, getSetpoint());
}

void HPWH::calcAndSetSoCFraction()
{
    double newSoCFraction = -1.;

    std::shared_ptr<SoCBasedHeatingLogic> logicSoC =
        std::dynamic_pointer_cast<SoCBasedHeatingLogic>(
            heatSources[compressorIndex].turnOnLogicSet[0]);
    newSoCFraction = calcSoCFraction(logicSoC->getMainsT_C(), logicSoC->getTempMinUseful_C());

    currentSoCFraction = newSoCFraction;
}

double HPWH::getMinOperatingTemp(UNITS units /*=UNITS_C*/) const
{
    if (!hasACompressor())
    {
        send_warning("No compressor found in this HPWH.");
        return HPWH_ABORT;
    }
    if (units == UNITS_C)
    {
        return heatSources[compressorIndex].minT;
    }
    else if (units == UNITS_F)
    {
        return C_TO_F(heatSources[compressorIndex].minT);
    }
    else
    {
        send_warning("Incorrect unit specification for getMinOperatingTemp.");
        return HPWH_ABORT;
    }
}

int HPWH::resetTankToSetpoint() { return tank->setNodeT_C(setpoint_C); }

//-----------------------------------------------------------------------------
///	@brief	Assigns new temps provided from a std::vector to tankTemps_C.
/// @param[in]	setTankTemps	new tank temps (arbitrary non-zero size)
///	@param[in]	units          temp units in setTankTemps (default = UNITS_C)
/// @return	Success: 0; Failure: HPWH_ABORT
//-----------------------------------------------------------------------------
int HPWH::setTankLayerTemperatures(std::vector<double> setTankTemps, const UNITS units)
{
    if ((units != UNITS_C) && (units != UNITS_F))
    {
        send_warning("Incorrect unit specification for setSetpoint.");
        return HPWH_ABORT;
    }

    if (units == UNITS_F)
        for (auto& T : setTankTemps)
            T = F_TO_C(T);

    return tank->setNodeTs_C(setTankTemps);
}

void HPWH::getTankTemps(std::vector<double>& tankTemps) { tank->getNodeTs_C(tankTemps); }

int HPWH::setAirFlowFreedom(double fanFraction)
{
    if (fanFraction < 0 || fanFraction > 1)
    {
        send_error("You have attempted to set the fan fraction outside of bounds.");
        return HPWH_ABORT;
    }
    else
    {
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i].isACompressor())
            {
                heatSources[i].airflowFreedom = fanFraction;
            }
        }
    }
    return 0;
}

int HPWH::setDoTempDepression(bool doTempDepress)
{
    this->doTempDepression = doTempDepress;
    return 0;
}

int HPWH::setTankSize_adjustUA(double volume, UNITS units /*=UNITS_L*/, bool forceChange /*=false*/)
{
    if (units == UNITS_L)
    {
    }
    else if (units == UNITS_GAL)
    {
        volume = GAL_TO_L(volume);
    }
    else
    {
        send_warning("Incorrect unit specification for setTankSize_adjustUA.");
        return HPWH_ABORT;
    }
    return tank->setVolumeAndAdjustUA(volume, forceChange);
}

/*static*/ double
HPWH::getTankSurfaceArea(double vol, UNITS volUnits /*=UNITS_L*/, UNITS surfAUnits /*=UNITS_FT2*/)
{
    if (volUnits == UNITS_L)
    {
    }
    else if (volUnits == UNITS_GAL)
    {
        vol = GAL_TO_L(vol);
    }
    else
    {
        return HPWH_ABORT;
    }
    double value = Tank::getSurfaceArea_m2(vol);
    if (surfAUnits == UNITS_M2)
    {
    }
    else if (surfAUnits == UNITS_FT2)
    {
        value = M2_TO_FT2(value);
    }
    else
    {
        return HPWH_ABORT;
    }
    return value;
}

double HPWH::getTankSurfaceArea(UNITS units /*=UNITS_FT2*/) const
{
    double value = Tank::getSurfaceArea_m2(tank->volume_L);
    if (value < 0.)
    {
        send_warning(fmt::format("Invalid volume for getTankSurfaceArea.", 0));
        value = HPWH_ABORT;
    }
    if (units == UNITS_M2)
    {
    }
    else if (units == UNITS_FT2)
    {
        value = M2_TO_FT2(value);
    }
    else
    {
        send_warning("Incorrect unit specification for setTankSize_adjustUA.");
        return HPWH_ABORT;
    }
    return value;
}

/*static*/ double
HPWH::getTankRadius(double vol, UNITS volUnits /*=UNITS_L*/, UNITS radiusUnits /*=UNITS_FT*/)
{
    if (volUnits == UNITS_L)
    {
    }
    else if (volUnits == UNITS_GAL)
    {
        vol = GAL_TO_L(vol);
    }
    else
    {
        return HPWH_ABORT;
    }
    double radius = Tank::getRadius_m(vol);
    if (radiusUnits == UNITS_M)
    {
    }
    else if (radiusUnits == UNITS_FT)
    {
        radius = FT_TO_M(radius);
    }
    else
    {
        return HPWH_ABORT;
    }
    return radius;
}

double HPWH::getTankRadius(UNITS units /*=UNITS_FT*/) const
{
    double value = getTankRadius(tank->getVolume_L(), UNITS_L, units);
    if (value < 0.)
    {
        send_warning("Incorrect unit specification for getTankRadius.");
        value = HPWH_ABORT;
    }
    return value;
}

bool HPWH::isTankSizeFixed() const { return tank->volumeFixed; }

int HPWH::setTankSize(double HPWH_size, UNITS units /*=UNITS_L*/, bool forceChange /*=false*/)
{
    if (units == UNITS_L)
    {
    }
    else if (units == UNITS_GAL)
    {
        HPWH_size = GAL_TO_L(HPWH_size);
    }
    else
    {
        send_warning("Incorrect unit specification for setTankSize.");
        return HPWH_ABORT;
    }
    tank->setVolume_L(HPWH_size, forceChange);

    tank->calcSizeConstants();

    return 0;
}

int HPWH::setDoInversionMixing(bool doInvMix) { return tank->setDoInversionMixing(doInvMix); }

int HPWH::setDoConduction(bool doCondu) { return tank->setDoConduction(doCondu); }

int HPWH::setUA(double UA, UNITS units /*=UNITS_kJperHrC*/)
{
    if (units == UNITS_kJperHrC)
    {
    }
    else if (units == UNITS_BTUperHrF)
    {
        UA = UAf_TO_UAc(UA);
    }
    else
    {
        send_warning("Incorrect unit specification for setUA.");
        return HPWH_ABORT;
    }
    tank->setUA_kJperHrC(UA);
    return 0;
}

int HPWH::getUA(double& UA, UNITS units /*=UNITS_kJperHrC*/) const
{
    UA = tank->getUA_kJperHrC();
    if (units == UNITS_kJperHrC)
    {
    }
    else if (units == UNITS_BTUperHrF)
    {
        UA = UA / UAf_TO_UAc(1.);
    }
    else
    {
        send_warning("Incorrect unit specification for getUA.");
        UA = -1.;
        return HPWH_ABORT;
    }
    return 0;
}

int HPWH::setFittingsUA(double UA, UNITS units /*=UNITS_kJperHrC*/)
{
    if (units == UNITS_kJperHrC)
    {
    }
    else if (units == UNITS_BTUperHrF)
    {
        UA = UAf_TO_UAc(UA);
    }
    else
    {
        send_warning("Incorrect unit specification for setFittingsUA.");
        return HPWH_ABORT;
    }
    fittingsUA_kJperHrC = UA;
    return 0;
}

int HPWH::getFittingsUA(double& UA, UNITS units /*=UNITS_kJperHrC*/) const
{
    UA = fittingsUA_kJperHrC;
    if (units == UNITS_kJperHrC)
    {
        // UA is already in correct units
    }
    else if (units == UNITS_BTUperHrF)
    {
        UA = UA / UAf_TO_UAc(1.);
    }
    else
    {
        send_warning("Incorrect unit specification for getUA.");
        UA = -1.;
        return HPWH_ABORT;
    }
    return 0;
}

int HPWH::setExternalInletHeightByFraction(double fractionalHeight)
{
    return setExternalPortHeightByFraction(fractionalHeight, 1);
}

int HPWH::setExternalOutletHeightByFraction(double fractionalHeight)
{
    return setExternalPortHeightByFraction(fractionalHeight, 2);
}

int HPWH::setExternalPortHeightByFraction(double fractionalHeight, int whichExternalPort)
{
    if (!hasExternalHeatSource())
    {
        send_warning("Does not have an external heat source.");
        return HPWH_ABORT;
    }

    int returnVal = 0;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].configuration == HeatSource::CONFIG_EXTERNAL)
        {
            if (whichExternalPort == 1)
            {
                returnVal = tank->setNodeNumFromFractionalHeight(
                    fractionalHeight, heatSources[i].externalInletHeight);
            }
            else
            {
                returnVal = tank->setNodeNumFromFractionalHeight(
                    fractionalHeight, heatSources[i].externalOutletHeight);
            }

            if (returnVal == HPWH_ABORT)
            {
                return returnVal;
            }
        }
    }
    return returnVal;
}

int HPWH::getExternalInletHeight() const
{
    if (!hasExternalHeatSource())
    {
        send_warning("Does not have an external heat source.");
        return HPWH_ABORT;
    }
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].configuration == HeatSource::CONFIG_EXTERNAL)
        {
            return heatSources[i].externalInletHeight; // Return the first one since all external
                                                       // sources have some ports
        }
    }
    return HPWH_ABORT;
}

int HPWH::getExternalOutletHeight() const
{
    if (!hasExternalHeatSource())
    {
        send_warning("Does not have an external heat source.");
        return HPWH_ABORT;
    }
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].configuration == HeatSource::CONFIG_EXTERNAL)
        {
            return heatSources[i].externalOutletHeight; // Return the first one since all external
                                                        // sources have some ports
        }
    }
    return HPWH_ABORT;
}

int HPWH::setTimerLimitTOT(double limit_min)
{
    if (limit_min > 24. * 60. || limit_min < 0.)
    {
        send_warning("Out of bounds time limit for setTimerLimitTOT.");
        return HPWH_ABORT;
    }

    timerLimitTOT = limit_min;

    return 0;
}

double HPWH::getTimerLimitTOT_minute() const { return timerLimitTOT; }

int HPWH::getInletHeight(int whichInlet) const { return tank->getInletHeight(whichInlet); }

int HPWH::setMaxTempDepression(double maxDepression, UNITS units /*=UNITS_C*/)
{
    if (units == UNITS_C)
    {
        this->maxDepression_C = maxDepression;
    }
    else if (units == UNITS_F)
    {
        this->maxDepression_C = F_TO_C(maxDepression);
    }
    else
    {
        send_warning("Incorrect unit specification for max Temp Depression.");
        return HPWH_ABORT;
    }
    return 0;
}

bool HPWH::hasEnteringWaterHighTempShutOff(int heatSourceIndex)
{
    bool retVal = false;
    if (heatSourceIndex >= getNumHeatSources() || heatSourceIndex < 0)
    {
        return retVal;
    }
    if (heatSources[heatSourceIndex].shutOffLogicSet.size() == 0)
    {
        return retVal;
    }

    for (std::shared_ptr<HeatingLogic> shutOffLogic : heatSources[heatSourceIndex].shutOffLogicSet)
    {
        if (shutOffLogic->getIsEnteringWaterHighTempShutoff())
        {
            retVal = true;
            break;
        }
    }
    return retVal;
}

int HPWH::setEnteringWaterHighTempShutOff(double highTemp,
                                          bool tempIsAbsolute,
                                          int heatSourceIndex,
                                          UNITS unit /*=UNITS_C*/)
{
    if (!hasEnteringWaterHighTempShutOff(heatSourceIndex))
    {
        send_warning("You have attempted to acess a heating logic that does not exist.");
        return HPWH_ABORT;
    }

    double highTemp_C;
    if (unit == UNITS_C)
    {
        highTemp_C = highTemp;
    }
    else if (unit == UNITS_F)
    {
        highTemp_C = F_TO_C(highTemp);
    }
    else
    {
        send_warning("Incorrect unit specification for set Entering Water High Temp Shut Off.");
        return HPWH_ABORT;
    }

    bool highTempIsNotValid = false;
    if (tempIsAbsolute)
    {
        // check differnce with setpoint
        if (setpoint_C - highTemp_C < MINSINGLEPASSLIFT)
        {
            highTempIsNotValid = true;
        }
    }
    else
    {
        if (highTemp_C < MINSINGLEPASSLIFT)
        {
            highTempIsNotValid = true;
        }
    }
    if (highTempIsNotValid)
    {
        send_warning(fmt::format("High temperature shut off is too close to the setpoint, expected "
                                 "a minimum difference of {}",
                                 MINSINGLEPASSLIFT));
        return HPWH_ABORT;
    }

    for (std::shared_ptr<HeatingLogic> shutOffLogic : heatSources[heatSourceIndex].shutOffLogicSet)
    {
        if (shutOffLogic->getIsEnteringWaterHighTempShutoff())
        {
            std::dynamic_pointer_cast<TempBasedHeatingLogic>(shutOffLogic)
                ->setDecisionPoint(highTemp_C, tempIsAbsolute);
            break;
        }
    }
    return 0;
}

int HPWH::setTargetSoCFraction(double target)
{
    if (!isSoCControlled())
    {
        send_warning("Can not set target state of charge if HPWH is not using state of charge "
                     "controls.");
        return HPWH_ABORT;
    }
    if (target < 0)
    {
        send_warning("Can not set a negative target state of charge.");
        return HPWH_ABORT;
    }

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i].shutOffLogicSet)
        {
            if (!logic->getIsEnteringWaterHighTempShutoff())
            {
                logic->setDecisionPoint(target);
            }
        }
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i].turnOnLogicSet)
        {
            logic->setDecisionPoint(target);
        }
    }
    return 0;
}

bool HPWH::isSoCControlled() const { return usesSoCLogic; }

bool HPWH::canUseSoCControls()
{
    bool retVal = true;
    if (getCompressorCoilConfig() != HPWH::HeatSource::CONFIG_EXTERNAL)
    {
        retVal = false;
    }
    return retVal;
}

int HPWH::switchToSoCControls(double targetSoC,
                              double hysteresisFraction /*= 0.05*/,
                              double tempMinUseful /*= 43.333*/,
                              bool constantMainsT /*= false*/,
                              double mainsT /*= 18.333*/,
                              UNITS tempUnit /*= UNITS_C*/)
{
    if (!canUseSoCControls())
    {
        send_warning("Cannot set up state of charge controls for integrated or wrapped HPWHs.");
        return HPWH_ABORT;
    }

    double tempMinUseful_C, mainsT_C;
    if (tempUnit == UNITS_C)
    {
        tempMinUseful_C = tempMinUseful;
        mainsT_C = mainsT;
    }
    else if (tempUnit == UNITS_F)
    {
        tempMinUseful_C = F_TO_C(tempMinUseful);
        mainsT_C = F_TO_C(mainsT);
    }
    else
    {
        send_warning("Incorrect unit specification for set Entering Water High Temp Shut Off.");
        return HPWH_ABORT;
    }

    if (mainsT_C >= tempMinUseful_C)
    {
        send_warning("The mains temperature can't be equal to or greater than the minimum useful "
                     "temperature.");
        return HPWH_ABORT;
    }

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].clearAllTurnOnLogic();

        heatSources[i].shutOffLogicSet.erase(
            std::remove_if(heatSources[i].shutOffLogicSet.begin(),
                           heatSources[i].shutOffLogicSet.end(),
                           [&](const auto logic) -> bool
                           { return !logic->getIsEnteringWaterHighTempShutoff(); }),
            heatSources[i].shutOffLogicSet.end());

        heatSources[i].shutOffLogicSet.push_back(shutOffSoC("SoC Shut Off",
                                                            targetSoC,
                                                            hysteresisFraction,
                                                            tempMinUseful_C,
                                                            constantMainsT,
                                                            mainsT_C));
        heatSources[i].turnOnLogicSet.push_back(turnOnSoC("SoC Turn On",
                                                          targetSoC,
                                                          hysteresisFraction,
                                                          tempMinUseful_C,
                                                          constantMainsT,
                                                          mainsT_C));
    }

    usesSoCLogic = true;

    return 0;
}

std::shared_ptr<HPWH::SoCBasedHeatingLogic> HPWH::turnOnSoC(string desc,
                                                            double targetSoC,
                                                            double hystFract,
                                                            double tempMinUseful_C,
                                                            bool constantMainsT,
                                                            double mainsT_C)
{
    return std::make_shared<SoCBasedHeatingLogic>(
        desc, targetSoC, this, -hystFract, tempMinUseful_C, constantMainsT, mainsT_C);
}

std::shared_ptr<HPWH::SoCBasedHeatingLogic> HPWH::shutOffSoC(string desc,
                                                             double targetSoC,
                                                             double hystFract,
                                                             double tempMinUseful_C,
                                                             bool constantMainsT,
                                                             double mainsT_C)
{
    return std::make_shared<SoCBasedHeatingLogic>(desc,
                                                  targetSoC,
                                                  this,
                                                  hystFract,
                                                  tempMinUseful_C,
                                                  constantMainsT,
                                                  mainsT_C,
                                                  std::greater<double>());
}

//-----------------------------------------------------------------------------
///	@brief	Builds a vector of logic node weights referred to a fixed number of
/// nodes given by LOGIC_SIZE.
/// @param[in]	bottomFraction	Lower bounding fraction (0 to 1)
///	@param[in]	topFraction		Upper bounding fraction (0 to 1)
/// @return	vector of node weights
//-----------------------------------------------------------------------------
std::vector<HPWH::NodeWeight> HPWH::getNodeWeightRange(double bottomFraction, double topFraction)
{
    std::vector<NodeWeight> nodeWeights;
    if (topFraction < bottomFraction)
        std::swap(bottomFraction, topFraction);
    auto bottomIndex = static_cast<std::size_t>(bottomFraction * LOGIC_SIZE);
    auto topIndex = static_cast<std::size_t>(topFraction * LOGIC_SIZE);
    for (auto index = bottomIndex; index < topIndex; ++index)
    {
        nodeWeights.emplace_back(static_cast<int>(index) + 1);
    }
    return nodeWeights;
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::wholeTank(double decisionPoint,
                                                             const UNITS units /* = UNITS_C */,
                                                             const bool absolute /* = false */)
{
    auto nodeWeights = getNodeWeightRange(0., 1.);
    double decisionPoint_C = convertTempToC(decisionPoint, units, absolute);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "whole tank", nodeWeights, decisionPoint_C, this, absolute);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThird(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top third", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThird_absolute(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top third absolute", nodeWeights, decisionPoint, this, true);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondThird(double decisionPoint,
                                                               const UNITS units /* = UNITS_C */,
                                                               const bool absolute /* = false */)
{
    auto nodeWeights = getNodeWeightRange(1. / 3., 2. / 3.);
    double decisionPoint_C = convertTempToC(decisionPoint, units, absolute);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second third", nodeWeights, decisionPoint_C, this, absolute);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomThird(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 3.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom third", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixth_absolute(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom sixth absolute", nodeWeights, decisionPoint, this, true);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondSixth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(1. / 6., 2. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::thirdSixth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(2. / 6., 3. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "third sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fourthSixth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(3. / 6., 4. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "fourth sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fifthSixth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(4. / 6., 5. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "fifth sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topSixth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(5. / 6., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomHalf(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 2.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom half", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomTwelfth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 12.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom twelfth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::standby(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights;
    nodeWeights.emplace_back(LOGIC_SIZE + 1); // uses very top computation node
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "standby", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topNodeMaxTemp(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights;
    nodeWeights.emplace_back(LOGIC_SIZE + 1); // uses very top computation node
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top node", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic>
HPWH::bottomNodeMaxTemp(double decisionPoint, bool isEnteringWaterHighTempShutoff /*=false*/)
{
    std::vector<NodeWeight> nodeWeights;
    nodeWeights.emplace_back(0); // uses very bottom computation node
    return std::make_shared<HPWH::TempBasedHeatingLogic>("bottom node",
                                                         nodeWeights,
                                                         decisionPoint,
                                                         this,
                                                         true,
                                                         std::greater<double>(),
                                                         isEnteringWaterHighTempShutoff);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomTwelfthMaxTemp(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 12.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom twelfth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThirdMaxTemp(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top third", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixthMaxTemp(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom sixth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondSixthMaxTemp(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(1. / 6., 2. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second sixth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fifthSixthMaxTemp(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(4. / 6., 5. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top sixth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topSixthMaxTemp(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(5. / 6., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top sixth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::largeDraw(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 4.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "large draw", nodeWeights, decisionPoint, this, true);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::largerDraw(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 2.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "larger draw", nodeWeights, decisionPoint, this, true);
}

void HPWH::setNumNodes(const std::size_t num_nodes) { tank->setNumNodes(num_nodes); }

int HPWH::getNumNodes() const { return tank->getNumNodes(); }

int HPWH::getIndexTopNode() const { return tank->getIndexTopNode(); }

double HPWH::getTankNodeTemp(int nodeNum, UNITS units /*=UNITS_C*/) const
{
    double result = tank->getNodeT_C(nodeNum);
    if (units == UNITS_C)
    {
    }
    else if (units == UNITS_F)
    {
        result = C_TO_F(result);
    }
    else
    {
        send_warning("Incorrect unit specification for getTankNodeTemp.");
        return double(HPWH_ABORT);
    }
    return result;
}

double HPWH::getNthSimTcouple(int iTCouple, int nTCouple, UNITS units /*=UNITS_C*/) const
{
    double result = tank->getNthSimTcouple(iTCouple, nTCouple);
    if (units == UNITS_C)
    {
    }
    else if (units == UNITS_F)
    {
        result = C_TO_F(result);
    }
    else
    {
        send_error("Incorrect unit specification for getNthSimTcouple.");
    }
    return result;
}

int HPWH::getNumHeatSources() const { return static_cast<int>(heatSources.size()); }

int HPWH::getCompressorIndex() const { return compressorIndex; }

int HPWH::getNumResistanceElements() const
{
    int count = 0;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        count += heatSources[i].isAResistance() ? 1 : 0;
    }
    return count;
}

double HPWH::getCompressorCapacity(double airTemp /*=19.722*/,
                                   double inletTemp /*=14.444*/,
                                   double outTemp /*=57.222*/,
                                   UNITS pwrUnit /*=UNITS_KW*/,
                                   UNITS tempUnit /*=UNITS_C*/)
{
    // calculate capacity btu/hr, input btu/hr, and cop
    double capTemp_BTUperHr, inputTemp_BTUperHr, copTemp; // temporary variables
    double airTemp_C, inletTemp_C, outTemp_C;

    if (!hasACompressor())
    {
        send_warning("Current model does not have a compressor.");
        return double(HPWH_ABORT);
    }

    if (tempUnit == UNITS_C)
    {
        airTemp_C = airTemp;
        inletTemp_C = inletTemp;
        outTemp_C = outTemp;
    }
    else if (tempUnit == UNITS_F)
    {
        airTemp_C = F_TO_C(airTemp);
        inletTemp_C = F_TO_C(inletTemp);
        outTemp_C = F_TO_C(outTemp);
    }
    else
    {
        send_warning("Incorrect unit specification for temperatures in getCompressorCapacity.");
        return double(HPWH_ABORT);
    }

    if (airTemp_C < heatSources[compressorIndex].minT ||
        airTemp_C > heatSources[compressorIndex].maxT)
    {
        send_warning("The compress does not operate at the specified air temperature.");
        return double(HPWH_ABORT);
    }

    double maxAllowedSetpoint_C =
        heatSources[compressorIndex].maxSetpoint_C -
        heatSources[compressorIndex].secondaryHeatExchanger.hotSideTemperatureOffset_dC;

    if (outTemp_C > maxAllowedSetpoint_C)
    {
        send_warning(
            "Inputted outlet temperature of the compressor is higher than can be produced.");
        return double(HPWH_ABORT);
    }

    if (heatSources[compressorIndex].isExternalMultipass())
    {
        double averageTemp_C = (outTemp_C + inletTemp_C) / 2.;
        heatSources[compressorIndex].getCapacityMP(
            airTemp_C, averageTemp_C, inputTemp_BTUperHr, capTemp_BTUperHr, copTemp);
    }
    else
    {
        heatSources[compressorIndex].getCapacity(
            airTemp_C, inletTemp_C, outTemp_C, inputTemp_BTUperHr, capTemp_BTUperHr, copTemp);
    }

    double outputCapacity = capTemp_BTUperHr;
    if (pwrUnit == UNITS_KW)
    {
        outputCapacity = BTU_TO_KWH(capTemp_BTUperHr);
    }
    else if (pwrUnit != UNITS_BTUperHr)
    {
        send_warning("Incorrect unit specification for capacity in getCompressorCapacity.");
        return double(HPWH_ABORT);
    }

    return outputCapacity;
}

double HPWH::getNthHeatSourceEnergyInput(int N, UNITS units /*=UNITS_KWH*/) const
{
    // energy used by the heat source is positive - this should always be positive
    if (N >= getNumHeatSources() || N < 0)
    {
        send_warning(
            "You have attempted to access the energy input of a heat source that does not exist.");
        return double(HPWH_ABORT);
    }

    if (units == UNITS_KWH)
    {
        return heatSources[N].energyInput_kWh;
    }
    else if (units == UNITS_BTU)
    {
        return KWH_TO_BTU(heatSources[N].energyInput_kWh);
    }
    else if (units == UNITS_KJ)
    {
        return KWH_TO_KJ(heatSources[N].energyInput_kWh);
    }
    else
    {
        send_warning("Incorrect unit specification for getNthHeatSourceEnergyInput.");
        return double(HPWH_ABORT);
    }
}

double HPWH::getNthHeatSourceEnergyOutput(int N, UNITS units /*=UNITS_KWH*/) const
{
    // returns energy from the heat source into the water - this should always be positive
    if (N >= getNumHeatSources() || N < 0)
    {
        send_warning(
            "You have attempted to access the energy output of a heat source that does not exist.");
        return double(HPWH_ABORT);
    }

    if (units == UNITS_KWH)
    {
        return heatSources[N].energyOutput_kWh;
    }
    else if (units == UNITS_BTU)
    {
        return KWH_TO_BTU(heatSources[N].energyOutput_kWh);
    }
    else if (units == UNITS_KJ)
    {
        return KWH_TO_KJ(heatSources[N].energyOutput_kWh);
    }
    else
    {
        send_warning("Incorrect unit specification for getNthHeatSourceEnergyInput.");
        return double(HPWH_ABORT);
    }
}

double HPWH::getNthHeatSourceRunTime(int N) const
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_warning(
            "You have attempted to access the run time of a heat source that does not exist.");
        return double(HPWH_ABORT);
    }
    return heatSources[N].runtime_min;
}

int HPWH::isNthHeatSourceRunning(int N) const
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_warning(
            "You have attempted to access the status of a heat source that does not exist.");
        return HPWH_ABORT;
    }
    if (heatSources[N].isEngaged())
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int HPWH::isCompressorRunning() const { return isNthHeatSourceRunning(getCompressorIndex()); }

HPWH::HEATSOURCE_TYPE HPWH::getNthHeatSourceType(int N) const
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_warning("You have attempted to access the type of a heat source that does not exist.");
        return HEATSOURCE_TYPE(HPWH_ABORT);
    }
    return heatSources[N].typeOfHeatSource;
}

bool HPWH::getNthHeatSource(int N, HPWH::HeatSource*& heatSource)
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_warning("You have attempted to access the type of a heat source that does not exist.");
        return false;
    }
    heatSource = &heatSources[N];
    return true;
}

double HPWH::getTankSize(UNITS units /*=UNITS_L*/) const
{
    double result = tank->getVolume_L();
    if (units == UNITS_L)
    {
    }
    else if (units == UNITS_GAL)
    {
        result = L_TO_GAL(result);
    }
    else
    {
        send_error("Incorrect unit specification for getTankSize.");
    }
    return result;
}

double HPWH::getExternalVolumeHeated(UNITS units /*=UNITS_L*/) const
{
    if (units == UNITS_L)
    {
        return externalVolumeHeated_L;
    }
    else if (units == UNITS_GAL)
    {
        return L_TO_GAL(externalVolumeHeated_L);
    }
    else
    {
        send_warning("Incorrect unit specification for getExternalVolumeHeated.");
        return double(HPWH_ABORT);
    }
}

double HPWH::getEnergyRemovedFromEnvironment(UNITS units /*=UNITS_KWH*/) const
{
    // moving heat from the space to the water is the positive direction
    if (units == UNITS_KWH)
    {
        return energyRemovedFromEnvironment_kWh;
    }
    else if (units == UNITS_BTU)
    {
        return KWH_TO_BTU(energyRemovedFromEnvironment_kWh);
    }
    else if (units == UNITS_KJ)
    {
        return KWH_TO_KJ(energyRemovedFromEnvironment_kWh);
    }
    else
    {
        send_warning("Incorrect unit specification for getEnergyRemovedFromEnvironment.");
        return double(HPWH_ABORT);
    }
}

double HPWH::getStandbyLosses(UNITS units /*=UNITS_KWH*/) const
{
    // moving heat from the water to the space is the positive direction
    if (units == UNITS_KWH)
    {
        return standbyLosses_kWh;
    }
    else if (units == UNITS_BTU)
    {
        return KWH_TO_BTU(standbyLosses_kWh);
    }
    else if (units == UNITS_KJ)
    {
        return KWH_TO_KJ(standbyLosses_kWh);
    }
    else
    {
        send_warning("Incorrect unit specification for getStandbyLosses.");
        return double(HPWH_ABORT);
    }
}

///////////////////////////////////////////////////////////////////////////////////

double HPWH::getOutletTemp(UNITS units /*=UNITS_C*/) const
{
    double result = tank->getOutletT_C();
    if (units == UNITS_C)
    {
    }
    else if (units == UNITS_F)
    {
        result = C_TO_F(result);
    }
    else
    {
        send_error("Incorrect unit specification for getOutletTemp.");
    }
    return result;
}

double HPWH::getCondenserWaterInletTemp(UNITS units /*=UNITS_C*/) const
{
    if (units == UNITS_C)
    {
        return condenserInlet_C;
    }
    else if (units == UNITS_F)
    {
        return C_TO_F(condenserInlet_C);
    }
    else
    {
        send_warning("Incorrect unit specification for getCondenserWaterInletTemp.");
        return double(HPWH_ABORT);
    }
}

double HPWH::getCondenserWaterOutletTemp(UNITS units /*=UNITS_C*/) const
{
    if (units == UNITS_C)
    {
        return condenserOutlet_C;
    }
    else if (units == UNITS_F)
    {
        return C_TO_F(condenserOutlet_C);
    }
    else
    {
        send_warning("Incorrect unit specification for getCondenserWaterInletTemp.");
        return double(HPWH_ABORT);
    }
}

double HPWH::getLocationTemp_C() const { return locationTemperature_C; }

//-----------------------------------------------------------------------------
///	@brief	Evaluates the tank temperature averaged uniformly
/// @param[in]	nodeWeights	Discrete set of weighted nodes
/// @return	Tank temperature (C)
//-----------------------------------------------------------------------------
double HPWH::getAverageTankTemp_C() const { return tank->getAverageNodeT_C(); }

double HPWH::getAverageTankTemp_C(const std::vector<double>& dist) const
{
    return tank->getAverageNodeT_C(dist);
}

double HPWH::getAverageTankTemp_C(const std::vector<HPWH::NodeWeight>& nodeWeights) const
{
    return tank->getAverageNodeT_C(nodeWeights);
}

int HPWH::setTankToTemperature(double temp_C) { return tank->setNodeT_C(temp_C); }

double HPWH::getTankHeatContent_kJ() const { return tank->getHeatContent_kJ(); }

int HPWH::getModel() const { return model; }

int HPWH::getCompressorCoilConfig() const
{
    if (!hasACompressor())
    {
        send_warning("Current model does not have a compressor.");
        return HPWH_ABORT;
    }
    return heatSources[compressorIndex].configuration;
}

int HPWH::isCompressorMultipass() const
{
    if (!hasACompressor())
    {
        send_warning("Current model does not have a compressor.");
        return HPWH_ABORT;
    }
    return static_cast<int>(heatSources[compressorIndex].isMultipass);
}

int HPWH::isCompressorExternalMultipass() const
{
    if (!hasACompressor())
    {
        send_warning("Current model does not have a compressor.");
        return HPWH_ABORT;
    }
    return static_cast<int>(heatSources[compressorIndex].isExternalMultipass());
}

bool HPWH::hasACompressor() const { return compressorIndex >= 0; }

bool HPWH::hasExternalHeatSource() const
{
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].configuration == HeatSource::CONFIG_EXTERNAL)
        {
            return true;
        }
    }
    return false;
}

double HPWH::getExternalMPFlowRate(UNITS units /*=UNITS_GPM*/) const
{
    if (!isCompressorExternalMultipass())
    {
        send_warning("Does not have an external multipass heat source.");
        return HPWH_ABORT;
    }

    if (units == HPWH::UNITS_LPS)
    {
        return heatSources[compressorIndex].mpFlowRate_LPS;
    }
    else if (units == HPWH::UNITS_GPM)
    {
        return LPS_TO_GPM(heatSources[compressorIndex].mpFlowRate_LPS);
    }
    else
    {
        send_warning("Incorrect unit specification for getExternalMPFlowRate.");
        return (double)HPWH_ABORT;
    }
}

double HPWH::getCompressorMinRuntime(UNITS units /*=UNITS_MIN*/) const
{

    if (hasACompressor())
    {
        double min_minutes = 10.;

        if (units == UNITS_MIN)
        {
            return min_minutes;
        }
        else if (units == UNITS_SEC)
        {
            return MIN_TO_SEC(min_minutes);
        }
        else if (units == UNITS_HR)
        {
            return MIN_TO_HR(min_minutes);
        }
        else
        {
            send_warning("Incorrect unit specification for getCompressorMinRunTime.");
            return (double)HPWH_ABORT;
        }
    }
    else
    {
        send_warning("This HPWH has no compressor.");
        return (double)HPWH_ABORT;
    }
}

int HPWH::getSizingFractions(double& aquaFract, double& useableFract) const
{
    double aFract = 1.;
    double useFract = 1.;

    if (!hasACompressor())
    {
        send_warning("Current model does not have a compressor.");
        return HPWH_ABORT;
    }
    else if (usesSoCLogic)
    {
        send_warning("Current model uses SOC control logic and does not have a definition for "
                     "sizing fractions.");
        return HPWH_ABORT;
    }

    // Every compressor must have at least one on logic
    for (std::shared_ptr<HeatingLogic> onLogic : heatSources[compressorIndex].turnOnLogicSet)
    {
        double tempA = onLogic->nodeWeightAvgFract(); // if standby logic will return 1
        aFract = tempA < aFract ? tempA : aFract;
    }
    aquaFract = aFract;

    // Compressors don't need to have an off logic
    if (heatSources[compressorIndex].shutOffLogicSet.size() != 0)
    {
        for (std::shared_ptr<HeatingLogic> offLogic : heatSources[compressorIndex].shutOffLogicSet)
        {
            double tempUse;
            if (offLogic->description == "large draw" || offLogic->description == "larger draw")
            {
                tempUse = 1.; // for checking if there's a big draw to switch to RE
            }
            else
            {
                tempUse = 1. - offLogic->nodeWeightAvgFract();
            }
            useFract =
                tempUse < useFract ? tempUse : useFract; // Will return the smallest case of the
                                                         // useable fraction for multiple off logics
        }
        useableFract = useFract;
    }
    else
    {
        useableFract = 1.;
    }

    // Check if doubles are approximately equal and adjust the relationship so it follows the
    // relationship we expect. The tolerance plays with 0.1 mm in position if the tank is 1m tall...
    double temp = 1. - useableFract;
    if (aboutEqual(aquaFract, temp))
    {
        useableFract = 1. - aquaFract + TOL_MINVALUE;
    }

    return 0;
}

int HPWH::setInletByFraction(double fractionalHeight)
{
    return tank->setInletByFraction(fractionalHeight);
}

bool HPWH::isScalable() const { return !(tank->volumeFixed); }

int HPWH::setScaleCapacityCOP(double scaleCapacity /*=1.0*/, double scaleCOP /*=1.0*/)
{
    if (!isScalable())
    {
        send_warning("Cannot scale the HPWH Capacity or COP.");
        return HPWH_ABORT;
    }
    if (!hasACompressor())
    {
        send_warning("Current model does not have a compressor.");
        return HPWH_ABORT;
    }
    if (scaleCapacity <= 0 || scaleCOP <= 0)
    {
        send_warning("Can not scale the HPWH Capacity or COP to 0 or less than 0.");
        return HPWH_ABORT;
    }

    for (auto& perfP : heatSources[compressorIndex].perfMap)
    {
        scaleVector(perfP.inputPower_coeffs, scaleCapacity);
        scaleVector(perfP.COP_coeffs, scaleCOP);
    }

    return 0;
}

int HPWH::setCompressorOutputCapacity(double newCapacity,
                                      double airTemp /*=19.722*/,
                                      double inletTemp /*=14.444*/,
                                      double outTemp /*=57.222*/,
                                      UNITS pwrUnit /*=UNITS_KW*/,
                                      UNITS tempUnit /*=UNITS_C*/)
{

    double oldCapacity = getCompressorCapacity(airTemp, inletTemp, outTemp, pwrUnit, tempUnit);
    if (oldCapacity == double(HPWH_ABORT))
    {
        return HPWH_ABORT;
    }

    double scale = newCapacity / oldCapacity;
    return setScaleCapacityCOP(scale, 1.); // Scale the compressor capacity
}

int HPWH::setResistanceCapacity(double power, int which /*=-1*/, UNITS pwrUnit /*=UNITS_KW*/)
{

    // Input checks
    if (!isScalable())
    {
        send_warning("Cannot scale the resistance elements.");
        return HPWH_ABORT;
    }
    if (getNumResistanceElements() == 0)
    {
        send_warning("There are no resistance elements.");
        return HPWH_ABORT;
    }
    if (which < -1 || which > getNumResistanceElements() - 1)
    {
        send_warning("Out of bounds value for which in setResistanceCapacity().");
        return HPWH_ABORT;
    }
    if (power < 0)
    {
        send_warning("Can not have a negative input power.");
        return HPWH_ABORT;
    }
    // Unit conversion
    double watts;
    if (pwrUnit == UNITS_KW)
    {
        watts = power * 1000; // kW to W
    }
    else if (pwrUnit == UNITS_BTUperHr)
    {
        watts = BTU_TO_KWH(power) * 1000; // BTU to kW then kW to W
    }
    else
    {
        send_warning("Incorrect unit specification for capacity in setResistanceCapacity.");
        return HPWH_ABORT;
    }

    // Whew so many checks...
    if (which == -1)
    {
        // Just get all the elements
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i].isAResistance())
            {
                heatSources[i].changeResistanceWatts(watts);
            }
        }
    }
    else
    {
        heatSources[resistanceHeightMap[which].index].changeResistanceWatts(watts);

        // Then check for repeats in the position
        int pos = resistanceHeightMap[which].position;
        for (int i = 0; i < getNumResistanceElements(); i++)
        {
            if (which != i && resistanceHeightMap[i].position == pos)
            {
                heatSources[resistanceHeightMap[i].index].changeResistanceWatts(watts);
            }
        }
    }

    return 0;
}

double HPWH::getResistanceCapacity(int which /*=-1*/, UNITS pwrUnit /*=UNITS_KW*/)
{

    // Input checks
    if (getNumResistanceElements() == 0)
    {
        send_warning("There are no resistance elements.");
        return HPWH_ABORT;
    }
    if (which < -1 || which > getNumResistanceElements() - 1)
    {
        send_warning("Out of bounds value for which in getResistanceCapacity().");
        return HPWH_ABORT;
    }

    double returnPower = 0;
    if (which == -1)
    {
        // Just get all the elements
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i].isAResistance())
            {
                returnPower += heatSources[i].perfMap[0].inputPower_coeffs[0];
            }
        }
    }
    else
    {
        // get the power from "which" element by height
        returnPower +=
            heatSources[resistanceHeightMap[which].index].perfMap[0].inputPower_coeffs[0];

        // Then check for repeats in the position
        int pos = resistanceHeightMap[which].position;
        for (int i = 0; i < getNumResistanceElements(); i++)
        {
            if (which != i && resistanceHeightMap[i].position == pos)
            {
                returnPower +=
                    heatSources[resistanceHeightMap[i].index].perfMap[0].inputPower_coeffs[0];
            }
        }
    }

    // Unit conversion
    if (pwrUnit == UNITS_KW)
    {
        returnPower /= 1000.; // W to KW
    }
    else if (pwrUnit == UNITS_BTUperHr)
    {
        returnPower = KWH_TO_BTU(returnPower / 1000.); // W to BTU/hr
    }
    else
    {
        send_warning("Incorrect unit specification for capacity in getResistanceCapacity.");
        return HPWH_ABORT;
    }

    return returnPower;
}

int HPWH::getResistancePosition(int elementIndex) const
{

    if (elementIndex < 0 || elementIndex > getNumHeatSources() - 1)
    {
        send_warning("Out of bounds value for which in getResistancePosition.");
        return HPWH_ABORT;
    }

    if (!heatSources[elementIndex].isAResistance())
    {
        send_warning("This index is not a resistance element.");
        return HPWH_ABORT;
    }

    for (int i = 0; i < heatSources[elementIndex].getCondensitySize(); i++)
    {
        if (heatSources[elementIndex].condensity[i] == 1)
        { // res elements have a condenstiy of 1 at a specific node
            return i;
        }
    }
    return HPWH_ABORT;
}

void HPWH::updateSoCIfNecessary()
{
    if (usesSoCLogic)
    {
        calcAndSetSoCFraction();
    }
}

// Inversion mixing modeled after bigladder EnergyPlus code PK
void HPWH::mixTankInversions() { tank->mixInversions(); }

double HPWH::addHeatAboveNode(double qAdd_kJ, int nodeNum, const double maxT_C)
{

    // Do not exceed maxT_C or setpoint
    double maxHeatToT_C = std::min(maxT_C, setpoint_C);

    return tank->addHeatAboveNode(qAdd_kJ, nodeNum, maxHeatToT_C);
}

void HPWH::addExtraHeatAboveNode(double qAdd_kJ, const int nodeNum)
{
    tank->addExtraHeatAboveNode(qAdd_kJ, nodeNum);
}

void HPWH::modifyHeatDistribution(std::vector<double>& heatDistribution_W)
{
    tank->modifyHeatDistribution(heatDistribution_W, setpoint_C);
}

//-----------------------------------------------------------------------------
///	@brief	Adds extra heat to tank.
/// @param[in]	extraHeatDist_W		A distribution of extra heat to add
//-----------------------------------------------------------------------------
void HPWH::addExtraHeat(std::vector<double>& extraHeatDist_W)
{

    auto modHeatDistribution_W = extraHeatDist_W;
    modifyHeatDistribution(modHeatDistribution_W);

    std::vector<double> heatDistribution_W(getNumNodes());
    resampleExtensive(heatDistribution_W, modHeatDistribution_W);

    // Unnecessary unit conversions used here to match former method
    double tot_qAdded_BTUperHr = 0.;
    for (int i = getNumNodes() - 1; i >= 0; i--)
    {
        if (heatDistribution_W[i] != 0)
        {
            double qAdd_BTUperHr = KWH_TO_BTU(heatDistribution_W[i] / 1000.);
            double qAdd_KJ = BTU_TO_KJ(qAdd_BTUperHr * minutesPerStep / min_per_hr);
            addExtraHeatAboveNode(qAdd_KJ, i);
            tot_qAdded_BTUperHr += qAdd_BTUperHr;
        }
    }
    // Write the input & output energy
    extraEnergyInput_kWh = BTU_TO_KWH(tot_qAdded_BTUperHr * minutesPerStep / min_per_hr);
}

///////////////////////////////////////////////////////////////////////////////////

void HPWH::turnAllHeatSourcesOff()
{
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].disengageHeatSource();
    }
    isHeating = false;
}

bool HPWH::areAllHeatSourcesOff() const
{
    bool allOff = true;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isEngaged() == true)
        {
            allOff = false;
        }
    }
    return allOff;
}

void HPWH::mixTankNodes(int mixBottomNode, int mixBelowNode, double mixFactor)
{
    tank->mixNodes(mixBottomNode, mixBelowNode, mixFactor);
}

void HPWH::calcDerivedValues()
{
    // condentropy/shrinkage and lowestNode are now in calcDerivedHeatingValues()
    calcDerivedHeatingValues();

    tank->calcSizeConstants();

    mapResRelativePosToHeatSources();

    // heat source ability to depress temp
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isACompressor())
        {
            heatSources[i].depressesTemperature = true;
        }
        else if (heatSources[i].isAResistance())
        {
            heatSources[i].depressesTemperature = false;
        }
    }
}

void HPWH::calcDerivedHeatingValues()
{
    // find condentropy/shrinkage
    for (int i = 0; i < getNumHeatSources(); ++i)
    {
        heatSources[i].Tshrinkage_C = findShrinkageT_C(heatSources[i].condensity);
    }

    // find lowest node
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].lowestNode = findLowestNode(heatSources[i].condensity, tank->getNumNodes());
    }

    // define condenser index and lowest resistance element index
    compressorIndex = -1;     // Default = No compressor
    lowestElementIndex = -1;  // Default = No resistance elements
    highestElementIndex = -1; // Default = No resistance elements
    VIPIndex = -1;            // Default = No VIP element
    double lowestPos = 1.;
    double highestPos = 0.; // -1 to make sure a an element on the bottom can still be identified.
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isACompressor())
        {
            compressorIndex = i; // NOTE: Maybe won't work with multiple compressors (last
                                 // compressor will be used)
        }
        else if (heatSources[i].isAResistance())
        {
            // Gets VIP element index
            if (heatSources[i].isVIP)
            {
                if (VIPIndex == -1)
                {
                    VIPIndex = i;
                }
                else
                {
                    send_warning("More than one resistance element is assigned to VIP.");
                }
            }
            int condensitySize = heatSources[i].getCondensitySize();
            for (int j = 0; j < condensitySize; ++j)
            {
                double pos = static_cast<double>(j) / condensitySize;
                if ((heatSources[i].condensity[j] > 0.) && (pos < lowestPos))
                {
                    lowestElementIndex = i;
                    lowestPos = pos;
                }
                if ((heatSources[i].condensity[j] > 0.) && (pos >= highestPos))
                {
                    highestElementIndex = i;
                    highestPos = pos;
                }
            }
        }
    }

    // heat source ability to depress temp
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isACompressor())
        {
            heatSources[i].depressesTemperature = true;
        }
        else if (heatSources[i].isAResistance())
        {
            heatSources[i].depressesTemperature = false;
        }
    }
}

void HPWH::mapResRelativePosToHeatSources()
{
    resistanceHeightMap.clear();
    resistanceHeightMap.reserve(getNumResistanceElements());

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isAResistance())
        {
            resistanceHeightMap.push_back({i, getResistancePosition(i)});
        }
    }

    // Sort by height, low to high
    std::sort(resistanceHeightMap.begin(),
              resistanceHeightMap.end(),
              [](const HPWH::resPoint& a, const HPWH::resPoint& b) -> bool
              {
                  return a.position < b.position; // (5 < 5)      // evaluates to false
              });
}

bool HPWH::shouldDRLockOut(HEATSOURCE_TYPE hs, DRMODES DR_signal) const
{
    if (hs == TYPE_compressor && (DR_signal & DR_LOC) != 0)
    {
        return true;
    }
    else if (hs == TYPE_resistance && (DR_signal & DR_LOR) != 0)
    {
        return true;
    }
    return false;
}

void HPWH::resetTopOffTimer() { timerTOT = 0.; }

//-----------------------------------------------------------------------------
///	@brief	Checks whether energy is balanced during a simulation step.
/// @note	Used in test/main.cc
/// @param[in]	drawVol_L				Water volume drawn during simulation step
///	@param[in]	prevHeatContent_kJ		Heat content of tank prior to simulation step
///	@param[in]	fracEnergyTolerance		Fractional tolerance for energy imbalance
/// @return	true if balanced; false otherwise.
//-----------------------------------------------------------------------------
bool HPWH::isEnergyBalanced(const double drawVol_L,
                            const double prevHeatContent_kJ,
                            const double fracEnergyTolerance /* = 0.001 */)
{
    double drawCp_kJperC =
        CPWATER_kJperkgC * DENSITYWATER_kgperL * drawVol_L; // heat capacity of draw

    // Check energy balancing.
    double qInElectrical_kJ = 0.;
    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        qInElectrical_kJ += getNthHeatSourceEnergyInput(iHS, UNITS_KJ);
    }

    double qInExtra_kJ = KWH_TO_KJ(extraEnergyInput_kWh);
    double qInHeatSourceEnviron_kJ = getEnergyRemovedFromEnvironment(UNITS_KJ);
    double qOutTankEnviron_kJ = KWH_TO_KJ(standbyLosses_kWh);
    double qOutWater_kJ =
        drawCp_kJperC * (tank->getOutletT_C() - member_inletT_C); // assumes only one inlet
    double expectedTankHeatContent_kJ =
        prevHeatContent_kJ        // previous heat content
        + qInElectrical_kJ        // electrical energy delivered to heat sources
        + qInExtra_kJ             // extra energy delivered to heat sources
        + qInHeatSourceEnviron_kJ // heat extracted from environment by condenser
        - qOutTankEnviron_kJ      // heat released from tank to environment
        - qOutWater_kJ;           // heat expelled to outlet by water flow

    double qBal_kJ = getTankHeatContent_kJ() - expectedTankHeatContent_kJ;
    double fracEnergyDiff = fabs(qBal_kJ) / std::max(prevHeatContent_kJ, 1.);
    if (fracEnergyDiff > fracEnergyTolerance)
    {
        send_warning(
            fmt::format("Energy-balance error: {} kJ, {} %%", qBal_kJ, 100. * fracEnergyDiff));
        return false;
    }
    return true;
}

bool compressorIsRunning(HPWH& hpwh)
{
    return (bool)hpwh.isNthHeatSourceRunning(hpwh.getCompressorIndex());
}

/* static */ bool HPWH::mapNameToPreset(const std::string& modelName, HPWH::MODELS& model)
{
    if (modelName == "Voltex60" || modelName == "AOSmithPHPT60")
    {
        model = HPWH::MODELS_AOSmithPHPT60;
    }
    else if (modelName == "Voltex80" || modelName == "AOSmith80")
    {
        model = HPWH::MODELS_AOSmithPHPT80;
    }
    else if (modelName == "GEred" || modelName == "GE")
    {
        model = HPWH::MODELS_GE2012;
    }
    else if (modelName == "SandenGAU" || modelName == "Sanden80" || modelName == "SandenGen3")
    {
        model = HPWH::MODELS_Sanden80;
    }
    else if (modelName == "Sanden120")
    {
        model = HPWH::MODELS_Sanden120;
    }
    else if (modelName == "SandenGES" || modelName == "Sanden40")
    {
        model = HPWH::MODELS_Sanden40;
    }
    else if (modelName == "AOSmithHPTU50")
    {
        model = HPWH::MODELS_AOSmithHPTU50;
    }
    else if (modelName == "AOSmithHPTU66")
    {
        model = HPWH::MODELS_AOSmithHPTU66;
    }
    else if (modelName == "AOSmithHPTU80")
    {
        model = HPWH::MODELS_AOSmithHPTU80;
    }
    else if (modelName == "AOSmithHPTS50")
    {
        model = HPWH::MODELS_AOSmithHPTS50;
    }
    else if (modelName == "AOSmithHPTS66")
    {
        model = HPWH::MODELS_AOSmithHPTS66;
    }
    else if (modelName == "AOSmithHPTS80")
    {
        model = HPWH::MODELS_AOSmithHPTS80;
    }
    else if (modelName == "AOSmithHPTU80DR")
    {
        model = HPWH::MODELS_AOSmithHPTU80_DR;
    }
    else if (modelName == "GE502014STDMode" || modelName == "GE2014STDMode")
    {
        model = HPWH::MODELS_GE2014STDMode;
    }
    else if (modelName == "GE502014" || modelName == "GE2014")
    {
        model = HPWH::MODELS_GE2014;
    }
    else if (modelName == "GE802014")
    {
        model = HPWH::MODELS_GE2014_80DR;
    }
    else if (modelName == "RheemHB50")
    {
        model = HPWH::MODELS_RheemHB50;
    }
    else if (modelName == "Stiebel220e" || modelName == "Stiebel220E")
    {
        model = HPWH::MODELS_Stiebel220E;
    }
    else if (modelName == "Generic1")
    {
        model = HPWH::MODELS_Generic1;
    }
    else if (modelName == "Generic2")
    {
        model = HPWH::MODELS_Generic2;
    }
    else if (modelName == "Generic3")
    {
        model = HPWH::MODELS_Generic3;
    }
    else if (modelName == "custom")
    {
        model = HPWH::MODELS_CustomFile;
    }
    else if (modelName == "restankRealistic")
    {
        model = HPWH::MODELS_restankRealistic;
    }
    else if (modelName == "StorageTank")
    {
        model = HPWH::MODELS_StorageTank;
    }
    else if (modelName == "BWC2020_65")
    {
        model = HPWH::MODELS_BWC2020_65;
    }
    // New Rheems
    else if (modelName == "Rheem2020Prem40")
    {
        model = HPWH::MODELS_Rheem2020Prem40;
    }
    else if (modelName == "Rheem2020Prem50")
    {
        model = HPWH::MODELS_Rheem2020Prem50;
    }
    else if (modelName == "Rheem2020Prem65")
    {
        model = HPWH::MODELS_Rheem2020Prem65;
    }
    else if (modelName == "Rheem2020Prem80")
    {
        model = HPWH::MODELS_Rheem2020Prem80;
    }
    else if (modelName == "Rheem2020Build40")
    {
        model = HPWH::MODELS_Rheem2020Build40;
    }
    else if (modelName == "Rheem2020Build50")
    {
        model = HPWH::MODELS_Rheem2020Build50;
    }
    else if (modelName == "Rheem2020Build65")
    {
        model = HPWH::MODELS_Rheem2020Build65;
    }
    else if (modelName == "Rheem2020Build80")
    {
        model = HPWH::MODELS_Rheem2020Build80;
    }
    else if (modelName == "RheemPlugInDedicated40")
    {
        model = HPWH::MODELS_RheemPlugInDedicated40;
    }
    else if (modelName == "RheemPlugInDedicated50")
    {
        model = HPWH::MODELS_RheemPlugInDedicated50;
    }
    else if (modelName == "RheemPlugInShared40")
    {
        model = HPWH::MODELS_RheemPlugInShared40;
    }
    else if (modelName == "RheemPlugInShared50")
    {
        model = HPWH::MODELS_RheemPlugInShared50;
    }
    else if (modelName == "RheemPlugInShared65")
    {
        model = HPWH::MODELS_RheemPlugInShared65;
    }
    else if (modelName == "RheemPlugInShared80")
    {
        model = HPWH::MODELS_RheemPlugInShared80;
    }
    // Large HPWH's
    else if (modelName == "AOSmithCAHP120")
    {
        model = HPWH::MODELS_AOSmithCAHP120;
    }
    else if (modelName == "ColmacCxV_5_SP")
    {
        model = HPWH::MODELS_ColmacCxV_5_SP;
    }
    else if (modelName == "ColmacCxA_10_SP")
    {
        model = HPWH::MODELS_ColmacCxA_10_SP;
    }
    else if (modelName == "ColmacCxA_15_SP")
    {
        model = HPWH::MODELS_ColmacCxA_15_SP;
    }
    else if (modelName == "ColmacCxA_20_SP")
    {
        model = HPWH::MODELS_ColmacCxA_20_SP;
    }
    else if (modelName == "ColmacCxA_25_SP")
    {
        model = HPWH::MODELS_ColmacCxA_25_SP;
    }
    else if (modelName == "ColmacCxA_30_SP")
    {
        model = HPWH::MODELS_ColmacCxA_30_SP;
    }

    else if (modelName == "ColmacCxV_5_MP")
    {
        model = HPWH::MODELS_ColmacCxV_5_MP;
    }
    else if (modelName == "ColmacCxA_10_MP")
    {
        model = HPWH::MODELS_ColmacCxA_10_MP;
    }
    else if (modelName == "ColmacCxA_15_MP")
    {
        model = HPWH::MODELS_ColmacCxA_15_MP;
    }
    else if (modelName == "ColmacCxA_20_MP")
    {
        model = HPWH::MODELS_ColmacCxA_20_MP;
    }
    else if (modelName == "ColmacCxA_25_MP")
    {
        model = HPWH::MODELS_ColmacCxA_25_MP;
    }
    else if (modelName == "ColmacCxA_30_MP")
    {
        model = HPWH::MODELS_ColmacCxA_30_MP;
    }

    else if (modelName == "RheemHPHD60")
    {
        model = HPWH::MODELS_RHEEM_HPHD60VNU_201_MP;
    }
    else if (modelName == "RheemHPHD135")
    {
        model = HPWH::MODELS_RHEEM_HPHD135VNU_483_MP;
    }
    // Nyle Single pass models
    else if (modelName == "NyleC25A_SP")
    {
        model = HPWH::MODELS_NyleC25A_SP;
    }
    else if (modelName == "NyleC60A_SP")
    {
        model = HPWH::MODELS_NyleC60A_SP;
    }
    else if (modelName == "NyleC90A_SP")
    {
        model = HPWH::MODELS_NyleC90A_SP;
    }
    else if (modelName == "NyleC185A_SP")
    {
        model = HPWH::MODELS_NyleC185A_SP;
    }
    else if (modelName == "NyleC250A_SP")
    {
        model = HPWH::MODELS_NyleC250A_SP;
    }
    else if (modelName == "NyleC60A_C_SP")
    {
        model = HPWH::MODELS_NyleC60A_C_SP;
    }
    else if (modelName == "NyleC90A_C_SP")
    {
        model = HPWH::MODELS_NyleC90A_C_SP;
    }
    else if (modelName == "NyleC185A_C_SP")
    {
        model = HPWH::MODELS_NyleC185A_C_SP;
    }
    else if (modelName == "NyleC250A_C_SP")
    {
        model = HPWH::MODELS_NyleC250A_C_SP;
    }
    // Nyle MP models
    else if (modelName == "NyleC60A_MP")
    {
        model = HPWH::MODELS_NyleC60A_MP;
    }
    else if (modelName == "NyleC90A_MP")
    {
        model = HPWH::MODELS_NyleC90A_MP;
    }
    else if (modelName == "NyleC125A_MP")
    {
        model = HPWH::MODELS_NyleC125A_MP;
    }
    else if (modelName == "NyleC185A_MP")
    {
        model = HPWH::MODELS_NyleC185A_MP;
    }
    else if (modelName == "NyleC250A_MP")
    {
        model = HPWH::MODELS_NyleC250A_MP;
    }
    else if (modelName == "NyleC60A_C_MP")
    {
        model = HPWH::MODELS_NyleC60A_C_MP;
    }
    else if (modelName == "NyleC90A_C_MP")
    {
        model = HPWH::MODELS_NyleC90A_C_MP;
    }
    else if (modelName == "NyleC125A_C_MP")
    {
        model = HPWH::MODELS_NyleC125A_C_MP;
    }
    else if (modelName == "NyleC185A_C_MP")
    {
        model = HPWH::MODELS_NyleC185A_C_MP;
    }
    else if (modelName == "NyleC250A_C_MP")
    {
        model = HPWH::MODELS_NyleC250A_C_MP;
    }
    else if (modelName == "QAHV_N136TAU_HPB_SP")
    {
        model = HPWH::MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP;
    }
    // Stack in a couple scalable models
    else if (modelName == "TamScalable_SP")
    {
        model = HPWH::MODELS_TamScalable_SP;
    }
    else if (modelName == "TamScalable_SP_2X")
    {
        model = HPWH::MODELS_TamScalable_SP_2X;
    }
    else if (modelName == "TamScalable_SP_Half")
    {
        model = HPWH::MODELS_TamScalable_SP_Half;
    }
    else if (modelName == "Scalable_MP")
    {
        model = HPWH::MODELS_Scalable_MP;
    }
    else if (modelName == "AWHSTier3Generic40")
    {
        model = HPWH::MODELS_AWHSTier3Generic40;
    }
    else if (modelName == "AWHSTier3Generic50")
    {
        model = HPWH::MODELS_AWHSTier3Generic50;
    }
    else if (modelName == "AWHSTier3Generic65")
    {
        model = HPWH::MODELS_AWHSTier3Generic65;
    }
    else if (modelName == "AWHSTier3Generic80")
    {
        model = HPWH::MODELS_AWHSTier3Generic80;
    }
    else if (modelName == "AWHSTier4Generic40") // Tier-4 Generic 40 gal
    {
        model = HPWH::MODELS_AWHSTier4Generic40;
    }
    else if (modelName == "AWHSTier4Generic50") // Tier-4 Generic 50 gal
    {
        model = HPWH::MODELS_AWHSTier4Generic50;
    }
    else if (modelName == "AWHSTier4Generic65") // Tier-4 Generic 65 gal
    {
        model = HPWH::MODELS_AWHSTier4Generic65;
    }
    else if (modelName == "AWHSTier4Generic80") // Tier-4 Generic 80 gal
    {
        model = HPWH::MODELS_AWHSTier4Generic80;
    }
    else if (modelName == "AquaThermAire")
    {
        model = HPWH::MODELS_AquaThermAire;
    }
    else if (modelName == "GenericUEF217")
    {
        model = HPWH::MODELS_GenericUEF217;
    }
    else
    {
        model = HPWH::MODELS_basicIntegrated;
        cout << "Couldn't find model " << modelName << ".  Exiting...\n";
        return false;
    }
    return true;
}

/// Initializes a preset from the modelName
int HPWH::initPreset(const std::string& modelName)
{
    HPWH::MODELS targetModel;
    if (mapNameToPreset(modelName, targetModel))
    {
        return initPreset(targetModel);
    }
    return HPWH_ABORT;
}

// Used to check a few inputs after the initialization of a tank model from a preset or a file.
int HPWH::checkInputs()
{
    int returnVal = 0;
    // use a returnVal so that all checks are processed and error messages written

    if (getNumHeatSources() <= 0 && (model != MODELS_StorageTank))
    {
        send_warning("You must have at least one HeatSource.");
        returnVal = HPWH_ABORT;
    }

    double condensitySum;
    // loop through all heat sources to check each for malconfigurations
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        // check the heat source type to make sure it has been set
        if (heatSources[i].typeOfHeatSource == TYPE_none)
        {
            send_warning(fmt::format(
                "Heat source {} does not have a specified type.  Initialization failed.", i));
            returnVal = HPWH_ABORT;
        }
        // check to make sure there is at least one onlogic or parent with onlogic
        int parent = heatSources[i].findParent();
        if (heatSources[i].turnOnLogicSet.size() == 0 &&
            (parent == -1 || heatSources[parent].turnOnLogicSet.size() == 0))
        {
            send_warning(
                "You must specify at least one logic to turn on the element or the element "
                "must be set as a backup for another heat source with at least one logic.");
            returnVal = HPWH_ABORT;
        }

        // Validate on logics
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i].turnOnLogicSet)
        {
            if (!logic->isValid())
            {
                send_warning(fmt::format("On logic at index %i is invalid", i));
                returnVal = HPWH_ABORT;
            }
        }
        // Validate off logics
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i].shutOffLogicSet)
        {
            if (!logic->isValid())
            {
                send_warning(fmt::format("Off logic at index %i is invalid", i));
                returnVal = HPWH_ABORT;
            }
        }

        // check is condensity sums to 1
        condensitySum = 0;

        for (int j = 0; j < heatSources[i].getCondensitySize(); j++)
            condensitySum += heatSources[i].condensity[j];
        if (fabs(condensitySum - 1.0) > 1e-6)
        {
            send_warning(fmt::format("The condensity for heatsource {} does not sum to 1. "
                                     "It sums to {}.",
                                     i,
                                     condensitySum));
            returnVal = HPWH_ABORT;
        }
        // check that air flows are all set properly
        if (heatSources[i].airflowFreedom > 1.0 || heatSources[i].airflowFreedom <= 0.0)
        {
            send_warning(
                fmt::format("The airflowFreedom must be between 0 and 1 for heatsource {}.", i));
            returnVal = HPWH_ABORT;
        }

        if (heatSources[i].isACompressor())
        {
            if (heatSources[i].doDefrost)
            {
                if (heatSources[i].defrostMap.size() < 3)
                {
                    send_warning(
                        "Defrost logic set to true but no valid defrost map of length 3 or "
                        "greater set.");
                    returnVal = HPWH_ABORT;
                }
                if (heatSources[i].configuration != HeatSource::CONFIG_EXTERNAL)
                {
                    send_warning("Defrost is only simulated for external compressors.");
                    returnVal = HPWH_ABORT;
                }
            }
        }
        if (heatSources[i].configuration == HeatSource::CONFIG_EXTERNAL)
        {

            if (heatSources[i].shutOffLogicSet.size() != 1)
            {
                send_warning("External heat sources can only have one shut off logic.");
                returnVal = HPWH_ABORT;
            }
            if (0 > heatSources[i].externalOutletHeight ||
                heatSources[i].externalOutletHeight > getNumNodes() - 1)
            {
                send_warning(
                    "External heat sources need an external outlet height within the bounds"
                    "from from 0 to numNodes-1.");
                returnVal = HPWH_ABORT;
            }
            if (0 > heatSources[i].externalInletHeight ||
                heatSources[i].externalInletHeight > getNumNodes() - 1)
            {
                send_warning(
                    "External heat sources need an external inlet height within the bounds "
                    "from from 0 to numNodes-1.");
                returnVal = HPWH_ABORT;
            }
        }
        else
        {
            if (heatSources[i].secondaryHeatExchanger.extraPumpPower_W != 0 ||
                heatSources[i].secondaryHeatExchanger.extraPumpPower_W)
            {
                send_warning(
                    fmt::format("Heatsource %d is not an external heat source but has an external "
                                "secondary heat exchanger.",
                                i));
                returnVal = HPWH_ABORT;
            }
        }

        // Check performance map
        // perfGrid and perfGridValues, and the length of vectors in perfGridValues are equal and
        // that ;
        if (heatSources[i].useBtwxtGrid)
        {
            // If useBtwxtGrid is true that the perfMap is empty
            if (heatSources[i].perfMap.size() != 0)
            {
                send_warning(
                    "Using the grid lookups but a regression based performance map is given.");
                returnVal = HPWH_ABORT;
            }

            // Check length of vectors in perfGridValue are equal
            if (heatSources[i].perfGridValues[0].size() !=
                    heatSources[i].perfGridValues[1].size() &&
                heatSources[i].perfGridValues[0].size() != 0)
            {
                send_warning(
                    "When using grid lookups for performance the vectors in perfGridValues must "
                    "be the same length.");
                returnVal = HPWH_ABORT;
            }

            // Check perfGrid's vectors lengths multiplied together == the perfGridValues vector
            // lengths
            size_t expLength = 1;
            for (const auto& v : heatSources[i].perfGrid)
            {
                expLength *= v.size();
            }
            if (expLength != heatSources[i].perfGridValues[0].size())
            {
                send_warning(
                    "When using grid lookups for perfmance the vectors in perfGridValues must "
                    "be the same length.");
                returnVal = HPWH_ABORT;
            }
        }
        else
        {
            // Check that perfmap only has 1 point if config_external and multipass
            if (heatSources[i].isExternalMultipass() && heatSources[i].perfMap.size() != 1)
            {
                send_warning(
                    "External multipass heat sources must have a perfMap of only one point "
                    "with regression equations.");
                returnVal = HPWH_ABORT;
            }
        }
    }

    // Check that the on logic and off logics are ordered properly
    if (hasACompressor())
    {
        double aquaF = 0., useF = 1.;
        getSizingFractions(aquaF, useF);
        if (aquaF < (1. - useF))
        {
            send_warning(
                "The relationship between the on logic and off logic is not supported. The off "
                "logic is beneath the on logic.");
            returnVal = HPWH_ABORT;
        }
    }

    double maxTemp;
    string why;
    double tempSetpoint = setpoint_C;
    if (!isNewSetpointPossible(tempSetpoint, maxTemp, why))
    {
        send_warning(fmt::format("Cannot set new setpoint. {}", why.c_str()));
        returnVal = HPWH_ABORT;
    }

    // Check if the UA is out of bounds
    if (tank->getUA_kJperHrC() < 0.0)
    {
        send_warning(
            fmt::format("The tankUA_kJperHrC is less than 0 for a HPWH, it must be greater than 0, "
                        "tankUA_kJperHrC is: {}",
                        tank->getUA_kJperHrC()));
        returnVal = HPWH_ABORT;
    }

    // Check single-node heat-exchange effectiveness validity
    if (tank->heatExchangerEffectiveness > 1.)
    {
        send_warning("Heat-exchanger effectiveness cannot exceed 1.");
        returnVal = HPWH_ABORT;
    }

    // if there's no failures, return 0
    return returnVal;
}

#ifndef HPWH_ABRIDGED
int HPWH::initFromFile(string configFile)
{
    setAllDefaults(); // reset all defaults if you're re-initilizing
    // return 0 on success, HPWH_ABORT for failure

    // open file, check and report errors
    std::ifstream inputFILE;
    inputFILE.open(configFile.c_str());
    if (!inputFILE.is_open())
    {
        send_error("Input file failed to open.");
        return HPWH_ABORT;
    }

    // some variables that will be handy
    std::size_t heatsource, sourceNum, nTemps, tempInt;
    std::size_t num_nodes = 0, numHeatSources = 0;
    bool hasInitialTankTemp = false;
    double initalTankT_C = F_TO_C(120.);

    string tempString, units;
    double tempDouble;

    // being file processing, line by line
    string line_s;
    std::stringstream line_ss;
    string token;
    while (std::getline(inputFILE, line_s))
    {
        line_ss.clear();
        line_ss.str(line_s);

        // grab the first word, and start comparing
        token = "";
        line_ss >> token;
        if (line_s.empty() || (token.length() == 0))
        {
            continue;
        }
        if (token.at(0) == '#')
        {
            // if you hit a comment, skip to next line
            continue;
        }
        else if (token == "numNodes")
        {
            line_ss >> num_nodes;
        }
        else if (token == "volume")
        {
            line_ss >> tempDouble >> units;
            if (units == "gal")
                tempDouble = GAL_TO_L(tempDouble);
            else if (units == "L")
                ; // do nothing, lol
            else
            {
                send_error(fmt::format("Incorrect units specification for {}.", token.c_str()));
                return HPWH_ABORT;
            }
            tank->volume_L = tempDouble;
        }
        else if (token == "UA")
        {
            line_ss >> tempDouble >> units;
            if (units != "kJperHrC")
            {
                send_error(fmt::format("Incorrect units specification for {}.", token.c_str()));
                return HPWH_ABORT;
            }
            tank->setUA_kJperHrC(tempDouble);
        }
        else if (token == "depressTemp")
        {
            line_ss >> tempString;
            if (tempString == "true")
            {
                doTempDepression = true;
            }
            else if (tempString == "false")
            {
                doTempDepression = false;
            }
            else
            {
                send_error(fmt::format("Improper value for {}", token.c_str()));
                return HPWH_ABORT;
            }
        }
        else if (token == "mixOnDraw")
        {
            line_ss >> tempString;
            if (tempString == "true")
            {
                tank->mixesOnDraw = true;
            }
            else if (tempString == "false")
            {
                tank->mixesOnDraw = false;
            }
            else
            {
                send_error(fmt::format("Improper value for {}.", token.c_str()));
                return HPWH_ABORT;
            }
        }
        else if (token == "mixBelowFractionOnDraw")
        {
            line_ss >> tempDouble;
            if (tempDouble < 0 || tempDouble > 1)
            {
                send_error(fmt::format("Out of bounds value for {}. Should be between 0 and 1.",
                                       token.c_str()));
                return HPWH_ABORT;
            }
            tank->mixBelowFractionOnDraw = tempDouble;
        }
        else if (token == "setpoint")
        {
            line_ss >> tempDouble >> units;
            if (units == "F")
                tempDouble = F_TO_C(tempDouble);
            else if (units == "C")
                ; // do nothing, lol
            else
            {
                send_error(fmt::format("Incorrect units specification for {}.", token.c_str()));
                return HPWH_ABORT;
            }
            setpoint_C = tempDouble;
            // tank will be set to setpoint at end of function
        }
        else if (token == "setpointFixed")
        {
            line_ss >> tempString;
            if (tempString == "true")
                setpointFixed = true;
            else if (tempString == "false")
                setpointFixed = false;
            else
            {
                send_error(fmt::format("Improper value for {}", token.c_str()));
                return HPWH_ABORT;
            }
        }
        else if (token == "initialTankTemp")
        {
            line_ss >> tempDouble >> units;
            if (units == "F")
                tempDouble = F_TO_C(tempDouble);
            else if (units == "C")
                ;
            else
            {
                send_error(fmt::format("Incorrect units specification for {}.", token.c_str()));
                return HPWH_ABORT;
            }
            initalTankT_C = tempDouble;
            hasInitialTankTemp = true;
        }
        else if (token == "hasHeatExchanger")
        {
            // false of this model uses heat exchange
            line_ss >> tempString;
            if (tempString == "true")
                tank->hasHeatExchanger = true;
            else if (tempString == "false")
                tank->hasHeatExchanger = false;
            else
            {
                send_error(fmt::format("Improper value for {}", token.c_str()));
                return HPWH_ABORT;
            }
        }
        else if (token == "heatExchangerEffectiveness")
        {
            // applies to heat-exchange models only
            line_ss >> tempDouble;
            tank->heatExchangerEffectiveness = tempDouble;
        }
        else if (token == "verbosity")
        {
        }

        else if (token == "numHeatSources")
        {
            line_ss >> numHeatSources;
            heatSources.reserve(numHeatSources);
            for (std::size_t i = 0; i < numHeatSources; i++)
            {
                heatSources.emplace_back(this);
            }
        }
        else if (token == "heatsource")
        {
            if (numHeatSources == 0)
            {
                send_error(
                    "You must specify the number of heatsources before setting their properties.");
                return HPWH_ABORT;
            }
            line_ss >> heatsource >> token;
            if (token == "isVIP")
            {
                line_ss >> tempString;
                if (tempString == "true")
                    heatSources[heatsource].isVIP = true;
                else if (tempString == "false")
                    heatSources[heatsource].isVIP = false;
                else
                {
                    send_error(fmt::format(
                        "Improper value for {} for heat source {}.", token.c_str(), heatsource));
                    return HPWH_ABORT;
                }
            }
            else if (token == "isOn")
            {
                line_ss >> tempString;
                if (tempString == "true")
                    heatSources[heatsource].isOn = true;
                else if (tempString == "false")
                    heatSources[heatsource].isOn = false;
                else
                {
                    send_error(fmt::format(
                        "Improper value for {} for heat source {}.", token.c_str(), heatsource));
                    return HPWH_ABORT;
                }
            }
            else if (token == "minT")
            {
                line_ss >> tempDouble >> units;
                if (units == "F")
                    tempDouble = F_TO_C(tempDouble);
                else if (units == "C")
                    ; // do nothing, lol
                else
                {
                    send_error(fmt::format("Incorrect units specification for {}.", token.c_str()));
                    return HPWH_ABORT;
                }
                heatSources[heatsource].minT = tempDouble;
            }
            else if (token == "maxT")
            {
                line_ss >> tempDouble >> units;
                if (units == "F")
                    tempDouble = F_TO_C(tempDouble);
                else if (units == "C")
                    ; // do nothing, lol
                else
                {
                    send_error(fmt::format("Incorrect units specification for {}.", token.c_str()));
                    return HPWH_ABORT;
                }
                heatSources[heatsource].maxT = tempDouble;
            }
            else if (token == "onlogic" || token == "offlogic" || token == "standbylogic")
            {
                line_ss >> tempString;
                if (tempString == "nodes")
                {
                    std::vector<int> nodeNums;
                    std::vector<double> weights;
                    std::string nextToken;
                    line_ss >> nextToken;
                    while (std::regex_match(nextToken, std::regex("\\d+")))
                    {
                        int nodeNum = std::stoi(nextToken);
                        if (nodeNum > LOGIC_SIZE + 1 || nodeNum < 0)
                        {
                            send_error(fmt::format(
                                "Node number for heatsource {} {} must be between 0 and {}.",
                                heatsource,
                                token.c_str(),
                                LOGIC_SIZE + 1));
                            return HPWH_ABORT;
                        }
                        nodeNums.push_back(nodeNum);
                        line_ss >> nextToken;
                    }
                    if (nextToken == "weights")
                    {
                        line_ss >> nextToken;
                        while (std::regex_match(nextToken, std::regex("-?\\d*\\.\\d+(?:e-?\\d+)?")))
                        {
                            weights.push_back(std::stod(nextToken));
                            line_ss >> nextToken;
                        }
                    }
                    else
                    {
                        for ([[maybe_unused]] auto n : nodeNums)
                        {
                            weights.push_back(1.0);
                        }
                    }
                    if (nodeNums.size() != weights.size())
                    {
                        send_error(fmt::format(
                            "Number of weights for heatsource {} {} ({}) does not match number "
                            "of nodes for {} ({}).",
                            heatsource,
                            token.c_str(),
                            weights.size(),
                            token.c_str(),
                            nodeNums.size()));
                        return HPWH_ABORT;
                    }
                    if (nextToken != "absolute" && nextToken != "relative")
                    {
                        send_error(fmt::format(
                            "Improper definition, \"{}\", for heat source {} {}. Should be "
                            "\"relative\" or \"absoute\".",
                            nextToken.c_str(),
                            heatsource,
                            token.c_str()));
                        return HPWH_ABORT;
                    }
                    bool absolute = (nextToken == "absolute");
                    std::string compareStr;
                    line_ss >> compareStr >> tempDouble >> units;
                    std::function<bool(double, double)> compare;
                    if (compareStr == "<")
                        compare = std::less<double>();
                    else if (compareStr == ">")
                        compare = std::greater<double>();
                    else
                    {
                        send_error(fmt::format(
                            "Improper comparison, \"%s\", for heat source %d %s. Should be "
                            "\"<\" or \">\".\n",
                            compareStr.c_str(),
                            heatsource,
                            token.c_str()));
                        return HPWH_ABORT;
                    }
                    if (units == "F")
                    {
                        if (absolute)
                        {
                            tempDouble = F_TO_C(tempDouble);
                        }
                        else
                        {
                            tempDouble = dF_TO_dC(tempDouble);
                        }
                    }
                    else if (units == "C")
                        ; // do nothing, lol
                    else
                    {
                        send_error(
                            fmt::format("Incorrect units specification for {} from heatsource {}.",
                                        token.c_str(),
                                        heatsource));
                        return HPWH_ABORT;
                    }
                    std::vector<NodeWeight> nodeWeights;
                    for (size_t i = 0; i < nodeNums.size(); i++)
                    {
                        nodeWeights.emplace_back(nodeNums[i], weights[i]);
                    }
                    std::shared_ptr<HPWH::TempBasedHeatingLogic> logic =
                        std::make_shared<HPWH::TempBasedHeatingLogic>(
                            "custom", nodeWeights, tempDouble, this, absolute, compare);
                    if (token == "onlogic")
                    {
                        heatSources[heatsource].addTurnOnLogic(logic);
                    }
                    else if (token == "offlogic")
                    {
                        heatSources[heatsource].addShutOffLogic(std::move(logic));
                    }
                    else
                    { // standby logic
                        heatSources[heatsource].standbyLogic =
                            std::make_shared<HPWH::TempBasedHeatingLogic>(
                                "standby logic", nodeWeights, tempDouble, this, absolute, compare);
                    }
                }
                else if (token == "onlogic")
                {
                    std::string nextToken;
                    line_ss >> nextToken;
                    bool absolute = (nextToken == "absolute");
                    if (absolute)
                    {
                        std::string compareStr;
                        line_ss >> compareStr >> tempDouble >> units;
                        std::function<bool(double, double)> compare;
                        if (compareStr == "<")
                            compare = std::less<double>();
                        else if (compareStr == ">")
                            compare = std::greater<double>();
                        else
                        {
                            send_error(fmt::format(
                                "Improper comparison, \"%s\", for heat source %d %s. Should be "
                                "\"<\" or \">\".",
                                compareStr.c_str(),
                                heatsource,
                                token.c_str()));
                            return HPWH_ABORT;
                        }
                        line_ss >> tempDouble;
                    }
                    else
                    {
                        tempDouble = std::stod(nextToken);
                    }
                    line_ss >> units;
                    if (units == "F")
                    {
                        if (absolute)
                        {
                            tempDouble = F_TO_C(tempDouble);
                        }
                        else
                        {
                            tempDouble = dF_TO_dC(tempDouble);
                        }
                    }
                    else if (units == "C")
                        ; // do nothing, lol
                    else
                    {
                        send_error(
                            fmt::format("Incorrect units specification for %s from heatsource %d.",
                                        token.c_str(),
                                        heatsource));
                        return HPWH_ABORT;
                    }
                    if (tempString == "wholeTank")
                    {
                        heatSources[heatsource].addTurnOnLogic(
                            HPWH::wholeTank(tempDouble, UNITS_C, absolute));
                    }
                    else if (tempString == "topThird")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::topThird(tempDouble));
                    }
                    else if (tempString == "bottomThird")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::bottomThird(tempDouble));
                    }
                    else if (tempString == "standby")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::standby(tempDouble));
                    }
                    else if (tempString == "bottomSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::bottomSixth(tempDouble));
                    }
                    else if (tempString == "secondSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::secondSixth(tempDouble));
                    }
                    else if (tempString == "thirdSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::thirdSixth(tempDouble));
                    }
                    else if (tempString == "fourthSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::fourthSixth(tempDouble));
                    }
                    else if (tempString == "fifthSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::fifthSixth(tempDouble));
                    }
                    else if (tempString == "topSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::topSixth(tempDouble));
                    }
                    else if (tempString == "bottomHalf")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::bottomHalf(tempDouble));
                    }
                    else
                    {
                        send_error(fmt::format(
                            "Improper %s for heat source %d\n", token.c_str(), heatsource));
                        return HPWH_ABORT;
                    }
                }
                else if (token == "offlogic")
                {
                    line_ss >> tempDouble >> units;
                    if (units == "F")
                        tempDouble = F_TO_C(tempDouble);
                    else if (units == "C")
                        ; // do nothing, lol
                    else
                    {
                        send_error(
                            fmt::format("Incorrect units specification for %s from heatsource %d.",
                                        token.c_str(),
                                        heatsource));
                        return HPWH_ABORT;
                    }
                    if (tempString == "topNodeMaxTemp")
                    {
                        heatSources[heatsource].addShutOffLogic(HPWH::topNodeMaxTemp(tempDouble));
                    }
                    else if (tempString == "bottomNodeMaxTemp")
                    {
                        heatSources[heatsource].addShutOffLogic(
                            HPWH::bottomNodeMaxTemp(tempDouble));
                    }
                    else if (tempString == "bottomTwelfthMaxTemp")
                    {
                        heatSources[heatsource].addShutOffLogic(
                            HPWH::bottomTwelfthMaxTemp(tempDouble));
                    }
                    else if (tempString == "bottomSixthMaxTemp")
                    {
                        heatSources[heatsource].addShutOffLogic(
                            HPWH::bottomSixthMaxTemp(tempDouble));
                    }
                    else if (tempString == "largeDraw")
                    {
                        heatSources[heatsource].addShutOffLogic(HPWH::largeDraw(tempDouble));
                    }
                    else if (tempString == "largerDraw")
                    {
                        heatSources[heatsource].addShutOffLogic(HPWH::largerDraw(tempDouble));
                    }
                    else
                    {
                        send_error(fmt::format(
                            "Improper %s for heat source %d\n", token.c_str(), heatsource));
                        return HPWH_ABORT;
                    }
                }
            }
            else if (token == "type")
            {
                line_ss >> tempString;
                if (tempString == "resistor")
                {
                    heatSources[heatsource].typeOfHeatSource = TYPE_resistance;
                }
                else if (tempString == "compressor")
                {
                    heatSources[heatsource].typeOfHeatSource = TYPE_compressor;
                }
                else
                {
                    send_error(
                        fmt::format("Improper %s for heat source %d\n", token.c_str(), heatsource));
                    return HPWH_ABORT;
                }
            }
            else if (token == "coilConfig")
            {
                line_ss >> tempString;
                if (tempString == "wrapped")
                {
                    heatSources[heatsource].configuration = HeatSource::CONFIG_WRAPPED;
                }
                else if (tempString == "submerged")
                {
                    heatSources[heatsource].configuration = HeatSource::CONFIG_SUBMERGED;
                }
                else if (tempString == "external")
                {
                    heatSources[heatsource].configuration = HeatSource::CONFIG_EXTERNAL;
                }
                else
                {
                    send_error(
                        fmt::format("Improper %s for heat source %d", token.c_str(), heatsource));
                    return HPWH_ABORT;
                }
            }
            else if (token == "heatCycle")
            {
                line_ss >> tempString;
                if (tempString == "singlepass")
                {
                    heatSources[heatsource].isMultipass = false;
                }
                else if (tempString == "multipass")
                {
                    heatSources[heatsource].isMultipass = true;
                }
                else
                {
                    send_error(
                        fmt::format("Improper %s for heat source %d\n", token.c_str(), heatsource));
                    return HPWH_ABORT;
                }
            }

            else if (token == "externalInlet")
            {
                line_ss >> tempInt;
                if (tempInt < num_nodes)
                {
                    heatSources[heatsource].externalInletHeight = static_cast<int>(tempInt);
                }
                else
                {
                    send_error(
                        fmt::format("Improper %s for heat source %d\n", token.c_str(), heatsource));
                    return HPWH_ABORT;
                }
            }
            else if (token == "externalOutlet")
            {
                line_ss >> tempInt;
                if (tempInt < num_nodes)
                {
                    heatSources[heatsource].externalOutletHeight = static_cast<int>(tempInt);
                }
                else
                {
                    send_error(
                        fmt::format("Improper %s for heat source %d\n", token.c_str(), heatsource));
                    return HPWH_ABORT;
                }
            }

            else if (token == "condensity")
            {
                double x;
                std::vector<double> condensity;
                while (line_ss >> x)
                    condensity.push_back(x);
                heatSources[heatsource].setCondensity(condensity);
            }
            else if (token == "nTemps")
            {
                line_ss >> nTemps;
                heatSources[heatsource].perfMap.resize(nTemps);
            }
            else if (std::regex_match(token, std::regex("T\\d+")))
            {
                std::smatch match;
                std::regex_match(token, match, std::regex("T(\\d+)"));
                nTemps = std::stoi(match[1].str());
                std::size_t maxTemps = heatSources[heatsource].perfMap.size();

                if (maxTemps < nTemps)
                {
                    if (maxTemps == 0)
                    {
                        send_error(fmt::format(
                            "%s specified for heatsource %d before definition of nTemps.",
                            token.c_str(),
                            heatsource));
                        return HPWH_ABORT;
                    }
                    else
                    {
                        send_error(fmt::format(
                            "Incorrect specification for %s from heatsource %d. nTemps, %d, is "
                            "less than %d.  \n",
                            token.c_str(),
                            heatsource,
                            maxTemps,
                            nTemps));
                        return HPWH_ABORT;
                    }
                }
                line_ss >> tempDouble >> units;
                //        if (units == "F")  tempDouble = F_TO_C(tempDouble);
                if (units == "F")
                    ;
                //        else if (units == "C") ; //do nothing, lol
                else if (units == "C")
                    tempDouble = C_TO_F(tempDouble);
                else
                {
                    send_error(
                        fmt::format("Incorrect units specification for %s from heatsource %d.",
                                    token.c_str(),
                                    heatsource));
                    return HPWH_ABORT;
                }
                heatSources[heatsource].perfMap[nTemps - 1].T_F = tempDouble;
            }
            else if (std::regex_match(token, std::regex("(?:inPow|cop)T\\d+(?:const|lin|quad)")))
            {
                std::smatch match;
                std::regex_match(token, match, std::regex("(inPow|cop)T(\\d+)(const|lin|quad)"));
                string var = match[1].str();
                nTemps = std::stoi(match[2].str());
                string coeff = match[3].str();

                /*
                // TODO: Currently relies on the coefficients being defined in the correct order
                int coeff_num;
                if (coeff == "const")
                {
                    coeff_num = 0;
                }
                else if (coeff == "lin")
                {
                    coeff_num = 1;
                }
                else if (coeff == "quad")
                {
                    coeff_num = 2;
                }
                */

                std::size_t maxTemps = heatSources[heatsource].perfMap.size();

                if (maxTemps < nTemps)
                {
                    if (maxTemps == 0)
                    {
                        send_error(fmt::format(
                            "%s specified for heatsource %d before definition of nTemps.",
                            token.c_str(),
                            heatsource));
                        return HPWH_ABORT;
                    }
                    else
                    {
                        send_error(fmt::format(
                            "Incorrect specification for %s from heatsource %d. nTemps, %d, is "
                            "less than %d.  \n",
                            token.c_str(),
                            heatsource,
                            maxTemps,
                            nTemps));
                        return HPWH_ABORT;
                    }
                }
                line_ss >> tempDouble;

                if (var == "inPow")
                {
                    heatSources[heatsource].perfMap[nTemps - 1].inputPower_coeffs.push_back(
                        tempDouble);
                }
                else if (var == "cop")
                {
                    heatSources[heatsource].perfMap[nTemps - 1].COP_coeffs.push_back(tempDouble);
                }
            }
            else if (token == "hysteresis")
            {
                line_ss >> tempDouble >> units;
                if (units == "F")
                    tempDouble = dF_TO_dC(tempDouble);
                else if (units == "C")
                    ; // do nothing, lol
                else
                {
                    send_error(
                        fmt::format("Incorrect units specification for %s from heatsource %d.",
                                    token.c_str(),
                                    heatsource));
                    return HPWH_ABORT;
                }
                heatSources[heatsource].hysteresis_dC = tempDouble;
            }
            else if (token == "backupSource")
            {
                line_ss >> sourceNum;
                heatSources[heatsource].backupHeatSource = &heatSources[sourceNum];
            }
            else if (token == "companionSource")
            {
                line_ss >> sourceNum;
                heatSources[heatsource].companionHeatSource = &heatSources[sourceNum];
            }
            else if (token == "followedBySource")
            {
                line_ss >> sourceNum;
                heatSources[heatsource].followedByHeatSource = &heatSources[sourceNum];
            }
            else
            {
                send_error(fmt::format(
                    "Improper specifier (%s) for heat source %d\n", token.c_str(), heatsource));
            }

        } // end heatsource options
        else
        {
            send_error(fmt::format("Improper keyword: %s", token.c_str()));
        }

    } // end while over lines

    // take care of the non-input processing
    model = MODELS_CustomFile;

    tank->setNumNodes(num_nodes);

    if (hasInitialTankTemp)
        setTankToTemperature(initalTankT_C);
    else
        resetTankToSetpoint();

    isHeating = false;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isOn)
        {
            isHeating = true;
        }
        heatSources[i].sortPerformanceMap();
    }

    calcDerivedValues();

    if (checkInputs() == HPWH_ABORT)
    {
        send_error("Invalid input.");
    }
    return 0;
}
#endif

void HPWH::init(hpwh_data_model::rsintegratedwaterheater_ns::RSINTEGRATEDWATERHEATER& rswh)
{
    auto& performance = rswh.performance;
    auto& rstank = performance.tank;

    setpoint_C = F_TO_C(135.0);

    tank->init(rstank);
    for (auto& heatsourceconfiguration : performance.heat_source_configurations)
    {
        HeatSource heatSource(this);
        heatSource.init(heatsourceconfiguration);
        heatSources.push_back(heatSource);
    }
    std::cout << "\n";
}

//-----------------------------------------------------------------------------
///	@brief	Performs a draw/heat cycle to prep for test
/// @return	true (success), false (failure).
//-----------------------------------------------------------------------------
bool HPWH::prepForTest(StandardTestOptions& testOptions)
{
    double flowRate_Lper_min = GAL_TO_L(3.);
    if (tank->getVolume_L() < GAL_TO_L(20.))
        flowRate_Lper_min = GAL_TO_L(1.5);

    constexpr double inletT_C = 14.4;   // EERE-2019-BT-TP-0032-0058, p. 40433
    constexpr double ambientT_C = 19.7; // EERE-2019-BT-TP-0032-0058, p. 40435
    constexpr double externalT_C = 19.7;

    if (testOptions.changeSetpoint)
    {
        if (!isSetpointFixed())
        {
            if (setSetpoint(testOptions.setpointT_C, UNITS_C) == HPWH_ABORT)
            {
                return false;
            }
        }
    }

    DRMODES drMode = DR_ALLOW;
    bool isDrawing = false;
    bool done = false;
    int step = 0;
    int time_min = 0;
    while (!done)
    {
        switch (step)
        {
        case 0: // start with heat off
        {
            if (!isHeating)
            {
                isDrawing = true;
                ++step;
            }
            break;
        }

        case 1: // draw until heat turns on
        {
            if (isHeating)
            {
                isDrawing = false;
                ++step;
            }
            break;
        }

        case 2: // wait for heat to turn on
        {
            if (!isHeating)
            {
                isDrawing = false;
                done = true;
            }
            break;
        }
        }

        // limit draw-volume increment to tank volume
        double incrementalDrawVolume_L = isDrawing ? flowRate_Lper_min * (1.) : 0.;
        if (incrementalDrawVolume_L > tank->getVolume_L())
        {
            incrementalDrawVolume_L = tank->getVolume_L();
        }

        if (runOneStep(inletT_C,                // inlet water temperature (C)
                       incrementalDrawVolume_L, // draw volume (L)
                       ambientT_C,              // ambient Temp (C)
                       externalT_C,             // external Temp (C)
                       drMode,                  // DDR Status
                       0.,                      // inlet-2 volume (L)
                       inletT_C,                // inlet-2 Temp (C)
                       NULL)                    // no extra heat
            == HPWH_ABORT)
        {
            return false;
        }

        ++time_min;
    }
    return true;
}

//-----------------------------------------------------------------------------
///	@brief	Find first-hour rating designation for 24-hr test
/// @param[out] firstHourRating	    contains first-hour rating designation
///	@param[in]	setpointT_C		    setpoint temperature (optional)
/// @return	true (success), false (failure).
//-----------------------------------------------------------------------------
bool HPWH::findFirstHourRating(FirstHourRating& firstHourRating, StandardTestOptions& testOptions)
{
    double flowRate_Lper_min = GAL_TO_L(3.);
    if (tank->getVolume_L() < GAL_TO_L(20.))
        flowRate_Lper_min = GAL_TO_L(1.5);

    constexpr double inletT_C = 14.4;   // EERE-2019-BT-TP-0032-0058, p. 40433
    constexpr double ambientT_C = 19.7; // EERE-2019-BT-TP-0032-0058, p. 40435
    constexpr double externalT_C = 19.7;

    if (testOptions.changeSetpoint)
    {
        if (!isSetpointFixed())
        {
            if (setSetpoint(testOptions.setpointT_C, UNITS_C) == HPWH_ABORT)
            {
                return false;
            }
        }
    }

    double tankT_C = getAverageTankTemp_C();
    double maxTankT_C = tankT_C;
    double maxOutletT_C = 0.;

    DRMODES drMode = DR_ALLOW;
    double drawVolume_L = 0.;

    firstHourRating.drawVolume_L = 0.;

    double sumOutletVolumeT_LC = 0.;
    double sumOutletVolume_L = 0.;

    double avgOutletT_C = 0.;
    double minOutletT_C = 0.;
    double prevAvgOutletT_C = 0.;
    double prevMinOutletT_C = 0.;

    bool isDrawing = false;
    bool done = false;
    int step = 0;

    if (!prepForTest(testOptions))
    {
        return false;
    }

    bool firstDraw = true;
    isDrawing = true;
    maxOutletT_C = 0.;
    drMode = DR_LOC;
    int elapsedTime_min = 0;
    while (!done)
    {

        // limit draw-volume increment to tank volume
        double incrementalDrawVolume_L = isDrawing ? flowRate_Lper_min * (1.) : 0.;
        if (incrementalDrawVolume_L > tank->getVolume_L())
        {
            incrementalDrawVolume_L = tank->getVolume_L();
        }

        if (runOneStep(inletT_C,                // inlet water temperature (C)
                       incrementalDrawVolume_L, // draw volume (L)
                       ambientT_C,              // ambient Temp (C)
                       externalT_C,             // external Temp (C)
                       drMode,                  // DDR Status
                       0.,                      // inlet-2 volume (L)
                       inletT_C,                // inlet-2 Temp (C)
                       NULL)                    // no extra heat
            == HPWH_ABORT)
        {
            return false;
        }
        tankT_C = getAverageTankTemp_C();

        switch (step)
        {
        case 0: // drawing
        {
            sumOutletVolume_L += incrementalDrawVolume_L;
            sumOutletVolumeT_LC += incrementalDrawVolume_L * tank->getOutletT_C();

            maxOutletT_C = std::max(tank->getOutletT_C(), maxOutletT_C);
            if (tank->getOutletT_C() <
                maxOutletT_C - dF_TO_dC(15.)) // outletT has dropped by 15 degF below max T
            {
                avgOutletT_C = sumOutletVolumeT_LC / sumOutletVolume_L;
                minOutletT_C = tank->getOutletT_C();
                if (elapsedTime_min >= 60)
                {
                    double fac = 1;
                    if (!firstDraw)
                    {
                        fac = (avgOutletT_C - prevMinOutletT_C) /
                              (prevAvgOutletT_C - prevMinOutletT_C);
                    }
                    firstHourRating.drawVolume_L += fac * drawVolume_L;
                    done = true;
                }
                else
                {
                    firstHourRating.drawVolume_L += drawVolume_L;
                    drawVolume_L = 0.;
                    isDrawing = false;
                    drMode = DR_ALLOW;
                    maxTankT_C = tankT_C;                // initialize for next pass
                    maxOutletT_C = tank->getOutletT_C(); // initialize for next pass
                    prevAvgOutletT_C = avgOutletT_C;
                    prevMinOutletT_C = minOutletT_C;
                    ++step;
                }
            }
            break;
        }

        case 1:
        {
            if (isHeating) // ensure heat is on before proceeding
            {
                ++step;
            }
            break;
        }

        case 2: // heating
        {
            if ((tankT_C > maxTankT_C) && isHeating &&
                (elapsedTime_min <
                 60)) // has not reached maxTankT, heat is on, and less than 1 hr has elpased
            {
                maxTankT_C = std::max(tankT_C, maxTankT_C);
            }
            else // start another draw
            {
                firstDraw = false;
                isDrawing = true;
                drawVolume_L = 0.;
                drMode = DR_LOC;
                step = 0; // repeat
            }
        }
        }

        drawVolume_L += incrementalDrawVolume_L;
        ++elapsedTime_min;
    }

    //

    if (firstHourRating.drawVolume_L < GAL_TO_L(18.))
    {
        firstHourRating.desig = FirstHourRating::Desig::VerySmall;
    }
    else if (firstHourRating.drawVolume_L < GAL_TO_L(51.))
    {
        firstHourRating.desig = FirstHourRating::Desig::Low;
    }
    else if (firstHourRating.drawVolume_L < GAL_TO_L(75.))
    {
        firstHourRating.desig = FirstHourRating::Desig::Medium;
    }
    else
    {
        firstHourRating.desig = FirstHourRating::Desig::High;
    }

    const std::string sFirstHourRatingDesig =
        HPWH::FirstHourRating::sDesigMap[firstHourRating.desig];

    std::cout << "\tFirst-Hour Rating:\n";
    std::cout << "\t\tVolume Drawn (L): " << firstHourRating.drawVolume_L << "\n";
    std::cout << "\t\tDesignation: " << sFirstHourRatingDesig << "\n";

    return true;
}

//-----------------------------------------------------------------------------
///	@brief	Performs standard 24-hr test
/// @note	see https://www.regulations.gov/document/EERE-2019-BT-TP-0032-0058
/// @param[in] firstHourRating          specifies first-hour rating
/// @param[out] testSummary	            contains test metrics on output
///	@param[in]	setpointT_C		        setpoint temperature (optional)
/// @return	true (success), false (failure).
//-----------------------------------------------------------------------------
bool HPWH::run24hrTest(const FirstHourRating firstHourRating,
                       StandardTestSummary& testSummary,
                       StandardTestOptions& testOptions)
{
    // select the first draw cluster size and pattern
    auto firstDrawClusterSize = firstDrawClusterSizes[firstHourRating.desig];
    DrawPattern& drawPattern = drawPatterns[firstHourRating.desig];

    constexpr double inletT_C = 14.4;   // EERE-2019-BT-TP-0032-0058, p. 40433
    constexpr double ambientT_C = 19.7; // EERE-2019-BT-TP-0032-0058, p. 40435
    constexpr double externalT_C = 19.7;
    DRMODES drMode = DR_ALLOW;

    if (testOptions.changeSetpoint)
    {
        if (!isSetpointFixed())
        {
            if (setSetpoint(testOptions.setpointT_C, UNITS_C) == HPWH_ABORT)
            {
                return false;
            }
        }
    }

    if (!prepForTest(testOptions))
    {
        return false;
    }

    std::vector<OutputData> outputDataSet;

    // idle for 1 hr
    int preTime_min = 0;
    bool heatersAreOn = false;
    while ((preTime_min < 60) || heatersAreOn)
    {
        if (runOneStep(inletT_C,    // inlet water temperature (C)
                       0,           // draw volume (L)
                       ambientT_C,  // ambient Temp (C)
                       externalT_C, // external Temp (C)
                       drMode,      // DDR Status
                       0.,          // inlet-2 volume (L)
                       inletT_C,    // inlet-2 Temp (C)
                       NULL)        // no extra heat
            == HPWH_ABORT)
        {
            return false;
        }

        heatersAreOn = false;
        for (auto& heatSource : heatSources)
        {
            heatersAreOn |= heatSource.isEngaged();
        }

        {
            OutputData outputData;
            outputData.time_min = preTime_min;
            outputData.ambientT_C = ambientT_C;
            outputData.setpointT_C = getSetpoint();
            outputData.inletT_C = inletT_C;
            outputData.drawVolume_L = 0.;
            outputData.drMode = drMode;

            for (int iHS = 0; iHS < getNumHeatSources(); ++iHS)
            {
                outputData.h_srcIn_kWh.push_back(getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KWH));
                outputData.h_srcOut_kWh.push_back(
                    getNthHeatSourceEnergyOutput(iHS, HPWH::UNITS_KWH));
            }

            for (int iTC = 0; iTC < testOptions.nTestTCouples; ++iTC)
            {
                outputData.thermocoupleT_C.push_back(
                    getNthSimTcouple(iTC + 1, testOptions.nTestTCouples, UNITS_C));
            }
            outputData.outletT_C = 0.;

            outputDataSet.push_back(outputData);
        }

        ++preTime_min;
    }

    // correct time to start test at 0 min
    for (auto& outputData : outputDataSet)
    {
        outputData.time_min -= preTime_min;
    }

    double tankT_C = getAverageTankTemp_C();
    double initialTankT_C = tankT_C;

    // used to find average draw temperatures
    double drawSumOutletVolumeT_LC = 0.;
    double drawSumInletVolumeT_LC = 0.;

    // used to find average 24-hr test temperatures
    double sumOutletVolumeT_LC = 0.;
    double sumInletVolumeT_LC = 0.;

    // first-recovery info
    testSummary.recoveryStoredEnergy_kJ = 0.;
    testSummary.recoveryDeliveredEnergy_kJ = 0.;
    testSummary.recoveryUsedEnergy_kJ = 0.;

    double deliveredEnergy_kJ = 0.; // total energy delivered to water
    testSummary.removedVolume_L = 0.;
    testSummary.usedEnergy_kJ = 0.;           // Q
    testSummary.usedFossilFuelEnergy_kJ = 0.; // total fossil-fuel energy consumed, Qf
    testSummary.usedElectricalEnergy_kJ = 0.; // total electrical energy consumed, Qe

    bool hasHeated = false;

    int endTime_min = 24 * static_cast<int>(min_per_hr);
    std::size_t iDraw = 0;
    double remainingDrawVolume_L = 0.;
    double drawVolume_L = 0.;
    double prevDrawEndTime_min = 0.;

    bool isFirstRecoveryPeriod = true;
    bool isInFirstDrawCluster = true;
    bool hasStandbyPeriodStarted = false;
    bool hasStandbyPeriodEnded = false;
    bool nextDraw = true;
    bool isDrawing = false;
    bool isDrawPatternComplete = false;

    int recoveryEndTime_min = 0;

    int standbyStartTime_min = 0;
    int standbyEndTime_min = 0;
    double standbyStartT_C = 0;
    double standbyEndT_C = 0;
    double standbyStartTankEnergy_kJ = 0.;
    double standbyEndTankEnergy_kJ = 0.;
    double standbySumTimeTankT_minC = 0.;
    double standbySumTimeAmbientT_minC = 0.;

    int noDrawTotalTime_min = 0;
    double noDrawSumTimeAmbientT_minC = 0.;

    bool inLastHour = false;
    double stepDrawVolume_L = 0.;
    for (int runTime_min = 0; runTime_min <= endTime_min; ++runTime_min)
    {
        if (inLastHour)
        {
            drMode = DR_LOC | DR_LOR;
        }

        if (nextDraw)
        {
            auto& draw = drawPattern[iDraw];
            if (runTime_min >= draw.startTime_min)
            {
                // limit draw-volume step to tank volume
                stepDrawVolume_L = draw.flowRate_Lper_min * (1.);
                if (stepDrawVolume_L > tank->volume_L)
                {
                    stepDrawVolume_L = tank->volume_L;
                }

                remainingDrawVolume_L = drawVolume_L = draw.volume_L;

                nextDraw = false;
                isDrawing = true;

                drawSumOutletVolumeT_LC = 0.;
                drawSumInletVolumeT_LC = 0.;

                if (hasStandbyPeriodStarted && (!hasStandbyPeriodEnded))
                {
                    hasStandbyPeriodEnded = true;
                    standbyEndTankEnergy_kJ = testSummary.usedEnergy_kJ; // Qsu,0
                    standbyEndT_C = tankT_C;                             // Tsu,0
                    standbyEndTime_min = runTime_min;
                }
            }
        }

        // iterate until 1) specified draw volume has been reached and 2) next draw has started
        // do not exceed specified draw volume
        if (isDrawing)
        {
            if (stepDrawVolume_L >= remainingDrawVolume_L)
            {
                stepDrawVolume_L = remainingDrawVolume_L;
                remainingDrawVolume_L = 0.;
                isDrawing = false;
                prevDrawEndTime_min = runTime_min;
            }
            else
            {
                remainingDrawVolume_L -= stepDrawVolume_L;
            }
        }
        else
        {
            remainingDrawVolume_L = stepDrawVolume_L = 0.;
            noDrawSumTimeAmbientT_minC += (1.) * ambientT_C;
            ++noDrawTotalTime_min;
        }

        // run a step
        int runResult = runOneStep(inletT_C,         // inlet water temperature (C)
                                   stepDrawVolume_L, // draw volume (L)
                                   ambientT_C,       // ambient Temp (C)
                                   externalT_C,      // external Temp (C)
                                   drMode,           // DDR Status
                                   0.,               // inlet-2 volume (L)
                                   inletT_C,         // inlet-2 Temp (C)
                                   NULL);            // no extra heat

        if (runResult == HPWH_ABORT)
        {
            return false;
        }

        {
            OutputData outputData;
            outputData.time_min = runTime_min;
            outputData.ambientT_C = ambientT_C;
            outputData.setpointT_C = getSetpoint();
            outputData.inletT_C = inletT_C;
            outputData.drawVolume_L = stepDrawVolume_L;
            outputData.drMode = drMode;

            for (int iHS = 0; iHS < getNumHeatSources(); ++iHS)
            {
                outputData.h_srcIn_kWh.push_back(getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KWH));
                outputData.h_srcOut_kWh.push_back(
                    getNthHeatSourceEnergyOutput(iHS, HPWH::UNITS_KWH));
            }

            for (int iTC = 0; iTC < testOptions.nTestTCouples; ++iTC)
            {
                outputData.thermocoupleT_C.push_back(
                    getNthSimTcouple(iTC + 1, testOptions.nTestTCouples, UNITS_C));
            }
            outputData.outletT_C = tank->getOutletT_C();

            outputDataSet.push_back(outputData);
        }

        tankT_C = getAverageTankTemp_C();
        hasHeated |= isHeating;

        drawSumOutletVolumeT_LC += stepDrawVolume_L * tank->getOutletT_C();
        drawSumInletVolumeT_LC += stepDrawVolume_L * inletT_C;

        sumOutletVolumeT_LC += stepDrawVolume_L * tank->getOutletT_C();
        sumInletVolumeT_LC += stepDrawVolume_L * inletT_C;

        // collect energy added to water
        double stepDrawMass_kg = DENSITYWATER_kgperL * stepDrawVolume_L;
        double stepDrawHeatCapacity_kJperC = CPWATER_kJperkgC * stepDrawMass_kg;
        deliveredEnergy_kJ += stepDrawHeatCapacity_kJperC * (tank->getOutletT_C() - inletT_C);

        // collect used-energy info
        double usedFossilFuelEnergy_kJ = 0.;
        double usedElectricalEnergy_kJ = 0.;
        for (int iHS = 0; iHS < getNumHeatSources(); ++iHS)
        {
            usedElectricalEnergy_kJ += getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KJ);
        }

        // collect 24-hr test info
        testSummary.removedVolume_L += stepDrawVolume_L;
        testSummary.usedFossilFuelEnergy_kJ += usedFossilFuelEnergy_kJ;
        testSummary.usedElectricalEnergy_kJ += usedElectricalEnergy_kJ;
        testSummary.usedEnergy_kJ += usedFossilFuelEnergy_kJ + usedElectricalEnergy_kJ;

        if (isFirstRecoveryPeriod)
        {
            testSummary.recoveryUsedEnergy_kJ += usedFossilFuelEnergy_kJ + usedElectricalEnergy_kJ;
        }

        if (!isDrawing)
        {
            if (isFirstRecoveryPeriod)
            {
                if (hasHeated && (!isHeating))
                {
                    // collect recovery info
                    isFirstRecoveryPeriod = false;

                    double tankContentMass_kg = DENSITYWATER_kgperL * tank->getVolume_L();
                    double tankHeatCapacity_kJperC = CPWATER_kJperkgC * tankContentMass_kg;
                    testSummary.recoveryStoredEnergy_kJ =
                        tankHeatCapacity_kJperC * (tankT_C - initialTankT_C);
                }

                if (!nextDraw)
                {
                    double meanDrawOutletT_C = drawSumOutletVolumeT_LC / drawVolume_L;
                    double meanDrawInletT_C = drawSumInletVolumeT_LC / drawVolume_L;

                    double drawMass_kg = DENSITYWATER_kgperL * drawVolume_L;
                    double drawHeatCapacity_kJperC = CPWATER_kJperkgC * drawMass_kg;

                    testSummary.recoveryDeliveredEnergy_kJ +=
                        drawHeatCapacity_kJperC * (meanDrawOutletT_C - meanDrawInletT_C);
                }
            }

            if (!hasStandbyPeriodEnded)
            {
                if (hasStandbyPeriodStarted)
                {
                    standbySumTimeTankT_minC += (1.) * tankT_C;
                    standbySumTimeAmbientT_minC += (1.) * ambientT_C;

                    if (runTime_min >= standbyStartTime_min + 8 * min_per_hr)
                    {
                        hasStandbyPeriodEnded = true;
                        standbyEndTankEnergy_kJ = testSummary.usedEnergy_kJ; // Qsu,0
                        standbyEndT_C = tankT_C;                             // Tsu,0
                        standbyEndTime_min = runTime_min;
                    }
                }
                else
                {
                    if (isHeating)
                    {
                        recoveryEndTime_min = runTime_min;
                    }
                    else
                    {
                        if ((!isInFirstDrawCluster) || isDrawPatternComplete)
                        {
                            if ((runTime_min > prevDrawEndTime_min + 5) &&
                                (runTime_min > recoveryEndTime_min + 5))
                            {
                                hasStandbyPeriodStarted = true;
                                standbyStartTime_min = runTime_min;
                                standbyStartTankEnergy_kJ = testSummary.usedEnergy_kJ; // Qsu,0
                                standbyStartT_C = tankT_C;                             // Tsu,0

                                if (isDrawPatternComplete &&
                                    (runTime_min + 8 * min_per_hr > endTime_min))
                                {
                                    endTime_min = runTime_min + 8 * static_cast<int>(min_per_hr);
                                }
                            }
                        }
                    }
                }
            }

            if (!nextDraw)
            {
                ++iDraw;
                if (iDraw < drawPattern.size())
                {
                    nextDraw = true;
                    isInFirstDrawCluster = (iDraw < firstDrawClusterSize);
                }
                else
                {
                    isDrawPatternComplete = true;
                }
            }
        }
    }

    double finalTankT_C = tankT_C;

    if (!hasStandbyPeriodEnded)
    {
        hasStandbyPeriodEnded = true;
        standbyEndTime_min = endTime_min;
        standbyEndTankEnergy_kJ = testSummary.usedEnergy_kJ; // Qsu,0
        standbyEndT_C = tankT_C;                             // Tsu,0
    }

    if (testOptions.saveOutput)
    {
        for (auto& outputData : outputDataSet)
        {
            writeRowAsCSV(testOptions.outputFile, outputData);
        }
    }

    testSummary.avgOutletT_C = sumOutletVolumeT_LC / testSummary.removedVolume_L;
    testSummary.avgInletT_C = sumInletVolumeT_LC / testSummary.removedVolume_L;

    constexpr double standardSetpointT_C = 51.7;
    constexpr double standardInletT_C = 14.4;
    constexpr double standardAmbientT_C = 19.7;

    double tankContentMass_kg = DENSITYWATER_kgperL * tank->getVolume_L();
    double tankHeatCapacity_kJperC = CPWATER_kJperkgC * tankContentMass_kg;

    double removedMass_kg = DENSITYWATER_kgperL * testSummary.removedVolume_L;
    double removedHeatCapacity_kJperC = CPWATER_kJperkgC * removedMass_kg;

    // require heating during 24-hr test for unit to qualify as consumer water heater
    if (hasHeated && !isDrawing)
    {
        testSummary.qualifies = true;
    }

    // find the "Recovery Efficiency" (6.3.3)
    testSummary.recoveryEfficiency = 0.;
    if (testSummary.recoveryUsedEnergy_kJ > 0.)
    {
        testSummary.recoveryEfficiency =
            (testSummary.recoveryStoredEnergy_kJ + testSummary.recoveryDeliveredEnergy_kJ) /
            testSummary.recoveryUsedEnergy_kJ;
    }

    // find the energy consumed during the standby-loss test, Qstdby
    testSummary.standbyUsedEnergy_kJ = standbyEndTankEnergy_kJ - standbyStartTankEnergy_kJ;

    int standbyPeriodTime_min = standbyEndTime_min - standbyStartTime_min - 1;
    testSummary.standbyPeriodTime_h = standbyPeriodTime_min / min_per_hr; // tau_stby,1
    if ((testSummary.standbyPeriodTime_h > 0) && (testSummary.recoveryEfficiency > 0.))
    {
        double standardTankEnergy_kJ = tankHeatCapacity_kJperC * (standbyEndT_C - standbyStartT_C) /
                                       testSummary.recoveryEfficiency;
        testSummary.standbyHourlyLossEnergy_kJperh =
            (testSummary.standbyUsedEnergy_kJ - standardTankEnergy_kJ) /
            testSummary.standbyPeriodTime_h;

        double standbyAverageTankT_C = standbySumTimeTankT_minC / standbyPeriodTime_min;
        double standbyAverageAmbientT_C = standbySumTimeAmbientT_minC / standbyPeriodTime_min;

        double dT_C = standbyAverageTankT_C - standbyAverageAmbientT_C;
        if (dT_C > 0.)
        {
            testSummary.standbyLossCoefficient_kJperhC =
                testSummary.standbyHourlyLossEnergy_kJperh / dT_C; // UA
        }
    }

    //
    testSummary.noDrawTotalTime_h = noDrawTotalTime_min / min_per_hr; // tau_stby,2
    if (noDrawTotalTime_min > 0)
    {
        testSummary.noDrawAverageAmbientT_C =
            noDrawSumTimeAmbientT_minC / noDrawTotalTime_min; // <Ta,stby,2>
    }

    // find the standard delivered daily energy
    double standardDeliveredEnergy_kJ =
        removedHeatCapacity_kJperC * (standardSetpointT_C - standardInletT_C);

    // find the "Daily Water Heating Energy Consumption (Q_d)" (6.3.5)
    testSummary.consumedHeatingEnergy_kJ = testSummary.usedEnergy_kJ;
    if (testSummary.recoveryEfficiency > 0.)
    {
        testSummary.consumedHeatingEnergy_kJ -= tankHeatCapacity_kJperC *
                                                (finalTankT_C - initialTankT_C) /
                                                testSummary.recoveryEfficiency;
    }

    // find the "Adjusted Daily Water Heating Energy Consumption (Q_da)" (6.3.6)
    testSummary.adjustedConsumedWaterHeatingEnergy_kJ =
        testSummary.consumedHeatingEnergy_kJ -
        (standardAmbientT_C - testSummary.noDrawAverageAmbientT_C) *
            testSummary.standbyLossCoefficient_kJperhC * testSummary.noDrawTotalTime_h;

    // find the "Energy Used to Heat Water (Q_HW)" (6.3.6)
    testSummary.waterHeatingEnergy_kJ = 0.;
    if (testSummary.recoveryEfficiency > 0.)
    {
        testSummary.waterHeatingEnergy_kJ = deliveredEnergy_kJ / testSummary.recoveryEfficiency;
    }

    // find the "Standard Energy Used to Heat Water (Q_HW,T)" (6.3.6)
    testSummary.standardWaterHeatingEnergy_kJ = 0.;
    if (testSummary.recoveryEfficiency > 0.)
    {
        double standardRemovedEnergy_kJ =
            removedHeatCapacity_kJperC * (standardSetpointT_C - standardInletT_C);
        testSummary.standardWaterHeatingEnergy_kJ =
            standardRemovedEnergy_kJ / testSummary.recoveryEfficiency;
    }

    // find the "Modified Daily Water Heating Energy Consumption (Q_dm = Q_da - Q_HWD) (p.
    // 40487) note: same as Q_HW,T
    double waterHeatingDifferenceEnergy_kJ =
        testSummary.standardWaterHeatingEnergy_kJ - testSummary.waterHeatingEnergy_kJ; // Q_HWD
    testSummary.modifiedConsumedWaterHeatingEnergy_kJ =
        testSummary.adjustedConsumedWaterHeatingEnergy_kJ + waterHeatingDifferenceEnergy_kJ;

    // find the "Uniform Energy Factor" (6.4.4)
    testSummary.UEF = 0.;
    if (testSummary.modifiedConsumedWaterHeatingEnergy_kJ > 0.)
    {
        testSummary.UEF =
            standardDeliveredEnergy_kJ / testSummary.modifiedConsumedWaterHeatingEnergy_kJ;
    }

    // find the "Annual Energy Consumption" (6.4.5)
    testSummary.annualConsumedEnergy_kJ = 0.;
    if (testSummary.UEF > 0.)
    {
        constexpr double days_per_year = 365.;
        const double nominalDifferenceT_C = F_TO_C(67.);
        testSummary.annualConsumedEnergy_kJ =
            days_per_year * removedHeatCapacity_kJperC * nominalDifferenceT_C / testSummary.UEF;
    }

    // find the "Annual Electrical Energy Consumption" (6.4.6)
    testSummary.annualConsumedElectricalEnergy_kJ = 0.;
    if (testSummary.usedEnergy_kJ > 0.)
    {
        testSummary.annualConsumedElectricalEnergy_kJ =
            (testSummary.usedElectricalEnergy_kJ / testSummary.usedEnergy_kJ) *
            testSummary.annualConsumedEnergy_kJ;
    }

    return true;
}

bool HPWH::measureMetrics(FirstHourRating& firstHourRating,
                          StandardTestOptions& standardTestOptions,
                          StandardTestSummary& standardTestSummary)
{
    if (standardTestOptions.saveOutput)
    {

        std::string sFullOutputFilename =
            standardTestOptions.sOutputDirectory + "/" + standardTestOptions.sOutputFilename;
        standardTestOptions.outputFile.open(sFullOutputFilename.c_str(), std::ifstream::out);
        if (!standardTestOptions.outputFile.is_open())
        {
            std::cout << "Could not open output file " << sFullOutputFilename << "\n";
            return false;
        }
        std::cout << "Output file: " << sFullOutputFilename << "\n";

        std::string strPreamble;
        std::string sHeader = "minutes,Ta,Tsetpoint,inletT,draw,";

        int csvOptions = HPWH::CSVOPT_NONE;
        WriteCSVHeading(standardTestOptions.outputFile,
                        sHeader.c_str(),
                        standardTestOptions.nTestTCouples,
                        csvOptions);
    }

    if (!findFirstHourRating(firstHourRating, standardTestOptions))
    {
        std::cout << "Unable to complete first-hour rating test.\n";
        if (!customTestOptions.overrideFirstHourRating)
        {
            return false;
        }
    }

    if (customTestOptions.overrideFirstHourRating)
    {
        firstHourRating.desig = customTestOptions.desig;
        const std::string sFirstHourRatingDesig =
            HPWH::FirstHourRating::sDesigMap[firstHourRating.desig];
        std::cout << "\t\tUser-Specified Designation: " << sFirstHourRatingDesig << "\n";
    }

    if (run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
    {

        std::cout << "\t24-Hour Test Results:\n";
        if (!standardTestSummary.qualifies)
        {
            std::cout << "\t\tDoes not qualify as consumer water heater.\n";
        }

        std::cout << "\t\tRecovery Efficiency: " << standardTestSummary.recoveryEfficiency << "\n";

        std::cout << "\t\tStandby Loss Coefficient (kJ/h degC): "
                  << standardTestSummary.standbyLossCoefficient_kJperhC << "\n";

        std::cout << "\t\tUEF: " << standardTestSummary.UEF << "\n";

        std::cout << "\t\tAverage Inlet Temperature (degC): " << standardTestSummary.avgInletT_C
                  << "\n";

        std::cout << "\t\tAverage Outlet Temperature (degC): " << standardTestSummary.avgOutletT_C
                  << "\n";

        std::cout << "\t\tTotal Volume Drawn (L): " << standardTestSummary.removedVolume_L << "\n";

        std::cout << "\t\tDaily Water-Heating Energy Consumption (kWh): "
                  << KJ_TO_KWH(standardTestSummary.waterHeatingEnergy_kJ) << "\n";

        std::cout << "\t\tAdjusted Daily Water-Heating Energy Consumption (kWh): "
                  << KJ_TO_KWH(standardTestSummary.adjustedConsumedWaterHeatingEnergy_kJ) << "\n";

        std::cout << "\t\tModified Daily Water-Heating Energy Consumption (kWh): "
                  << KJ_TO_KWH(standardTestSummary.modifiedConsumedWaterHeatingEnergy_kJ) << "\n";

        std::cout << "\tAnnual Values:\n";
        std::cout << "\t\tAnnual Electrical Energy Consumption (kWh): "
                  << KJ_TO_KWH(standardTestSummary.annualConsumedElectricalEnergy_kJ) << "\n";

        std::cout << "\t\tAnnual Energy Consumption (kWh): "
                  << KJ_TO_KWH(standardTestSummary.annualConsumedEnergy_kJ) << "\n";
    }
    else
    {
        std::cout << "Unable to complete 24-hr test.\n";
        return false;
    }

    if (standardTestOptions.saveOutput)
    {
        standardTestOptions.outputFile.close();
    }
    return true;
}

bool HPWH::makeGeneric(const double targetUEF)
{
    static HPWH::FirstHourRating firstHourRating;
    static HPWH::StandardTestOptions standardTestOptions;

    struct Inverter
    {
        static bool getLeftDampedInv(const double nu,
                                     const std::vector<double>& matV, // 1 x 2
                                     std::vector<double>& invMatV     // 2 x 1
        )
        {
            constexpr double thresh = 1.e-12;

            if (matV.size() != 2)
            {
                return false;
            }

            double a = matV[0];
            double b = matV[1];

            double A = (1. + nu) * a * a;
            double B = (1. + nu) * b * b;
            double C = a * b;
            double det = A * B - C * C;

            if (abs(det) < thresh)
            {
                return false;
            }

            invMatV.resize(2);
            invMatV[0] = (a * B - b * C) / det;
            invMatV[1] = (-a * C + b * A) / det;
            return true;
        }
    };

    struct ParamInfo
    {
        enum class Type
        {
            none,
            copCoef
        };

        virtual Type paramType() = 0;

        virtual bool assign(HPWH& hpwh, double*& val) = 0;
        virtual void showInfo(std::ostream& os) = 0;
    };

    struct CopCoefInfo : public ParamInfo
    {
        unsigned heatSourceIndex;
        unsigned tempIndex;
        unsigned power;

        Type paramType() override { return Type::copCoef; }

        CopCoefInfo(const unsigned heatSourceIndex_in,
                    const unsigned tempIndex_in,
                    const unsigned power_in)
            : ParamInfo()
            , heatSourceIndex(heatSourceIndex_in)
            , tempIndex(tempIndex_in)
            , power(power_in)
        {
        }

        bool assign(HPWH& hpwh, double*& val) override
        {
            val = nullptr;
            HPWH::HeatSource* heatSource;
            if (!hpwh.getNthHeatSource(heatSourceIndex, heatSource))
            {
                std::cout << "Invalid heat source index.\n";
                return false;
            }

            auto& perfMap = heatSource->perfMap;
            if (tempIndex >= perfMap.size())
            {
                std::cout << "Invalid heat-source performance-map temperature index.\n";
                return false;
            }

            auto& perfPoint = perfMap[tempIndex];
            auto& copCoeffs = perfPoint.COP_coeffs;
            if (power >= copCoeffs.size())
            {
                std::cout << "Invalid heat-source performance-map cop-coefficient power.\n";
                return false;
            }
            std::cout << "Valid parameter:";
            showInfo(std::cout);
            std::cout << "\n";
            val = &copCoeffs[power];
            return true;
        };

        void showInfo(std::ostream& os) override
        {
            os << " heat-source index = " << heatSourceIndex;
            os << ", temperature index = " << tempIndex;
            os << ", power = " << power;
        }
    };

    struct Param
    {
        double* val;
        double dVal;

        Param() : val(nullptr), dVal(1.e3) {}

        virtual bool assignVal(HPWH& hpwh) = 0;
        virtual void show(std::ostream& os) = 0;
    };

    struct CopCoef : public Param,
                     CopCoefInfo
    {
        CopCoef(CopCoefInfo& copCoefInfo) : Param(), CopCoefInfo(copCoefInfo) { dVal = 1.e-9; }

        bool assignVal(HPWH& hpwh) override { return assign(hpwh, val); }

        void show(std::ostream& os) override
        {
            showInfo(os);
            os << ": " << *val << "\n";
        }
    };

    struct Merit
    {
        double val;
        double targetVal;
        double tolVal;

        Merit() : val(0.), targetVal(0.), tolVal(1.e-6) {}

        virtual bool eval(HPWH& hpwh) = 0;
        virtual bool evalDiff(HPWH& hpwh, double& diff) = 0;
    };

    struct UEF_Merit : public Merit
    {
        UEF_Merit(double targetVal_in) : Merit() { targetVal = targetVal_in; }

        bool eval(HPWH& hpwh) override
        {
            static HPWH::StandardTestSummary standardTestSummary;
            if (!hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
            {
                std::cout << "Unable to complete 24-hr test.\n";
                return false;
            }
            val = standardTestSummary.UEF;
            return true;
        }

        bool evalDiff(HPWH& hpwh, double& diff) override
        {
            if (eval(hpwh))
            {
                diff = (val - targetVal) / tolVal;
                return true;
            };
            return false;
        }
    };

    standardTestOptions.saveOutput = false;
    standardTestOptions.changeSetpoint = true;
    standardTestOptions.nTestTCouples = 6;
    standardTestOptions.setpointT_C = 51.7;

    if (!findFirstHourRating(firstHourRating, standardTestOptions))
    {
        std::cout << "Unable to complete first-hour rating test.\n";
        if (!customTestOptions.overrideFirstHourRating)
        {
            return false;
        }
    }

    if (customTestOptions.overrideFirstHourRating)
    {
        firstHourRating.desig = customTestOptions.desig;
        const std::string sFirstHourRatingDesig =
            HPWH::FirstHourRating::sDesigMap[firstHourRating.desig];
        std::cout << "\t\tUser-Specified Designation: " << sFirstHourRatingDesig << "\n";
    }

    // set up merit parameter
    Merit* pMerit;
    UEF_Merit uefMerit(targetUEF);
    pMerit = &uefMerit;

    // set up parameters
    std::vector<Param*> pParams;

    CopCoefInfo copT2constInfo = {2, 1, 0}; // heatSourceIndex, tempIndex, power
    CopCoefInfo copT2linInfo = {2, 1, 1};   // heatSourceIndex, tempIndex, power

    CopCoef copT2const(copT2constInfo);
    CopCoef copT2lin(copT2linInfo);

    pParams.push_back(&copT2const);
    pParams.push_back(&copT2lin);

    std::vector<double> dParams;

    //
    bool foundParams = true;
    for (auto& pParam : pParams)
    {
        foundParams &= pParam->assignVal(*this);
        dParams.push_back(pParam->dVal);
    }
    if (!foundParams)
    {
        return false;
    }
    auto nParams = pParams.size();

    double nu = 0.1;
    const int maxIters = 20;
    for (auto iter = 0; iter < maxIters; ++iter)
    {
        if (!pMerit->eval(*this))
        {
            return false;
        }
        std::cout << iter << ": ";
        std::cout << "UEF: " << pMerit->val << "; ";

        bool first = true;
        for (std::size_t j = 0; j < nParams; ++j)
        {
            if (!first)
                std::cout << ",";

            std::cout << " " << j << ": ";
            std::cout << *(pParams[j]->val);
            first = false;
        }

        double dMerit0 = 0.;
        if (!(pMerit->evalDiff(*this, dMerit0)))
        {
            return false;
        }
        double FOM0 = dMerit0 * dMerit0;
        double FOM1 = 0., FOM2 = 0.;
        std::cout << ", FOM: " << FOM0 << "\n";

        std::vector<double> paramV(nParams);
        for (std::size_t i = 0; i < nParams; ++i)
        {
            paramV[i] = *(pParams[i]->val);
        }

        std::vector<double> jacobiV(nParams); // 1 x 2
        for (std::size_t j = 0; j < nParams; ++j)
        {
            *(pParams[j]->val) = paramV[j] + (pParams[j]->dVal);
            double dMerit;
            if (!(pMerit->evalDiff(*this, dMerit)))
            {
                return false;
            }
            jacobiV[j] = (dMerit - dMerit0) / (pParams[j]->dVal);
            *(pParams[j]->val) = paramV[j];
        }

        // try nu
        std::vector<double> invJacobiV1;
        bool got1 = Inverter::getLeftDampedInv(nu, jacobiV, invJacobiV1);

        std::vector<double> inc1ParamV(2);
        std::vector<double> paramV1 = paramV;
        if (got1)
        {
            for (std::size_t j = 0; j < nParams; ++j)
            {
                inc1ParamV[j] = -invJacobiV1[j] * dMerit0;
                paramV1[j] += inc1ParamV[j];
                *(pParams[j]->val) = paramV1[j];
            }
            double dMerit;
            if (!pMerit->evalDiff(*this, dMerit))
            {
                return false;
            }
            FOM1 = dMerit * dMerit;

            // restore
            for (std::size_t j = 0; j < nParams; ++j)
            {
                *(pParams[j]->val) = paramV[j];
            }
        }

        // try nu / 2
        std::vector<double> invJacobiV2;
        bool got2 = Inverter::getLeftDampedInv(nu / 2., jacobiV, invJacobiV2);

        std::vector<double> inc2ParamV(2);
        std::vector<double> paramV2 = paramV;
        if (got2)
        {
            for (std::size_t j = 0; j < nParams; ++j)
            {
                inc2ParamV[j] = -invJacobiV2[j] * dMerit0;
                paramV2[j] += inc2ParamV[j];
                *(pParams[j]->val) = paramV2[j];
            }
            double dMerit;
            if (!pMerit->evalDiff(*this, dMerit))
            {
                return false;
            }
            FOM2 = dMerit * dMerit;

            // restore
            for (std::size_t j = 0; j < nParams; ++j)
            {
                *(pParams[j]->val) = paramV[j];
            }
        }

        // check for improvement
        if (got1 && got2)
        {
            if ((FOM1 < FOM0) || (FOM2 < FOM0))
            { // at least one improved
                if (FOM1 < FOM2)
                { // pick 1
                    for (std::size_t i = 0; i < nParams; ++i)
                    {
                        *(pParams[i]->val) = paramV1[i];
                        (pParams[i]->dVal) = inc1ParamV[i] / 1.e3;
                        FOM0 = FOM1;
                    }
                }
                else
                { // pick 2
                    for (std::size_t i = 0; i < nParams; ++i)
                    {
                        *(pParams[i]->val) = paramV2[i];
                        (pParams[i]->dVal) = inc2ParamV[i] / 1.e3;
                        FOM0 = FOM2;
                    }
                }
            }
            else
            { // no improvement
                nu *= 10.;
                if (nu > 1.e6)
                {
                    return false;
                }
            }
        }
    }
    return false;
}
