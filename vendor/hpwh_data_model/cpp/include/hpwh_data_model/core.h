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
			static constexpr std::string_view schema_title = "Core";
			static constexpr std::string_view schema_version = "0.1.0";
			static constexpr std::string_view schema_description = "Common Data Types, String Types, Enumerations, and Data Groups";
		};
		struct Metadata {
			std::string schema_author;
			bool schema_author_is_set = false;
			static constexpr std::string_view schema_author_units = "";
			static constexpr std::string_view schema_author_description = "Name of the organization that published the schema";
			static constexpr std::string_view schema_author_name = "schema_author";
			std::string schema_name;
			bool schema_name_is_set = false;
			static constexpr std::string_view schema_name_units = "";
			static constexpr std::string_view schema_name_description = "Schema name or identifier";
			static constexpr std::string_view schema_name_name = "schema_name";
			std::string schema_version;
			bool schema_version_is_set = false;
			static constexpr std::string_view schema_version_units = "";
			static constexpr std::string_view schema_version_description = "The version of the schema the data complies with";
			static constexpr std::string_view schema_version_name = "schema_version";
			std::string schema_url;
			bool schema_url_is_set = false;
			static constexpr std::string_view schema_url_units = "";
			static constexpr std::string_view schema_url_description = "The Uniform Resource Locator (url) for the schema definition and/or documentation";
			static constexpr std::string_view schema_url_name = "schema_url";
			std::string author;
			bool author_is_set = false;
			static constexpr std::string_view author_units = "";
			static constexpr std::string_view author_description = "Name of the entity creating the serialization";
			static constexpr std::string_view author_name = "author";
			std::string id;
			bool id_is_set = false;
			static constexpr std::string_view id_units = "";
			static constexpr std::string_view id_description = "Unique data set identifier";
			static constexpr std::string_view id_name = "id";
			std::string description;
			bool description_is_set = false;
			static constexpr std::string_view description_units = "";
			static constexpr std::string_view description_description = "Description of data content (suitable for display)";
			static constexpr std::string_view description_name = "description";
			std::string time_of_creation;
			bool time_of_creation_is_set = false;
			static constexpr std::string_view time_of_creation_units = "";
			static constexpr std::string_view time_of_creation_description = "Timestamp indicating when the serialization was created";
			static constexpr std::string_view time_of_creation_name = "time_of_creation";
			std::string version;
			bool version_is_set = false;
			static constexpr std::string_view version_units = "";
			static constexpr std::string_view version_description = "Integer version identifier for the data";
			static constexpr std::string_view version_name = "version";
			std::string source;
			bool source_is_set = false;
			static constexpr std::string_view source_units = "";
			static constexpr std::string_view source_description = "Source(s) of the data";
			static constexpr std::string_view source_name = "source";
			std::string disclaimer;
			bool disclaimer_is_set = false;
			static constexpr std::string_view disclaimer_units = "";
			static constexpr std::string_view disclaimer_description = "Characterization of accuracy, limitations, and applicability of this data";
			static constexpr std::string_view disclaimer_name = "disclaimer";
			std::string notes;
			bool notes_is_set = false;
			static constexpr std::string_view notes_units = "";
			static constexpr std::string_view notes_description = "Additional Information";
			static constexpr std::string_view notes_name = "notes";
		};
		void from_json(const nlohmann::json& j, Metadata& x);
		void to_json(nlohmann::json& j, const Metadata& x);
	}
}
#endif
