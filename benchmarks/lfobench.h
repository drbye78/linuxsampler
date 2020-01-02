/*
    Shared includes and definitions for all LFO benchmarks
    (triang.cpp, saw.cpp, sine.cpp, square.cpp in this directory)

    Copyright (C) 2005 - 2019 Christian Schoenebeck <cuse@users.sf.net>
*/

#ifndef LS_LFO_BENCHMARK_H
#define LS_LFO_BENCHMARK_H

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

#include "../src/engines/common/LFOBase.h"

// whether we should not show any messages on the console
#ifndef SILENT
# define SILENT 0
#endif

// set to 1 if you want to output the three calculated waves as RAW files
// you can e.g. open it as RAW file in Rezound
// (32 bit SP-FP PCM, mono, little endian, 44100kHz)
#ifndef OUTPUT_AS_RAW_WAVE
# define OUTPUT_AS_RAW_WAVE	0
#endif

// how many sample points should we calculate in one sequence
#ifndef STEPS
# define STEPS			16384
#endif

// how often should we repeat the benchmark loop of each solution
#ifndef RUNS
# define RUNS			6000
#endif

// whether the wave should have positive and negative range (signed -> 1) or just positive (unsigned -> 0)
#ifndef SIGNED
# define SIGNED			1
#endif

// maximum value of the LFO output (also depends on SIGNED above)
#ifndef MAX
# define MAX			1.0f
#endif

// pro forma
#ifndef SAMPLING_RATE
# define SAMPLING_RATE		44100.0f
#endif

// we use 32 bit single precision floating point as sample point format
typedef float smpl_t; // (sample_t is already defined as int16_t by global_private.h)

using namespace LinuxSampler;

// output calculated values as RAW audio format (32 bit floating point, mono) file
inline void output_as_raw_file(const char* filename, smpl_t* pOutputBuffer, int steps) {
    FILE* file = fopen(filename, "w");
    if (file) {
        fwrite((void*) pOutputBuffer, sizeof(float), steps, file);
        fclose(file);
    } else {
        fprintf(stderr, "Could not open %s\n", filename);
    }
}

struct BenchRes {
    int algorithmID; ///< Numeric ID of the implementation been benchmarked.
    std::string algorithmName; ///< Human readable name of the algorithm (to be printed in the results summary).
    double timeMSecs; ///< How long the algorithm executed (in milliseconds).
};

inline void sortResultsFirstToBeBest(std::vector<BenchRes>& results) {
    std::sort(results.begin(), results.end(), [](const BenchRes& a, const BenchRes& b) {
        return a.timeMSecs < b.timeMSecs;
    });
}

inline void printResultSummary(const std::vector<BenchRes>& results) {
    #if ! SILENT
    printf("\n");
    printf("RESULT\n");
    printf("======\n\n");
    for (int i = 0; i < results.size(); ++i) {
        const BenchRes& res = results[i];
        printf("%d. %s solution: %.1fms%s\n", i+1, res.algorithmName.c_str(), res.timeMSecs, (i==0) ? " <-- [WINNER]" : "");
    }
    printf("\n");
    printf("[Exit Result (Winner's ID) = %d]\n\n", results[0].algorithmID);
    #endif
}

#endif // LS_LFO_BENCHMARK_H
