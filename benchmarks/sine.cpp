/*
    Sine wave generator benchmark

    This is a benchmark for comparison between a built-in sin() function call
    solution, and a numeric complex number solution.

    Copyright (C) 2019 Christian Schoenebeck <cuse@users.sf.net>
*/

#include "lfobench.h"

#include "../src/engines/common/LFOSineNumericComplexNr.h"
#include "../src/engines/common/LFOSineBuiltinFn.h"

// return value of this benchmark
// to indicate the best performing solution
#define SINE_BUILTIN_SOLUTION               40
#define SINE_NUMERIC_COMPLEX_NR_SOLUTION    41
#define INVALID_RESULT                      -1

#if SIGNED
LFOSineNumericComplexNr<LFO::range_signed>* pSineLFO = NULL;
LFOSineBuiltinFn<LFO::range_signed>* pSineLFOBuiltin = NULL;
#else // unsigned
LFOSineNumericComplexNr<LFO::range_unsigned>* pSineLFO = NULL;
LFOSineBuiltinFn<LFO::range_unsigned>* pSineLFOBuiltin = NULL;
#endif

double sine_complex_nr(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    pSineLFO->trigger(frequency, LFO::start_level_max, 0 /* max. internal depth */, 1200, true, (unsigned int) SAMPLING_RATE);
    //pSineLFO->setPhase(0);
    //pSineLFO->setFrequency(frequency*2, SAMPLING_RATE);

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        pSineLFO->updateByMIDICtrlValue(127); // pro forma
        for (int i = 0; i < steps; ++i) {
            //pSineLFO->updateByMIDICtrlValue(float(i)/float(steps)*127.f);
            pDestinationBuffer[i] = pSineLFO->render() * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    double elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("Numeric complex nr solution elapsed time: %.1f ms\n", elapsed_time);
    #endif

    return elapsed_time;
}

double sine_builtin(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    pSineLFOBuiltin->trigger(frequency, LFO::start_level_max, 0 /* max. internal depth */, 1200, true, (unsigned int) SAMPLING_RATE);
    //pSineLFOBuiltin->setPhase(0);
    //pSineLFOBuiltin->setFrequency(frequency*2, SAMPLING_RATE);

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        pSineLFOBuiltin->updateByMIDICtrlValue(127); // pro forma
        for (int i = 0; i < steps; ++i) {
            //pSineLFOBuiltin->updateByMIDICtrlValue(float(i)/float(steps)*127.f);
            pDestinationBuffer[i] = pSineLFOBuiltin->render() * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    double elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("Built-in function solution elapsed time: %.1f ms\n", elapsed_time);
    #endif

    return elapsed_time;
}

int main() {
    const int steps = STEPS;
    const int sinusoidFrequency = 100; // Hz

    #if ! SILENT
    printf("\n");
    # if SIGNED
    printf("Signed sine wave benchmark\n");
    # else
    printf("Unsigned sine wave benchmark\n");
    # endif
    printf("----------------------------------\n");
    printf("\n");
    #endif

    #if SIGNED
    pSineLFO = new LFOSineNumericComplexNr<LFO::range_signed>(MAX);
    pSineLFOBuiltin = new LFOSineBuiltinFn<LFO::range_signed>(MAX);
    #else // unsigned
    pSineLFO = new LFOSineNumericComplexNr<LFO::range_unsigned>(MAX);
    pSineLFOBuiltin = new LFOSineBuiltinFn<LFO::range_unsigned>(MAX);
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
        .algorithmID = SINE_BUILTIN_SOLUTION,
        .algorithmName = "Built-in function",
        .timeMSecs = sine_builtin(pOutputBuffer, pAmplitude, steps, sinusoidFrequency)
    });
    #if OUTPUT_AS_RAW_WAVE
    output_as_raw_file("sine_builtin_fn.raw", pOutputBuffer, steps);
    #endif


    results.push_back({
        .algorithmID = SINE_NUMERIC_COMPLEX_NR_SOLUTION,
        .algorithmName = "Numeric complex nr",
        .timeMSecs = sine_complex_nr(pOutputBuffer, pAmplitude, steps, sinusoidFrequency)
    });
    #if OUTPUT_AS_RAW_WAVE
    output_as_raw_file("sine_numeric_complex_nr.raw", pOutputBuffer, steps);
    #endif


    #if ! SILENT
    printf("\nOK, 2nd try\n\n");
    #endif


    results[0].timeMSecs += sine_builtin(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    results[1].timeMSecs += sine_complex_nr(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);


    if (pAmplitude)    delete[] pAmplitude;
    if (pOutputBuffer) delete[] pOutputBuffer;

    if (pSineLFO) delete pSineLFO;
    if (pSineLFOBuiltin) delete pSineLFOBuiltin;

    sortResultsFirstToBeBest(results);
    printResultSummary(results);

    return results[0].algorithmID; // return the winner's numeric algorithm ID
}
