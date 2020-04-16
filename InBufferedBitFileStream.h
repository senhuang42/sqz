#include <fstream>
#include <iostream>

class InBufferedBitFileStream : std::streambuf {
public:
    InBufferedBitFileStream(std::string fileName) {
        in_.open(fileName.c_str(), std::ios::in | std::ios::binary);
        buf_ = 0;
        buf_size_ = 0;
    }

    void align() {
        buf_size_ -= buf_size_ % 8;
    }

    int readOneByte() {
        if (buf_size_ >= 8) {
            return readUnsignedInt(8);
        } else {
            return in_.get();
        }
    }

    int readSignedInt(int nBits) {
        return (readUnsignedInt(nBits) << (32 - nBits)) >> (32 - nBits);
    }
    
    int readUnsignedInt(int nBits) {
        while (buf_size_ < nBits) {
			buf_ = (buf_ << 8) | in_.get();
			buf_size_ += 8;
		}
		buf_size_ -= nBits;
		int result = (int)((unsigned long)buf_ >> buf_size_);
		if (nBits < 32)
			result &= (1 << nBits) - 1;
		return result;
    }

    void readIntoBuf(char* dstBuf, int size) {
        for (int i = 0; i < size; ++i) {
            dstBuf[i] = readUnsignedInt(8);
        }
    }

private:
    std::ifstream in_;
    long buf_;
    int buf_size_;  // size in bits
};