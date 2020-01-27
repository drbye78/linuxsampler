#include "NKSPTest.h"
#include <iostream>

#define TEST_ASSERT CPPUNIT_ASSERT
#define NO_MAIN 1
#define SILENT_TEST 1
#include "../scriptvm/tests/NKSPCoreLangTest.cpp"

CPPUNIT_TEST_SUITE_REGISTRATION(NKSPTest);

// NKSPTest

void NKSPTest::printTestSuiteName() {
    cout << "\b \nRunning NKSP Tests: " << flush;
}

void NKSPTest::testNKSPBuiltInExitFunction() {
    testBuiltInExitFunction();
}

void NKSPTest::testNKSPStringConcatOperator() {
    testStringConcatOperator();
}

void NKSPTest::testNKSPNegOperator() {
    testNegOperator();
}

void NKSPTest::testNKSPPlusOperator() {
    testPlusOperator();
}

void NKSPTest::testNKSPMinusOperator() {
    testMinusOperator();
}

void NKSPTest::testNKSPModuloOperator() {
    testModuloOperator();
}

void NKSPTest::testNKSPMultiplyOperator() {
    testMultiplyOperator();
}

void NKSPTest::testNKSPDivideOperator() {
    testDivideOperator();
}

void NKSPTest::testNKSPSmallerThanOperator() {
    testSmallerThanOperator();
}

void NKSPTest::testNKSPGreaterThanOperator() {
    testGreaterThanOperator();
}

void NKSPTest::testNKSPSmallerOrEqualOperator() {
    testSmallerOrEqualOperator();
}

void NKSPTest::testNKSPGreaterOrEqualOperator() {
    testGreaterOrEqualOperator();
}

void NKSPTest::testNKSPEqualOperator() {
    testEqualOperator();
}

void NKSPTest::testNKSPUnequalOperator() {
    testUnequalOperator();
}

void NKSPTest::testNKSPLogicalAndOperator() {
    testLogicalAndOperator();
}

void NKSPTest::testNKSPLogicalOrOperator() {
    testLogicalOrOperator();
}

void NKSPTest::testNKSPLogicalNotOperator() {
    testLogicalNotOperator();
}

void NKSPTest::testNKSPBitwiseAndOperator() {
    testBitwiseAndOperator();
}

void NKSPTest::testNKSPBitwiseOrOperator() {
    testBitwiseOrOperator();
}

void NKSPTest::testNKSPBitwiseNotOperator() {
    testBitwiseNotOperator();
}

void NKSPTest::testNKSPPrecedenceOfOperators() {
    testPrecedenceOfOperators();
}

void NKSPTest::testNKSPIntVarDeclaration() {
    testIntVarDeclaration();
}

void NKSPTest::testNKSPIntArrayVarDeclaration() {
    testIntArrayVarDeclaration();
}

void NKSPTest::testNKSPRealVarDeclaration() {
    testRealVarDeclaration();
}

void NKSPTest::testNKSPRealArrayVarDeclaration() {
    testRealArrayVarDeclaration();
}

void NKSPTest::testNKSPStringVarDeclaration() {
    testStringVarDeclaration();
}

void NKSPTest::testNKSPBuiltInMinFunction() {
    testBuiltInMinFunction();
}

void NKSPTest::testNKSPBuiltInMaxFunction() {
    testBuiltInMaxFunction();
}

void NKSPTest::testNKSPBuiltInAbsFunction() {
    testBuiltInAbsFunction();
}

void NKSPTest::testNKSPBuiltInIncFunction() {
    testBuiltInIncFunction();
}

void NKSPTest::testNKSPBuiltInDecFunction() {
    testBuiltInDecFunction();
}

void NKSPTest::testNKSPBuiltInInRangeFunction() {
    testBuiltInInRangeFunction();
}

void NKSPTest::testNKSPBuiltInRandomFunction() {
    testBuiltInRandomFunction();
}

void NKSPTest::testNKSPBuiltInShiftLeftFunction() {
    testBuiltInShiftLeftFunction();
}

void NKSPTest::testNKSPBuiltInShiftRightFunction() {
    testBuiltInShiftRightFunction();
}

void NKSPTest::testNKSPBuiltInMsbFunction() {
    testBuiltInMsbFunction();
}

void NKSPTest::testNKSPBuiltInLsbFunction() {
    testBuiltInLsbFunction();
}

void NKSPTest::testNKSPBuiltInIntToRealFunction() {
    testBuiltInIntToRealFunction();
}

void NKSPTest::testNKSPBuiltInRealFunction() {
    testBuiltInRealFunction();
}

void NKSPTest::testNKSPBuiltInRealToIntFunction() {
    testBuiltInRealToIntFunction();
}

void NKSPTest::testNKSPBuiltInIntFunction() {
    testBuiltInIntFunction();
}

void NKSPTest::testNKSPBuiltInRoundFunction() {
    testBuiltInRoundFunction();
}

void NKSPTest::testNKSPBuiltInCeilFunction() {
    testBuiltInCeilFunction();
}

void NKSPTest::testNKSPBuiltInFloorFunction() {
    testBuiltInFloorFunction();
}

void NKSPTest::testNKSPBuiltInSqrtFunction() {
    testBuiltInSqrtFunction();
}

void NKSPTest::testNKSPBuiltInLogFunction() {
    testBuiltInLogFunction();
}

void NKSPTest::testNKSPBuiltInLog2Function() {
    testBuiltInLog2Function();
}

void NKSPTest::testNKSPBuiltInLog10Function() {
    testBuiltInLog10Function();
}

void NKSPTest::testNKSPBuiltInExpFunction() {
    testBuiltInExpFunction();
}

void NKSPTest::testNKSPBuiltInPowFunction() {
    testBuiltInPowFunction();
}

void NKSPTest::testNKSPBuiltInSinFunction() {
    testBuiltInSinFunction();
}

void NKSPTest::testNKSPBuiltInCosFunction() {
    testBuiltInCosFunction();
}

void NKSPTest::testNKSPBuiltInTanFunction() {
    testBuiltInTanFunction();
}

void NKSPTest::testNKSPBuiltInAsinFunction() {
    testBuiltInAsinFunction();
}

void NKSPTest::testNKSPBuiltInAcosFunction() {
    testBuiltInAcosFunction();
}

void NKSPTest::testNKSPBuiltInAtanFunction() {
    testBuiltInAtanFunction();
}

void NKSPTest::testNKSPBuiltInArrayEqualFunction() {
    testBuiltInArrayEqualFunction();
}

void NKSPTest::testNKSPBuiltInSortFunction() {
    testBuiltInSortFunction();
}

void NKSPTest::testNKSPBuiltInNumElementsFunction() {
    testBuiltInNumElementsFunction();
}

void NKSPTest::testNKSPBuiltInSearchFunction() {
    testBuiltInSearchFunction();
}

void NKSPTest::testNKSPIfStatement() {
    testIfStatement();
}

void NKSPTest::testNKSPWhileStatement() {
    testWhileStatement();
}

void NKSPTest::testNKSPBuiltInVars() {
    testBuiltInVars();
}
