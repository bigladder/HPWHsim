#ifndef HPWH_DATA_MODEL_H_
#define HPWH_DATA_MODEL_H_

#include <ASHRAE205.h>
#include <load-object.h>
#include <HPWHSimInput.h>
#include <HeatSourceConfiguration.h>
#include <RSRESISTANCEWATERHEATSOURCE.h>
#include <RSCONDENSERWATERHEATSOURCE.h>
#include <RSINTEGRATEDWATERHEATER.h>
#include <RSAIRTOWATERHEATPUMP.h>
#include <CentralWaterHeatingSystem.h>
#include <RSTANK.h>

#define JSON_HAS_CPP_17

namespace hpwh_data_model
{
void init(std::shared_ptr<Courier::Courier> logger_in);
}
#endif

