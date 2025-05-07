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

#include <fstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <queue>

#include <fmt/format.h>

#include <btwxt/btwxt.h>

#include "HPWH.hh"
#include "HPWHUtils.hh"
#include "HPWHFitter.hh"
#include "HPWHHeatingLogic.hh"
#include "HPWHHeatSource.hh"
#include "Tank.hh"
#include "Condenser.hh"
#include "Resistance.hh"

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

// see EERE-2019-BT-TP-0032-0058 (p. 40433, 40435, 40476)
HPWH::TestConfiguration HPWH::testConfiguration_E50 = {F_TO_C(50.), F_TO_C(50.), F_TO_C(50.)};
HPWH::TestConfiguration HPWH::testConfiguration_UEF = {F_TO_C(67.5), F_TO_C(58.), F_TO_C(67.5)};
HPWH::TestConfiguration HPWH::testConfiguration_E95 = {F_TO_C(95.), F_TO_C(67.), F_TO_C(95.)};

// stipulated setpoint for 24-hr test, see EERE-2019-BT-TP-0032-0058 (p. 40475)
double HPWH::testSetpointT_C = F_TO_C(125.);

std::unordered_map<HPWH::FirstHourRating::Designation, std::size_t> HPWH::firstDrawClusterSizes = {
    {HPWH::FirstHourRating::Designation::VerySmall, 5},
    {HPWH::FirstHourRating::Designation::Low, 3},
    {HPWH::FirstHourRating::Designation::Medium, 3},
    {HPWH::FirstHourRating::Designation::High, 4}};

std::unordered_map<HPWH::FirstHourRating::Designation, HPWH::DrawPattern> HPWH::drawPatterns = {
    {HPWH::FirstHourRating::Designation::VerySmall,
     {{HM_TO_MIN(0, 00), 7.6, 3.8},
      {HM_TO_MIN(1, 00), 3.8, 3.8},
      {HM_TO_MIN(1, 05), 1.9, 3.8},
      {HM_TO_MIN(1, 10), 1.9, 3.8},
      {HM_TO_MIN(1, 15), 1.9, 3.8},
      {HM_TO_MIN(8, 00), 3.8, 3.8},
      {HM_TO_MIN(8, 15), 7.6, 3.8},
      {HM_TO_MIN(9, 00), 5.7, 3.8},
      {HM_TO_MIN(9, 15), 3.8, 3.8}}},

    {HPWH::FirstHourRating::Designation::Low,
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

    {HPWH::FirstHourRating::Designation::Medium,
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

    {HPWH::FirstHourRating::Designation::High,
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

void HPWH::setMinutesPerStep(const double minutesPerStep_in)
{
    minutesPerStep = minutesPerStep_in;
    secondsPerStep = sec_per_min * minutesPerStep;
    hoursPerStep = minutesPerStep / min_per_hr;
}

// public HPWH functions
HPWH::HPWH(const std::shared_ptr<Courier::Courier>& courier, const std::string& name_in /*"hpwh"*/)
    : Sender("HPWH", name_in, courier)
{
    tank = std::make_shared<Tank>(this, courier);
    setAllDefaults();
}

HPWH::HPWH(const HPWH& hpwh) : Sender(hpwh) { *this = hpwh; }

void HPWH::setAllDefaults()
{
    tank->setAllDefaults();
    heatSources.clear();

    canScale = false;
    member_inletT_C = -1.; // invalid unit setInletT called
    haveInletT = false;

    isHeating = false;
    setpointFixed = false;

    member_inletT_C = -1.; // invalid unit setInletT called
    currentSoCFraction = 1.;
    doTempDepression = false;
    locationTemperature_C = UNINITIALIZED_LOCATIONTEMP;

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
        heatSource->hpwh = this;
    }

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

    // description = hpwh.description;
    productInformation = hpwh.productInformation;

    return *this;
}

HPWH::~HPWH() {}

HPWH::Condenser* HPWH::addCondenser(const std::string& name_in)
{
    heatSources.emplace_back(std::make_shared<Condenser>(this, get_courier(), name_in));
    return reinterpret_cast<Condenser*>(heatSources.back().get());
}

HPWH::Resistance* HPWH::addResistance(const std::string& name_in)
{
    heatSources.emplace_back(std::make_shared<Resistance>(this, get_courier(), name_in));
    return reinterpret_cast<Resistance*>(heatSources.back().get());
}

void HPWH::runOneStep(double drawVolume_L,
                      double tankAmbientT_C,
                      double heatSourceAmbientT_C,
                      DRMODES DRstatus,
                      double inletVol2_L,
                      double inletT2_C,
                      std::vector<double>* extraHeatDist_W)
{

    // check for errors
    if (doTempDepression && (minutesPerStep != 1))
    {
        send_error("minutesPerStep must equal one for temperature depression to work.");
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
        heatSources[i]->runtime_min = 0;
        heatSources[i]->energyInput_kWh = 0.;
        heatSources[i]->energyOutput_kWh = 0.;
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
                heatSources[compressorIndex]->engageHeatSource(DRstatus);
            }
            if (lowestElementIndex >= 0)
            {
                heatSources[lowestElementIndex]->engageHeatSource(DRstatus);
            }
        }

        // do HeatSource choice
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (isHeating)
            {
                // check if anything that is on needs to turn off (generally for lowT cutoffs)
                // things that just turn on later this step are checked for this in shouldHeat
                if (heatSources[i]->isEngaged() && heatSources[i]->shutsOff())
                {
                    heatSources[i]->disengageHeatSource();
                    // check if the backup heat source would have to shut off too
                    if ((heatSources[i]->backupHeatSource != NULL) &&
                        !heatSources[i]->backupHeatSource->shutsOff())
                    {
                        // and if not, go ahead and turn it on
                        heatSources[i]->backupHeatSource->engageHeatSource(DRstatus);
                    }
                }

                // if there's a priority HeatSource (e.g. upper resistor) and it needs to
                // come on, then turn  off and start it up
                if (heatSources[i]->isVIP)
                {
                    if (heatSources[i]->shouldHeat())
                    {
                        if (shouldDRLockOut(heatSources[i]->typeOfHeatSource(), DRstatus))
                        {
                            if (hasACompressor())
                            {
                                heatSources[compressorIndex]->engageHeatSource(DRstatus);
                                break;
                            }
                        }
                        else
                        {
                            turnAllHeatSourcesOff();
                            heatSources[i]->engageHeatSource(DRstatus);
                            // stop looking if the VIP needs to run
                            break;
                        }
                    }
                }
            }
            // if nothing is currently on, then check if something should come on
            else /* (isHeating == false) */
            {
                if (heatSources[i]->shouldHeat())
                {
                    heatSources[i]->engageHeatSource(DRstatus);
                    // engaging a heat source sets isHeating to true, so this will only trigger once
                }
            }

        } // end loop over heat sources

        // do heating logic
        double minutesToRun = minutesPerStep;
        for (int i = 0; i < getNumHeatSources(); i++)
        {

            // check/apply lock-outs
            if (shouldDRLockOut(heatSources[i]->typeOfHeatSource(), DRstatus))
            {
                heatSources[i]->lockOutHeatSource();
            }
            else
            {
                // locks or unlocks the heat source
                if (heatSources[i]->typeOfHeatSource() == TYPE_compressor)
                {
                    auto condenser = reinterpret_cast<Condenser*>(heatSources[i].get());
                    condenser->toLockOrUnlock(heatSourceAmbientT_C);
                }
                else if (heatSources[i]->typeOfHeatSource() == TYPE_resistance)
                {
                    auto resistance = reinterpret_cast<Resistance*>(heatSources[i].get());
                    resistance->unlockHeatSource();
                }
            }
            if (heatSources[i]->isLockedOut() && heatSources[i]->backupHeatSource == NULL)
            {
                heatSources[i]->disengageHeatSource();
            }

            // going through in order, check if the heat source is on
            if (heatSources[i]->isEngaged())
            {

                HeatSource* heatSourcePtr;
                if (heatSources[i]->isLockedOut() && heatSources[i]->backupHeatSource != NULL)
                {

                    // Check that the backup isn't locked out too or already engaged then it will
                    // heat on its own.
                    bool shouldLockOut =
                        heatSources[i]->backupHeatSource->isEngaged() ||
                        shouldDRLockOut(heatSources[i]->backupHeatSource->typeOfHeatSource(),
                                        DRstatus);
                    if (heatSources[i]->backupHeatSource->typeOfHeatSource() == TYPE_compressor)
                    {
                        auto condenser = reinterpret_cast<Condenser*>(heatSources[i].get());
                        shouldLockOut |= condenser->toLockOrUnlock(heatSourceAmbientT_C);
                    }
                    else if (heatSources[i]->typeOfHeatSource() == TYPE_resistance)
                    {
                        auto resistance = reinterpret_cast<Resistance*>(heatSources[i].get());
                        shouldLockOut |= resistance->toLockOrUnlock();
                    }
                    if (shouldLockOut)
                    {
                        continue;
                    }
                    // Don't turn the backup electric resistance heat source on if the VIP
                    // resistance element is on .
                    else if (VIPIndex >= 0 && heatSources[VIPIndex]->isOn &&
                             heatSources[i]->backupHeatSource->isAResistance())
                    {
                        continue;
                    }
                    else
                    {
                        heatSourcePtr = heatSources[i]->backupHeatSource;
                    }
                }
                else
                {
                    heatSourcePtr = heatSources[i].get();
                }

                addHeatParent(heatSourcePtr, heatSourceAmbientT_C, minutesToRun);

                // if it finished early. i.e. shuts off early like if the heatsource met setpoint or
                // maxed out
                if (heatSourcePtr->runtime_min < minutesToRun)
                {
                    // subtract time it ran and turn it off
                    minutesToRun -= heatSourcePtr->runtime_min;
                    heatSources[i]->disengageHeatSource();
                    // and if there's a heat source that follows this heat source (regardless of
                    // lockout) that's able to come on,
                    if ((heatSources[i]->followedByHeatSource != NULL) &&
                        !heatSources[i]->followedByHeatSource->shutsOff())
                    {
                        // turn it on
                        heatSources[i]->followedByHeatSource->engageHeatSource(DRstatus);
                    }
                    // or if there heat source can't produce hotter water (i.e. it's maxed out) and
                    // the tank still isn't at setpoint. the compressor should get locked out when
                    // the maxedOut is true but have to run the resistance first during this
                    // timestep to make sure tank is above the max temperature for the compressor.
                    else if (heatSources[i]->maxedOut() && heatSources[i]->backupHeatSource != NULL)
                    {
                        auto backupHeatSource = heatSources[i]->backupHeatSource;
                        // Check that the backup isn't locked out or already engaged then it will
                        // heat or already heated on its own.
                        bool isBackupAvailable =
                            !backupHeatSource->isEngaged() &&
                            !shouldDRLockOut(backupHeatSource->typeOfHeatSource(), DRstatus);
                        if (backupHeatSource->typeOfHeatSource() == TYPE_compressor)
                        {
                            auto condenser = reinterpret_cast<Condenser*>(backupHeatSource);
                            isBackupAvailable &= !condenser->toLockOrUnlock(heatSourceAmbientT_C);
                        }
                        else if (heatSources[i]->typeOfHeatSource() == TYPE_resistance)
                        {
                            auto resistance = reinterpret_cast<Resistance*>(heatSources[i].get());
                            isBackupAvailable &= !resistance->toLockOrUnlock();
                        }
                        if (isBackupAvailable)
                        {
                            // turn it on
                            backupHeatSource->engageHeatSource(DRstatus);
                            // add heat if it hasn't heated up this whole minute already
                            if (backupHeatSource->runtime_min >= 0.)
                            {
                                addHeatParent(backupHeatSource,
                                              heatSourceAmbientT_C,
                                              minutesToRun - backupHeatSource->runtime_min);
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
            if (heatSources[i]->isEngaged() && !heatSources[i]->isLockedOut() &&
                heatSources[i]->depressesTemperature)
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
            (heatSources[i]->energyOutput_kWh - heatSources[i]->energyInput_kWh);
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
} // end runOneStep

void HPWH::runNSteps(int N,
                     double* inletT_C,
                     double* drawVolume_L,
                     double* tankAmbientT_C,
                     double* heatSourceAmbientT_C,
                     DRMODES* DRstatus)
{
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
        heatSources[i]->runtime_min = heatSources_runTimes_SUM[i];
        heatSources[i]->energyInput_kWh = heatSources_energyInputs_SUM[i];
        heatSources[i]->energyOutput_kWh = heatSources_energyOutputs_SUM[i];
    }
}

void HPWH::addHeatParent(HeatSource* heatSourcePtr,
                         double heatSourceAmbientT_C,
                         double minutesToRun)
{
    if (heatSourcePtr->isACompressor())
    {
        auto cond_ptr = reinterpret_cast<Condenser*>(heatSourcePtr);

        // Check the air temprature and setpoint against maxOut_at_LowT
        double tempSetpoint_C = -273.15;
        if (heatSourceAmbientT_C <= cond_ptr->maxOut_at_LowT.airT_C &&
            setpoint_C >= cond_ptr->maxOut_at_LowT.outT_C)
        {
            tempSetpoint_C = setpoint_C; // Store setpoint
            setSetpoint(cond_ptr->maxOut_at_LowT
                            .outT_C); // Reset to new setpoint as this is used in the add heat calc
        }

        cond_ptr->addHeat(heatSourceAmbientT_C, minutesToRun);

        // Change the setpoint back to what it was pre-compressor depression
        if (tempSetpoint_C > -273.15)
        {
            setSetpoint(tempSetpoint_C);
        }
    }
    else
    {
        auto res_ptr = reinterpret_cast<Resistance*>(heatSourcePtr);
        res_ptr->addHeat(minutesToRun);
    }
    // and add heat if it is
}

// public members to write to CSV file
int HPWH::writeCSVHeading(std::ofstream& outFILE,
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

int HPWH::writeCSVRow(std::ofstream& outFILE,
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

bool HPWH::isSetpointFixed() const { return setpointFixed; }

void HPWH::setSetpoint(double newSetpoint, UNITS units /*=UNITS_C*/)
{
    double newSetpoint_C = newSetpoint;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        newSetpoint_C = F_TO_C(newSetpoint);
        break;
    default:
        send_error("Invalid units.");
    }

    double maxAllowedSetpointT_C;
    string why;
    if (isNewSetpointPossible(newSetpoint_C, maxAllowedSetpointT_C, why))
    {
        setpoint_C = newSetpoint_C;
    }
    else
    {
        send_error(fmt::format("Cannot set this setpoint for the currently selected model, "
                               "max setpoint is {:0.2f} C. {}",
                               maxAllowedSetpointT_C,
                               why.c_str()));
    }
}

double HPWH::getSetpoint(UNITS units /*=UNITS_C*/) const
{
    double result = setpoint_C;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        result = C_TO_F(result);
        break;
    default:
        send_error("Invalid units.");
    }
    return result;
}

double HPWH::getMaxCompressorSetpoint(UNITS units /*=UNITS_C*/) const
{

    if (!hasACompressor())
    {
        send_error("Unit does not have a compressor.");
    }

    auto condenser = reinterpret_cast<Condenser*>(heatSources[compressorIndex].get());
    double returnVal = condenser->getMaxSetpointT_C();
    if (units == UNITS_C)
    {
    }
    else if (units == UNITS_F)
    {
        returnVal = C_TO_F(returnVal);
    }
    else
        send_error("Invalid units.");

    return returnVal;
}

bool HPWH::isNewSetpointPossible(double newSetpoint,
                                 double& maxAllowedSetpoint,
                                 string& why,
                                 UNITS units /*=UNITS_C*/) const
{
    double newSetpoint_C = newSetpoint;
    double maxAllowedSetpoint_C = -273.15;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        newSetpoint_C = F_TO_C(newSetpoint);
        break;
    default:
        send_error("Invalid units.");
    }

    bool returnVal = false;

    constexpr double tolT_C = 1.e-9;
    if (isSetpointFixed())
    {
        returnVal = (newSetpoint_C == setpoint_C);
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

            auto cond_ptr = reinterpret_cast<Condenser*>(heatSources[compressorIndex].get());

            maxAllowedSetpoint_C = cond_ptr->maxSetpoint_C -
                                   cond_ptr->secondaryHeatExchanger.hotSideTemperatureOffset_dC;

            if ((newSetpoint_C > maxAllowedSetpoint_C + tolT_C) && lowestElementIndex == -1)
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

            maxAllowedSetpoint_C = 100.;
            if (newSetpoint_C > maxAllowedSetpoint_C + tolT_C)
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

    maxAllowedSetpoint = maxAllowedSetpoint_C;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        maxAllowedSetpoint = C_TO_F(maxAllowedSetpoint_C);
        break;
    default:
        send_error("Invalid units.");
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
            heatSources[compressorIndex]->turnOnLogicSet[0]);
    newSoCFraction = calcSoCFraction(logicSoC->getMainsT_C(), logicSoC->getTempMinUseful_C());

    currentSoCFraction = newSoCFraction;
}

double HPWH::getMinOperatingTemp(UNITS units /*=UNITS_C*/) const
{
    if (!hasACompressor())
    {
        send_error("No compressor found in this HPWH.");
    }

    double result = heatSources[compressorIndex]->minT;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        result = C_TO_F(result);
        break;
    default:
        send_error("Invalid units.");
    }

    return result;
}

void HPWH::resetTankToSetpoint() { tank->setNodeT_C(setpoint_C); }

//-----------------------------------------------------------------------------
///	@brief	Assigns new temps provided from a std::vector to tankTemps_C.
/// @param[in]	setTankTemps	new tank temps (arbitrary non-zero size)
///	@param[in]	units          temp units in setTankTemps (default = UNITS_C)
//-----------------------------------------------------------------------------
void HPWH::setTankLayerTemperatures(std::vector<double> setTankTemps, const UNITS units)
{

    // convert setTankTemps to C, if necessary
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        for (auto& T : setTankTemps)
            T = F_TO_C(T);
        break;
    default:
        send_error("Invalid units.");
    }

    tank->setNodeTs_C(setTankTemps);
}

void HPWH::getTankTemps(std::vector<double>& tankTemps) { tank->getNodeTs_C(tankTemps); }

void HPWH::setAirFlowFreedom(double fanFraction)
{
    if (fanFraction < 0 || fanFraction > 1)
    {
        send_error("You have attempted to set the fan fraction outside of bounds.");
    }
    else
    {
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i]->isACompressor())
            {
                heatSources[i]->airflowFreedom = fanFraction;
            }
        }
    }
}

