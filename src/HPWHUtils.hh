#ifndef HPWHUTILS_hh
#define HPWHUTILS_hh

//-----------------------------------------------------------------------------
///	@brief	assign t_new to t if is_set, else assign t_default
//-----------------------------------------------------------------------------
template <typename T>
void checkFrom(T& t, const bool is_set, const T t_new, const T t_default)
{
    t = is_set ? t_new : t_default;
}

//-----------------------------------------------------------------------------
///	@brief	assign t_new to t if j contains key, else assign t_default
//-----------------------------------------------------------------------------
template <typename T>
bool checkFrom(T& t, nlohmann::json& j, std::string_view key, const T t_default)
{
    bool has_key = false;
    if (j.contains(key))
    {
        has_key = true;
        t = j[key];
    }
    else
        t = t_default;
    return has_key;
}

//-----------------------------------------------------------------------------
///	@brief	set t to t_new if has_value
//-----------------------------------------------------------------------------
template <typename T>
void checkTo(const T t, bool& is_set, T& t_new, const bool has_value = true)
{
    is_set = has_value;
    if (has_value)
    {
        t_new = t;
    }
}

//-----------------------------------------------------------------------------
///	@brief	Transfer ProductInformation fields from schema
//-----------------------------------------------------------------------------
template <typename RSTYPE>
void fromProductInformation(nlohmann::json& productInformation, const RSTYPE& rs)
{
    productInformation = {};
    if (rs.description_is_set)
    {
        auto& desc = rs.description;
        if (desc.product_information_is_set)
        {
            auto& info = desc.product_information;
            if (info.manufacturer_is_set)
                productInformation["manufacturer"] = info.manufacturer;
            if (info.model_number_is_set)
                productInformation["model_number"] = info.model_number;
        }
    }
}

//-----------------------------------------------------------------------------
///	@brief	Transfer ProductInformation fields to schema
//-----------------------------------------------------------------------------
template <typename RSTYPE>
void toProductInformation(const nlohmann::json& productInformation, RSTYPE& rs)
{
    auto& desc = rs.description;
    auto& prod_info = desc.product_information;

    if (productInformation.contains("manufacturer"))
    {
        prod_info.manufacturer_is_set = true;
        prod_info.manufacturer = productInformation["manufacturer"];
    }

    if (productInformation.contains("model_number"))
    {
        prod_info.model_number_is_set = true;
        prod_info.model_number = productInformation["model_number"];
    }

    bool prod_info_set = prod_info.manufacturer_is_set || prod_info.model_number_is_set;
    checkTo(prod_info, desc.product_information_is_set, desc.product_information, prod_info_set);

    bool desc_set = prod_info_set;
    checkTo(desc, rs.description_is_set, rs.description, desc_set);
}

//-----------------------------------------------------------------------------
///	@brief	Transfer Rating10CFR430 from schema
//-----------------------------------------------------------------------------
template <typename RSTYPE>
void fromRating10CFR430(nlohmann::json& rating10CFR430, const RSTYPE& rs)
{
    rating10CFR430 = {};
    if (rs.description_is_set)
    {
        auto& desc = rs.description;
        if (desc.rating_10_cfr_430_is_set)
        {
            auto& data = desc.rating_10_cfr_430;
            if(data.certified_reference_number_is_set)
                rating10CFR430["certified_reference_number"] = data.certified_reference_number;
            if(data.nominal_tank_volume_is_set)
                rating10CFR430["nominal_tank_volume"] = data.nominal_tank_volume;
            if(data.first_hour_rating_is_set)
                rating10CFR430["first_hour_rating"] = data.first_hour_rating;
            if(data.recovery_efficiency_is_set)
                rating10CFR430["recovery_efficiency"] = data.recovery_efficiency;
            if(data.uniform_energy_factor_is_set)
                rating10CFR430["uniform_energy_factor"] = data.uniform_energy_factor;
        }
    }
}

//-----------------------------------------------------------------------------
///	@brief	Transfer Rating10CFR430 to schema
//-----------------------------------------------------------------------------
template <typename RSTYPE>
void toRating10CFR430(const nlohmann::json& rating10CFR430, RSTYPE& rs)
{
    auto& desc = rs.description;
    auto& data = desc.rating_10_cfr_430;

    if((data.certified_reference_number_is_set = rating10CFR430.contains("certified_reference_number")))
        data.certified_reference_number = rating10CFR430["certified_reference_number"];

    if((data.nominal_tank_volume_is_set = rating10CFR430.contains("nominal_tank_volume")))
        data.nominal_tank_volume = rating10CFR430["nominal_tank_volume"];

    if((data.first_hour_rating_is_set = rating10CFR430.contains("first_hour_rating")))
        data.first_hour_rating = rating10CFR430["first_hour_rating"];

    if((data.recovery_efficiency_is_set = rating10CFR430.contains("recovery_efficiency")))
        data.recovery_efficiency = rating10CFR430["recovery_efficiency"];

    if((data.uniform_energy_factor_is_set = rating10CFR430.contains("uniform_energy_factor")))
        data.uniform_energy_factor = rating10CFR430["uniform_energy_factor"];

    desc.rating_10_cfr_430_is_set |= !rating10CFR430.empty();
    rs.description_is_set |= desc.rating_10_cfr_430_is_set;
}
#endif
