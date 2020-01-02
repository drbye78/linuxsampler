/*
    Triangle wave generator benchmark

    This is a benchmark for comparison between a integer math, table lookup
    and numeric sine wave harmonics solution.

    Copyright (C) 2005 - 2019 Christian Schoenebeck <cuse@users.sf.net>
*/

#include "lfobench.h"

#include "../src/engines/common/LFOTriangleIntMath.h"
#include "../src/engines/common/LFOTriangleIntAbsMath.h"
#include "../src/engines/common/LFOTriangleDiHarmonic.h"

// return value of this benchmark
// to indicate the best performing solution
#define TRIANG_INT_MATH_SOLUTION        2  /* we don't start with 1, as this is reserved for unknown errors */
#define TRIANG_DI_HARMONIC_SOLUTION     3
#define TRIANG_TABLE_LOOKUP_SOLUTION    4  /* table lookup solution is currently disabled in this benchmark, see below */
#define TRIANG_INT_MATH_ABS_SOLUTION    5  /* integer math with abs() */
#define INVALID_RESULT                 -1

#if SIGNED
LFOTriangleIntMath<LFO::range_signed>*    pIntLFO        = NULL;
LFOTriangleIntAbsMath<LFO::range_signed>* pIntAbsLFO     = NULL;
LFOTriangleDiHarmonic<LFO::range_signed>* pDiHarmonicLFO = NULL;
#else // unsigned
LFOTriangleIntMath<LFO::range_unsigned>*    pIntLFO        = NULL;
LFOTriangleIntAbsMath<LFO::range_unsigned>* pIntAbsLFO     = NULL;
LFOTriangleDiHarmonic<LFO::range_unsigned>* pDiHarmonicLFO = NULL;
#endif

