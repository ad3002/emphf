#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string_view>
#include <unordered_set>

#include "common.hpp"
#include "perfutils.hpp"

namespace emphf {

    template <typename MPHF>
    int test_mphf_main(int argc, char** argv)
    {
        if (argc < 3) {
            std::cerr << "Expected: " << argv[0]
                      << " <values_filename> <hash_filename> [--check]" << std::endl;
            std::terminate();
        }

        const char* values_filename = argv[1];
        const char* hash_filename = argv[2];

        bool check = false;
        if (argc > 3 && std::string_view(argv[3]) == "--check") {
            logger() << "Will perform results checking (this affects avg. time)"
                     << std::endl;
            check = true;
        }

        logger() << "Testing " << values_filename << std::endl;

        // Load strings into memory for faster lookup
        std::vector<char> strings_pool;
        std::vector<size_t> string_endpoints;
        string_endpoints.push_back(0);

        {
            logger() << "Loading strings" << std::endl;
            file_lines lines(values_filename);
            for (const auto& s : lines) {
                strings_pool.insert(strings_pool.end(), s.begin(), s.end());
                string_endpoints.push_back(strings_pool.size());
            }
        }

        size_t test_strings = string_endpoints.size() - 1;
        logger() << "Loaded " << test_strings << " strings." << std::endl;

        // // Debug: Print the first few strings
        // for (size_t i = 0; i < std::min(test_strings, size_t(5)); ++i) {
        //     logger() << "String " << i << ": " << (strings_pool.data() + string_endpoints[i]) << std::endl;
        // }

        identity_adaptor adaptor;
        MPHF mphf;
        size_t file_size;
        {
            logger() << "Loading MPHF" << std::endl;
            std::ifstream is(hash_filename, std::ios::binary);
            if (!is) {
                logger() << "Error opening hash file: " << hash_filename << std::endl;
                return 3;
            }
            mphf.load(is);
            file_size = static_cast<size_t>(is.tellg());
        }

        size_t n = mphf.size();
        logger() << "MPHF size: " << n << std::endl;

        std::vector<uint64_t> all_lookups;
        if (check) {
            all_lookups.reserve(n);
        }

        const uint8_t* pool_base = reinterpret_cast<const uint8_t*>(strings_pool.data());

        logger() << "Performing base hashing (for reference)" << std::endl;
        double tick = get_time_usecs();
        for (size_t i = 0; i < test_strings; ++i) {
            byte_range_t s(pool_base + string_endpoints[i],
                           pool_base + string_endpoints[i + 1]);

            auto h = mphf.base_hasher()(adaptor(s));
            do_not_optimize_away(std::get<0>(h));
        }
        double elapsed = get_time_usecs() - tick;

        logger() << "Avg. " << elapsed / static_cast<double>(test_strings)
                 << " usecs per base hash computation" << std::endl;

        logger() << "Performing lookups" << std::endl;

        size_t runs = check ? 1 : 10;

        stats_accumulator stats;
        tick = get_time_usecs();
        size_t lookups = 0;
        static const size_t lookups_per_sample = 1 << 16;

        std::unordered_set<uint64_t> lookup_set;

        for (size_t run = 0; run < runs; ++run) {
            for (size_t i = 0; i < test_strings; ++i) {
                byte_range_t s(pool_base + string_endpoints[i],
                               pool_base + string_endpoints[i + 1]);

                uint64_t h = mphf.lookup(s, adaptor);
                do_not_optimize_away(h);

                if (check) {
                    if (h >= n) {
                        logger() << "ERROR: value out of bounds "
                                 << h << std::endl;
                        return 2;
                    }
                    all_lookups.push_back(h);
                    lookup_set.insert(h);
                }

                if (++lookups == lookups_per_sample) {
                    elapsed = get_time_usecs() - tick;
                    stats.add(elapsed / static_cast<double>(lookups));
                    tick = get_time_usecs();
                    lookups = 0;
                }
            }
        }

        logger() << "Avg. " << stats.mean()
                 << " usecs per lookup" << std::endl;

        if (check) {
            logger() << "Checking hash output" << std::endl;
            std::sort(all_lookups.begin(), all_lookups.end());
            auto distinct_lookups = static_cast<size_t>(std::distance(all_lookups.begin(),
                                                                     std::unique(all_lookups.begin(),
                                                                                 all_lookups.end())));
            logger() << "Number of unique lookups: " << lookup_set.size() << std::endl;
            if (distinct_lookups == n) {
                logger() << "OK" << std::endl;
            } else {
                logger() << "Expected " << n << " distinct values, got "
                         << distinct_lookups << std::endl;
                return 1;
            }
        }

        double bits_per_key = 8.0 * static_cast<double>(file_size) / static_cast<double>(mphf.size());
        std::cout << "avg_lookup_time\t" << stats.mean() << std::endl
                  << "stddev_lookup_time_percentage\t"
                  << stats.relative_stddev() << std::endl
                  << "bits_per_key\t" << bits_per_key << std::endl;

        return 0;
    }

}
