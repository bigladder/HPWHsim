#ifndef CORE_H_
#define CORE_H_
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>
#include <ashrae205.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace data_model {
	namespace core_ns {
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
			ashrae205_ns::SchemaType schema;
			std::string schema_version;
			std::string description;
			std::string timestamp;
			std::string id;
			int version;
			std::string data_source;
			std::string disclaimer;
			std::string notes;
			bool schema_author_is_set;
			bool schema_is_set;
			bool schema_version_is_set;
			bool description_is_set;
			bool timestamp_is_set;
			bool id_is_set;
			bool version_is_set;
			bool data_source_is_set;
			bool disclaimer_is_set;
			bool notes_is_set;
			const static std::string_view schema_author_units;
			const static std::string_view schema_units;
			const static std::string_view schema_version_units;
			const static std::string_view description_units;
			const static std::string_view timestamp_units;
			const static std::string_view id_units;
			const static std::string_view version_units;
			const static std::string_view data_source_units;
			const static std::string_view disclaimer_units;
			const static std::string_view notes_units;
			const static std::string_view schema_author_description;
			const static std::string_view schema_description;
			const static std::string_view schema_version_description;
			const static std::string_view description_description;
			const static std::string_view timestamp_description;
			const static std::string_view id_description;
			const static std::string_view version_description;
			const static std::string_view data_source_description;
			const static std::string_view disclaimer_description;
			const static std::string_view notes_description;
			const static std::string_view schema_author_name;
			const static std::string_view schema_name;
			const static std::string_view schema_version_name;
			const static std::string_view description_name;
			const static std::string_view timestamp_name;
			const static std::string_view id_name;
			const static std::string_view version_name;
			const static std::string_view data_source_name;
			const static std::string_view disclaimer_name;
			const static std::string_view notes_name;
		};
		void from_json (const nlohmann::json& j, Metadata& x);
	}
}
#endif
