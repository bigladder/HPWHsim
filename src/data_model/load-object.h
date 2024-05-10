#ifndef LOAD_OBJECT_H_
#define LOAD_OBJECT_H_

#include <fstream>
#include <vector>
#include <courier/courier.h>
#include <nlohmann/json.hpp>

namespace hpwh_data_model {

    inline void read_binary_file(const char *filename, std::vector<char> &bytes)
    {
        std::ifstream is(filename, std::ifstream::binary);
        if (is) {
            // get length of file:
            is.seekg(0, is.end);
            size_t length = static_cast<size_t>(is.tellg());
            is.seekg(0, is.beg);

            bytes.resize(length);
            // read data as a block:
            is.read(bytes.data(), length);

            is.close();
        }
    }

    inline nlohmann::json load_json(const char *input_file)
    {
        std::string filename(input_file);
        std::string::size_type idx = filename.rfind('.');

        using namespace nlohmann;

        json j;

        if (idx != std::string::npos) {
            std::string extension = filename.substr(idx + 1);

            if (extension == "cbor") {
                std::vector<char> bytearray;
                read_binary_file(input_file, bytearray);
                j = json::from_cbor(bytearray);
            }
            else if (extension == "json") {
                std::string schema(input_file);
                std::ifstream in(schema);
                in >> j;
            }
        }
        return j;
    }

    template<class T>
    void json_get(nlohmann::json j,
                  Courier::Courier& logger,
                  const char *subnode,
                  T& object,
                  bool& object_is_set,
                  bool required = false)
    {
		try
        {
            object = j.at(subnode).get<T>();
            object_is_set = true;
        }
		catch (nlohmann::json::out_of_range & ex)
        {
            object_is_set = false;
            if (required)
            {
                logger.send_warning(ex.what());
            }
        }
    }


}

#endif
