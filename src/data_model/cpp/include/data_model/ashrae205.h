#ifndef ASHRAE205_H_
#define ASHRAE205_H_
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>
#include <ashrae205.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace data_model
{
namespace ashrae205_ns
{
enum class SchemaType
{
    RS0001,
    RS0002,
    RS0003,
    RS0004,
    RS0005,
    RS0006,
    RS0007,
    RSINTEGRATEDWATERHEATER,
    RSTANK,
    RSRESISTANCEWATERHEATSOURCE,
    RSCONDENSERWATERHEATSOURCE,
    UNKNOWN
};
const static std::unordered_map<SchemaType, enum_info> SchemaType_info {
    {SchemaType::RS0001, {"RS0001", "RS0001", "Liquid-Cooled Chiller"}},
    {SchemaType::RS0002, {"RS0002", "RS0002", "Unitary Cooling Air-Conditioning Equipment"}},
    {SchemaType::RS0003, {"RS0003", "RS0003", "Fan Assembly"}},
    {SchemaType::RS0004,
     {"RS0004", "RS0004", "Air-to-Air Direct Expansion Refrigerant Coil System"}},
    {SchemaType::RS0005, {"RS0005", "RS0005", "Motor"}},
    {SchemaType::RS0006, {"RS0006", "RS0006", "Electronic Motor Drive"}},
    {SchemaType::RS0007, {"RS0007", "RS0007", "Mechanical Drive"}},
    {SchemaType::RSINTEGRATEDWATERHEATER,
     {"RSINTEGRATEDWATERHEATER", "RSINTEGRATEDWATERHEATER", "Integrated Heat-Pump Water Heater"}},
    {SchemaType::RSTANK, {"RSTANK", "RSTANK", "Water Tank"}},
    {SchemaType::RSRESISTANCEWATERHEATSOURCE,
     {"RSRESISTANCEWATERHEATSOURCE",
      "RSRESISTANCEWATERHEATSOURCE",
      "Resistance Water Heat Source"}},
    {SchemaType::RSCONDENSERWATERHEATSOURCE,
     {"RSCONDENSERWATERHEATSOURCE", "RSCONDENSERWATERHEATSOURCE", "Condenser Water Heat Source"}},
    {SchemaType::UNKNOWN, {"UNKNOWN", "None", "None"}}};
enum class CompressorType
{
    RECIPROCATING,
    SCREW,
    CENTRIFUGAL,
    ROTARY,
    SCROLL,
    UNKNOWN
};
const static std::unordered_map<CompressorType, enum_info> CompressorType_info {
    {CompressorType::RECIPROCATING, {"RECIPROCATING", "Reciprocating", "Reciprocating compressor"}},
    {CompressorType::SCREW, {"SCREW", "Screw", "Screw compressor"}},
    {CompressorType::CENTRIFUGAL, {"CENTRIFUGAL", "Centrifugal", "Centrifugal compressor"}},
    {CompressorType::ROTARY, {"ROTARY", "Rotary", "Rotary compressor"}},
    {CompressorType::SCROLL, {"SCROLL", "Scroll", "Scroll compressor"}},
    {CompressorType::UNKNOWN, {"UNKNOWN", "None", "None"}}};
enum class CompressorSpeedControlType
{
    DISCRETE,
    CONTINUOUS,
    UNKNOWN
};
const static std::unordered_map<CompressorSpeedControlType, enum_info>
    CompressorSpeedControlType_info {
        {CompressorSpeedControlType::DISCRETE,
         {"DISCRETE",
          "Discrete",
          "Compressor loading is controlled by cycling between one or more discrete stages"}},
        {CompressorSpeedControlType::CONTINUOUS,
         {"CONTINUOUS",
          "Continuous",
          "Compressor loading is controlled by continuously varying the speed of the compressor"}},
        {CompressorSpeedControlType::UNKNOWN, {"UNKNOWN", "None", "None"}}};
enum class CondenserType
{
    AIR,
    LIQUID,
    EVAPORATIVE,
    UNKNOWN
};
const static std::unordered_map<CondenserType, enum_info> CondenserType_info {
    {CondenserType::AIR, {"AIR", "Air", "Air-cooled condenser"}},
    {CondenserType::LIQUID, {"LIQUID", "Liquid", "Liquid-cooled condenser"}},
    {CondenserType::EVAPORATIVE, {"EVAPORATIVE", "Evaporative", "Evaporative condenser"}},
    {CondenserType::UNKNOWN, {"UNKNOWN", "None", "None"}}};
enum class LiquidConstituent
{
    WATER,
    PROPYLENE_GLYCOL,
    ETHYLENE_GLYCOL,
    SODIUM_CHLORIDE,
    CALCIUM_CHLORIDE,
    ETHANOL,
    METHANOL,
    UNKNOWN
};
const static std::unordered_map<LiquidConstituent, enum_info> LiquidConstituent_info {
    {LiquidConstituent::WATER, {"WATER", "Water", "Water"}},
    {LiquidConstituent::PROPYLENE_GLYCOL,
     {"PROPYLENE_GLYCOL", "Propylene Glycol", "Propylene glycol"}},
    {LiquidConstituent::ETHYLENE_GLYCOL, {"ETHYLENE_GLYCOL", "Ethylene Glycol", "Ethylene glycol"}},
    {LiquidConstituent::SODIUM_CHLORIDE, {"SODIUM_CHLORIDE", "Sodium Chloride", "Sodium chloride"}},
    {LiquidConstituent::CALCIUM_CHLORIDE,
     {"CALCIUM_CHLORIDE", "Calcium Chloride", "Calcium chloride"}},
    {LiquidConstituent::ETHANOL, {"ETHANOL", "Ethanol", "Ethanol"}},
    {LiquidConstituent::METHANOL, {"METHANOL", "Methanol", "Methanol"}},
    {LiquidConstituent::UNKNOWN, {"UNKNOWN", "None", "None"}}};
enum class ConcentrationType
{
    BY_VOLUME,
    BY_MASS,
    UNKNOWN
};
const static std::unordered_map<ConcentrationType, enum_info> ConcentrationType_info {
    {ConcentrationType::BY_VOLUME,
     {"BY_VOLUME",
      "By Volume",
      "Concentration is defined as a fraction of total liquid mixture volume"}},
    {ConcentrationType::BY_MASS,
     {"BY_MASS", "By Mass", "Concentration is defined as a fraction of total liquid mixture mass"}},
    {ConcentrationType::UNKNOWN, {"UNKNOWN", "None", "None"}}};
inline std::shared_ptr<Courier::Courier> logger;
void set_logger(std::shared_ptr<Courier::Courier> value);
struct Schema
{
    const static std::string_view schema_title;
    const static std::string_view schema_version;
    const static std::string_view schema_description;
};
struct LiquidComponent
{
    ashrae205_ns::LiquidConstituent liquid_constituent;
    double concentration;
    bool liquid_constituent_is_set;
    bool concentration_is_set;
    const static std::string_view liquid_constituent_units;
    const static std::string_view concentration_units;
    const static std::string_view liquid_constituent_description;
    const static std::string_view concentration_description;
    const static std::string_view liquid_constituent_name;
    const static std::string_view concentration_name;
};
struct LiquidMixture
{
    std::vector<ashrae205_ns::LiquidComponent> liquid_components;
    ashrae205_ns::ConcentrationType concentration_type;
    bool liquid_components_is_set;
    bool concentration_type_is_set;
    const static std::string_view liquid_components_units;
    const static std::string_view concentration_type_units;
    const static std::string_view liquid_components_description;
    const static std::string_view concentration_type_description;
    const static std::string_view liquid_components_name;
    const static std::string_view concentration_type_name;
};
NLOHMANN_JSON_SERIALIZE_ENUM(
    SchemaType,
    {
        {SchemaType::UNKNOWN, "UNKNOWN"},
        {SchemaType::RS0001, "RS0001"},
        {SchemaType::RS0002, "RS0002"},
        {SchemaType::RS0003, "RS0003"},
        {SchemaType::RS0004, "RS0004"},
        {SchemaType::RS0005, "RS0005"},
        {SchemaType::RS0006, "RS0006"},
        {SchemaType::RS0007, "RS0007"},
        {SchemaType::RSINTEGRATEDWATERHEATER, "RSINTEGRATEDWATERHEATER"},
        {SchemaType::RSTANK, "RSTANK"},
        {SchemaType::RSRESISTANCEWATERHEATSOURCE, "RSRESISTANCEWATERHEATSOURCE"},
        {SchemaType::RSCONDENSERWATERHEATSOURCE, "RSCONDENSERWATERHEATSOURCE"},
    })
NLOHMANN_JSON_SERIALIZE_ENUM(CompressorType,
                             {
                                 {CompressorType::UNKNOWN, "UNKNOWN"},
                                 {CompressorType::RECIPROCATING, "RECIPROCATING"},
                                 {CompressorType::SCREW, "SCREW"},
                                 {CompressorType::CENTRIFUGAL, "CENTRIFUGAL"},
                                 {CompressorType::ROTARY, "ROTARY"},
                                 {CompressorType::SCROLL, "SCROLL"},
                             })
NLOHMANN_JSON_SERIALIZE_ENUM(CompressorSpeedControlType,
                             {
                                 {CompressorSpeedControlType::UNKNOWN, "UNKNOWN"},
                                 {CompressorSpeedControlType::DISCRETE, "DISCRETE"},
                                 {CompressorSpeedControlType::CONTINUOUS, "CONTINUOUS"},
                             })
NLOHMANN_JSON_SERIALIZE_ENUM(CondenserType,
                             {
                                 {CondenserType::UNKNOWN, "UNKNOWN"},
                                 {CondenserType::AIR, "AIR"},
                                 {CondenserType::LIQUID, "LIQUID"},
                                 {CondenserType::EVAPORATIVE, "EVAPORATIVE"},
                             })
NLOHMANN_JSON_SERIALIZE_ENUM(LiquidConstituent,
                             {
                                 {LiquidConstituent::UNKNOWN, "UNKNOWN"},
                                 {LiquidConstituent::WATER, "WATER"},
                                 {LiquidConstituent::PROPYLENE_GLYCOL, "PROPYLENE_GLYCOL"},
                                 {LiquidConstituent::ETHYLENE_GLYCOL, "ETHYLENE_GLYCOL"},
                                 {LiquidConstituent::SODIUM_CHLORIDE, "SODIUM_CHLORIDE"},
                                 {LiquidConstituent::CALCIUM_CHLORIDE, "CALCIUM_CHLORIDE"},
                                 {LiquidConstituent::ETHANOL, "ETHANOL"},
                                 {LiquidConstituent::METHANOL, "METHANOL"},
                             })
NLOHMANN_JSON_SERIALIZE_ENUM(ConcentrationType,
                             {
                                 {ConcentrationType::UNKNOWN, "UNKNOWN"},
                                 {ConcentrationType::BY_VOLUME, "BY_VOLUME"},
                                 {ConcentrationType::BY_MASS, "BY_MASS"},
                             })
void from_json(const nlohmann::json& j, LiquidMixture& x);
void from_json(const nlohmann::json& j, LiquidComponent& x);
} // namespace ashrae205_ns
} // namespace data_model
#endif
