The water draws and standby losses have already been taken care by the time addHeat is called.

The step running function has this call:
int runOneStep(double inletT_C, double drawVolume_L, 
					double ambientT_C, double externalT_C,
					double DRstatus, double minutesPerStep);
					
The variables passed in to it are available to you, but probably the only ones you 
need I have put in the addHeat declaration:

void HPWH::Element::addHeat(double externalT_C, double minutesPerStep);

externalT_C is the temperature to be used by the heat source when calculating
COP/Input power curves.  minutesPerStep is the number of minutes that this one
step is simulating - should be 1 in most cases, but it's a variable for versatility.


Inside this function, variables from the HPWH class are accessed by "hpwh->variableName"
e.g.
hpwh->numNodes
hpwh->tankTemps_C[i]


Variables from the Element class are accessed just by name, e.g.:
COP_T1_constant
condensity[i]


class definitions are available in HPWH.hh
If you need any variables not listed in the class definition, just make a name 
and make a note of it and I will add it in.

*Note:  condensity should have 12 nodes

I'm expecting that the capacity will be calculated inside the addHeat function 
and not passed in, as was done previously.

Outputs from addHeat, such as the runtime, and the amount of heat input and output, are
stored in variables, i.e.:
	double runtime_min;
	double energyInput_kWh;
	double energyOutput_kWh;

units to be used are part of variable name




