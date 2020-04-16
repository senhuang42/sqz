#include "decode.h"
#include "lib/fse.h"

#include <cmath>
#include <vector>
#include <algorithm>

using namespace std;

// Util functions for writing to file buffer
static void putStr(std::string s, OutBufferedBitFileStream& outputFileStream) {
    std::for_each(s.begin(), s.end(), [&](char c){ outputFileStream.putVal(8, c); });
}

static void putLEInt(int nBytes, int val, OutBufferedBitFileStream& outputFileStream) {
    for (int i = 0; i < nBytes; i++) {
		outputFileStream.putVal(8, (unsigned int)val >> (i * 8));
    }
}

// Output the WAV header
static void writeWavHeaderBlock(OutBufferedBitFileStream& outputFileStream,
                                long sampleSize, int nChannels, int sampleRate, int sampleDepth) {
    putStr("RIFF", outputFileStream);
    putLEInt(4, (int)sampleSize + 36, outputFileStream);
    putStr("WAVE", outputFileStream);
    putStr("fmt ", outputFileStream);
    putLEInt(4, 16, outputFileStream);
    putLEInt(2, 0x0001, outputFileStream);
    putLEInt(2, nChannels, outputFileStream);
    putLEInt(4, sampleRate, outputFileStream);
    putLEInt(4, sampleRate * nChannels * (sampleDepth / 8), outputFileStream);
    putLEInt(2, nChannels * (sampleDepth / 8), outputFileStream);
    putLEInt(2, sampleDepth, outputFileStream);
    putStr("data", outputFileStream);
    putLEInt(4, (int)sampleSize, outputFileStream);
}

void Decoder::Decode(std::string inFileName, std::string outFileName) {
    OutBufferedBitFileStream outputFileStream(outFileName);
    InBufferedBitFileStream inputFileStream(inFileName);
    if (inputFileStream.readUnsignedInt(32) != SQZ_MAGIC_NUM){
		throw std::runtime_error("Sqz magic num invalid");
    }

    int sampleRate, nChannels, sampleDepth;
    long nSamples;
    sampleRate = nChannels = sampleDepth = nSamples = -1;
    bool last = false;

    // extract this into a function
    // read inputFileStream and throw away a bunch of FLAC-style
    // header info (which we may want to use inputFileStream the future)
    while (!last) {
        last = inputFileStream.readUnsignedInt(1) != 0;
        int type = inputFileStream.readUnsignedInt(7);
        if (type == 0) {
            sampleRate = inputFileStream.readUnsignedInt(20);
            nChannels = inputFileStream.readUnsignedInt(3) + 1;
            sampleDepth = inputFileStream.readUnsignedInt(5) + 1;
            nSamples = (long)inputFileStream.readUnsignedInt(18) << 18 | inputFileStream.readUnsignedInt(18);
        } else {
            throw std::runtime_error("bad type");
        }
    }

    if (sampleRate == -1) {
		throw std::runtime_error("bad sample rate");
    }

    if (sampleDepth % 8 != 0) {
        throw std::runtime_error("bad sample depth");
    }

    writeWavHeaderBlock(outputFileStream, nSamples * nChannels * (sampleDepth / 8),
                        nChannels, sampleRate, sampleDepth);
    // decode frame by frame
    for (int i = 0; decodeFrame(inputFileStream, nChannels, sampleDepth, outputFileStream); ++i) {
        //printf("Decoded frame: %d\n", i);
    }
    
    outputFileStream.flush();
}

bool Decoder::decodeFrame(InBufferedBitFileStream& inputFileStream, int nChannels, int sampleDepth, OutBufferedBitFileStream& outputFileStream) {
    int code = inputFileStream.readOneByte();
    // we're done decoding here
    if (code == -1) {
        return false;
    }

    if (((unsigned)code <<  6 | inputFileStream.readUnsignedInt(6)) != 0x3FFE) {
        throw std::runtime_error("sync error\n");
    }
    
    inputFileStream.readUnsignedInt(2); // just to round out the byte
    int blockSizeCode = inputFileStream.readUnsignedInt(4);
    int sampleRateCode = inputFileStream.readUnsignedInt(4);
    int channelCode = inputFileStream.readUnsignedInt(4);
    inputFileStream.readUnsignedInt(3); // reads sample depth, not useful since we don't support 32bit+ yet
    inputFileStream.readUnsignedInt(1); // just to round out the byte

    int bytesToRead = __builtin_clz(~(inputFileStream.readUnsignedInt(8) << 24)) - 1;
    for (int i = 0; i < bytesToRead; i++){
		inputFileStream.readUnsignedInt(8); // again, just consume these for now
    }

    int blockSize;
    if (blockSizeCode == 1) {
        blockSize = 192;
    } else if (2 <= blockSizeCode && blockSizeCode <= 5) {
        blockSize = 576 << (blockSizeCode - 2);
    } else if (blockSizeCode == 6) {
        blockSize = inputFileStream.readUnsignedInt(8) + 1;
    } else if (blockSizeCode == 7) {
        blockSize = inputFileStream.readUnsignedInt(16) + 1;
    } else if (8 <= blockSizeCode && blockSizeCode <= 15) {
        blockSize = 256 << (blockSizeCode - 8);
    } else {
        throw std::runtime_error("bad block size");
    }
    
    // discard sample rates, we don't support really high ones/don't use this for now
    if (sampleRateCode == 12) {
        inputFileStream.readUnsignedInt(8);
    } else if (sampleRateCode == 13 || sampleRateCode == 14) {
        inputFileStream.readUnsignedInt(16);
    }
    
    inputFileStream.readUnsignedInt(8);
    vector<vector<int>> samples(nChannels, vector<int>(blockSize));
    decodeAllSubframes(inputFileStream, sampleDepth, channelCode, samples);
    inputFileStream.align();
    inputFileStream.readUnsignedInt(16);

    // output the decoded samples
    for (int i = 0; i < blockSize; i++) {
        for (int j = 0; j < nChannels; j++) {
            int val = samples[j][i];
            if (sampleDepth == 8) {
                val += 128;
            }
            putLEInt(sampleDepth / 8, val, outputFileStream);
        }
    }
    return true;
}

