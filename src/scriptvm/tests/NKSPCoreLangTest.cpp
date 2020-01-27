/*
 * Copyright (c) 2019 - 2020 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

// This file contains automated test cases against the NKSP real-time
// instrument script engine.

#include "../../common/global.h"
#include "../../common/optional.h"
#include "../../common/RTMath.h"
#include "../ScriptVM.h"
#include "../common.h"
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <functional>

#ifndef TEST_ASSERT
# define TEST_ASSERT assert
#endif

using namespace LinuxSampler;
using namespace std;

struct RunScriptOpt {
    String code;
    bool expectParseError;
    bool expectParseWarning;
    bool expectRuntimeError;
    bool expectNoExitResult;
    bool expectExitResultIsInt;
    bool expectExitResultIsReal;
    bool prohibitExitFunctionArguments;
    optional<vmint> expectIntExitResult;
    optional<bool> expectBoolExitResult;
    optional<vmfloat> expectRealExitResult;
    optional<String> expectStringExitResult;
    vector<MetricPrefix_t> expectExitResultUnitPrefix;
    optional<StdUnit_t> expectExitResultUnit;
    optional<bool> expectExitResultFinal;
};

static void runScript(const RunScriptOpt& opt) {
    ScriptVM vm;
    vm.setAutoSuspendEnabled(false);
    if (!opt.prohibitExitFunctionArguments)
        vm.setExitResultEnabled(true);
    unique_ptr<VMParserContext> parserCtx(
        vm.loadScript(opt.code)
    );
    vector<ParserIssue> errors = parserCtx->errors();
    vector<ParserIssue> warnings = parserCtx->warnings();
    if (opt.expectParseError) {
        TEST_ASSERT(!errors.empty());
        return;
    } else {
        for (ParserIssue& err : errors) {
            err.dump();
        }
        TEST_ASSERT(errors.empty());
    }
    if (opt.expectParseWarning) {
        TEST_ASSERT(!warnings.empty());
    } else {
        for (ParserIssue& wrn : warnings) {
            wrn.dump();
        }
    }
    TEST_ASSERT(parserCtx->eventHandler(0));
    unique_ptr<VMExecContext> execCtx(
        vm.createExecContext(&*parserCtx)
    );
    for (int i = 0; parserCtx->eventHandler(i); ++i) {
        VMEventHandler* handler = parserCtx->eventHandler(i);
        VMExecStatus_t result = vm.exec(&*parserCtx, &*execCtx, handler);
        if (opt.expectRuntimeError) {
            TEST_ASSERT(result & VM_EXEC_ERROR);
        } else {
            TEST_ASSERT(!(result & VM_EXEC_ERROR));
        }
        if (opt.expectNoExitResult) {
            VMExpr* resExpr = execCtx->exitResult();
            TEST_ASSERT(!resExpr);
        }
        if (opt.expectExitResultIsInt) {
            VMExpr* resExpr = execCtx->exitResult();
            TEST_ASSERT(resExpr);
            TEST_ASSERT(resExpr->exprType() == INT_EXPR);
        }
        if (opt.expectExitResultIsReal) {
            VMExpr* resExpr = execCtx->exitResult();
            TEST_ASSERT(resExpr);
            TEST_ASSERT(resExpr->exprType() == REAL_EXPR);
        }
        if (opt.expectIntExitResult) {
            VMExpr* resExpr = execCtx->exitResult();
            TEST_ASSERT(resExpr);
            TEST_ASSERT(resExpr->exprType() == INT_EXPR);
            TEST_ASSERT(resExpr->asInt()->evalInt() == *opt.expectIntExitResult);
        }
        if (opt.expectRealExitResult) {
            VMExpr* resExpr = execCtx->exitResult();
            TEST_ASSERT(resExpr);
            TEST_ASSERT(resExpr->exprType() == REAL_EXPR);
            if (sizeof(vmfloat) == sizeof(float)) {
                TEST_ASSERT(RTMath::fEqual32(resExpr->asReal()->evalReal(), *opt.expectRealExitResult));
            } else {
                TEST_ASSERT(RTMath::fEqual64(resExpr->asReal()->evalReal(), *opt.expectRealExitResult));
            }
        }
        if (opt.expectBoolExitResult) {
            VMExpr* resExpr = execCtx->exitResult();
            TEST_ASSERT(resExpr);
            TEST_ASSERT(resExpr->exprType() == INT_EXPR);
            TEST_ASSERT(bool(resExpr->asInt()->evalInt()) == *opt.expectBoolExitResult);
        }
        if (opt.expectStringExitResult) {
            VMExpr* resExpr = execCtx->exitResult();
            TEST_ASSERT(resExpr);
            TEST_ASSERT(resExpr->exprType() == STRING_EXPR);
            TEST_ASSERT(resExpr->asString()->evalStr() == *opt.expectStringExitResult);
        }
        if (opt.expectExitResultUnit) {
            VMExpr* resExpr = execCtx->exitResult();
            TEST_ASSERT(resExpr);
            VMNumberExpr* numberExpr = resExpr->asNumber();
            TEST_ASSERT(numberExpr);
            TEST_ASSERT(numberExpr->unitType() == *opt.expectExitResultUnit);
        }
        if (!opt.expectExitResultUnitPrefix.empty()) {
            VMExpr* resExpr = execCtx->exitResult();
            TEST_ASSERT(resExpr);
            VMNumberExpr* numberExpr = resExpr->asNumber();
            TEST_ASSERT(numberExpr);
            auto prefixes = opt.expectExitResultUnitPrefix;
            if (*prefixes.rbegin() != VM_NO_PREFIX)
                prefixes.push_back(VM_NO_PREFIX); // VM_NO_PREFIX termination required by unitFactr() call
            vmfloat expectedFactor = VMUnit::unitFactor(&prefixes[0]);
            vmfloat actualFactor = numberExpr->unitFactor();
            if (sizeof(vmfloat) == sizeof(float)) {
                TEST_ASSERT(RTMath::fEqual32(expectedFactor, actualFactor));
            } else {
                TEST_ASSERT(RTMath::fEqual64(expectedFactor, actualFactor));
            }
        }
        if (opt.expectExitResultFinal) {
            VMExpr* resExpr = execCtx->exitResult();
            TEST_ASSERT(resExpr);
            VMNumberExpr* numberExpr = resExpr->asNumber();
            TEST_ASSERT(numberExpr);
            TEST_ASSERT(numberExpr->isFinal() == *opt.expectExitResultFinal);
        }
    }
}

static void testBuiltInExitFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in exit() function\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
end on
)NKSP_CODE",
        .expectNoExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit
end on
)NKSP_CODE",
        .expectNoExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit()
end on
)NKSP_CODE",
        .expectNoExitResult = true
    });

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42)
end on
)NKSP_CODE",
        .expectIntExitResult = 42
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  if ($foo)
    exit(21)
  end if
end on
)NKSP_CODE",
        .expectIntExitResult = 21
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 0
  if ($foo)
    exit(21)
  end if
  exit(99)
end on
)NKSP_CODE",
        .expectIntExitResult = 99
    });

    // string tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit("fourtytwo!")
end on
)NKSP_CODE",
        .expectStringExitResult = "fourtytwo!"
    });

    // in production environment we prohibit the built-in exit() function to
    // accept any arguments
    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42)
end on
)NKSP_CODE",
        .expectParseError = true, // see comment above why
        .prohibitExitFunctionArguments = true // simulate production environment
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.14)
end on
)NKSP_CODE",
        .expectRealExitResult = 3.14
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  if ($foo)
    exit(3.14)
  end if
end on
)NKSP_CODE",
        .expectRealExitResult = 3.14
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 0
  if ($foo)
    exit(3.14)
  end if
  exit(6.9)
end on
)NKSP_CODE",
        .expectRealExitResult = 6.9
    });

    // int array tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3]
  %foo[0] := 21
  exit(%foo[0])
end on
)NKSP_CODE",
        .expectIntExitResult = 21
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 12, 23, 34 )
  exit(%foo[0])
end on
)NKSP_CODE",
        .expectIntExitResult = 12
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 12, 23, 34 )
  exit(%foo[1])
end on
)NKSP_CODE",
        .expectIntExitResult = 23
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 12, 23, 34 )
  exit(%foo[2])
end on
)NKSP_CODE",
        .expectIntExitResult = 34
    });

    runScript({ // make sure array is entirely initialized with zeroes
        .code = R"NKSP_CODE(
on init
  declare $i
  declare $result
  declare %foo[100]
  while ($i < 100)
      $result := $result .or. %foo[$i]
      inc($i)
  end while
  exit($result)
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    // real array tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3]
  ?foo[0] := 34.9
  exit(?foo[0])
end on
)NKSP_CODE",
        .expectRealExitResult = 34.9
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 0.3, 23.5, 900.1 )
  exit(?foo[0])
end on
)NKSP_CODE",
        .expectRealExitResult = 0.3
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 0.3, 23.5, 900.1 )
  exit(?foo[1])
end on
)NKSP_CODE",
        .expectRealExitResult = 23.5
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 0.3, 23.5, 900.1 )
  exit(?foo[2])
end on
)NKSP_CODE",
        .expectRealExitResult = 900.1
    });

    runScript({ // make sure array is entirely initialized with zeroes
        .code = R"NKSP_CODE(
on init
  declare $i
  declare ?foo[100]
  while ($i < 100)
      if (?foo[$i] # 0.0)
        exit(-1) { test failed }
      end if
      inc($i)
  end while
  exit(0) { test succeeded }
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42s)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42Hz)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnit = VM_HERTZ
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42B)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42us)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42cs)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_CENTI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42ds)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_DECI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42das)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_DECA },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42hs)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_HECTO },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42ks)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42s)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42uHz)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_HERTZ
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42mHz)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_HERTZ
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42cHz)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_CENTI },
        .expectExitResultUnit = VM_HERTZ
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42dHz)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_DECI },
        .expectExitResultUnit = VM_HERTZ
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42daHz)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_DECA },
        .expectExitResultUnit = VM_HERTZ
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42hHz)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_HECTO },
        .expectExitResultUnit = VM_HERTZ
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42kHz)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42Hz)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_HERTZ
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42uB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42mB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42cB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_CENTI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42dB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42daB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_DECA },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42hB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_HECTO },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42kB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42B)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42udB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_MICRO, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42mdB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42cdB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_CENTI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42ddB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_DECI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42dadB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_DECA, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42hdB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_HECTO, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42kdB)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_KILO, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 42mdB
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.14s)
end on
)NKSP_CODE",
        .expectRealExitResult = 3.14,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.14us)
end on
)NKSP_CODE",
        .expectRealExitResult = 3.14,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.14ms)
end on
)NKSP_CODE",
        .expectRealExitResult = 3.14,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-0.1B)
end on
)NKSP_CODE",
        .expectRealExitResult = -0.1,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-0.1dB)
end on
)NKSP_CODE",
        .expectRealExitResult = -0.1,
        .expectExitResultUnitPrefix = { VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-0.1mdB)
end on
)NKSP_CODE",
        .expectRealExitResult = -0.1,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := -0.1mdB
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = -0.1,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := 0.0dB
  ~foo := -0.1mdB
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = -0.1,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := 0.0dB
  ~foo := -0.1Hz
  exit(~foo)
end on
)NKSP_CODE",
        .expectParseError = true // assigning different unit type to a variable is not allowed
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!42)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := !42
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 42
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 42,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := !3.14
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 3.14,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := 3.14
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 3.14,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := !3.14mdB
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 3.14,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := !0.0mdB
  ~foo := !3.14mdB
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 3.14,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := !0.0mdB
  ~foo := 3.14mdB
  exit(~foo)
end on
)NKSP_CODE",
        .expectParseError = true // assigning non-final to a final variable not allowed
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := 0.0mdB
  ~foo := !3.14mdB
  exit(~foo)
end on
)NKSP_CODE",
        .expectParseError = true // assigning final to a non-final variable not allowed
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testStringConcatOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: string concatenation (&) operator\n";
    #endif

    // strings only tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare @s := "foo" & " bar"
  exit(@s)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo bar"
    });

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare @s := "foo" & " bar" & " " & 123
  exit(@s)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo bar 123"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $i := 123
  declare @s := "foo" & " bar" & " " & $i
  exit(@s)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo bar 123"
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare @s := "foo" & " bar" & " " & 1.23
  exit(@s)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo bar 1.23"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~r := 3.14
  declare @s := "foo" & " bar" & " " & ~r
  exit(@s)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo bar 3.14"
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $i := 500Hz
  declare @s := "foo" & " bar" & " " & $i
  exit(@s)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo bar 500Hz"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~r := 3.14s
  declare @s := "foo" & " bar" & " " & ~r
  exit(@s)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo bar 3.14s"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~r := -22.3mdB
  declare @s := "foo" & " bar" & " " & ~r
  exit(@s)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo bar -22.3mdB"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $i := 20us
  declare @s := "foo" & " bar" & " " & $i
  exit(@s)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo bar 20us"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $i := 20kHz
  declare @s := "foo" & " bar" & " " & $i
  exit(@s)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo bar 20kHz"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $i := -6dB
  declare @s := "foo" & " bar" & " " & $i
  exit(@s)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo bar -6dB"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $i := 1us * 1d
  declare @s := "foo" & " bar" & " " & $i
  exit(@s)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo bar 1*10^-7s"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~r := 12.4mc
  declare @s := "foo" & " bar" & " " & ~r
  exit(@s)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo bar 12.4mc"
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testNegOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: negate (-) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-87)
end on
)NKSP_CODE",
        .expectIntExitResult = -87
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := -87
  exit(-$foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 87
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-99.3)
end on
)NKSP_CODE",
        .expectRealExitResult = -99.3
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := -99.3
  exit(-~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 99.3
    });

    // std unit tests

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := -87mdB
  exit(-$foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 87,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := !-87
  exit(-$foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 87,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := -87
  exit(-$foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 87,
        .expectExitResultFinal = false
    });

    //TODO: the following are unary '+' operator tests which should be moved to their own function (lazy me).

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(+54)
end on
)NKSP_CODE",
        .expectIntExitResult = 54
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := +54
  exit( $foo )
end on
)NKSP_CODE",
        .expectIntExitResult = 54
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(+0.45)
end on
)NKSP_CODE",
        .expectRealExitResult = 0.45
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := +0.29
  exit( ~foo )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.29
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testPlusOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: plus (+) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 + 3)
end on
)NKSP_CODE",
        .expectIntExitResult = 7
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42 + 145)
end on
)NKSP_CODE",
        .expectIntExitResult = 187
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4 + 2)
end on
)NKSP_CODE",
        .expectIntExitResult = -2
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4.0 + 3.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 7.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42.3 + 145.2)
end on
)NKSP_CODE",
        .expectRealExitResult = 187.5
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4.0 + 2.2)
end on
)NKSP_CODE",
        .expectRealExitResult = -1.8
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42ms + 145ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 187,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s + 145ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 1145,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42ms + 145)
end on
)NKSP_CODE",
        .expectParseError = true // units must match for + operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42 + 145ms)
end on
)NKSP_CODE",
        .expectParseError = true // units must match for + operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42Hz + 145s)
end on
)NKSP_CODE",
        .expectParseError = true // units must match for + operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42.1ms + 145.3ms)
end on
)NKSP_CODE",
        .expectRealExitResult = 187.4,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.1s + 145.0ms)
end on
)NKSP_CODE",
        .expectRealExitResult = 1245.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42.1ms + 145.3)
end on
)NKSP_CODE",
        .expectParseError = true // units must match for + operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42.0 + 145.0ms)
end on
)NKSP_CODE",
        .expectParseError = true // units must match for + operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(42.0Hz + 145.0s)
end on
)NKSP_CODE",
        .expectParseError = true // units must match for + operator
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!4 + !3)
end on
)NKSP_CODE",
        .expectIntExitResult = 7,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 + 3)
end on
)NKSP_CODE",
        .expectIntExitResult = 7,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!4.1 + !3.3)
end on
)NKSP_CODE",
        .expectRealExitResult = 7.4,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4.1 + 3.3)
end on
)NKSP_CODE",
        .expectRealExitResult = 7.4,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testMinusOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: minus (-) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 - 3)
end on
)NKSP_CODE",
        .expectIntExitResult = 1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(139 - 74)
end on
)NKSP_CODE",
        .expectIntExitResult = 65
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 - 9)
end on
)NKSP_CODE",
        .expectIntExitResult = -6
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-3 - 18)
end on
)NKSP_CODE",
        .expectIntExitResult = -21
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4.0 - 0.2)
end on
)NKSP_CODE",
        .expectRealExitResult = 3.8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.1 - 9.65)
end on
)NKSP_CODE",
        .expectRealExitResult = -6.55
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-3.0 - 18.1)
end on
)NKSP_CODE",
        .expectRealExitResult = -21.1
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1000ms - 145ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 855,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s - 145ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 855,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s - 145)
end on
)NKSP_CODE",
        .expectParseError = true // units must match for - operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 - 145s)
end on
)NKSP_CODE",
        .expectParseError = true // units must match for - operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1ms - 145mB)
end on
)NKSP_CODE",
        .expectParseError = true // units must match for - operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0ms - 0.1ms)
end on
)NKSP_CODE",
        .expectRealExitResult = 0.9,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.1s - 106.0ms)
end on
)NKSP_CODE",
        .expectRealExitResult = 994.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1100.0ms - 0.106s)
end on
)NKSP_CODE",
        .expectRealExitResult = 994.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0s - 145.0)
end on
)NKSP_CODE",
        .expectParseError = true // units must match for - operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0 - 145.0s)
end on
)NKSP_CODE",
        .expectParseError = true // units must match for - operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0ms - 145.0mB)
end on
)NKSP_CODE",
        .expectParseError = true // units must match for - operator
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!5 - !3)
end on
)NKSP_CODE",
        .expectIntExitResult = 2,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(5 - 3)
end on
)NKSP_CODE",
        .expectIntExitResult = 2,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!5.9 - !3.3)
end on
)NKSP_CODE",
        .expectRealExitResult = 2.6,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(5.9 - 3.3)
end on
)NKSP_CODE",
        .expectRealExitResult = 2.6,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testModuloOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: modulo (mod) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10 mod 8)
end on
)NKSP_CODE",
        .expectIntExitResult = 2
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := 10
  declare $b := 8
  exit($a mod $b)
end on
)NKSP_CODE",
        .expectIntExitResult = 2
    });

    // real number tests ...
    // (mod operator prohibits real numbers ATM)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10.0 mod 8.0)
end on
)NKSP_CODE",
        .expectParseError = true // mod operator prohibits real numbers ATM
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10 mod 8.0)
end on
)NKSP_CODE",
        .expectParseError = true // mod operator prohibits real numbers ATM
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10.0 mod 8)
end on
)NKSP_CODE",
        .expectParseError = true // mod operator prohibits real numbers ATM
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := 10.0
  declare ~b := 8.0
  exit(~a mod ~b)
end on
)NKSP_CODE",
        .expectParseError = true // mod operator prohibits real numbers ATM
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10s mod 8)
end on
)NKSP_CODE",
        .expectParseError = true // mod operator prohibits std units ATM
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10 mod 8s)
end on
)NKSP_CODE",
        .expectParseError = true // mod operator prohibits std units ATM
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10s mod 8s)
end on
)NKSP_CODE",
        .expectParseError = true // mod operator prohibits std units ATM
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!10 mod !8)
end on
)NKSP_CODE",
        .expectIntExitResult = 2,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10 mod 8)
end on
)NKSP_CODE",
        .expectIntExitResult = 2,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testMultiplyOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: multiply (*) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10 * 8)
end on
)NKSP_CODE",
        .expectIntExitResult = 80
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-3 * -4)
end on
)NKSP_CODE",
        .expectIntExitResult = 12
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-52 * 63)
end on
)NKSP_CODE",
        .expectIntExitResult = -3276
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(123 * -59)
end on
)NKSP_CODE",
        .expectIntExitResult = -7257
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10.2 * 8.4)
end on
)NKSP_CODE",
        .expectRealExitResult = 85.68
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10.0 * -3.33)
end on
)NKSP_CODE",
        .expectRealExitResult = -33.3
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-3.33 * 10.0)
end on
)NKSP_CODE",
        .expectRealExitResult = -33.3
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-3.33 * -10.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 33.3
    });

    // mixed type tests ...
    // (mixed int * real forbidden ATM)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(2 * 3.0)
end on
)NKSP_CODE",
        .expectParseError = true // mixed int * real forbidden ATM
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(2.0 * 3)
end on
)NKSP_CODE",
        .expectParseError = true // mixed int * real forbidden ATM
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10ms * 8)
end on
)NKSP_CODE",
        .expectIntExitResult = 80,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10 * 8ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 80,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10s * 8s)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides not allowed for * ATM
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10cs * 8d)
end on
)NKSP_CODE",
        .expectIntExitResult = 80,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10m * 8ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 80,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10ms * 8k)
end on
)NKSP_CODE",
        .expectIntExitResult = 80,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10.1ms * 8.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 80.8,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10.1 * 8.0ms)
end on
)NKSP_CODE",
        .expectRealExitResult = 80.8,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10.0s * 8.0s)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides not allowed for * ATM
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10.1ds * 8.0c)
end on
)NKSP_CODE",
        .expectRealExitResult = 80.8,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10.1m * 8.0ms)
end on
)NKSP_CODE",
        .expectRealExitResult = 80.8,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10.1m * 8.0ks)
end on
)NKSP_CODE",
        .expectRealExitResult = 80.8,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_SECOND
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!10 * !8)
end on
)NKSP_CODE",
        .expectIntExitResult = 80,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10 * 8)
end on
)NKSP_CODE",
        .expectIntExitResult = 80,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!10 * 8)
end on
)NKSP_CODE",
        .expectIntExitResult = 80,
        .expectExitResultFinal = true,
        .expectParseWarning = true // since final only on one side, result will be final though
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10 * !8)
end on
)NKSP_CODE",
        .expectIntExitResult = 80,
        .expectExitResultFinal = true,
        .expectParseWarning = true // since final only on one side, result will be final though
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!10.1 * !8.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 80.8,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10.1 * 8.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 80.8,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!10.1 * 8.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 80.8,
        .expectExitResultFinal = true,
        .expectParseWarning = true // since final only on one side, result will be final though
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(10.1 * !8.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 80.8,
        .expectExitResultFinal = true,
        .expectParseWarning = true // since final only on one side, result will be final though
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testDivideOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: divide (/) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9 / 3)
end on
)NKSP_CODE",
        .expectIntExitResult = 3
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-27 / 3)
end on
)NKSP_CODE",
        .expectIntExitResult = -9
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(35 / -5)
end on
)NKSP_CODE",
        .expectIntExitResult = -7
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(39 / -5)
end on
)NKSP_CODE",
        .expectIntExitResult = -7
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9.0 / 10.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 0.9
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-9.0 / 10.0)
end on
)NKSP_CODE",
        .expectRealExitResult = -0.9
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9.0 / -10.0)
end on
)NKSP_CODE",
        .expectRealExitResult = -0.9
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-9.0 / -10.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 0.9
    });

    // mixed type tests ...
    // (mixed int / real forbidden ATM)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9 / 10.0)
end on
)NKSP_CODE",
        .expectParseError = true // mixed int / real forbidden ATM
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9.0 / 10)
end on
)NKSP_CODE",
        .expectParseError = true // mixed int / real forbidden ATM
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-27us / 3)
end on
)NKSP_CODE",
        .expectIntExitResult = -9,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-27mdB / 3mdB)
end on
)NKSP_CODE",
        .expectIntExitResult = -9,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-27s / 3m)
end on
)NKSP_CODE",
        .expectIntExitResult = -9,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-27us / 3m)
end on
)NKSP_CODE",
        .expectIntExitResult = -9,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-27 / 3s)
end on
)NKSP_CODE",
        .expectParseError = true // illegal unit type arrangement for divisions
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-27s / 3Hz)
end on
)NKSP_CODE",
        .expectParseError = true // unit types are not matching
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!-27 / !3)
end on
)NKSP_CODE",
        .expectIntExitResult = -9,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-27 / 3)
end on
)NKSP_CODE",
        .expectIntExitResult = -9,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!-27 / 3)
end on
)NKSP_CODE",
        .expectIntExitResult = -9,
        .expectExitResultFinal = true,
        .expectParseWarning = true // final only on one side, result will be final though
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-27 / !3)
end on
)NKSP_CODE",
        .expectIntExitResult = -9,
        .expectExitResultFinal = true,
        .expectParseWarning = true // final only on one side, result will be final though
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testSmallerThanOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: smaller than (<) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 < 4)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 < 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4 < 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 < -4)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(123 < -45)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-45 < 123)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 < 4.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4.0 < 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.2 < 1.23)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.23 < 1.2)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4.0 < 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 < -4.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(123.0 < -45.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-45.0 < 123.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // mixed type tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9 < 9.1)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9.1 < 9)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13ms < 14ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(14ms < 13ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s < 990ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(990ms < 1s)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1000ms < 1s)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s < 1000ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s < 1)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 < 1s)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1Hz < 1B)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13.0ms < 13.1ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13.1ms < 13.0ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(0.9s < 600.0ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(600.0ms < 0.9s)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(5.1kHz < 5100.0Hz)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(5100.0Hz < 5.1kHz)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0Hz < 1.1)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.2 < 1.34mdB)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9.23us < 3.14kHz)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    // 'final' ('!') operator tests ...
    // (should always yield in false for relation operators)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!-4 < !3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4 < 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testGreaterThanOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: greater than (>) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 > 4)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 > 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4 > 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 > -4)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(123 > -45)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-45 > 123)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 > 4.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4.0 > 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.2 > 1.23)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.23 > 1.2)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4.0 > 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 > -4.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(123.0 > -45.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-45.0 > 123.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    // mixed type tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9 > 9.1)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9.1 > 9)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13ms > 14ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(14ms > 13ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s > 990ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(990ms > 1s)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1000ms > 1s)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s > 1000ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s > 1)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 > 1s)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1Hz > 1B)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13.0ms > 13.1ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13.1ms > 13.0ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(0.9s > 600.0ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(600.0ms > 0.9s)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(5.1kHz > 5100.0Hz)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(5100.0Hz > 5.1kHz)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0Hz > 1.1)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.2 > 1.34mdB)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9.23us > 3.14kHz)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    // 'final' ('!') operator tests ...
    // (should always yield in false for relation operators)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!-4 > !3)
end on
)NKSP_CODE",
        .expectBoolExitResult = false,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4 > 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = false,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testSmallerOrEqualOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: smaller-or-equal (<=) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 <= 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 <= 4)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-23 <= -23)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(23 <= -23)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 <= 4)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 <= 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4 <= 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 <= -4)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(123 <= -45)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-45 <= 123)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 <= 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4.33 <= 4.33)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-23.1 <= -23.1)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(23.3 <= -23.3)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 <= 4.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4.0 <= 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4.0 <= 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 <= -4.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(123.0 <= -45.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-45.0 <= 123.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // mixed type tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9 <= 9.1)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9.1 <= 9)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9 <= 9.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9.0 <= 9)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13ms <= 14ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(14ms <= 13ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s <= 990ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(990ms <= 1s)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1000ms <= 1s)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s <= 1000ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s <= 1)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 <= 1s)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1Hz <= 1B)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13.0ms <= 13.1ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13.1ms <= 13.0ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(0.9s <= 600.0ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(600.0ms <= 0.9s)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(5.1kHz <= 5100.0Hz)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(5100.0Hz <= 5.1kHz)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0Hz <= 1.1)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.2 <= 1.34mdB)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9.23us <= 3.14kHz)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    // 'final' ('!') operator tests ...
    // (should always yield in false for relation operators)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!-4 <= !3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4 <= 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testGreaterOrEqualOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: greater-or-equal (>=) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 >= 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 >= 4)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-23 >= -23)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(23 >= -23)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 >= 4)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 >= 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4 >= 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 >= -4)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(123 >= -45)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-45 >= 123)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 >= 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.1 >= 3.1)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.1 >= 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 >= 3.1)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-23.33 >= -23.33)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(23.0 >= -23.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 >= 4.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4.0 >= 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4.0 >= 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 >= -4.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(123.0 >= -45.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-45.0 >= 123.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    // mixed type tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9 >= 9.1)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9.1 >= 9)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9 >= 9.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9.0 >= 9)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13ms >= 14ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(14ms >= 13ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s >= 990ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(990ms >= 1s)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1000ms >= 1s)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s >= 1000ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s >= 1)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 >= 1s)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1Hz >= 1B)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13.0ms >= 13.1ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13.1ms >= 13.0ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(0.9s >= 600.0ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(600.0ms >= 0.9s)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(5.1kHz >= 5100.0Hz)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(5100.0Hz >= 5.1kHz)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0Hz >= 1.1)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.2 >= 1.34mdB)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(9.23us >= 3.14kHz)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    // 'final' ('!') operator tests ...
    // (should always yield in false for relation operators)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!-4 >= !3)
end on
)NKSP_CODE",
        .expectBoolExitResult = false,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4 >= 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = false,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testEqualOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: equal (=) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 = 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 = 4)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 = 4)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(23 = -23)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 = 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4.33 = 4.33)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4.31 = 4.35)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 = 4.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(23.0 = -23.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    // deal with inaccuracy of float point
    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := 0.165
  declare ~b := 0.185
  declare ~x := 0.1
  declare ~y := 0.25
  exit(~a + ~b = ~x + ~y) { both sides should actually be 0.35, they slightly deviate both though }
end on
)NKSP_CODE",
        .expectBoolExitResult = true // our implementation of real number equal comparison should take care about floating point tolerance
    });

    // deal with inaccuracy of float point
    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := 0.166
  declare ~b := 0.185
  declare ~x := 0.1
  declare ~y := 0.25
  exit(~a + ~b = ~x + ~y) { left side approx. 0.351, right side approx. 0.35 }
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    // mixed type tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(23 = 23.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(23.0 = 23)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(23 = 23.1)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(23.1 = 23)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13ms = 14ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(14ms = 13ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s = 1ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1ms = 1s)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.14kHz = 3140Hz)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3140Hz = 3.14kHz)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s = 1)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 = 1s)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1Hz = 1B)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    // 'final' ('!') operator tests ...
    // (should always yield in false for relation operators)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!-4 = !3)
end on
)NKSP_CODE",
        .expectBoolExitResult = false,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4 = 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = false,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testUnequalOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: unequal (#) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 # 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 # 4)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 # 4)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(23 # -23)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 # 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.14 # 3.14)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.19 # 3.12)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(23.0 # -23.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // deal with inaccuracy of float point
    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := 0.165
  declare ~b := 0.185
  declare ~x := 0.1
  declare ~y := 0.25
        exit(~a + ~b # ~x + ~y) { both sides should actually be 0.35, they slightly deviate both though }
end on
)NKSP_CODE",
        .expectBoolExitResult = false // our implementation of real number unequal comparison should take care about floating point tolerance
    });

    // deal with inaccuracy of float point
    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := 0.166
  declare ~b := 0.185
  declare ~x := 0.1
  declare ~y := 0.25
        exit(~a + ~b # ~x + ~y) { left side approx. 0.351, right side approx. 0.35 }
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // mixed type tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 # 3.0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.0 # 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.1 # 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 # 3.1)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(13ms # 14ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(14ms # 13ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s # 1ms)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1ms # 1s)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.14kHz # 3140Hz)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3140Hz # 3.14kHz)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s # 1)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 # 1s)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1Hz # 1B)
end on
)NKSP_CODE",
        .expectParseError = true // units on both sides must match
    });

    // 'final' ('!') operator tests ...
    // (should always yield in false for relation operators)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!-4 # !3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(-4 # 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testLogicalAndOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: logical and (and) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 and 1)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 and 2)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 and 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 and 0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(0 and 1)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(0 and 0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    // real number tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0 and 1.0)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    // mixed type tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0 and 1)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 and 1.0)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    // std unit tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s and 0)
end on
)NKSP_CODE",
        .expectParseError = true // std units not allowed for this operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(0 and 1s)
end on
)NKSP_CODE",
        .expectParseError = true // std units not allowed for this operator
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!0 and !0)
end on
)NKSP_CODE",
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(0 and 0)
end on
)NKSP_CODE",
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testLogicalOrOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: logical or (or) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 or 1)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 or 2)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 or 3)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 or 0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(0 or 1)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(0 or 0)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    // real number tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0 or 1.0)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    // mixed type tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0 or 1)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 or 1.0)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    // std unit tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s or 0)
end on
)NKSP_CODE",
        .expectParseError = true // std units not allowed for this operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(0 or 1s)
end on
)NKSP_CODE",
        .expectParseError = true // std units not allowed for this operator
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!0 or !0)
end on
)NKSP_CODE",
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(0 or 0)
end on
)NKSP_CODE",
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testLogicalNotOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: logical not (not) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(not 1)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(not 2)
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(not 0)
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // real number tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(not 1.0)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    // std unit tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(not 1s)
end on
)NKSP_CODE",
        .expectParseError = true // std units not allowed for this operator
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(not !1)
end on
)NKSP_CODE",
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(not 1)
end on
)NKSP_CODE",
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBitwiseAndOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: bitwise and (.and.) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 .and. 1)
end on
)NKSP_CODE",
        .expectIntExitResult = 1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(43 .and. 142)
end on
)NKSP_CODE",
        .expectIntExitResult = 10
    });

    // real number tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0 .and. 1.0)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    // mixed type tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0 .and. 1)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 .and. 1.0)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    // std unit tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s .and. 1)
end on
)NKSP_CODE",
        .expectParseError = true // std units not allowed for this operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 .and. 1s)
end on
)NKSP_CODE",
        .expectParseError = true // std units not allowed for this operator
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!43 .and. !142)
end on
)NKSP_CODE",
        .expectIntExitResult = 10,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(43 .and. 142)
end on
)NKSP_CODE",
        .expectIntExitResult = 10,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBitwiseOrOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: bitwise or (.or.) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 .or. 1)
end on
)NKSP_CODE",
        .expectIntExitResult = 1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(0 .or. 0)
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(43 .or. 142)
end on
)NKSP_CODE",
        .expectIntExitResult = 175
    });

    // real number tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0 .or. 1.0)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    // mixed type tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1.0 .or. 1)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 .or. 1.0)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    // std unit tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1s .or. 1)
end on
)NKSP_CODE",
        .expectParseError = true // std units not allowed for this operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(1 .or. 1s)
end on
)NKSP_CODE",
        .expectParseError = true // std units not allowed for this operator
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(!43 .or. !142)
end on
)NKSP_CODE",
        .expectIntExitResult = 175,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(43 .or. 142)
end on
)NKSP_CODE",
        .expectIntExitResult = 175,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBitwiseNotOperator() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: bitwise not (.not.) operator\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(.not. -1)
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(.not. 0)
end on
)NKSP_CODE",
        .expectIntExitResult = -1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(.not. 3)
end on
)NKSP_CODE",
        .expectIntExitResult = ssize_t(size_t(-1) << 2)
    });

    // real number tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(.not. 1.0)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(.not. -1.0)
end on
)NKSP_CODE",
        .expectParseError = true // real numbers not allowed for this operator
    });

    // std unit tests ...
    // (not allowed for this operator)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(.not. 1s)
end on
)NKSP_CODE",
        .expectParseError = true // std units not allowed for this operator
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(.not. !0)
end on
)NKSP_CODE",
        .expectIntExitResult = -1,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(.not. 0)
end on
)NKSP_CODE",
        .expectIntExitResult = -1,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testPrecedenceOfOperators() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: precedence of operators\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3 + 4 * 2)
end on
)NKSP_CODE",
        .expectIntExitResult = 11
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 * 2 + 3)
end on
)NKSP_CODE",
        .expectIntExitResult = 11
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit((3 + 4) * 2)
end on
)NKSP_CODE",
        .expectIntExitResult = 14
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 * (2 + 3))
end on
)NKSP_CODE",
        .expectIntExitResult = 20
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(3.2 + 4.0 * 2.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 11.2
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4.0 * 2.0 + 3.2)
end on
)NKSP_CODE",
        .expectRealExitResult = 11.2
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit((3.2 + 4.0) * 2.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 14.4
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4.0 * (2.0 + 3.2))
end on
)NKSP_CODE",
        .expectRealExitResult = 20.8
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4 * (2us + 3us) + 7ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 7020,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := 4
  declare $c := 3us
  exit($a * (2us + $c) + 7ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 7020,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := 4
  declare $b := 2us
  declare $c := 3us
  exit($a * ($b + $c) + 7ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 7020,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := 4
  declare $b := 2us
  declare $c := 3us
  declare $d := 7ms
  exit($a * ($b + $c) + 7ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 7020,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $c := 3us
  declare $a := 4
  declare $d := 7ms
  declare $b := 2us
  exit($a * ($b + $c) + 7ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 7020,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(4.0 * (2.0mdB + 3.2mdB) / 2.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 10.4,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := 2.0mdB
  declare ~b := 3.2mdB
  exit(4.0 * (~a + ~b) / 2.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 10.4,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~b := 3.2mdB
  declare ~a := 2.0mdB
  exit(4.0 * (~a + ~b) / 2.0)
end on
)NKSP_CODE",
        .expectRealExitResult = 10.4,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := 4.0
  declare ~b := 2.0mdB
  declare ~c := 3.2mdB
  declare ~d := 2.0
  exit((~a * (~b + ~c) / ~d) + 1.1mdB)
end on
)NKSP_CODE",
        .expectRealExitResult = 11.5,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~c := 3.2mdB
  declare ~a := 4.0
  declare ~d := 2.0
  declare ~b := 2.0mdB
  exit(~a * (~b + ~c) / ~d + 1.1mdB)
end on
)NKSP_CODE",
        .expectRealExitResult = 11.5,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := 4
  declare $b := !2us
  declare $c := 3us
  declare $d := 7ms
  exit($a * ($b + $c) + 7ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 7020,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND,
        .expectExitResultFinal = true,
        .expectParseWarning = true // only one operand is defined as 'final', result will be final though
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := 4
  declare $b := 2us
  declare $c := !3us
  declare $d := 7ms
  exit($a * ($b + $c) + 7ms)
end on
)NKSP_CODE",
        .expectIntExitResult = 7020,
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND,
        .expectExitResultFinal = true,
        .expectParseWarning = true // only one operand is defined as 'final', result will be final though
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testIntVarDeclaration() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: int var declaration\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a
  exit($a)
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := 24
  exit($a)
end on
)NKSP_CODE",
        .expectIntExitResult = 24
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := 24
  $a := 8
  exit($a)
end on
)NKSP_CODE",
        .expectIntExitResult = 8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a
  declare $a
end on
)NKSP_CODE",
        .expectParseError = true // variable re-declaration
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const $a
end on
)NKSP_CODE",
        .expectParseError = true // const variable declaration without assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const $a := 24
  exit($a)
end on
)NKSP_CODE",
        .expectIntExitResult = 24
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const $a := 24
  $a := 8
end on
)NKSP_CODE",
        .expectParseError = true // attempt to modify const variable
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const $a := 24
  declare const $b := $a
  exit($b)
end on
)NKSP_CODE",
        .expectIntExitResult = 24
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := 24
  declare const $b := $a
end on
)NKSP_CODE",
        .expectParseError = true // const variable defined with non-const assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic $a
  exit($a)
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const polyphonic $a
end on
)NKSP_CODE",
        .expectParseError = true // combination of qualifiers 'const' and 'polyphonic' is pointless
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic const $a
end on
)NKSP_CODE",
        .expectParseError = true // combination of qualifiers 'const' and 'polyphonic' is pointless
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const polyphonic $a := 3
end on
)NKSP_CODE",
        .expectParseError = true // combination of qualifiers 'const' and 'polyphonic' is pointless
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic const $a := 3
end on
)NKSP_CODE",
        .expectParseError = true // combination of qualifiers 'const' and 'polyphonic' is pointless
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := 24
  exit(~a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // real type declaration vs. int value assignment
        .expectIntExitResult = 24
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a := 24
  exit(%a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // int array type declaration vs. int scalar value assignment
        .expectIntExitResult = 24
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const %a := 24
  exit(%a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // int array type declaration vs. int scalar value assignment
        .expectIntExitResult = 24
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a := 24
  exit(?a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // real array type declaration vs. int scalar value assignment
        .expectIntExitResult = 24
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ?a := 24
  exit(?a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // real array type declaration vs. int scalar value assignment
        .expectIntExitResult = 24
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare @a := 24
  exit(@a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // string type declaration vs. int scalar value assignment
        .expectIntExitResult = 24
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const @a := 24
  exit(@a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // string type declaration vs. int scalar value assignment
        .expectIntExitResult = 24
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := ( 0, 1, 2 )
end on
)NKSP_CODE",
        .expectParseError = true // int scalar type declaration vs. int array value assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const $a := ( 0, 1, 2 )
end on
)NKSP_CODE",
        .expectParseError = true // int scalar type declaration vs. int array value assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare a
end on
)NKSP_CODE",
        .expectParseError = true // missing type prefix character in variable name
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare a := 24
end on
)NKSP_CODE",
        .expectParseError = true // missing type prefix character in variable name
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const a := 24
end on
)NKSP_CODE",
        .expectParseError = true // missing type prefix character in variable name
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic a
end on
)NKSP_CODE",
        .expectParseError = true // missing type prefix character in variable name
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := max(8,24)
  exit($a)
end on
)NKSP_CODE",
        .expectIntExitResult = 24
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := abort($NI_CALLBACK_ID)
end on
)NKSP_CODE",
        .expectParseError = true // assigned expression does not result in a value
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const $a := abort($NI_CALLBACK_ID)
end on
)NKSP_CODE",
        .expectParseError = true // assigned expression does not result in a value
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testIntArrayVarDeclaration() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: int array var declaration\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[3]
  exit( %a[0] + %a[1] + %a[2] )
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[0]
end on
)NKSP_CODE",
        .expectParseError = true // illegal array size
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[-1]
end on
)NKSP_CODE",
        .expectParseError = true // illegal array size
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[3] := ( 1, 2, 3 )
  exit( %a[0] + %a[1] + %a[2] )
end on
)NKSP_CODE",
        .expectIntExitResult = (1 + 2 + 3)
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const $sz := 3
  declare %a[$sz] := ( 1, 2, 3 )
  exit( %a[0] + %a[1] + %a[2] )
end on
)NKSP_CODE",
        .expectIntExitResult = (1 + 2 + 3)
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const $sz := 3
  declare const %a[$sz] := ( 1, 2, 3 )
  exit( %a[0] + %a[1] + %a[2] )
end on
)NKSP_CODE",
        .expectIntExitResult = (1 + 2 + 3)
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $sz := 3
  declare %a[$sz] := ( 1, 2, 3 )
end on
)NKSP_CODE",
        .expectParseError = true // array size must be constant expression
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $sz := 3
  declare const %a[$sz] := ( 1, 2, 3 )
end on
)NKSP_CODE",
        .expectParseError = true // array size must be constant expression
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ~sz := 3.0
  declare const %a[~sz] := ( 1, 2, 3 )
end on
)NKSP_CODE",
        .expectParseError = true // array size must be integer type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[3s] := ( 1, 2, 3 )
end on
)NKSP_CODE",
        .expectParseError = true // units not allowed for array size
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[3m] := ( 1, 2, 3 )
end on
)NKSP_CODE",
        .expectParseError = true // units not allowed for array size
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const %a[!3] := ( 1, 2, 3 )
  exit( %a[0] + %a[1] + %a[2] )
end on
)NKSP_CODE",
        .expectIntExitResult = (1 + 2 + 3),
        .expectParseWarning = true // 'final' operator is meaningless for array size
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[3] := ( 1, 2, 3 )
  %a[0] := 4
  %a[1] := 5
  %a[2] := 6
  exit( %a[0] + %a[1] + %a[2] )
end on
)NKSP_CODE",
        .expectIntExitResult = (4 + 5 + 6)
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[3]
  declare %a[3]
end on
)NKSP_CODE",
        .expectParseError = true // variable re-declaration
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const %a[3]
end on
)NKSP_CODE",
        .expectParseError = true // const variable declaration without assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const %a[3] := ( 1, 2, 3 )
  exit( %a[0] + %a[1] + %a[2] )
end on
)NKSP_CODE",
        .expectIntExitResult = (1 + 2 + 3)
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const %a[3] := ( 1, 2, 3, 4 )
end on
)NKSP_CODE",
        .expectParseError = true // incompatible array sizes
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const %a[3] := ( 1, 2, 3 )
  %a[0] := 8
end on
)NKSP_CODE",
        .expectParseError = true // attempt to modify const variable
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const %a[3] := ( 1, 2, 3 )
  declare const %b[3] := ( %a[0], %a[1], %a[2] )
  exit( %b[0] + %b[1] + %b[2] )
end on
)NKSP_CODE",
        .expectIntExitResult = (1 + 2 + 3)
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[3] := ( 1, 2, 3 )
  declare const %b[3] := ( %a[0], %a[1], %a[2] )
end on
)NKSP_CODE",
        .expectParseError = true // const array defined with non-const assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic %a[3]
end on
)NKSP_CODE",
        .expectParseError = true // polyphonic not allowed for array types
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic %a[3] := ( 1, 2, 3 )
end on
)NKSP_CODE",
        .expectParseError = true // polyphonic not allowed for array types
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const polyphonic %a[3]
end on
)NKSP_CODE",
        .expectParseError = true // polyphonic not allowed for array types
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const polyphonic %a[3] := ( 1, 2, 3 )
end on
)NKSP_CODE",
        .expectParseError = true // polyphonic not allowed for array types
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic const %a[3]
end on
)NKSP_CODE",
        .expectParseError = true // polyphonic not allowed for array types
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic const %a[3] := ( 1, 2, 3 )
end on
)NKSP_CODE",
        .expectParseError = true // polyphonic not allowed for array types
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[3] := ( 1, max(8,24), 3 )
  exit( %a[0] + %a[1] + %a[2] )
end on
)NKSP_CODE",
        .expectIntExitResult = ( 1 + 24 + 3 )
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[3] := ( 1, abort($NI_CALLBACK_ID), 3 )
end on
)NKSP_CODE",
        .expectParseError = true // assigned expression does not result in a value
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const %a[3] := ( 1, abort($NI_CALLBACK_ID), 3 )
end on
)NKSP_CODE",
        .expectParseError = true // assigned expression does not result in a value
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[3] := ( 1.0, 2.0, 3.0 )
end on
)NKSP_CODE",
        .expectParseError = true // int array declaration vs. real array assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[3] := ( 1, 2, 3.0 )
end on
)NKSP_CODE",
        .expectParseError = true // 3rd element not an integer
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a[3] := ( "x", "y", "z" )
end on
)NKSP_CODE",
        .expectParseError = true // int array declaration vs. string array assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare a[3] := ( 1, 2, 3 )
end on
)NKSP_CODE",
        .expectParseError = true // missing type prefix character in variable name
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare a[3]
end on
)NKSP_CODE",
        .expectParseError = true // missing type prefix character in variable name
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const %a[3] := ( 1, 2s, 3 )
end on
)NKSP_CODE",
        .expectParseError = true // unit types not allowed for arrays
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const %a[3] := ( 1, !2, 3 )
end on
)NKSP_CODE",
        .expectParseError = true // 'final' not allowed for arrays
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testRealVarDeclaration() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: real var declaration\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a
  exit(~a)
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := 24.8
  exit(~a)
end on
)NKSP_CODE",
        .expectRealExitResult = 24.8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := 8.24
  ~a := 24.8
  exit(~a)
end on
)NKSP_CODE",
        .expectRealExitResult = 24.8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a
  declare ~a
end on
)NKSP_CODE",
        .expectParseError = true // variable re-declaration
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ~a
end on
)NKSP_CODE",
        .expectParseError = true // const variable declaration without assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ~a := 8.24
  exit(~a)
end on
)NKSP_CODE",
        .expectRealExitResult = 8.24
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ~a := 28.0
  ~a := 8.0
end on
)NKSP_CODE",
        .expectParseError = true // attempt to modify const variable
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ~a := 24.8
  declare const ~b := ~a
  exit(~b)
end on
)NKSP_CODE",
        .expectRealExitResult = 24.8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := 24.0
  declare const ~b := ~a
end on
)NKSP_CODE",
        .expectParseError = true // const variable defined with non-const assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic ~a
  exit(~a)
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const polyphonic ~a
end on
)NKSP_CODE",
        .expectParseError = true // combination of qualifiers 'const' and 'polyphonic' is pointless
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic const ~a
end on
)NKSP_CODE",
        .expectParseError = true // combination of qualifiers 'const' and 'polyphonic' is pointless
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const polyphonic ~a := 3.0
end on
)NKSP_CODE",
        .expectParseError = true // combination of qualifiers 'const' and 'polyphonic' is pointless
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic const ~a := 3.0
end on
)NKSP_CODE",
        .expectParseError = true // combination of qualifiers 'const' and 'polyphonic' is pointless
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := 24.8
  exit($a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // int type declaration vs. real value assignment
        .expectRealExitResult = 24.8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a := 24.8
  exit(%a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // int array type declaration vs. real scalar value assignment
        .expectRealExitResult = 24.8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const %a := 24.8
  exit(%a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // int array type declaration vs. real scalar value assignment
        .expectRealExitResult = 24.8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a := 24.8
  exit(?a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // real array type declaration vs. real scalar value assignment
        .expectRealExitResult = 24.8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ?a := 24.8
  exit(?a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // real array type declaration vs. real scalar value assignment
        .expectRealExitResult = 24.8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare @a := 24.8
  exit(@a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // string type declaration vs. real scalar value assignment
        .expectRealExitResult = 24.8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const @a := 24.8
  exit(@a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // string type declaration vs. real scalar value assignment
        .expectRealExitResult = 24.8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := ( 0, 1, 2 )
end on
)NKSP_CODE",
        .expectParseError = true // real scalar type declaration vs. int array value assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ~a := ( 0, 1, 2 )
end on
)NKSP_CODE",
        .expectParseError = true // real scalar type declaration vs. int array value assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare a := 24.8
end on
)NKSP_CODE",
        .expectParseError = true // missing type prefix character in variable name
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const a := 24.8
end on
)NKSP_CODE",
        .expectParseError = true // missing type prefix character in variable name
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := max(8.1,24.2)
  exit(~a)
end on
)NKSP_CODE",
        .expectRealExitResult = 24.2
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := abort($NI_CALLBACK_ID)
end on
)NKSP_CODE",
        .expectParseError = true // assigned expression does not result in a value
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ~a := abort($NI_CALLBACK_ID)
end on
)NKSP_CODE",
        .expectParseError = true // assigned expression does not result in a value
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testRealArrayVarDeclaration() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: real array var declaration\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[3]
  exit( ?a[0] + ?a[1] + ?a[2] )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[0]
end on
)NKSP_CODE",
        .expectParseError = true // illegal array size
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[-1]
end on
)NKSP_CODE",
        .expectParseError = true // illegal array size
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[3] := ( 1.1, 2.2, 3.3 )
  exit( ?a[0] + ?a[1] + ?a[2] )
end on
)NKSP_CODE",
        .expectRealExitResult = (1.1 + 2.2 + 3.3)
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const $sz := 3
  declare ?a[$sz] := ( 1.1, 2.2, 3.3 )
  exit( ?a[0] + ?a[1] + ?a[2] )
end on
)NKSP_CODE",
        .expectRealExitResult = (1.1 + 2.2 + 3.3)
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const $sz := 3
  declare const ?a[$sz] := ( 1.1, 2.2, 3.3 )
  exit( ?a[0] + ?a[1] + ?a[2] )
end on
)NKSP_CODE",
        .expectRealExitResult = (1.1 + 2.2 + 3.3)
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $sz := 3
  declare ?a[$sz] := ( 1.1, 2.2, 3.3 )
end on
)NKSP_CODE",
        .expectParseError = true // array size must be constant expression
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $sz := 3
  declare const ?a[$sz] := ( 1.1, 2.2, 3.3 )
end on
)NKSP_CODE",
        .expectParseError = true // array size must be constant expression
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ~sz := 3.0
  declare const ?a[~sz] := ( 1.1, 2.2, 3.3 )
end on
)NKSP_CODE",
        .expectParseError = true // array size must be integer type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[3s] := ( 1.1, 2.2, 3.3 )
end on
)NKSP_CODE",
        .expectParseError = true // units not allowed for array size
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[3m] := ( 1.1, 2.2, 3.3 )
end on
)NKSP_CODE",
        .expectParseError = true // units not allowed for array size
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ?a[!3] := ( 1.1, 2.2, 3.3 )
  exit( ?a[0] + ?a[1] + ?a[2] )
end on
)NKSP_CODE",
        .expectRealExitResult = (1.1 + 2.2 + 3.3),
        .expectParseWarning = true // 'final' operator is meaningless for array size
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[3] := ( 1.0, 2.0, 3.0 )
  ?a[0] := 4.5
  ?a[1] := 5.5
  ?a[2] := 6.5
  exit( ?a[0] + ?a[1] + ?a[2] )
end on
)NKSP_CODE",
        .expectRealExitResult = (4.5 + 5.5 + 6.5)
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[3]
  declare ?a[3]
end on
)NKSP_CODE",
        .expectParseError = true // variable re-declaration
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ?a[3]
end on
)NKSP_CODE",
        .expectParseError = true // const variable declaration without assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ?a[3] := ( 1.1, 2.2, 3.3 )
  exit( ?a[0] + ?a[1] + ?a[2] )
end on
)NKSP_CODE",
        .expectRealExitResult = (1.1 + 2.2 + 3.3)
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ?a[3] := ( 1.1, 2.2, 3.3, 4.4 )
end on
)NKSP_CODE",
        .expectParseError = true // incompatible array sizes
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ?a[3] := ( 1.0, 2.0, 3.0 )
  ?a[0] := 8.0
end on
)NKSP_CODE",
        .expectParseError = true // attempt to modify const variable
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ?a[3] := ( 1.1, 2.2, 3.3 )
  declare const ?b[3] := ( ?a[0], ?a[1], ?a[2] )
  exit( ?b[0] + ?b[1] + ?b[2] )
end on
)NKSP_CODE",
        .expectRealExitResult = (1.1 + 2.2 + 3.3)
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[3] := ( 1.1, 2.2, 3.3 )
  declare const ?b[3] := ( ?a[0], ?a[1], ?a[2] )
end on
)NKSP_CODE",
        .expectParseError = true // const array defined with non-const assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic ?a[3]
end on
)NKSP_CODE",
        .expectParseError = true // polyphonic not allowed for array types
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic ?a[3] := ( 1.0, 2.0, 3.0 )
end on
)NKSP_CODE",
        .expectParseError = true // polyphonic not allowed for array types
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const polyphonic ?a[3]
end on
)NKSP_CODE",
        .expectParseError = true // polyphonic not allowed for array types
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const polyphonic ?a[3] := ( 1.0, 2.0, 3.0 )
end on
)NKSP_CODE",
        .expectParseError = true // polyphonic not allowed for array types
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic const ?a[3]
end on
)NKSP_CODE",
        .expectParseError = true // polyphonic not allowed for array types
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic const ?a[3] := ( 1.0, 2.0, 3.0 )
end on
)NKSP_CODE",
        .expectParseError = true // polyphonic not allowed for array types
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[3] := ( 1.0, max(8.3,24.6), 3.0 )
  exit( ?a[0] + ?a[1] + ?a[2] )
end on
)NKSP_CODE",
        .expectRealExitResult = ( 1.0 + 24.6 + 3.0 )
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[3] := ( 1.0, abort($NI_CALLBACK_ID), 3.0 )
end on
)NKSP_CODE",
        .expectParseError = true // assigned expression does not result in a value
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ?a[3] := ( 1.0, abort($NI_CALLBACK_ID), 3.0 )
end on
)NKSP_CODE",
        .expectParseError = true // assigned expression does not result in a value
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[3] := ( 1, 2, 3 )
end on
)NKSP_CODE",
        .expectParseError = true // real array declaration vs. int array assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[3] := ( 1.0, 2.0, 3 )
end on
)NKSP_CODE",
        .expectParseError = true // 3rd element not a real value
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?a[3] := ( "x", "y", "z" )
end on
)NKSP_CODE",
        .expectParseError = true // real array declaration vs. string array assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare a[3] := ( 1.0, 2.0, 3.0 )
end on
)NKSP_CODE",
        .expectParseError = true // missing type prefix character in variable name
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ?a[3] := ( 1.0, 2.0s, 3.0 )
end on
)NKSP_CODE",
        .expectParseError = true // unit types not allowed for arrays
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ?a[3] := ( 1.0, !2.0, 3.0 )
end on
)NKSP_CODE",
        .expectParseError = true // 'final' not allowed for arrays
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testStringVarDeclaration() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: string var declaration\n";
    #endif

runScript({
        .code = R"NKSP_CODE(
on init
  declare @a
  exit(@a)
end on
)NKSP_CODE",
        .expectStringExitResult = ""
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare @a := "foo"
  exit(@a)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare @a := "foo"
  @a := "bar"
  exit(@a)
end on
)NKSP_CODE",
        .expectStringExitResult = "bar"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare @a
  declare @a
end on
)NKSP_CODE",
        .expectParseError = true // variable re-declaration
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const @a
end on
)NKSP_CODE",
        .expectParseError = true // const variable declaration without assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const @a := "foo"
  exit(@a)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const @a := "foo"
  @a := "bar"
end on
)NKSP_CODE",
        .expectParseError = true // attempt to modify const variable
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const @a := "foo"
  declare const @b := @a
  exit(@b)
end on
)NKSP_CODE",
        .expectStringExitResult = "foo"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare @a := "foo"
  declare const @b := @a
end on
)NKSP_CODE",
        .expectParseError = true // const variable defined with non-const assignment
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic @a
end on
)NKSP_CODE",
        .expectParseError = true // 'polyphonic' not allowed for string type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const polyphonic @a
end on
)NKSP_CODE",
        .expectParseError = true // 'polyphonic' not allowed for string type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic const @a
end on
)NKSP_CODE",
        .expectParseError = true // 'polyphonic' not allowed for string type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic @a = "foo"
end on
)NKSP_CODE",
        .expectParseError = true // 'polyphonic' not allowed for string type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare polyphonic const @a = "foo"
end on
)NKSP_CODE",
        .expectParseError = true // 'polyphonic' not allowed for string type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const polyphonic @a = "foo"
end on
)NKSP_CODE",
        .expectParseError = true // 'polyphonic' not allowed for string type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $a := "foo"
  exit($a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // int type declaration vs. string assignment
        .expectStringExitResult = "foo"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~a := "foo"
  exit(~a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // real type declaration vs. string assignment
        .expectStringExitResult = "foo"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %a := "foo"
  exit(%a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // int array type declaration vs. string assignment
        .expectStringExitResult = "foo"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const $a := "foo"
  exit($a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // int type declaration vs. string assignment
        .expectStringExitResult = "foo"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const ~a := "foo"
  exit(~a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // real type declaration vs. string assignment
        .expectStringExitResult = "foo"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const %a := "foo"
  exit(%a)
end on
)NKSP_CODE",
        .expectParseWarning = true, // int array type declaration vs. string assignment
        .expectStringExitResult = "foo"
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare a := "foo"
end on
)NKSP_CODE",
        .expectParseError = true // missing type prefix character in variable name
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const a := "foo"
end on
)NKSP_CODE",
        .expectParseError = true // missing type prefix character in variable name
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare @a := abort($NI_CALLBACK_ID)
end on
)NKSP_CODE",
        .expectParseError = true // assigned expression does not result in a value
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare const @a := abort($NI_CALLBACK_ID)
end on
)NKSP_CODE",
        .expectParseError = true // assigned expression does not result in a value
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInMinFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in min() function\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min
end on
)NKSP_CODE",
        .expectParseError = true // because min() function requires 2 arguments
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min()
end on
)NKSP_CODE",
        .expectParseError = true // because min() function requires 2 arguments
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(1)
end on
)NKSP_CODE",
        .expectParseError = true // because min() function requires 2 arguments
    });

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(1,2)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(-30,4)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = -30
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := min(1.0, 2.0)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := min(-30.0, 4.0)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = -30.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := min(1.1, 1.13)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 1.1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := min(1.13, 1.1)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 1.1
    });

    // mixed type tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := min(1, 1.16)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectParseWarning = true // min() warns if data types of arguments not matching
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := min(-3.92, 9)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = -3.92,
        .expectParseWarning = true // min() warns if data types of arguments not matching
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(30ms,4s)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 30,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND,
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(4s,30ms)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 30,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND,
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(-30mdB,-4dB)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = -4,
        .expectExitResultUnitPrefix = { VM_DECI },
        .expectExitResultUnit = VM_BEL,
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(-4dB,-30mdB)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = -4,
        .expectExitResultUnitPrefix = { VM_DECI },
        .expectExitResultUnit = VM_BEL,
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(-4s,-30Hz)
  exit($foo)
end on
)NKSP_CODE",
        .expectParseError = true // min() requires arguments to have same unit type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(-4s,-30)
  exit($foo)
end on
)NKSP_CODE",
        .expectParseError = true // min() requires arguments to have same unit type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(-4,-30s)
  exit($foo)
end on
)NKSP_CODE",
        .expectParseError = true // min() requires arguments to have same unit type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := min(0.9s,1.0s)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 0.9,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_SECOND,
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(!30,!4)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 4,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(30,4)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 4,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(30,!4)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 4,
        .expectExitResultFinal = true,
        .expectParseWarning = true // min() warns if only one argument is 'final'
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := min(!30,4)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 4,
        .expectExitResultFinal = true,
        .expectParseWarning = true // min() warns if only one argument is 'final'
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := min(!12.1,!12.2)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 12.1,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := min(12.1,12.2)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 12.1,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := min(!12.1,12.2)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 12.1,
        .expectExitResultFinal = true,
        .expectParseWarning = true // min() warns if only one argument is 'final'
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := min(12.1,!12.2)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 12.1,
        .expectExitResultFinal = true,
        .expectParseWarning = true // min() warns if only one argument is 'final'
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInMaxFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in max() function\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max
end on
)NKSP_CODE",
        .expectParseError = true // because max() function requires 2 arguments
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max()
end on
)NKSP_CODE",
        .expectParseError = true // because max() function requires 2 arguments
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(1)
end on
)NKSP_CODE",
        .expectParseError = true // because max() function requires 2 arguments
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(1,2)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 2
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(-30,4)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 4
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := max(1.0, 2.0)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 2.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := max(-30.0, 4.0)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 4.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := max(1.1, 1.13)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 1.13
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := max(1.13, 1.1)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 1.13
    });

    // mixed type tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := max(1, 1.16)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 1.16,
        .expectParseWarning = true // max() warns if data types of arguments not matching
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := max(-3.92, 9)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 9.0,
        .expectParseWarning = true // max() warns if data types of arguments not matching
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(30ms,4s)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 4,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_SECOND,
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(4s,30ms)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 4,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_SECOND,
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(-30mdB,-4dB)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = -30,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL,
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(-4dB,-30mdB)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = -30,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL,
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(-4s,-30Hz)
  exit($foo)
end on
)NKSP_CODE",
        .expectParseError = true // max() requires arguments to have same unit type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(-4s,-30)
  exit($foo)
end on
)NKSP_CODE",
        .expectParseError = true // max() requires arguments to have same unit type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(-4,-30s)
  exit($foo)
end on
)NKSP_CODE",
        .expectParseError = true // max() requires arguments to have same unit type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := max(0.9s,1.0s)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_SECOND,
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(!30,!4)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 30,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(30,4)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 30,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(30,!4)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 30,
        .expectExitResultFinal = true,
        .expectParseWarning = true // max() warns if only one argument is 'final'
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := max(!30,4)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 30,
        .expectExitResultFinal = true,
        .expectParseWarning = true // max() warns if only one argument is 'final'
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := max(!12.1,!12.2)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 12.2,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := max(12.1,12.2)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 12.2,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := max(!12.1,12.2)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 12.2,
        .expectExitResultFinal = true,
        .expectParseWarning = true // max() warns if only one argument is 'final'
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := max(12.1,!12.2)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 12.2,
        .expectExitResultFinal = true,
        .expectParseWarning = true // max() warns if only one argument is 'final'
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInAbsFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in abs() function\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := abs
end on
)NKSP_CODE",
        .expectParseError = true // because abs() function requires 1 argument
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := abs()
end on
)NKSP_CODE",
        .expectParseError = true // because abs() function requires 1 argument
    });

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := abs(23)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 23
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := abs(-23)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 23
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := abs(23.0)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 23.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := abs(23.11)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 23.11
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := abs(-23.11)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 23.11
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~bar := -23.11
  declare ~foo := abs(~bar)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 23.11
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := abs(-23kHz)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 23,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := abs(-23.4kHz)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 23.4,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := abs(!-23)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 23,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := abs(-23)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 23,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := abs(!-23.2)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 23.2,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := abs(-23.9)
  exit(~foo)
end on
)NKSP_CODE",
        .expectRealExitResult = 23.9,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInIncFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in inc() function\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 5
  inc($foo)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 6
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 5
  inc($foo)
  inc($foo)
  inc($foo)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 5
  inc($foo)
  exit( inc($foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 7
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 53mdB
  inc($foo)
  exit( inc($foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 55,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL,
        .expectParseWarning = true // inc() warns if argument has a unit
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := !53
  inc($foo)
  exit( inc($foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 55,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 53
  inc($foo)
  exit( inc($foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 55,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 53
  inc($foo)
  exit( !inc($foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 55,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInDecFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in dec() function\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 5
  dec($foo)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 4
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 5
  dec($foo)
  dec($foo)
  dec($foo)
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 2
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 5
  dec($foo)
  exit( dec($foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 3
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 53mdB
  dec($foo)
  exit( dec($foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 51,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL,
        .expectParseWarning = true // dec() warns if argument has a unit
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := !53
  dec($foo)
  exit( dec($foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 51,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 53
  dec($foo)
  exit( dec($foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 51,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 53
  dec($foo)
  exit( !dec($foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 51,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInInRangeFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in in_range() function\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(1,4,9) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(5,4,9) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(9,4,9) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(10,4,9) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(-6,-5,5) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(-5,-5,5) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(0,-5,5) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(5,-5,5) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(6,-5,5) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(12.2,12.1,12.9) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(12.2,12.9,12.1) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(12.0,12.1,12.9) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(12.0,12.9,12.1) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(0.0,-0.3,0.3) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(-0.34,-0.3,0.3) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(0.34,-0.3,0.3) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(-0.3,-0.3,0.3) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(0.3,-0.3,0.3) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // mixed type tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(4.0,-5,5) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectParseWarning = true // in_range() warns if not all arguments are of same type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(5,-5,5.0) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectParseWarning = true // in_range() warns if not all arguments are of same type
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(-5,-5.0,5) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectParseWarning = true // in_range() warns if not all arguments are of same type
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(4000Hz,3kHz,5kHz) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(5000Hz,3kHz,5kHz) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(5001Hz,3kHz,5kHz) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(3000Hz,3kHz,5kHz) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(2999Hz,3kHz,5kHz) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(0.003s,3000.0us,5ms) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectParseWarning = true, // in_range() warns if not all arguments are of same type
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(0.005s,3000.0us,5ms) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectParseWarning = true, // in_range() warns if not all arguments are of same type,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(0.0051s,3000.0us,5ms) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false,
        .expectParseWarning = true, // in_range() warns if not all arguments are of same type
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(3s,2Hz,5Hz) )
end on
)NKSP_CODE",
        .expectParseError = true // in_range() throws error if not all arguments' unit types equal,
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(3Hz,2s,5Hz) )
end on
)NKSP_CODE",
        .expectParseError = true // in_range() throws error if not all arguments' unit types equal
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(3Hz,2Hz,5s) )
end on
)NKSP_CODE",
        .expectParseError = true // in_range() throws error if not all arguments' unit types equal
    });

    // 'final' ('!') operator tests ...
    // (result should always be NOT final)

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( in_range(!9,!4,!9) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInRandomFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in random() function\n";
    #endif

    // integer tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(-5,5) )
end on
)NKSP_CODE",
        .expectExitResultIsInt = true // only check type, exact value is irrelevant here
    });

    for (int run = 0; run < 20; ++run) {
        runScript({
            .code = R"NKSP_CODE(
on init
  declare $foo := random(-5,5)
  exit( in_range($foo,-5,5) )
end on
)NKSP_CODE",
            .expectBoolExitResult = true
        });
    }

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(-0.5,0.5) )
end on
)NKSP_CODE",
        .expectExitResultIsReal = true // only check type, exact value is irrelevant here
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := random(-5.0,5.0)
  exit(~foo)
end on
)NKSP_CODE",
        .expectExitResultIsReal = true // only check type, exact value is irrelevant here
    });

    for (int run = 0; run < 20; ++run) {
        runScript({
            .code = R"NKSP_CODE(
on init
  declare ~foo := random(-0.5,0.5)
  exit( in_range(~foo,-0.5,0.5) )
end on
)NKSP_CODE",
            .expectBoolExitResult = true
        });
    }

    for (int run = 0; run < 20; ++run) {
        runScript({
            .code = R"NKSP_CODE(
on init
  declare ~foo := random(-5.0,12.0)
  exit( in_range(~foo,-5.0,12.0) )
end on
)NKSP_CODE",
            .expectBoolExitResult = true
        });
    }

    for (int run = 0; run < 20; ++run) {
        runScript({
            .code = R"NKSP_CODE(
on init
  declare ~foo := random(23.3,98.4)
  exit( in_range(~foo,23.3,98.4) )
end on
)NKSP_CODE",
            .expectBoolExitResult = true
        });
    }

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(-5Hz,5Hz) )
end on
)NKSP_CODE",
        .expectExitResultIsInt = true, // only check type, exact value is irrelevant here
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_HERTZ
    });

    for (int run = 0; run < 20; ++run) {
        runScript({
            .code = R"NKSP_CODE(
on init
  declare $foo := random(-5Hz,5Hz)
  exit( in_range($foo,-5Hz,5Hz) )
end on
)NKSP_CODE",
            .expectBoolExitResult = true
        });
    }

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(5us,1ms) )
end on
)NKSP_CODE",
        .expectExitResultIsInt = true, // only check type, exact value is irrelevant here
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    for (int run = 0; run < 20; ++run) {
        runScript({
            .code = R"NKSP_CODE(
on init
  declare $foo := random(5us,1ms)
  exit( in_range($foo,5us,1ms) )
end on
)NKSP_CODE",
            .expectBoolExitResult = true
        });
    }

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(1ms,5000us) )
end on
)NKSP_CODE",
        .expectExitResultIsInt = true, // only check type, exact value is irrelevant here
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    for (int run = 0; run < 20; ++run) {
        runScript({
            .code = R"NKSP_CODE(
on init
  declare $foo := random(1ms,5000us)
  exit( in_range($foo,1ms,5000us) )
end on
)NKSP_CODE",
            .expectBoolExitResult = true
        });
    }

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(1kHz,20kHz) )
end on
)NKSP_CODE",
        .expectExitResultIsInt = true, // only check type, exact value is irrelevant here
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    for (int run = 0; run < 20; ++run) {
        runScript({
            .code = R"NKSP_CODE(
on init
  declare $foo := random(1kHz,20kHz)
  exit( in_range($foo,1kHz,20kHz) )
end on
)NKSP_CODE",
            .expectBoolExitResult = true
        });
    }

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(1.2us,3.5us) )
end on
)NKSP_CODE",
        .expectExitResultIsReal = true, // only check type, exact value is irrelevant here
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    for (int run = 0; run < 20; ++run) {
        runScript({
            .code = R"NKSP_CODE(
on init
  declare ~foo := random(1.2us,3.5us)
  exit( in_range(~foo,1.2us,3.5us) )
end on
)NKSP_CODE",
            .expectBoolExitResult = true
        });
    }

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(5.2us,1.1ms) )
end on
)NKSP_CODE",
        .expectExitResultIsReal = true, // only check type, exact value is irrelevant here
        .expectExitResultUnitPrefix = { VM_MICRO },
        .expectExitResultUnit = VM_SECOND
    });

    for (int run = 0; run < 20; ++run) {
        runScript({
            .code = R"NKSP_CODE(
on init
  declare ~foo := random(5.2us,1.1ms)
  exit( in_range(~foo,5.2us,1.1ms) )
end on
)NKSP_CODE",
            .expectBoolExitResult = true
        });
    }

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(1Hz,12s) )
end on
)NKSP_CODE",
        .expectParseError = true // random() throws error if arguments' unit types don't match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(1,12s) )
end on
)NKSP_CODE",
        .expectParseError = true // random() throws error if arguments' unit types don't match
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(1s,12) )
end on
)NKSP_CODE",
        .expectParseError = true // random() throws error if arguments' unit types don't match
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(!1,!12) )
end on
)NKSP_CODE",
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(1,12) )
end on
)NKSP_CODE",
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(!1,12) )
end on
)NKSP_CODE",
        .expectExitResultFinal = true,
        .expectParseWarning = true // random() warns if only one argument is 'final'
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( random(1,!12) )
end on
)NKSP_CODE",
        .expectExitResultFinal = true,
        .expectParseWarning = true // random() warns if only one argument is 'final'
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInShiftLeftFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in sh_left() function\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sh_left(1,0) )
end on
)NKSP_CODE",
        .expectIntExitResult = 1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sh_left(1,1) )
end on
)NKSP_CODE",
        .expectIntExitResult = 2
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sh_left(1,2) )
end on
)NKSP_CODE",
        .expectIntExitResult = 4
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sh_left(1,3) )
end on
)NKSP_CODE",
        .expectIntExitResult = 8
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInShiftRightFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in sh_right() function\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sh_right(8,0) )
end on
)NKSP_CODE",
        .expectIntExitResult = 8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sh_right(8,1) )
end on
)NKSP_CODE",
        .expectIntExitResult = 4
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sh_right(8,2) )
end on
)NKSP_CODE",
        .expectIntExitResult = 2
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sh_right(8,3) )
end on
)NKSP_CODE",
        .expectIntExitResult = 1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sh_right(8,4) )
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInMsbFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in msb() function\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( msb(0) )
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( msb(127) )
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( msb(128) )
end on
)NKSP_CODE",
        .expectIntExitResult = 1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( msb(16255) )
end on
)NKSP_CODE",
        .expectIntExitResult = 126
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( msb(16256) )
end on
)NKSP_CODE",
        .expectIntExitResult = 127
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( msb(16383) )
end on
)NKSP_CODE",
        .expectIntExitResult = 127
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInLsbFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in lsb() function\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( lsb(0) )
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( lsb(1) )
end on
)NKSP_CODE",
        .expectIntExitResult = 1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( lsb(126) )
end on
)NKSP_CODE",
        .expectIntExitResult = 126
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( lsb(127) )
end on
)NKSP_CODE",
        .expectIntExitResult = 127
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( lsb(128) )
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( lsb(16255) )
end on
)NKSP_CODE",
        .expectIntExitResult = 127
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( lsb(16256) )
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInIntToRealFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in int_to_real() function\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( int_to_real(8) )
end on
)NKSP_CODE",
        .expectRealExitResult = 8.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 23
  exit( int_to_real($foo) )
end on
)NKSP_CODE",
        .expectRealExitResult = 23.0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( int_to_real(-58mdB) )
end on
)NKSP_CODE",
        .expectRealExitResult = -58.0,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := -58mdB
  exit( int_to_real($foo) )
end on
)NKSP_CODE",
        .expectRealExitResult = -58.0,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := !-58
  exit( int_to_real($foo) )
end on
)NKSP_CODE",
        .expectRealExitResult = -58.0,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := -58
  exit( int_to_real($foo) )
end on
)NKSP_CODE",
        .expectRealExitResult = -58.0,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInRealFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in real() function\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( real(8) )
end on
)NKSP_CODE",
        .expectRealExitResult = 8.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 23
  exit( real($foo) )
end on
)NKSP_CODE",
        .expectRealExitResult = 23.0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( real(-58mdB) )
end on
)NKSP_CODE",
        .expectRealExitResult = -58.0,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := -58mdB
  exit( real($foo) )
end on
)NKSP_CODE",
        .expectRealExitResult = -58.0,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := !-58
  exit( real($foo) )
end on
)NKSP_CODE",
        .expectRealExitResult = -58.0,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := -58
  exit( real($foo) )
end on
)NKSP_CODE",
        .expectRealExitResult = -58.0,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInRealToIntFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in real_to_int() function\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( real_to_int(8.9) )
end on
)NKSP_CODE",
        .expectIntExitResult = 8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := 8.9
  exit( real_to_int(~foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 8
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := 8.9mdB
  exit( real_to_int(~foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 8,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := !8.9
  exit( real_to_int(~foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 8,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := 8.9
  exit( real_to_int(~foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 8,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInIntFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in int() function\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( int(8.9) )
end on
)NKSP_CODE",
        .expectIntExitResult = 8
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := 8.9
  exit( int(~foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 8
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := 8.9mdB
  exit( int(~foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 8,
        .expectExitResultUnitPrefix = { VM_MILLI, VM_DECI },
        .expectExitResultUnit = VM_BEL
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := !8.9
  exit( int(~foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 8,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ~foo := 8.9
  exit( int(~foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 8,
        .expectExitResultFinal = false
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInArrayEqualFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in array_equal() function\n";
    #endif

    // integer array tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 1, 2, 3 )
  declare %bar[3] := ( 1, 2, 3 )
  exit( array_equal(%foo, %bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[1] := ( 1 )
  declare %bar[1] := ( 1 )
  exit( array_equal(%foo, %bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 1, 2, 3 )
  declare %bar[3] := ( 0, 2, 3 )
  exit( array_equal(%foo, %bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 1, 2, 3 )
  declare %bar[3] := ( 3, 2, 1 )
  exit( array_equal(%foo, %bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 1, 2, 3 )
  declare %bar[2] := ( 1, 2 )
  exit( array_equal(%foo, %bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false,
        .expectParseWarning = true // array_equal() warns if array sizes do not match
    });

    // real number array tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 1.0, 2.0, 3.0 )
  declare ?bar[3] := ( 1.0, 2.0, 3.0 )
  exit( array_equal(?foo, ?bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 1.0, 1.1, 3.4 )
  declare ?bar[3] := ( 1.0, 1.1, 3.4 )
  exit( array_equal(?foo, ?bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 1.0, 1.1, 3.4 )
  declare ?bar[3] := ( 1.0, 1.2, 3.4 )
  exit( array_equal(?foo, ?bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 1.0, 1.1, 3.4 )
  declare ?bar[2] := ( 1.0, 1.1 )
  exit( array_equal(?foo, ?bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false,
        .expectParseWarning = true // array_equal() warns if array sizes do not match
    });

    // std unit tests ...
    // (only metric prefixes allowed, unit types are prohibited for arrays ATM)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 1, 1s, 1 )
  declare %bar[3] := ( 1, 1,  1 )
  exit( array_equal(%foo, %bar) )
end on
)NKSP_CODE",
        .expectParseError = true // see comment above
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 1k, 1, 1m )
  declare %bar[3] := ( 1k, 1, 1m )
  exit( array_equal(%foo, %bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 1m, 1, 1k )
  declare %bar[3] := ( 1k, 1, 1m )
  exit( array_equal(%foo, %bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 1, 1k, 1 )
  declare %bar[3] := ( 1, 1,  1 )
  exit( array_equal(%foo, %bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 1, 1k, 1 )
  declare %bar[3] := ( 1, 1000, 1 )
  exit( array_equal(%foo, %bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 1, 2, 3000 )
  declare %bar[3] := ( 1, 2, 3k )
  exit( array_equal(%foo, %bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 1, 2, 3m )
  declare %bar[3] := ( 1, 2, 3k )
  exit( array_equal(%foo, %bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[4] := ( 1.0, 1.0m, 1.1m, 3.4k )
  declare ?bar[4] := ( 1.0, 1.0m, 1.1m, 3.4k )
  exit( array_equal(?foo, ?bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[4] := ( 1.0, 1.0m, 1.1m, 3.4k )
  declare ?bar[4] := ( 1.0, 1.0 , 1.1m, 3.4k )
  exit( array_equal(?foo, ?bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[4] := ( 1.0, 1.0m, 1.1m, 3.4k )
  declare ?bar[4] := ( 1.0, 1.0m, 1.1k, 3.4k )
  exit( array_equal(?foo, ?bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[4] := ( 1.0, 1.0m, 1.1m, 3.4k )
  declare ?bar[4] := ( 1.0, 1.0m, 1.1m, 3.4u )
  exit( array_equal(?foo, ?bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 1.0, 1.0k, 1.1m )
  declare ?bar[3] := ( 1.0, 1000.0, 1.1m )
  exit( array_equal(?foo, ?bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 1.0, 1.0k, 1.1u )
  declare ?bar[3] := ( 1.0, 1000.0, 0.0011m )
  exit( array_equal(?foo, ?bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 1.0, 1.0k, 1.1u )
  declare ?bar[3] := ( 1.0, 1000.0, 0.0016m )
  exit( array_equal(?foo, ?bar) )
end on
)NKSP_CODE",
        .expectBoolExitResult = false
    });

    // 'final' ('!') operator tests ...
    // (currently prohibited for arrays)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( !1, !1, !1 )
  declare %bar[3] := ( !1, !1, !1 )
  exit( array_equal(%foo, %bar) )
end on
)NKSP_CODE",
        .expectParseError = true // see comment above
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( !1.0, !1.0, !1.0 )
  declare ?bar[3] := ( !1.0, !1.0, !1.0 )
  exit( array_equal(?foo, ?bar) )
end on
)NKSP_CODE",
        .expectParseError = true // see comment above
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInSortFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in sort() function\n";
    #endif

    // integer array tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %input[3] := ( 19, 3, 6 )
  declare %expected[3] := ( 3, 6, 19 )
  sort(%input, 0)
  exit( array_equal(%input, %expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %input[3] := ( 19, 3, 6 )
  declare %expected[3] := ( 19, 6, 3 )
  sort(%input, 1)
  exit( array_equal(%input, %expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %input[6] := ( 1, 80, 120, 40, 3000, 20 )
  declare %expected[6] := ( 1, 20, 40, 80, 120, 3000  )
  sort(%input, 0)
  exit( array_equal(%input, %expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // real number array tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?input[4] := ( 1.4, 1.0, 13.4, 2.7 )
  declare ?expected[4] := ( 1.0, 1.4, 2.7, 13.4 )
  sort(?input, 0)
  exit( array_equal(?input, ?expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?input[4] := ( 1.4, 1.0, 13.4, 2.7 )
  declare ?expected[4] := ( 13.4, 2.7, 1.4, 1.0 )
  sort(?input, 1)
  exit( array_equal(?input, ?expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    // std unit tests ...
    // (only metric prefixes are allowed for arrays ATM)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %input[3] := ( 1k, 6, 900 )
  declare %expected[3] := ( 6, 900, 1k )
  sort(%input, 0)
  exit( array_equal(%input, %expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %input[3] := ( 900, 1k, 6 )
  declare %expected[3] := ( 1k, 900, 6 )
  sort(%input, 1)
  exit( array_equal(%input, %expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %input[6] := ( 1k, 8m, 4k, 12000u, 3000u, 1000u )
  declare %expected[6] := ( 1000u, 3000u, 8m, 12000u, 1k, 4k )
  sort(%input, 0)
  exit( array_equal(%input, %expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?input[3] := ( 1.0k, 6.0, 900.0 )
  declare ?expected[3] := ( 6.0, 900.0, 1.0k )
  sort(?input, 0)
  exit( array_equal(?input, ?expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?input[3] := ( 900.0, 1.0k, 6.0 )
  declare ?expected[3] := ( 1.0k, 900.0, 6.0 )
  sort(?input, 1)
  exit( array_equal(?input, ?expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?input[3] := ( 1.0k, 0.9k, 1.1k )
  declare ?expected[3] := ( 0.9k, 1.0k, 1.1k )
  sort(?input, 0)
  exit( array_equal(?input, ?expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?input[3] := ( 2.0m, 1000.1u, 1.0 )
  declare ?expected[3] := ( 1000.1u, 2.0m, 1.0 )
  sort(?input, 0)
  exit( array_equal(?input, ?expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?input[4] := ( 2.0m, 1000.1u, 1.0, 1400.0u )
  declare ?expected[4] := ( 1000.1u, 1400.0u, 2.0m, 1.0 )
  sort(?input, 0)
  exit( array_equal(?input, ?expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?input[4] := ( 2.0m, 1000.1u, 1.0, 1400.0u )
  declare ?expected[4] := ( 1.0, 2.0m, 1400.0u, 1000.1u )
  sort(?input, 1)
  exit( array_equal(?input, ?expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?input[6] := ( 0.9k, 8.0m, 1.1k, 12000.0u, 3000.0u, 1000.0u )
  declare ?expected[6] := ( 1000.0u, 3000.0u, 8.0m, 12000.0u, 0.9k, 1.1k )
  sort(?input, 0)
  exit( array_equal(?input, ?expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?input[6] := ( 0.9k, 8.0m, 1.1k, 12000.0u, 3000.0u, 1000.0u )
  declare ?expected[6] := ( 1.1k, 0.9k, 12000.0u, 8.0m, 3000.0u, 1000.0u )
  sort(?input, 1)
  exit( array_equal(?input, ?expected) )
end on
)NKSP_CODE",
        .expectBoolExitResult = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInRoundFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in round() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( round($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( round(99.4) )
end on
)NKSP_CODE",
        .expectRealExitResult = 99.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( round(99.5) )
end on
)NKSP_CODE",
        .expectRealExitResult = 100.0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( round(2.4ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 2.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( round(2.6kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 3.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( round(123.8) )
end on
)NKSP_CODE",
        .expectRealExitResult = 124.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( round(!123.8) )
end on
)NKSP_CODE",
        .expectRealExitResult = 124.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInCeilFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in ceil() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( ceil($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( ceil(99.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 99.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( ceil(99.1) )
end on
)NKSP_CODE",
        .expectRealExitResult = 100.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( ceil(99.9) )
end on
)NKSP_CODE",
        .expectRealExitResult = 100.0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( ceil(2.4ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 3.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( ceil(2.6kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 3.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( ceil(123.1) )
end on
)NKSP_CODE",
        .expectRealExitResult = 124.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( ceil(!123.8) )
end on
)NKSP_CODE",
        .expectRealExitResult = 124.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInFloorFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in floor() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( floor($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( floor(99.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 99.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( floor(99.1) )
end on
)NKSP_CODE",
        .expectRealExitResult = 99.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( floor(99.9) )
end on
)NKSP_CODE",
        .expectRealExitResult = 99.0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( floor(2.4ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 2.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( floor(2.6kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 2.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( floor(123.1) )
end on
)NKSP_CODE",
        .expectRealExitResult = 123.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( floor(!123.8) )
end on
)NKSP_CODE",
        .expectRealExitResult = 123.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInSqrtFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in sqrt() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( sqrt($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sqrt(36.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 6.0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sqrt(100.0ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 10.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sqrt(5.76kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 2.4,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sqrt(25.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 5.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sqrt(!25.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 5.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInLogFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in log() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( log($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log(1.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log(~NI_MATH_E) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log(~NI_MATH_E * 1.0ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log(~NI_MATH_E * 1.0kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log(~NI_MATH_E * 1.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log(!(~NI_MATH_E * 1.0)) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInLog2Function() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in log2() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( log2($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log2(1.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log2(32.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 5.0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log2(32.0ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 5.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log2(32.0kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 5.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log2(32.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 5.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log2(!32.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 5.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInLog10Function() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in log10() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( log10($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log10(1000.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 3.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log10(1000.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 3.0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log10(1000.0ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 3.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log10(1000.0kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 3.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log10(1000.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 3.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( log10(!1000.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 3.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInExpFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in exp() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( exp($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( exp(0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( exp(1.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = M_E
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( exp(0.0ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( exp(0.0kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( exp(0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( exp(!0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInPowFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in pow() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( pow($foo,$foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( pow(1.0) )
end on
)NKSP_CODE",
        .expectParseError = true // because pow() requires exactly 2 arguments
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( pow(3.0,4.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 81.0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( pow(3.0ms,4.0ms) )
end on
)NKSP_CODE",
        .expectParseError = true // because units are prohibited for 2nd argument
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( pow(3.0,4.0ms) )
end on
)NKSP_CODE",
        .expectParseError = true // because units are prohibited for 2nd argument
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( pow(3.0ms,4.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 81.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( pow(3.0kHz,4.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 81.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( pow(3.0,4.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 81.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( pow(!3.0,4.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 81.0,
        .expectExitResultFinal = true
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( pow(3.0,!4.0) )
end on
)NKSP_CODE",
        .expectParseError = true // because 'final' is meaningless for 2nd argument
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInSinFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in sin() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( sin($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sin(0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sin(0.5 * ~NI_MATH_PI) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sin(~NI_MATH_PI) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sin(1.5 * ~NI_MATH_PI) )
end on
)NKSP_CODE",
        .expectRealExitResult = -1.0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sin(0.0ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sin(0.0kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sin(0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( sin(!0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInCosFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in cos() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( cos($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( cos(0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( cos(0.5 * ~NI_MATH_PI) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( cos(~NI_MATH_PI) )
end on
)NKSP_CODE",
        .expectRealExitResult = -1.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( cos(1.5 * ~NI_MATH_PI) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( cos(0.0ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( cos(0.0kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( cos(0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( cos(!0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInTanFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in tan() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( tan($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( tan(0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( tan(0.25 * ~NI_MATH_PI) )
end on
)NKSP_CODE",
        .expectRealExitResult = 1.0
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( tan(0.0ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( tan(0.0kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( tan(0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( tan(!0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInAsinFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in asin() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( asin($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( asin(0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( asin(1.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.5 * M_PI
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( asin(-1.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = -0.5 * M_PI
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( asin(0.0ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( asin(0.0kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( asin(0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( asin(!0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInAcosFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in acos() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( acos($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( acos(1.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( acos(0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.5 * M_PI
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( acos(-1.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = M_PI
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( acos(1.0ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( acos(1.0kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( acos(1.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( acos(!1.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInAtanFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in atan() function\n";
    #endif

    // integer tests ...
    // (ATM not allowed for this function)

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  exit( atan($foo) )
end on
)NKSP_CODE",
        .expectParseError = true // integer not allowed for this function ATM
    });

    // real number tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( atan(0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( atan(1.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.25 * M_PI
    });

    // std unit tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( atan(0.0ms) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultUnitPrefix = { VM_MILLI },
        .expectExitResultUnit = VM_SECOND
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( atan(0.0kHz) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultUnitPrefix = { VM_KILO },
        .expectExitResultUnit = VM_HERTZ
    });

    // 'final' ('!') operator tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( atan(0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultFinal = false
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit( atan(!0.0) )
end on
)NKSP_CODE",
        .expectRealExitResult = 0.0,
        .expectExitResultFinal = true
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInNumElementsFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in num_elements() function\n";
    #endif

    // integer array tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 19, 3, 6 )
  exit( num_elements(%foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 3
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[1] := ( 19 )
  exit( num_elements(%foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[5] := ( 1, 2, 3, 4, 5 )
  exit( num_elements(%foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 5
    });

    // real array tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 19.0, 3.2, 6.5 )
  exit( num_elements(?foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 3
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[1] := ( 19.0 )
  exit( num_elements(?foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[5] := ( 1.1, 2.2, 3.3, 4.4, 5.5 )
  exit( num_elements(?foo) )
end on
)NKSP_CODE",
        .expectIntExitResult = 5
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInSearchFunction() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in search() function\n";
    #endif

    // integer array tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 19, 3, 6 )
  exit( search(%foo, 19) )
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 19, 3, 6 )
  exit( search(%foo, 3) )
end on
)NKSP_CODE",
        .expectIntExitResult = 1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 19, 3, 6 )
  exit( search(%foo, 6) )
end on
)NKSP_CODE",
        .expectIntExitResult = 2
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare %foo[3] := ( 19, 3, 6 )
  exit( search(%foo, 2) )
end on
)NKSP_CODE",
        .expectIntExitResult = -1
    });

    // real array tests ...

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 19.12, 3.45, 6.89 )
  exit( search(?foo, 19.12) )
end on
)NKSP_CODE",
        .expectIntExitResult = 0
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 19.12, 3.45, 6.89 )
  exit( search(?foo, 3.45) )
end on
)NKSP_CODE",
        .expectIntExitResult = 1
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 19.12, 3.45, 6.89 )
  exit( search(?foo, 6.89) )
end on
)NKSP_CODE",
        .expectIntExitResult = 2
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare ?foo[3] := ( 19.12, 3.45, 6.89 )
  exit( search(?foo, 6.99) )
end on
)NKSP_CODE",
        .expectIntExitResult = -1
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testIfStatement() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: if statement\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  if ($foo)
    exit(42)
  end if
end on
)NKSP_CODE",
        .expectIntExitResult = 42
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 0
  if ($foo)
    exit(42)
  end if
  exit(3)
end on
)NKSP_CODE",
        .expectIntExitResult = 3
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 1
  if ($foo)
    exit(42)
  else
    exit(3)
  end if
end on
)NKSP_CODE",
        .expectIntExitResult = 42
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 0
  if ($foo)
    exit(42)
  else
    exit(3)
  end if
end on
)NKSP_CODE",
        .expectIntExitResult = 3
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testWhileStatement() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: while statement\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  declare $foo := 100
  declare $i := 50
  while ($i)
      $foo := $foo + 1
      $i := $i - 1
  end while
  exit($foo)
end on
)NKSP_CODE",
        .expectIntExitResult = 150
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

static void testBuiltInVars() {
    #if !SILENT_TEST
    std::cout << "UNIT TEST: built-in variables\n";
    #endif

    runScript({
        .code = R"NKSP_CODE(
on init
  exit($NKSP_PERF_TIMER)
end on
)NKSP_CODE",
        .expectExitResultIsInt = true,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit($NKSP_REAL_TIMER)
end on
)NKSP_CODE",
        .expectExitResultIsInt = true,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit($KSP_TIMER)
end on
)NKSP_CODE",
        .expectExitResultIsInt = true,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(~NI_MATH_PI)
end on
)NKSP_CODE",
        .expectExitResultIsReal = true,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit(~NI_MATH_E)
end on
)NKSP_CODE",
        .expectExitResultIsReal = true,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit($NI_CB_TYPE_INIT)
end on
)NKSP_CODE",
        .expectIntExitResult = VM_EVENT_HANDLER_INIT,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit($NI_CB_TYPE_NOTE)
end on
)NKSP_CODE",
        .expectIntExitResult = VM_EVENT_HANDLER_NOTE,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit($NI_CB_TYPE_RELEASE)
end on
)NKSP_CODE",
        .expectIntExitResult = VM_EVENT_HANDLER_RELEASE,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit($NI_CB_TYPE_CONTROLLER)
end on
)NKSP_CODE",
        .expectIntExitResult = VM_EVENT_HANDLER_CONTROLLER,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit($NI_CB_TYPE_RPN)
end on
)NKSP_CODE",
        .expectIntExitResult = VM_EVENT_HANDLER_RPN,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    runScript({
        .code = R"NKSP_CODE(
on init
  exit($NI_CB_TYPE_NRPN)
end on
)NKSP_CODE",
        .expectIntExitResult = VM_EVENT_HANDLER_NRPN,
        .expectExitResultUnitPrefix = { VM_NO_PREFIX },
        .expectExitResultUnit = VM_NO_UNIT
    });

    #if !SILENT_TEST
    std::cout << std::endl;
    #endif
}

#if !NO_MAIN

int main() {
    testBuiltInExitFunction();
    testStringConcatOperator();
    testNegOperator();
    testPlusOperator();
    testMinusOperator();
    testModuloOperator();
    testMultiplyOperator();
    testDivideOperator();
    testSmallerThanOperator();
    testGreaterThanOperator();
    testSmallerOrEqualOperator();
    testGreaterOrEqualOperator();
    testEqualOperator();
    testUnequalOperator();
    testLogicalAndOperator();
    testLogicalOrOperator();
    testLogicalNotOperator();
    testBitwiseAndOperator();
    testBitwiseOrOperator();
    testBitwiseNotOperator();
    testPrecedenceOfOperators();
    testIntVarDeclaration();
    testIntArrayVarDeclaration();
    testRealVarDeclaration();
    testRealArrayVarDeclaration();
    testStringVarDeclaration();
    testBuiltInMinFunction();
    testBuiltInMaxFunction();
    testBuiltInAbsFunction();
    testBuiltInIncFunction();
    testBuiltInDecFunction();
    testBuiltInInRangeFunction();
    testBuiltInRandomFunction();
    testBuiltInShiftLeftFunction();
    testBuiltInShiftRightFunction();
    testBuiltInMsbFunction();
    testBuiltInLsbFunction();
    testBuiltInIntToRealFunction();
    testBuiltInRealFunction();
    testBuiltInRealToIntFunction();
    testBuiltInIntFunction();
    testBuiltInRoundFunction();
    testBuiltInCeilFunction();
    testBuiltInFloorFunction();
    testBuiltInSqrtFunction();
    testBuiltInLogFunction();
    testBuiltInLog2Function();
    testBuiltInLog10Function();
    testBuiltInExpFunction();
    testBuiltInPowFunction();
    testBuiltInSinFunction();
    testBuiltInCosFunction();
    testBuiltInTanFunction();
    testBuiltInAsinFunction();
    testBuiltInAcosFunction();
    testBuiltInAtanFunction();
    testBuiltInArrayEqualFunction();
    testBuiltInSortFunction();
    testBuiltInNumElementsFunction();
    testBuiltInSearchFunction();
    testIfStatement();
    testWhileStatement();
    testBuiltInVars();
    std::cout << "\nAll tests passed successfully. :-)\n";
    return 0;
}

#endif // !NO_MAIN
