#ifndef __LS_NKSPCPPUNITTESTS_H__
#define __LS_NKSPCPPUNITTESTS_H__

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

class NKSPTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(NKSPTest);
    CPPUNIT_TEST(printTestSuiteName);
    CPPUNIT_TEST(testNKSPBuiltInExitFunction);
    CPPUNIT_TEST(testNKSPStringConcatOperator);
    CPPUNIT_TEST(testNKSPNegOperator);
    CPPUNIT_TEST(testNKSPPlusOperator);
    CPPUNIT_TEST(testNKSPMinusOperator);
    CPPUNIT_TEST(testNKSPModuloOperator);
    CPPUNIT_TEST(testNKSPMultiplyOperator);
    CPPUNIT_TEST(testNKSPDivideOperator);
    CPPUNIT_TEST(testNKSPSmallerThanOperator);
    CPPUNIT_TEST(testNKSPGreaterThanOperator);
    CPPUNIT_TEST(testNKSPSmallerOrEqualOperator);
    CPPUNIT_TEST(testNKSPGreaterOrEqualOperator);
    CPPUNIT_TEST(testNKSPEqualOperator);
    CPPUNIT_TEST(testNKSPUnequalOperator);
    CPPUNIT_TEST(testNKSPLogicalAndOperator);
    CPPUNIT_TEST(testNKSPLogicalOrOperator);
    CPPUNIT_TEST(testNKSPLogicalNotOperator);
    CPPUNIT_TEST(testNKSPBitwiseAndOperator);
    CPPUNIT_TEST(testNKSPBitwiseOrOperator);
    CPPUNIT_TEST(testNKSPBitwiseNotOperator);
    CPPUNIT_TEST(testNKSPPrecedenceOfOperators);
    CPPUNIT_TEST(testNKSPIntVarDeclaration);
    CPPUNIT_TEST(testNKSPIntArrayVarDeclaration);
    CPPUNIT_TEST(testNKSPRealVarDeclaration);
    CPPUNIT_TEST(testNKSPRealArrayVarDeclaration);
    CPPUNIT_TEST(testNKSPStringVarDeclaration);
    CPPUNIT_TEST(testNKSPBuiltInMinFunction);
    CPPUNIT_TEST(testNKSPBuiltInMaxFunction);
    CPPUNIT_TEST(testNKSPBuiltInAbsFunction);
    CPPUNIT_TEST(testNKSPBuiltInIncFunction);
    CPPUNIT_TEST(testNKSPBuiltInDecFunction);
    CPPUNIT_TEST(testNKSPBuiltInInRangeFunction);
    CPPUNIT_TEST(testNKSPBuiltInRandomFunction);
    CPPUNIT_TEST(testNKSPBuiltInShiftLeftFunction);
    CPPUNIT_TEST(testNKSPBuiltInShiftRightFunction);
    CPPUNIT_TEST(testNKSPBuiltInMsbFunction);
    CPPUNIT_TEST(testNKSPBuiltInLsbFunction);
    CPPUNIT_TEST(testNKSPBuiltInIntToRealFunction);
    CPPUNIT_TEST(testNKSPBuiltInRealFunction);
    CPPUNIT_TEST(testNKSPBuiltInRealToIntFunction);
    CPPUNIT_TEST(testNKSPBuiltInIntFunction);
    CPPUNIT_TEST(testNKSPBuiltInRoundFunction);
    CPPUNIT_TEST(testNKSPBuiltInCeilFunction);
    CPPUNIT_TEST(testNKSPBuiltInFloorFunction);
    CPPUNIT_TEST(testNKSPBuiltInSqrtFunction);
    CPPUNIT_TEST(testNKSPBuiltInLogFunction);
    CPPUNIT_TEST(testNKSPBuiltInLog2Function);
    CPPUNIT_TEST(testNKSPBuiltInLog10Function);
    CPPUNIT_TEST(testNKSPBuiltInExpFunction);
    CPPUNIT_TEST(testNKSPBuiltInPowFunction);
    CPPUNIT_TEST(testNKSPBuiltInSinFunction);
    CPPUNIT_TEST(testNKSPBuiltInCosFunction);
    CPPUNIT_TEST(testNKSPBuiltInTanFunction);
    CPPUNIT_TEST(testNKSPBuiltInAsinFunction);
    CPPUNIT_TEST(testNKSPBuiltInAcosFunction);
    CPPUNIT_TEST(testNKSPBuiltInAtanFunction);
    CPPUNIT_TEST(testNKSPBuiltInArrayEqualFunction);
    CPPUNIT_TEST(testNKSPBuiltInSortFunction);
    CPPUNIT_TEST(testNKSPBuiltInNumElementsFunction);
    CPPUNIT_TEST(testNKSPBuiltInSearchFunction);
    CPPUNIT_TEST(testNKSPIfStatement);
    CPPUNIT_TEST(testNKSPWhileStatement);
    CPPUNIT_TEST(testNKSPBuiltInVars);
    CPPUNIT_TEST_SUITE_END();

    public:
        void printTestSuiteName();
        void testNKSPBuiltInExitFunction();
        void testNKSPStringConcatOperator();
        void testNKSPNegOperator();
        void testNKSPPlusOperator();
        void testNKSPMinusOperator();
        void testNKSPModuloOperator();
        void testNKSPMultiplyOperator();
        void testNKSPDivideOperator();
        void testNKSPSmallerThanOperator();
        void testNKSPGreaterThanOperator();
        void testNKSPSmallerOrEqualOperator();
        void testNKSPGreaterOrEqualOperator();
        void testNKSPEqualOperator();
        void testNKSPUnequalOperator();
        void testNKSPLogicalAndOperator();
        void testNKSPLogicalOrOperator();
        void testNKSPLogicalNotOperator();
        void testNKSPBitwiseAndOperator();
        void testNKSPBitwiseOrOperator();
        void testNKSPBitwiseNotOperator();
        void testNKSPPrecedenceOfOperators();
        void testNKSPIntVarDeclaration();
        void testNKSPIntArrayVarDeclaration();
        void testNKSPRealVarDeclaration();
        void testNKSPRealArrayVarDeclaration();
        void testNKSPStringVarDeclaration();
        void testNKSPBuiltInMinFunction();
        void testNKSPBuiltInMaxFunction();
        void testNKSPBuiltInAbsFunction();
        void testNKSPBuiltInIncFunction();
        void testNKSPBuiltInDecFunction();
        void testNKSPBuiltInInRangeFunction();
        void testNKSPBuiltInRandomFunction();
        void testNKSPBuiltInShiftLeftFunction();
        void testNKSPBuiltInShiftRightFunction();
        void testNKSPBuiltInMsbFunction();
        void testNKSPBuiltInLsbFunction();
        void testNKSPBuiltInIntToRealFunction();
        void testNKSPBuiltInRealFunction();
        void testNKSPBuiltInRealToIntFunction();
        void testNKSPBuiltInIntFunction();
        void testNKSPBuiltInRoundFunction();
        void testNKSPBuiltInCeilFunction();
        void testNKSPBuiltInFloorFunction();
        void testNKSPBuiltInSqrtFunction();
        void testNKSPBuiltInLogFunction();
        void testNKSPBuiltInLog2Function();
        void testNKSPBuiltInLog10Function();
        void testNKSPBuiltInExpFunction();
        void testNKSPBuiltInPowFunction();
        void testNKSPBuiltInSinFunction();
        void testNKSPBuiltInCosFunction();
        void testNKSPBuiltInTanFunction();
        void testNKSPBuiltInAsinFunction();
        void testNKSPBuiltInAcosFunction();
        void testNKSPBuiltInAtanFunction();
        void testNKSPBuiltInArrayEqualFunction();
        void testNKSPBuiltInSortFunction();
        void testNKSPBuiltInNumElementsFunction();
        void testNKSPBuiltInSearchFunction();
        void testNKSPIfStatement();
        void testNKSPWhileStatement();
        void testNKSPBuiltInVars();
};

#endif // __LS_NKSPCPPUNITTESTS_H__
