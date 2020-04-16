#include <cmath>
#include <vector>

#pragma once

// Class for fast dot product calculations with cached results
// inspired from a FLAC implementation
class MemorizedDotProd {
public:
    MemorizedDotProd(std::vector<long>& data, int maxDiff) {
        data_ = data;
        memo_ = std::vector<double>(maxDiff + 1);
        for (int i = 0; i < memo_.size(); i++) {
            double sum = 0;
            for (int j = 0; i + j < data_.size(); j++) {
                sum += (double)data_[j] * data_[i + j];
            }
            memo_[i] = sum;
        }
    }

    double dotProduct(int j, int k, int len) {
        if (j > k) {
            return dotProduct(k, j, len);
        }

        double remove = 0;
        for (int i = 0; i < j; i++) {
            remove += (double)data_[i] * (double)data_[i + k - j];
        }
        for (int i = k + len; i < data_.size(); i++) {
            remove += (double)data_[i] * (double)data_[i - k - j];
        }

        return memo_[k - j] - remove;
    } 

private:
    std::vector<long> data_;
    std::vector<double> memo_;
};