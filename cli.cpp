/*
 * CLI for Squeeze lossless audio compression codec
 * Just supports compress/decompress
 */

#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>

#include "string.h"
#include "encode.h"
#include "decode.h"

#include <vector>

using namespace std;

// Returns true if file with filename exists in the system
bool exists(const char* filename) {
    struct stat buf;   
    return (stat(filename, &buf) == 0);
}

std::ifstream::pos_type filesize(const char* filename) {
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg(); 
}

int main (int argc, char** argv) {
    if (argc > 4) {
        fprintf(stderr, "Too many inputs: encode [file]");
        return 1;
    } else if (argc < 2) {
        fprintf(stderr, "not enough inputs");
        return 1;
    }
    string infile(argv[2]);
    if (!exists(infile.c_str())) {
        fprintf(stderr, "Input file doesn't exist, try again.");
        return 1;
    }
    size_t fileSize = filesize(infile.c_str());
    string outfile(argv[3]);    

    auto start = chrono::high_resolution_clock::now();
    if (strcmp(argv[1], "-d") == 0) {
        printf("Decoding...\n");
        Decoder::Decode(infile, outfile);
        printf("Success: ");
    } else if (strcmp(argv[1], "-e") == 0) {
        printf("Encoding...\n");
        Encoder::Encode(infile, outfile);
        printf("Success: ");
    } else {
        printf("Usage: sqz [-e|-d] input_file output_file\n");
        return 1;
    }
    auto stop = chrono::high_resolution_clock::now(); 
    size_t us = chrono::duration_cast<chrono::microseconds>(stop - start).count();
    double speed = ((double)fileSize / 1000) / ((double)us / 1e6);
    printf("file processed at: %.2f kBps\n", speed);
    
    return 0;
}