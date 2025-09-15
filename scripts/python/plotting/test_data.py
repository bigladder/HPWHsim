import os
import sys
import pandas as pd  # type: ignore
import math
import numpy as np
from common import read_file, write_file, get_tank_volume

def call_csv(path, skip_rows):
    data = pd.read_csv(path, skiprows=skip_rows)
    df = pd.DataFrame(data)
    return df

def convert_values(df_column, from_units, to_units):
	converted_column = df_column.apply(lambda x: convert(x, from_units, to_units))
	return converted_column
	
def C_to_F(T_C):
	return 1.8 * T_C + 32

class EF_Bounds:
	def __init__(self):
		self.test_start_time = 0
		self.test_end_time = -1
		self.first_recovery_period_start_time = 0
		self.first_recovery_period_end_time = -1
		self.standby_period_start_time = 0
		self.standby_period_end_time = -1
		
class DataSet:
	def __init__(self, data_spec):
		self.model_id = data_spec['model_id']
		self.test_id = data_spec['test_id']
		self.ef_bounds = EF_Bounds()
		self.test_summary = {'tank_volume_L': data_spec['tank_volume_L'] if 'tank_volume_L' in data_spec else 173}
			
		self.variable_type = data_spec['type']
		self.filepath = data_spec['filepath']
		
		try:
			df = call_csv(self.filepath, 0)
			self.filepath = self.filepath
		except:
			df = {}
			return	
	
		NUMBER_OF_THERMOCOUPLES = 6
		self.columns = {}
		if self.variable_type == "Measured":
			self.columns = {
				"Time": "Time(min)",
				"Power Input": ["PowerIn(W)"],
	    	"Flow Rate": "FlowRate(gal/min)",
	    	"Temperature": {
					"Inlet": "InletT(C)",
					"Outlet": "OutletT(C)",			
					"Ambient": "AmbientT(C)",
					"Tank": 
						[f"TankT{number}(C)" for number in range(1, NUMBER_OF_THERMOCOUPLES + 1)]
					}
	     }	
		elif self.variable_type == "Simulated":
			self.columns = {
				"Time": "minutes",
				"Power Input": [f"h_src{i+1}In (Wh)" for i in range(100) if df.columns.isin([f"h_src{i+1}In (Wh)"]).any()],
	    	"Flow Rate": "draw",
	    	"Temperature": {
					"Inlet": "inletT",
					"Outlet": "toutlet (C)",			
					"Ambient": "Ta",	
					"Tank": 
						[f"tcouple{number} (C)" for number in reversed(range(1, NUMBER_OF_THERMOCOUPLES + 1))]
	     	}						         
	   	}
			
		try:
			df = call_csv(self.filepath, 0)
			self.filepath = self.filepath
		except:
			df = {}
			return	

		data = {}
		data['Time'] = list(df[self.columns['Time']]) #min

		data["Power Input"] = list(df[self.columns["Power Input"]].sum(axis=1))#W
		if self.variable_type == "Simulated": # convert simulated energy consumption (Wh) for every minute to power (W)
			data["Power Input"] = [60 * power for power in data["Power Input"]]

		data["Flow Rate"] = list(df[self.columns["Flow Rate"]])
		data["Tank Inlet Temperature"] = list(df[self.columns["Temperature"]["Inlet"]])
		data["Tank Outlet Temperature"] = list(df[self.columns["Temperature"]["Outlet"]])
		data["Ambient Temperature"] = list(df[self.columns["Temperature"]["Ambient"]])
		for i, tank in enumerate(self.columns["Temperature"]["Tank"]):
			data[f"Tank Temperature - #{i + 1}"] = list(df[tank])
		data["Tank Average Temperature"] = list(df[self.columns["Temperature"]["Tank"]].mean(axis=1))
		x_list = data['Time']
		y_list = data["Flow Rate"]
		prevNan = False		
		x_arr = []
		y_arr = []
		for i, y in enumerate(y_list):
			if np.isnan(y):
				if not prevNan:
					x_arr.append(x_list[i])
					y_arr.append(0)					
				prevNan = True	
			else:
				if prevNan:
					x_arr.append(x_list[i-1])
					y_arr.append(0)
				x_arr.append(x_list[i])
				y_arr.append(x_list[i])
				prevNan = False
			
		self.df = pd.DataFrame(data)
		
	def find_EF_bounds(self):	
		self.ef_bounds.test_start_time = self.df["Time"].iloc[0]
		self.ef_bounds.test_end_time = self.df["Time"].iloc[-1]
		
		hasFirstRecoveryPeriodStarted = True
		hasFirstRecoveryPeriodEnded = False
		self.ef_bounds.first_recovery_period_start_time = self.ef_bounds.test_start_time
		self.ef_bounds.first_recovery_period_end_time = self.ef_bounds.test_end_time
		
		hasStandbyPeriodStarted = False
		hasStandbyPeriodEnded = False
		self.ef_bounds.standby_period_start_time = self.ef_bounds.test_start_time
		self.ef_bounds.standby_period_end_time = self.ef_bounds.test_end_time
		has_heated = False
		has_drawn = False

		prev_input_energy_kJ = [0, 0, 0]
		for index in range(len(self.df)):
			t_min = self.df["Time"].iloc[index]
			draw_volume_L = self.df.loc[index, "Flow Rate"] * 3.78541 # gal->L
			input_energy_kJ = float(self.df.loc[index, "Power Input"]) * 60 / 1000	# W->kJ
			prev_input_energy_kJ = [input_energy_kJ, prev_input_energy_kJ[0], prev_input_energy_kJ[1]]
			if math.isnan(draw_volume_L):
				draw_volume_L = 0
				
			is_heating = input_energy_kJ > 0.5
			has_heated = has_heated or is_heating
			
			is_drawing = draw_volume_L > 0
			has_drawn = has_drawn or is_drawing
			
			self.ef_bounds.test_end_time = t_min
			if hasFirstRecoveryPeriodStarted:
				if hasFirstRecoveryPeriodEnded:
					if hasStandbyPeriodStarted:	
						if hasStandbyPeriodEnded:
							continue
						elif is_drawing or is_heating:	
							hasStandbyPeriodEnded = True		
							self.ef_bounds.standby_period_end_time = t_min - 1
							continue
					elif t_min >= self.ef_bounds.first_recovery_period_end_time + 5:
						if not(is_drawing) and not(is_heating):
							hasStandbyPeriodStarted = True
							self.ef_bounds.standby_period_start_time = t_min
				else:
					if has_drawn and not(is_drawing) and has_heated and not(is_heating):	
						hasFirstRecoveryPeriodEnded = True
						frac_min  = 1.0
						if prev_input_energy_kJ[2] > prev_input_energy_kJ[0]:
							frac_min = (prev_input_energy_kJ[1] - prev_input_energy_kJ[0]) / (prev_input_energy_kJ[2] - prev_input_energy_kJ[0])
						self.ef_bounds.first_recovery_period_end_time = t_min	- (2.0 - frac_min)

	def analyze(self):		
			self.find_EF_bounds()
					
			initialTankAvgT_C = self.df["Tank Average Temperature"].iloc[self.ef_bounds.test_start_time]
			finalTankAvgT_C = self.df["Tank Average Temperature"].iloc[self.ef_bounds.test_end_time]
			
			#draw_col = self.df["Flow Rate"][self.variable_type]
			#self.df["Power Input"]
			
			water_specific_heat_kJperkgC = 4.180
			water_density_kgperL = 0.995
			tank_heat_capacity_kJperC = water_specific_heat_kJperkgC * water_density_kgperL * self.test_summary['tank_volume_L']
			
			sumDrawOutletT = 0
			sumDrawInletT = 0
			sumDrawDiffT = 0
			sumDraw_L = 0
			sumDrawHeatCap_kJperC = 0
			sumInputEnergy_kJ = 0
			sumNoDrawTime_min = 0
			noDrawSumAmbientTTime = 0
			
			isFirstRecoveryPeriod = True
			recoveryTotalDraw_L = 0
			recoverySumDrawInletT = 0
			recoverySumDrawOutletT = 0
			recoveryUsedEnergy_kJ = 0
			recoveryStoredEnergy_kJ = 0
			recoveryDeliveredEnergy_kJ = 0
		
			isStandbyPeriod = False
			standbyUsedEnergy_kJ = 0
			standbySumTime_min = self.ef_bounds.standby_period_end_time - self.ef_bounds.standby_period_start_time + 1
			standbySumTimeTankT_minC = 0
			standbySumTimeAmbientT_minC = 0
			
			deliveredEnergy_kJ = 0
			standardSetpointT_C = 51.7
			standardInletT_C = 14.4
			standardAmbientT_C = 19.7
			
			noDrawAvgAmbientT_C = standardAmbientT_C
			maxTankAfterFirstRecoveryT_C = -10
			for index in range(len(self.df)):
				t_min = float(self.df["Time"].iloc[index])
				if t_min < self.ef_bounds.test_start_time or t_min > self.ef_bounds.test_end_time:
					continue
				
				ambientT_C = float(self.df["Ambient Temperature"].iloc[index])
				tankAvgT_C = float(self.df["Tank Average Temperature"].iloc[index])
				draw_volume_L = float(self.df["Flow Rate"].iloc[index]) * 3.78541 #gal->L
				input_energy_kJ = float(self.df["Power Input"].iloc[index]) * 60 / 1000 #W->kJ
				
				sumInputEnergy_kJ += input_energy_kJ
				if math.isnan(draw_volume_L):
					draw_volume_L = 0
				inletT_C = 0 if draw_volume_L == 0 else float(self.df["Tank Inlet Temperature"].iloc[index])
				outletT_C = 0 if draw_volume_L == 0 else float(self.df["Tank Outlet Temperature"].iloc[index])

				if draw_volume_L == 0:
					is_drawing = False
				else:
					sumDrawInletT += draw_volume_L * inletT_C		
					sumDrawOutletT += draw_volume_L * outletT_C		
					sumDrawDiffT += draw_volume_L * (outletT_C - inletT_C)
					sumDraw_L += draw_volume_L			
					draw_heat_capacity_kJperC = water_specific_heat_kJperkgC * water_density_kgperL * draw_volume_L
					sumDrawHeatCap_kJperC += draw_heat_capacity_kJperC
					deliveredEnergy_kJ += draw_heat_capacity_kJperC * (outletT_C - inletT_C)
					is_drawing = True			

				if not(is_drawing):
					noDrawSumAmbientTTime += ambientT_C
					sumNoDrawTime_min += 1
		
				if isFirstRecoveryPeriod:
					tRecover = t_min
					if tRecover > self.ef_bounds.first_recovery_period_end_time:
						tRecover = self.ef_bounds.first_recovery_period_end_time
						isFirstRecoveryPeriod = False
					dt = tRecover - (t_min - 1)
					recoveryTotalDraw_L += draw_volume_L * dt
					recoverySumDrawInletT += draw_volume_L * inletT_C * dt
					recoverySumDrawOutletT += draw_volume_L * outletT_C * dt
					recoveryUsedEnergy_kJ += input_energy_kJ * dt
				
				if not isStandbyPeriod:	
					if t_min >= self.ef_bounds.standby_period_start_time and t_min < self.ef_bounds.standby_period_end_time:
						isStandbyPeriod = True
						standbyStartT_C = tankAvgT_C
						
				if isStandbyPeriod:
					if t_min < self.ef_bounds.standby_period_end_time:	
						tStandby = t_min
						maxTankAfterFirstRecoveryT_C = tankAvgT_C if tankAvgT_C > maxTankAfterFirstRecoveryT_C else maxTankAfterFirstRecoveryT_C	
					else:
						tStandby = self.ef_bounds.standby_period_end_time
						isStandbyPeriod = False
						standbyEndT_C = tankAvgT_C
					dt = tStandby - (t_min - 1)
					standbySumTimeTankT_minC += tankAvgT_C * dt
					standbySumTimeAmbientT_minC += ambientT_C * dt
					standbyUsedEnergy_kJ += input_energy_kJ * dt				
						
			recoveryAvgOutletT_C = recoverySumDrawOutletT / recoveryTotalDraw_L
			recoveryAvgInletT_C = recoverySumDrawInletT / recoveryTotalDraw_L
			recoveryStoredEnergy_kJ = tank_heat_capacity_kJperC * (maxTankAfterFirstRecoveryT_C - initialTankAvgT_C)
			recoveryDeliveredEnergy_kJ = water_specific_heat_kJperkgC * water_density_kgperL * recoveryTotalDraw_L * (recoveryAvgOutletT_C - recoveryAvgInletT_C)

			if sumNoDrawTime_min > 0:
				noDrawAvgAmbientT_C = noDrawSumAmbientTTime / sumNoDrawTime_min
				
			recoveryEfficiency = 0.
			if recoveryUsedEnergy_kJ > 0:
				recoveryEfficiency = (recoveryStoredEnergy_kJ + recoveryDeliveredEnergy_kJ) / recoveryUsedEnergy_kJ
						
			recovery_summary = {
				'firstRecoveryPeriodEndTime_min': self.ef_bounds.first_recovery_period_end_time,
				'maxTankAfterFirstRecoveryT_C': maxTankAfterFirstRecoveryT_C,
				'recoveryTotalDraw_L': recoveryTotalDraw_L,
				'recoveryAvgInletT_C': recoveryAvgInletT_C,
				'recoveryAvgOutletT_C': recoveryAvgOutletT_C,
				'recoveryStoredEnergy_kJ': recoveryStoredEnergy_kJ,
				'recoveryDeliveredEnergy_kJ': recoveryDeliveredEnergy_kJ,
				'recoveryUsedEnergy_kJ': recoveryUsedEnergy_kJ,
				'recoveryEfficiency': recoveryEfficiency
			}
			for k, v in recovery_summary.items():
				recovery_summary[k] = float(v)
			self.test_summary['first-recovery_period'] = recovery_summary
			
			standbyAvgTankT_C = standbySumTimeTankT_minC / standbySumTime_min
			standbyAvgAmbientT_C = standbySumTimeAmbientT_minC / standbySumTime_min
			standbyTankLoss_kJ = tank_heat_capacity_kJperC * (standbyEndT_C - standbyStartT_C)
			standbyHourlyLossEnergy_kJperh = (standbyUsedEnergy_kJ - standbyTankLoss_kJ) / recoveryEfficiency / (standbySumTime_min / 60)
			standbyLossCoefficient_kJperhC = standbyHourlyLossEnergy_kJperh / (standbyAvgTankT_C - standbyAvgAmbientT_C)			
			standby_summary = {
				'standbyPeriodStartTime_min': self.ef_bounds.standby_period_start_time,
				'standbyPeriodEndTime_min': self.ef_bounds.standby_period_end_time,
				'standbyTestDuration h': standbySumTime_min / 60,
				'standbyStartT_C': standbyStartT_C,
				'standbyEndT_C': standbyEndT_C,
				'standbyAvgTankT_C': standbyAvgTankT_C,
				'standbyAvgAmbientT_C': standbyAvgAmbientT_C,
				'standbyUsedEnergy_kJ': standbyUsedEnergy_kJ,
				'standbyTankLoss_kJ': standbyTankLoss_kJ,
				'standbyHourlyLossEnergy_kJperh': standbyHourlyLossEnergy_kJperh,
				'standbyLossCoefficient_kJperhC': standbyLossCoefficient_kJperhC
			}
			for k, v in standby_summary.items():
				standby_summary[k] = float(v)
			self.test_summary['standby_period'] = standby_summary
			
			avgInletT_C = sumDrawInletT / sumDraw_L	
			avgOutletT_C =	sumDrawOutletT / sumDraw_L	
			waterHeatingEnergy_kJ = deliveredEnergy_kJ / recoveryEfficiency
			standardRemovedEnergy_kJ = sumDrawHeatCap_kJperC * (standardSetpointT_C - standardInletT_C)
			standardWaterHeatingEnergy_kJ = standardRemovedEnergy_kJ / recoveryEfficiency
			waterHeatingDifferenceEnergy_kJ = standardWaterHeatingEnergy_kJ - waterHeatingEnergy_kJ
			removedHeatCapacity_kJperC = water_specific_heat_kJperkgC * water_density_kgperL * sumDraw_L	
			standardDeliveredEnergy_kJ = removedHeatCapacity_kJperC * (standardSetpointT_C - standardInletT_C)
			standardEnergyUsedToHeatWater_kJ = standardDeliveredEnergy_kJ/ recoveryEfficiency							
			actualDeliveredEnergy_kJ = removedHeatCapacity_kJperC * (avgOutletT_C - avgInletT_C)
			energyUsedToHeatWater_kJ = actualDeliveredEnergy_kJ / recoveryEfficiency
			waterHeatingDifferenceEnergy_kJ = standardEnergyUsedToHeatWater_kJ - energyUsedToHeatWater_kJ			
			waterHeatingEnergy_kJ = tank_heat_capacity_kJperC * (tankAvgT_C - initialTankAvgT_C) / recoveryEfficiency
			dailyWaterHeaterEnergyConsumption_kJ = sumInputEnergy_kJ - waterHeatingEnergy_kJ	
			adjustedDailyWaterHeaterEnergyConsumption_kJ = dailyWaterHeaterEnergyConsumption_kJ - (standardAmbientT_C - noDrawAvgAmbientT_C) * standbyLossCoefficient_kJperhC * (sumNoDrawTime_min / 60)
			modifiedConsumedWaterHeatingEnergy_kJ = adjustedDailyWaterHeaterEnergyConsumption_kJ + waterHeatingDifferenceEnergy_kJ
			_24_hr_test_summary = {
				'initialTankAvgT_C': initialTankAvgT_C,
				'avgInletT_C': avgInletT_C,
				'avgOutletT_C': avgOutletT_C,
				'sumDraw_L': sumDraw_L,
				'energyUsed_kJ': sumInputEnergy_kJ,
				'standardDeliveredEnergy_kJ': standardDeliveredEnergy_kJ,
				'waterHeatingEnergy_kJ': waterHeatingEnergy_kJ,
				'dailyWaterHeaterEnergyConsumption_kJ': dailyWaterHeaterEnergyConsumption_kJ,
				'standardWaterHeatingEnergy_kJ': standardWaterHeatingEnergy_kJ,
				'energyUsedToHeatWater_kJ': energyUsedToHeatWater_kJ,
				'actualDeliveredEnergy_kJ': actualDeliveredEnergy_kJ,
				'waterHeatingEnergy_kJ': waterHeatingEnergy_kJ,
				'waterHeatingDifferenceEnergy_kJ': waterHeatingDifferenceEnergy_kJ,
				'adjustedDailyWaterHeaterEnergyConsumption_kJ': adjustedDailyWaterHeaterEnergyConsumption_kJ,
				'modifiedConsumedWaterHeatingEnergy_kJ': modifiedConsumedWaterHeatingEnergy_kJ,
				'EF': standardDeliveredEnergy_kJ / modifiedConsumedWaterHeatingEnergy_kJ
			}
			for k, v in _24_hr_test_summary.items():
				_24_hr_test_summary[k] = float(v)
			self.test_summary['24-hr-test'] = _24_hr_test_summary