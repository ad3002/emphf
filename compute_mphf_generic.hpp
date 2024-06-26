#pragma once

#include <iostream>
#include <fstream>
#include <iterator>
#include <random>
#include <cmath>

#include "common.hpp"
#include "mphf.hpp"
#include "base_hash.hpp"
#include "perfutils.hpp"

namespace emphf {

    template <typename HypergraphSorter32,
              typename HypergraphSorter64,
              typename BaseHasher>
    int compute_mphf_main(int argc, char** argv)
    {
        if (argc < 2) {
            std::cerr << "Expected: " << argv[0] << " <filename> [output_filename]" << std::endl;
            std::terminate();
        }

        const char* filename = argv[1];
        std::string output_filename;
        if (argc >= 3) {
            output_filename = argv[2];
        }

        logger() << "Processing " << filename << std::endl;

        file_lines lines(filename);
        size_t n = lines.size();
        logger() << n << " strings to process." << std::endl;

        stl_string_adaptor adaptor;
        using mphf_t = mphf<BaseHasher>;
        mphf_t mphf;

        size_t max_nodes = (static_cast<size_t>(std::ceil(static_cast<double>(n) * 1.23)) + 2) / 3 * 3;
        if (max_nodes >= uint64_t(1) << 32) {
            logger() << "Using 64-bit sorter" << std::endl;
            HypergraphSorter64 sorter;
            mphf_t mphf_instance(sorter, n, lines, adaptor);
            mphf_instance.swap(mphf);
        } else {
            logger() << "Using 32-bit sorter" << std::endl;
            HypergraphSorter32 sorter;
            mphf_t mphf_instance(sorter, n, lines, adaptor);
            mphf_instance.swap(mphf);
        }

        if (!output_filename.empty()) {
            std::ofstream os(output_filename, std::ios::binary);
            mphf.save(os);
        }

        return 0;
    }
}
