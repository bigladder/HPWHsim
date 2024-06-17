#include <core.h>
#include <load-object.h>

namespace data_model  {

	namespace core_ns  {
	
		void set_logger (std::shared_ptr<Courier::Courier> value) { logger = std::move(value); }

		void from_json(const nlohmann::json& j, Schema& x) {
		}
		const std::string_view Schema::schema_title = "Core";

		const std::string_view Schema::schema_version = "0.1.0";

		const std::string_view Schema::schema_description = "Common Data Types, String Types, Enumerations, and Data Groups";

		void from_json(const nlohmann::json& j, Metadata& x) {
			json_get<std::string>(j, logger.get(), "schema_author", x.schema_author, x.schema_author_is_set, true);
			json_get<ashrae205_ns::SchemaType>(j, logger.get(), "schema", x.schema, x.schema_is_set, true);
			json_get<std::string>(j, logger.get(), "schema_version", x.schema_version, x.schema_version_is_set, true);
			json_get<std::string>(j, logger.get(), "description", x.description, x.description_is_set, true);
			json_get<std::string>(j, logger.get(), "timestamp", x.timestamp, x.timestamp_is_set, true);
			json_get<std::string>(j, logger.get(), "id", x.id, x.id_is_set, false);
			json_get<int>(j, logger.get(), "version", x.version, x.version_is_set, false);
			json_get<std::string>(j, logger.get(), "data_source", x.data_source, x.data_source_is_set, false);
			json_get<std::string>(j, logger.get(), "disclaimer", x.disclaimer, x.disclaimer_is_set, false);
			json_get<std::string>(j, logger.get(), "notes", x.notes, x.notes_is_set, false);
		}
		const std::string_view Metadata::schema_author_units = "";

		const std::string_view Metadata::schema_units = "";

		const std::string_view Metadata::schema_version_units = "";

		const std::string_view Metadata::description_units = "";

		const std::string_view Metadata::timestamp_units = "";

		const std::string_view Metadata::id_units = "";

		const std::string_view Metadata::version_units = "";

		const std::string_view Metadata::data_source_units = "";

		const std::string_view Metadata::disclaimer_units = "";

		const std::string_view Metadata::notes_units = "";

		const std::string_view Metadata::schema_author_description = "Name of the schema author";

		const std::string_view Metadata::schema_description = "Schema name or identifier";

		const std::string_view Metadata::schema_version_description = "Version of the root schema this data complies with";

		const std::string_view Metadata::description_description = "Description of data content (suitable for display)";

		const std::string_view Metadata::timestamp_description = "Date of data publication";

		const std::string_view Metadata::id_description = "Unique data set identifier";

		const std::string_view Metadata::version_description = "Integer version identifier for the data";

		const std::string_view Metadata::data_source_description = "Source(s) of the data";

		const std::string_view Metadata::disclaimer_description = "Characterization of accuracy, limitations, and applicability of this data";

		const std::string_view Metadata::notes_description = "Additional Information";

		const std::string_view Metadata::schema_author_name = "schema_author";

		const std::string_view Metadata::schema_name = "schema";

		const std::string_view Metadata::schema_version_name = "schema_version";

		const std::string_view Metadata::description_name = "description";

		const std::string_view Metadata::timestamp_name = "timestamp";

		const std::string_view Metadata::id_name = "id";

		const std::string_view Metadata::version_name = "version";

		const std::string_view Metadata::data_source_name = "data_source";

		const std::string_view Metadata::disclaimer_name = "disclaimer";

		const std::string_view Metadata::notes_name = "notes";

	}
}

