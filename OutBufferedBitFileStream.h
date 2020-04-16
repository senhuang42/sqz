#include <iostream>
#include <string>
#include <fstream>

#pragma once

// int32_t representation of "sqzZ"
static int32_t SQZ_MAGIC_NUM = 0x73517a5a;

class OutBufferedBitFileStream : std::streambuf {
public:
    OutBufferedBitFileStream(std::string outFile) {
        out_.open(outFile.c_str(), std::ios::out | std::ios::binary);
        buf_ = 0;
		buf_size_ = 0;
		bytes_written_ = 0;
		resetCrc();
    }
    
    void writeBuffer(const char* buf_, size_t size) {
        for (int i = 0; i < size; ++i) {
            putVal(8, buf_[i]);
        }
        flush();
    }

    void putVal(int n, int x) {
        if (buf_size_ + n > 64) {
			flush();
		}
		buf_ <<= n;
		buf_ |= x & ((1L << n) - 1);
		buf_size_ += n;
    }

    void flush() {
        while (buf_size_ >= 8) {
			buf_size_ -= 8;
			int b = (int)((unsigned long)buf_ >> buf_size_) & 0xFF;
			out_.put(b); // check
			bytes_written_++;
			check8_ ^= b;
			check16_ ^= b << 8;
			for (int i = 0; i < 8; i++) {
				check8_ <<= 1;
				check16_ <<= 1;
				check8_ ^= 0x107 * ((unsigned)check8_ >> 8);
				check16_ ^= 0x18005 * ((unsigned)check16_ >> 16);
			}
		}
		out_.flush();
    }

    void resetCrc() {
        flush();
        check8_ = check16_ = 0;
    }

    // include cyclic redundancy check functionality like FLAC
    int Crc8() {
        assertAlignment();
        flush();
        if (((unsigned)check8_ >> 8) != 0) {
            throw std::runtime_error("Bad check8_ code");
        }
        return check8_;
    }

    int Crc16() {
        assertAlignment();
        flush();
        if (((unsigned)check16_ >> 16) != 0) {
            throw std::runtime_error("Bad check16_ code");
        }
        return check16_;
    }

    long numBytesWritten() {
        return bytes_written_ + buf_size_ / 8;
    }

    void seek(int pos) {
        out_.seekp(std::ios_base::beg + pos);
    }

    void align() {
        int bytesToAlign = (64 - buf_size_) % 8;
        putVal(bytesToAlign, 0);
    }

    void assertAlignment() {
        if (buf_size_ % 8 != 0) {
            throw std::runtime_error("not at a byte boundary");
        }
    }

private:
    std::ofstream out_;
    long buf_;
    int buf_size_;
    long bytes_written_;
    int check8_;
    int check16_;
};