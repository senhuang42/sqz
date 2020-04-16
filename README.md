# Squeeze: A lossless audio compression codec
## In fulfillment of Sen Huang's CPSC 490: Senior Project

Squeeze is an **experimental novel audio compression codec** which uses linear prediction combined with
as ANS-based coding of the residuals from the LPC, which theoretically would provide better compression ratio
than existing codecs which use Golomb-Rice coding (like FLAC, ALAC, etc.) and theoretically would be faster
than existing codecs which use Arithmetic Coding (like OptimFROG). Compatible with OS X.

We use a super-fast Finite State Entropy library (https://github.com/Cyan4973/FiniteStateEntropy) (shoutout to Yann Collett for making this)
to do the FSE compression.

In practice, it's slower, and offers a worse compression rate than productionized, industry-standard codecs. It's more comparable to the best general-purpose compression algorithms (zstd) in terms of performance. The worse speed
to be expected, as this codec is optimized for speed. It's not quite as good of a compressor as other lossless
codecs for a couple of reasons:
    - Block size selection is suboptimal
    - No use of inter-channel decorrelation (if we were to do stereo encoding too)
    - Wasted bits when encoding the residuals
Each of these are still open questions, since use of FSE to encode residuals has not been a thoroughly researched
topic, but I use some workable placeholders instead.

Generally, this has been tested for correctness/no-loss, but this project is not
significantly fuzzed, so use at your own risk. Currently, this will encode single-channel audio with 16-bit depth. Higher bitrates are currently not supported.

Usage:
`make && ./sqz [-e|-d] input_file.wav output_file.sqz`

I've included some sample .wav files that fit these requirements that you can use to test it out.

Performance:

| Compressor name         | Ratio  |  Compress    |  Decompress  |
| ---------------         |--------|--------------|--------------|
| **sqz**                 | 1.266  |   1500 KB/s  |   1300 KB/s  |
| zstd 1.4.4 -13          | 1.160  |   9900 KB/s  |   50400 KB/s |
| flac -8                 | 1.526  |   29700 KB/s |   15900 KB/s |
| alac                    | 1.525  |   20100 KB/s |   14400 KB/s |
| optimFROG -5            | 1.626  |   1200 KB/s  |   1344 KB/s  |

Recorded on Mac OS X with a very worn-down Macbook Pro 13 with 2Ghz i5 with `time` on
`test_files/CantinaBand.wav` (60 seconds duration) for end-to-end file processing with an SSD