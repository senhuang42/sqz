#include <vector>
#include <cstddef>

#include "OutBufferedBitFileStream.h"

#pragma once

class SubframeProcessor {
public:
    SubframeProcessor(int shift, int depth) : sample_shift_(shift), sample_depth_(depth) {}

    // This is just overloaded, shouldn't be called from SubframeProcessor directly ever
    virtual void encode(std::vector<long> &samples, OutBufferedBitFileStream &out) {
        std::cout << "oops" << std::endl;
    };

    // Returns a pair of SubframeProcessor and expected size
    // In practice, the expected size doesn't matter since we just use the highest order predictor for now
    static std::pair<SubframeProcessor*, long> calculateCoefs(std::vector<long>& samples, int sampleDepth);

    static int numExtraBits(std::vector<long> data);

protected:
    void putTypeShift(int type, OutBufferedBitFileStream &out);

    void putRawLong(long val, OutBufferedBitFileStream &out);

    int sample_shift_;
    int sample_depth_;
};