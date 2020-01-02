/*
    Saw wave generator benchmark

    This is a benchmark for comparison between two different integer math
    solutions.

    Copyright (C) 2019 Christian Schoenebeck <cuse@users.sf.net>
*/

#include "lfobench.h"

#include "../src/engines/common/LFOSawIntMathNew.h"
#include "../src/engines/common/LFOSawIntMathOld.h"

// return value of this benchmark
// to indicate the best performing solution
#define SAW_OLD_INT_MATH_SOLUTION	20
#define SAW_NEW_INT_MATH_SOLUTION	21
#define INVALID_RESULT		        -1

#if SIGNED
LFOSawIntMathNew<LFO::range_signed>* pSawIntLFO = NULL;
LFOSawIntMathOld<LFO::range_signed,true>* pSawLFOold = NULL;
#else // unsigned
LFOSawIntMathNew<LFO::range_unsigned>* pSawIntLFO = NULL;
LFOSawIntMathOld<LFO::range_unsigned,true>* pSawLFOold = NULL;
#endif

double saw_new_int_math(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    pSawIntLFO->trigger(frequency, LFO::start_level_min, 0 /* max. internal depth */, 1200, false, (unsigned int) SAMPLING_RATE);
    //pSawIntLFO->setPhase(0);
    //pSawIntLFO->setFrequency(frequency*2, SAMPLING_RATE);

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        pSawIntLFO->updateByMIDICtrlValue(127); // pro forma
        for (int i = 0; i < steps; ++i) {
            //pSawIntLFO->updateByMIDICtrlValue(float(i)/float(steps)*127.f);
            pDestinationBuffer[i] = pSawIntLFO->render() * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    double elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("New int math solution elapsed time: %.1f ms\n", elapsed_time);
    #endif

    return elapsed_time;
}

double saw_old_int_math(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    pSawLFOold->trigger(frequency, LFO::start_level_min, 0 /* max. internal depth */, 1200, false, (unsigned int) SAMPLING_RATE);
    //pSawLFOold->setPhase(0);
    //pSawLFOold->setFrequency(frequency*2, SAMPLING_RATE);

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        pSawLFOold->updateByMIDICtrlValue(127); // pro forma
        for (int i = 0; i < steps; ++i) {
            //pSawLFOold->updateByMIDICtrlValue(float(i)/float(steps)*127.f);
            pDestinationBuffer[i] = pSawLFOold->render() * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    double elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("Old int math solution elapsed time: %.1f ms\n", elapsed_time);
    #endif

    return elapsed_time;
}

int main() {
    const int steps = STEPS;
    const int sinusoidFrequency = 100; // Hz

    #if ! SILENT
    printf("\n");
    # if SIGNED
    printf("Signed saw wave benchmark\n");
    # else
    printf("Unsigned saw wave benchmark\n");
    # endif
    printf("----------------------------------\n");
    printf("\n");
    #endif

    #if SIGNED
    pSawIntLFO = new LFOSawIntMathNew<LFO::range_signed>(MAX);
    pSawLFOold = new LFOSawIntMathOld<LFO::range_signed,true>(MAX);
    #else // unsigned
    pSawIntLFO = new LFOSawIntMathNew<LFO::range_unsigned>(MAX);
    pSawLFOold = new LFOSawIntMathOld<LFO::range_unsigned,true>(MAX);
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
        .algorithmID = SAW_NEW_INT_MATH_SOLUTION,
        .algorithmName = "New int math",
        .timeMSecs = saw_new_int_math(pOutputBuffer, pAmplitude, steps, sinusoidFrequency)
    });
    #if OUTPUT_AS_RAW_WAVE
    output_as_raw_file("saw_new_int_math.raw", pOutputBuffer, steps);
    #endif


    results.push_back({
        .algorithmID = SAW_OLD_INT_MATH_SOLUTION,
        .algorithmName = "Old int math",
        .timeMSecs = saw_old_int_math(pOutputBuffer, pAmplitude, steps, sinusoidFrequency)
    });
    #if OUTPUT_AS_RAW_WAVE
    output_as_raw_file("saw_old_int_math.raw", pOutputBuffer, steps);
    #endif


    #if ! SILENT
    printf("\nOK, 2nd try\n\n");
    #endif


    results[0].timeMSecs += saw_new_int_math(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    results[1].timeMSecs += saw_old_int_math(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);


    if (pAmplitude)    delete[] pAmplitude;
    if (pOutputBuffer) delete[] pOutputBuffer;

    if (pSawIntLFO) delete pSawIntLFO;
    if (pSawLFOold) delete pSawLFOold;

    sortResultsFirstToBeBest(results);
    printResultSummary(results);

    return results[0].algorithmID; // return the winner's numeric algorithm ID
}
