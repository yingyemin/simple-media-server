#ifndef DTS_GENERATOR_H
#define DTS_GENERATOR_H

#include <set>
#include <cstdint>

// dts生成器，  [AUTO-TRANSLATED:d8a794a2]
// dts generator,
// pts排序后就是dts  [AUTO-TRANSLATED:439ac368]
// pts after sorting is dts
class DtsGenerator{
public:
    bool getDts(uint64_t pts, uint64_t &dts);

private:
    bool getDts_l(uint64_t pts, uint64_t &dts);

private:
    uint64_t _dts_pts_offset = 0;
    uint64_t _last_dts = 0;
    uint64_t _last_pts = 0;
    uint64_t _last_max_pts = 0;
    size_t _frames_since_last_max_pts = 0;
    size_t _sorter_max_size = 0;
    size_t _count_sorter_max_size = 0;
    std::set<uint64_t> _pts_sorter;
};

#endif // DTS_GENERATOR_H
