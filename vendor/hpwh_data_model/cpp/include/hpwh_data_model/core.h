#ifndef CORE_H_
#define CORE_H_
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model {
	namespace core {
		typedef std::string UUID;
		typedef std::string Date;
		typedef std::string Timestamp;
		typedef std::string Version;
		inline std::shared_ptr<Courier::Courier> logger;
		void set_logger(std::shared_ptr<Courier::Courier> value);
		struct Schema {
			const static std::string_view schema_title;
			const static std::string_view schema_version;
			const static std::string_view schema_description;
		};
		struct Metadata {
			std::string schema_author;
			bool schema_author_is_set;
			const static std::string_view schema_author_units;
			const static std::string_view schema_author_description;
			const static std::string_view schema_author_name;
			std::string schema_name;
			bool schema_name_is_set;
			const static std::string_view schema_name_units;
			const static std::string_view schema_name_description;
			const static std::string_view schema_name_name;
			std::string schema_version;
			bool schema_version_is_set;
			const static std::string_view schema_version_units;
			const static std::string_view schema_version_description;
			const static std::string_view schema_version_name;
			std::string schema_url;
			bool schema_url_is_set;
			const static std::string_view schema_url_units;
			const static std::string_view schema_url_description;
			const static std::string_view schema_url_name;
			std::string author;
			bool author_is_set;
			const static std::string_view author_units;
			const static std::string_view author_description;
			const static std::string_view author_name;
			std::string id;
			bool id_is_set;
			const static std::string_view id_units;
			const static std::string_view id_description;
			const static std::string_view id_name;
			std::string description;
			bool description_is_set;
			const static std::string_view description_units;
			const static std::string_view description_description;
			const static std::string_view description_name;
			std::string time_of_creation;
			bool time_of_creation_is_set;
			const static std::string_view time_of_creation_units;
			const static std::string_view time_of_creation_description;
			const static std::string_view time_of_creation_name;
			std::string version;
			bool version_is_set;
			const static std::string_view version_units;
			const static std::string_view version_description;
			const static std::string_view version_name;
			std::string source;
			bool source_is_set;
			const static std::string_view source_units;
			const static std::string_view source_description;
			const static std::string_view source_name;
			std::string disclaimer;
			bool disclaimer_is_set;
			const static std::string_view disclaimer_units;
			const static std::string_view disclaimer_description;
			const static std::string_view disclaimer_name;
			std::string notes;
			bool notes_is_set;
			const static std::string_view notes_units;
			const static std::string_view notes_description;
			const static std::string_view notes_name;
		};
		void from_json(const nlohmann::json& j, Metadata& x);
	}
}
#endif
