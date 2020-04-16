#include "FseEncoder.h"
#include "lib/fse.h"

void FseEncoder::encode(std::vector<long>& data, int warmup, int order, OutBufferedBitFileStream& out) {
    out.putVal(4, order);
    int numPartitions = 1 << order;
    int start = warmup;
    int end = (unsigned long)data.size() >> order;
    for (int i = 0; i < numPartitions; i++) {
        encodeData(data, start, end, 10, out);
        start = end;
        end += (unsigned long)data.size() >> order;
    }
}

void FseEncoder::encodeData(std::vector<long>& data, int start, int end, int param, OutBufferedBitFileStream& out) {
    int partitionSize = end-start;

    int16_t* tempTruncatedBuf = (int16_t*)malloc(sizeof(int16_t)*partitionSize);
    if (!tempTruncatedBuf) {
        throw std::runtime_error("Malloc failed somehow");
    }
    for (int i = start, j = 0; i < end; ++i) {
        tempTruncatedBuf[j++] = (int16_t)data[i];
    }

    size_t cSizeBound = FSE_compressBound(partitionSize*sizeof(int16_t));
    char* fseCompressedResiduals = (char*)malloc(sizeof(char)*cSizeBound);
    if (!fseCompressedResiduals) {
        throw std::runtime_error("Malloc failed somehow");
    }
    int compressedSize = FSE_compress(fseCompressedResiduals, cSizeBound, tempTruncatedBuf, partitionSize*sizeof(int16_t));
    
    out.putVal(24, compressedSize);
    out.flush();
    out.writeBuffer(fseCompressedResiduals, compressedSize);

    free(fseCompressedResiduals);
    free(tempTruncatedBuf);
}