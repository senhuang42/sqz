#include <vector>

#include "OutBufferedBitFileStream.h"

#pragma once

class FseEncoder {
public:
    static void encode(std::vector<long>& data, int warmup, int order, OutBufferedBitFileStream& out);

private:
    static void encodeData(std::vector<long>& data, int start, int end, int param, OutBufferedBitFileStream& out);
};