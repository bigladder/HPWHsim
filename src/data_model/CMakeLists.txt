
set(data_model_headers
        core.h
        ashrae205.h
        load-object.h
        enum-info.h
        rscondenserwaterheatsource.h
        rsintegratedwaterheater.h
        rsresistancewaterheatsource.h
        rstank.h
        rsinstance.h
        HeatSource.h
        hpwh_data_model.h
        )

set(data_model_sources
        core.cpp
        ashrae205.cpp
        rscondenserwaterheatsource.cpp
        rsintegratedwaterheater.cpp
        rsresistancewaterheatsource.cpp
        rstank.cpp
        )

set(data_model_files
        ${data_model_headers}
        ${data_model_sources}
        )

target_sources(${PROJECT_NAME} PRIVATE ${data_model_files})