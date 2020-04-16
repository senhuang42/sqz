#include <iostream>
#include <cmath>
#include <bitset>
#include <fstream>
#include <cstddef>

#include "OutBufferedBitFileStream.h"
#include "MDP.h"
#include "StreamHeader.h"
#include "SubframeProcessor.h"
#include "LPC.h"

#pragma once

class Encoder {
public:
    static void Encode(std::string infile, std::string outfile);
};

class FrameHeader {
public:
    FrameHeader();

	// Write the frame header into outstream
    int writeHeader(OutBufferedBitFileStream& out) {
        int written = 0;
        out.resetCrc();
        out.putVal(14, 0x3FFE);
		out.putVal(2, 1);
		out.putVal(4, getBlockSize(block_size_));
		out.putVal(4, getSampleRate(sample_rate_));
		out.putVal(4, channel_assignment_);
		out.putVal(3, getSampleDepth(sample_depth_));
		out.putVal(1, 0);

        if (frame_index_ != -1 && sample_offset_ == -1) {
			putUtf8Long(sample_offset_, out);
		} else if (sample_offset_ != -1 && frame_index_ == -1) {
			putUtf8Long(sample_offset_, out);
		} else {
			throw std::runtime_error("shit");
		}

        written += 40;	// forty bits so far

		// int:blockSize mappings determine if we need more bits to write the sizes
		switch(getBlockSize(block_size_)) {
			case 6:
				out.putVal(8, block_size_ - 1);
				written += 8;
				break;
			case 7:
				out.putVal(16, block_size_ - 1);
            	written += 16;
				break;
		}

		switch(getSampleRate(sample_rate_)) {
			case 12:
				out.putVal(8, sample_rate_);
				written += 8;
			case 13:
				out.putVal(16, sample_rate_);
            	written += 16;
			case 14:
				out.putVal(16, sample_rate_ / 10);
            	written += 16;
		}
		
		out.putVal(8, out.Crc8());
        written += 8;
        return written;
    }

    // Write val into OutBufferedBitFileStream as Utf8
    void putUtf8Long(long val, OutBufferedBitFileStream& out) {
		int nBits = 64 - __builtin_clzl(val);
		if (nBits > 7) {
			int n = (nBits - 2) / 5;
			out.putVal(8, ((unsigned long)0xFF80 >> n) | (int)((unsigned long)val >> (n * 6)));
			for (int i = n - 1; i >= 0; i--) {
				out.putVal(8, 0x80 | ((int)((unsigned long)val >> (i * 6)) & 0x3F));
			}
		} else {
			out.putVal(8, (int)val);
		}
	}

    static int getBlockSize(int blockSize);

    static int getSampleRate(int sampleRate);

    static int getSampleDepth(int sampleDepth);

	static int findFirst(std::vector<std::vector<int>>& table, int key) {
		for (std::vector<int>& each : table) {
			if (each[0] == key) {
				return each[1];
			}
		}
		return -1;
	}
	
	static int findSecond(std::vector<std::vector<int>>& table, int key) {
		for (std::vector<int>& each : table) {
			if (each[1] == key) {
				return each[0];
			}				
		}
		return -1;
	}
	
	static std::vector<std::vector<int>> BLOCK_SIZE_MAP;
	static std::vector<std::vector<int>> SAMPLE_DEPTH_MAP;
	static std::vector<std::vector<int>> SAMPLE_RATE_MAP;
    int frame_index_;
    long sample_offset_;
    int num_channels_;
    int channel_assignment_;
	int block_size_;
	int sample_rate_;
	int sample_depth_;
	int frame_size_;
};

class FrameProcessor {
public:
    static std::pair<FrameProcessor*, long> calculateCoefs(int sampleOffset, std::vector<std::vector<long>>& samples, int sampleDepth, int sampleRate) {
        FrameProcessor* curEncoder = new FrameProcessor(sampleOffset, samples, sampleDepth, sampleRate);
        int numChannels = samples.size();
        std::vector<std::pair<SubframeProcessor*, long>> allEncoders(numChannels);
        if (numChannels < 2) {
            curEncoder->frame_header_.channel_assignment_ = numChannels - 1;
            for (int i = 0; i < allEncoders.size(); i++) {
                allEncoders[i] = SubframeProcessor::calculateCoefs(samples[i], sampleDepth);
            }
        } else { 
			throw std::runtime_error("stereo encoding not supported yet\n");
        }

        // Add up subframe sizes
		long size = 0;
		curEncoder->subframe_encoders = std::vector<SubframeProcessor*>(allEncoders.size());
		for (int i = 0; i < curEncoder->subframe_encoders.size(); i++) {
			size += allEncoders[i].second;
			curEncoder->subframe_encoders[i] = allEncoders[i].first;
		}
		
		// Count length of header
        OutBufferedBitFileStream bitout("/dev/null");	// this is hacky, but eh..
        size += curEncoder->frame_header_.writeHeader(bitout);
        
		// space for crc16 code
		size += 7;
		size /= 8;
		size += 2;  
		return make_pair(curEncoder, size);
    }

    FrameProcessor(int sampleOffset, std::vector<std::vector<long>> samples, int sampleDepth, int sampleRate) {
		frame_header_.sample_offset_ = sampleOffset;
		frame_header_.sample_depth_ = sampleDepth;
		frame_header_.sample_rate_ = sampleRate;
		frame_header_.block_size_ = samples[0].size();
		frame_header_.channel_assignment_ = samples.size() - 1;
	}

    void encode(std::vector<std::vector<long>>& samples, OutBufferedBitFileStream& out);

    FrameHeader frame_header_;

private:
	std::vector<SubframeProcessor*> subframe_encoders;
};

class DataEncoder {
public:
    DataEncoder(StreamHeader& info, std::vector<std::vector<int>>& samples, int blockSize, OutBufferedBitFileStream &out);

    static std::vector<std::vector<long>> getRange(std::vector<std::vector<int>>& array, int off, int len);
};