#include <core.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace core  {
	
		void set_logger (std::shared_ptr<Courier::Courier> value) { logger = value; }

		void from_json(const nlohmann::json& j, Metadata& x) {
			json_get<std::string>(j, logger.get(), "schema_author", x.schema_author, x.schema_author_is_set, true);
			json_get<std::string>(j, logger.get(), "schema_name", x.schema_name, x.schema_name_is_set, true);
			json_get<std::string>(j, logger.get(), "schema_version", x.schema_version, x.schema_version_is_set, true);
			json_get<std::string>(j, logger.get(), "schema_url", x.schema_url, x.schema_url_is_set, false);
			json_get<std::string>(j, logger.get(), "author", x.author, x.author_is_set, true);
			json_get<std::string>(j, logger.get(), "id", x.id, x.id_is_set, false);
			json_get<std::string>(j, logger.get(), "description", x.description, x.description_is_set, true);
			json_get<std::string>(j, logger.get(), "time_of_creation", x.time_of_creation, x.time_of_creation_is_set, true);
			json_get<std::string>(j, logger.get(), "version", x.version, x.version_is_set, false);
			json_get<std::string>(j, logger.get(), "source", x.source, x.source_is_set, false);
			json_get<std::string>(j, logger.get(), "disclaimer", x.disclaimer, x.disclaimer_is_set, false);
			json_get<std::string>(j, logger.get(), "notes", x.notes, x.notes_is_set, false);
		}
		void to_json(nlohmann::json& j, const Metadata& x) {
			json_set<std::string>(j, "schema_author", x.schema_author, x.schema_author_is_set);
			json_set<std::string>(j, "schema_name", x.schema_name, x.schema_name_is_set);
			json_set<std::string>(j, "schema_version", x.schema_version, x.schema_version_is_set);
			json_set<std::string>(j, "schema_url", x.schema_url, x.schema_url_is_set);
			json_set<std::string>(j, "author", x.author, x.author_is_set);
			json_set<std::string>(j, "id", x.id, x.id_is_set);
			json_set<std::string>(j, "description", x.description, x.description_is_set);
			json_set<std::string>(j, "time_of_creation", x.time_of_creation, x.time_of_creation_is_set);
			json_set<std::string>(j, "version", x.version, x.version_is_set);
			json_set<std::string>(j, "source", x.source, x.source_is_set);
			json_set<std::string>(j, "disclaimer", x.disclaimer, x.disclaimer_is_set);
			json_set<std::string>(j, "notes", x.notes, x.notes_is_set);
		}
	}
}

