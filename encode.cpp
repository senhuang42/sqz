#include "encode.h"

#include <iostream>
#include <cmath>
#include <string>
#include <cstddef>
#include <exception>

#include "string.h"
#include "decode.h"

#include <math.h>
#include <vector>

using namespace std;

// Note that it's a non null-terminated string
static std::string readStr(FILE* in, int len) {
    char* charBuf = (char*)malloc(sizeof(char)*len);
    if (!charBuf) {
        throw std::runtime_error("Malloc failed somehow");
    }
    fread(charBuf, sizeof(char), len, in);
    std::string str(charBuf, len);
    free(charBuf);
    return str;
}

// reads little endian
static int read16Or32BitUInt(FILE* in, int len) {
    if (len == 4) {
        uint32_t res;
        fread(&res, 4, 1, in);
        ntohs(res); // supposed to correct endianness
        return res;
    } else if (len == 2) {
        uint16_t res;
        fread(&res, 2, 1, in);
        ntohs(res);
        return res;
    } else {
        // Q: why is this function not just two functions? and why is there a
        //    length param if you only allow 4 or 2 bytes?
        // A: just because
        throw std::runtime_error("only 16 or 32 bits allowed");
    }
}

void Encoder::Encode(string infile, string outfile) {
    FILE *in = fopen(infile.c_str(), "rb");
    if (!in) {
        throw std::runtime_error("Invalid/no file\n");
    }

    // Interpret the header, read in necessary info
    if (readStr(in, 4) != "RIFF") {
        throw std::runtime_error("RIFF file header missing");
    }
    read16Or32BitUInt(in, 4);  // won't use these bytes
    if (readStr(in, 4) != "WAVE") {
        throw std::runtime_error("WAVE file header missing");
    }
    if (readStr(in, 4) != "fmt ") {
        throw std::runtime_error("RIFF file header missing");
    }
    if (read16Or32BitUInt(in, 4) != 16) {
        throw std::runtime_error("Bad wave file type");
    }
    if (read16Or32BitUInt(in, 2) != 0x0001) {
        throw std::runtime_error("Wave codec not supported");
    }
    int channelCount = read16Or32BitUInt(in, 2);
    int sampleRate = read16Or32BitUInt(in, 4);
    int byteRate = read16Or32BitUInt(in, 4);
    int blockAlignVal = read16Or32BitUInt(in, 2);
    int sampleDepth = read16Or32BitUInt(in, 2);
    int bytesPerSample = sampleDepth / 8;
    if (bytesPerSample > 2) {
        throw std::runtime_error("Sqz doesn't support higher than 16-bit audio for now");
    }

    // complain about bad values here
    if (channelCount < 0 || channelCount > 1) {
        throw std::runtime_error("Sqz only supports mono audio (for now)");
    }
    if (sampleRate <= 0 || sampleRate >= (1 << 20)) {
        throw std::runtime_error("Sample rate too high or too low");
    }
    if (sampleDepth == 0 || sampleDepth > 32 || sampleDepth % 8 != 0) {
        throw std::runtime_error("Sample depth no good");
    }
    if (bytesPerSample * channelCount != blockAlignVal) {
        throw std::runtime_error("Block align value bad");
    }
    if (bytesPerSample * channelCount * sampleRate != byteRate) {
        throw std::runtime_error("Bad byte rate");
    }
    
    // deal with data
    if (readStr(in, 4) != "data")
        throw std::runtime_error("WAV file chunk bad");
    int sampleDataLength = read16Or32BitUInt(in, 4);
    if (sampleDataLength <= 0 || sampleDataLength % (channelCount * bytesPerSample) != 0) {
        throw std::runtime_error("Audio length bad");
    }
    int numSamples = sampleDataLength / (channelCount * bytesPerSample);
    vector<vector<int>> samples(channelCount, vector<int>(numSamples));

    // read all of our samples into samples[][]
    for (int i = 0; i < numSamples; i++) {
        // loop through all channels (even though we don't allow multichannel)
        for (int j = 0; j < channelCount; j++) {
            int val = read16Or32BitUInt(in, bytesPerSample);
            if (sampleDepth == 8) {
                val -= 128;
            } else {
                val = (val << (32 - sampleDepth)) >> (32 - sampleDepth);
            }
            samples[j][i] = val;
        }
    }

    OutBufferedBitFileStream outFilestream(outfile);
    outFilestream.putVal(32, SQZ_MAGIC_NUM);
    
    StreamHeader streamHeader;
    streamHeader.sample_rate_ = sampleRate;
    streamHeader.num_channels_ = samples.size();
    streamHeader.sample_depth_ = sampleDepth;
    streamHeader.num_samples_ = samples[0].size();
    streamHeader.write(true, outFilestream);
    outFilestream.flush();

    // Use larger block size for better FSE encoding
    DataEncoder flac(streamHeader, samples, 8192*8, outFilestream);
    outFilestream.flush();

    // rewrite the info into our file buffer
    outFilestream.seek(4);
    streamHeader.write(true, outFilestream);
    outFilestream.flush();
    fclose(in);
}