void HPWH::setDoTempDepression(bool doTempDepress) { doTempDepression = doTempDepress; }

void HPWH::setTankSize_adjustUA(double volume,
                                UNITS units /*=UNITS_L*/,
                                bool forceChange /*=false*/)
{
    switch (units)
    {
    case UNITS_L:
        break;
    case UNITS_GAL:
        volume = GAL_TO_L(volume);
        break;
    default:
        send_error("Invalid units.");
    }
    tank->setVolumeAndAdjustUA(volume, forceChange);
}

/*static*/ double HPWH::getTankSurfaceArea(double volume_L,
                                           UNITS volUnits /*=UNITS_L*/,
                                           UNITS surfAUnits /*=UNITS_FT2*/)
{
    switch (volUnits)
    {
    case UNITS_L:
        break;
    case UNITS_GAL:
        volume_L = GAL_TO_L(volume_L);
        break;
    default:;
    }

    double SA = Tank::getSurfaceArea_m2(volume_L);
    switch (surfAUnits)
    {
    case UNITS_M2:
        break;
    case UNITS_FT2:
        SA = M2_TO_FT2(SA);
        break;
    default:;
    }
    return SA;
}

double HPWH::getTankSurfaceArea(UNITS units /*=UNITS_FT2*/) const
{
    double value = Tank::getSurfaceArea_m2(tank->volume_L);
    switch (units)
    {
    case UNITS_M2:
        break;
    case UNITS_FT2:
        value = M2_TO_FT2(value);
        break;
    default:
        send_error("Invalid units.");
    }
    return value;
}

/*static*/ double
HPWH::getTankRadius(double vol, UNITS volUnits /*=UNITS_L*/, UNITS radiusUnits /*=UNITS_FT*/)
{
    switch (volUnits)
    {
    case UNITS_L:
        break;
    case UNITS_GAL:
        vol = GAL_TO_L(vol);
        break;
    default:;
    }

    double radius = Tank::getRadius_m(vol);
    switch (radiusUnits)
    {
    case UNITS_M:
        break;
    case UNITS_FT:
        radius = FT_TO_M(radius);
        break;
    default:;
    }
    return radius;
}

double HPWH::getTankRadius(UNITS units /*=UNITS_FT*/) const
{
    return getTankRadius(tank->getVolume_L(), UNITS_L, units);
}

bool HPWH::isTankSizeFixed() const { return tank->volumeFixed; }

void HPWH::setTankSize(double volume, UNITS units /*=UNITS_L*/, bool forceChange /*=false*/)
{
    switch (units)
    {
    case UNITS_L:
        break;
    case UNITS_GAL:
        volume = GAL_TO_L(volume);
        break;
    default:
        send_error("Invalid units.");
    }
    tank->setVolume_L(volume, forceChange);
    tank->calcSizeConstants();
}

void HPWH::setDoInversionMixing(bool doInversionMixing_in)
{
    tank->setDoInversionMixing(doInversionMixing_in);
}

void HPWH::setDoConduction(bool doConduction_in) { tank->setDoConduction(doConduction_in); }

void HPWH::setUA(double UA, UNITS units /*=UNITS_kJperHrC*/)
{
    switch (units)
    {
    case UNITS_kJperHrC:
        break;
    case UNITS_BTUperHrF:
        UA = UAf_TO_UAc(UA);
        break;
    default:
        send_error("Invalid units.");
    }
    tank->setUA_kJperHrC(UA);
}

void HPWH::getUA(double& UA, UNITS units /*=UNITS_kJperHrC*/) const
{
    UA = tank->getUA_kJperHrC();
    switch (units)
    {
    case UNITS_kJperHrC:;
        break;
    case UNITS_BTUperHrF:
        UA = UA / UAf_TO_UAc(1.);
        break;
    default:
        send_error("Invalid units.");
    }
}

void HPWH::setFittingsUA(double fittingsUA, UNITS units /*=UNITS_kJperHrC*/)
{
    switch (units)
    {
    case UNITS_kJperHrC:
        break;
    case UNITS_BTUperHrF:
        fittingsUA = UAf_TO_UAc(fittingsUA);
        break;
    default:
        send_error("Invalid units.");
    }
    tank->setFittingsUA_kJperHrC(fittingsUA);
}

void HPWH::getFittingsUA(double& fittingsUA, UNITS units /*=UNITS_kJperHrC*/) const
{
    fittingsUA = tank->getFittingsUA_kJperHrC();
    switch (units)
    {
    case UNITS_kJperHrC:
        break;
    case UNITS_BTUperHrF:
        fittingsUA = fittingsUA / UAf_TO_UAc(1.);
        break;
    default:
        send_error("Invalid units.");
    }
}

void HPWH::setExternalInletHeightByFraction(double fractionalHeight)
{
    setExternalPortHeightByFraction(fractionalHeight, 1);
}

void HPWH::setExternalOutletHeightByFraction(double fractionalHeight)
{
    setExternalPortHeightByFraction(fractionalHeight, 2);
}

void HPWH::setExternalPortHeightByFraction(double fractionalHeight, int whichExternalPort)
{
    std::size_t heatSourceIndex;
    if (!hasExternalHeatSource(heatSourceIndex))
    {
        send_error("Does not have an external heat source.");
    }

    if (whichExternalPort == 1)
    {
        setNodeNumFromFractionalHeight(fractionalHeight,
                                       heatSources[heatSourceIndex]->externalInletHeight);
    }
    else
    {
        setNodeNumFromFractionalHeight(fractionalHeight,
                                       heatSources[heatSourceIndex]->externalOutletHeight);
    }
}

void HPWH::setNodeNumFromFractionalHeight(double fractionalHeight, int& inletNum)
{
    if (fractionalHeight > 1. || fractionalHeight < 0.)
    {
        send_error("Out of bounds fraction for setInletByFraction.");
    }

    int node = (int)std::floor(getNumNodes() * fractionalHeight);
    inletNum = (node == getNumNodes()) ? getIndexTopNode() : node;
}

