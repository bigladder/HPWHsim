#ifndef HPWH_DATA_MODEL_H_
#define HPWH_DATA_MODEL_H_

#include <ashrae205.h>
#include <load-object.h>
#include <rstank.h>
#include <rsresistancewaterheatsource.h>
#include <rscondenserwaterheatsource.h>
#include <rsintegratedwaterheater.h>

#define JSON_HAS_CPP_17

namespace data_model
{
void init(std::shared_ptr<Courier::Courier> logger_in);
}
#endif