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

#include <stdexcept>
#include <iostream>

#include "tiff.hpp"
#include "util.hpp"

using namespace std;
using namespace nervana;
using namespace nervana::tiff;

bool nervana::tiff::is_tiff(const char* data, size_t size)
{
    bool rc = true;
    bstream_mem bs{data, size};

    auto byte_order = bs.readU16();
    if(byte_order == 0x4949)
    {
        bs.set_endian(bstream_base::endian_t::LITTLE);
    }
    else if(byte_order == 0x4D4D)
    {
        bs.set_endian(bstream_base::endian_t::BIG);
    }
    else
    {
        rc = false;
    }
    auto file_id = bs.readU16();
    rc &= file_id == 42;
    return rc;
}

file_header::file_header(bstream_base& bs)
{
    byte_order = bs.readU16();
    if(byte_order == 0x4949)
    {
        bs.set_endian(bstream_base::endian_t::LITTLE);
    }
    else if(byte_order == 0x4D4D)
    {
        bs.set_endian(bstream_base::endian_t::BIG);
    }
    else
    {
        throw runtime_error("file not tiff" );
    }
    file_id    = bs.readU16();
    ifd_offset = bs.readU32();
}

directory_entry::directory_entry(bstream_base& bs)
{
    uint16_t    tag;
    uint16_t    type;
    uint32_t    count;
    uint32_t    value_offset;
}

reader::reader(const char* data, size_t size) :
    bstream{data, size},
    header{bstream}
{
    dump(cout, data, 128);
    cout << __FILE__ << " " << __LINE__ << " " << hex << header.byte_order << dec << endl;
    cout << __FILE__ << " " << __LINE__ << " " << header.file_id    << endl;
    cout << __FILE__ << " " << __LINE__ << " " << header.ifd_offset << endl;
    bstream.seek(header.ifd_offset);
}
