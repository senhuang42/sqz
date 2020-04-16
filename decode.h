#include <cmath>
#include <string>
#include <iostream>

#include "OutBufferedBitFileStream.h"
#include "InBufferedBitFileStream.h"

class Decoder {
public:
    static void Decode(std::string inFileName, std::string outFileName);

private:
    // decoding is much more straightforward: cascade down through these functions
    static bool decodeFrame(InBufferedBitFileStream& in, int numChannels, int sampleDepth, OutBufferedBitFileStream& out);

    static void decodeAllSubframes(InBufferedBitFileStream& in, int sampleDepth, int chanAsgn, std::vector<std::vector<int>>& result);

    static void decodeSubframe(InBufferedBitFileStream& in, int sampleDepth, std::vector<long>& result);

    static void decodeLPC(InBufferedBitFileStream& in, int lpcOrder, int sampleDepth, std::vector<long>& result);

    static void decodeFseBlockToLPCErrors(InBufferedBitFileStream& in, int warmup, std::vector<long>& result);

    static void calculateOriginalSignal(std::vector<long>& result, std::vector<int>& coefs, int shift);
};