/*
 Copyright 2016 Nervana Systems Inc.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#pragma once

#include "bstream.hpp"

namespace nervana
{
    namespace tiff
    {
        class file_header;
        class directory_entry;

        enum class data_type
        {
            BYTE = 1,
            ASCII = 2,
            SHORT = 3,
            LONG = 4,
            RATIONAL = 5,
            SBYTE = 6,
            UNDEFINED = 7,
            SSHORT = 8,
            SLONG = 9,
            SRATIONAL = 10,
            FLOAT = 11,
            DOUBLE = 12
        };

        bool is_tiff(const char* data, size_t size);
    }
}

class nervana::tiff::file_header
{
public:
    file_header(bstream_base& bs);

    uint16_t    byte_order;
    uint16_t    file_id;
    uint32_t    ifd_offset;
};

class nervana::tiff::directory_entry
{
public:
    directory_entry(bstream_base& bs);

    uint16_t    tag;
    uint16_t    type;
    uint32_t    count;
    uint32_t    value_offset;
};