int HPWH::getExternalInletHeight() const
{
    std::size_t heatSourceIndex;
    if (!hasExternalHeatSource(heatSourceIndex))
    {
        send_error("Does not have an external heat source.");
    }
    return heatSources[heatSourceIndex]
        ->externalInletHeight; // Return the first one since all external
    // sources have some ports
}

int HPWH::getExternalOutletHeight() const
{
    std::size_t heatSourceIndex;
    if (!hasExternalHeatSource(heatSourceIndex))
    {
        send_error("Does not have an external heat source.");
    }
    return heatSources[heatSourceIndex]->externalOutletHeight; // Return the first one since all
                                                               // external sources have some ports
}

void HPWH::setTimerLimitTOT(double limit_min)
{
    if (limit_min > 24. * 60. || limit_min < 0.)
    {
        send_error("Out of bounds time limit for setTimerLimitTOT.");
    }
    timerLimitTOT = limit_min;
}

double HPWH::getTimerLimitTOT_minute() const { return timerLimitTOT; }

int HPWH::getInletHeight(int whichInlet) const { return tank->getInletHeight(whichInlet); }

void HPWH::setMaxTempDepression(double maxDepression, UNITS units /*=UNITS_C*/)
{
    maxDepression_C = maxDepression;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        maxDepression_C = F_TO_C(maxDepression);
        break;
    default:
        send_error("Invalid units.");
    }
}

bool HPWH::hasEnteringWaterHighTempShutOff(int heatSourceIndex)
{
    bool retVal = false;
    if (heatSourceIndex >= getNumHeatSources() || heatSourceIndex < 0)
    {
        return retVal;
    }
    if (heatSources[heatSourceIndex]->shutOffLogicSet.size() == 0)
    {
        return retVal;
    }

    for (std::shared_ptr<HeatingLogic> shutOffLogic : heatSources[heatSourceIndex]->shutOffLogicSet)
    {
        if (shutOffLogic->getIsEnteringWaterHighTempShutoff())
        {
            retVal = true;
            break;
        }
    }
    return retVal;
}

void HPWH::setEnteringWaterHighTempShutOff(double highTemp,
                                           bool tempIsAbsolute,
                                           int heatSourceIndex,
                                           UNITS units /*=UNITS_C*/)
{
    if (!hasEnteringWaterHighTempShutOff(heatSourceIndex))
    {
        send_error("You have attempted to access a heating logic that does not exist.");
    }

    double highTemp_C = highTemp;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        highTemp_C = F_TO_C(highTemp);
        break;
    default:
        send_error("Invalid units.");
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
        send_error(fmt::format("High temperature shut off is too close to the setpoint, expected "
                               "a minimum difference of {:g}",
                               MINSINGLEPASSLIFT));
    }

    for (std::shared_ptr<HeatingLogic> shutOffLogic : heatSources[heatSourceIndex]->shutOffLogicSet)
    {
        if (shutOffLogic->getIsEnteringWaterHighTempShutoff())
        {
            std::dynamic_pointer_cast<TempBasedHeatingLogic>(shutOffLogic)
                ->setDecisionPoint(highTemp_C, tempIsAbsolute);
            break;
        }
    }
}

void HPWH::setTargetSoCFraction(double target)
{
    if (!isSoCControlled())
    {
        send_error("Can not set target state of charge if HPWH is not using state of charge "
                   "controls.");
    }
    if (target < 0)
    {
        send_error("Can not set a negative target state of charge.");
    }

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i]->shutOffLogicSet)
        {
            if (!logic->getIsEnteringWaterHighTempShutoff())
            {
                logic->setDecisionPoint(target);
            }
        }
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i]->turnOnLogicSet)
        {
            logic->setDecisionPoint(target);
        }
    }
}

bool HPWH::isSoCControlled() const { return usesSoCLogic; }

bool HPWH::canUseSoCControls()
{
    bool retVal = true;
    if (getCompressorCoilConfig() != HPWH::Condenser::CONFIG_EXTERNAL)
    {
        retVal = false;
    }
    return retVal;
}

void HPWH::switchToSoCControls(double targetSoC,
                               double hysteresisFraction /*= 0.05*/,
                               double tempMinUseful /*= 43.333*/,
                               bool constantMainsT /*= false*/,
                               double mainsT /*= 18.333*/,
                               UNITS tempUnits /*= UNITS_C*/)
{
    if (!canUseSoCControls())
    {
        send_error("Cannot set up state of charge controls for integrated or wrapped HPWHs.");
    }

    double tempMinUseful_C = tempMinUseful;
    double mainsT_C = mainsT;
    switch (tempUnits)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        tempMinUseful_C = F_TO_C(tempMinUseful);
        mainsT_C = F_TO_C(mainsT);
        break;
    default:
        send_error("Invalid units.");
    }

    if (mainsT_C >= tempMinUseful_C)
    {
        send_error("The mains temperature can't be equal to or greater than the minimum useful "
                   "temperature.");
    }

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i]->clearAllTurnOnLogic();

        heatSources[i]->shutOffLogicSet.erase(
            std::remove_if(heatSources[i]->shutOffLogicSet.begin(),
                           heatSources[i]->shutOffLogicSet.end(),
                           [&](const auto logic) -> bool
                           { return !logic->getIsEnteringWaterHighTempShutoff(); }),
            heatSources[i]->shutOffLogicSet.end());

        heatSources[i]->shutOffLogicSet.push_back(shutOffSoC("SoC Shut Off",
                                                             targetSoC,
                                                             hysteresisFraction,
                                                             tempMinUseful_C,
                                                             constantMainsT,
                                                             mainsT_C));
        heatSources[i]->turnOnLogicSet.push_back(turnOnSoC("SoC Turn On",
                                                           targetSoC,
                                                           hysteresisFraction,
                                                           tempMinUseful_C,
                                                           constantMainsT,
                                                           mainsT_C));
    }

    usesSoCLogic = true;
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

