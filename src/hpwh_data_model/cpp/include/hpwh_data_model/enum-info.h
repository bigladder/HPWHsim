#ifndef ENUM_INFO_H_
#define ENUM_INFO_H_

#include <string_view>

namespace hpwh_data_model {

    struct enum_info
    { 
        std::string_view enumerant_name; 
        std::string_view display_text; 
        std::string_view description;
    };
}

#endif