// Note: these are the same sample code maps used by FLAC
vector<vector<int>> FrameHeader::BLOCK_SIZE_MAP = {
    {192,  1}, {576,  2}, {1152,  3}, {2304,  4}, {4608,  5},
    {256,  8}, {512,  9}, {1024, 10}, {2048, 11}, {4096, 12},
    {8192, 13}, {16384, 14}, {32768, 15},
};

vector<vector<int>> FrameHeader::SAMPLE_DEPTH_MAP = {
    { 8, 1}, {12, 2}, {16, 4}, {20, 5}, {24, 6},
};  

vector<vector<int>> FrameHeader::SAMPLE_RATE_MAP = {
    {88200,  1}, {176400,  2}, {192000,  3}, {8000,  4},
    {16000,  5}, {22050,  6}, {24000,  7}, {32000,  8},
    {44100,  9}, {48000, 10}, {96000, 11},
};

FrameHeader::FrameHeader() : frame_index_(-1), sample_offset_(-1),
                             num_channels_(-1), channel_assignment_(-1),
                             block_size_(-1), sample_rate_(-1),
                             sample_depth_(-1), frame_size_(-1) {}  

int FrameHeader::getBlockSize(int blockSize) {
    int result = findFirst(FrameHeader::BLOCK_SIZE_MAP, blockSize);
    if (result != -1) {
        return result;
    } else if (1 <= blockSize && blockSize <= 256) {
        return 6;
    } else if (1 <= blockSize && blockSize <= 65536) {
        return 7;
    } 

    return result;
}

int FrameHeader::getSampleRate(int sampleRate) {
    int result = findFirst(SAMPLE_RATE_MAP, sampleRate);
    if (result != -1){
        return result;
    } else if (0 <= sampleRate && sampleRate < 256) {
        return 12;
    }  else if (0 <= sampleRate && sampleRate < 65536) {
        return 13;
    } else if (0 <= sampleRate && sampleRate < 655360 && sampleRate % 10 == 0) {
        return 14;
    } else {
        return 0;
    }
}   

int FrameHeader::getSampleDepth(int sampleDepth) {
    int result = findFirst(SAMPLE_DEPTH_MAP, sampleDepth);
    if (result == -1) {
        return 0;
    }
    return result;
}

void FrameProcessor::encode(vector<vector<long>> &samples, OutBufferedBitFileStream& out) {
    // Check arguments
    frame_header_.writeHeader(out);
    
    int assignedChannel = frame_header_.channel_assignment_;
    if (0 <= assignedChannel && assignedChannel <= 7) {
        for (size_t i = 0; i < samples.size(); i++) {
            subframe_encoders[i]->encode(samples[i], out);
        }
    }

    out.align();
    out.putVal(16, out.Crc16());
}

vector<vector<long>> DataEncoder::getRange(vector<vector<int>>& array, int offset, int n) {
    vector<vector<long>> result(array.size(), vector<long>(n));
    for (size_t i = 0; i < array.size(); i++) {
        vector<int> src = array[i];
        vector<long> dst = result[i];
        for (int j = 0; j < n; j++) {
            dst[j] = src[offset + j];
        }
        result[i] = dst;
    }
    return result;
}

DataEncoder::DataEncoder(StreamHeader& streamHeader, vector<vector<int>>& samples, int blockSize, OutBufferedBitFileStream &out) {
    streamHeader.min_block_size_ = blockSize;
    streamHeader.max_block_size_ = blockSize;
    streamHeader.min_frame_size_ = streamHeader.max_frame_size_ = 0;
    for (size_t i = 0, pos = 0; pos < samples[0].size(); i++) {
        int n = min(samples[0].size() - pos, (unsigned long)blockSize);
        vector<vector<long>> subsamples = getRange(samples, pos, n);

        // perform an initial run of LPC
        FrameProcessor* enc = FrameProcessor::calculateCoefs(pos, subsamples, streamHeader.sample_depth_, streamHeader.sample_rate_).first;
        long startByte = out.numBytesWritten();

        // encode it all!
        enc->encode(subsamples, out);

        // adjust frame sizes after encoding
        long frameSize = out.numBytesWritten() - startByte;
        if (streamHeader.min_frame_size_ == 0 || frameSize < streamHeader.min_frame_size_) {
            streamHeader.min_frame_size_ = (int)frameSize;
        }
        if (frameSize > streamHeader.max_frame_size_) {
            streamHeader.max_frame_size_ = (int)frameSize;
        }
        pos += n;
    }
}