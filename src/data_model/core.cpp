#include <core.h>
#include <load-object.h>
#include <rsintegratedwaterheater.h>

namespace hpwh_data_model
{

namespace core_ns
{

const std::string_view Schema::schema_title = "Core";

const std::string_view Schema::schema_version = "0.1.0";

const std::string_view Schema::schema_description =
    "Common Data Types, String Types, Enumerations, and Data Groups";

void from_json(const nlohmann::json& j, Metadata& x)
{
    json_get<std::string>(j, "schema_author", x.schema_author, x.schema_author_is_set, true);
    json_get<ashrae205_ns::SchemaType>(j, "schema", x.schema, x.schema_is_set, true);
    json_get<std::string>(j, "schema_version", x.schema_version, x.schema_version_is_set, true);
    json_get<std::string>(j, "description", x.description, x.description_is_set, true);
    json_get<std::string>(j, "id", x.id, x.id_is_set, false);
    json_get<std::string>(j, "data_timestamp", x.data_timestamp, x.timestamp_is_set, true);
    json_get<int>(j, "data_version", x.data_version, x.version_is_set, false);
    json_get<std::string>(j, "data_source", x.data_source, x.data_source_is_set, false);
    json_get<std::string>(j, "disclaimer", x.disclaimer, x.disclaimer_is_set, false);
    json_get<std::string>(j, "notes", x.notes, x.notes_is_set, false);
}
const std::string_view Metadata::schema_author_units = "";

const std::string_view Metadata::schema_units = "";

const std::string_view Metadata::schema_version_units = "";

const std::string_view Metadata::description_units = "";

const std::string_view Metadata::id_units = "";

const std::string_view Metadata::data_timestamp_units = "";

const std::string_view Metadata::data_version_units = "";

const std::string_view Metadata::data_source_units = "";

const std::string_view Metadata::disclaimer_units = "";

const std::string_view Metadata::notes_units = "";

const std::string_view Metadata::schema_author_description = "Name of the schema author";

const std::string_view Metadata::schema_description = "Schema name or identifier";

const std::string_view Metadata::schema_version_description =
    "Version of the root schema this data complies with";

const std::string_view Metadata::description_description =
    "Description of data content (suitable for display)";

const std::string_view Metadata::data_timestamp_description = "Date of data publication";

const std::string_view Metadata::id_description = "Unique data set identifier";

const std::string_view Metadata::version_description = "Integer version identifier for the data";

const std::string_view Metadata::data_source_description = "Source(s) of the data";

const std::string_view Metadata::disclaimer_description =
    "Characterization of accuracy, limitations, and applicability of this data";

const std::string_view Metadata::notes_description = "Additional Information";

const std::string_view Metadata::schema_author_name = "schema_author";

const std::string_view Metadata::schema_name = "schema";

const std::string_view Metadata::schema_version_name = "schema_version";

const std::string_view Metadata::description_name = "description";

const std::string_view Metadata::data_timestamp_name = "data_timestamp";

const std::string_view Metadata::id_name = "id";

const std::string_view Metadata::version_name = "data_version";

const std::string_view Metadata::data_source_name = "data_source";

const std::string_view Metadata::disclaimer_name = "disclaimer";

const std::string_view Metadata::notes_name = "notes";

} // namespace core_ns
} // namespace hpwh_data_model
