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
#include "courier/helpers.h"

#define JSON_HAS_CPP_17

namespace hpwh_data_model
{
class QuietCourier : public Courier::DefaultCourier
{
  public:
    static inline std::vector<std::string> warnings = {};

  protected:
    void receive_warning(const std::string& message) override { warnings.push_back(message); }
};
void init(std::shared_ptr<Courier::Courier> logger_in = std::make_shared<QuietCourier>());
} // namespace hpwh_data_model
#endif