HPWH::Distribution HPWH::getRangeDistribution(double bottomFraction, double topFraction)
{
    std::vector<double> heights = {}, weights = {};
    if (bottomFraction > 0.)
    {
        heights.push_back(bottomFraction);
        weights.push_back(0.);
    }
    heights.push_back(topFraction);
    weights.push_back(1.);
    if (topFraction < 1.)
    {
        heights.push_back(1.);
        weights.push_back(0.);
    }
    return {heights, weights};
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::wholeTank(double decisionPoint,
                                                             const UNITS units /* = UNITS_C */,
                                                             const bool absolute /* = false */)
{
    auto dist = getRangeDistribution(0., 1.);
    double decisionPoint_C = convertTempToC(decisionPoint, units, absolute);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "whole tank", dist, decisionPoint_C, this, absolute);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThird(double decisionPoint)
{
    auto dist = getRangeDistribution(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>("top third", dist, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThird_absolute(double decisionPoint)
{
    auto dist = getRangeDistribution(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top third absolute", dist, decisionPoint, this, true);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondThird(double decisionPoint,
                                                               const UNITS units /* = UNITS_C */,
                                                               const bool absolute /* = false */)
{
    auto dist = getRangeDistribution(1. / 3., 2. / 3.);
    double decisionPoint_C = convertTempToC(decisionPoint, units, absolute);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second third", dist, decisionPoint_C, this, absolute);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomThird(double decisionPoint)
{
    auto dist = getRangeDistribution(0., 1. / 3.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>("bottom third", dist, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixth(double decisionPoint)
{
    auto dist = getRangeDistribution(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>("bottom sixth", dist, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixth_absolute(double decisionPoint)
{
    auto dist = getRangeDistribution(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom sixth absolute", dist, decisionPoint, this, true);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondSixth(double decisionPoint)
{
    auto dist = getRangeDistribution(1. / 6., 2. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>("second sixth", dist, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::thirdSixth(double decisionPoint)
{
    auto dist = getRangeDistribution(2. / 6., 3. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>("third sixth", dist, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fourthSixth(double decisionPoint)
{
    auto dist = getRangeDistribution(3. / 6., 4. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>("fourth sixth", dist, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fifthSixth(double decisionPoint)
{
    auto dist = getRangeDistribution(4. / 6., 5. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>("fifth sixth", dist, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topSixth(double decisionPoint)
{
    auto dist = getRangeDistribution(5. / 6., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>("top sixth", dist, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomHalf(double decisionPoint)
{
    auto dist = getRangeDistribution(0., 1. / 2.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>("bottom half", dist, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomTwelfth(double decisionPoint)
{
    auto dist = getRangeDistribution(0., 1. / 12.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom twelfth", dist, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::standby(double decisionPoint)
{
    HPWH::Distribution dist = DistributionType::TopOfTank;
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "standby", dist, decisionPoint, this, false, std::less<double>(), false, true);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topNodeMaxTemp(double decisionPoint)
{
    HPWH::Distribution dist = DistributionType::TopOfTank;
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top of tank", dist, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic>
HPWH::bottomNodeMaxTemp(double decisionPoint, bool isEnteringWaterHighTempShutoff /*=false*/)
{
    HPWH::Distribution dist = DistributionType::BottomOfTank;
    return std::make_shared<HPWH::TempBasedHeatingLogic>("bottom of tank",
                                                         dist,
                                                         decisionPoint,
                                                         this,
                                                         true,
                                                         std::greater<double>(),
                                                         isEnteringWaterHighTempShutoff);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomTwelfthMaxTemp(double decisionPoint)
{
    auto dist = getRangeDistribution(0., 1. / 12.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom twelfth", dist, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThirdMaxTemp(double decisionPoint)
{
    auto dist = getRangeDistribution(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top third", dist, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixthMaxTemp(double decisionPoint)
{
    auto dist = getRangeDistribution(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom sixth", dist, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondSixthMaxTemp(double decisionPoint)
{
    auto dist = getRangeDistribution(1. / 6., 2. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second sixth", dist, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fifthSixthMaxTemp(double decisionPoint)
{
    auto dist = getRangeDistribution(4. / 6., 5. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top sixth", dist, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topSixthMaxTemp(double decisionPoint)
{
    auto dist = getRangeDistribution(5. / 6., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top sixth", dist, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::largeDraw(double decisionPoint)
{
    auto dist = getRangeDistribution(0., 1. / 4.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "large draw", dist, decisionPoint, this, true);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::largerDraw(double decisionPoint)
{
    auto dist = getRangeDistribution(0., 1. / 2.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "larger draw", dist, decisionPoint, this, true);
}

void HPWH::setNumNodes(const std::size_t num_nodes) { tank->setNumNodes(num_nodes); }

int HPWH::getNumNodes() const { return tank->getNumNodes(); }

int HPWH::getIndexTopNode() const { return tank->getIndexTopNode(); }

double HPWH::getTankNodeTemp(int nodeNum, UNITS units /*=UNITS_C*/) const
{
    double result = tank->getNodeT_C(nodeNum);
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        result = C_TO_F(result);
        break;
    default:
        send_error("Invalid units.");
    }
    return result;
}

double HPWH::getNthSimTcouple(int iTCouple, int nTCouple, UNITS units /*=UNITS_C*/) const
{
    double result = tank->getNthSimTcouple(iTCouple, nTCouple);
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        result = C_TO_F(result);
        break;
    default:
        send_error("Invalid units.");
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
        count += heatSources[i]->isAResistance() ? 1 : 0;
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
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }

    double airTemp_C = airTemp;
    double inletTemp_C = inletTemp;
    double outTemp_C = outTemp;
    switch (tempUnit)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        airTemp_C = F_TO_C(airTemp);
        inletTemp_C = F_TO_C(inletTemp);
        outTemp_C = F_TO_C(outTemp);
        break;
    default:
        send_error("Invalid units.");
    }

    auto cond_ptr = reinterpret_cast<Condenser*>(heatSources[compressorIndex].get());

    double tolT_C = 1.e-9;
    if (airTemp_C < cond_ptr->minT || (airTemp_C > cond_ptr->maxT + tolT_C))
    {
        send_error("The compress does not operate at the specified air temperature.");
    }

    double maxAllowedSetpoint_C =
        cond_ptr->maxSetpoint_C - cond_ptr->secondaryHeatExchanger.hotSideTemperatureOffset_dC;

    if (outTemp_C > maxAllowedSetpoint_C + tolT_C)
    {
        send_error("Inputted outlet temperature of the compressor is higher than can be produced.");
    }

    Condenser::Performance performance = {0., 0., 0.};
    if (cond_ptr->configuration == Condenser::COIL_CONFIG::CONFIG_EXTERNAL)
    {
        if (cond_ptr->isExternalMultipass())
        {
            double averageTemp_C = (outTemp_C + inletTemp_C) / 2.;
            performance = cond_ptr->getPerformance(airTemp_C, averageTemp_C);
        }
        else
        {
            performance = cond_ptr->getPerformance(airTemp_C, inletTemp_C);
        }
    }
    else
    {
        performance = cond_ptr->getPerformance(airTemp_C, inletTemp_C);
    }

    double outputCapacity = performance.output_BTUperHr;
    switch (pwrUnit)
    {
    case UNITS_BTUperHr:
        break;
    case UNITS_KW:
        outputCapacity = BTU_TO_KWH(performance.output_BTUperHr);
        break;
    default:
        send_error("Invalid units.");
    }

    return outputCapacity;
}

double HPWH::getNthHeatSourceEnergyInput(int N, UNITS units /*=UNITS_KWH*/) const
{
    // energy used by the heat source is positive - this should always be positive
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error(
            "You have attempted to access the energy input of a heat source that does not exist.");
    }

    double energyInput = heatSources[N]->energyInput_kWh;
    switch (units)
    {
    case UNITS_KWH:
        break;
    case UNITS_BTU:
        energyInput = KWH_TO_BTU(heatSources[N]->energyInput_kWh);
        break;
    case UNITS_KJ:
        energyInput = KWH_TO_KJ(heatSources[N]->energyInput_kWh);
        break;
    default:
        send_error("Invalid units.");
    }

    return energyInput;
}

double HPWH::getNthHeatSourceEnergyOutput(int N, UNITS units /*=UNITS_KWH*/) const
{
    // returns energy from the heat source into the water - this should always be positive
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error(
            "You have attempted to access the energy output of a heat source that does not exist.");
    }

    double energyOutput = heatSources[N]->energyOutput_kWh;
    switch (units)
    {
    case UNITS_KWH:
        break;
    case UNITS_BTU:
        energyOutput = KWH_TO_BTU(heatSources[N]->energyOutput_kWh);
        break;
    case UNITS_KJ:
        energyOutput = KWH_TO_KJ(heatSources[N]->energyOutput_kWh);
        break;
    default:
        send_error("Invalid units.");
    }

    return energyOutput;
}

double HPWH::getNthHeatSourceRunTime(int N) const
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error(
            "You have attempted to access the run time of a heat source that does not exist.");
    }
    return heatSources[N]->runtime_min;
}

int HPWH::isNthHeatSourceRunning(int N) const
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error("You have attempted to access the status of a heat source that does not exist.");
    }
    int result = 0;
    if (heatSources[N]->isEngaged())
    {
        result = 1;
    }
    return result;
}

int HPWH::isCompressorRunning() const { return isNthHeatSourceRunning(getCompressorIndex()); }

HPWH::HEATSOURCE_TYPE HPWH::getNthHeatSourceType(int N) const
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error("You have attempted to access the type of a heat source that does not exist.");
    }
    return heatSources[N]->typeOfHeatSource();
}

bool HPWH::getNthHeatSource(int N, HPWH::HeatSource*& heatSource)
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_warning("You have attempted to access the type of a heat source that does not exist.");
        return false;
    }
    heatSource = heatSources[N].get();
    return true;
}

HPWH::HeatSource* HPWH::getNthHeatSource(int N)
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error("You have attempted to access the type of a heat source that does not exist.");
    }
    return heatSources[N].get();
}

double HPWH::getTankSize(UNITS units /*=UNITS_L*/) const
{
    double volume = tank->getVolume_L();
    switch (units)
    {
    case UNITS_L:
        break;
    case UNITS_GAL:
        volume = L_TO_GAL(volume);
        break;
    default:
        send_error("Invalid units.");
    }
    return volume;
}

double HPWH::getExternalVolumeHeated(UNITS units /*=UNITS_L*/) const
{
    double volume = externalVolumeHeated_L;
    switch (units)
    {
    case UNITS_L:
        break;
    case UNITS_GAL:
        volume = L_TO_GAL(externalVolumeHeated_L);
        break;
    default:
        send_error("Invalid units.");
    }
    return volume;
}

double HPWH::getEnergyRemovedFromEnvironment(UNITS units /*=UNITS_KWH*/) const
{
    // moving heat from the space to the water is the positive direction
    double energy = energyRemovedFromEnvironment_kWh;
    switch (units)
    {
    case UNITS_KWH:
        break;
    case UNITS_BTU:
        energy = KWH_TO_BTU(energyRemovedFromEnvironment_kWh);
        break;
    case UNITS_KJ:
        energy = KWH_TO_KJ(energyRemovedFromEnvironment_kWh);
        break;
    default:
        send_error("Invalid units.");
    }
    return energy;
}

double HPWH::getStandbyLosses(UNITS units /*=UNITS_KWH*/) const
{
    // moving heat from the water to the space is the positive direction
    double energy = standbyLosses_kWh;
    switch (units)
    {
    case UNITS_KWH:
        break;
    case UNITS_BTU:
        energy = KWH_TO_BTU(standbyLosses_kWh);
        break;
    case UNITS_KJ:
        energy = KWH_TO_KJ(standbyLosses_kWh);
        break;
    default:
        send_error("Invalid units.");
    }
    return energy;
}

double HPWH::getOutletTemp(UNITS units /*=UNITS_C*/) const
{
    double temp = tank->getOutletT_C();
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        temp = C_TO_F(temp);
        break;
    default:
        send_error("Invalid units.");
    }
    return temp;
}

double HPWH::getCondenserWaterInletTemp(UNITS units /*=UNITS_C*/) const
{
    double temp = condenserInlet_C;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        temp = C_TO_F(condenserInlet_C);
        break;
    default:
        send_error("Invalid units.");
    }
    return temp;
}

double HPWH::getCondenserWaterOutletTemp(UNITS units /*=UNITS_C*/) const
{
    double temp = condenserOutlet_C;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        temp = C_TO_F(condenserOutlet_C);
        break;
    default:
        send_error("Invalid units.");
    }
    return temp;
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

double HPWH::getAverageTankTemp_C(const WeightedDistribution& wdist) const
{
    return tank->getAverageNodeT_C(wdist);
}

double HPWH::getAverageTankTemp_C(const Distribution& dist) const
{
    return tank->getAverageNodeT_C(dist);
}

void HPWH::setTankToTemperature(double temp_C) { tank->setNodeT_C(temp_C); }

double HPWH::getTankVolume_L() const { return tank->getVolume_L(); }

double HPWH::getTankHeatContent_kJ() const { return tank->getHeatContent_kJ(); }

int HPWH::getModel() const { return model; }

int HPWH::getCompressorCoilConfig() const
{
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    auto cond_ptr = reinterpret_cast<Condenser*>(heatSources[compressorIndex].get());
    return cond_ptr->configuration;
}

int HPWH::isCompressorMultipass() const
{
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    auto cond_ptr = reinterpret_cast<Condenser*>(heatSources[compressorIndex].get());
    return static_cast<int>(cond_ptr->isMultipass);
}

int HPWH::isCompressorExternalMultipass() const
{
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    auto cond_ptr = reinterpret_cast<Condenser*>(heatSources[compressorIndex].get());
    return static_cast<int>(cond_ptr->isExternalMultipass());
}

int HPWH::isCompressorExternal() const
{
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    auto cond_ptr = reinterpret_cast<Condenser*>(heatSources[compressorIndex].get());
    return static_cast<int>(cond_ptr->isExternal());
}

bool HPWH::hasACompressor() const { return compressorIndex >= 0; }

bool HPWH::hasExternalHeatSource(std::size_t& heatSourceIndex) const
{
    heatSourceIndex = 0;
    for (auto heatSource_ptr : heatSources)
    {
        if (heatSource_ptr->isACompressor())
        {
            auto cond_ptr = reinterpret_cast<Condenser*>(heatSources[compressorIndex].get());
            if (cond_ptr->configuration == Condenser::CONFIG_EXTERNAL)
            {
                return true;
            }
        }
        ++heatSourceIndex;
    }
    return false;
}

double HPWH::getExternalMPFlowRate(UNITS units /*=UNITS_GPM*/) const
{
    if (!isCompressorExternalMultipass())
    {
        send_error("Does not have an external multipass heat source.");
    }
    auto cond_ptr = reinterpret_cast<Condenser*>(heatSources[compressorIndex].get());
    double flowRate = cond_ptr->mpFlowRate_LPS;
    if (units == HPWH::UNITS_LPS)
    {
    }
    else if (units == HPWH::UNITS_GPM)
    {
        flowRate = LPS_TO_GPM(flowRate);
    }
    else
        send_error("Invalid units.");

    return flowRate;
}

double HPWH::getCompressorMinRuntime(UNITS units /*=UNITS_MIN*/) const
{
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    double minimumRuntime = 10.;
    switch (units)
    {
    case UNITS_MIN:
        break;
    case UNITS_SEC:
        minimumRuntime = MIN_TO_SEC(minimumRuntime);
        break;
    case UNITS_HR:
        minimumRuntime = MIN_TO_HR(minimumRuntime);
        break;
    default:
        send_error("Invalid units.");
    }
    return minimumRuntime;
}

void HPWH::getSizingFractions(double& aquaFract, double& useableFract) const
{
    double aFract = 1.;
    double useFract = 1.;

    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    else if (usesSoCLogic)
    {
        send_error("Current model uses SOC control logic and does not have a definition for "
                   "sizing fractions.");
    }

    // Every compressor must have at least one on logic
    for (std::shared_ptr<HeatingLogic> onLogic : heatSources[compressorIndex]->turnOnLogicSet)
    {
        double tempA = onLogic->nodeWeightAvgFract(); // if standby logic will return 1
        aFract = tempA < aFract ? tempA : aFract;
    }
    aquaFract = aFract;

    // Compressors don't need to have an off logic
    if (heatSources[compressorIndex]->shutOffLogicSet.size() != 0)
    {
        for (std::shared_ptr<HeatingLogic> offLogic : heatSources[compressorIndex]->shutOffLogicSet)
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
}

void HPWH::setInletByFraction(double fractionalHeight)
{
    tank->setInletByFraction(fractionalHeight);
}

void HPWH::setInlet2ByFraction(double fractionalHeight)
{
    tank->setInlet2ByFraction(fractionalHeight);
}

bool HPWH::isScalable() const { return canScale; }

void HPWH::setScaleCapacityCOP(double scaleCapacity /*=1.0*/, double scaleCOP /*=1.0*/)
{
    if (!isScalable())
    {
        send_error("Cannot scale the HPWH Capacity or COP.");
    }
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    if (scaleCapacity <= 0 || scaleCOP <= 0)
    {
        send_error("Can not scale the HPWH Capacity or COP to 0 or less than 0.");
    }

    auto cond_ptr = reinterpret_cast<Condenser*>(heatSources[compressorIndex].get());

    if (cond_ptr->useBtwxtGrid)
    {
        scaleVector(cond_ptr->perfGridValues[0], scaleCapacity);
        scaleVector(cond_ptr->perfGridValues[1], scaleCOP);
        cond_ptr->makeBtwxt();
    }
    else
        for (auto& performancePoint : cond_ptr->performanceMap)
        {
            scaleVector(performancePoint.inputPower_coeffs, scaleCapacity);
            scaleVector(performancePoint.COP_coeffs, scaleCOP);
        }
}

void HPWH::setCompressorOutputCapacity(double newCapacity,
                                       double airTemp /*=19.722*/,
                                       double inletTemp /*=14.444*/,
                                       double outTemp /*=57.222*/,
                                       UNITS pwrUnit /*=UNITS_KW*/,
                                       UNITS tempUnit /*=UNITS_C*/)
{

    double oldCapacity = getCompressorCapacity(airTemp, inletTemp, outTemp, pwrUnit, tempUnit);
    double scale = newCapacity / oldCapacity;
    setScaleCapacityCOP(scale, 1.); // Scale the compressor capacity
}

void HPWH::setResistanceCapacity(double power, int which /*=-1*/, UNITS pwrUnit /*=UNITS_KW*/)
{

    // Input checks
    if (!isScalable())
    {
        send_error("Cannot scale the resistance elements.");
    }
    if (getNumResistanceElements() == 0)
    {
        send_error("There are no resistance elements.");
    }
    if (which < -1 || which > getNumResistanceElements() - 1)
    {
        send_error("Out of bounds value for \"which\" in setResistanceCapacity().");
    }
    if (power < 0)
    {
        send_error("Can not have a negative input power.");
    }
    // Unit conversion
    double watts = 1000. * power;
    switch (pwrUnit)
    {
    case UNITS_KW:
        break;
    case UNITS_BTUperHr:
        watts = BTU_TO_KWH(watts); // kBTU/h to W
        break;
    default:
        send_error("Invalid units.");
    }

    // Whew so many checks...
    if (which == -1)
    {
        // Just get all the elements
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i]->isAResistance())
            {
                reinterpret_cast<Resistance*>(heatSources[i].get())->setPower_W(watts);
            }
        }
    }
    else
    {
        reinterpret_cast<Resistance*>(heatSources[resistanceHeightMap[which].index].get())
            ->setPower_W(watts);

        // Then check for repeats in the position
        int pos = resistanceHeightMap[which].position;
        for (int i = 0; i < getNumResistanceElements(); i++)
        {
            if (which != i && resistanceHeightMap[i].position == pos)
            {
                reinterpret_cast<Resistance*>(heatSources[resistanceHeightMap[i].index].get())
                    ->setPower_W(watts);
            }
        }
    }
}

double HPWH::getResistanceCapacity(int which /*=-1*/, UNITS pwrUnit /*=UNITS_KW*/)
{

    // Input checks
    if (getNumResistanceElements() == 0)
    {
        send_error("There are no resistance elements.");
    }
    if (which < -1 || which > getNumResistanceElements() - 1)
    {
        send_error("Out of bounds value for \"which\" in getResistanceCapacity().");
    }

    double returnPower_W = 0;
    if (which == -1)
    {
        // Just get all the elements
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i]->isAResistance())
            {
                returnPower_W += reinterpret_cast<Resistance*>(heatSources[i].get())->getPower_W();
            }
        }
    }
    else
    {
        // get the power from "which" element by height
        returnPower_W +=
            reinterpret_cast<Resistance*>(heatSources[resistanceHeightMap[which].index].get())
                ->getPower_W();

        // Then check for repeats in the position
        int pos = resistanceHeightMap[which].position;
        for (int i = 0; i < getNumResistanceElements(); i++)
        {
            if (which != i && resistanceHeightMap[i].position == pos)
            {
                returnPower_W +=
                    reinterpret_cast<Resistance*>(heatSources[resistanceHeightMap[i].index].get())
                        ->getPower_W();
            }
        }
    }

    // Unit conversion
    switch (pwrUnit)
    {
    case UNITS_KW:
        return returnPower_W / 1000.;
    case UNITS_BTUperHr:
        return KWH_TO_BTU(returnPower_W / 1000.); // kW to BTU/h
    default:
        send_error("Invalid units.");
    }

    return 0.;
}

int HPWH::getResistancePosition(int elementIndex) const
{
    if (elementIndex < 0 || elementIndex > getNumHeatSources() - 1)
    {
        send_error("Out of bounds value for which in getResistancePosition.");
    }

    if (!heatSources[elementIndex]->isAResistance())
    {
        send_error("This index is not a resistance element.");
    }
    return static_cast<int>(HeatSource::CONDENSITY_SIZE *
                            heatSources[elementIndex]->heatDist.lowestNormalizedHeight());
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
        heatSources[i]->disengageHeatSource();
    }
    isHeating = false;
}

bool HPWH::areAllHeatSourcesOff() const
{
    bool allOff = true;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i]->isEngaged() == true)
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
        if (heatSources[i]->isACompressor())
        {
            heatSources[i]->depressesTemperature = true;

            auto condenser = reinterpret_cast<Condenser*>(heatSources[i].get());
            condenser->setEvaluatePerformanceFunction();
        }
        else if (heatSources[i]->isAResistance())
        {
            heatSources[i]->depressesTemperature = false;
        }
    }
}

void HPWH::calcDerivedHeatingValues()
{
    // find condentropy/shrinkage
    for (int i = 0; i < getNumHeatSources(); ++i)
    {
        heatSources[i]->Tshrinkage_C =
            findShrinkageT_C(heatSources[i]->heatDist, tank->getNumNodes());
    }

    // find lowest node
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i]->lowestNode = findLowestNode(heatSources[i]->heatDist, tank->getNumNodes());
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
        if (heatSources[i]->isACompressor())
        {
            compressorIndex = i; // NOTE: Maybe won't work with multiple compressors (last
                                 // compressor will be used)
        }
        else if (heatSources[i]->isAResistance())
        {
            // Gets VIP element index
            if (heatSources[i]->isVIP)
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
            double pos = heatSources[i]->heatDist.lowestNormalizedHeight();
            if (pos < lowestPos)
            {
                lowestElementIndex = i;
                lowestPos = pos;
            }
            pos = heatSources[i]->heatDist.lowestNormalizedHeight();
            if (pos >= highestPos)
            {
                highestElementIndex = i;
                highestPos = pos;
            }
        }
    }

    // heat source ability to depress temp
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i]->isACompressor())
        {
            heatSources[i]->depressesTemperature = true;
        }
        else if (heatSources[i]->isAResistance())
        {
            heatSources[i]->depressesTemperature = false;
        }
    }
}

void HPWH::mapResRelativePosToHeatSources()
{
    resistanceHeightMap.clear();
    resistanceHeightMap.reserve(getNumResistanceElements());

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i]->isAResistance())
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
/// @param[in]	drawVol1_L				Inlet-1 water volume
/// @param[in]	inlet1T_C				Inlet-1 water temperature
/// @param[in]	drawVol2_L				Inlet 2 water volume
/// @param[in]	inlet2T_C				Inlet-2 water temperature
///	@param[in]	prevHeatContent_kJ		Heat content of tank prior to simulation step
///	@param[in]	fracEnergyTolerance		Fractional tolerance for energy imbalance
/// @return	true if balanced; false otherwise.
//-----------------------------------------------------------------------------
bool HPWH::isEnergyBalanced(const double drawVol1_L,
                            const double inlet1T_C,
                            const double drawVol2_L,
                            const double inlet2T_C,
                            const double prevHeatContent_kJ,
                            const double fracEnergyTolerance /* = 0.001 */)
{

    // Check energy balancing.
    double qInElectrical_kJ = 0.;
    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        qInElectrical_kJ += getNthHeatSourceEnergyInput(iHS, UNITS_KJ);
    }
    double qInExtra_kJ = KWH_TO_KJ(extraEnergyInput_kWh);
    double qInHeatSourceEnviron_kJ = getEnergyRemovedFromEnvironment(UNITS_KJ);
    double qOutTankEnviron_kJ = KWH_TO_KJ(standbyLosses_kWh);

    double drawCp1_kJperC =
        CPWATER_kJperkgC * DENSITYWATER_kgperL * drawVol1_L; // heat capacity of inlet1 draw

    double drawCp2_kJperC =
        CPWATER_kJperkgC * DENSITYWATER_kgperL * drawVol2_L; // heat capacity of inlet2 draw

    double qOutWater_kJ = drawCp1_kJperC * (tank->getOutletT_C() - inlet1T_C) +
                          drawCp2_kJperC * (tank->getOutletT_C() - inlet2T_C);

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
            fmt::format("Energy-balance error: {:g} kJ, {:g} %", qBal_kJ, 100. * fracEnergyDiff));
        return false;
    }
    return true;
}

bool compressorIsRunning(HPWH& hpwh)
{
    return (bool)hpwh.isNthHeatSourceRunning(hpwh.getCompressorIndex());
}

// Used to check a few inputs after the initialization of a tank model from a preset or a file.
void HPWH::checkInputs()
{
    std::queue<std::string> error_msgs = {};

    if (getNumHeatSources() <= 0 && (model != MODELS_StorageTank))
    {
        error_msgs.push("You must have at least one HeatSource.");
    }

    // loop through all heat sources to check each for malconfigurations
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        // check the heat source type to make sure it has been set
        if (heatSources[i]->typeOfHeatSource() == TYPE_none)
        {
            error_msgs.push(fmt::format(
                "Heat source {} does not have a specified type.  Initialization failed.", i));
        }
        // check to make sure there is at least one onlogic or parent with onlogic
        int parent = heatSources[i]->findParent();
        if (heatSources[i]->turnOnLogicSet.size() == 0 &&
            (parent == -1 || heatSources[parent]->turnOnLogicSet.size() == 0))
        {
            error_msgs.push(
                "You must specify at least one logic to turn on the element or the element "
                "must be set as a backup for another heat source with at least one logic.");
        }

        // Validate on logics
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i]->turnOnLogicSet)
        {
            if (!logic->isValid())
            {
                error_msgs.push(fmt::format("On logic at index {:d} is invalid.", i));
            }
        }
        // Validate off logics
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i]->shutOffLogicSet)
        {
            if (!logic->isValid())
            {
                error_msgs.push(fmt::format("Off logic at index {:d} is invalid.", i));
            }
        }
        // check is distribution is valid
        if (!heatSources[i]->heatDist.isValid())
        {
            error_msgs.push(
                fmt::format("The heat distribution for heatsource {:d} is invalid.", i));
        }
        // check that air flows are all set properly
        if (heatSources[i]->airflowFreedom > 1.0 || heatSources[i]->airflowFreedom <= 0.0)
        {
            error_msgs.push(fmt::format(
                "\n\tThe airflowFreedom must be between 0 and 1 for heatsource {:d}.", i));
        }

        if (heatSources[i]->isACompressor())
        {
            auto cond_ptr = reinterpret_cast<Condenser*>(heatSources[i].get());
            if (cond_ptr->doDefrost)
            {
                if (cond_ptr->defrostMap.size() < 3)
                {
                    error_msgs.push(
                        "Defrost logic set to true but no valid defrost map of length 3 or "
                        "greater set.");
                }
                if (cond_ptr->configuration != Condenser::CONFIG_EXTERNAL)
                {
                    error_msgs.push("Defrost is only simulated for external compressors.");
                }
            }

            if (cond_ptr->configuration == Condenser::CONFIG_EXTERNAL)
            {
                if (heatSources[i]->shutOffLogicSet.size() != 1)
                {
                    error_msgs.push("External heat sources can only have one shut off logic.");
                }
                if (0 > heatSources[i]->externalOutletHeight ||
                    heatSources[i]->externalOutletHeight > getNumNodes() - 1)
                {
                    error_msgs.push(
                        "External heat sources need an external outlet height within the bounds"
                        "from from 0 to numNodes-1.");
                }
                if (0 > heatSources[i]->externalInletHeight ||
                    heatSources[i]->externalInletHeight > getNumNodes() - 1)
                {
                    error_msgs.push(
                        "External heat sources need an external inlet height within the bounds "
                        "from from 0 to numNodes-1.");
                }
            }
            else
            {
                if (cond_ptr->secondaryHeatExchanger.extraPumpPower_W != 0 ||
                    cond_ptr->secondaryHeatExchanger.extraPumpPower_W)
                {
                    error_msgs.push(fmt::format(
                        "Heatsource {:d} is not an external heat source but has an external "
                        "secondary heat exchanger.",
                        i));
                }
            }

            // Check performance map
            // perfGrid and perfGridValues, and the length of vectors in perfGridValues are equal
            // and that ;
            if (cond_ptr->useBtwxtGrid)
            {
                // If useBtwxtGrid is true that the perfMap is empty
                if (cond_ptr->performanceMap.size() != 0)
                {
                    error_msgs.push("\n\tUsing the grid lookups but a regression-based performance "
                                    "map is given.");
                }

                // Check length of vectors in perfGridValue are equal
                if (cond_ptr->perfGridValues[0].size() != cond_ptr->perfGridValues[1].size() &&
                    cond_ptr->perfGridValues[0].size() != 0)
                {
                    error_msgs.push("When using grid lookups for performance the vectors in "
                                    "perfGridValues must "
                                    "be the same length.");
                }

                // Check perfGrid's vectors lengths multiplied together == the perfGridValues vector
                // lengths
                size_t expLength = 1;
                for (const auto& v : cond_ptr->perfGrid)
                {
                    expLength *= v.size();
                }
                if (expLength != cond_ptr->perfGridValues[0].size())
                {
                    error_msgs.push(
                        "When using grid lookups for perfmance the vectors in perfGridValues must "
                        "be the same length.");
                }
            }

            else
            {
                // Check that perfmap only has 1 point if config_external and multipass
                if (cond_ptr->isExternalMultipass() && cond_ptr->performanceMap.size() != 1)
                {
                    error_msgs.push(
                        "External multipass heat sources must have a perfMap of only one point "
                        "with regression equations.");
                }
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
            error_msgs.push(
                "The relationship between the on logic and off logic is not supported. The off "
                "logic is beneath the on logic.");
        }
    }

    double maxTemp;
    string why;
    double tempSetpoint = setpoint_C;
    if (!isNewSetpointPossible(tempSetpoint, maxTemp, why))
    {
        error_msgs.push(fmt::format("Cannot set new setpoint. ({})", why.c_str()));
    }

    // Check if the UA is out of bounds
    if (tank->UA_kJperHrC < 0.0)
    {
        error_msgs.push(
            fmt::format("The tankUA_kJperHrC is less than 0 for a HPWH, it must be greater than 0, "
                        "tankUA_kJperHrC is: {:g}",
                        tank->getUA_kJperHrC()));
    }

    // Check single-node heat-exchange effectiveness validity
    if (tank->heatExchangerEffectiveness > 1.)
    {
        error_msgs.push("Heat-exchanger effectiveness cannot exceed 1.");
    }

    if (error_msgs.size() > 0)
    {
        while (error_msgs.size() > 1)
        {
            try
            {
                send_error(error_msgs.front());
            }
            catch (...)
            {
                error_msgs.pop();
            }
        }
        send_error(error_msgs.front());
    }
}

/* static */ bool HPWH::getPresetNumberFromName(const std::string& modelName, HPWH::MODELS& model)
{
    for (auto& preset : hpwh_presets::index)
    {
        if (modelName == preset.second.name)
        {
            model = static_cast<HPWH::MODELS>(preset.first);
            return true;
        }
        else if (modelName == "StorageTank")
        {
            model = HPWH::MODELS_StorageTank;
            return true;
        }
    }
    model = HPWH::MODELS_basicIntegrated;
    return false;
}

/* static */
bool HPWH::getPresetNameFromNumber(std::string& modelName, const HPWH::MODELS& model)
{
    modelName = hpwh_presets::index[model].name;
    return true;
}

void HPWH::init(HPWH::MODELS presetNum)
{
    const auto& jsonText = hpwh_presets::index[presetNum].json_text;
    nlohmann::json j = nlohmann::json::parse(jsonText);

    hpwh_data_model::init(get_courier());
    hpwh_data_model::hpwh_sim_input::HPWHSimInput hsi;
    hpwh_data_model::hpwh_sim_input::from_json(j, hsi);
    model = presetNum;
    from(hsi);
}

void HPWH::init(const std::string& presetName)
{
    HPWH::MODELS presetNum;
    if (getPresetNumberFromName(presetName, presetNum))
    {
        initPreset(presetNum);
        name = presetName;
    }
    else
    {
        send_error("Unable to initialize model.");
    }
}

/// Initializes a preset from the modelName
void HPWH::initPreset(const std::string& modelName)
{
    HPWH::MODELS presetNum;
    if (getPresetNumberFromName(modelName, presetNum))
    {
        initPreset(presetNum);
    }
    else
    {
        send_error("Unable to initialize model.");
    }
    name = modelName;
}

#ifndef HPWH_ABRIDGED

void HPWH::initFromJSON(const HPWH::MODELS presetNumber)
{
    initFromJSON({*hpwh_presets::index[presetNumber].name});
}

void HPWH::initFromJSON(const std::string modelName)
{
    auto sInputFileName = "models_json/" + modelName + ".json";
    std::ifstream inputFile(sInputFileName);
    nlohmann::json j = nlohmann::json::parse(inputFile);
    hpwh_data_model::init(get_courier());

    getPresetNumberFromName(modelName, model);

    hpwh_data_model::hpwh_sim_input::HPWHSimInput hsi;
    hpwh_data_model::hpwh_sim_input::from_json(j, hsi);
    from(hsi);
}

#endif

void HPWH::from(const hpwh_data_model::hpwh_sim_input::HPWHSimInput& hsi)
{
    checkFrom(doTempDepression, hsi.depresses_temperature_is_set, hsi.depresses_temperature, false);

    int number_of_nodes;
    checkFrom(number_of_nodes, hsi.number_of_nodes_is_set, hsi.number_of_nodes, 12);
    tank->setNumNodes(number_of_nodes);
    checkFrom(
        setpoint_C, hsi.standard_setpoint_is_set, K_TO_C(hsi.standard_setpoint), F_TO_C(135.));

    if (hsi.system_type_is_set)
        switch (hsi.system_type)
        {
        case hpwh_data_model::hpwh_sim_input::HPWHSystemType::INTEGRATED:
        {
            if (hsi.integrated_system_is_set)
                from(hsi.integrated_system);

            break;
        }
        case hpwh_data_model::hpwh_sim_input::HPWHSystemType::CENTRAL:
        {
            if (hsi.central_system_is_set)
            {
                from(hsi.central_system);
                auto compressor =
                    reinterpret_cast<Condenser*>(heatSources[getCompressorIndex()].get());
                compressor->externalInletHeight = static_cast<int>(
                    hsi.central_system.external_inlet_height * hsi.number_of_nodes);
                compressor->externalOutletHeight = static_cast<int>(
                    hsi.central_system.external_outlet_height * hsi.number_of_nodes);
                if (hsi.central_system.fixed_flow_rate_is_set)
                {
                    compressor->isMultipass = true;
                    compressor->mpFlowRate_LPS = 1000. * hsi.central_system.fixed_flow_rate;
                }
                else
                    compressor->isMultipass = false;

                compressor->configuration = Condenser::COIL_CONFIG::CONFIG_EXTERNAL;
            }
            break;
        }
        case hpwh_data_model::hpwh_sim_input::HPWHSystemType::UNKNOWN:
            return;
        }

    checkFrom(tank->volumeFixed, hsi.fixed_volume_is_set, hsi.fixed_volume, false);
}

void HPWH::from(const hpwh_data_model::rsintegratedwaterheater::RSINTEGRATEDWATERHEATER& rswh)
{
    description.from(rswh);
    productInformation.from(rswh);
    rating10CFR430.from(rswh);

    auto& performance = rswh.performance;

    auto& rstank = performance.tank;
    tank->from(rstank);

    auto& configurations = performance.heat_source_configurations;
    std::size_t num_heat_sources = configurations.size();

    heatSources.clear();
    heatSources.reserve(num_heat_sources);

    std::unordered_map<std::string, std::size_t> heat_source_lookup;
    heat_source_lookup.reserve(num_heat_sources);

    // heat-source priority is retained from the entry order
    for (std::size_t iHeatSource = 0; iHeatSource < num_heat_sources; ++iHeatSource)
    {
        auto& config = configurations[iHeatSource];
        switch (config.heat_source_type)
        {
        case hpwh_data_model::heat_source_configuration::HeatSourceType::CONDENSER:
        {
            auto condenser = std::make_shared<Condenser>(this, get_courier(), config.id);
            heatSources.push_back(condenser);

            auto cond_ptr = reinterpret_cast<
                hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE*>(
                config.heat_source.get());
            condenser->from(*cond_ptr);
            break;
        }
        case hpwh_data_model::heat_source_configuration::HeatSourceType::RESISTANCE:
        {
            heatSources.push_back(std::make_shared<Resistance>(this, get_courier(), config.id));
            heatSources[iHeatSource]->from(config.heat_source);
            break;
        }
        default:
        {
        }
        }

        heatSources[iHeatSource]->from(config);
        heat_source_lookup[config.id] = iHeatSource;
    }

    std::string sPrimaryHeatSource_id = {};
    checkFrom(sPrimaryHeatSource_id,
              performance.primary_heat_source_id_is_set,
              performance.primary_heat_source_id,
              sPrimaryHeatSource_id);
    for (std::size_t iHeatSource = 0; iHeatSource < num_heat_sources; ++iHeatSource)
    {
        heatSources[iHeatSource]->isVIP = (heatSources[iHeatSource]->name == sPrimaryHeatSource_id);
    }

    // set associations between heat sources
    for (std::size_t iHeatSource = 0; iHeatSource < num_heat_sources; ++iHeatSource)
    {
        auto& configuration = configurations[iHeatSource];

        if (configuration.backup_heat_source_id_is_set)
        {
            auto iBackup = heat_source_lookup[configuration.backup_heat_source_id];
            heatSources[iHeatSource]->backupHeatSource = heatSources[iBackup].get();
        }

        if (configuration.followed_by_heat_source_id_is_set)
        {
            auto iFollowedBy = heat_source_lookup[configuration.followed_by_heat_source_id];
            heatSources[iHeatSource]->followedByHeatSource = heatSources[iFollowedBy].get();
        }

        if (configuration.companion_heat_source_id_is_set)
        {
            auto iCompanion = heat_source_lookup[configuration.companion_heat_source_id];
            heatSources[iHeatSource]->companionHeatSource = heatSources[iCompanion].get();
        }
    }

    // calculate oft-used derived values
    calcDerivedValues();
    checkInputs();
    resetTankToSetpoint();
    isHeating = false;
    for (auto i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i]->isOn)
        {
            isHeating = true;
        }
    }
}

void HPWH::from(
    const hpwh_data_model::central_water_heating_system::CentralWaterHeatingSystem& cwhs)
{
    auto& rstank = cwhs.tank;
    tank->from(rstank);

    auto& configurations = cwhs.heat_source_configurations;
    std::size_t num_heat_sources = configurations.size();

    heatSources.clear();
    heatSources.reserve(num_heat_sources);

    std::unordered_map<std::string, std::size_t> heat_source_lookup;
    heat_source_lookup.reserve(num_heat_sources);

    // heat-source priority is retained from the entry order
    for (std::size_t iHeatSource = 0; iHeatSource < num_heat_sources; ++iHeatSource)
    {
        auto& config = configurations[iHeatSource];
        switch (config.heat_source_type)
        {
        case hpwh_data_model::heat_source_configuration::HeatSourceType::AIRTOWATERHEATPUMP:
        {
            auto condenser = std::make_shared<Condenser>(this, get_courier(), config.id);
            heatSources.push_back(condenser);

            double ratio;
            checkFrom(ratio, cwhs.external_inlet_height_is_set, cwhs.external_inlet_height, 1.);
            condenser->externalInletHeight = static_cast<int>(ratio * (tank->getNumNodes() - 1));

            checkFrom(ratio, cwhs.external_outlet_height_is_set, cwhs.external_outlet_height, 1.);
            condenser->externalOutletHeight = static_cast<int>(ratio * (tank->getNumNodes() - 1));

            if (cwhs.fixed_flow_rate_is_set)
            {
                condenser->isMultipass = true;
                condenser->mpFlowRate_LPS = 1000. * cwhs.fixed_flow_rate;
            }
            else
                condenser->isMultipass = false;

            if (cwhs.secondary_heat_exchanger_is_set)
            {
                auto& shs = cwhs.secondary_heat_exchanger;
                condenser->secondaryHeatExchanger = {shs.cold_side_temperature_offset,
                                                     shs.hot_side_temperature_offset,
                                                     shs.extra_pump_power};
            }
            auto ptr =
                reinterpret_cast<hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP*>(
                    config.heat_source.get());
            condenser->from(*ptr);
            break;
        }
        case hpwh_data_model::heat_source_configuration::HeatSourceType::RESISTANCE:
        {
            heatSources.push_back(std::make_shared<Resistance>(this, get_courier(), config.id));
            heatSources[iHeatSource]->from(config.heat_source);
            break;
        }
        default:
        {
        }
        }

        heatSources[iHeatSource]->from(config);
        heat_source_lookup[config.id] = iHeatSource;
    }

    std::string sPrimaryHeatSource_id = {};
    checkFrom(sPrimaryHeatSource_id,
              cwhs.primary_heat_source_id_is_set,
              cwhs.primary_heat_source_id,
              sPrimaryHeatSource_id);
    for (std::size_t iHeatSource = 0; iHeatSource < num_heat_sources; ++iHeatSource)
    {
        heatSources[iHeatSource]->isVIP = (heatSources[iHeatSource]->name == sPrimaryHeatSource_id);
    }

    // set associations between heat sources
    for (std::size_t iHeatSource = 0; iHeatSource < num_heat_sources; ++iHeatSource)
    {
        auto& configuration = configurations[iHeatSource];

        if (configuration.backup_heat_source_id_is_set)
        {
            auto iBackup = heat_source_lookup[configuration.backup_heat_source_id];
            heatSources[iHeatSource]->backupHeatSource = heatSources[iBackup].get();
        }

        if (configuration.followed_by_heat_source_id_is_set)
        {
            auto iFollowedBy = heat_source_lookup[configuration.followed_by_heat_source_id];
            heatSources[iHeatSource]->followedByHeatSource = heatSources[iFollowedBy].get();
        }

        if (configuration.companion_heat_source_id_is_set)
        {
            auto iCompanion = heat_source_lookup[configuration.companion_heat_source_id];
            heatSources[iHeatSource]->companionHeatSource = heatSources[iCompanion].get();
        }
    }

    // hard-code fix: two VIPs assigned in preset
    if ((MODELS_TamScalable_SP <= model) && (model <= MODELS_TamScalable_SP_Half))
        heatSources[0]->isVIP = heatSources[2]->isVIP = true;

    // calculate oft-used derived values
    calcDerivedValues();
    checkInputs();
    resetTankToSetpoint();
    isHeating = false;
    for (auto i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i]->isOn)
        {
            isHeating = true;
        }
    }
}

void HPWH::to(hpwh_data_model::hpwh_sim_input::HPWHSimInput& hsi) const
{
    generate_metadata<hpwh_data_model::hpwh_sim_input::Schema>(
        hsi,
        "HPWHSimInput",
        "https://github.com/bigladder/hpwh-data-model/blob/main/schema/HPWHSimInput.schema.yaml");

    checkTo(doTempDepression, hsi.depresses_temperature_is_set, hsi.depresses_temperature);

    checkTo(tank->getNumNodes(), hsi.number_of_nodes_is_set, hsi.number_of_nodes);

    checkTo(tank->volumeFixed, hsi.fixed_volume_is_set, hsi.fixed_volume);

    checkTo(C_TO_K(setpoint_C), hsi.standard_setpoint_is_set, hsi.standard_setpoint);

    if (hasACompressor() && (getCompressorCoilConfig() == Condenser::COIL_CONFIG::CONFIG_EXTERNAL))
    {
        checkTo(hpwh_data_model::hpwh_sim_input::HPWHSystemType::CENTRAL,
                hsi.system_type_is_set,
                hsi.system_type);
        to(hsi.central_system);
        hsi.central_system_is_set = true;
        hsi.integrated_system_is_set = false;
    }
    else
    {
        checkTo(hpwh_data_model::hpwh_sim_input::HPWHSystemType::INTEGRATED,
                hsi.system_type_is_set,
                hsi.system_type);
        to(hsi.integrated_system);
        hsi.integrated_system_is_set = true;
        hsi.central_system_is_set = false;
    }
}

void HPWH::to(hpwh_data_model::rsintegratedwaterheater::RSINTEGRATEDWATERHEATER& rswh) const
{
    generate_metadata<hpwh_data_model::rsintegratedwaterheater::Schema>(
        rswh,
        "RSINTEGRATEDWATERHEATER",
        "https://github.com/bigladder/hpwh-data-model/blob/main/schema/"
        "RSINTEGRATEDWATERHEATER.schema.yaml");

    description.to(rswh);
    productInformation.to(rswh);
    rating10CFR430.to(rswh);

    auto& performance = rswh.performance;

    auto& rstank = performance.tank;
    tank->to(rstank);

    // heat-source priority is retained from the entry order
    auto& configurations = performance.heat_source_configurations;
    configurations.resize(getNumHeatSources());
    bool hasPrimaryHeatSource = false;
    std::string sPrimaryHeatSource_id = {};
    for (int iHeatSource = 0; iHeatSource < getNumHeatSources(); ++iHeatSource)
    {
        auto& configuration = configurations[iHeatSource];
        heatSources[iHeatSource]->to(configuration);
        if (heatSources[iHeatSource]->isVIP)
        {
            hasPrimaryHeatSource = true;
            sPrimaryHeatSource_id = heatSources[iHeatSource]->name;
        }
    }
    checkTo(sPrimaryHeatSource_id,
            performance.primary_heat_source_id_is_set,
            performance.primary_heat_source_id,
            hasPrimaryHeatSource);
}

void HPWH::to(hpwh_data_model::central_water_heating_system::CentralWaterHeatingSystem& cwhs) const
{
    tank->to(cwhs.tank);

    // heat-source priority is retained from the entry order
    auto& configurations = cwhs.heat_source_configurations;
    configurations.resize(getNumHeatSources());
    bool hasPrimaryHeatSource = false;
    std::string sPrimaryHeatSource_id = {};
    for (int iHeatSource = 0; iHeatSource < getNumHeatSources(); ++iHeatSource)
    {
        auto& configuration = configurations[iHeatSource];

        heatSources[iHeatSource]->to(configuration);
        if (heatSources[iHeatSource]->isVIP)
        {
            hasPrimaryHeatSource = true;
            sPrimaryHeatSource_id = heatSources[iHeatSource]->name;
        }
    }
    checkTo(sPrimaryHeatSource_id,
            cwhs.primary_heat_source_id_is_set,
            cwhs.primary_heat_source_id,
            hasPrimaryHeatSource);

    auto condenser = reinterpret_cast<Condenser*>(heatSources[getCompressorIndex()].get());

    checkTo(static_cast<double>(condenser->externalInletHeight) / tank->getNumNodes(),
            cwhs.external_inlet_height_is_set,
            cwhs.external_inlet_height);

    checkTo(static_cast<double>(condenser->externalOutletHeight) / tank->getNumNodes(),
            cwhs.external_outlet_height_is_set,
            cwhs.external_outlet_height);

    hpwh_data_model::central_water_heating_system::ControlType ct =
        (condenser->isMultipass)
            ? hpwh_data_model::central_water_heating_system::ControlType::FIXED_FLOW_RATE
            : hpwh_data_model::central_water_heating_system::ControlType::FIXED_OUTLET_TEMPERATURE;

    checkTo(ct, cwhs.control_type_is_set, cwhs.control_type);

    condenser->isMultipass =
        (ct == hpwh_data_model::central_water_heating_system::ControlType::FIXED_FLOW_RATE);
    checkTo(condenser->mpFlowRate_LPS / 1000.,
            cwhs.fixed_flow_rate_is_set,
            cwhs.fixed_flow_rate,
            condenser->isMultipass);

    if ((cwhs.secondary_heat_exchanger_is_set =
             (condenser->secondaryHeatExchanger.extraPumpPower_W > 0.)))
    {
        auto& she = cwhs.secondary_heat_exchanger;
        checkTo(condenser->secondaryHeatExchanger.coldSideTemperatureOffset_dC,
                she.cold_side_temperature_offset_is_set,
                she.cold_side_temperature_offset);
        checkTo(condenser->secondaryHeatExchanger.hotSideTemperatureOffset_dC,
                she.hot_side_temperature_offset_is_set,
                she.hot_side_temperature_offset);
        checkTo(condenser->secondaryHeatExchanger.extraPumpPower_W,
                she.extra_pump_power_is_set,
                she.extra_pump_power);
    }
}

//-----------------------------------------------------------------------------
///	@brief	convert condenser performance map to grid representation
//-----------------------------------------------------------------------------
void HPWH::convertMapToGrid()
{
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }

    auto condenser = reinterpret_cast<Condenser*>(heatSources[compressorIndex].get());
    if (!condenser->useBtwxtGrid)
    {
        condenser->convertMapToGrid();
    }
}
//-----------------------------------------------------------------------------
///	@brief	Performs a draw/heat cycle to prepare for test
///         Draw until heating begins, wait for recovery.
/// @note	see EERE-2019-BT-TP-0032-0058, p. 40479 (5.2.4)
//-----------------------------------------------------------------------------
void HPWH::prepareForTest(const TestConfiguration& testConfiguration)
{
    // apply first-hour-rating criterion EERE-2019-BT-TP-0032-0058, p. 40479
    double flowRate_Lper_min = GAL_TO_L(3.);
    if (tank->getVolume_L() < GAL_TO_L(20.))
        flowRate_Lper_min = GAL_TO_L(1.5);

    const double inletT_C = testConfiguration.inletT_C;
    const double ambientT_C = testConfiguration.ambientT_C;
    const double externalT_C = testConfiguration.externalT_C;

    DRMODES drMode = DR_ALLOW;
    bool isDrawing = false;
    bool done = false;
    int step = 0;
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

        case 2: // wait for heat to turn off
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

        runOneStep(inletT_C,                // inlet water temperature (C)
                   incrementalDrawVolume_L, // draw volume (L)
                   ambientT_C,              // ambient Temp (C)
                   externalT_C,             // external Temp (C)
                   drMode,                  // DDR Status
                   0.,                      // inlet-2 volume (L)
                   inletT_C,                // inlet-2 Temp (C)
                   NULL);                   // no extra heat
    }
}

//-----------------------------------------------------------------------------
///	@brief	Find first-hour rating designation for 24-hr test
/// @note	see EERE-2019-BT-TP-0032-0058, p. 40479 (5.3.3)
/// @param[out] firstHourRating	    contains first-hour rating designation
//-----------------------------------------------------------------------------
HPWH::FirstHourRating HPWH::findFirstHourRating()
{
    double flowRate_Lper_min = GAL_TO_L(3.);
    if (tank->getVolume_L() < GAL_TO_L(20.))
        flowRate_Lper_min = GAL_TO_L(1.5);

    const double inletT_C = testConfiguration_UEF.inletT_C;
    const double ambientT_C = testConfiguration_UEF.ambientT_C;
    const double externalT_C = testConfiguration_UEF.externalT_C;

    // change setpoint to DOE value, or highest available
    // see EERE-2019-BT-TP-0032-0058 (p. 40475)
    double maxAllowedSetpointT_C;
    std::string why;
    if (isNewSetpointPossible(HPWH::testSetpointT_C, maxAllowedSetpointT_C, why, UNITS_C))
    {
        setSetpoint(HPWH::testSetpointT_C);
    }
    else
    {
        if (isNewSetpointPossible(maxAllowedSetpointT_C, maxAllowedSetpointT_C, why, UNITS_C))
            setSetpoint(maxAllowedSetpointT_C);
    }

    double tankT_C = getAverageTankTemp_C();
    double maxTankT_C = tankT_C;
    double maxOutletT_C = 0.;

    DRMODES drMode = DR_ALLOW;
    double drawVolume_L = 0.;

    FirstHourRating firstHourRating;
    firstHourRating.drawVolume_L = 0.;

    double sumOutletVolumeT_LC = 0.;
    double sumOutletVolume_L = 0.;

    double averageOutletT_C = 0.;
    double minOutletT_C = 0.;
    double previousAverageOutletT_C = 0.;
    double prevMinOutletT_C = 0.;

    bool isDrawing = false;
    bool done = false;
    int step = 0;

    prepareForTest(testConfiguration_UEF);

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

        runOneStep(inletT_C,                // inlet water temperature (C)
                   incrementalDrawVolume_L, // draw volume (L)
                   ambientT_C,              // ambient Temp (C)
                   externalT_C,             // external Temp (C)
                   drMode,                  // DDR Status
                   0.,                      // inlet-2 volume (L)
                   inletT_C,                // inlet-2 Temp (C)
                   NULL);                   // no extra heat

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
                averageOutletT_C = sumOutletVolumeT_LC / sumOutletVolume_L;
                minOutletT_C = tank->getOutletT_C();
                if (elapsedTime_min >= 60)
                {
                    double fac = 1;
                    if (!firstDraw)
                    {
                        fac = (averageOutletT_C - prevMinOutletT_C) /
                              (previousAverageOutletT_C - prevMinOutletT_C);
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
                    previousAverageOutletT_C = averageOutletT_C;
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
        firstHourRating.designation = FirstHourRating::Designation::VerySmall;
    }
    else if (firstHourRating.drawVolume_L < GAL_TO_L(51.))
    {
        firstHourRating.designation = FirstHourRating::Designation::Low;
    }
    else if (firstHourRating.drawVolume_L < GAL_TO_L(75.))
    {
        firstHourRating.designation = FirstHourRating::Designation::Medium;
    }
    else
    {
        firstHourRating.designation = FirstHourRating::Designation::High;
    }
    return firstHourRating;
}

//-----------------------------------------------------------------------------
///	@brief	Performs standard 24-hr test
/// @note	see https://www.regulations.gov/document/EERE-2019-BT-TP-0032-0058 (5.4.2)
/// @param[in/out] testOptions          reporting and configuration
/// @param[out] testSummary	            contains test metrics on output
//-----------------------------------------------------------------------------
HPWH::TestSummary HPWH::run24hrTest(TestConfiguration testConfiguration,
                                    FirstHourRating::Designation designation,
                                    bool saveOutput)
{
    // select the first draw cluster size and pattern
    auto firstDrawClusterSize = firstDrawClusterSizes[designation];
    DrawPattern& drawPattern = drawPatterns[designation];

    const double inletT_C = testConfiguration.inletT_C;
    const double ambientT_C = testConfiguration.ambientT_C;
    const double externalT_C = testConfiguration.externalT_C;

    DRMODES drMode = DR_ALLOW;

    // change setpoint to DOE value, or highest available
    // see EERE-2019-BT-TP-0032-0058 (p. 40475)
    double maxAllowedSetpointT_C;
    std::string why;
    if (isNewSetpointPossible(HPWH::testSetpointT_C, maxAllowedSetpointT_C, why, UNITS_C))
    {
        setSetpoint(HPWH::testSetpointT_C);
    }
    else
    {
        if (isNewSetpointPossible(maxAllowedSetpointT_C, maxAllowedSetpointT_C, why, UNITS_C))
            setSetpoint(maxAllowedSetpointT_C);
    }

    prepareForTest(testConfiguration);

    const int nTestTCouples = 6;
    TestSummary testSummary;
    testSummary.testDataSet = {};

    std::string strPreamble;
    std::string sHeader = "minutes,Ta,Tsetpoint,inletT,draw,";

    // idle for 1 hr
    int preTime_min = 0;
    bool heatersAreOn = false;
    while ((preTime_min < 60) || heatersAreOn)
    {
        runOneStep(inletT_C,    // inlet water temperature (C)
                   0,           // draw volume (L)
                   ambientT_C,  // ambient Temp (C)
                   externalT_C, // external Temp (C)
                   drMode,      // DDR Status
                   0.,          // inlet-2 volume (L)
                   inletT_C,    // inlet-2 Temp (C)
                   NULL);       // no extra heat

        heatersAreOn = false;
        for (auto& heatSource : heatSources)
        {
            heatersAreOn |= heatSource->isEngaged();
        }

        if (saveOutput)
        {
            TestData testData;
            testData.time_min = preTime_min;
            testData.ambientT_C = ambientT_C;
            testData.setpointT_C = getSetpoint();
            testData.inletT_C = inletT_C;
            testData.drawVolume_L = 0.;
            testData.drMode = drMode;

            for (int iHS = 0; iHS < getNumHeatSources(); ++iHS)
            {
                testData.h_srcIn_kWh.push_back(getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KWH));
                testData.h_srcOut_kWh.push_back(getNthHeatSourceEnergyOutput(iHS, HPWH::UNITS_KWH));
            }

            for (int iTC = 0; iTC < nTestTCouples; ++iTC)
            {
                testData.thermocoupleT_C.push_back(
                    getNthSimTcouple(iTC + 1, nTestTCouples, UNITS_C));
            }
            testData.outletT_C = 0.;

            testSummary.testDataSet.push_back(testData);
        }

        ++preTime_min;
    }

    // correct time to start test at 0 min
    for (auto& testData : testSummary.testDataSet)
    {
        testData.time_min -= preTime_min;
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
                stepDrawVolume_L = draw.flowRate_L_per_min * (1.);
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
        runOneStep(inletT_C,         // inlet water temperature (C)
                   stepDrawVolume_L, // draw volume (L)
                   ambientT_C,       // ambient Temp (C)
                   externalT_C,      // external Temp (C)
                   drMode,           // DDR Status
                   0.,               // inlet-2 volume (L)
                   inletT_C,         // inlet-2 Temp (C)
                   NULL);            // no extra heat

        if (saveOutput)
        {
            TestData testData;
            testData.time_min = runTime_min;
            testData.ambientT_C = ambientT_C;
            testData.setpointT_C = getSetpoint();
            testData.inletT_C = inletT_C;
            testData.drawVolume_L = stepDrawVolume_L;
            testData.drMode = drMode;

            for (int iHS = 0; iHS < getNumHeatSources(); ++iHS)
            {
                testData.h_srcIn_kWh.push_back(getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KWH));
                testData.h_srcOut_kWh.push_back(getNthHeatSourceEnergyOutput(iHS, HPWH::UNITS_KWH));
            }

            for (int iTC = 0; iTC < nTestTCouples; ++iTC)
            {
                testData.thermocoupleT_C.push_back(
                    getNthSimTcouple(iTC + 1, nTestTCouples, UNITS_C));
            }
            testData.outletT_C = tank->getOutletT_C();

            testSummary.testDataSet.push_back(testData);
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

    testSummary.averageOutletT_C = sumOutletVolumeT_LC / testSummary.removedVolume_L;
    testSummary.averageInletT_C = sumInletVolumeT_LC / testSummary.removedVolume_L;

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
    testSummary.EF = 0.;
    if (testSummary.modifiedConsumedWaterHeatingEnergy_kJ > 0.)
    {
        testSummary.EF =
            standardDeliveredEnergy_kJ / testSummary.modifiedConsumedWaterHeatingEnergy_kJ;
    }

    // find the "Annual Energy Consumption" (6.4.5)
    testSummary.annualConsumedEnergy_kJ = 0.;
    if (testSummary.EF > 0.)
    {
        constexpr double days_per_year = 365.;
        const double nominalDifferenceT_C = F_TO_C(67.);
        testSummary.annualConsumedEnergy_kJ =
            days_per_year * removedHeatCapacity_kJperC * nominalDifferenceT_C / testSummary.EF;
    }

    // find the "Annual Electrical Energy Consumption" (6.4.6)
    testSummary.annualConsumedElectricalEnergy_kJ = 0.;
    if (testSummary.usedEnergy_kJ > 0.)
    {
        testSummary.annualConsumedElectricalEnergy_kJ =
            (testSummary.usedElectricalEnergy_kJ / testSummary.usedEnergy_kJ) *
            testSummary.annualConsumedEnergy_kJ;
    }

    return testSummary;
}

//-----------------------------------------------------------------------------
///	@brief	first-hour rating as a verbose string
//-----------------------------------------------------------------------------
std::string HPWH::FirstHourRating::report()
{
    std::string results = "\tFirst-Hour Rating:\n";

    results.append(fmt::format("\t\tVolume Drawn (L): {:g}\n", drawVolume_L));

    results.append(fmt::format("\t\tDesignation: {}\n", DesignationMap[designation]));

    return results;
}

//-----------------------------------------------------------------------------
///	@brief	24-hr test summary as a verbose string
//-----------------------------------------------------------------------------
std::string HPWH::TestSummary::report()
{
    std::string results = "\t24-Hour Test Results:\n";
    if (!qualifies)
    {
        results.append("\t\tDoes not qualify as consumer water heater.\n");
    }

    results.append(fmt::format("\t\tRecovery Efficiency: {:g}\n", recoveryEfficiency));

    results.append(fmt::format("\t\tStandby Loss Coefficient (kJ/h degC): {:g}\n",
                               standbyLossCoefficient_kJperhC));

    results.append(fmt::format("\t\tEF: {:g}\n", EF));

    results.append(fmt::format("\t\tAverage Inlet Temperature (degC): {:g}\n", averageInletT_C));

    results.append(fmt::format("\t\tAverage Outlet Temperature (degC): {:g}\n", averageOutletT_C));

    results.append(fmt::format("\t\tTotal Volume Drawn (L): {:g}\n", removedVolume_L));

    results.append(fmt::format("\t\tDaily Water-Heating Energy Consumption (kWh): {:g}\n",
                               KJ_TO_KWH(waterHeatingEnergy_kJ)));

    results.append(fmt::format("\t\tAdjusted Daily Water-Heating Energy Consumption (kWh): {:g}\n",
                               KJ_TO_KWH(adjustedConsumedWaterHeatingEnergy_kJ)));

    results.append(fmt::format("\t\tModified Daily Water-Heating Energy Consumption (kWh): {:g}\n",
                               KJ_TO_KWH(modifiedConsumedWaterHeatingEnergy_kJ)));

    results.append("\tAnnual Values:\n");
    results.append(fmt::format("\t\tAnnual Electrical Energy Consumption (kWh): {:g}\n",
                               KJ_TO_KWH(annualConsumedElectricalEnergy_kJ)));

    results.append(fmt::format("\t\tAnnual Energy Consumption (kWh): {:g}\n",
                               KJ_TO_KWH(annualConsumedEnergy_kJ)));

    return results;
}

//-----------------------------------------------------------------------------
///	@brief	    Make a generic model at specified configuration by varying C
///             COP coefficients of the current model
/// @returns	Test summary using UEF configuration
//-----------------------------------------------------------------------------
HPWH::TestSummary HPWH::makeGenericEF(double targetEF,
                                      TestConfiguration testConfiguration,
                                      FirstHourRating::Designation designation)
{
    // set up metrics
    std::vector<std::shared_ptr<Fitter::Metric>> metrics = {};
    auto ef_metric = std::make_shared<HPWH::Fitter::EF_Metric>(
        targetEF, testConfiguration, designation, get_courier(), this);
    metrics.push_back(ef_metric);

    auto compressor = reinterpret_cast<Condenser*>(heatSources[compressorIndex].get());

    int i_ambientT = compressor->getAmbientT_index(testConfiguration.ambientT_C);

    // set up parameters
    std::vector<std::shared_ptr<Fitter::Parameter>> parameters;
    auto copCoefficient0 =
        std::make_shared<HPWH::Fitter::COP_Coefficient>(i_ambientT, 0, get_courier(), this);
    parameters.push_back(copCoefficient0);

    // auto copCoefficient1 =
    //     std::make_shared<HPWH::Fitter::COP_Coefficient>(i_ambientT, 1, get_courier(), this);
    // parameters.push_back(copCoefficient1);

    Fitter fitter(metrics, parameters, get_courier());
    fitter.fit();

    auto performance1 =
        compressor->getPerformance(testConfiguration.ambientT_C, compressor->maxSetpoint_C);

    if (performance1.cop < 0.)
        send_error("COP is negative at maximum condenser temperature.");

    /// low condenserT_C
    auto performance0 = compressor->getPerformance(testConfiguration.ambientT_C, 0.);

    if (performance0.cop < performance1.cop)
        send_error("COP slope is positive.");

    return ef_metric->getTestSummary();
}

//-----------------------------------------------------------------------------
///	@brief	    Modify *this hpwh using UEF configuration, then adjust E50 and E95
///             coefficients
/// @returns	Test summary using UEF configuration
//-----------------------------------------------------------------------------
HPWH::TestSummary HPWH::makeGenericUEF(double targetUEF,
                                       HPWH::FirstHourRating::Designation designation)
{
    auto compressor = reinterpret_cast<Condenser*>(heatSources[compressorIndex].get());

    // pick the nearest temperature index
    int i_ambientT = compressor->getAmbientT_index(testConfiguration_UEF.ambientT_C);

    auto originalCoefficient = compressor->performanceMap[i_ambientT].COP_coeffs[0];
    auto testSummary = makeGenericEF(targetUEF, testConfiguration_UEF, designation);

    double dCOP_Coefficient =
        compressor->performanceMap[i_ambientT].COP_coeffs[0] - originalCoefficient;

    int nPerfPts = static_cast<int>(compressor->performanceMap.size());
    for (int i = 0; i < nPerfPts; ++i)
    {
        if (i != i_ambientT)
            compressor->performanceMap[i].COP_coeffs[0] += dCOP_Coefficient;
    }

    return testSummary;
}

//-----------------------------------------------------------------------------
///	@brief	Modify *this hpwh to meet at E50, UEF, and E95 configurations
//-----------------------------------------------------------------------------
void HPWH::makeGenericE50_UEF_E95(double targetE50,
                                  double targetUEF,
                                  double targetE95,
                                  FirstHourRating::Designation designation)
{
    // return test summaries unused
    makeGenericEF(targetE50, testConfiguration_E50, designation);
    makeGenericEF(targetUEF, testConfiguration_UEF, designation);
    makeGenericEF(targetE95, testConfiguration_E95, designation);
}
