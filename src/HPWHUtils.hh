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
///	@brief	Transfer ProductInformation fields to JSON
//-----------------------------------------------------------------------------
template <typename RSTYPE>
void productInformation_to_json(const RSTYPE& rs, nlohmann::json& j)
{
    if (rs.description_is_set)
    {
        auto& desc = rs.description;
        if (desc.product_information_is_set)
        {
            nlohmann::json j_prod_info = {};
            auto& prod_info = desc.product_information;
            if (prod_info.manufacturer_is_set)
                j_prod_info["manufacturer"] = prod_info.manufacturer;
            if (prod_info.model_number_is_set)
                j_prod_info["model_number"] = prod_info.model_number;

            if (!j_prod_info.empty())
            {
                if (!j.contains("description"))
                    j["description"] = {};
                j["description"]["product_information"] = j_prod_info;
            }
        }
    }
}

#endif
