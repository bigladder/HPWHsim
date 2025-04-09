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
#endif
