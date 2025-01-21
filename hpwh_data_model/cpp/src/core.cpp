#include <core.h>
#include <load-object.h>

namespace hpwh_data_model
{

namespace core
{

void set_logger(std::shared_ptr<Courier::Courier> value) { logger = value; }

const std::string_view Schema::schema_title = "Core";

const std::string_view Schema::schema_version = "0.1.0";

const std::string_view Schema::schema_description =
    "Common Data Types, String Types, Enumerations, and Data Groups";

void from_json(const nlohmann::json& j, Metadata& x)
{
    json_get<std::string>(
        j, logger.get(), "schema_author", x.schema_author, x.schema_author_is_set, true);
    json_get<std::string>(
        j, logger.get(), "schema_name", x.schema_name, x.schema_name_is_set, true);
    json_get<std::string>(
        j, logger.get(), "schema_version", x.schema_version, x.schema_version_is_set, true);
    json_get<std::string>(j, logger.get(), "schema_url", x.schema_url, x.schema_url_is_set, false);
    json_get<std::string>(j, logger.get(), "author", x.author, x.author_is_set, true);
    json_get<std::string>(j, logger.get(), "id", x.id, x.id_is_set, false);
    json_get<std::string>(
        j, logger.get(), "description", x.description, x.description_is_set, true);
    json_get<std::string>(
        j, logger.get(), "time_of_creation", x.time_of_creation, x.time_of_creation_is_set, true);
    json_get<std::string>(j, logger.get(), "version", x.version, x.version_is_set, false);
    json_get<std::string>(j, logger.get(), "source", x.source, x.source_is_set, false);
    json_get<std::string>(j, logger.get(), "disclaimer", x.disclaimer, x.disclaimer_is_set, false);
    json_get<std::string>(j, logger.get(), "notes", x.notes, x.notes_is_set, false);
}
const std::string_view Metadata::schema_author_units = "";

const std::string_view Metadata::schema_author_description =
    "Name of the organization that published the schema";

const std::string_view Metadata::schema_author_name = "schema_author";

const std::string_view Metadata::schema_name_units = "";

const std::string_view Metadata::schema_name_description = "Schema name or identifier";

const std::string_view Metadata::schema_name_name = "schema_name";

const std::string_view Metadata::schema_version_units = "";

const std::string_view Metadata::schema_version_description =
    "The version of the schema the data complies with";

const std::string_view Metadata::schema_version_name = "schema_version";

const std::string_view Metadata::schema_url_units = "";

const std::string_view Metadata::schema_url_description =
    "The Uniform Resource Locator (url) for the schema definition and/or documentation";

const std::string_view Metadata::schema_url_name = "schema_url";

const std::string_view Metadata::author_units = "";

const std::string_view Metadata::author_description =
    "Name of the entity creating the serialization";

const std::string_view Metadata::author_name = "author";

const std::string_view Metadata::id_units = "";

const std::string_view Metadata::id_description = "Unique data set identifier";

const std::string_view Metadata::id_name = "id";

const std::string_view Metadata::description_units = "";

const std::string_view Metadata::description_description =
    "Description of data content (suitable for display)";

const std::string_view Metadata::description_name = "description";

const std::string_view Metadata::time_of_creation_units = "";

const std::string_view Metadata::time_of_creation_description =
    "Timestamp indicating when the serialization was created";

const std::string_view Metadata::time_of_creation_name = "time_of_creation";

const std::string_view Metadata::version_units = "";

const std::string_view Metadata::version_description = "Integer version identifier for the data";

const std::string_view Metadata::version_name = "version";

const std::string_view Metadata::source_units = "";

const std::string_view Metadata::source_description = "Source(s) of the data";

const std::string_view Metadata::source_name = "source";

const std::string_view Metadata::disclaimer_units = "";

const std::string_view Metadata::disclaimer_description =
    "Characterization of accuracy, limitations, and applicability of this data";

const std::string_view Metadata::disclaimer_name = "disclaimer";

const std::string_view Metadata::notes_units = "";

const std::string_view Metadata::notes_description = "Additional Information";

const std::string_view Metadata::notes_name = "notes";

} // namespace core
} // namespace hpwh_data_model
