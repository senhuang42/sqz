#include "LPC.h"
#include "FseEncoder.h"

std::pair<SubframeProcessor*, long> LinearPredictiveEncoder::calculateCoefs(vector<long>& samples, int shift, int depth, int order, int roundVars, MemorizedDotProd& fdp) {
    LinearPredictiveEncoder* enc = new LinearPredictiveEncoder(samples, shift, depth, order, fdp);
    samples = enc->rightShiftAllValues(samples, shift);
    long bestSize = LONG_MAX;
    vector<int> finalCoefficients = enc->coefficients_;

    // So it's actually unclear to me how to get a really good estimate of expected size
    // based on LPC coefficients + FSE encoded blocks. So... for now we will just use the last one, hardcoding
    // a "size" value into our returned pair. If in the future I come up with something better,
    // we can use that as a drop-in. Until then, just say it's 5000. Everything will still work
    // since this would just be kind of a heuristic selection.

    // And yes, this is doing extra unnecessary work.
    for (int i = 0; i < (1 << roundVars); ++i) {
        vector<long> newData = samples;
        doLinearPrediction(newData, enc->coefficients_, enc->coefficients_shift_amount_);
        bestSize = 5000;
        finalCoefficients = enc->coefficients_;
    }

    enc->coefficients_ = std::move(finalCoefficients);
    return std::make_pair(enc, bestSize);
}

LinearPredictiveEncoder::LinearPredictiveEncoder(vector<long>& samples, int shift, int depth, int order, MemorizedDotProd& mdp) : SubframeProcessor(shift, depth) {
    order_ = order;

    // construct the matrix to solve
    vector<vector<double>> matrix(order_, vector<double>(order_+1));

    // Constructing a toeplitz matrix
    for (int row = 0; row < order_; ++row) {
        for (int col = 0; col < order_ + 1; ++col) {
            double prod;
            if (col >= row) {
                prod = mdp.dotProduct(row, col, samples.size() - order_);
            } else {
                prod = matrix[col][row];
            }
            matrix[row][col] = prod;
        }
    }

    unnormalized_coefficients_ = gaussianElimate(matrix);

    double maxCoef = 0.0;
    for (auto x : unnormalized_coefficients_) {
        maxCoef = std::max(abs(x), maxCoef);
    }

    int fullBits;
    if (maxCoef >= 1) {
        fullBits = (int)(log(maxCoef) / log(2)) + 1;
    } else {
        fullBits = 0;
    }

    coefficients_.resize(order);
    // determines number of partitions
    coefficients_depth_ = 15;
    coefficients_shift_amount_ = coefficients_depth_ - 1 - fullBits;

    // normalize the coefficients into values that make sense for us
    for (int i = 0; i < unnormalized_coefficients_.size(); ++i) {
        double coef = unnormalized_coefficients_[unnormalized_coefficients_.size() - 1 - i];
        int val = (int)round(coef * (1 << coefficients_shift_amount_));
        coefficients_[i] = std::max(
                                std::min(val, (1 << (coefficients_depth_ - 1)) - 1),
                                -(1 << (coefficients_depth_ - 1))
                            );
    }
}

void LinearPredictiveEncoder::encode(vector<long> &samples, OutBufferedBitFileStream &out) {
    putTypeShift(32 + order_ - 1, out);
    samples = rightShiftAllValues(samples, sample_shift_);
    
    // first output "warmup" # of samples as raw values
    for (int i = 0; i < order_; i++) {
        putRawLong(samples[i], out);
    }

    out.putVal(4, coefficients_depth_ - 1);
    out.putVal(5, coefficients_shift_amount_);
    for_each(coefficients_.begin(), coefficients_.end(), [&](auto val){ out.putVal(coefficients_depth_, val); });

    // Do the LPC itself
    doLinearPrediction(samples, coefficients_, coefficients_shift_amount_);

    // Encode the error residuals
    FseEncoder::encode(samples, order_, 0, out);
}

void LinearPredictiveEncoder::doLinearPrediction(vector<long> &data, const vector<int> &coefficients, int shift) {
    for (int i = data.size() - 1; i >= coefficients.size(); i--) {
        long sum = 0;
        for (int j = 0; j < coefficients.size(); j++) {
            sum += data[i - 1 - j] * coefficients[j];
        }
        data[i] -= (sum >> shift);
    }
}