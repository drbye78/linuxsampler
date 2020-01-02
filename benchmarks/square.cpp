/*
    Square wave generator benchmark

    This is a benchmark for comparison between an integer math solution and a
    specialized pulse solution.

    Copyright (C) 2019 Christian Schoenebeck <cuse@users.sf.net>
*/

#include "lfobench.h"

#include "../src/engines/common/LFOSquareIntMath.h"
#include "../src/engines/common/LFOSquarePulse.h"

// return value of this benchmark
// to indicate the best performing solution
#define SQUARE_INT_MATH_SOLUTION    60
#define SQUARE_PULSE_SOLUTION       61
#define INVALID_RESULT              -1

#if SIGNED
LFOSquareIntMath<LFO::range_signed>* pSquareIntLFO = NULL;
LFOSquarePulse<LFO::range_signed>* pSquarePulse = NULL;
#else // unsigned
LFOSquareIntMath<LFO::range_unsigned>* pSquareIntLFO = NULL;
LFOSquarePulse<LFO::range_unsigned>* pSquarePulse = NULL;
#endif

double square_int_math(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    pSquareIntLFO->trigger(frequency, LFO::start_level_min, 0 /* max. internal depth */, 1200, false, (unsigned int) SAMPLING_RATE);
    //pSquareIntLFO->setPhase(180);
    //pSquareIntLFO->setFrequency(frequency*2, SAMPLING_RATE);

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        pSquareIntLFO->updateByMIDICtrlValue(127); // pro forma
        for (int i = 0; i < steps; ++i) {
            //pSquareIntLFO->updateByMIDICtrlValue(float(i)/float(steps)*127.f);
            pDestinationBuffer[i] = pSquareIntLFO->render() * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    double elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("int math solution elapsed time: %.1f ms\n", elapsed_time);
    #endif

    return elapsed_time;
}

double square_pulse(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    pSquarePulse->trigger(frequency, LFO::start_level_min, 0 /* max. internal depth */, 1200, false, (unsigned int) SAMPLING_RATE);
    //pSquarePulse->setPhase(180);
    //pSquarePulse->setFrequency(frequency*2, SAMPLING_RATE);

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        pSquarePulse->updateByMIDICtrlValue(127); // pro forma
        for (int i = 0; i < steps; ++i) {
            //pSquarePulse->updateByMIDICtrlValue(float(i)/float(steps)*127.f);
            pDestinationBuffer[i] = pSquarePulse->render() * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    double elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("Pulse solution elapsed time: %.1f ms\n", elapsed_time);
    #endif

    return elapsed_time;
}

int main() {
    const int steps = STEPS;
    const int sinusoidFrequency = 100; // Hz

    #if ! SILENT
    printf("\n");
    # if SIGNED
    printf("Signed square wave benchmark\n");
    # else
    printf("Unsigned square wave benchmark\n");
    # endif
    printf("----------------------------------\n");
    printf("\n");
    #endif

    #if SIGNED
    pSquareIntLFO = new LFOSquareIntMath<LFO::range_signed>(MAX);
    pSquarePulse = new LFOSquarePulse<LFO::range_signed>(MAX);
    #else // unsigned
    pSquareIntLFO = new LFOSquareIntMath<LFO::range_unsigned>(MAX);
    pSquarePulse = new LFOSquarePulse<LFO::range_unsigned>(MAX);
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
        .algorithmID = SQUARE_INT_MATH_SOLUTION,
        .algorithmName = "int math",
        .timeMSecs = square_int_math(pOutputBuffer, pAmplitude, steps, sinusoidFrequency)
    });
    #if OUTPUT_AS_RAW_WAVE
    output_as_raw_file("square_int_math.raw", pOutputBuffer, steps);
    #endif


    results.push_back({
        .algorithmID = SQUARE_PULSE_SOLUTION,
        .algorithmName = "Pulse",
        .timeMSecs = square_pulse(pOutputBuffer, pAmplitude, steps, sinusoidFrequency)
    });
    #if OUTPUT_AS_RAW_WAVE
    output_as_raw_file("square_pulse.raw", pOutputBuffer, steps);
    #endif


    #if ! SILENT
    printf("\nOK, 2nd try\n\n");
    #endif


    results[0].timeMSecs += square_int_math(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    results[1].timeMSecs += square_pulse(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);


    if (pAmplitude)    delete[] pAmplitude;
    if (pOutputBuffer) delete[] pOutputBuffer;

    if (pSquareIntLFO) delete pSquareIntLFO;
    if (pSquarePulse) delete pSquarePulse;

    sortResultsFirstToBeBest(results);
    printResultSummary(results);

    return results[0].algorithmID; // return the winner's numeric algorithm ID
}
