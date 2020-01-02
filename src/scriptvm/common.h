/*
 * Copyright (c) 2014-2019 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

// This header defines data types shared between the VM core implementation
// (inside the current source directory) and other parts of the sampler
// (located at other source directories). It also acts as public API of the
// Real-Time script engine for other applications.

#ifndef LS_INSTR_SCRIPT_PARSER_COMMON_H
#define LS_INSTR_SCRIPT_PARSER_COMMON_H

#include "../common/global.h"
#include <vector>
#include <map>
#include <stddef.h> // offsetof()
#include <functional> // std::function<>

namespace LinuxSampler {

    /**
     * Native data type used by the script engine both internally, as well as
     * for all integer data types used by scripts (i.e. for all $foo variables
     * in NKSP scripts). Note that this is different from the original KSP which
     * is limited to 32 bit for integer variables in KSP scripts.
     */
    typedef int64_t vmint;

    /**
     * Native data type used internally by the script engine for all unsigned
     * integer types. This type is currently not exposed to scripts.
     */
    typedef uint64_t vmuint;

    /**
     * Native data type used by the script engine both internally for floating
     * point data, as well as for all @c real data types used by scripts (i.e.
     * for all ~foo variables in NKSP scripts).
     */
    typedef float vmfloat;

    /**
     * Identifies the type of a noteworthy issue identified by the script
     * parser. That's either a parser error or parser warning.
     */
    enum ParserIssueType_t {
        PARSER_ERROR, ///< Script parser encountered an error, the script cannot be executed.
        PARSER_WARNING ///< Script parser encountered a warning, the script may be executed if desired, but the script may not necessarily behave as originally intended by the script author.
    };

    /** @brief Expression's data type.
     *
     * Identifies to which data type an expression within a script evaluates to.
     * This can for example reflect the data type of script function arguments,
     * script function return values, but also the resulting data type of some
     * mathematical formula within a script.
     */
    enum ExprType_t {
        EMPTY_EXPR, ///< i.e. on invalid expressions or i.e. a function call that does not return a result value (the built-in wait() or message() functions for instance)
        INT_EXPR, ///< integer (scalar) expression
        INT_ARR_EXPR, ///< integer array expression
        STRING_EXPR, ///< string expression
        STRING_ARR_EXPR, ///< string array expression
        REAL_EXPR, ///< floating point (scalar) expression
        REAL_ARR_EXPR, ///< floating point array expression
    };

    /** @brief Result flags of a script statement or script function call.
     *
     * A set of bit flags which provide informations about the success or
     * failure of a statement within a script. That's also especially used for
     * providing informations about success / failure of a call to a built-in
     * script function. The virtual machine evaluates these flags during runtime
     * to decide whether it should i.e. stop or suspend execution of a script.
     *
     * Since these are bit flags, these constants are bitwise combined.
     */
    enum StmtFlags_t {
       STMT_SUCCESS = 0, ///< Function / statement was executed successfully, no error occurred.
       STMT_ABORT_SIGNALLED = 1, ///< VM should stop the current callback execution (usually because of an error, but might also be without an error reason, i.e. when the built-in script function exit() was called).
       STMT_SUSPEND_SIGNALLED = (1<<1), ///< VM supended execution, either because the script called the built-in wait() script function or because the script consumed too much execution time and was forced by the VM to be suspended for some time
       STMT_ERROR_OCCURRED = (1<<2), ///< VM stopped execution due to some script runtime error that occurred 
    };

    /** @brief Virtual machine execution status.
     *
     * A set of bit flags which reflect the current overall execution status of
     * the virtual machine concerning a certain script execution instance.
     *
     * Since these are bit flags, these constants are bitwise combined.
     */
    enum VMExecStatus_t {
        VM_EXEC_NOT_RUNNING = 0, ///< Script is currently not executed by the VM.
        VM_EXEC_RUNNING = 1, ///< The VM is currently executing the script.
        VM_EXEC_SUSPENDED = (1<<1), ///< Script is currently suspended by the VM, either because the script called the built-in wait() script function or because the script consumed too much execution time and was forced by the VM to be suspended for some time.
        VM_EXEC_ERROR = (1<<2), ///< A runtime error occurred while executing the script (i.e. a call to some built-in script function failed).
    };

    /** @brief Script event handler type.
     *
     * Identifies one of the possible event handler callback types defined by
     * the NKSP script language.
     *
     * IMPORTANT: this type is forced to be emitted as int32_t type ATM, because
     * that's the native size expected by the built-in instrument script
     * variable bindings (see occurrences of VMInt32RelPtr and DECLARE_VMINT
     * respectively. A native type mismatch between the two could lead to
     * undefined behavior! Background: By definition the C/C++ compiler is free
     * to choose a bit size for individual enums which it might find
     * appropriate, which is usually decided by the compiler according to the
     * biggest enum constant value defined (in practice it is usually 32 bit).
     */
    enum VMEventHandlerType_t : int32_t {
        VM_EVENT_HANDLER_INIT, ///< Initilization event handler, that is script's "on init ... end on" code block.
        VM_EVENT_HANDLER_NOTE, ///< Note event handler, that is script's "on note ... end on" code block.
        VM_EVENT_HANDLER_RELEASE, ///< Release event handler, that is script's "on release ... end on" code block.
        VM_EVENT_HANDLER_CONTROLLER, ///< Controller event handler, that is script's "on controller ... end on" code block.
    };

    /**
     * All metric unit prefixes (actually just scale factors) supported by this
     * script engine.
     */
    enum MetricPrefix_t {
        VM_NO_PREFIX = 0, ///< = 1
        VM_KILO,          ///< = 10^3, short 'k'
        VM_HECTO,         ///< = 10^2, short 'h'
        VM_DECA,          ///< = 10, short 'da'
        VM_DECI,          ///< = 10^-1, short 'd'
        VM_CENTI,         ///< = 10^-2, short 'c' (this is also used for tuning "cents")
        VM_MILLI,         ///< = 10^-3, short 'm'
        VM_MICRO,         ///< = 10^-6, short 'u'
    };

    /**
     * This constant is used for comparison with Unit::unitFactor() to check
     * whether a number does have any metric unit prefix at all.
     *
     * @see Unit::unitFactor()
     */
    static const vmfloat VM_NO_FACTOR = vmfloat(1);

    /**
     * All measurement unit types supported by this script engine.
     *
     * @e Note: there is no standard unit "cents" here (for pitch/tuning), use
     * @c VM_CENTI for the latter instad. That's because the commonly cited
     * "cents" unit is actually no measurement unit type but rather a metric
     * unit prefix.
     *
     * @see MetricPrefix_t
     */
    enum StdUnit_t {
        VM_NO_UNIT = 0, ///< No unit used, the number is just an abstract number.
        VM_SECOND,      ///< Measuring time.
        VM_HERTZ,       ///< Measuring frequency.
        VM_BEL,         ///< Measuring relation between two energy levels (in logarithmic scale). Since we are using it for accoustics, we are always referring to A-weighted Bels (i.e. dBA).
    };

    //TODO: see Unit::hasUnitFactorEver()
    enum EverTriState_t {
        VM_NEVER = 0,
        VM_MAYBE,
        VM_ALWAYS,
    };

    // just symbol prototyping
    class VMIntExpr;
    class VMRealExpr;
    class VMStringExpr;
    class VMNumberExpr;
    class VMArrayExpr;
    class VMIntArrayExpr;
    class VMRealArrayExpr;
    class VMStringArrayExpr;
    class VMParserContext;

    /** @brief Virtual machine standard measuring unit.
     *
     * Abstract base class representing standard measurement units throughout
     * the script engine. These might be e.g. "dB" (deci Bel) for loudness,
     * "Hz" (Hertz) for frequencies or "s" for "seconds". These unit types can
     * combined with metric prefixes, for instance "kHz" (kilo Hertz),
     * "us" (micro second), etc.
     *
     * Originally the script engine only supported abstract integer values for
     * controlling any synthesis parameter or built-in function argument or
     * variable. Under certain situations it makes sense though for an
     * instrument script author to provide values in real, standard measurement
     * units to provide a more natural and intuitive approach for writing
     * instrument scripts, for example by setting the frequency of some LFO
     * directly to "20Hz" or reducing loudness by "-4.2dB". Hence support for
     * standard units in scripts was added as an extension to the NKSP script
     * engine.
     *
     * So a unit consists of 1) a sequence of metric prefixes as scale factor
     * (e.g. "k" for kilo) and 2) the actual unit type (e.g. "Hz" for Hertz).
     * The unit type is a constant feature of number literals and variables, so
     * once a variable was declared with a unit type (or no unit type at all)
     * then that unit type of that variable cannot be changed for the entire
     * life time of the script. This is different from the unit's metric
     * prefix(es) of variables which may freely be changed at runtime.
     */
    class VMUnit {
    public:
        /**
         * Returns the metric prefix(es) of this unit as unit factor. A metric
         * prefix essentially is just a mathematical scale factor that should be
         * applied to the number associated with the measurement unit. Consider
         * a string literal in an NKSP script like '3kHz' where 'k' (kilo) is
         * the metric prefix, which essentically is a scale factor of 1000.
         *
         * Usually a unit either has exactly none or one metric prefix, but note
         * that there might also be units with more than one prefix, for example
         * @c mdB (milli deci Bel) is used sometimes which has two prefixes. The
         * latter is an exception though and more than two prefixes is currently
         * not supported by the script engine.
         *
         * The factor returned by this method is the final mathematical factor
         * that should be multiplied against the number associated with this
         * unit. This factor results from the sequence of metric prefixes of
         * this unit.
         *
         * @see MetricPrefix_t, hasUnitFactorNow(), hasUnitFactorEver(),
         *      VM_NO_FACTOR
         * @returns current metric unit factor
         */
        virtual vmfloat unitFactor() const = 0;

        //TODO: this still needs to be implemented in tree.h/.pp, built-in functions and as 2nd pass of parser appropriately
        /*virtual*/ EverTriState_t hasUnitFactorEver() const { return VM_NEVER; }

        /**
         * Whether this unit currently does have any metric unit prefix.
         *
         * This is actually just a convenience method which returns @c true if
         * unitFactor() is not @c 1.0.
         *
         * @see MetricPrefix_t, unitFactor(), hasUnitFactorEver(), VM_NO_FACTOR
         * @returns @c true if this unit currently has any metric prefix
         */
        bool hasUnitFactorNow() const;

        /**
         * This is the actual fundamental measuring unit base type of this unit,
         * which might be either Hertz, second or Bel.
         *
         * Note that a number without a unit type may still have metric
         * prefixes.
         *
         * @returns standard unit type identifier or VM_NO_UNIT if no unit type
         *          is used for this object
         */
        virtual StdUnit_t unitType() const = 0;

        /**
         * Returns the actual mathematical factor represented by the passed
         * @a prefix argument.
         */
        static vmfloat unitFactor(MetricPrefix_t prefix);

        /**
         * Returns the actual mathematical factor represented by the passed
         * two @a prefix1 and @a prefix2 arguments.
         *
         * @returns scale factor of given metric unit prefixes
         */
        static vmfloat unitFactor(MetricPrefix_t prefix1, MetricPrefix_t prefix2);

        /**
         * Returns the actual mathematical factor represented by the passed
         * @a prefixes array. The passed array should always be terminated by a
         * VM_NO_PREFIX value as last element.
         *
         * @param prefixes - sequence of metric prefixes
         * @param size - max. amount of elements of array @a prefixes
         * @returns scale factor of given metric unit prefixes
         */
        static vmfloat unitFactor(const MetricPrefix_t* prefixes, vmuint size = 2);
    };

    /** @brief Virtual machine expression
     *
     * This is the abstract base class for all expressions of scripts.
     * Deriving classes must implement the abstract method exprType().
     *
     * An expression within a script is translated into one instance of this
     * class. It allows a high level access for the virtual machine to evaluate
     * and handle expressions appropriately during execution. Expressions are
     * for example all kinds of formulas, function calls, statements or a
     * combination of them. Most of them evaluate to some kind of value, which
     * might be further processed as part of encompassing expressions to outer
     * expression results and so forth.
     */
    class VMExpr {
    public:
        /**
         * Identifies the data type to which the result of this expression
         * evaluates to. This abstract method must be implemented by deriving
         * classes.
         */
        virtual ExprType_t exprType() const = 0;

        /**
         * In case this expression is an integer expression, then this method
         * returns a casted pointer to that VMIntExpr object. It returns NULL
         * if this expression is not an integer expression.
         *
         * @b Note: type casting performed by this method is strict! That means
         * if this expression is i.e. actually a string expression like "12",
         * calling asInt() will @b not cast that numerical string expression to
         * an integer expression 12 for you, instead this method will simply
         * return NULL! Same applies if this expression is actually a real
         * number expression: asInt() would return NULL in that case as well.
         *
         * @see exprType(), asReal(), asNumber()
         */
        VMIntExpr* asInt() const;

        /**
         * In case this expression is a real number (floating point) expression,
         * then this method returns a casted pointer to that VMRealExpr object.
         * It returns NULL if this expression is not a real number expression.
         *
         * @b Note: type casting performed by this method is strict! That means
         * if this expression is i.e. actually a string expression like "12",
         * calling asReal() will @b not cast that numerical string expression to
         * a real number expression 12.0 for you, instead this method will
         * simply return NULL! Same applies if this expression is actually an
         * integer expression: asReal() would return NULL in that case as well.
         *
         * @see exprType(), asInt(), asNumber()
         */
        VMRealExpr* asReal() const;

        /**
         * In case this expression is a scalar number expression, that is either
         * an integer (scalar) expression or a real number (floating point
         * scalar) expression, then this method returns a casted pointer to that
         * VMNumberExpr base class object. It returns NULL if this
         * expression is neither an integer (scalar), nor a real number (scalar)
         * expression.
         *
         * Since the methods asInt() and asReal() are very strict, this method
         * is provided as convenience access in case only very general
         * information (e.g. which standard measurement unit is being used or
         * whether final operator being effective to this expression) is
         * intended to be retrieved of this scalar number expression independent
         * from whether this expression is actually an integer or a real number
         * expression.
         *
         * @see exprType(), asInt(), asReal()
         */
        VMNumberExpr* asNumber() const;

        /**
         * In case this expression is a string expression, then this method
         * returns a casted pointer to that VMStringExpr object. It returns NULL
         * if this expression is not a string expression.
         *
         * @b Note: type casting performed by this method is strict! That means
         * if this expression is i.e. actually an integer expression like 120,
         * calling asString() will @b not cast that integer expression to a
         * string expression "120" for you, instead this method will simply
         * return NULL!
         *
         * @see exprType()
         */
        VMStringExpr* asString() const;

        /**
         * In case this expression is an integer array expression, then this
         * method returns a casted pointer to that VMIntArrayExpr object. It
         * returns NULL if this expression is not an integer array expression.
         *
         * @b Note: type casting performed by this method is strict! That means
         * if this expression is i.e. an integer scalar expression, a real
         * number expression or a string expression, calling asIntArray() will
         * @b not cast those expressions to an integer array expression for you,
         * instead this method will simply return NULL!
         *
         * @b Note: this method is currently, and in contrast to its other
         * counter parts, declared as virtual method. Some deriving classes are
         * currently using this to override this default implementation in order
         * to implement an "evaluate now as integer array" behavior. This has
         * efficiency reasons, however this also currently makes this part of
         * the API less clean and should thus be addressed in future with
         * appropriate changes to the API.
         *
         * @see exprType()
         */
        virtual VMIntArrayExpr* asIntArray() const;

        /**
         * In case this expression is a real number (floating point) array
         * expression, then this method returns a casted pointer to that
         * VMRealArrayExpr object. It returns NULL if this expression is not a
         * real number array expression.
         *
         * @b Note: type casting performed by this method is strict! That means
         * if this expression is i.e. a real number scalar expression, an
         * integer expression or a string expression, calling asRealArray() will
         * @b not cast those scalar expressions to a real number array
         * expression for you, instead this method will simply return NULL!
         *
         * @b Note: this method is currently, and in contrast to its other
         * counter parts, declared as virtual method. Some deriving classes are
         * currently using this to override this default implementation in order
         * to implement an "evaluate now as real number array" behavior. This
         * has efficiency reasons, however this also currently makes this part
         * of the API less clean and should thus be addressed in future with
         * appropriate changes to the API.
         *
         * @see exprType()
         */
        virtual VMRealArrayExpr* asRealArray() const;

        /**
         * This is an alternative to calling either asIntArray() or
         * asRealArray(). This method here might be used if the fundamental
         * scalar data type (real or integer) of the array is not relevant,
         * i.e. for just getting the size of the array. Since all as*() methods
         * here are very strict regarding type casting, this asArray() method
         * sometimes can reduce code complexity.
         *
         * Likewise calling this method only returns a valid pointer if the
         * expression is some array type (currently either integer array or real
         * number array). For any other expression type this method will return
         * NULL instead.
         *
         * @see exprType()
         */
        VMArrayExpr* asArray() const;

        /**
         * Returns true in case this expression can be considered to be a
         * constant expression. A constant expression will retain the same
         * value throughout the entire life time of a script and the
         * expression's constant value may be evaluated already at script
         * parse time, which may result in performance benefits during script
         * runtime.
         *
         * @b NOTE: A constant expression is per se always also non modifyable.
         * But a non modifyable expression may not necessarily be a constant
         * expression!
         *
         * @see isModifyable()
         */
        virtual bool isConstExpr() const = 0;

        /**
         * Returns true in case this expression is allowed to be modified.
         * If this method returns @c false then this expression must be handled
         * as read-only expression, which means that assigning a new value to it
         * is either not possible or not allowed.
         *
         * @b NOTE: A constant expression is per se always also non modifyable.
         * But a non modifyable expression may not necessarily be a constant
         * expression!
         *
         * @see isConstExpr()
         */
        bool isModifyable() const;
    };

    /** @brief Virtual machine scalar number expression
     *
     * This is the abstract base class for integer (scalar) expressions and
     * real number (floating point scalar) expressions of scripts.
     */
    class VMNumberExpr : virtual public VMExpr, virtual public VMUnit {
    public:
        /**
         * Returns @c true if the value of this expression should be applied
         * as final value to the respective destination synthesis chain
         * parameter.
         *
         * This property is somewhat special and dedicated for the purpose of
         * this expression's (integer or real number) value to be applied as
         * parameter to the synthesis chain of the sampler (i.e. for altering a
         * filter cutoff frequency). Now historically and by default all values
         * of scripts are applied relatively to the sampler's synthesis chain,
         * that is the synthesis parameter value of a script is multiplied
         * against other sources for the same synthesis parameter (i.e. an LFO
         * or a dedicated MIDI controller either hard wired in the engine or
         * defined by the instrument patch). So by default the resulting actual
         * final synthesis parameter is a combination of all these sources. This
         * has the advantage that it creates a very living and dynamic overall
         * sound.
         *
         * However sometimes there are requirements by script authors where this
         * is not what you want. Therefore the NKSP script engine added a
         * language extension by prefixing a value in scripts with a @c !
         * character the value will be defined as being the "final" value of the
         * destination synthesis parameter, so that causes this value to be
         * applied exclusively, and the values of all other sources are thus
         * entirely ignored by the sampler's synthesis core as long as this
         * value is assigned by the script engine as "final" value for the
         * requested synthesis parameter.
         */
        virtual bool isFinal() const = 0;

        /**
         * Calling this method evaluates the expression and returns the value
         * of the expression as integer. If this scalar number expression is a
         * real number expression then this method automatically casts the value
         * from real number to integer.
         */
        vmint evalCastInt();

        /**
         * Calling this method evaluates the expression and returns the value
         * of the expression as integer and thus behaves similar to the previous
         * method, however this overridden method automatically takes unit
         * prefixes into account and returns a converted value corresponding to
         * the given unit @a prefix expected by the caller.
         *
         * Example: Assume this expression was an integer expression '12kHz'
         * then calling this method as @c evalCastInt(VM_MILLI) would return
         * the value @c 12000000.
         *
         * @param prefix - measuring unit prefix expected for result by caller
         */
        vmint evalCastInt(MetricPrefix_t prefix);

        /**
         * This method behaves like the previous method, just that it takes a
         * measuring unit prefix with two elements (e.g. "milli cents" for
         * tuning).
         *
         * @param prefix1 - 1st measuring unit prefix element expected by caller
         * @param prefix2 - 2nd measuring unit prefix element expected by caller
         */
        vmint evalCastInt(MetricPrefix_t prefix1, MetricPrefix_t prefix2);

        /**
         * Calling this method evaluates the expression and returns the value
         * of the expression as real number. If this scalar number expression is
         * an integer expression then this method automatically casts the value
         * from integer to real number.
         */
        vmfloat evalCastReal();

        /**
         * Calling this method evaluates the expression and returns the value
         * of the expression as real number and thus behaves similar to the
         * previous method, however this overridden method automatically takes
         * unit prefixes into account and returns a converted value
         * corresponding to the given unit @a prefix expected by the caller.
         *
         * Example: Assume this expression was an integer expression '8ms' then
         * calling this method as @c evalCastReal(VM_NO_PREFIX) would return the
         * value @c 0.008.
         *
         * @param prefix - measuring unit prefix expected for result by caller
         */
        vmfloat evalCastReal(MetricPrefix_t prefix);

        /**
         * This method behaves like the previous method, just that it takes a
         * measuring unit prefix with two elements (e.g. "milli cents" for
         * tuning).
         *
         * @param prefix1 - 1st measuring unit prefix element expected by caller
         * @param prefix2 - 2nd measuring unit prefix element expected by caller
         */
        vmfloat evalCastReal(MetricPrefix_t prefix1, MetricPrefix_t prefix2);
    };

    /** @brief Virtual machine integer expression
     *
     * This is the abstract base class for all expressions inside scripts which
     * evaluate to an integer (scalar) value. Deriving classes implement the
     * abstract method evalInt() to return the actual integer result value of
     * the expression.
     */
    class VMIntExpr : virtual public VMNumberExpr {
    public:
        /**
         * Returns the result of this expression as integer (scalar) value.
         * This abstract method must be implemented by deriving classes.
         */
        virtual vmint evalInt() = 0;

        /**
         * Returns the result of this expression as integer (scalar) value and
         * thus behaves similar to the previous method, however this overridden
         * method automatically takes unit prefixes into account and returns a
         * value corresponding to the expected given unit @a prefix.
         *
         * @param prefix - default measurement unit prefix expected by caller
         */
        vmint evalInt(MetricPrefix_t prefix);

        /**
         * This method behaves like the previous method, just that it takes
         * a default measurement prefix with two elements (i.e. "milli cents"
         * for tuning).
         */
        vmint evalInt(MetricPrefix_t prefix1, MetricPrefix_t prefix2);

        /**
         * Returns always INT_EXPR for instances of this class.
         */
        ExprType_t exprType() const OVERRIDE { return INT_EXPR; }
    };

    /** @brief Virtual machine real number (floating point scalar) expression
     *
     * This is the abstract base class for all expressions inside scripts which
     * evaluate to a real number (floating point scalar) value. Deriving classes
     * implement the abstract method evalReal() to return the actual floating
     * point result value of the expression.
     */
    class VMRealExpr : virtual public VMNumberExpr {
    public:
        /**
         * Returns the result of this expression as real number (floating point
         * scalar) value. This abstract method must be implemented by deriving
         * classes.
         */
        virtual vmfloat evalReal() = 0;

        /**
         * Returns the result of this expression as real number (floating point
         * scalar) value and thus behaves similar to the previous method,
         * however this overridden method automatically takes unit prefixes into
         * account and returns a value corresponding to the expected given unit
         * @a prefix.
         *
         * @param prefix - default measurement unit prefix expected by caller
         */
        vmfloat evalReal(MetricPrefix_t prefix);

        /**
         * This method behaves like the previous method, just that it takes
         * a default measurement prefix with two elements (i.e. "milli cents"
         * for tuning).
         */
        vmfloat evalReal(MetricPrefix_t prefix1, MetricPrefix_t prefix2);

        /**
         * Returns always REAL_EXPR for instances of this class.
         */
        ExprType_t exprType() const OVERRIDE { return REAL_EXPR; }
    };

    /** @brief Virtual machine string expression
     *
     * This is the abstract base class for all expressions inside scripts which
     * evaluate to a string value. Deriving classes implement the abstract
     * method evalStr() to return the actual string result value of the
     * expression.
     */
    class VMStringExpr : virtual public VMExpr {
    public:
        /**
         * Returns the result of this expression as string value. This abstract
         * method must be implemented by deriving classes.
         */
        virtual String evalStr() = 0;

        /**
         * Returns always STRING_EXPR for instances of this class.
         */
        ExprType_t exprType() const OVERRIDE { return STRING_EXPR; }
    };

    /** @brief Virtual Machine Array Value Expression
     *
     * This is the abstract base class for all expressions inside scripts which
     * evaluate to some kind of array value. Deriving classes implement the
     * abstract method arraySize() to return the amount of elements within the
     * array.
     */
    class VMArrayExpr : virtual public VMExpr {
    public:
        /**
         * Returns amount of elements in this array. This abstract method must
         * be implemented by deriving classes.
         */
        virtual vmint arraySize() const = 0;
    };

    /** @brief Virtual Machine Number Array Expression
     *
     * This is the abstract base class for all expressions which either evaluate
     * to an integer array or real number array.
     */
    class VMNumberArrayExpr : virtual public VMArrayExpr {
    public:
        /**
         * Returns the metric unit factor of the requested array element.
         *
         * @param i - array element index (must be between 0 .. arraySize() - 1)
         * @see VMUnit::unitFactor() for details about metric unit factors
         */
        virtual vmfloat unitFactorOfElement(vmuint i) const = 0;

        /**
         * Changes the current unit factor of the array element given by element
         * index @a i.
         *
         * @param i - array element index (must be between 0 .. arraySize() - 1)
         * @param factor - new unit factor to be assigned
         * @see VMUnit::unitFactor() for details about metric unit factors
         */
        virtual void assignElementUnitFactor(vmuint i, vmfloat factor) = 0;
    };

    /** @brief Virtual Machine Integer Array Expression
     *
     * This is the abstract base class for all expressions inside scripts which
     * evaluate to an array of integer values. Deriving classes implement the
     * abstract methods arraySize(), evalIntElement() and assignIntElement() to
     * access the individual integer array values.
     */
    class VMIntArrayExpr : virtual public VMNumberArrayExpr {
    public:
        /**
         * Returns the (scalar) integer value of the array element given by
         * element index @a i.
         *
         * @param i - array element index (must be between 0 .. arraySize() - 1)
         */
        virtual vmint evalIntElement(vmuint i) = 0;

        /**
         * Changes the current value of an element (given by array element
         * index @a i) of this integer array.
         *
         * @param i - array element index (must be between 0 .. arraySize() - 1)
         * @param value - new integer scalar value to be assigned to that array element
         */
        virtual void assignIntElement(vmuint i, vmint value) = 0;

        /**
         * Returns always INT_ARR_EXPR for instances of this class.
         */
        ExprType_t exprType() const OVERRIDE { return INT_ARR_EXPR; }
    };

    /** @brief Virtual Machine Real Number Array Expression
     *
     * This is the abstract base class for all expressions inside scripts which
     * evaluate to an array of real numbers (floating point values). Deriving
     * classes implement the abstract methods arraySize(), evalRealElement() and
     * assignRealElement() to access the array's individual real numbers.
     */
    class VMRealArrayExpr : virtual public VMNumberArrayExpr {
    public:
        /**
         * Returns the (scalar) real mumber (floating point value) of the array
         * element given by element index @a i.
         *
         * @param i - array element index (must be between 0 .. arraySize() - 1)
         */
        virtual vmfloat evalRealElement(vmuint i) = 0;

        /**
         * Changes the current value of an element (given by array element
         * index @a i) of this real number array.
         *
         * @param i - array element index (must be between 0 .. arraySize() - 1)
         * @param value - new real number value to be assigned to that array element
         */
        virtual void assignRealElement(vmuint i, vmfloat value) = 0;

        /**
         * Returns always REAL_ARR_EXPR for instances of this class.
         */
        ExprType_t exprType() const OVERRIDE { return REAL_ARR_EXPR; }
    };

    /** @brief Arguments (parameters) for being passed to a built-in script function.
     *
     * An argument or a set of arguments passed to a script function are
     * translated by the parser to an instance of this class. This abstract
     * interface class is used by implementations of built-in functions to
     * obtain the individual function argument values being passed to them at
     * runtime.
     */
    class VMFnArgs {
    public:
        /**
         * Returns the amount of arguments going to be passed to the script
         * function.
         */
        virtual vmint argsCount() const = 0;

        /**
         * Returns the respective argument (requested by argument index @a i) of
         * this set of arguments. This method is called by implementations of
         * built-in script functions to obtain the value of each function
         * argument passed to the function at runtime.
         *
         * @param i - function argument index (indexed from left to right)
         * @return requested function argument or NULL if @a i out of bounds
         */
        virtual VMExpr* arg(vmint i) = 0;
    };

    /** @brief Result value returned from a call to a built-in script function.
     *
     * Implementations of built-in script functions return an instance of this
     * object to let the virtual machine obtain the result value of the function
     * call, which might then be further processed by the virtual machine
     * according to the script. It also provides informations about the success
     * or failure of the function call.
     */
    class VMFnResult {
    public:
        /**
         * Returns the result value of the function call, represented by a high
         * level expression object.
         */
        virtual VMExpr* resultValue() = 0;

        /**
         * Provides detailed informations of the success / failure of the
         * function call. The virtual machine is evaluating the flags returned
         * here to decide whether it must abort or suspend execution of the
         * script at this point.
         */
        virtual StmtFlags_t resultFlags() { return STMT_SUCCESS; }
    };

    /** @brief Virtual machine built-in function.
     *
     * Abstract base class for built-in script functions, defining the interface
     * for all built-in script function implementations. All built-in script
     * functions are deriving from this abstract interface class in order to
     * provide their functionality to the virtual machine with this unified
     * interface.
     *
     * The methods of this interface class provide two purposes:
     *
     * 1. When a script is loaded, the script parser uses the methods of this
     *    interface to check whether the script author was calling the
     *    respective built-in script function in a correct way. For example
     *    the parser checks whether the required amount of parameters were
     *    passed to the function and whether the data types passed match the
     *    data types expected by the function. If not, loading the script will
     *    be aborted with a parser error, describing to the user (i.e. script
     *    author) the precise misusage of the respective function.
     * 2. After the script was loaded successfully and the script is executed,
     *    the virtual machine calls the exec() method of the respective built-in
     *    function to provide the actual functionality of the built-in function
     *    call.
     */
    class VMFunction {
    public:
        /**
         * Script data type of the function's return value. If the function does
         * not return any value (void), then it returns EMPTY_EXPR here.
         *
         * Some functions may have a different return type depending on the
         * arguments to be passed to this function. That's what the @a args
         * parameter is for, so that the method implementation can look ahead
         * of what kind of parameters are going to be passed to the built-in
         * function later on in order to decide which return value type would
         * be used and returned by the function accordingly in that case.
         *
         * @param args - function arguments going to be passed for executing
         *               this built-in function later on
         */
        virtual ExprType_t returnType(VMFnArgs* args) = 0;

        /**
         * Standard measuring unit type of the function's result value
         * (e.g. second, Hertz).
         *
         * Some functions may have a different standard measuring unit type for
         * their return value depending on the arguments to be passed to this
         * function. That's what the @a args parameter is for, so that the
         * method implementation can look ahead of what kind of parameters are
         * going to be passed to the built-in function later on in order to
         * decide which return value type would be used and returned by the
         * function accordingly in that case.
         *
         * @param args - function arguments going to be passed for executing
         *               this built-in function later on
         * @see Unit for details about standard measuring units
         */
        virtual StdUnit_t returnUnitType(VMFnArgs* args) = 0;

        /**
         * Whether the result value returned by this built-in function is
         * considered to be a 'final' value.
         *
         * Some functions may have a different 'final' feature for their return
         * value depending on the arguments to be passed to this function.
         * That's what the @a args parameter is for, so that the method
         * implementation can look ahead of what kind of parameters are going to
         * be passed to the built-in function later on in order to decide which
         * return value type would be used and returned by the function
         * accordingly in that case.
         *
         * @param args - function arguments going to be passed for executing
         *               this built-in function later on
         * @see VMNumberExpr::isFinal() for details about 'final' values
         */
        virtual bool returnsFinal(VMFnArgs* args) = 0;

        /**
         * Minimum amount of function arguments this function accepts. If a
         * script is calling this function with less arguments, the script
         * parser will throw a parser error.
         */
        virtual vmint minRequiredArgs() const = 0;

        /**
         * Maximum amount of function arguments this functions accepts. If a
         * script is calling this function with more arguments, the script
         * parser will throw a parser error.
         */
        virtual vmint maxAllowedArgs() const = 0;

        /**
         * This method is called by the parser to check whether arguments
         * passed in scripts to this function are accepted by this function. If
         * a script calls this function with an argument's data type not
         * accepted by this function, the parser will throw a parser error.
         *
         * The parser will also use this method to assemble a list of actually
         * supported data types accepted by this built-in function for the
         * function argument in question, that is to provide an appropriate and
         * precise parser error message in such cases.
         *
         * @param iArg - index of the function argument in question
         *               (must be between 0 .. maxAllowedArgs() - 1)
         * @param type - script data type used for this function argument by
         *               currently parsed script
         * @return true if the given data type would be accepted for the
         *         respective function argument by the function
         */
        virtual bool acceptsArgType(vmint iArg, ExprType_t type) const = 0;

        /**
         * This method is called by the parser to check whether arguments
         * passed in scripts to this function are accepted by this function. If
         * a script calls this function with an argument's measuremnt unit type
         * not accepted by this function, the parser will throw a parser error.
         *
         * This default implementation of this method does not accept any
         * measurement unit. Deriving subclasses would override this method
         * implementation in case they do accept any measurement unit for its
         * function arguments.
         *
         * @param iArg - index of the function argument in question
         *               (must be between 0 .. maxAllowedArgs() - 1)
         * @param type - standard measurement unit data type used for this
         *               function argument by currently parsed script
         * @return true if the given standard measurement unit type would be
         *         accepted for the respective function argument by the function
         */
        virtual bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const;

        /**
         * This method is called by the parser to check whether arguments
         * passed in scripts to this function are accepted by this function. If
         * a script calls this function with a metric unit prefix and metric
         * prefixes are not accepted for that argument by this function, then
         * the parser will throw a parser error.
         *
         * This default implementation of this method does not accept any
         * metric prefix. Deriving subclasses would override this method
         * implementation in case they do accept any metric prefix for its
         * function arguments.
         *
         * @param iArg - index of the function argument in question
         *               (must be between 0 .. maxAllowedArgs() - 1)
         * @param type - standard measurement unit data type used for that
         *               function argument by currently parsed script
         *
         * @return true if a metric prefix would be accepted for the respective
         *         function argument by this function
         *
         * @see MetricPrefix_t
         */
        virtual bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const;

        /**
         * This method is called by the parser to check whether arguments
         * passed in scripts to this function are accepted by this function. If
         * a script calls this function with an argument that is declared to be
         * a "final" value and this is not accepted by this function, the parser
         * will throw a parser error.
         *
         * This default implementation of this method does not accept a "final"
         * value. Deriving subclasses would override this method implementation
         * in case they do accept a "final" value for its function arguments.
         *
         * @param iArg - index of the function argument in question
         *               (must be between 0 .. maxAllowedArgs() - 1)
         * @return true if a "final" value would be accepted for the respective
         *         function argument by the function
         *
         * @see VMNumberExpr::isFinal(), returnsFinal()
         */
        virtual bool acceptsArgFinal(vmint iArg) const;

        /**
         * This method is called by the parser to check whether some arguments
         * (and if yes which ones) passed to this script function will be
         * modified by this script function. Most script functions simply use
         * their arguments as inputs, that is they only read the argument's
         * values. However some script function may also use passed
         * argument(s) as output variables. In this case the function
         * implementation must return @c true for the respective argument
         * index here.
         *
         * @param iArg - index of the function argument in question
         *               (must be between 0 .. maxAllowedArgs() - 1)
         */
        virtual bool modifiesArg(vmint iArg) const = 0;

        /** @brief Parse-time check of function arguments.
         *
         * This method is called by the parser to let the built-in function
         * perform its own, individual parse time checks on the arguments to be
         * passed to the built-in function. So this method is the place for
         * implementing custom checks which are very specific to the individual
         * built-in function's purpose and its individual requirements.
         *
         * For instance the built-in 'in_range()' function uses this method to
         * check whether the last 2 of their 3 arguments are of same data type
         * and if not it triggers a parser error. 'in_range()' also checks
         * whether all of its 3 arguments do have the same standard measuring
         * unit type and likewise raises a parser error if not.
         *
         * For less critical issues built-in functions may also raise parser
         * warnings instead.
         *
         * It is recommended that classes implementing (that is overriding) this
         * method should always call their super class's implementation of this
         * method to ensure their potential parse time checks are always
         * performed as well.
         *
         * @param args - function arguments going to be passed for executing
         *               this built-in function later on
         * @param err - the parser's error handler to be called by this method
         *              implementation to trigger a parser error with the
         *              respective error message text
         * @param wrn - the parser's warning handler to be called by this method
         *              implementation to trigger a parser warning with the
         *              respective warning message text
         */
        virtual void checkArgs(VMFnArgs* args,
                               std::function<void(String)> err,
                               std::function<void(String)> wrn);

        /**
         * Implements the actual function execution. This exec() method is
         * called by the VM whenever this function implementation shall be
         * executed at script runtime. This method blocks until the function
         * call completed.
         *
         * @param args - function arguments for executing this built-in function
         * @returns function's return value (if any) and general status
         *          informations (i.e. whether the function call caused a
         *          runtime error)
         */
        virtual VMFnResult* exec(VMFnArgs* args) = 0;

        /**
         * Convenience method for function implementations to show warning
         * messages during actual execution of the built-in function.
         *
         * @param txt - runtime warning text to be shown to user
         */
        void wrnMsg(const String& txt);

        /**
         * Convenience method for function implementations to show error
         * messages during actual execution of the built-in function.
         *
         * @param txt - runtime error text to be shown to user
         */
        void errMsg(const String& txt);
    };

    /** @brief Virtual machine relative pointer.
     *
     * POD base of VMInt64RelPtr, VMInt32RelPtr and VMInt8RelPtr structures. Not
     * intended to be used directly. Use VMInt64RelPtr, VMInt32RelPtr,
     * VMInt8RelPtr instead.
     *
     * @see VMInt64RelPtr, VMInt32RelPtr, VMInt8RelPtr
     */
    struct VMRelPtr {
        void** base; ///< Base pointer.
        vmint offset;  ///< Offset (in bytes) relative to base pointer.
        bool readonly; ///< Whether the pointed data may be modified or just be read.
    };

    /** @brief Pointer to built-in VM integer variable (interface class).
     *
     * This class acts as an abstract interface to all built-in integer script
     * variables, independent of their actual native size (i.e. some built-in
     * script variables are internally using a native int size of 64 bit or 32
     * bit or 8 bit). The virtual machine is using this interface class instead
     * of its implementing descendants (VMInt64RelPtr, VMInt32RelPtr,
     * VMInt8RelPtr) in order for the virtual machine for not being required to
     * handle each of them differently.
     */
    struct VMIntPtr {
        virtual vmint evalInt() = 0;
        virtual void assign(vmint i) = 0;
        virtual bool isAssignable() const = 0;
    };

    /** @brief Pointer to built-in VM integer variable (of C/C++ type int64_t).
     *
     * Used for defining built-in 64 bit integer script variables.
     *
     * @b CAUTION: You may only use this class for pointing to C/C++ variables
     * of type "int64_t" (thus being exactly 64 bit in size). If the C/C++ int
     * variable you want to reference is only 32 bit in size then you @b must
     * use VMInt32RelPtr instead! Respectively for a referenced native variable
     * with only 8 bit in size you @b must use VMInt8RelPtr instead!
     *
     * For efficiency reasons the actual native C/C++ int variable is referenced
     * by two components here. The actual native int C/C++ variable in memory
     * is dereferenced at VM run-time by taking the @c base pointer dereference
     * and adding @c offset bytes. This has the advantage that for a large
     * number of built-in int variables, only one (or few) base pointer need
     * to be re-assigned before running a script, instead of updating each
     * built-in variable each time before a script is executed.
     *
     * Refer to DECLARE_VMINT() for example code.
     *
     * @see VMInt32RelPtr, VMInt8RelPtr, DECLARE_VMINT()
     */
    struct VMInt64RelPtr : VMRelPtr, VMIntPtr {
        VMInt64RelPtr() {
            base   = NULL;
            offset = 0;
            readonly = false;
        }
        VMInt64RelPtr(const VMRelPtr& data) {
            base   = data.base;
            offset = data.offset;
            readonly = false;
        }
        vmint evalInt() OVERRIDE {
            return (vmint)*(int64_t*)&(*(uint8_t**)base)[offset];
        }
        void assign(vmint i) OVERRIDE {
            *(int64_t*)&(*(uint8_t**)base)[offset] = (int64_t)i;
        }
        bool isAssignable() const OVERRIDE { return !readonly; }
    };

    /** @brief Pointer to built-in VM integer variable (of C/C++ type int32_t).
     *
     * Used for defining built-in 32 bit integer script variables.
     *
     * @b CAUTION: You may only use this class for pointing to C/C++ variables
     * of type "int32_t" (thus being exactly 32 bit in size). If the C/C++ int
     * variable you want to reference is 64 bit in size then you @b must use
     * VMInt64RelPtr instead! Respectively for a referenced native variable with
     * only 8 bit in size you @b must use VMInt8RelPtr instead!
     *
     * For efficiency reasons the actual native C/C++ int variable is referenced
     * by two components here. The actual native int C/C++ variable in memory
     * is dereferenced at VM run-time by taking the @c base pointer dereference
     * and adding @c offset bytes. This has the advantage that for a large
     * number of built-in int variables, only one (or few) base pointer need
     * to be re-assigned before running a script, instead of updating each
     * built-in variable each time before a script is executed.
     *
     * Refer to DECLARE_VMINT() for example code.
     *
     * @see VMInt64RelPtr, VMInt8RelPtr, DECLARE_VMINT()
     */
    struct VMInt32RelPtr : VMRelPtr, VMIntPtr {
        VMInt32RelPtr() {
            base   = NULL;
            offset = 0;
            readonly = false;
        }
        VMInt32RelPtr(const VMRelPtr& data) {
            base   = data.base;
            offset = data.offset;
            readonly = false;
        }
        vmint evalInt() OVERRIDE {
            return (vmint)*(int32_t*)&(*(uint8_t**)base)[offset];
        }
        void assign(vmint i) OVERRIDE {
            *(int32_t*)&(*(uint8_t**)base)[offset] = (int32_t)i;
        }
        bool isAssignable() const OVERRIDE { return !readonly; }
    };

    /** @brief Pointer to built-in VM integer variable (of C/C++ type int8_t).
     *
     * Used for defining built-in 8 bit integer script variables.
     *
     * @b CAUTION: You may only use this class for pointing to C/C++ variables
     * of type "int8_t" (8 bit integer). If the C/C++ int variable you want to
     * reference is not exactly 8 bit in size then you @b must respectively use
     * either VMInt32RelPtr for native 32 bit variables or VMInt64RelPtrl for
     * native 64 bit variables instead!
     *
     * For efficiency reasons the actual native C/C++ int variable is referenced
     * by two components here. The actual native int C/C++ variable in memory
     * is dereferenced at VM run-time by taking the @c base pointer dereference
     * and adding @c offset bytes. This has the advantage that for a large
     * number of built-in int variables, only one (or few) base pointer need
     * to be re-assigned before running a script, instead of updating each
     * built-in variable each time before a script is executed.
     *
     * Refer to DECLARE_VMINT() for example code.
     *
     * @see VMIntRel32Ptr, VMIntRel64Ptr, DECLARE_VMINT()
     */
    struct VMInt8RelPtr : VMRelPtr, VMIntPtr {
        VMInt8RelPtr() {
            base   = NULL;
            offset = 0;
            readonly = false;
        }
        VMInt8RelPtr(const VMRelPtr& data) {
            base   = data.base;
            offset = data.offset;
            readonly = false;
        }
        vmint evalInt() OVERRIDE {
            return (vmint)*(uint8_t*)&(*(uint8_t**)base)[offset];
        }
        void assign(vmint i) OVERRIDE {
            *(uint8_t*)&(*(uint8_t**)base)[offset] = (uint8_t)i;
        }
        bool isAssignable() const OVERRIDE { return !readonly; }
    };

    /** @brief Pointer to built-in VM integer variable (of C/C++ type vmint).
     *
     * Use this typedef if the native variable to be pointed to is using the
     * typedef vmint. If the native C/C++ variable to be pointed to is using
     * another C/C++ type then better use one of VMInt64RelPtr or VMInt32RelPtr
     * instead.
     */
    typedef VMInt64RelPtr VMIntRelPtr;

    #if HAVE_CXX_EMBEDDED_PRAGMA_DIAGNOSTICS
    # define COMPILER_DISABLE_OFFSETOF_WARNING                    \
        _Pragma("GCC diagnostic push")                            \
        _Pragma("GCC diagnostic ignored \"-Winvalid-offsetof\"")
    # define COMPILER_RESTORE_OFFSETOF_WARNING \
        _Pragma("GCC diagnostic pop")
    #else
    # define COMPILER_DISABLE_OFFSETOF_WARNING
    # define COMPILER_RESTORE_OFFSETOF_WARNING
    #endif

    /**
     * Convenience macro for initializing VMInt64RelPtr, VMInt32RelPtr and
     * VMInt8RelPtr structures. Usage example:
     * @code
     * struct Foo {
     *   uint8_t a; // native representation of a built-in integer script variable
     *   int64_t b; // native representation of another built-in integer script variable
     *   int64_t c; // native representation of another built-in integer script variable
     *   uint8_t d; // native representation of another built-in integer script variable
     * };
     *
     * // initializing the built-in script variables to some values
     * Foo foo1 = (Foo) { 1, 2000, 3000, 4 };
     * Foo foo2 = (Foo) { 5, 6000, 7000, 8 };
     *
     * Foo* pFoo;
     *
     * VMInt8RelPtr varA = DECLARE_VMINT(pFoo, class Foo, a);
     * VMInt64RelPtr varB = DECLARE_VMINT(pFoo, class Foo, b);
     * VMInt64RelPtr varC = DECLARE_VMINT(pFoo, class Foo, c);
     * VMInt8RelPtr varD = DECLARE_VMINT(pFoo, class Foo, d);
     *
     * pFoo = &foo1;
     * printf("%d\n", varA->evalInt()); // will print 1
     * printf("%d\n", varB->evalInt()); // will print 2000
     * printf("%d\n", varC->evalInt()); // will print 3000
     * printf("%d\n", varD->evalInt()); // will print 4
     *
     * // same printf() code, just with pFoo pointer being changed ...
     *
     * pFoo = &foo2;
     * printf("%d\n", varA->evalInt()); // will print 5
     * printf("%d\n", varB->evalInt()); // will print 6000
     * printf("%d\n", varC->evalInt()); // will print 7000
     * printf("%d\n", varD->evalInt()); // will print 8
     * @endcode
     * As you can see above, by simply changing one single pointer, you can
     * remap a huge bunch of built-in integer script variables to completely
     * different native values/native variables. Which especially reduces code
     * complexity inside the sampler engines which provide the actual script
     * functionalities.
     */
    #define DECLARE_VMINT(basePtr, T_struct, T_member) (          \
        /* Disable offsetof warning, trust us, we are cautios. */ \
        COMPILER_DISABLE_OFFSETOF_WARNING                         \
        (VMRelPtr) {                                              \
            (void**) &basePtr,                                    \
            offsetof(T_struct, T_member),                         \
            false                                                 \
        }                                                         \
        COMPILER_RESTORE_OFFSETOF_WARNING                         \
    )                                                             \

    /**
     * Same as DECLARE_VMINT(), but this one defines the VMInt64RelPtr,
     * VMInt32RelPtr and VMInt8RelPtr structures to be of read-only type.
     * That means the script parser will abort any script at parser time if the
     * script is trying to modify such a read-only built-in variable.
     *
     * @b NOTE: this is only intended for built-in read-only variables that
     * may change during runtime! If your built-in variable's data is rather
     * already available at parser time and won't change during runtime, then
     * you should rather register a built-in constant in your VM class instead!
     *
     * @see ScriptVM::builtInConstIntVariables()
     */
    #define DECLARE_VMINT_READONLY(basePtr, T_struct, T_member) ( \
        /* Disable offsetof warning, trust us, we are cautios. */ \
        COMPILER_DISABLE_OFFSETOF_WARNING                         \
        (VMRelPtr) {                                              \
            (void**) &basePtr,                                    \
            offsetof(T_struct, T_member),                         \
            true                                                  \
        }                                                         \
        COMPILER_RESTORE_OFFSETOF_WARNING                         \
    )                                                             \

    /** @brief Built-in VM 8 bit integer array variable.
     *
     * Used for defining built-in integer array script variables (8 bit per
     * array element). Currently there is no support for any other kind of
     * built-in array type. So all built-in integer arrays accessed by scripts
     * use 8 bit data types.
     */
    struct VMInt8Array {
        int8_t* data;
        vmint size;
        bool readonly; ///< Whether the array data may be modified or just be read.

        VMInt8Array() : data(NULL), size(0), readonly(false) {}
    };

    /** @brief Virtual machine script variable.
     *
     * Common interface for all variables accessed in scripts, independent of
     * their precise data type.
     */
    class VMVariable : virtual public VMExpr {
    public:
        /**
         * Whether a script may modify the content of this variable by
         * assigning a new value to it.
         *
         * @see isConstExpr(), assign()
         */
        virtual bool isAssignable() const = 0;

        /**
         * In case this variable is assignable, this method will be called to
         * perform the value assignment to this variable with @a expr
         * reflecting the new value to be assigned.
         *
         * @param expr - new value to be assigned to this variable
         */
        virtual void assignExpr(VMExpr* expr) = 0;
    };

    /** @brief Dynamically executed variable (abstract base class).
     *
     * Interface for the implementation of a dynamically generated content of
     * a built-in script variable. Most built-in variables are simply pointers
     * to some native location in memory. So when a script reads them, the
     * memory location is simply read to get the value of the variable. A
     * dynamic variable however is not simply a memory location. For each access
     * to a dynamic variable some native code is executed to actually generate
     * and provide the content (value) of this type of variable.
     */
    class VMDynVar : public VMVariable {
    public:
        /**
         * Returns true in case this dynamic variable can be considered to be a
         * constant expression. A constant expression will retain the same value
         * throughout the entire life time of a script and the expression's
         * constant value may be evaluated already at script parse time, which
         * may result in performance benefits during script runtime.
         *
         * However due to the "dynamic" behavior of dynamic variables, almost
         * all dynamic variables are probably not constant expressions. That's
         * why this method returns @c false by default. If you are really sure
         * that your dynamic variable implementation can be considered a
         * constant expression then you may override this method and return
         * @c true instead. Note that when you return @c true here, your
         * dynamic variable will really just be executed once; and exectly
         * already when the script is loaded!
         *
         * As an example you may implement a "constant" built-in dynamic
         * variable that checks for a certain operating system feature and
         * returns the result of that OS feature check as content (value) of
         * this dynamic variable. Since the respective OS feature might become
         * available/unavailable after OS updates, software migration, etc. the
         * OS feature check should at least be performed once each time the
         * application is launched. And since the OS feature check might take a
         * certain amount of execution time, it might make sense to only
         * perform the check if the respective variable name is actually
         * referenced at all in the script to be loaded. Note that the dynamic
         * variable will still be evaluated again though if the script is
         * loaded again. So it is up to you to probably cache the result in the
         * implementation of your dynamic variable.
         *
         * On doubt, please rather consider to use a constant built-in script
         * variable instead of implementing a "constant" dynamic variable, due
         * to the runtime overhead a dynamic variable may cause.
         *
         * @see isAssignable()
         */
        bool isConstExpr() const OVERRIDE { return false; }

        /**
         * In case this dynamic variable is assignable, the new value (content)
         * to be assigned to this dynamic variable.
         *
         * By default this method does nothing. Override and implement this
         * method in your subclass in case your dynamic variable allows to
         * assign a new value by script.
         *
         * @param expr - new value to be assigned to this variable
         */
        void assignExpr(VMExpr* expr) OVERRIDE {}

        virtual ~VMDynVar() {}
    };

    /** @brief Dynamically executed variable (of integer data type).
     *
     * This is the base class for all built-in integer script variables whose
     * variable content needs to be provided dynamically by executable native
     * code on each script variable access.
     */
    class VMDynIntVar : virtual public VMDynVar, virtual public VMIntExpr {
    public:
        vmfloat unitFactor() const OVERRIDE { return VM_NO_FACTOR; }
        StdUnit_t unitType() const OVERRIDE { return VM_NO_UNIT; }
        bool isFinal() const OVERRIDE { return false; }
    };

    /** @brief Dynamically executed variable (of string data type).
     *
     * This is the base class for all built-in string script variables whose
     * variable content needs to be provided dynamically by executable native
     * code on each script variable access.
     */
    class VMDynStringVar : virtual public VMDynVar, virtual public VMStringExpr {
    public:
    };

    /** @brief Dynamically executed variable (of integer array data type).
     *
     * This is the base class for all built-in integer array script variables
     * whose variable content needs to be provided dynamically by executable
     * native code on each script variable access.
     */
    class VMDynIntArrayVar : virtual public VMDynVar, virtual public VMIntArrayExpr {
    public:
    };

    /** @brief Provider for built-in script functions and variables.
     *
     * Abstract base class defining the high-level interface for all classes
     * which add and implement built-in script functions and built-in script
     * variables.
     */
    class VMFunctionProvider {
    public:
        /**
         * Returns pointer to the built-in function with the given function
         * @a name, or NULL if there is no built-in function with that function
         * name.
         *
         * @param name - function name (i.e. "wait" or "message" or "exit", etc.)
         */
        virtual VMFunction* functionByName(const String& name) = 0;

        /**
         * Returns @c true if the passed built-in function is disabled and
         * should be ignored by the parser. This method is called by the
         * parser on preprocessor level for each built-in function call within
         * a script. Accordingly if this method returns @c true, then the
         * respective function call is completely filtered out on preprocessor
         * level, so that built-in function won't make into the result virtual
         * machine representation, nor would expressions of arguments passed to
         * that built-in function call be evaluated, nor would any check
         * regarding correct usage of the built-in function be performed.
         * In other words: a disabled function call ends up as a comment block.
         *
         * @param fn - built-in function to be checked
         * @param ctx - parser context at the position where the built-in
         *              function call is located within the script
         */
        virtual bool isFunctionDisabled(VMFunction* fn, VMParserContext* ctx) = 0;

        /**
         * Returns a variable name indexed map of all built-in script variables
         * which point to native "int" scalar (usually 32 bit) variables.
         */
        virtual std::map<String,VMIntPtr*> builtInIntVariables() = 0;

        /**
         * Returns a variable name indexed map of all built-in script integer
         * array variables with array element type "int8_t" (8 bit).
         */
        virtual std::map<String,VMInt8Array*> builtInIntArrayVariables() = 0;

        /**
         * Returns a variable name indexed map of all built-in constant script
         * variables of integer type, which never change their value at runtime.
         */
        virtual std::map<String,vmint> builtInConstIntVariables() = 0;

        /**
         * Returns a variable name indexed map of all built-in constant script
         * variables of real number (floating point) type, which never change
         * their value at runtime.
         */
        virtual std::map<String,vmfloat> builtInConstRealVariables() = 0;

        /**
         * Returns a variable name indexed map of all built-in dynamic variables,
         * which are not simply data stores, rather each one of them executes
         * natively to provide or alter the respective script variable data.
         */
        virtual std::map<String,VMDynVar*> builtInDynamicVariables() = 0;
    };

    /** @brief Execution state of a virtual machine.
     *
     * An instance of this abstract base class represents exactly one execution
     * state of a virtual machine. This encompasses most notably the VM
     * execution stack, and VM polyphonic variables. It does not contain global
     * variables. Global variables are contained in the VMParserContext object.
     * You might see a VMExecContext object as one virtual thread of the virtual
     * machine.
     *
     * In contrast to a VMParserContext, a VMExecContext is not tied to a
     * ScriptVM instance. Thus you can use a VMExecContext with different
     * ScriptVM instances, however not concurrently at the same time.
     *
     * @see VMParserContext
     */
    class VMExecContext {
    public:
        virtual ~VMExecContext() {}

        /**
         * In case the script was suspended for some reason, this method returns
         * the amount of microseconds before the script shall continue its
         * execution. Note that the virtual machine itself does never put its
         * own execution thread(s) to sleep. So the respective class (i.e. sampler
         * engine) which is using the virtual machine classes here, must take
         * care by itself about taking time stamps, determining the script
         * handlers that shall be put aside for the requested amount of
         * microseconds, indicated by this method by comparing the time stamps in
         * real-time, and to continue passing the respective handler to
         * ScriptVM::exec() as soon as its suspension exceeded, etc. Or in other
         * words: all classes in this directory never have an idea what time it
         * is.
         *
         * You should check the return value of ScriptVM::exec() to determine
         * whether the script was actually suspended before calling this method
         * here.
         *
         * @see ScriptVM::exec()
         */
        virtual vmint suspensionTimeMicroseconds() const = 0;

        /**
         * Causes all polyphonic variables to be reset to zero values. A
         * polyphonic variable is expected to be zero when entering a new event
         * handler instance. As an exception the values of polyphonic variables
         * shall only be preserved from an note event handler instance to its
         * correspending specific release handler instance. So in the latter
         * case the script author may pass custom data from the note handler to
         * the release handler, but only for the same specific note!
         */
        virtual void resetPolyphonicData() = 0;

        /**
         * Returns amount of virtual machine instructions which have been
         * performed the last time when this execution context was executing a
         * script. So in case you need the overall amount of instructions
         * instead, then you need to add them by yourself after each
         * ScriptVM::exec() call.
         */
        virtual size_t instructionsPerformed() const = 0;

        /**
         * Sends a signal to this script execution instance to abort its script
         * execution as soon as possible. This method is called i.e. when one
         * script execution instance intends to stop another script execution
         * instance.
         */
        virtual void signalAbort() = 0;

        /**
         * Copies the current entire execution state from this object to the
         * given object. So this can be used to "fork" a new script thread which
         * then may run independently with its own polyphonic data for instance.
         */
        virtual void forkTo(VMExecContext* ectx) const = 0;

        /**
         * In case the script called the built-in exit() function and passed a
         * value as argument to the exit() function, then this method returns
         * the value that had been passed as argument to the exit() function.
         * Otherwise if the exit() function has not been called by the script
         * or no argument had been passed to the exit() function, then this
         * method returns NULL instead.
         *
         * Currently this is only used for automated test cases against the
         * script engine, which return some kind of value in the individual
         * test case scripts to check their behaviour in automated way. There
         * is no purpose for this mechanism in production use. Accordingly this
         * exit result value is @b always completely ignored by the sampler
         * engines.
         *
         * Officially the built-in exit() function does not expect any arguments
         * to be passed to its function call, and by default this feature is
         * hence disabled and will yield in a parser error unless
         * ScriptVM::setExitResultEnabled() was explicitly set.
         *
         * @see ScriptVM::setExitResultEnabled()
         */
        virtual VMExpr* exitResult() = 0;
    };

    /** @brief Script callback for a certain event.
     *
     * Represents a script callback for a certain event, i.e.
     * "on note ... end on" code block.
     */
    class VMEventHandler {
    public:
        /**
         * Type of this event handler, which identifies its purpose. For example
         * for a "on note ... end on" script callback block,
         * @c VM_EVENT_HANDLER_NOTE would be returned here.
         */
        virtual VMEventHandlerType_t eventHandlerType() const = 0;

        /**
         * Name of the event handler which identifies its purpose. For example
         * for a "on note ... end on" script callback block, the name "note"
         * would be returned here.
         */
        virtual String eventHandlerName() const = 0;

        /**
         * Whether or not the event handler makes any use of so called
         * "polyphonic" variables.
         */
        virtual bool isPolyphonic() const = 0;
    };

    /**
     * Reflects the precise position and span of a specific code block within
     * a script. This is currently only used for the locations of commented
     * code blocks due to preprocessor statements, and for parser errors and
     * parser warnings.
     *
     * @see ParserIssue for code locations of parser errors and parser warnings
     *
     * @see VMParserContext::preprocessorComments() for locations of code which
     *      have been filtered out by preprocessor statements
     */
    struct CodeBlock {
        int firstLine; ///< The first line number of this code block within the script (indexed with 1 being the very first line).
        int lastLine; ///< The last line number of this code block within the script.
        int firstColumn; ///< The first column of this code block within the script (indexed with 1 being the very first column).
        int lastColumn; ///< The last column of this code block within the script.
    };

    /**
     * Encapsulates a noteworty parser issue. This encompasses the type of the
     * issue (either a parser error or parser warning), a human readable
     * explanation text of the error or warning and the location of the
     * encountered parser issue within the script.
     *
     * @see VMSourceToken for processing syntax highlighting instead.
     */
    struct ParserIssue : CodeBlock {
        String txt; ///< Human readable explanation text of the parser issue.
        ParserIssueType_t type; ///< Whether this issue is either a parser error or just a parser warning.

        /**
         * Print this issue out to the console (stdio).
         */
        inline void dump() {
            switch (type) {
                case PARSER_ERROR:
                    printf("[ERROR] line %d, column %d: %s\n", firstLine, firstColumn, txt.c_str());
                    break;
                case PARSER_WARNING:
                    printf("[Warning] line %d, column %d: %s\n", firstLine, firstColumn, txt.c_str());
                    break;
            }
        }

        /**
         * Returns true if this issue is a parser error. In this case the parsed
         * script may not be executed!
         */
        inline bool isErr() const { return type == PARSER_ERROR;   }

        /**
         * Returns true if this issue is just a parser warning. A parsed script
         * that only raises warnings may be executed if desired, however the
         * script may not behave exactly as intended by the script author.
         */
        inline bool isWrn() const { return type == PARSER_WARNING; }
    };

    /**
     * Convenience function used for converting an ExprType_t constant to a
     * string, i.e. for generating error message by the parser.
     */
    inline String typeStr(const ExprType_t& type) {
        switch (type) {
            case EMPTY_EXPR: return "empty";
            case INT_EXPR: return "integer";
            case INT_ARR_EXPR: return "integer array";
            case REAL_EXPR: return "real number";
            case REAL_ARR_EXPR: return "real number array";
            case STRING_EXPR: return "string";
            case STRING_ARR_EXPR: return "string array";
        }
        return "invalid";
    }

    /**
     * Returns @c true in case the passed data type is some array data type.
     */
    inline bool isArray(const ExprType_t& type) {
        return type == INT_ARR_EXPR || type == REAL_ARR_EXPR ||
               type == STRING_ARR_EXPR;
    }

    /**
     * Returns @c true in case the passed data type is some scalar number type
     * (i.e. not an array and not a string).
     */
    inline bool isNumber(const ExprType_t& type) {
        return type == INT_EXPR || type == REAL_EXPR;
    }

    /**
     * Convenience function used for converting an StdUnit_t constant to a
     * string, i.e. for generating error message by the parser.
     */
    inline String unitTypeStr(const StdUnit_t& type) {
        switch (type) {
            case VM_NO_UNIT: return "none";
            case VM_SECOND: return "seconds";
            case VM_HERTZ: return "Hz";
            case VM_BEL: return "Bel";
        }
        return "invalid";
    }

    /** @brief Virtual machine representation of a script.
     *
     * An instance of this abstract base class represents a parsed script,
     * translated into a virtual machine tree. You should first check if there
     * were any parser errors. If there were any parser errors, you should
     * refrain from executing the virtual machine. Otherwise if there were no
     * parser errors (i.e. only warnings), then you might access one of the
     * script's event handlers by i.e. calling eventHandlerByName() and pass the
     * respective event handler to the ScriptVM class (or to one of the ScriptVM
     * descendants) for execution.
     *
     * @see VMExecContext, ScriptVM
     */
    class VMParserContext {
    public:
        virtual ~VMParserContext() {}

        /**
         * Returns all noteworthy issues encountered when the script was parsed.
         * These are parser errors and parser warnings.
         */
        virtual std::vector<ParserIssue> issues() const = 0;

        /**
         * Same as issues(), but this method only returns parser errors.
         */
        virtual std::vector<ParserIssue> errors() const = 0;

        /**
         * Same as issues(), but this method only returns parser warnings.
         */
        virtual std::vector<ParserIssue> warnings() const = 0;

        /**
         * Returns all code blocks of the script which were filtered out by the
         * preprocessor.
         */
        virtual std::vector<CodeBlock> preprocessorComments() const = 0;

        /**
         * Returns the translated virtual machine representation of an event
         * handler block (i.e. "on note ... end on" code block) within the
         * parsed script. This translated representation of the event handler
         * can be executed by the virtual machine.
         *
         * @param index - index of the event handler within the script
         */
        virtual VMEventHandler* eventHandler(uint index) = 0;

        /**
         * Same as eventHandler(), but this method returns the event handler by
         * its name. So for a "on note ... end on" code block of the parsed
         * script you would pass "note" for argument @a name here.
         *
         * @param name - name of the event handler (i.e. "init", "note",
         *               "controller", "release")
         */
        virtual VMEventHandler* eventHandlerByName(const String& name) = 0;
    };

    class SourceToken;

    /** @brief Recognized token of a script's source code.
     *
     * Represents one recognized token of a script's source code, for example
     * a keyword, variable name, etc. and it provides further informations about
     * that particular token, i.e. the precise location (line and column) of the
     * token within the original script's source code.
     *
     * This class is not actually used by the sampler itself. It is rather
     * provided for external script editor applications. Primary purpose of
     * this class is syntax highlighting for external script editors.
     *
     * @see ParserIssue for processing compile errors and warnings instead.
     */
    class VMSourceToken {
    public:
        VMSourceToken();
        VMSourceToken(SourceToken* ct);
        VMSourceToken(const VMSourceToken& other);
        virtual ~VMSourceToken();

        // original text of this token as it is in the script's source code
        String text() const;

        // position of token in script
        int firstLine() const; ///< First line this source token is located at in script source code (indexed with 0 being the very first line). Most source code tokens are not spanning over multiple lines, the only current exception are comments, in the latter case you need to process text() to get the last line and last column for the comment.
        int firstColumn() const; ///< First column on the first line this source token is located at in script source code (indexed with 0 being the very first column). To get the length of this token use text().length().

        // base types
        bool isEOF() const; ///< Returns true in case this source token represents the end of the source code file.
        bool isNewLine() const; ///< Returns true in case this source token represents a line feed character (i.e. "\n" on Unix systems).
        bool isKeyword() const; ///< Returns true in case this source token represents a language keyword (i.e. "while", "function", "declare", "on", etc.).
        bool isVariableName() const; ///< Returns true in case this source token represents a variable name (i.e. "$someIntVariable", "%someArrayVariable", "\@someStringVariable"). @see isIntegerVariable(), isStringVariable(), isArrayVariable() for the precise variable type.
        bool isIdentifier() const; ///< Returns true in case this source token represents an identifier, which currently always means a function name.
        bool isNumberLiteral() const; ///< Returns true in case this source token represents a number literal (i.e. 123).
        bool isStringLiteral() const; ///< Returns true in case this source token represents a string literal (i.e. "Some text").
        bool isComment() const; ///< Returns true in case this source token represents a source code comment.
        bool isPreprocessor() const; ///< Returns true in case this source token represents a preprocessor statement.
        bool isMetricPrefix() const;
        bool isStdUnit() const;
        bool isOther() const; ///< Returns true in case this source token represents anything else not covered by the token types mentioned above.

        // extended types
        bool isIntegerVariable() const; ///< Returns true in case this source token represents an integer variable name (i.e. "$someIntVariable").
        bool isRealVariable() const; ///< Returns true in case this source token represents a floating point variable name (i.e. "~someRealVariable").
        bool isStringVariable() const; ///< Returns true in case this source token represents an string variable name (i.e. "\@someStringVariable").
        bool isIntArrayVariable() const; ///< Returns true in case this source token represents an integer array variable name (i.e. "%someArrayVariable").
        bool isRealArrayVariable() const; ///< Returns true in case this source token represents a real number array variable name (i.e. "?someArrayVariable").
        bool isArrayVariable() const DEPRECATED_API; ///< Returns true in case this source token represents an @b integer array variable name (i.e. "%someArrayVariable"). @deprecated This method will be removed, use isIntArrayVariable() instead.
        bool isEventHandlerName() const; ///< Returns true in case this source token represents an event handler name (i.e. "note", "release", "controller").

        VMSourceToken& operator=(const VMSourceToken& other);

    private:
        SourceToken* m_token;
    };

} // namespace LinuxSampler

#endif // LS_INSTR_SCRIPT_PARSER_COMMON_H
