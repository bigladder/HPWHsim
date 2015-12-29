#include "HPWH.hh"

// A few helper functions
double expitFunc(double x, double offset);
void normalize(double *Z, int n);

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
      runtime_min += addHeatOneNode(captmp_kJ, i, minutesPerStep);
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



void HPWH::HeatSource::getCapacity(double externalT_C, double *input_BTUperHr, double *cap_BTUperHr, double *cop) {
  double COP_T1, COP_T2;    			   //cop at ambient temperatures T1 and T2
  double inputPower_T1_Watts = 0, inputPower_T2_Watts = 0; //input power at ambient temperatures T1 and T2	
  double condenserTemp_C = 0, externalT_F, condenserTemp_F;

  // Calculate the current water temp at the "condenser"
  condenserTemp_C = getCondenserTemp();
  // Convert Celsius to Fahrenheit for the curve fits
  condenserTemp_F = C_TO_F(condenserTemp_C);
  externalT_F = C_TO_F(externalT_C);

  // Calculate COP and Input Power at each of the two reference temepratures
  COP_T1 = COP_T1_constant;
  COP_T1 += COP_T1_linear * condenserTemp_F ;
  COP_T1 += COP_T1_quadratic * condenserTemp_F * condenserTemp_F;

  COP_T2 = COP_T2_constant;
  COP_T2 += COP_T2_linear * condenserTemp_F;
  COP_T2 += COP_T2_quadratic * condenserTemp_F * condenserTemp_F;

  inputPower_T1_Watts = inputPower_T1_constant;
  inputPower_T1_Watts += inputPower_T1_linear * condenserTemp_F;
  inputPower_T1_Watts += inputPower_T1_quadratic * condenserTemp_F * condenserTemp_F;

  inputPower_T2_Watts = inputPower_T2_constant;
  inputPower_T2_Watts += inputPower_T2_constant * condenserTemp_F;
  inputPower_T2_Watts += inputPower_T2_constant * condenserTemp_F * condenserTemp_F;


  // Interpolate to get COP and input power at the current ambient temperature
  *cop = COP_T1 + (externalT_F - T1) * ((COP_T2 - COP_T1) / (T2 - T1));
  *input_BTUperHr = (inputPower_T1_Watts + (externalT_F - T1) * ((inputPower_T2_Watts - inputPower_T1_Watts) / (T2 - T1))) * 3412;
  *cap_BTUperHr = (*cop) * (*input_BTUperHr);

/*
  //here is where the scaling for flow restriction goes
  //the input power doesn't change, we just scale the cop by a small percentage that is based on the ducted flow rate
  //the equation is a fit to three points, measured experimentally - 12 percent reduction at 150 cfm, 10 percent at 200, and 0 at 375
  //it's slightly adjust to be equal to 1 at 375
  if(hpwh->ductingType != 0){
    cop_interpolated *= 0.00056*hpwh->fanFlow + 0.79;
  }
*/
}

void HPWH::HeatSource::calcHeatDist(double *heatDistribution) {
  double condentropy, s; // Should probably have shrinkage (by way of condensity) be a property of the HeatSource class
  double alpha = 1;
  double beta = 2; // Mapping from condentropy to shrinkage
  double offset = 5;
  int i, k;

  // Calculate condentropy and ==> shrinkage. Again this could/should be a property of the HeatSource.
  condentropy = 0;
  for(i = 0; i < hpwh->numNodes; i++) {
    if(condensity[i] > 0) {
      condentropy -= condensity[i] * log(condensity[i]);
    }
  }
  s = alpha + condentropy * beta;

  // Populate the vector of heat distribution
  for(i = 0; i < hpwh->numNodes; i++) {
    if(i < lowestNode()) {
      heatDistribution[i] = 0;
    } else {
      if(location == 1) { // Inside the tank, no swoopiness required
	k = floor(i / 12 * hpwh->numNodes);
        heatDistribution[i] = condensity[k];
      } else if(location == 2) { // Wrapped around the tank, send through the logistic function
        heatDistribution[i] = expitFunc( (hpwh->tankTemps_C[i] - hpwh->tankTemps_C[lowestNode()]) / s , offset);
        heatDistribution[i] *= (hpwh->setpoint_C - hpwh->tankTemps_C[i]);
      }
    }
  }
  normalize(heatDistribution, hpwh->numNodes);

}


