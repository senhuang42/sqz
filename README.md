# Squeeze: A lossless audio compression codec
## In fulfillment of Sen Huang's CPSC 490: Senior Project

Squeeze is an **experimental novel audio compression codec** which uses linear prediction combined with
as ANS-based coding of the residuals from the LPC, which theoretically would provide better compression ratio
than existing codecs which use Golomb-Rice coding (like FLAC, ALAC, etc.) and theoretically would be faster
than existing codecs which use Arithmetic Coding (like Monkey's Audio).

We use a Finite State Entropy library (https://github.com/Cyan4973/FiniteStateEntropy) provided by Yann Collett
to do the FSE compression.

In practice, it's slower, and offers a worse compression rate than productionized, industry-standard codecs. It's more comparable to the best general-purpose compression algorithms in terms of performance. Generally, this has
been tested for correctness/no-loss, but this project is not significantly fuzzed, so use at your own risk.

Currently, this will encode single-channel audio with 16-bit depth. Higher bitrates are currently not supported.

I've included some sample .wav files that fit these requirements that you can use to test it out.