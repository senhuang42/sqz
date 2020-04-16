#include <vector>
#include <cmath>
#include <algorithm>

#include "SubframeProcessor.h"
#include "MDP.h"

#pragma once

// technically bad practice to do this, oh well
using namespace std;

class LinearPredictiveEncoder : public SubframeProcessor {
public:
    void print() {}

    static std::pair<SubframeProcessor*, long> calculateCoefs(
        vector<long>& samples, int shift, int depth, int order, int roundVars, MemorizedDotProd& fdp);

    LinearPredictiveEncoder(
        vector<long>& samples, int shift, int depth, int order, MemorizedDotProd& fdp);

    virtual void encode(vector<long> &samples, OutBufferedBitFileStream &out);

    static void doLinearPrediction(vector<long> &data, const vector<int> &coefs, int shift);

private:
    int order_;
    vector<double> unnormalized_coefficients_;
    vector<int> coefficients_;
    int coefficients_depth_;
    int coefficients_shift_amount_;

    vector<long> rightShiftAllValues(const vector<long>& data, int shift) {
        vector<long> result = data;
        for_each(result.begin(), result.end(), [&](long& val){ val = val >> shift; });
        return result;
    }
    
    void swapRows(vector<vector<double>> mat, int i, int j) {
        for (int k = 0; k <= mat.size(); k++) { 
            auto tmp = mat[i][k]; 
            mat[i][k] = mat[j][k]; 
            mat[j][k] = tmp; 
        } 
    }

    vector<double> backSub(vector<vector<double>>& mat) {
        int N = mat.size();
        vector<double> x(N);
        for (int i = N - 1; i >= 0; i--) {
            x[i] = mat[i][N];
            for (int j = i + 1; j < N; j++) {
                x[i] -= mat[i][j]*x[j]; 
            }
            x[i] /= mat[i][i]; 
        } 
        return x;
    }

    // Typical Gaussian elimination to solve a matrix
    vector<double> gaussianElimate(vector<vector<double>>& mat) {
        int N = mat.size();
        bool singular = false;
        for (size_t k = 0; k < N; k++) { 
            int iMaxVal = k; 
            int vMaxVal = mat[iMaxVal][k]; 
            for (size_t i = k + 1; i < N; i++)  {
                if (abs(mat[i][k]) > vMaxVal) {
                    vMaxVal = mat[i][k];
                    iMaxVal = i; 
                }
            }
            if (!mat[k][iMaxVal]) 
                singular = true; // Matrix is singular 
            if (iMaxVal != k) {
                swapRows(mat, k, iMaxVal); 
            }
    
            for (size_t i = k + 1; i < N; i++) { 
                double f = mat[i][k]/mat[k][k]; 
                for (int j = k + 1; j <= N; j++) {
                    mat[i][j] -= mat[k][j]*f; 
                }
                mat[i][k] = 0; 
            } 
        }
        if (singular) {
            throw std::runtime_error("singular matrix\n");
        }
        return backSub(mat);
    }
};