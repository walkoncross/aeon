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

#include "csv_manifest_maker.hpp"

#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <cstdio>
#include <unistd.h>

using namespace std;

manifest_maker::manifest_maker(uint32_t num_records, std::vector<uint32_t> sizes)
{
    manifest_name = tmp_manifest_file(num_records, sizes);
}

manifest_maker::manifest_maker()
{
}

manifest_maker::~manifest_maker()
{
    remove_files();
}

std::string manifest_maker::get_manifest_name()
{
    return manifest_name;
}


void manifest_maker::remove_files()
{
    for(auto it : tmp_filenames)
    {
        remove(it.c_str());
    }
}

string manifest_maker::tmp_filename()
{
    char *tmpname = strdup("/tmp/tmpfileXXXXXX");

    // mkstemp opens the file with open() so we need to close it
    close(mkstemp(tmpname));

    tmp_filenames.push_back(tmpname);
    return tmpname;
}

string manifest_maker::tmp_manifest_file(uint32_t num_records, vector<uint32_t> sizes)
{
    string tmpname = tmp_filename();
    ofstream f(tmpname);

    for(uint32_t i = 0; i < num_records; ++i) {
        // stick a unique uint32_t into each file
        for(uint32_t j = 0; j < sizes.size(); ++j) {
            if(j != 0) {
                f << ",";
            }

            f << tmp_file_repeating(sizes[j], (i * sizes.size()) + j);
        }
        f << endl;
    }

    f.close();

    return tmpname;
}

string manifest_maker::tmp_file_repeating(uint32_t size, uint32_t x)
{
    // create a temp file of `size` bytes filled with uint32_t x
    string tmpname = tmp_filename();
    ofstream f(tmpname, ios::binary);

    uint32_t repeats = size / sizeof(x);
    for(uint32_t i = 0; i < repeats; ++i) {
        f.write(reinterpret_cast <const char*>(&x), sizeof(x));
    }

    f.close();

    return tmpname;
}

std::string manifest_maker::tmp_manifest_file_with_invalid_filename()
{
    string tmpname = tmp_filename();
    ofstream f(tmpname);

    for(uint32_t i = 0; i < 10; ++i) {
        f << tmp_filename() + ".this_file_shouldnt_exist" << ',';
        f << tmp_filename() + ".this_file_shouldnt_exist" << endl;
    }

    f.close();
    return tmpname;
}

std::string manifest_maker::tmp_manifest_file_with_ragged_fields()
{
    string tmpname = tmp_filename();
    ofstream f(tmpname);

    for(uint32_t i = 0; i < 10; ++i) {
        for(uint32_t j = 0; j < i % 3 + 1; ++j) {
            f << (j != 0 ? "," : "") << tmp_filename();
        }
        f << endl;
    }

    f.close();
    return tmpname;
}
