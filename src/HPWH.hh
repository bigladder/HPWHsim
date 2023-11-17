#ifndef HPWH_hh
#define HPWH_hh

#include "HPWHversion.hh"
#include "HPWHMain.hh"
#include "HPWHHeatSources.hh"
#include "HPWHHeatingLogics.hh"

// a few extra functions for unit converesion
inline double dF_TO_dC(double temperature) { return (temperature*5.0/9.0); }
inline double F_TO_C(double temperature) { return ((temperature - 32.0)*5.0/9.0); }
inline double C_TO_F(double temperature) { return (((9.0/5.0)*temperature) + 32.0); }
inline double KWH_TO_BTU(double kwh) { return (3412.14 * kwh); }
inline double KWH_TO_KJ(double kwh) { return (kwh * 3600.0); }
inline double BTU_TO_KWH(double btu) { return (btu / 3412.14); }
inline double BTUperH_TO_KW(double btu) { return (btu / 3412.14); }
inline double KW_TO_BTUperH(double kw) { return (kw * 3412.14); }
inline double W_TO_BTUperH(double w) { return (w * 3.41214); }
inline double KJ_TO_KWH(double kj) { return (kj/3600.0); }
inline double BTU_TO_KJ(double btu) { return (btu * 1.055); }
inline double GAL_TO_L(double gallons) { return (gallons * 3.78541); }
inline double L_TO_GAL(double liters) { return (liters / 3.78541); }
inline double L_TO_FT3(double liters) { return (liters / 28.31685); }
inline double UAf_TO_UAc(double UAf) { return (UAf * 1.8 / 0.9478); }
inline double GPM_TO_LPS(double gpm) { return (gpm * 3.78541 / 60.0); }
inline double LPS_TO_GPM(double lps) { return (lps * 60.0 / 3.78541); }

inline double FT_TO_M(double feet) { return (feet / 3.2808); }
inline double FT2_TO_M2(double feet2) { return (feet2 / 10.7640); }

inline double MIN_TO_SEC(double minute) { return minute * 60.; }
inline double MIN_TO_HR(double minute) { return minute / 60.; }

inline HPWH::DRMODES operator|(HPWH::DRMODES a,HPWH::DRMODES b)
{
	return static_cast<HPWH::DRMODES>(static_cast<int>(a) | static_cast<int>(b));
}

template< typename T> inline bool aboutEqual(T a,T b) { return fabs(a - b) < HPWH::TOL_MINVALUE; }

// resampling utility functions
double getResampledValue(const std::vector<double> &values,double beginFraction,double endFraction);
bool resample(std::vector<double> &values,const std::vector<double> &sampleValues);
inline bool resampleIntensive(std::vector<double> &values,const std::vector<double> &sampleValues)
{
	return resample(values,sampleValues);
}
bool resampleExtensive(std::vector<double> &values,const std::vector<double> &sampleValues);

#endif
