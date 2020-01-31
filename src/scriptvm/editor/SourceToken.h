/*
 * Copyright (c) 2015-2020 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef CRUDEBYTE_CODE_TOKEN_H
#define CRUDEBYTE_CODE_TOKEN_H

#include "../../common/global.h"

namespace LinuxSampler {

class SourceToken {
public:
    enum BaseType_t {
        // most fundamental tokens
        END_OF_FILE = 0,
        NEW_LINE,
        // base token types
        KEYWORD,
        VARIABLE_NAME,
        IDENTIFIER, // i.e. function name
        NUMBER_LITERAL,
        STRING_LITERAL,
        COMMENT,
        PREPROCESSOR,
        METRIC_PREFIX,
        STANDARD_UNIT,
        OTHER, // anything else not classified here
    };

    enum ExtType_t {
        NO_EXT, ///< no extended type available for this token
        INTEGER_VARIABLE,
        REAL_VARIABLE,
        STRING_VARIABLE,
        INTEGER_ARRAY_VARIABLE,
        REAL_ARRAY_VARIABLE,
        EVENT_HANDLER_NAME // only NKSP language
    };

    SourceToken() : baseType(END_OF_FILE), extType(NO_EXT), line(0), column(0),
                    offset(0), length(0)
    {
    }

    SourceToken(BaseType_t t, String s = "") : baseType(t), extType(NO_EXT),
                                               txt(s), line(0), column(0),
                                               offset(0), length(0)
    {
    }

    SourceToken(ExtType_t t, String s = "") : baseType(OTHER), extType(t),
                                              txt(s), line(0), column(0),
                                              offset(0), length(0)
    {
        switch (t) {
            case NO_EXT: baseType = OTHER; break;
            case INTEGER_VARIABLE: baseType = VARIABLE_NAME; break;
            case REAL_VARIABLE: baseType = VARIABLE_NAME; break;
            case STRING_VARIABLE: baseType = VARIABLE_NAME; break;
            case INTEGER_ARRAY_VARIABLE: baseType = VARIABLE_NAME; break;
            case REAL_ARRAY_VARIABLE: baseType = VARIABLE_NAME; break;
            case EVENT_HANDLER_NAME: baseType = IDENTIFIER; break;
        }
    }

    BaseType_t baseType;
    ExtType_t extType;
    String txt;
    int line;
    int column;
    int offset;
    int length;

    String text() const { return txt; }
    int firstLine() const { return line; }
    int firstColumn() const { return column; }
    int firstByte() const { return offset; }
    int lengthBytes() const { return length; }

    // base types
    bool isEOF() const { return baseType == END_OF_FILE; }
    bool isNewLine() const { return baseType == NEW_LINE; }
    bool isKeyword() const { return baseType == KEYWORD; }
    bool isVariableName() const { return baseType == VARIABLE_NAME; }
    bool isIdentifier() const { return baseType == IDENTIFIER; }
    bool isNumberLiteral() const { return baseType == NUMBER_LITERAL; }
    bool isStringLiteral() const { return baseType == STRING_LITERAL; }
    bool isComment() const { return baseType == COMMENT; }
    bool isPreprocessor() const { return baseType == PREPROCESSOR; }
    bool isMetricPrefix() const { return baseType == METRIC_PREFIX; }
    bool isStdUnit() const { return baseType == STANDARD_UNIT; }
    bool isOther() const { return baseType == OTHER; }

    // extended types
    bool isIntegerVariable() const { return extType == INTEGER_VARIABLE; }
    bool isRealVariable() const { return extType == REAL_VARIABLE; }
    bool isStringVariable() const { return extType == STRING_VARIABLE; }
    bool isIntegerArrayVariable() const { return extType == INTEGER_ARRAY_VARIABLE; }
    bool isRealArrayVariable() const { return extType == REAL_ARRAY_VARIABLE; }
    bool isEventHandlerName() const { return extType == EVENT_HANDLER_NAME; }

    operator bool() const { return baseType != END_OF_FILE; }

    bool equalsType(const SourceToken& other) const {
        return baseType == other.baseType && extType == other.extType;
    }
};

} // namespace LinuxSampler

#endif // CRUDEBYTE_CODE_TOKEN_H
