#include <vector>
#include <cstddef>

#pragma once

class StreamHeader {
public:
    StreamHeader() : min_block_size_(0) {}

    void write(bool last, OutBufferedBitFileStream& out) {
		out.putVal(1, last ? 1 : 0);
		out.putVal(7, 0);	// require 7-bit "0" code for the header
		
		// Write stream info block fields
		out.putVal(20, sample_rate_);
		out.putVal(3, num_channels_ - 1);
		out.putVal(5, sample_depth_ - 1);
		out.putVal(18, (int)((unsigned)num_samples_ >> 18));
		out.putVal(18, (int)((unsigned)num_samples_ >>  0));
    }
	
    int min_block_size_;
	int max_block_size_;
	int min_frame_size_;
	int max_frame_size_;
	int sample_rate_;
	int num_channels_;
	int sample_depth_;
	long num_samples_;
};