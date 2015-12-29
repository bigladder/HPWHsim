#include "HPWH.hh"


void HPWH::HeatSource::addHeat(double externalT_C, double minutesPerStep) {
  double input_BTUperHr, cap_BTUperHr, cop, captmp_kJ;
  int i;
  double *heatDistribution;

  // Reset the runtime of the Heat Source
  runtime_min = 0.0;

  // Could also have heatDistribution be part of the HeatSource class, rather than creating and destroying a vector every time we add heat
  heatDistribution = new double [hpwh->numNodes];

  // calculate capacity btu/hr? Just btu??
  getCapacity(externalT_C, &input_BTUperHr, &cap_BTUperHr, &cop);

  if((location == 1) | (location == 3)) {
    // Either the heat source is within the tank or wrapped around it
    calcHeatDist(heatDistribution);
    for(i = 0; i < hpwh->numNodes; i++){
      captmp_kJ = BTU_TO_KJ(cap_BTUperHr * minutesPerStep / 60.0 * heatDistribution[i]);
      runtime_min += addHeatAboveNode(captmp_kJ, i, minutesPerStep);
    }
  } else {
    // Else the heat source is external. Sanden thingy
    runtime_min = addHeatExternal(cap_BTUperHr, minutesPerStep);
  }

  // Write the input & output energy
  energyInput_kWh = BTU_TO_KWH(input_BTUperHr * runtime_min / 60.0);
  energyOutput_kWh = BTU_TO_KWH(cap_BTUperHr * runtime_min / 60.0);

  delete[] heatDistribution;
}



// Add heat from a source outside of the tank. Assume the condensity is where the water is drawn from
// and hot water is put at the top of the tank.
double HPWH::HeatSource::addHeatExternal(double cap_BTUperHr, double minutesPerStep) {
  double nodeHeat_kJ, heatingCapacity_kJ, remainingCapacity_kJ, deltaT_C, nodeFrac;
  int n;
  double heatingCutoff_C = 20; // When to turn off the compressor. Should be a property of the HeatSource?

  //how much heat is available this timestep
  heatingCapacity_kJ = BTU_TO_KJ(cap_BTUperHr * (minutesPerStep / 60.0));
  remainingCapacity_kJ = heatingCapacity_kJ;
	
  //if there's still remaining capacity and you haven't heated to "setpoint", keep heating
  while(remainingCapacity_kJ > 0 && getCondenserTemp() <= hpwh->setpoint_C - heatingCutoff_C) {
    // calculate what percentage of the bottom node can be heated to setpoint with amount of heat available this timestep
    deltaT_C = hpwh->setpoint_C - getCondenserTemp();
    nodeHeat_kJ = hpwh->tankVolume_L * DENSITYWATER_kgperL * CPWATER_kJperkgC * deltaT_C;
    nodeFrac = remainingCapacity_kJ / nodeHeat_kJ;

  //if more than one, round down to 1 and subtract that amount of heat energy from the capacity
    if(nodeFrac > 1){
      nodeFrac = 1;
      remainingCapacity_kJ -= nodeHeat_kJ;
    }
    //substract the amount of heat used - a full node's heat if nodeFrac == 1, otherwise just the fraction available 
    //this should make heatingCapacity == 0  if nodeFrac < 1
    else{
      remainingCapacity_kJ = 0;
    }
	
    //move all nodes down, mixing if less than a full node
    for(n = 0; n < hpwh->numNodes - 1; n++) {
      hpwh->tankTemps_C[n] = hpwh->tankTemps_C[n] * (1 - nodeFrac) + hpwh->tankTemps_C[n + 1] * nodeFrac;
    }
    //add water to top node, heated to setpoint
    hpwh->tankTemps_C[hpwh->numNodes - 1] = hpwh->tankTemps_C[hpwh->numNodes - 1] * (1 - nodeFrac) + hpwh->setpoint_C * nodeFrac;
		
  }
		
  //This is runtime - is equal to timestep if compressor ran the whole time
  return (1 - remainingCapacity_kJ / heatingCapacity_kJ) * minutesPerStep;
}