void Decoder::decodeAllSubframes(InBufferedBitFileStream& inputFileStream, int sampleDepth, int channelCode, vector<vector<int>>& result) {
    int blockSize = result[0].size();
    vector<vector<long>> subframes(result.size(), vector<long>(blockSize));
    if (channelCode <= 7) {
        for (int i = 0; i < result.size(); i++) {
            decodeSubframe(inputFileStream, sampleDepth, subframes[i]);
        }
    } else {
        throw std::runtime_error("stereo decoding not supported");
    }

    for (int i = 0; i < result.size(); i++) {
        for (int j = 0; j < blockSize; j++) {
            result[i][j] = (int)subframes[i][j];
        }
    }
}

void Decoder::decodeSubframe(InBufferedBitFileStream& inputFileStream, int sampleDepth, vector<long>& result) {
    inputFileStream.readUnsignedInt(1); // discard first bit
    int type = inputFileStream.readUnsignedInt(6);
    int shift = inputFileStream.readUnsignedInt(1);
    if (shift == 1) {
        while (inputFileStream.readUnsignedInt(1) == 0) {
            ++shift;
        }
    }
    sampleDepth -= shift;
    
    // We use the same "type" scheme as FLAC - to allow future exensibility for verbatim/fixed encodings
    if (32 <= type && type <= 63) {
        decodeLPC(inputFileStream, type - 31, sampleDepth, result);
    } else {
        throw std::runtime_error("Bad type (how did this happen?)");
    }
    
    std::for_each(result.begin(), result.end(), [&](auto& element){ element <<= shift; });
}

void Decoder::decodeLPC(InBufferedBitFileStream& inputFileStream, int lpcOrder, int sampleDepth, vector<long>& result) {
    for (int i = 0; i < lpcOrder; i++) {
        result[i] = inputFileStream.readSignedInt(sampleDepth);
    }

    int prec = inputFileStream.readUnsignedInt(4) + 1;
    int shift = inputFileStream.readSignedInt(5);
    vector<int> coefs(lpcOrder);
    for (int i = 0; i < coefs.size(); i++) {
        coefs[i] = inputFileStream.readSignedInt(prec);
    }

    decodeFseBlockToLPCErrors(inputFileStream, lpcOrder, result);
    calculateOriginalSignal(result, coefs, shift);
}

void Decoder::decodeFseBlockToLPCErrors(InBufferedBitFileStream& inputFileStream, int warmup, vector<long>& result) {
    int numPartitions = 1 << inputFileStream.readUnsignedInt(4);
    int partitionSize = result.size() / numPartitions;

    for (int i = 0; i < numPartitions; i++) {
        int start = i * partitionSize + (i == 0 ? warmup : 0);
        int end = (i + 1) * partitionSize;
        int size = inputFileStream.readUnsignedInt(24);
        
        char* fseEncodedBuf = (char*)malloc(sizeof(char)*size);
        if (!fseEncodedBuf) {
            throw std::runtime_error("Malloc failed somehow");
        }
        inputFileStream.readIntoBuf(fseEncodedBuf, size);
        int16_t* fseDecodedBuf = (int16_t*)malloc(sizeof(int16_t)*size*8);  // probably won't compress more than 8x
        if (!fseDecodedBuf) {
            throw std::runtime_error("Malloc failed somehow");
        }
        FSE_decompress(fseDecodedBuf, sizeof(int16_t)*size*8, fseEncodedBuf, size);

        for (int j = start, i = 0; j < end; ++j) {
            result[j] = fseDecodedBuf[i++];
        }
        
        free(fseEncodedBuf);
        free(fseDecodedBuf);
    }
}

void Decoder::calculateOriginalSignal(vector<long>& result, vector<int>& coefs, int shift) {
    for (size_t i = coefs.size(); i < result.size(); i++) {
        long sum = 0;
        for (size_t j = 0; j < coefs.size(); j++) {
            sum += result[i - 1 - j] * coefs[j];
        }
        result[i] += sum >> shift;
    }
}