
from common import read_file, write_file, get_perf_map, set_perf_map, get_heat_source_configuration, set_heat_source_configuration
import math

class EF_Bounds:
	def __init__(self):
		self.test_start_time = 0
		self.test_end_time = -1
		self.first_recovery_period_start_time = 0
		self.first_recovery_period_end_time = -1
		self.standby_period_start_time = 0
		self.standby_period_end_time = -1

class TestAnalyzer:

	def find_EF_bounds(self, data_set):	
		column_time = data_set.df[self.variables["X-Variables"]["Time"]["Column Names"][data_set.variable_type]]
		data_set.ef_bounds.test_start_time = column_time.iloc[0]
		data_set.ef_bounds.test_end_time = column_time.iloc[-1]
		
		hasFirstRecoveryPeriodStarted = True
		hasFirstRecoveryPeriodEnded = False
		data_set.ef_bounds.first_recovery_period_start_time = data_set.ef_bounds.test_start_time
		data_set.ef_bounds.first_recovery_period_end_time = data_set.ef_bounds.test_end_time
		
		hasStandbyPeriodStarted = False
		hasStandbyPeriodEnded = False
		data_set.ef_bounds.standby_period_start_time = data_set.ef_bounds.test_start_time
		data_set.ef_bounds.standby_period_end_time = data_set.ef_bounds.test_end_time
		has_heated = False
		has_drawn = False
		
		for index in range(len(data_set.df)):
			t_min = column_time.iloc[index]
			draw_volume_L = data_set.df.loc[index, self.variables["Y-Variables"]["Flow Rate"]["Column Names"][data_set.variable_type][0]] * 3.78541	
			input_energy_kJ = data_set.df.loc[index, self.variables["Y-Variables"]["Power Input"]["Column Names"][data_set.variable_type][0]] * 60 / 1000

			if math.isnan(draw_volume_L):
				draw_volume_L = 0
				
			is_heating = input_energy_kJ > 0.5
			has_heated = has_heated or is_heating
			
			is_drawing = draw_volume_L > 0
			has_drawn = has_drawn or is_drawing
			
			data_set.ef_bounds.test_end_time = t_min
			if hasFirstRecoveryPeriodStarted:
				if hasFirstRecoveryPeriodEnded:
					if hasStandbyPeriodStarted:	
						if hasStandbyPeriodEnded:
							continue
						elif is_drawing or is_heating:	
							hasStandbyPeriodEnded = True		
							data_set.ef_bounds.standby_period_end_time = t_min - 1
							continue
					elif t_min >= data_set.ef_bounds.first_recovery_period_end_time + 5:
						if not(is_drawing) and not(is_heating):
							hasStandbyPeriodStarted = True
							data_set.ef_bounds.standby_period_start_time = t_min
				else:
					if has_drawn and not(is_drawing) and has_heated and not(is_heating):	
						hasFirstRecoveryPeriodEnded = True
						data_set.ef_bounds.first_recovery_period_end_time = t_min	- 1	
				
	def analyze(self, data_set):		
		self.find_EF_bounds(data_set)
				
		initialTankAvgT_C = data_set.df["Tank Average Temperature_C"].iloc[data_set.ef_bounds.test_start_time]
		finalTankAvgT_C = data_set.df["Tank Average Temperature_C"].iloc[data_set.ef_bounds.test_end_time]
		
		draw_col = data_set.df[self.variables["Y-Variables"]["Flow Rate"]["Column Names"][data_set.variable_type][0]]
		power_col = data_set.df[self.variables["Y-Variables"]["Power Input"]["Column Names"][data_set.variable_type][0]]
		
		water_specific_heat_kJperkgC = 4.180
		water_density_kgperL = 0.995
		tank_heat_capacity_kJperC = water_specific_heat_kJperkgC * water_density_kgperL * data_set.test_summary['tank_volume_L']
		
		sumDrawOutletT = 0
		sumDrawInletT = 0
		sumDrawDiffT = 0
		sumDraw_L = 0
		sumDrawHeatCap_kJperC = 0
		sumInputEnergy_kJ = 0
		sumNoDrawTime_min = 0
		noDrawSumAmbientTTime = 0
		
		recoveryUsedEnergy_kJ = 0
		recoveryStoredEnergy_kJ = 0
		recoveryDeliveredEnergy_kJ = 0
	
		standbyUsedEnergy_kJ = 0
		standbySumTime_min = data_set.ef_bounds.standby_period_end_time - data_set.ef_bounds.standby_period_start_time + 1
		standbySumTimeTankT_minC = 0
		standbySumTimeAmbientT_minC = 0
		
		deliveredEnergy_kJ = 0
		column_time = data_set.df[self.variables["X-Variables"]["Time"]["Column Names"][data_set.variable_type]]
		standardSetpointT_C = 51.7
		standardInletT_C = 14.4
		standardAmbientT_C = 19.7
		
		noDrawAvgAmbientT_C = standardAmbientT_C
		maxTankAfterFirstRecoveryT_C = -10
		for index in range(len(data_set.df)):
			t_min = column_time.iloc[index]
			if t_min < data_set.ef_bounds.test_start_time or t_min > data_set.ef_bounds.test_end_time:
				continue
			
			ambientT_C = (data_set.df.loc[index, self.variables["Y-Variables"]["Temperature"]["Ambient"][data_set.variable_type]] - 32) / 1.8
			tankAvgT_C = data_set.df.loc[index, "Tank Average Temperature_C"]
			draw_volume_L = draw_col[index] * 3.78541	
			input_energy_kJ = power_col[index] * 60 / 1000
			inletT_C = 0 if draw_volume_L == 0 else (data_set.df.loc[index, self.variables["Y-Variables"]["Temperature"]["Inlet"][data_set.variable_type]] - 32) / 1.8
			outletT_C = 0 if draw_volume_L == 0 else (data_set.df.loc[index, self.variables["Y-Variables"]["Temperature"]["Outlet"][data_set.variable_type]] - 32) / 1.8
			
			sumInputEnergy_kJ += input_energy_kJ
			if math.isnan(draw_volume_L):
				draw_volume_L = 0
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
	
			if t_min == data_set.ef_bounds.first_recovery_period_end_time:
				recoveryTotalDraw_L = sumDraw_L		
				recoveryAvgInletT_C =	sumDrawInletT / sumDraw_L
				recoveryAvgOutletT_C =	sumDrawOutletT / sumDraw_L
				recoveryUsedEnergy_kJ = sumInputEnergy_kJ	

			if t_min > data_set.ef_bounds.first_recovery_period_end_time and t_min <= data_set.ef_bounds.standby_period_end_time:
				maxTankAfterFirstRecoveryT_C = tankAvgT_C if tankAvgT_C > maxTankAfterFirstRecoveryT_C else maxTankAfterFirstRecoveryT_C
				
			if t_min == data_set.ef_bounds.standby_period_start_time:
				standbyStartT_C = tankAvgT_C
				
			if t_min == data_set.ef_bounds.standby_period_end_time:
				standbyEndT_C = tankAvgT_C
				
			if t_min >= data_set.ef_bounds.standby_period_start_time and t_min <= data_set.ef_bounds.standby_period_end_time:	
				standbySumTimeTankT_minC += (1.) * tankAvgT_C 
				standbySumTimeAmbientT_minC += (1.) * ambientT_C
				standbyUsedEnergy_kJ += input_energy_kJ

		recoveryStoredEnergy_kJ = tank_heat_capacity_kJperC * (maxTankAfterFirstRecoveryT_C - initialTankAvgT_C)
		recoveryDeliveredEnergy_kJ = water_specific_heat_kJperkgC * water_density_kgperL * recoveryTotalDraw_L * (recoveryAvgOutletT_C - recoveryAvgInletT_C)

		if sumNoDrawTime_min > 0:
			noDrawAvgAmbientT_C = noDrawSumAmbientTTime / sumNoDrawTime_min
			
		recoveryEfficiency = 0.
		if recoveryUsedEnergy_kJ > 0:
			recoveryEfficiency = (recoveryStoredEnergy_kJ + recoveryDeliveredEnergy_kJ) / recoveryUsedEnergy_kJ
					
		recovery_summary = {
			'firstRecoveryPeriodEndTime_min': data_set.ef_bounds.first_recovery_period_end_time,
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
		data_set.test_summary['first-recovery_period'] = recovery_summary
		
		standbyAvgTankT_C = standbySumTimeTankT_minC / standbySumTime_min
		standbyAvgAmbientT_C = standbySumTimeAmbientT_minC / standbySumTime_min
		standbyTankLoss_kJ = tank_heat_capacity_kJperC * (standbyEndT_C - standbyStartT_C)
		standbyHourlyLossEnergy_kJperh = (standbyUsedEnergy_kJ - standbyTankLoss_kJ) / recoveryEfficiency / (standbySumTime_min / 60)
		standbyLossCoefficient_kJperhC = standbyHourlyLossEnergy_kJperh / (standbyAvgTankT_C - standbyAvgAmbientT_C)			
		standby_summary = {
			'standbyPeriodStartTime_min': data_set.ef_bounds.standby_period_start_time,
			'standbyPeriodEndTime': data_set.ef_bounds.standby_period_end_time,
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
		data_set.test_summary['standby_period'] = standby_summary
		
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
		data_set.test_summary['24-hr-test'] = _24_hr_test_summary