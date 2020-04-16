#include "SubframeProcessor.h"

#include <vector>
#include <cstddef>
#include <algorithm>
#include <numeric>

#include "MDP.h"
#include "LPC.h"

std::pair<SubframeProcessor*, long> SubframeProcessor::calculateCoefs(std::vector<long>& samples, int sampleDepth) {
    int shift = numExtraBits(samples);
    // Though we loop through order 2 to 12 LPCs, we're only gonna use the order 12 ones anyways
    int min = 2;
    int max = 12;
    int roundingVars = 0;
    MemorizedDotProd fdp(samples, max);
    long minSize = LONG_MAX;
    std::pair<SubframeProcessor*, long> result;
    
    for (int order = min; order <= max; ++order) {
        auto temp = LinearPredictiveEncoder::calculateCoefs(
            samples, shift, sampleDepth, order, std::min(roundingVars, order), fdp);
        if (temp.second <= minSize) {
            result.first = temp.first;
            result.second = temp.second;
            minSize = result.second;
        }
    }
    return result;
}

void SubframeProcessor::putTypeShift(int type, OutBufferedBitFileStream &out) {
    out.putVal(1, 0);
    out.putVal(6, type);

    if (sample_shift_ == 0) {
        out.putVal(1, 0);
    } else {
        out.putVal(1, 1);
        for (int i = 0; i < sample_shift_ - 1; ++i) {
            out.putVal(1, 0);
        }
        out.putVal(1, 1);
    }
}

void SubframeProcessor::putRawLong(long val, OutBufferedBitFileStream& out) {
    int width = sample_depth_ - sample_shift_;
    if (width <= 32) {
        out.putVal(width, (int)val);
    } else {
        out.putVal(1, (int)((unsigned long)val >> 32));
        out.putVal(32, (int)val);
    }
}

int SubframeProcessor::numExtraBits(std::vector<long> data) {
    long acc = std::accumulate(
        data.begin(),
        data.end(),
        0,
        [&](long a, long b) {
            return a |= b;
        }
    );

    if (acc == 0) {
        return 0;
    } else {
        return __builtin_ctzl(acc);
    }
}

