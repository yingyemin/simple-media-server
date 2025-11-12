#include "DtsGenerator.h"
#include "Log/Logger.h"

// copy from https://github.com/zlmediakit

bool DtsGenerator::getDts(uint64_t pts, uint64_t &dts) {
    bool ret = false;
    if (pts == _last_pts) {
        // pts未变，说明dts也不会变，返回上次dts  [AUTO-TRANSLATED:dc0972e0]
        // pts does not change, indicating that dts will not change, return the last dts
        if (_last_dts) {
            dts = _last_dts;
            ret = true;
        }
    } else {
        // pts变了，尝试计算dts  [AUTO-TRANSLATED:f527d0f6]
        // pts changed, try to calculate dts
        ret = getDts_l(pts, dts);
        if (ret) {
            // 获取到了dts，保存本次结果  [AUTO-TRANSLATED:d6a5ce6d]
            // Get the dts, save the current result
            _last_dts = dts;
        }
    }

    if (!ret) {
        // pts排序列队长度还不知道，也就是不知道有没有B帧，  [AUTO-TRANSLATED:e5ad4327]
        // The pts sorting queue length is not yet known, that is, it is not known whether there is a B frame,
        // 那么先强制dts == pts，这样可能导致有B帧的情况下，起始画面有几帧回退  [AUTO-TRANSLATED:74c97de1]
        // Then force dts == pts first, which may cause the starting picture to have a few frames rollback in the case of B frames
        dts = pts;
    }

    // 记录上次pts  [AUTO-TRANSLATED:4ecd474b]
    // Record the last pts
    _last_pts = pts;
    return ret;
}

// 该算法核心思想是对pts进行排序，排序好的pts就是dts。  [AUTO-TRANSLATED:efb36e04]
// The core idea of this algorithm is to sort the pts, and the sorted pts is the dts.
// 排序有一定的滞后性，那么需要加上排序导致的时间戳偏移量  [AUTO-TRANSLATED:5ada843a]
// Sorting has a certain lag, so it is necessary to add the timestamp offset caused by sorting
bool DtsGenerator::getDts_l(uint64_t pts, uint64_t &dts) {
    if (_sorter_max_size == 1) {
        // 没有B帧，dts就等于pts  [AUTO-TRANSLATED:9cfae4ea]
        // There is no B frame, dts is equal to pts
        dts = pts;
        return true;
    }

    if (!_sorter_max_size) {
        // 尚未计算出pts排序列队长度(也就是P帧间B帧个数)  [AUTO-TRANSLATED:8bedb754]
        // The length of the pts sorting queue (that is, the number of B frames between P frames) has not been calculated yet
        if (pts > _last_max_pts) {
            // pts时间戳增加了，那么说明这帧画面不是B帧(说明是P帧或关键帧)  [AUTO-TRANSLATED:4c5ef2b8]
            // The pts timestamp has increased, which means that this frame is not a B frame (it means it is a P frame or a key frame)
            if (_frames_since_last_max_pts && _count_sorter_max_size++ > 0) {
                // 已经出现多次非B帧的情况，那么我们就能知道P帧间B帧的个数  [AUTO-TRANSLATED:fd747b3c]
                // There have been multiple non-B frames, so we can know the number of B frames between P frames
                _sorter_max_size = _frames_since_last_max_pts;
                // 我们记录P帧间时间间隔(也就是多个B帧时间戳增量累计)  [AUTO-TRANSLATED:66c0cc14]
                // We record the time interval between P frames (that is, the cumulative increment of multiple B frame timestamps)
                _dts_pts_offset = (pts - _last_max_pts);
                // 除以2，防止dts大于pts  [AUTO-TRANSLATED:52b5b3ab]
                // Divide by 2 to prevent dts from being greater than pts
                _dts_pts_offset /= 2;
            }
            // 遇到P帧或关键帧，连续B帧计数清零  [AUTO-TRANSLATED:537bb54d]
            // When encountering a P frame or a key frame, the continuous B frame count is cleared
            _frames_since_last_max_pts = 0;
            // 记录上次非B帧的pts时间戳(同时也是dts)，用于统计连续B帧时间戳增量  [AUTO-TRANSLATED:194f8cdb]
            // Record the pts timestamp of the last non-B frame (which is also dts), used to count the continuous B frame timestamp increment
            _last_max_pts = pts;
        }
        // 如果pts时间戳小于上一个P帧，那么断定这个是B帧,我们记录B帧连续个数  [AUTO-TRANSLATED:1a7e33e2]
        // If the pts timestamp is less than the previous P frame, then it is determined that this is a B frame, and we record the number of consecutive B frames
        ++_frames_since_last_max_pts;
    }

    // pts放入排序缓存列队，缓存列队最大等于连续B帧个数  [AUTO-TRANSLATED:ff598a97]
    // Put pts into the sorting cache queue, the maximum cache queue is equal to the number of consecutive B frames
    _pts_sorter.emplace(pts);

    // 对前面的_sorter_max_size个pts排序结果，取出最早的那个，作为该帧的dts基准  [AUTO-TRANSLATED:6632922a]
    // Take the earliest pts sorting result of the previous _sorter_max_size pts, and use it as the dts baseline for this frame
    // 因为dts是递增的，可以将排序后的pts当作dts用，加上时间偏移量是为了避免dts和pts相差太大  [AUTO-TRANSLATED:74c97de1]
    // Since dts is incremented, we can use the sorted pts as dts, and add the timestamp offset to avoid the difference between dts and pts being too large
    if (_sorter_max_size && _pts_sorter.size() > _sorter_max_size) {
        // 如果启用了pts排序(意味着存在B帧)，并且pts排序缓存列队长度大于连续B帧个数，  [AUTO-TRANSLATED:002c0d03]
        // If pts sorting is enabled (meaning there are B frames), and the length of the pts sorting cache queue is greater than the number of consecutive B frames,
        // 意味着后续的pts都会比最早的pts大，那么说明可以取出最早的pts了，这个pts将当做该帧的dts基准  [AUTO-TRANSLATED:86b8f679]
        // It means that the subsequent pts will be larger than the earliest pts, which means that the earliest pts can be taken out, and this pts will be used as the dts baseline for this frame
        auto it = _pts_sorter.begin();

        // 由于该pts是前面偏移了个_sorter_max_size帧的pts(也就是那帧画面的dts),  [AUTO-TRANSLATED:eb3657aa]
        // Since this pts is the pts of the previous _sorter_max_size frames (that is, the dts of that frame),
        // 那么我们加上时间戳偏移量，基本等于该帧的dts  [AUTO-TRANSLATED:245aac6e]
        // Then we add the timestamp offset, which is basically equal to the dts of this frame
        dts = *it + _dts_pts_offset;
        if (dts > pts) {
            // dts不能大于pts(基本不可能到达这个逻辑)  [AUTO-TRANSLATED:847c4531]
            // dts cannot be greater than pts (it is basically impossible to reach this logic)
            dts = pts;
        }

        // pts排序缓存出列  [AUTO-TRANSLATED:8b580191]
        // pts sorting cache dequeue
        _pts_sorter.erase(it);
        return true;
    }

    // 排序缓存尚未满  [AUTO-TRANSLATED:3f502460]
    // The sorting cache is not full yet
    return false;
}