// Return the lowest node of the HeatSource
int HPWH::HeatSource::lowestNode() {
  int i, lowest;
  for(i = 0; i < hpwh->numNodes; i++) {
    if(condensity[i] > 0) {
      lowest = i;
      break;
    }
  }
  return lowest;
}

// Return the temperature at the condenser
double HPWH::HeatSource::getCondenserTemp() {
  double condenserTemp_C = 0.0;
  int i;

  for(i = 0; i < hpwh->numNodes; i++) {
    condenserTemp_C += condensity[i] * hpwh->tankTemps_C[i];
  }
  
  return condenserTemp_C;
}


double HPWH::HeatSource::addHeatOneNode(double cap_kJ, int node, double minutesPerStep) {
  double Q_kJ, deltaT_C, targetTemp_C, runtime = 0.0;
  int i = 0, j = 0, setPointNodeNum;
  
  
  // find the first node (from the bottom) that does not have the same temperature as the one above it
  // if they all have the same temp., use the top node, hpwh->numNodes-1
  setPointNodeNum = node;
  for(i = node; i < hpwh->numNodes-1; i++){
    if(hpwh->tankTemps_C[i] != hpwh->tankTemps_C[i+1]) {
      break;
    }
    else{
      setPointNodeNum = i+1;
    }
  }

  // maximum heat deliverable in this timestep
  while(cap_kJ > 0 && setPointNodeNum < hpwh->numNodes) {
    // if the whole tank is at the same temp, the target temp is the setpoint
    if(setPointNodeNum == (hpwh->numNodes-1)) {
      targetTemp_C = hpwh->setpoint_C;
    }
    //otherwise the target temp is the first non-equal-temp node
    else {
      targetTemp_C = hpwh->tankTemps_C[setPointNodeNum+1];
    }

    deltaT_C = targetTemp_C - hpwh->tankTemps_C[setPointNodeNum];
    
    //heat needed to bring all equal temp. nodes up to the temp of the next node. kJ
    Q_kJ = CPWATER_kJperkgC * hpwh->tankVolume_L * DENSITYWATER_kgperL * (setPointNodeNum+1 - node) * deltaT_C;

    //Running the rest of the time won't recover
    if(Q_kJ > cap_kJ){
      for(j = node; j <= setPointNodeNum; j++) {
        hpwh->tankTemps_C[j] += cap_kJ / CPWATER_kJperkgC / hpwh->tankVolume_L / DENSITYWATER_kgperL / (setPointNodeNum + 1 - node);
      }
      runtime_min = minutesPerStep;
      cap_kJ = 0;
    }
    //temp will recover by/before end of timestep
    else{
      for(j = node; j <= setPointNodeNum; j++){
        hpwh->tankTemps_C[j] = targetTemp_C;
      }
      ++setPointNodeNum;
      //calculate how long the element was on, in minutes
      //(should be less than minutesPerStep, typically 1...),
      //based on the capacity and the required amount of heat
      runtime_min += (Q_kJ / cap_kJ) * minutesPerStep;
      cap_kJ -= Q_kJ;
    }
  }
  
  return runtime_min;
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



double expitFunc(double x, double offset) {
  double val;
  val = 1 / (1 + exp(x - offset));
  return val;
}


void normalize(double *Z, int n) {
  double sum_tmp = 0.0;
  int i;

  for(i = 0; i < n; i++) {
    sum_tmp += Z[i];
  }

  for(i = 0; i < n; i++) {
    Z[i] /= sum_tmp;
  }
}



