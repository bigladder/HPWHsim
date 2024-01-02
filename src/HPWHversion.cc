/*
 * Version control
 */

#include "HPWHversion.hh"
#include "HPWH.hh"

// ugh, this should be in the header
const std::string HPWH::version_maint = HPWHVRSN_META;

std::string HPWH::getVersion()
{
    std::stringstream version;

    version << version_major << '.' << version_minor << '.' << version_patch << version_maint;

    return version.str();
}