// integer math solution
double int_math(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    pIntLFO->trigger(frequency, LFO::start_level_min, 0 /* max. internal depth */, 1200, false, (unsigned int) SAMPLING_RATE);
    //pIntLFO->setPhase(0);
    //pIntLFO->setFrequency(frequency*2, SAMPLING_RATE);

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        pIntLFO->updateByMIDICtrlValue(127); // pro forma
        for (int i = 0; i < steps; ++i) {
            //pIntLFO->updateByMIDICtrlValue(float(i)/float(steps)*127.f);
            pDestinationBuffer[i] = pIntLFO->render() * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    double elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("int math solution elapsed time: %.1f ms\n", elapsed_time);
    #endif

    return elapsed_time;
}

// integer math abs solution
double int_math_abs(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    pIntAbsLFO->trigger(frequency, LFO::start_level_min, 0 /* max. internal depth */, 1200, false, (unsigned int) SAMPLING_RATE);
    //pIntAbsLFO->setPhase(0);
    //pIntAbsLFO->setFrequency(frequency*2, SAMPLING_RATE);

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        pIntAbsLFO->updateByMIDICtrlValue(127); // pro forma
        for (int i = 0; i < steps; ++i) {
            //pIntAbsLFO->updateByMIDICtrlValue(float(i)/float(steps)*127.f);
            pDestinationBuffer[i] = pIntAbsLFO->render() * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    double elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("int math abs solution elapsed time: %.1f ms\n", elapsed_time);
    #endif

    return elapsed_time;
}

// table lookup solution (currently disabled)
//
// This solution is not yet implemented in LinuxSampler yet and probably
// never will, I simply haven't found an architectures / system where this
// turned out to be the best solution and it introduces too many problems
// anyway. If you found an architecture where this seems to be the best
// solution, please let us know!
#if 0
double table_lookup(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    const float r = frequency / SAMPLING_RATE; // frequency alteration quotient
    #if SIGNED
    float c = r * 4.0f;
    #else
    float c = r * 2.0f;
    #endif
    const int wl = (int) (SAMPLING_RATE / frequency); // wave length (in sample points)

    // 'volatile' to avoid the cache to fudge the benchmark result
    volatile float* pPrerenderingBuffer = new float[wl];

    // prerendering of the triangular wave
    {
        float level = 0.0f;
        for (int i = 0; i < wl; ++i) {
            level += c;
            #if SIGNED
            if (level >= 1.0f) {
                c = -c;
                level = 1.0f;
            }
            else if (level <= -1.0f) {
                c = -c;
                level = -1.0f;
            }
            #else
            if (level >= 1.0f) {
                c = -c;
                level = 1.0f;
            }
            else if (level <= 0.0f) {
                c = -c;
                level = 0.0f;
            }
            #endif
            pPrerenderingBuffer[i] = level;
        }
    }

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        for (int i = 0; i < steps; ++i) {
            pDestinationBuffer[i] = pPrerenderingBuffer[i % wl] * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    double elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("Table lookup solution elapsed time: %.1f ms\n", elapsed_time);
    #endif

    if (pPrerenderingBuffer) delete[] pPrerenderingBuffer;

    return elapsed_time;
}
#endif

// numeric, di-harmonic solution
double numeric_di_harmonic_solution(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    pDiHarmonicLFO->trigger(frequency, LFO::start_level_min, 0 /* max. internal depth */, 1200, false, (unsigned int) SAMPLING_RATE);
    //pDiHarmonicLFO->setPhase(0);
    //pDiHarmonicLFO->setFrequency(frequency*2, SAMPLING_RATE);

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        pDiHarmonicLFO->updateByMIDICtrlValue(127); // pro forma
        for (int i = 0; i < steps; ++i) {
            //pDiHarmonicLFO->updateByMIDICtrlValue(float(i)/float(steps)*127.f);
            pDestinationBuffer[i] = pDiHarmonicLFO->render() * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    double elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("Numeric harmonics solution elapsed time: %.1f ms\n", elapsed_time);
    #endif

    return elapsed_time;
}

int main() {
    const int steps = STEPS;
    const int sinusoidFrequency = 100; // Hz

    #if ! SILENT
    printf("\n");
    # if SIGNED
    printf("Signed triangular wave benchmark\n");
    # else
    printf("Unsigned triangular wave benchmark\n");
    # endif
    printf("----------------------------------\n");
    printf("\n");
    #endif

    #if SIGNED
    pIntLFO        = new LFOTriangleIntMath<LFO::range_signed>(MAX);
    pIntAbsLFO     = new LFOTriangleIntAbsMath<LFO::range_signed>(MAX);
    pDiHarmonicLFO = new LFOTriangleDiHarmonic<LFO::range_signed>(MAX);
    #else // unsigned
    pIntLFO        = new LFOTriangleIntMath<LFO::range_unsigned>(MAX);
    pIntAbsLFO     = new LFOTriangleIntAbsMath<LFO::range_unsigned>(MAX);
    pDiHarmonicLFO = new LFOTriangleDiHarmonic<LFO::range_unsigned>(MAX);
    #endif

    // output buffer for the calculated sinusoid wave
    smpl_t* pOutputBuffer = new smpl_t[steps];
    // just an arbitrary amplitude envelope to simulate a bit higher memory bandwidth
    float* pAmplitude  = new float[steps];

    // pro forma - an arbitary amplitude envelope
    for (int i = 0; i < steps; ++i)
        pAmplitude[i] = (float) i / (float) steps;

    // going to store how long each solution took (in seconds)
    std::vector<BenchRes> results;


    results.push_back({
        .algorithmID = TRIANG_INT_MATH_SOLUTION,
        .algorithmName = "int math",
        .timeMSecs = int_math(pOutputBuffer, pAmplitude, steps, sinusoidFrequency)
    });
    #if OUTPUT_AS_RAW_WAVE
    output_as_raw_file("bench_int_math.raw", pOutputBuffer, steps);
    #endif


    results.push_back({
        .algorithmID = TRIANG_INT_MATH_ABS_SOLUTION,
        .algorithmName = "int math abs",
        .timeMSecs = int_math_abs(pOutputBuffer, pAmplitude, steps, sinusoidFrequency)
    });
    #if OUTPUT_AS_RAW_WAVE
    output_as_raw_file("bench_int_math_abs.raw", pOutputBuffer, steps);
    #endif


    //table_lookup_result = table_lookup(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    //#if OUTPUT_AS_RAW_WAVE
    //  output_as_raw_file("bench_table_lookup.raw", pOutputBuffer, steps);
    //#endif


    results.push_back({
        .algorithmID = TRIANG_DI_HARMONIC_SOLUTION,
        .algorithmName = "Numeric di harmonic",
        .timeMSecs = numeric_di_harmonic_solution(pOutputBuffer, pAmplitude, steps, sinusoidFrequency)
    });
    #if OUTPUT_AS_RAW_WAVE
    output_as_raw_file("bench_numeric_harmonics.raw", pOutputBuffer, steps);
    #endif


    #if ! SILENT
    printf("\nOK, 2nd try\n\n");
    #endif


    results[0].timeMSecs += int_math(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    results[1].timeMSecs += int_math_abs(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    //table_lookup_result += table_lookup(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    results[2].timeMSecs += numeric_di_harmonic_solution(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);


    if (pAmplitude)    delete[] pAmplitude;
    if (pOutputBuffer) delete[] pOutputBuffer;

    if (pIntLFO)        delete pIntLFO;
    if (pIntAbsLFO)     delete pIntAbsLFO;
    if (pDiHarmonicLFO) delete pDiHarmonicLFO;

    sortResultsFirstToBeBest(results);
    printResultSummary(results);

    return results[0].algorithmID; // return the winner's numeric algorithm ID
}
