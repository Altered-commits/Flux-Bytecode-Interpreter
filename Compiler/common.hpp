//All the common stuff shared by files in the compiler folder
#ifndef UNNAMED_COMMON_DEFINES_HPP
#define UNNAMED_COMMON_DEFINES_HPP

//Used by Parser and Ilgen, for Continue, Break and Return keywords
//Constants to be bitwise'd
#define IS_LOOP         1
#define IS_FOR_LOOP     2
#define IS_USING_SYMTBL 4
#define IS_FUNCTION     8

#define CBR_PARAMS_CHECK_CONDITION(var, cond) (var & cond)

#endif