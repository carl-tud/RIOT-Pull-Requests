#define COMPILER_HINTS_H 
#define DECLARE_CONSTANT(identifier,const_expr) WITHOUT_PEDANTIC(enum { identifier = const_expr };)
#define DO_PRAGMA(x) DO_PRAGMA____(x)
#define DO_PRAGMA_(x) _Pragma (#x)
#define DO_PRAGMA__(x) DO_PRAGMA_(x)
#define DO_PRAGMA___(x) DO_PRAGMA__(x)
#define DO_PRAGMA____(x) DO_PRAGMA___(x)
#define INT16_C(v) (v)
#define INT16_MAX 32767
#define INT16_MIN -32768
#define INT32_C(v) (v)
#define INT32_MAX 2147483647
#define INT32_MIN (-INT32_MAX-1)
#define INT64_C(v) (v ## LL)
#define INT64_MAX 9223372036854775807LL
#define INT64_MIN (-INT64_MAX-1)
#define INT8_C(v) (v)
#define INT8_MAX 127
#define INT8_MIN -128
#define INTMAX_C(v) (v ## L)
#define INTMAX_MAX INTMAX_C(9223372036854775807)
#define INTMAX_MIN (-INTMAX_MAX-1)
#define INTPTR_MAX 9223372036854775807L
#define INTPTR_MIN (-INTPTR_MAX-1)
#define INT_FAST16_MAX INT16_MAX
#define INT_FAST16_MIN INT16_MIN
#define INT_FAST32_MAX INT32_MAX
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST64_MAX INT64_MAX
#define INT_FAST64_MIN INT64_MIN
#define INT_FAST8_MAX INT8_MAX
#define INT_FAST8_MIN INT8_MIN
#define INT_LEAST16_MAX INT16_MAX
#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST32_MAX INT32_MAX
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST64_MAX INT64_MAX
#define INT_LEAST64_MIN INT64_MIN
#define INT_LEAST8_MAX INT8_MAX
#define INT_LEAST8_MIN INT8_MIN
#define IS_CT_CONSTANT(expr) __builtin_constant_p(expr)
#define MAC_OS_VERSION_11_0 __MAC_11_0
#define MAC_OS_VERSION_11_1 __MAC_11_1
#define MAC_OS_VERSION_11_3 __MAC_11_3
#define MAC_OS_VERSION_11_4 __MAC_11_4
#define MAC_OS_VERSION_11_5 __MAC_11_5
#define MAC_OS_VERSION_11_6 __MAC_11_6
#define MAC_OS_VERSION_12_0 __MAC_12_0
#define MAC_OS_VERSION_12_1 __MAC_12_1
#define MAC_OS_VERSION_12_2 __MAC_12_2
#define MAC_OS_VERSION_12_3 __MAC_12_3
#define MAC_OS_VERSION_12_4 __MAC_12_4
#define MAC_OS_VERSION_12_5 __MAC_12_5
#define MAC_OS_VERSION_12_6 __MAC_12_6
#define MAC_OS_VERSION_12_7 __MAC_12_7
#define MAC_OS_VERSION_13_0 __MAC_13_0
#define MAC_OS_VERSION_13_1 __MAC_13_1
#define MAC_OS_VERSION_13_2 __MAC_13_2
#define MAC_OS_VERSION_13_3 __MAC_13_3
#define MAC_OS_VERSION_13_4 __MAC_13_4
#define MAC_OS_VERSION_13_5 __MAC_13_5
#define MAC_OS_VERSION_13_6 __MAC_13_6
#define MAC_OS_VERSION_14_0 __MAC_14_0
#define MAC_OS_VERSION_14_1 __MAC_14_1
#define MAC_OS_VERSION_14_2 __MAC_14_2
#define MAC_OS_VERSION_14_3 __MAC_14_3
#define MAC_OS_VERSION_14_4 __MAC_14_4
#define MAC_OS_VERSION_14_5 __MAC_14_5
#define MAC_OS_VERSION_15_0 __MAC_15_0
#define MAC_OS_VERSION_15_1 __MAC_15_1
#define MAC_OS_VERSION_15_2 __MAC_15_2
#define MAC_OS_X_VERSION_10_0 __MAC_10_0
#define MAC_OS_X_VERSION_10_1 __MAC_10_1
#define MAC_OS_X_VERSION_10_10 __MAC_10_10
#define MAC_OS_X_VERSION_10_10_2 __MAC_10_10_2
#define MAC_OS_X_VERSION_10_10_3 __MAC_10_10_3
#define MAC_OS_X_VERSION_10_11 __MAC_10_11
#define MAC_OS_X_VERSION_10_11_2 __MAC_10_11_2
#define MAC_OS_X_VERSION_10_11_3 __MAC_10_11_3
#define MAC_OS_X_VERSION_10_11_4 __MAC_10_11_4
#define MAC_OS_X_VERSION_10_12 __MAC_10_12
#define MAC_OS_X_VERSION_10_12_1 __MAC_10_12_1
#define MAC_OS_X_VERSION_10_12_2 __MAC_10_12_2
#define MAC_OS_X_VERSION_10_12_4 __MAC_10_12_4
#define MAC_OS_X_VERSION_10_13 __MAC_10_13
#define MAC_OS_X_VERSION_10_13_1 __MAC_10_13_1
#define MAC_OS_X_VERSION_10_13_2 __MAC_10_13_2
#define MAC_OS_X_VERSION_10_13_4 __MAC_10_13_4
#define MAC_OS_X_VERSION_10_14 __MAC_10_14
#define MAC_OS_X_VERSION_10_14_1 __MAC_10_14_1
#define MAC_OS_X_VERSION_10_14_4 __MAC_10_14_4
#define MAC_OS_X_VERSION_10_14_5 __MAC_10_14_5
#define MAC_OS_X_VERSION_10_14_6 __MAC_10_14_6
#define MAC_OS_X_VERSION_10_15 __MAC_10_15
#define MAC_OS_X_VERSION_10_15_1 __MAC_10_15_1
#define MAC_OS_X_VERSION_10_15_4 __MAC_10_15_4
#define MAC_OS_X_VERSION_10_16 __MAC_10_16
#define MAC_OS_X_VERSION_10_2 __MAC_10_2
#define MAC_OS_X_VERSION_10_3 __MAC_10_3
#define MAC_OS_X_VERSION_10_4 __MAC_10_4
#define MAC_OS_X_VERSION_10_5 __MAC_10_5
#define MAC_OS_X_VERSION_10_6 __MAC_10_6
#define MAC_OS_X_VERSION_10_7 __MAC_10_7
#define MAC_OS_X_VERSION_10_8 __MAC_10_8
#define MAC_OS_X_VERSION_10_9 __MAC_10_9
#define MAYBE_UNUSED __attribute__((unused))
#define NORETURN __attribute__((noreturn))
#define NO_SANITIZE_ARRAY __attribute__((no_sanitize("address")))
#define PRIX16 "hX"
#define PRIX32 "X"
#define PRIX64 __PRI_64_LENGTH_MODIFIER__ "X"
#define PRIX8 __PRI_8_LENGTH_MODIFIER__ "X"
#define PRIXFAST16 PRIX16
#define PRIXFAST32 PRIX32
#define PRIXFAST64 PRIX64
#define PRIXFAST8 PRIX8
#define PRIXLEAST16 PRIX16
#define PRIXLEAST32 PRIX32
#define PRIXLEAST64 PRIX64
#define PRIXLEAST8 PRIX8
#define PRIXMAX __PRI_MAX_LENGTH_MODIFIER__ "X"
#define PRIXPTR "lX"
#define PRId16 "hd"
#define PRId32 "d"
#define PRId64 __PRI_64_LENGTH_MODIFIER__ "d"
#define PRId8 __PRI_8_LENGTH_MODIFIER__ "d"
#define PRIdFAST16 PRId16
#define PRIdFAST32 PRId32
#define PRIdFAST64 PRId64
#define PRIdFAST8 PRId8
#define PRIdLEAST16 PRId16
#define PRIdLEAST32 PRId32
#define PRIdLEAST64 PRId64
#define PRIdLEAST8 PRId8
#define PRIdMAX __PRI_MAX_LENGTH_MODIFIER__ "d"
#define PRIdPTR "ld"
#define PRIi16 "hi"
#define PRIi32 "i"
#define PRIi64 __PRI_64_LENGTH_MODIFIER__ "i"
#define PRIi8 __PRI_8_LENGTH_MODIFIER__ "i"
#define PRIiFAST16 PRIi16
#define PRIiFAST32 PRIi32
#define PRIiFAST64 PRIi64
#define PRIiFAST8 PRIi8
#define PRIiLEAST16 PRIi16
#define PRIiLEAST32 PRIi32
#define PRIiLEAST64 PRIi64
#define PRIiLEAST8 PRIi8
#define PRIiMAX __PRI_MAX_LENGTH_MODIFIER__ "i"
#define PRIiPTR "li"
#define PRIo16 "ho"
#define PRIo32 "o"
#define PRIo64 __PRI_64_LENGTH_MODIFIER__ "o"
#define PRIo8 __PRI_8_LENGTH_MODIFIER__ "o"
#define PRIoFAST16 PRIo16
#define PRIoFAST32 PRIo32
#define PRIoFAST64 PRIo64
#define PRIoFAST8 PRIo8
#define PRIoLEAST16 PRIo16
#define PRIoLEAST32 PRIo32
#define PRIoLEAST64 PRIo64
#define PRIoLEAST8 PRIo8
#define PRIoMAX __PRI_MAX_LENGTH_MODIFIER__ "o"
#define PRIoPTR "lo"
#define PRIu16 "hu"
#define PRIu32 "u"
#define PRIu64 __PRI_64_LENGTH_MODIFIER__ "u"
#define PRIu8 __PRI_8_LENGTH_MODIFIER__ "u"
#define PRIuFAST16 PRIu16
#define PRIuFAST32 PRIu32
#define PRIuFAST64 PRIu64
#define PRIuFAST8 PRIu8
#define PRIuLEAST16 PRIu16
#define PRIuLEAST32 PRIu32
#define PRIuLEAST64 PRIu64
#define PRIuLEAST8 PRIu8
#define PRIuMAX __PRI_MAX_LENGTH_MODIFIER__ "u"
#define PRIuPTR "lu"
#define PRIx16 "hx"
#define PRIx32 "x"
#define PRIx64 __PRI_64_LENGTH_MODIFIER__ "x"
#define PRIx8 __PRI_8_LENGTH_MODIFIER__ "x"
#define PRIxFAST16 PRIx16
#define PRIxFAST32 PRIx32
#define PRIxFAST64 PRIx64
#define PRIxFAST8 PRIx8
#define PRIxLEAST16 PRIx16
#define PRIxLEAST32 PRIx32
#define PRIxLEAST64 PRIx64
#define PRIxLEAST8 PRIx8
#define PRIxMAX __PRI_MAX_LENGTH_MODIFIER__ "x"
#define PRIxPTR "lx"
#define PTRDIFF_MAX INTMAX_MAX
#define PTRDIFF_MIN INTMAX_MIN
#define PURE __attribute__((pure))
#define RSIZE_MAX (SIZE_MAX >> 1)
#define SCNd16 "hd"
#define SCNd32 "d"
#define SCNd64 __SCN_64_LENGTH_MODIFIER__ "d"
#define SCNd8 __PRI_8_LENGTH_MODIFIER__ "d"
#define SCNdFAST16 SCNd16
#define SCNdFAST32 SCNd32
#define SCNdFAST64 SCNd64
#define SCNdFAST8 SCNd8
#define SCNdLEAST16 SCNd16
#define SCNdLEAST32 SCNd32
#define SCNdLEAST64 SCNd64
#define SCNdLEAST8 SCNd8
#define SCNdMAX __SCN_MAX_LENGTH_MODIFIER__ "d"
#define SCNdPTR "ld"
#define SCNi16 "hi"
#define SCNi32 "i"
#define SCNi64 __SCN_64_LENGTH_MODIFIER__ "i"
#define SCNi8 __PRI_8_LENGTH_MODIFIER__ "i"
#define SCNiFAST16 SCNi16
#define SCNiFAST32 SCNi32
#define SCNiFAST64 SCNi64
#define SCNiFAST8 SCNi8
#define SCNiLEAST16 SCNi16
#define SCNiLEAST32 SCNi32
#define SCNiLEAST64 SCNi64
#define SCNiLEAST8 SCNi8
#define SCNiMAX __SCN_MAX_LENGTH_MODIFIER__ "i"
#define SCNiPTR "li"
#define SCNo16 "ho"
#define SCNo32 "o"
#define SCNo64 __SCN_64_LENGTH_MODIFIER__ "o"
#define SCNo8 __PRI_8_LENGTH_MODIFIER__ "o"
#define SCNoFAST16 SCNo16
#define SCNoFAST32 SCNo32
#define SCNoFAST64 SCNo64
#define SCNoFAST8 SCNo8
#define SCNoLEAST16 SCNo16
#define SCNoLEAST32 SCNo32
#define SCNoLEAST64 SCNo64
#define SCNoLEAST8 SCNo8
#define SCNoMAX __SCN_MAX_LENGTH_MODIFIER__ "o"
#define SCNoPTR "lo"
#define SCNu16 "hu"
#define SCNu32 "u"
#define SCNu64 __SCN_64_LENGTH_MODIFIER__ "u"
#define SCNu8 __PRI_8_LENGTH_MODIFIER__ "u"
#define SCNuFAST16 SCNu16
#define SCNuFAST32 SCNu32
#define SCNuFAST64 SCNu64
#define SCNuFAST8 SCNu8
#define SCNuLEAST16 SCNu16
#define SCNuLEAST32 SCNu32
#define SCNuLEAST64 SCNu64
#define SCNuLEAST8 SCNu8
#define SCNuMAX __SCN_MAX_LENGTH_MODIFIER__ "u"
#define SCNuPTR "lu"
#define SCNx16 "hx"
#define SCNx32 "x"
#define SCNx64 __SCN_64_LENGTH_MODIFIER__ "x"
#define SCNx8 __PRI_8_LENGTH_MODIFIER__ "x"
#define SCNxFAST16 SCNx16
#define SCNxFAST32 SCNx32
#define SCNxFAST64 SCNx64
#define SCNxFAST8 SCNx8
#define SCNxLEAST16 SCNx16
#define SCNxLEAST32 SCNx32
#define SCNxLEAST64 SCNx64
#define SCNxLEAST8 SCNx8
#define SCNxMAX __SCN_MAX_LENGTH_MODIFIER__ "x"
#define SCNxPTR "lx"
#define SIG_ATOMIC_MAX INT32_MAX
#define SIG_ATOMIC_MIN INT32_MIN
#define SIZE_MAX UINTPTR_MAX
#define UINT16_C(v) (v)
#define UINT16_MAX 65535
#define UINT32_C(v) (v ## U)
#define UINT32_MAX 4294967295U
#define UINT64_C(v) (v ## ULL)
#define UINT64_MAX 18446744073709551615ULL
#define UINT8_C(v) (v)
#define UINT8_MAX 255
#define UINTMAX_C(v) (v ## UL)
#define UINTMAX_MAX UINTMAX_C(18446744073709551615)
#define UINTPTR_MAX 18446744073709551615UL
#define UINT_FAST16_MAX UINT16_MAX
#define UINT_FAST32_MAX UINT32_MAX
#define UINT_FAST64_MAX UINT64_MAX
#define UINT_FAST8_MAX UINT8_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define UINT_LEAST64_MAX UINT64_MAX
#define UINT_LEAST8_MAX UINT8_MAX
#define UNREACHABLE() __builtin_unreachable()
#define WCHAR_MAX __WCHAR_MAX__
#define WCHAR_MIN (-WCHAR_MAX-1)
#define WINT_MAX INT32_MAX
#define WINT_MIN INT32_MIN
#define WITHOUT_PEDANTIC(...) _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wpedantic\"") __VA_ARGS__ _Pragma("GCC diagnostic pop")
#define XFA(type,xfa_name,prio,...) _XFA_ELEMENT(type, xfa_name, 5_ ## prio, __VA_ARGS__)
#define XFA_ADD_PTR(xfa_name,prio,element_name,entry) _XFA_CONST(__typeof__(entry), xfa_name, 5_ ## prio, element_name, entry) = entry
#define XFA_CONST(type,xfa_name,prio,...) _XFA_ELEMENT_CONST(type, xfa_name, 5_ ## prio, __VA_ARGS__)
#define XFA_H 
#define XFA_INIT(type,xfa_name) _XFA_INIT(type, xfa_name)
#define XFA_INIT_CONST(type,xfa_name) _XFA_INIT_CONST(type, xfa_name)
#define XFA_LEN(type,xfa_name) (((uintptr_t)xfa_name ## _end - (uintptr_t)xfa_name) / sizeof(type))
#define XFA_USE(type,xfa_name) _XFA_USE(type, xfa_name)
#define XFA_USE_CONST(type,xfa_name) _XFA_USE_CONST(type, xfa_name)
#define _ASSERT_H_ 
#define _BSD_ARM__TYPES_H_ 
#define _BSD_MACHINE__TYPES_H_ 
#define _CDEFS_H_ 
#define _DARWIN_FEATURE_64_BIT_INODE 1
#define _DARWIN_FEATURE_ONLY_64_BIT_INODE 1
#define _DARWIN_FEATURE_ONLY_UNIX_CONFORMANCE 1
#define _DARWIN_FEATURE_ONLY_VERS_1050 1
#define _DARWIN_FEATURE_UNIX_CONFORMANCE 3
#define _FORTIFY_SOURCE 2
#define _IF_EMPTY(if_empty,if_not_empty,...) _IF_EMPTY_HELPER(__VA_OPT__(,) if_not_empty, if_empty)
#define _IF_EMPTY_HELPER(_0,_1,...) _1
#define _INT16_T 
#define _INT32_T 
#define _INT64_T 
#define _INT8_T 
#define _INTMAX_T 
#define _INTPTR_T 
#define _INTTYPES_H_ 
#define _LP64 1
#define _STDINT_H_ 
#define _SYS__PTHREAD_TYPES_H_ 
#define _SYS__TYPES_H_ 
#define _UINT16_T 
#define _UINT32_T 
#define _UINT64_T 
#define _UINT8_T 
#define _UINTMAX_T 
#define _UINTPTR_T 
#define _WCHAR_T 
#define _XFA_ALIAS(old,new) DO_PRAGMA(redefine_extname old new)
#define _XFA_ELEMENT(type,xfa_name,prio,element_name) _IF_EMPTY( _XFA_ERROR("Element of cross-file array '" #xfa_name "' " "must have an identifier (4th argument) on macOS."), _XFA_ALIAS(element_name, _XFA_ID(xfa_name, prio, element_name)), element_name) __attribute__((used)) _Alignas(type) type element_name
#define _XFA_ELEMENT_CONST(type,xfa_name,prio,element_name) _XFA_ELEMENT(type, xfa_name, prio, element_name)
#define _XFA_ERROR(text) DO_PRAGMA(message text)
#define _XFA_ID(xfa_name,prio,element_name) __xfa__ ## $ ## xfa_name ## $ ## prio ## $ ## element_name
#define _XFA_INIT(type,xfa_name) _XFA_ALIAS(xfa_name, _XFA_ID(xfa_name, 0, __start__)) extern type xfa_name [] __attribute__((weak_import)); _XFA_ALIAS(xfa_name ## _end, _XFA_ID(xfa_name, 9, __end__)) __attribute__((section("__DATA,__data"), used)) type xfa_name ## _end [] = {0};
#define _XFA_INIT_CONST(type,xfa_name) _XFA_INIT(type, xfa_name)
#define _XFA_USE(type,xfa_name) _XFA_ALIAS(xfa_name, _XFA_ID(xfa_name, 0, __start__)) extern type xfa_name [] __attribute__((weak_import)); _XFA_ALIAS(xfa_name ## _end, _XFA_ID(xfa_name, 9, __end__)) extern type xfa_name ## _end [];
#define _XFA_USE_CONST(type,xfa_name) _XFA_USE(type, xfa_name)
#define __AARCH64EL__ 1
#define __AARCH64_CMODEL_SMALL__ 1
#define __AARCH64_SIMD__ 1
#define __API_A(x) __attribute__((availability(__API_AVAILABLE_PLATFORM_##x)))
#define __API_APPLY_TO any(record, enum, enum_constant, function, objc_method, objc_category, objc_protocol, objc_interface, objc_property, type_alias, variable, field)
#define __API_AVAILABLE(...) __API_AVAILABLE_GET_MACRO(__VA_ARGS__,__API_AVAILABLE8,__API_AVAILABLE7,__API_AVAILABLE6,__API_AVAILABLE5,__API_AVAILABLE4,__API_AVAILABLE3,__API_AVAILABLE2,__API_AVAILABLE1,__API_AVAILABLE0,0)(__VA_ARGS__)
#define __API_AVAILABLE0(arg0) __API_A(arg0)
#define __API_AVAILABLE2(arg0,arg1,arg2) __API_A(arg0) __API_A(arg1) __API_A(arg2)
#define __API_AVAILABLE4(arg0,arg1,arg2,arg3,arg4) __API_A(arg0) __API_A(arg1) __API_A(arg2) __API_A(arg3) __API_A(arg4)
#define __API_AVAILABLE6(arg0,arg1,arg2,arg3,arg4,arg5,arg6) __API_A(arg0) __API_A(arg1) __API_A(arg2) __API_A(arg3) __API_A(arg4) __API_A(arg5) __API_A(arg6)
#define __API_AVAILABLE8(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8) __API_A(arg0) __API_A(arg1) __API_A(arg2) __API_A(arg3) __API_A(arg4) __API_A(arg5) __API_A(arg6) __API_A(arg7) __API_A(arg8)
#define __API_AVAILABLE_BEGIN(...) _Pragma("clang attribute push") __API_AVAILABLE_BEGIN_GET_MACRO(__VA_ARGS__,__API_AVAILABLE_BEGIN8,__API_AVAILABLE_BEGIN7,__API_AVAILABLE_BEGIN6,__API_AVAILABLE_BEGIN5,__API_AVAILABLE_BEGIN4,__API_AVAILABLE_BEGIN3,__API_AVAILABLE_BEGIN2,__API_AVAILABLE_BEGIN1,__API_AVAILABLE_BEGIN0,0)(__VA_ARGS__)
#define __API_AVAILABLE_BEGIN0(arg0) __API_A_BEGIN(arg0)
#define __API_AVAILABLE_BEGIN2(arg0,arg1,arg2) __API_A_BEGIN(arg0) __API_A_BEGIN(arg1) __API_A_BEGIN(arg2)
#define __API_AVAILABLE_BEGIN4(arg0,arg1,arg2,arg3,arg4) __API_A_BEGIN(arg0) __API_A_BEGIN(arg1) __API_A_BEGIN(arg2) __API_A_BEGIN(arg3) __API_A_BEGIN(arg4)
#define __API_AVAILABLE_BEGIN6(arg0,arg1,arg2,arg3,arg4,arg5,arg6) __API_A_BEGIN(arg0) __API_A_BEGIN(arg1) __API_A_BEGIN(arg2) __API_A_BEGIN(arg3) __API_A_BEGIN(arg4) __API_A_BEGIN(arg5) __API_A_BEGIN(arg6)
#define __API_AVAILABLE_BEGIN8(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8) __API_A_BEGIN(arg0) __API_A_BEGIN(arg1) __API_A_BEGIN(arg2) __API_A_BEGIN(arg3) __API_A_BEGIN(arg4) __API_A_BEGIN(arg5) __API_A_BEGIN(arg6) __API_A_BEGIN(arg7) __API_A_BEGIN(arg8)
#define __API_AVAILABLE_PLATFORM_driverkit(x) driverkit,introduced=x
#define __API_AVAILABLE_PLATFORM_ios(x) ios,introduced=x
#define __API_AVAILABLE_PLATFORM_macCatalyst(x) macCatalyst,introduced=x
#define __API_AVAILABLE_PLATFORM_macos(x) macos,introduced=x
#define __API_AVAILABLE_PLATFORM_tvos(x) tvos,introduced=x
#define __API_AVAILABLE_PLATFORM_xros(x) visionos,introduced=x
#define __API_A_BEGIN(x) _Pragma(__API_RANGE_STRINGIFY (clang attribute (__attribute__((availability(__API_AVAILABLE_PLATFORM_##x))), apply_to = __API_APPLY_TO)))
#define __API_D(msg,x) __attribute__((availability(__API_DEPRECATED_PLATFORM_##x,message=msg)))
#define __API_DEPRECATED(...) __API_DEPRECATED_MSG_GET_MACRO(__VA_ARGS__,__API_DEPRECATED_MSG8,__API_DEPRECATED_MSG7,__API_DEPRECATED_MSG6,__API_DEPRECATED_MSG5,__API_DEPRECATED_MSG4,__API_DEPRECATED_MSG3,__API_DEPRECATED_MSG2,__API_DEPRECATED_MSG1,__API_DEPRECATED_MSG0,0,0)(__VA_ARGS__)
#define __API_DEPRECATED_BEGIN(...) _Pragma("clang attribute push") __API_DEPRECATED_BEGIN_GET_MACRO(__VA_ARGS__,__API_DEPRECATED_BEGIN8,__API_DEPRECATED_BEGIN7,__API_DEPRECATED_BEGIN6,__API_DEPRECATED_BEGIN5,__API_DEPRECATED_BEGIN4,__API_DEPRECATED_BEGIN3,__API_DEPRECATED_BEGIN2,__API_DEPRECATED_BEGIN1,__API_DEPRECATED_BEGIN0,0,0)(__VA_ARGS__)
#define __API_DEPRECATED_BEGIN0(msg,arg0) __API_D_BEGIN(msg,arg0)
#define __API_DEPRECATED_BEGIN2(msg,arg0,arg1,arg2) __API_D_BEGIN(msg,arg0) __API_D_BEGIN(msg,arg1) __API_D_BEGIN(msg,arg2)
#define __API_DEPRECATED_BEGIN4(msg,arg0,arg1,arg2,arg3,arg4) __API_D_BEGIN(msg,arg0) __API_D_BEGIN(msg,arg1) __API_D_BEGIN(msg,arg2) __API_D_BEGIN(msg,arg3) __API_D_BEGIN(msg,arg4)
#define __API_DEPRECATED_BEGIN6(msg,arg0,arg1,arg2,arg3,arg4,arg5,arg6) __API_D_BEGIN(msg,arg0) __API_D_BEGIN(msg,arg1) __API_D_BEGIN(msg,arg2) __API_D_BEGIN(msg,arg3) __API_D_BEGIN(msg,arg4) __API_D_BEGIN(msg,arg5) __API_D_BEGIN(msg,arg6)
#define __API_DEPRECATED_BEGIN8(msg,arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8) __API_D_BEGIN(msg,arg0) __API_D_BEGIN(msg,arg1) __API_D_BEGIN(msg,arg2) __API_D_BEGIN(msg,arg3) __API_D_BEGIN(msg,arg4) __API_D_BEGIN(msg,arg5) __API_D_BEGIN(msg,arg6) __API_D_BEGIN(msg,arg7) __API_D_BEGIN(msg,arg8)
#define __API_DEPRECATED_MSG0(msg,arg0) __API_D(msg,arg0)
#define __API_DEPRECATED_MSG2(msg,arg0,arg1,arg2) __API_D(msg,arg0) __API_D(msg,arg1) __API_D(msg,arg2)
#define __API_DEPRECATED_MSG4(msg,arg0,arg1,arg2,arg3,arg4) __API_D(msg,arg0) __API_D(msg,arg1) __API_D(msg,arg2) __API_D(msg,arg3) __API_D(msg,arg4)
#define __API_DEPRECATED_MSG6(msg,arg0,arg1,arg2,arg3,arg4,arg5,arg6) __API_D(msg,arg0) __API_D(msg,arg1) __API_D(msg,arg2) __API_D(msg,arg3) __API_D(msg,arg4) __API_D(msg,arg5) __API_D(msg,arg6)
#define __API_DEPRECATED_MSG8(msg,arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8) __API_D(msg,arg0) __API_D(msg,arg1) __API_D(msg,arg2) __API_D(msg,arg3) __API_D(msg,arg4) __API_D(msg,arg5) __API_D(msg,arg6) __API_D(msg,arg7) __API_D(msg,arg8)
#define __API_DEPRECATED_PLATFORM_macCatalyst(x,y) macCatalyst,introduced=x,deprecated=y
#define __API_DEPRECATED_PLATFORM_macosx(x,y) macos,introduced=x,deprecated=y
#define __API_DEPRECATED_PLATFORM_visionos(x,y) visionos,introduced=x,deprecated=y
#define __API_DEPRECATED_PLATFORM_watchos(x,y) watchos,introduced=x,deprecated=y
#define __API_DEPRECATED_REP0(msg,arg0) __API_R(msg,arg0)
#define __API_DEPRECATED_REP2(msg,arg0,arg1,arg2) __API_R(msg,arg0) __API_R(msg,arg1) __API_R(msg,arg2)
#define __API_DEPRECATED_REP4(msg,arg0,arg1,arg2,arg3,arg4) __API_R(msg,arg0) __API_R(msg,arg1) __API_R(msg,arg2) __API_R(msg,arg3) __API_R(msg,arg4)
#define __API_DEPRECATED_REP6(msg,arg0,arg1,arg2,arg3,arg4,arg5,arg6) __API_R(msg,arg0) __API_R(msg,arg1) __API_R(msg,arg2) __API_R(msg,arg3) __API_R(msg,arg4) __API_R(msg,arg5) __API_R(msg,arg6)
#define __API_DEPRECATED_REP8(msg,arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8) __API_R(msg,arg0) __API_R(msg,arg1) __API_R(msg,arg2) __API_R(msg,arg3) __API_R(msg,arg4) __API_R(msg,arg5) __API_R(msg,arg6) __API_R(msg,arg7) __API_R(msg,arg8)
#define __API_DEPRECATED_WITH_REPLACEMENT_BEGIN(...) _Pragma("clang attribute push") __API_DEPRECATED_WITH_REPLACEMENT_BEGIN_GET_MACRO(__VA_ARGS__,__API_DEPRECATED_WITH_REPLACEMENT_BEGIN8,__API_DEPRECATED_WITH_REPLACEMENT_BEGIN7,__API_DEPRECATED_WITH_REPLACEMENT_BEGIN6,__API_DEPRECATED_WITH_REPLACEMENT_BEGIN5,__API_DEPRECATED_WITH_REPLACEMENT_BEGIN4,__API_DEPRECATED_WITH_REPLACEMENT_BEGIN3,__API_DEPRECATED_WITH_REPLACEMENT_BEGIN2,__API_DEPRECATED_WITH_REPLACEMENT_BEGIN1,__API_DEPRECATED_WITH_REPLACEMENT_BEGIN0,0,0)(__VA_ARGS__)
#define __API_DEPRECATED_WITH_REPLACEMENT_BEGIN0(msg,arg0) __API_R_BEGIN(msg,arg0)
#define __API_DEPRECATED_WITH_REPLACEMENT_BEGIN2(msg,arg0,arg1,arg2) __API_R_BEGIN(msg,arg0) __API_R_BEGIN(msg,arg1) __API_R_BEGIN(msg,arg2)
#define __API_DEPRECATED_WITH_REPLACEMENT_BEGIN4(msg,arg0,arg1,arg2,arg3,arg4) __API_R_BEGIN(msg,arg0) __API_R_BEGIN(msg,arg1) __API_R_BEGIN(msg,arg2) __API_R_BEGIN(msg,arg3) __API_R_BEGIN(msg,arg4)
#define __API_DEPRECATED_WITH_REPLACEMENT_BEGIN6(msg,arg0,arg1,arg2,arg3,arg4,arg5,arg6) __API_R_BEGIN(msg,arg0) __API_R_BEGIN(msg,arg1) __API_R_BEGIN(msg,arg2) __API_R_BEGIN(msg,arg3) __API_R_BEGIN(msg,arg4) __API_R_BEGIN(msg,arg5) __API_R_BEGIN(msg,arg6)
#define __API_DEPRECATED_WITH_REPLACEMENT_BEGIN8(msg,arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8) __API_R_BEGIN(msg,arg0) __API_R_BEGIN(msg,arg1) __API_R_BEGIN(msg,arg2) __API_R_BEGIN(msg,arg3) __API_R_BEGIN(msg,arg4) __API_R_BEGIN(msg,arg5) __API_R_BEGIN(msg,arg6) __API_R_BEGIN(msg,arg7) __API_R_BEGIN(msg,arg8)
#define __API_D_BEGIN(msg,x) _Pragma(__API_RANGE_STRINGIFY (clang attribute (__attribute__((availability(__API_DEPRECATED_PLATFORM_##x,message=msg))), apply_to = __API_APPLY_TO)))
#define __API_RANGE_STRINGIFY2(x) #x
#define __API_TO_BE_DEPRECATED_DRIVERKIT 100000
#define __API_TO_BE_DEPRECATED_IOS 100000
#define __API_TO_BE_DEPRECATED_MACCATALYST 100000
#define __API_TO_BE_DEPRECATED_MACOS 100000
#define __API_TO_BE_DEPRECATED_TVOS 100000
#define __API_TO_BE_DEPRECATED_VISIONOS 100000
#define __API_TO_BE_DEPRECATED_WATCHOS 100000
#define __API_U(x) __attribute__((availability(__API_UNAVAILABLE_PLATFORM_##x)))
#define __API_UNAVAILABLE(...) __API_UNAVAILABLE_GET_MACRO(__VA_ARGS__,__API_UNAVAILABLE8,__API_UNAVAILABLE7,__API_UNAVAILABLE6,__API_UNAVAILABLE5,__API_UNAVAILABLE4,__API_UNAVAILABLE3,__API_UNAVAILABLE2,__API_UNAVAILABLE1,__API_UNAVAILABLE0,0)(__VA_ARGS__)
#define __API_UNAVAILABLE0(arg0) __API_U(arg0)
#define __API_UNAVAILABLE2(arg0,arg1,arg2) __API_U(arg0) __API_U(arg1) __API_U(arg2)
#define __API_UNAVAILABLE4(arg0,arg1,arg2,arg3,arg4) __API_U(arg0) __API_U(arg1) __API_U(arg2) __API_U(arg3) __API_U(arg4)
#define __API_UNAVAILABLE6(arg0,arg1,arg2,arg3,arg4,arg5,arg6) __API_U(arg0) __API_U(arg1) __API_U(arg2) __API_U(arg3) __API_U(arg4) __API_U(arg5) __API_U(arg6)
#define __API_UNAVAILABLE8(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8) __API_U(arg0) __API_U(arg1) __API_U(arg2) __API_U(arg3) __API_U(arg4) __API_U(arg5) __API_U(arg6) __API_U(arg7) __API_U(arg8)
#define __API_UNAVAILABLE_BEGIN(...) _Pragma("clang attribute push") __API_UNAVAILABLE_BEGIN_GET_MACRO(__VA_ARGS__,__API_UNAVAILABLE_BEGIN8,__API_UNAVAILABLE_BEGIN7,__API_UNAVAILABLE_BEGIN6,__API_UNAVAILABLE_BEGIN5,__API_UNAVAILABLE_BEGIN4,__API_UNAVAILABLE_BEGIN3,__API_UNAVAILABLE_BEGIN2,__API_UNAVAILABLE_BEGIN1,__API_UNAVAILABLE_BEGIN0,0)(__VA_ARGS__)
#define __API_UNAVAILABLE_BEGIN0(arg0) __API_U_BEGIN(arg0)
#define __API_UNAVAILABLE_BEGIN2(arg0,arg1,arg2) __API_U_BEGIN(arg0) __API_U_BEGIN(arg1) __API_U_BEGIN(arg2)
#define __API_UNAVAILABLE_BEGIN4(arg0,arg1,arg2,arg3,arg4) __API_U_BEGIN(arg0) __API_U_BEGIN(arg1) __API_U_BEGIN(arg2) __API_U_BEGIN(arg3) __API_U_BEGIN(arg4)
#define __API_UNAVAILABLE_BEGIN6(arg0,arg1,arg2,arg3,arg4,arg5,arg6) __API_U_BEGIN(arg0) __API_U_BEGIN(arg1) __API_U_BEGIN(arg2) __API_U_BEGIN(arg3) __API_U_BEGIN(arg4) __API_U_BEGIN(arg5) __API_U_BEGIN(arg6)
#define __API_UNAVAILABLE_BEGIN8(arg0,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8) __API_U_BEGIN(arg0) __API_U_BEGIN(arg1) __API_U_BEGIN(arg2) __API_U_BEGIN(arg3) __API_U_BEGIN(arg4) __API_U_BEGIN(arg5) __API_U_BEGIN(arg6) __API_U_BEGIN(arg7) __API_U_BEGIN(arg8)
#define __API_UNAVAILABLE_PLATFORM_driverkit driverkit,unavailable
#define __API_UNAVAILABLE_PLATFORM_ios ios,unavailable
#define __API_UNAVAILABLE_PLATFORM_macCatalyst macCatalyst,unavailable
#define __API_UNAVAILABLE_PLATFORM_macos macos,unavailable
#define __API_UNAVAILABLE_PLATFORM_tvos tvos,unavailable
#define __API_UNAVAILABLE_PLATFORM_xros visionos,unavailable
#define __API_U_BEGIN(x) _Pragma(__API_RANGE_STRINGIFY (clang attribute (__attribute__((availability(__API_UNAVAILABLE_PLATFORM_##x))), apply_to = __API_APPLY_TO)))
#define __APPLE_CC__ 6000
#define __APPLE__ 1
#define __ARM64_ARCH_8__ 1
#define __ARM_64BIT_STATE 1
#define __ARM_ACLE 200
#define __ARM_ALIGN_MAX_STACK_PWR 4
#define __ARM_ARCH 8
#define __ARM_ARCH_8_3__ 1
#define __ARM_ARCH_8_4__ 1
#define __ARM_ARCH_8_5__ 1
#define __ARM_ARCH_ISA_A64 1
#define __ARM_ARCH_PROFILE 'A'
#define __ARM_FEATURE_AES 1
#define __ARM_FEATURE_ATOMICS 1
#define __ARM_FEATURE_BTI 1
#define __ARM_FEATURE_CLZ 1
#define __ARM_FEATURE_COMPLEX 1
#define __ARM_FEATURE_CRC32 1
#define __ARM_FEATURE_CRYPTO 1
#define __ARM_FEATURE_DIRECTED_ROUNDING 1
#define __ARM_FEATURE_DIV 1
#define __ARM_FEATURE_DOTPROD 1
#define __ARM_FEATURE_FMA 1
#define __ARM_FEATURE_FP16_FML 1
#define __ARM_FEATURE_FP16_SCALAR_ARITHMETIC 1
#define __ARM_FEATURE_FP16_VECTOR_ARITHMETIC 1
#define __ARM_FEATURE_FRINT 1
#define __ARM_FEATURE_IDIV 1
#define __ARM_FEATURE_JCVT 1
#define __ARM_FEATURE_LDREX 0xF
#define __ARM_FEATURE_NUMERIC_MAXMIN 1
#define __ARM_FEATURE_PAUTH 1
#define __ARM_FEATURE_QRDMX 1
#define __ARM_FEATURE_RCPC 1
#define __ARM_FEATURE_SHA2 1
#define __ARM_FEATURE_SHA3 1
#define __ARM_FEATURE_SHA512 1
#define __ARM_FEATURE_UNALIGNED 1
#define __ARM_FP 0xE
#define __ARM_FP16_ARGS 1
#define __ARM_FP16_FORMAT_IEEE 1
#define __ARM_NEON 1
#define __ARM_NEON_FP 0xE
#define __ARM_NEON__ 1
#define __ARM_PCS_AAPCS64 1
#define __ARM_SIZEOF_MINIMAL_ENUM 4
#define __ARM_SIZEOF_WCHAR_T 4
#define __ASSERT_FILE_NAME __FILE_NAME__
#define __ASSERT_H_ 
#define __ASSUME_PTR_ABI_SINGLE_BEGIN __ptrcheck_abi_assume_single()
#define __ASSUME_PTR_ABI_SINGLE_END __ptrcheck_abi_assume_unsafe_indexable()
#define __ATOMIC_ACQUIRE 2
#define __ATOMIC_ACQ_REL 4
#define __ATOMIC_CONSUME 1
#define __ATOMIC_RELAXED 0
#define __ATOMIC_RELEASE 3
#define __ATOMIC_SEQ_CST 5
#define __AVAILABILITY_INTERNAL_DEPRECATED __attribute__((deprecated))
#define __AVAILABILITY_INTERNAL_DEPRECATED_MSG(_msg) __attribute__((deprecated))
#define __AVAILABILITY_INTERNAL_LEGACY__ 
#define __AVAILABILITY_INTERNAL_REGULAR 
#define __AVAILABILITY_INTERNAL_UNAVAILABLE __attribute__((unavailable))
#define __AVAILABILITY_INTERNAL_WEAK_IMPORT __attribute__((weak_import))
#define __AVAILABILITY_INTERNAL__ 
#define __AVAILABILITY_INTERNAL__IPHONE_10_0 __attribute__((availability(ios,introduced=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_0_DEP__IPHONE_12_0 __attribute__((availability(ios,introduced=10.0,deprecated=12.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_0_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_1_DEP__IPHONE_10_1 __attribute__((availability(ios,introduced=10.1,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_1_DEP__IPHONE_10_1_MSG(_msg) __attribute__((availability(ios,introduced=10.1,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_1_DEP__IPHONE_10_2 __attribute__((availability(ios,introduced=10.1,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_1_DEP__IPHONE_10_2_MSG(_msg) __attribute__((availability(ios,introduced=10.1,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_1_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=10.1,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_1_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=10.1,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_1_DEP__IPHONE_NA __attribute__((availability(ios,introduced=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_2 __attribute__((availability(ios,introduced=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_2_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_3_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=10.3,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_3_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=10.3,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_10_3_DEP__IPHONE_NA __attribute__((availability(ios,introduced=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_11 __attribute__((availability(ios,introduced=11)))
#define __AVAILABILITY_INTERNAL__IPHONE_11_3 __attribute__((availability(ios,introduced=11.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_13_0 __attribute__((availability(ios,introduced=13.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0 __attribute__((availability(ios,introduced=2.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_2_0 __attribute__((availability(ios,introduced=2.0,deprecated=2.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_2_0_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=2.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_2_1 __attribute__((availability(ios,introduced=2.0,deprecated=2.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_2_1_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=2.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_2_2 __attribute__((availability(ios,introduced=2.0,deprecated=2.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_2_2_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=2.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_3_0 __attribute__((availability(ios,introduced=2.0,deprecated=3.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_3_0_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=3.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_3_1 __attribute__((availability(ios,introduced=2.0,deprecated=3.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_3_1_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=3.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_3_2 __attribute__((availability(ios,introduced=2.0,deprecated=3.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_3_2_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=3.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_4_0 __attribute__((availability(ios,introduced=2.0,deprecated=4.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_4_0_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=4.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_4_1 __attribute__((availability(ios,introduced=2.0,deprecated=4.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_4_1_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=4.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_4_2 __attribute__((availability(ios,introduced=2.0,deprecated=4.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_4_2_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=4.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_4_3 __attribute__((availability(ios,introduced=2.0,deprecated=4.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_4_3_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=4.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_5_0 __attribute__((availability(ios,introduced=2.0,deprecated=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_5_0_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_5_1 __attribute__((availability(ios,introduced=2.0,deprecated=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_5_1_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_6_0 __attribute__((availability(ios,introduced=2.0,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_6_0_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_6_1 __attribute__((availability(ios,introduced=2.0,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_6_1_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_7_0 __attribute__((availability(ios,introduced=2.0,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_7_0_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_7_1 __attribute__((availability(ios,introduced=2.0,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_7_1_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_8_0 __attribute__((availability(ios,introduced=2.0,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_8_0_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_8_1 __attribute__((availability(ios,introduced=2.0,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_8_1_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_8_2 __attribute__((availability(ios,introduced=2.0,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_8_2_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_8_3 __attribute__((availability(ios,introduced=2.0,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_8_3_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_8_4 __attribute__((availability(ios,introduced=2.0,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_8_4_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_9_0 __attribute__((availability(ios,introduced=2.0,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_9_0_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_9_1 __attribute__((availability(ios,introduced=2.0,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_9_1_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_9_2 __attribute__((availability(ios,introduced=2.0,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_9_2_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_9_3 __attribute__((availability(ios,introduced=2.0,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_9_3_MSG(_msg) __attribute__((availability(ios,introduced=2.0,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_0_DEP__IPHONE_NA __attribute__((availability(ios,introduced=2.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_1 __attribute__((availability(ios,introduced=2.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_1_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=2.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_10_0 __attribute__((availability(ios,introduced=2.2,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_10_0_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_10_1 __attribute__((availability(ios,introduced=2.2,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_10_1_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_10_2 __attribute__((availability(ios,introduced=2.2,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_10_2_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=2.2,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_2_2 __attribute__((availability(ios,introduced=2.2,deprecated=2.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_2_2_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=2.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_3_0 __attribute__((availability(ios,introduced=2.2,deprecated=3.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_3_0_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=3.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_3_1 __attribute__((availability(ios,introduced=2.2,deprecated=3.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_3_1_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=3.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_3_2 __attribute__((availability(ios,introduced=2.2,deprecated=3.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_3_2_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=3.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_4_0 __attribute__((availability(ios,introduced=2.2,deprecated=4.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_4_0_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=4.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_4_1 __attribute__((availability(ios,introduced=2.2,deprecated=4.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_4_1_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=4.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_4_2 __attribute__((availability(ios,introduced=2.2,deprecated=4.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_4_2_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=4.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_4_3 __attribute__((availability(ios,introduced=2.2,deprecated=4.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_4_3_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=4.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_5_0 __attribute__((availability(ios,introduced=2.2,deprecated=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_5_0_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_5_1 __attribute__((availability(ios,introduced=2.2,deprecated=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_5_1_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_6_0 __attribute__((availability(ios,introduced=2.2,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_6_0_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_6_1 __attribute__((availability(ios,introduced=2.2,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_6_1_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_7_0 __attribute__((availability(ios,introduced=2.2,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_7_0_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_7_1 __attribute__((availability(ios,introduced=2.2,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_7_1_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_8_0 __attribute__((availability(ios,introduced=2.2,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_8_0_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_8_1 __attribute__((availability(ios,introduced=2.2,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_8_1_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_8_2 __attribute__((availability(ios,introduced=2.2,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_8_2_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_8_3 __attribute__((availability(ios,introduced=2.2,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_8_3_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_8_4 __attribute__((availability(ios,introduced=2.2,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_8_4_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_9_0 __attribute__((availability(ios,introduced=2.2,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_9_0_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_9_1 __attribute__((availability(ios,introduced=2.2,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_9_1_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_9_2 __attribute__((availability(ios,introduced=2.2,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_9_2_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_9_3 __attribute__((availability(ios,introduced=2.2,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_9_3_MSG(_msg) __attribute__((availability(ios,introduced=2.2,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_2_2_DEP__IPHONE_NA __attribute__((availability(ios,introduced=2.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_0 __attribute__((availability(ios,introduced=3.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_0_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=3.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_10_0 __attribute__((availability(ios,introduced=3.1,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_10_0_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_10_1 __attribute__((availability(ios,introduced=3.1,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_10_1_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_10_2 __attribute__((availability(ios,introduced=3.1,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_10_2_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=3.1,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_3_1 __attribute__((availability(ios,introduced=3.1,deprecated=3.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_3_1_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=3.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_3_2 __attribute__((availability(ios,introduced=3.1,deprecated=3.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_3_2_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=3.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_4_0 __attribute__((availability(ios,introduced=3.1,deprecated=4.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_4_0_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=4.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_4_1 __attribute__((availability(ios,introduced=3.1,deprecated=4.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_4_1_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=4.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_4_2 __attribute__((availability(ios,introduced=3.1,deprecated=4.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_4_2_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=4.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_4_3 __attribute__((availability(ios,introduced=3.1,deprecated=4.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_4_3_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=4.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_5_0 __attribute__((availability(ios,introduced=3.1,deprecated=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_5_0_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_5_1 __attribute__((availability(ios,introduced=3.1,deprecated=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_5_1_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_6_0 __attribute__((availability(ios,introduced=3.1,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_6_0_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_6_1 __attribute__((availability(ios,introduced=3.1,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_6_1_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_7_0 __attribute__((availability(ios,introduced=3.1,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_7_0_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_7_1 __attribute__((availability(ios,introduced=3.1,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_7_1_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_8_0 __attribute__((availability(ios,introduced=3.1,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_8_0_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_8_1 __attribute__((availability(ios,introduced=3.1,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_8_1_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_8_2 __attribute__((availability(ios,introduced=3.1,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_8_2_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_8_3 __attribute__((availability(ios,introduced=3.1,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_8_3_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_8_4 __attribute__((availability(ios,introduced=3.1,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_8_4_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_9_0 __attribute__((availability(ios,introduced=3.1,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_9_0_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_9_1 __attribute__((availability(ios,introduced=3.1,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_9_1_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_9_2 __attribute__((availability(ios,introduced=3.1,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_9_2_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_9_3 __attribute__((availability(ios,introduced=3.1,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_9_3_MSG(_msg) __attribute__((availability(ios,introduced=3.1,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_1_DEP__IPHONE_NA __attribute__((availability(ios,introduced=3.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_2 __attribute__((availability(ios,introduced=3.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_3_2_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=3.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_0_DEP__IPHONE_10_0 __attribute__((availability(ios,introduced=4.0,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_0_DEP__IPHONE_10_0_MSG(_msg) __attribute__((availability(ios,introduced=4.0,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_0_DEP__IPHONE_10_1 __attribute__((availability(ios,introduced=4.0,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_0_DEP__IPHONE_10_1_MSG(_msg) __attribute__((availability(ios,introduced=4.0,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_0_DEP__IPHONE_10_2 __attribute__((availability(ios,introduced=4.0,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_0_DEP__IPHONE_10_2_MSG(_msg) __attribute__((availability(ios,introduced=4.0,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_0_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=4.0,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_0_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=4.0,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_0_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=4.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_10_0 __attribute__((availability(ios,introduced=4.1,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_10_0_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_10_1 __attribute__((availability(ios,introduced=4.1,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_10_1_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_10_2 __attribute__((availability(ios,introduced=4.1,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_10_2_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=4.1,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_4_1 __attribute__((availability(ios,introduced=4.1,deprecated=4.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_4_1_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=4.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_4_2 __attribute__((availability(ios,introduced=4.1,deprecated=4.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_4_2_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=4.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_4_3 __attribute__((availability(ios,introduced=4.1,deprecated=4.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_4_3_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=4.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_5_0 __attribute__((availability(ios,introduced=4.1,deprecated=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_5_0_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_5_1 __attribute__((availability(ios,introduced=4.1,deprecated=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_5_1_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_6_0 __attribute__((availability(ios,introduced=4.1,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_6_0_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_6_1 __attribute__((availability(ios,introduced=4.1,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_6_1_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_7_0 __attribute__((availability(ios,introduced=4.1,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_7_0_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_7_1 __attribute__((availability(ios,introduced=4.1,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_7_1_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_8_0 __attribute__((availability(ios,introduced=4.1,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_8_0_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_8_1 __attribute__((availability(ios,introduced=4.1,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_8_1_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_8_2 __attribute__((availability(ios,introduced=4.1,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_8_2_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_8_3 __attribute__((availability(ios,introduced=4.1,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_8_3_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_8_4 __attribute__((availability(ios,introduced=4.1,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_8_4_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_9_0 __attribute__((availability(ios,introduced=4.1,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_9_0_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_9_1 __attribute__((availability(ios,introduced=4.1,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_9_1_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_9_2 __attribute__((availability(ios,introduced=4.1,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_9_2_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_9_3 __attribute__((availability(ios,introduced=4.1,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_9_3_MSG(_msg) __attribute__((availability(ios,introduced=4.1,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_1_DEP__IPHONE_NA __attribute__((availability(ios,introduced=4.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_2 __attribute__((availability(ios,introduced=4.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_2_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=4.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_10_0 __attribute__((availability(ios,introduced=4.3,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_10_0_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_10_1 __attribute__((availability(ios,introduced=4.3,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_10_1_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_10_2 __attribute__((availability(ios,introduced=4.3,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_10_2_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=4.3,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_4_3 __attribute__((availability(ios,introduced=4.3,deprecated=4.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_4_3_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=4.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_5_0 __attribute__((availability(ios,introduced=4.3,deprecated=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_5_0_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_5_1 __attribute__((availability(ios,introduced=4.3,deprecated=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_5_1_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_6_0 __attribute__((availability(ios,introduced=4.3,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_6_0_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_6_1 __attribute__((availability(ios,introduced=4.3,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_6_1_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_7_0 __attribute__((availability(ios,introduced=4.3,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_7_0_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_7_1 __attribute__((availability(ios,introduced=4.3,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_7_1_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_8_0 __attribute__((availability(ios,introduced=4.3,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_8_0_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_8_1 __attribute__((availability(ios,introduced=4.3,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_8_1_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_8_2 __attribute__((availability(ios,introduced=4.3,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_8_2_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_8_3 __attribute__((availability(ios,introduced=4.3,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_8_3_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_8_4 __attribute__((availability(ios,introduced=4.3,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_8_4_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_9_0 __attribute__((availability(ios,introduced=4.3,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_9_0_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_9_1 __attribute__((availability(ios,introduced=4.3,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_9_1_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_9_2 __attribute__((availability(ios,introduced=4.3,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_9_2_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_9_3 __attribute__((availability(ios,introduced=4.3,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_9_3_MSG(_msg) __attribute__((availability(ios,introduced=4.3,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_4_3_DEP__IPHONE_NA __attribute__((availability(ios,introduced=4.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0 __attribute__((availability(ios,introduced=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_5_0 __attribute__((availability(ios,introduced=5.0,deprecated=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_5_0_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_5_1 __attribute__((availability(ios,introduced=5.0,deprecated=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_5_1_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_6_0 __attribute__((availability(ios,introduced=5.0,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_6_0_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_6_1 __attribute__((availability(ios,introduced=5.0,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_6_1_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_7_0 __attribute__((availability(ios,introduced=5.0,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_7_0_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_7_1 __attribute__((availability(ios,introduced=5.0,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_7_1_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_8_0 __attribute__((availability(ios,introduced=5.0,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_8_0_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_8_1 __attribute__((availability(ios,introduced=5.0,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_8_1_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_8_2 __attribute__((availability(ios,introduced=5.0,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_8_2_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_8_3 __attribute__((availability(ios,introduced=5.0,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_8_3_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_8_4 __attribute__((availability(ios,introduced=5.0,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_8_4_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_9_0 __attribute__((availability(ios,introduced=5.0,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_9_0_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_9_1 __attribute__((availability(ios,introduced=5.0,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_9_1_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_9_2 __attribute__((availability(ios,introduced=5.0,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_9_2_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_9_3 __attribute__((availability(ios,introduced=5.0,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_9_3_MSG(_msg) __attribute__((availability(ios,introduced=5.0,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_0_DEP__IPHONE_NA __attribute__((availability(ios,introduced=5.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_1 __attribute__((availability(ios,introduced=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_5_1_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=5.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_10_0 __attribute__((availability(ios,introduced=6.0,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_10_0_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_10_1 __attribute__((availability(ios,introduced=6.0,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_10_1_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_10_2 __attribute__((availability(ios,introduced=6.0,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_10_2_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=6.0,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_6_0 __attribute__((availability(ios,introduced=6.0,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_6_0_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_6_1 __attribute__((availability(ios,introduced=6.0,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_6_1_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_7_0 __attribute__((availability(ios,introduced=6.0,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_7_0_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_7_1 __attribute__((availability(ios,introduced=6.0,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_7_1_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_8_0 __attribute__((availability(ios,introduced=6.0,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_8_0_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_8_1 __attribute__((availability(ios,introduced=6.0,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_8_1_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_8_2 __attribute__((availability(ios,introduced=6.0,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_8_2_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_8_3 __attribute__((availability(ios,introduced=6.0,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_8_3_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_8_4 __attribute__((availability(ios,introduced=6.0,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_8_4_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_9_0 __attribute__((availability(ios,introduced=6.0,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_9_0_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_9_1 __attribute__((availability(ios,introduced=6.0,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_9_1_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_9_2 __attribute__((availability(ios,introduced=6.0,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_9_2_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_9_3 __attribute__((availability(ios,introduced=6.0,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_9_3_MSG(_msg) __attribute__((availability(ios,introduced=6.0,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_0_DEP__IPHONE_NA __attribute__((availability(ios,introduced=6.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_1 __attribute__((availability(ios,introduced=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_6_1_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=6.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_0_DEP__IPHONE_10_0 __attribute__((availability(ios,introduced=7.0,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_0_DEP__IPHONE_10_0_MSG(_msg) __attribute__((availability(ios,introduced=7.0,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_0_DEP__IPHONE_10_1 __attribute__((availability(ios,introduced=7.0,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_0_DEP__IPHONE_10_1_MSG(_msg) __attribute__((availability(ios,introduced=7.0,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_0_DEP__IPHONE_10_2 __attribute__((availability(ios,introduced=7.0,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_0_DEP__IPHONE_10_2_MSG(_msg) __attribute__((availability(ios,introduced=7.0,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_0_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=7.0,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_0_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=7.0,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_0_DEP__IPHONE_11_0 __attribute__((availability(ios,introduced=7.0,deprecated=11.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_0_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=7.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_10_0 __attribute__((availability(ios,introduced=7.1,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_10_0_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_10_1 __attribute__((availability(ios,introduced=7.1,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_10_1_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_10_2 __attribute__((availability(ios,introduced=7.1,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_10_2_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=7.1,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_7_1 __attribute__((availability(ios,introduced=7.1,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_7_1_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_8_0 __attribute__((availability(ios,introduced=7.1,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_8_0_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_8_1 __attribute__((availability(ios,introduced=7.1,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_8_1_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_8_2 __attribute__((availability(ios,introduced=7.1,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_8_2_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_8_3 __attribute__((availability(ios,introduced=7.1,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_8_3_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_8_4 __attribute__((availability(ios,introduced=7.1,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_8_4_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_9_0 __attribute__((availability(ios,introduced=7.1,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_9_0_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_9_1 __attribute__((availability(ios,introduced=7.1,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_9_1_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_9_2 __attribute__((availability(ios,introduced=7.1,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_9_2_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_9_3 __attribute__((availability(ios,introduced=7.1,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_9_3_MSG(_msg) __attribute__((availability(ios,introduced=7.1,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_7_1_DEP__IPHONE_NA __attribute__((availability(ios,introduced=7.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0 __attribute__((availability(ios,introduced=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_11_0_MSG(_msg) __attribute__((availability(ios,introduced=8.0,deprecated=11)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_11_3 __attribute__((availability(ios,introduced=8.0,deprecated=11.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_8_0 __attribute__((availability(ios,introduced=8.0,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_8_0_MSG(_msg) __attribute__((availability(ios,introduced=8.0,deprecated=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_8_1 __attribute__((availability(ios,introduced=8.0,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_8_1_MSG(_msg) __attribute__((availability(ios,introduced=8.0,deprecated=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_8_2 __attribute__((availability(ios,introduced=8.0,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_8_2_MSG(_msg) __attribute__((availability(ios,introduced=8.0,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_8_3 __attribute__((availability(ios,introduced=8.0,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_8_3_MSG(_msg) __attribute__((availability(ios,introduced=8.0,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_8_4 __attribute__((availability(ios,introduced=8.0,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_8_4_MSG(_msg) __attribute__((availability(ios,introduced=8.0,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_9_0 __attribute__((availability(ios,introduced=8.0,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_9_0_MSG(_msg) __attribute__((availability(ios,introduced=8.0,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_9_1 __attribute__((availability(ios,introduced=8.0,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_9_1_MSG(_msg) __attribute__((availability(ios,introduced=8.0,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_9_2 __attribute__((availability(ios,introduced=8.0,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_9_2_MSG(_msg) __attribute__((availability(ios,introduced=8.0,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_9_3 __attribute__((availability(ios,introduced=8.0,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_9_3_MSG(_msg) __attribute__((availability(ios,introduced=8.0,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_0_DEP__IPHONE_NA __attribute__((availability(ios,introduced=8.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_1 __attribute__((availability(ios,introduced=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_1_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=8.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_10_0 __attribute__((availability(ios,introduced=8.2,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_10_0_MSG(_msg) __attribute__((availability(ios,introduced=8.2,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_10_1 __attribute__((availability(ios,introduced=8.2,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_10_1_MSG(_msg) __attribute__((availability(ios,introduced=8.2,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_10_2 __attribute__((availability(ios,introduced=8.2,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_10_2_MSG(_msg) __attribute__((availability(ios,introduced=8.2,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=8.2,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=8.2,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_8_2 __attribute__((availability(ios,introduced=8.2,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_8_2_MSG(_msg) __attribute__((availability(ios,introduced=8.2,deprecated=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_8_3 __attribute__((availability(ios,introduced=8.2,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_8_3_MSG(_msg) __attribute__((availability(ios,introduced=8.2,deprecated=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_8_4 __attribute__((availability(ios,introduced=8.2,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_8_4_MSG(_msg) __attribute__((availability(ios,introduced=8.2,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_9_0 __attribute__((availability(ios,introduced=8.2,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_9_0_MSG(_msg) __attribute__((availability(ios,introduced=8.2,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_9_1 __attribute__((availability(ios,introduced=8.2,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_9_1_MSG(_msg) __attribute__((availability(ios,introduced=8.2,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_9_2 __attribute__((availability(ios,introduced=8.2,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_9_2_MSG(_msg) __attribute__((availability(ios,introduced=8.2,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_9_3 __attribute__((availability(ios,introduced=8.2,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_9_3_MSG(_msg) __attribute__((availability(ios,introduced=8.2,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_2_DEP__IPHONE_NA __attribute__((availability(ios,introduced=8.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_3 __attribute__((availability(ios,introduced=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_3_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=8.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_10_0 __attribute__((availability(ios,introduced=8.4,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_10_0_MSG(_msg) __attribute__((availability(ios,introduced=8.4,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_10_1 __attribute__((availability(ios,introduced=8.4,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_10_1_MSG(_msg) __attribute__((availability(ios,introduced=8.4,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_10_2 __attribute__((availability(ios,introduced=8.4,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_10_2_MSG(_msg) __attribute__((availability(ios,introduced=8.4,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=8.4,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=8.4,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_8_4 __attribute__((availability(ios,introduced=8.4,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_8_4_MSG(_msg) __attribute__((availability(ios,introduced=8.4,deprecated=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_9_0 __attribute__((availability(ios,introduced=8.4,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_9_0_MSG(_msg) __attribute__((availability(ios,introduced=8.4,deprecated=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_9_1 __attribute__((availability(ios,introduced=8.4,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_9_1_MSG(_msg) __attribute__((availability(ios,introduced=8.4,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_9_2 __attribute__((availability(ios,introduced=8.4,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_9_2_MSG(_msg) __attribute__((availability(ios,introduced=8.4,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_9_3 __attribute__((availability(ios,introduced=8.4,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_9_3_MSG(_msg) __attribute__((availability(ios,introduced=8.4,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_8_4_DEP__IPHONE_NA __attribute__((availability(ios,introduced=8.4)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_0 __attribute__((availability(ios,introduced=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_0_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=9.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_10_0 __attribute__((availability(ios,introduced=9.1,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_10_0_MSG(_msg) __attribute__((availability(ios,introduced=9.1,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_10_1 __attribute__((availability(ios,introduced=9.1,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_10_1_MSG(_msg) __attribute__((availability(ios,introduced=9.1,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_10_2 __attribute__((availability(ios,introduced=9.1,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_10_2_MSG(_msg) __attribute__((availability(ios,introduced=9.1,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=9.1,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=9.1,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_9_1 __attribute__((availability(ios,introduced=9.1,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_9_1_MSG(_msg) __attribute__((availability(ios,introduced=9.1,deprecated=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_9_2 __attribute__((availability(ios,introduced=9.1,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_9_2_MSG(_msg) __attribute__((availability(ios,introduced=9.1,deprecated=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_9_3 __attribute__((availability(ios,introduced=9.1,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_9_3_MSG(_msg) __attribute__((availability(ios,introduced=9.1,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_1_DEP__IPHONE_NA __attribute__((availability(ios,introduced=9.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_2 __attribute__((availability(ios,introduced=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_2_DEP__IPHONE_NA_MSG(_msg) __attribute__((availability(ios,introduced=9.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_3_DEP__IPHONE_10_0 __attribute__((availability(ios,introduced=9.3,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_3_DEP__IPHONE_10_0_MSG(_msg) __attribute__((availability(ios,introduced=9.3,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_3_DEP__IPHONE_10_1 __attribute__((availability(ios,introduced=9.3,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_3_DEP__IPHONE_10_1_MSG(_msg) __attribute__((availability(ios,introduced=9.3,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_3_DEP__IPHONE_10_2 __attribute__((availability(ios,introduced=9.3,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_3_DEP__IPHONE_10_2_MSG(_msg) __attribute__((availability(ios,introduced=9.3,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_3_DEP__IPHONE_10_3 __attribute__((availability(ios,introduced=9.3,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_3_DEP__IPHONE_10_3_MSG(_msg) __attribute__((availability(ios,introduced=9.3,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_3_DEP__IPHONE_9_3 __attribute__((availability(ios,introduced=9.3,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_3_DEP__IPHONE_9_3_MSG(_msg) __attribute__((availability(ios,introduced=9.3,deprecated=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_9_3_DEP__IPHONE_NA __attribute__((availability(ios,introduced=9.3)))
#define __AVAILABILITY_INTERNAL__IPHONE_COMPAT_VERSION_DEP__IPHONE_COMPAT_VERSION __attribute__((availability(ios,unavailable)))
#define __AVAILABILITY_INTERNAL__IPHONE_COMPAT_VERSION_DEP__IPHONE_COMPAT_VERSION_MSG(_msg) __attribute__((availability(ios,introduced=4.0,deprecated=4.0)))
#define __AVAILABILITY_INTERNAL__IPHONE_NA __attribute__((availability(ios,unavailable)))
#define __AVAILABILITY_INTERNAL__IPHONE_NA_DEP__IPHONE_NA __attribute__((availability(ios,unavailable)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_0 __attribute__((availability(macosx,introduced=10.0,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_0_MSG(_msg) __attribute__((availability(macosx,introduced=10.0,deprecated=10.0)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_1 __attribute__((availability(macosx,introduced=10.0,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_10_2 __attribute__((availability(macosx,introduced=10.0,deprecated=10.10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_10_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.0,deprecated=10.10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_10_3 __attribute__((availability(macosx,introduced=10.0,deprecated=10.10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_10_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.0,deprecated=10.10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_11_2 __attribute__((availability(macosx,introduced=10.0,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_11_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.0,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_11_3 __attribute__((availability(macosx,introduced=10.0,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_11_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.0,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_11_4 __attribute__((availability(macosx,introduced=10.0,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_11_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.0,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_12_1 __attribute__((availability(macosx,introduced=10.0,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_12_1_MSG(_msg) __attribute__((availability(macosx,introduced=10.0,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_12_2 __attribute__((availability(macosx,introduced=10.0,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_12_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.0,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_12_4 __attribute__((availability(macosx,introduced=10.0,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_12_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.0,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_13_MSG(_msg) __attribute__((availability(macosx,introduced=10.0,deprecated=10.13)))
#define __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_NA __attribute__((availability(macosx,introduced=10.0)))
#define __AVAILABILITY_INTERNAL__MAC_10_1 __attribute__((availability(macosx,introduced=10.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2 __attribute__((availability(macosx,introduced=10.10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2_DEP__MAC_10_11_2 __attribute__((availability(macosx,introduced=10.10.2,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2_DEP__MAC_10_11_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.10.2,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2_DEP__MAC_10_11_3 __attribute__((availability(macosx,introduced=10.10.2,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2_DEP__MAC_10_11_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.10.2,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2_DEP__MAC_10_11_4 __attribute__((availability(macosx,introduced=10.10.2,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2_DEP__MAC_10_11_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.10.2,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2_DEP__MAC_10_12_1 __attribute__((availability(macosx,introduced=10.10.2,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2_DEP__MAC_10_12_1_MSG(_msg) __attribute__((availability(macosx,introduced=10.10.2,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2_DEP__MAC_10_12_2 __attribute__((availability(macosx,introduced=10.10.2,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2_DEP__MAC_10_12_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.10.2,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2_DEP__MAC_10_12_4 __attribute__((availability(macosx,introduced=10.10.2,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2_DEP__MAC_10_12_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.10.2,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_2_DEP__MAC_NA_MSG(_msg) __attribute__((availability(macosx,introduced=10.10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_3_DEP__MAC_10_10_3 __attribute__((availability(macosx,introduced=10.10.3,deprecated=10.10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_3_DEP__MAC_10_10_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.10.3,deprecated=10.10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_3_DEP__MAC_10_11 __attribute__((availability(macosx,introduced=10.10.3,deprecated=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_3_DEP__MAC_10_11_MSG(_msg) __attribute__((availability(macosx,introduced=10.10.3,deprecated=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_3_DEP__MAC_10_12 __attribute__((availability(macosx,introduced=10.10.3,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_3_DEP__MAC_10_12_MSG(_msg) __attribute__((availability(macosx,introduced=10.10.3,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_3_DEP__MAC_NA __attribute__((availability(macosx,introduced=10.10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_1 __attribute__((availability(macosx,introduced=10.10,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_10_2 __attribute__((availability(macosx,introduced=10.10,deprecated=10.10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_10_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.10,deprecated=10.10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_10_3 __attribute__((availability(macosx,introduced=10.10,deprecated=10.10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_10_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.10,deprecated=10.10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_11_2 __attribute__((availability(macosx,introduced=10.10,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_11_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.10,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_11_3 __attribute__((availability(macosx,introduced=10.10,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_11_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.10,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_11_4 __attribute__((availability(macosx,introduced=10.10,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_11_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.10,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_12_1 __attribute__((availability(macosx,introduced=10.10,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_12_1_MSG(_msg) __attribute__((availability(macosx,introduced=10.10,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_12_2 __attribute__((availability(macosx,introduced=10.10,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_12_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.10,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_12_4 __attribute__((availability(macosx,introduced=10.10,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_10_12_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.10,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_10_DEP__MAC_NA __attribute__((availability(macosx,introduced=10.10)))
#define __AVAILABILITY_INTERNAL__MAC_10_11 __attribute__((availability(macosx,introduced=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_2_DEP__MAC_10_11_2 __attribute__((availability(macosx,introduced=10.11.2,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_2_DEP__MAC_10_11_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.11.2,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_2_DEP__MAC_10_11_3 __attribute__((availability(macosx,introduced=10.11.2,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_2_DEP__MAC_10_11_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.11.2,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_2_DEP__MAC_10_11_4 __attribute__((availability(macosx,introduced=10.11.2,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_2_DEP__MAC_10_11_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.11.2,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_2_DEP__MAC_10_12 __attribute__((availability(macosx,introduced=10.11.2,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_2_DEP__MAC_10_12_MSG(_msg) __attribute__((availability(macosx,introduced=10.11.2,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_2_DEP__MAC_NA __attribute__((availability(macosx,introduced=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_3 __attribute__((availability(macosx,introduced=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_3_DEP__MAC_10_12_1 __attribute__((availability(macosx,introduced=10.11.3,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_3_DEP__MAC_10_12_1_MSG(_msg) __attribute__((availability(macosx,introduced=10.11.3,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_3_DEP__MAC_10_12_2 __attribute__((availability(macosx,introduced=10.11.3,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_3_DEP__MAC_10_12_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.11.3,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_3_DEP__MAC_10_12_4 __attribute__((availability(macosx,introduced=10.11.3,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_3_DEP__MAC_10_12_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.11.3,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_3_DEP__MAC_NA_MSG(_msg) __attribute__((availability(macosx,introduced=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_4_DEP__MAC_10_11_4 __attribute__((availability(macosx,introduced=10.11.4,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_4_DEP__MAC_10_11_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.11.4,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_4_DEP__MAC_10_12 __attribute__((availability(macosx,introduced=10.11.4,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_4_DEP__MAC_10_12_MSG(_msg) __attribute__((availability(macosx,introduced=10.11.4,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_4_DEP__MAC_NA __attribute__((availability(macosx,introduced=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_10_1 __attribute__((availability(macosx,introduced=10.11,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_10_11_2 __attribute__((availability(macosx,introduced=10.11,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_10_11_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.11,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_10_11_3 __attribute__((availability(macosx,introduced=10.11,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_10_11_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.11,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_10_11_4 __attribute__((availability(macosx,introduced=10.11,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_10_11_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.11,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_10_12_1 __attribute__((availability(macosx,introduced=10.11,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_10_12_1_MSG(_msg) __attribute__((availability(macosx,introduced=10.11,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_10_12_2 __attribute__((availability(macosx,introduced=10.11,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_10_12_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.11,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_10_12_4 __attribute__((availability(macosx,introduced=10.11,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_10_12_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.11,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_11_DEP__MAC_NA_MSG(_msg) __attribute__((availability(macosx,introduced=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_1 __attribute__((availability(macosx,introduced=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_1_DEP__MAC_NA_MSG(_msg) __attribute__((availability(macosx,introduced=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_2_DEP__MAC_10_12_2 __attribute__((availability(macosx,introduced=10.12.2,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_2_DEP__MAC_10_12_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.12.2,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_2_DEP__MAC_10_12_4 __attribute__((availability(macosx,introduced=10.12.2,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_2_DEP__MAC_10_12_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.12.2,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_2_DEP__MAC_NA __attribute__((availability(macosx,introduced=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_4 __attribute__((availability(macosx,introduced=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_4_DEP__MAC_NA_MSG(_msg) __attribute__((availability(macosx,introduced=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_DEP__MAC_10_12_1 __attribute__((availability(macosx,introduced=10.12,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_DEP__MAC_10_12_1_MSG(_msg) __attribute__((availability(macosx,introduced=10.12,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_DEP__MAC_10_12_2 __attribute__((availability(macosx,introduced=10.12,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_DEP__MAC_10_12_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.12,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_DEP__MAC_10_12_4 __attribute__((availability(macosx,introduced=10.12,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_DEP__MAC_10_12_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.12,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_DEP__MAC_10_14 __attribute__((availability(macosx,introduced=10.12,deprecated=10.14)))
#define __AVAILABILITY_INTERNAL__MAC_10_12_DEP__MAC_NA_MSG(_msg) __attribute__((availability(macosx,introduced=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_13_4 __attribute__((availability(macosx,introduced=10.13.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_14_DEP__MAC_10_14 __attribute__((availability(macosx,introduced=10.14,deprecated=10.14)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_12_1 __attribute__((availability(macosx,introduced=10.1,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_12_1_MSG(_msg) __attribute__((availability(macosx,introduced=10.1,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_12_2 __attribute__((availability(macosx,introduced=10.1,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_12_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.1,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_12_4 __attribute__((availability(macosx,introduced=10.1,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_12_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.1,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_1_MSG(_msg) __attribute__((availability(macosx,introduced=10.1,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_2 __attribute__((availability(macosx,introduced=10.1,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.1,deprecated=10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_3 __attribute__((availability(macosx,introduced=10.1,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.1,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_4 __attribute__((availability(macosx,introduced=10.1,deprecated=10.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.1,deprecated=10.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_5 __attribute__((availability(macosx,introduced=10.1,deprecated=10.5)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_5_MSG(_msg) __attribute__((availability(macosx,introduced=10.1,deprecated=10.5)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_6 __attribute__((availability(macosx,introduced=10.1,deprecated=10.6)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_6_MSG(_msg) __attribute__((availability(macosx,introduced=10.1,deprecated=10.6)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_7 __attribute__((availability(macosx,introduced=10.1,deprecated=10.7)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_7_MSG(_msg) __attribute__((availability(macosx,introduced=10.1,deprecated=10.7)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_8 __attribute__((availability(macosx,introduced=10.1,deprecated=10.8)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_8_MSG(_msg) __attribute__((availability(macosx,introduced=10.1,deprecated=10.8)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_9 __attribute__((availability(macosx,introduced=10.1,deprecated=10.9)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_10_9_MSG(_msg) __attribute__((availability(macosx,introduced=10.1,deprecated=10.9)))
#define __AVAILABILITY_INTERNAL__MAC_10_1_DEP__MAC_NA __attribute__((availability(macosx,introduced=10.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_2 __attribute__((availability(macosx,introduced=10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_2_DEP__MAC_10_10 __attribute__((availability(macosx,introduced=10.2,deprecated=10.10)))
#define __AVAILABILITY_INTERNAL__MAC_10_2_DEP__MAC_10_10_MSG(_msg) __attribute__((availability(macosx,introduced=10.2,deprecated=10.10)))
#define __AVAILABILITY_INTERNAL__MAC_10_2_DEP__MAC_10_11 __attribute__((availability(macosx,introduced=10.2,deprecated=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_2_DEP__MAC_10_11_MSG(_msg) __attribute__((availability(macosx,introduced=10.2,deprecated=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_2_DEP__MAC_10_12 __attribute__((availability(macosx,introduced=10.2,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_2_DEP__MAC_10_12_MSG(_msg) __attribute__((availability(macosx,introduced=10.2,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_2_DEP__MAC_10_13 __attribute__((availability(macosx,introduced=10.2,deprecated=10.13)))
#define __AVAILABILITY_INTERNAL__MAC_10_2_DEP__MAC_NA_MSG(_msg) __attribute__((availability(macosx,introduced=10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_1 __attribute__((availability(macosx,introduced=10.3,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_10_2 __attribute__((availability(macosx,introduced=10.3,deprecated=10.10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_10_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_10_3 __attribute__((availability(macosx,introduced=10.3,deprecated=10.10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_10_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_11_2 __attribute__((availability(macosx,introduced=10.3,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_11_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_11_3 __attribute__((availability(macosx,introduced=10.3,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_11_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_11_4 __attribute__((availability(macosx,introduced=10.3,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_11_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_12_1 __attribute__((availability(macosx,introduced=10.3,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_12_1_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_12_2 __attribute__((availability(macosx,introduced=10.3,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_12_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_12_4 __attribute__((availability(macosx,introduced=10.3,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_12_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_3 __attribute__((availability(macosx,introduced=10.3,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_4 __attribute__((availability(macosx,introduced=10.3,deprecated=10.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_5 __attribute__((availability(macosx,introduced=10.3,deprecated=10.5)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_5_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.5)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_6 __attribute__((availability(macosx,introduced=10.3,deprecated=10.6)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_6_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.6)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_7 __attribute__((availability(macosx,introduced=10.3,deprecated=10.7)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_7_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.7)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_8 __attribute__((availability(macosx,introduced=10.3,deprecated=10.8)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_8_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.8)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_9 __attribute__((availability(macosx,introduced=10.3,deprecated=10.9)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_10_9_MSG(_msg) __attribute__((availability(macosx,introduced=10.3,deprecated=10.9)))
#define __AVAILABILITY_INTERNAL__MAC_10_3_DEP__MAC_NA __attribute__((availability(macosx,introduced=10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_4 __attribute__((availability(macosx,introduced=10.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_4_DEP__MAC_10_10 __attribute__((availability(macosx,introduced=10.4,deprecated=10.10)))
#define __AVAILABILITY_INTERNAL__MAC_10_4_DEP__MAC_10_10_MSG(_msg) __attribute__((availability(macosx,introduced=10.4,deprecated=10.10)))
#define __AVAILABILITY_INTERNAL__MAC_10_4_DEP__MAC_10_11 __attribute__((availability(macosx,introduced=10.4,deprecated=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_4_DEP__MAC_10_11_MSG(_msg) __attribute__((availability(macosx,introduced=10.4,deprecated=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_4_DEP__MAC_10_12 __attribute__((availability(macosx,introduced=10.4,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_4_DEP__MAC_10_12_MSG(_msg) __attribute__((availability(macosx,introduced=10.4,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_4_DEP__MAC_10_13 __attribute__((availability(macosx,introduced=10.4,deprecated=10.13)))
#define __AVAILABILITY_INTERNAL__MAC_10_4_DEP__MAC_NA_MSG(_msg) __attribute__((availability(macosx,introduced=10.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEPRECATED__MAC_10_7 __attribute__((availability(macosx,introduced=10.5.DEPRECATED..MAC.10.7)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_10 __attribute__((availability(macosx,introduced=10.5,deprecated=10.10)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_10_MSG(_msg) __attribute__((availability(macosx,introduced=10.5,deprecated=10.10)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_11 __attribute__((availability(macosx,introduced=10.5,deprecated=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_11_MSG(_msg) __attribute__((availability(macosx,introduced=10.5,deprecated=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_12 __attribute__((availability(macosx,introduced=10.5,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_12_MSG(_msg) __attribute__((availability(macosx,introduced=10.5,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_5 __attribute__((availability(macosx,introduced=10.5,deprecated=10.5)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_5_MSG(_msg) __attribute__((availability(macosx,introduced=10.5,deprecated=10.5)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_6 __attribute__((availability(macosx,introduced=10.5,deprecated=10.6)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_6_MSG(_msg) __attribute__((availability(macosx,introduced=10.5,deprecated=10.6)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_7 __attribute__((availability(macosx,introduced=10.5,deprecated=10.7)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_7_MSG(_msg) __attribute__((availability(macosx,introduced=10.5,deprecated=10.7)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_8 __attribute__((availability(macosx,introduced=10.5,deprecated=10.8)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_8_MSG(_msg) __attribute__((availability(macosx,introduced=10.5,deprecated=10.8)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_9 __attribute__((availability(macosx,introduced=10.5,deprecated=10.9)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_10_9_MSG(_msg) __attribute__((availability(macosx,introduced=10.5,deprecated=10.9)))
#define __AVAILABILITY_INTERNAL__MAC_10_5_DEP__MAC_NA __attribute__((availability(macosx,introduced=10.5)))
#define __AVAILABILITY_INTERNAL__MAC_10_6 __attribute__((availability(macosx,introduced=10.6)))
#define __AVAILABILITY_INTERNAL__MAC_10_6_DEP__MAC_10_10 __attribute__((availability(macosx,introduced=10.6,deprecated=10.10)))
#define __AVAILABILITY_INTERNAL__MAC_10_6_DEP__MAC_10_10_MSG(_msg) __attribute__((availability(macosx,introduced=10.6,deprecated=10.10)))
#define __AVAILABILITY_INTERNAL__MAC_10_6_DEP__MAC_10_11 __attribute__((availability(macosx,introduced=10.6,deprecated=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_6_DEP__MAC_10_11_MSG(_msg) __attribute__((availability(macosx,introduced=10.6,deprecated=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_6_DEP__MAC_10_12 __attribute__((availability(macosx,introduced=10.6,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_6_DEP__MAC_10_12_MSG(_msg) __attribute__((availability(macosx,introduced=10.6,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_6_DEP__MAC_10_13 __attribute__((availability(macosx,introduced=10.6,deprecated=10.13)))
#define __AVAILABILITY_INTERNAL__MAC_10_6_DEP__MAC_NA_MSG(_msg) __attribute__((availability(macosx,introduced=10.6)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_1 __attribute__((availability(macosx,introduced=10.7,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_10_2 __attribute__((availability(macosx,introduced=10.7,deprecated=10.10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_10_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.7,deprecated=10.10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_10_3 __attribute__((availability(macosx,introduced=10.7,deprecated=10.10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_10_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.7,deprecated=10.10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_11_2 __attribute__((availability(macosx,introduced=10.7,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_11_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.7,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_11_3 __attribute__((availability(macosx,introduced=10.7,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_11_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.7,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_11_4 __attribute__((availability(macosx,introduced=10.7,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_11_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.7,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_12_1 __attribute__((availability(macosx,introduced=10.7,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_12_1_MSG(_msg) __attribute__((availability(macosx,introduced=10.7,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_12_2 __attribute__((availability(macosx,introduced=10.7,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_12_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.7,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_12_4 __attribute__((availability(macosx,introduced=10.7,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_12_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.7,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_7 __attribute__((availability(macosx,introduced=10.7,deprecated=10.7)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_7_MSG(_msg) __attribute__((availability(macosx,introduced=10.7,deprecated=10.7)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_8 __attribute__((availability(macosx,introduced=10.7,deprecated=10.8)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_8_MSG(_msg) __attribute__((availability(macosx,introduced=10.7,deprecated=10.8)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_9 __attribute__((availability(macosx,introduced=10.7,deprecated=10.9)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_10_9_MSG(_msg) __attribute__((availability(macosx,introduced=10.7,deprecated=10.9)))
#define __AVAILABILITY_INTERNAL__MAC_10_7_DEP__MAC_NA __attribute__((availability(macosx,introduced=10.7)))
#define __AVAILABILITY_INTERNAL__MAC_10_8 __attribute__((availability(macosx,introduced=10.8)))
#define __AVAILABILITY_INTERNAL__MAC_10_8_DEP__MAC_10_10 __attribute__((availability(macosx,introduced=10.8,deprecated=10.10)))
#define __AVAILABILITY_INTERNAL__MAC_10_8_DEP__MAC_10_10_MSG(_msg) __attribute__((availability(macosx,introduced=10.8,deprecated=10.10)))
#define __AVAILABILITY_INTERNAL__MAC_10_8_DEP__MAC_10_11 __attribute__((availability(macosx,introduced=10.8,deprecated=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_8_DEP__MAC_10_11_MSG(_msg) __attribute__((availability(macosx,introduced=10.8,deprecated=10.11)))
#define __AVAILABILITY_INTERNAL__MAC_10_8_DEP__MAC_10_12 __attribute__((availability(macosx,introduced=10.8,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_8_DEP__MAC_10_12_MSG(_msg) __attribute__((availability(macosx,introduced=10.8,deprecated=10.12)))
#define __AVAILABILITY_INTERNAL__MAC_10_8_DEP__MAC_10_13 __attribute__((availability(macosx,introduced=10.8,deprecated=10.13)))
#define __AVAILABILITY_INTERNAL__MAC_10_8_DEP__MAC_NA_MSG(_msg) __attribute__((availability(macosx,introduced=10.8)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_1 __attribute__((availability(macosx,introduced=10.9,deprecated=10.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_10_2 __attribute__((availability(macosx,introduced=10.9,deprecated=10.10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_10_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.9,deprecated=10.10.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_10_3 __attribute__((availability(macosx,introduced=10.9,deprecated=10.10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_10_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.9,deprecated=10.10.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_11_2 __attribute__((availability(macosx,introduced=10.9,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_11_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.9,deprecated=10.11.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_11_3 __attribute__((availability(macosx,introduced=10.9,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_11_3_MSG(_msg) __attribute__((availability(macosx,introduced=10.9,deprecated=10.11.3)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_11_4 __attribute__((availability(macosx,introduced=10.9,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_11_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.9,deprecated=10.11.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_12_1 __attribute__((availability(macosx,introduced=10.9,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_12_1_MSG(_msg) __attribute__((availability(macosx,introduced=10.9,deprecated=10.12.1)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_12_2 __attribute__((availability(macosx,introduced=10.9,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_12_2_MSG(_msg) __attribute__((availability(macosx,introduced=10.9,deprecated=10.12.2)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_12_4 __attribute__((availability(macosx,introduced=10.9,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_12_4_MSG(_msg) __attribute__((availability(macosx,introduced=10.9,deprecated=10.12.4)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_10_14 __attribute__((availability(macosx,introduced=10.9,deprecated=10.14)))
#define __AVAILABILITY_INTERNAL__MAC_10_9_DEP__MAC_NA_MSG(_msg) __attribute__((availability(macosx,introduced=10.9)))
#define __AVAILABILITY_INTERNAL__MAC_NA __attribute__((availability(macosx,unavailable)))
#define __AVAILABILITY_INTERNAL__MAC_NA_DEP__MAC_NA_MSG(_msg) __attribute__((availability(macosx,unavailable)))
#define __AVAILABILITY_VERSIONS__ 
#define __AVAILABILITY__ 
#define __BEGIN_DECLS 
#define __BIGGEST_ALIGNMENT__ 8
#define __BITINT_MAXWIDTH__ 128
#define __BLOCKS__ 1
#define __BOOL_WIDTH__ 8
#define __BRIDGEOS_2_0 20000
#define __BRIDGEOS_3_0 30000
#define __BRIDGEOS_3_1 30100
#define __BRIDGEOS_3_4 30400
#define __BRIDGEOS_4_0 40000
#define __BRIDGEOS_4_1 40100
#define __BRIDGEOS_5_0 50000
#define __BRIDGEOS_5_1 50100
#define __BRIDGEOS_5_3 50300
#define __BRIDGEOS_6_0 60000
#define __BRIDGEOS_6_2 60200
#define __BRIDGEOS_6_4 60400
#define __BRIDGEOS_6_5 60500
#define __BRIDGEOS_6_6 60600
#define __BRIDGEOS_7_0 70000
#define __BRIDGEOS_7_1 70100
#define __BRIDGEOS_7_2 70200
#define __BRIDGEOS_7_3 70300
#define __BRIDGEOS_7_4 70400
#define __BRIDGEOS_7_6 70600
#define __BRIDGEOS_8_0 80000
#define __BRIDGEOS_8_1 80100
#define __BRIDGEOS_8_2 80200
#define __BRIDGEOS_8_3 80300
#define __BRIDGEOS_8_4 80400
#define __BRIDGEOS_8_5 80500
#define __BRIDGEOS_9_0 90000
#define __BRIDGEOS_9_1 90100
#define __BRIDGEOS_9_2 90200
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
#define __CAST_AWAY_QUALIFIER(variable,qualifier,type) _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wcast-qual\"") _Pragma("GCC diagnostic ignored \"-Wcast-align\"") _Pragma("GCC diagnostic ignored \"-Waddress-of-packed-member\"") ((type)(variable)) _Pragma("GCC diagnostic pop")
#define __CHAR16_TYPE__ unsigned short
#define __CHAR32_TYPE__ unsigned int
#define __CHAR_BIT__ 8
#define __CLANG_ATOMIC_BOOL_LOCK_FREE 2
#define __CLANG_ATOMIC_CHAR16_T_LOCK_FREE 2
#define __CLANG_ATOMIC_CHAR32_T_LOCK_FREE 2
#define __CLANG_ATOMIC_CHAR_LOCK_FREE 2
#define __CLANG_ATOMIC_INT_LOCK_FREE 2
#define __CLANG_ATOMIC_LLONG_LOCK_FREE 2
#define __CLANG_ATOMIC_LONG_LOCK_FREE 2
#define __CLANG_ATOMIC_POINTER_LOCK_FREE 2
#define __CLANG_ATOMIC_SHORT_LOCK_FREE 2
#define __CLANG_ATOMIC_WCHAR_T_LOCK_FREE 2
#define __CLANG_INTTYPES_H 
#define __CLANG_STDINT_H 
#define __CONCAT(x,y) x y
#define __CONSTANT_CFSTRINGS__ 1
#define __COPYRIGHT(s) __IDSTRING(copyright,s)
#define __DARWIN_1050(sym) __asm("_" __STRING(sym) __DARWIN_SUF_1050)
#define __DARWIN_1050ALIAS(sym) __asm("_" __STRING(sym) __DARWIN_SUF_1050 __DARWIN_SUF_UNIX03)
#define __DARWIN_1050ALIAS_C(sym) __asm("_" __STRING(sym) __DARWIN_SUF_1050 __DARWIN_SUF_NON_CANCELABLE __DARWIN_SUF_UNIX03)
#define __DARWIN_1050ALIAS_I(sym) __asm("_" __STRING(sym) __DARWIN_SUF_1050 __DARWIN_SUF_64_BIT_INO_T __DARWIN_SUF_UNIX03)
#define __DARWIN_1050INODE64(sym) __asm("_" __STRING(sym) __DARWIN_SUF_1050 __DARWIN_SUF_64_BIT_INO_T)
#define __DARWIN_64_BIT_INO_T 1
#define __DARWIN_ALIAS(sym) __asm("_" __STRING(sym) __DARWIN_SUF_UNIX03)
#define __DARWIN_ALIAS_C(sym) __asm("_" __STRING(sym) __DARWIN_SUF_NON_CANCELABLE __DARWIN_SUF_UNIX03)
#define __DARWIN_ALIAS_I(sym) __asm("_" __STRING(sym) __DARWIN_SUF_64_BIT_INO_T __DARWIN_SUF_UNIX03)
#define __DARWIN_ALIAS_STARTING(_mac,_iphone,x) __DARWIN_ALIAS_STARTING_MAC_##_mac(x)
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_10_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_10_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_10_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_10_3(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_11_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_11_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_11_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_11_3(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_11_4(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_12_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_12_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_12_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_12_3(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_12_4(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_13_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_13_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_13_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_13_3(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_13_4(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_13_5(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_13_6(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_13_7(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_14_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_14_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_14_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_14_3(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_14_4(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_14_5(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_14_6(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_14_7(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_14_8(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_15_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_15_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_15_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_15_3(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_15_4(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_15_5(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_15_6(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_15_7(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_15_8(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_16_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_16_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_16_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_16_3(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_16_4(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_16_5(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_16_6(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_16_7(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_17_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_17_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_17_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_17_3(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_17_4(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_17_5(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_18_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_18_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_18_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_2_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_2_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_2_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_3_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_3_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_3_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_4_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_4_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_4_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_4_3(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_5_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_5_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_6_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_6_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_7_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_7_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_8_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_8_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_8_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_8_3(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_8_4(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_9_0(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_9_1(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_9_2(x) 
#define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_9_3(x) 
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_0(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_1(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_10(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_10_2(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_10_3(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_11(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_11_2(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_11_3(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_11_4(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_12(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_12_1(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_12_2(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_12_4(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_13(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_13_1(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_13_2(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_13_4(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_14(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_14_1(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_14_4(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_14_5(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_14_6(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_15(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_15_1(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_15_4(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_16(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_2(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_3(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_4(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_5(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_6(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_7(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_8(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_10_9(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_11_0(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_11_1(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_11_3(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_11_4(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_11_5(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_11_6(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_12_0(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_12_1(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_12_2(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_12_3(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_12_4(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_12_5(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_12_6(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_12_7(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_13_0(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_13_1(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_13_2(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_13_3(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_13_4(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_13_5(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_13_6(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_14_0(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_14_1(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_14_2(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_14_3(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_14_4(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_14_5(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_15_0(x) x
#define __DARWIN_ALIAS_STARTING_MAC___MAC_15_1(x) 
#define __DARWIN_ALIAS_STARTING_MAC___MAC_15_2(x) 
#define __DARWIN_C_ANSI 010000L
#define __DARWIN_C_FULL 900000L
#define __DARWIN_C_LEVEL __DARWIN_C_FULL
#define __DARWIN_EXTSN(sym) __asm("_" __STRING(sym) __DARWIN_SUF_EXTSN)
#define __DARWIN_EXTSN_C(sym) __asm("_" __STRING(sym) __DARWIN_SUF_EXTSN __DARWIN_SUF_NON_CANCELABLE)
#define __DARWIN_INODE64(sym) __asm("_" __STRING(sym) __DARWIN_SUF_64_BIT_INO_T)
#define __DARWIN_NOCANCEL(sym) __asm("_" __STRING(sym) __DARWIN_SUF_NON_CANCELABLE)
#define __DARWIN_NON_CANCELABLE 0
#define __DARWIN_NO_LONG_LONG 0
#define __DARWIN_NULL ((void *)0)
#define __DARWIN_ONLY_64_BIT_INO_T 1
#define __DARWIN_ONLY_UNIX_CONFORMANCE 1
#define __DARWIN_ONLY_VERS_1050 1
#define __DARWIN_SUF_1050 
#define __DARWIN_SUF_64_BIT_INO_T 
#define __DARWIN_SUF_EXTSN "$DARWIN_EXTSN"
#define __DARWIN_SUF_NON_CANCELABLE 
#define __DARWIN_SUF_UNIX03 
#define __DARWIN_UNIX03 1
#define __DARWIN_VERS_1050 1
#define __DARWIN_WCHAR_MAX __WCHAR_MAX__
#define __DARWIN_WCHAR_MIN (-0x7fffffff - 1)
#define __DARWIN_WEOF ((__darwin_wint_t)-1)
#define __DBL_DECIMAL_DIG__ 17
#define __DBL_DENORM_MIN__ 4.9406564584124654e-324
#define __DBL_DIG__ 15
#define __DBL_EPSILON__ 2.2204460492503131e-16
#define __DBL_HAS_DENORM__ 1
#define __DBL_HAS_INFINITY__ 1
#define __DBL_HAS_QUIET_NAN__ 1
#define __DBL_MANT_DIG__ 53
#define __DBL_MAX_10_EXP__ 308
#define __DBL_MAX_EXP__ 1024
#define __DBL_MAX__ 1.7976931348623157e+308
#define __DBL_MIN_10_EXP__ (-307)
#define __DBL_MIN_EXP__ (-1021)
#define __DBL_MIN__ 2.2250738585072014e-308
#define __DECIMAL_DIG__ __LDBL_DECIMAL_DIG__
#define __DECONST(type,var) __CAST_AWAY_QUALIFIER(var, const, type)
#define __DEQUALIFY(type,var) __CAST_AWAY_QUALIFIER(var, const volatile, type)
#define __DEVOLATILE(type,var) __CAST_AWAY_QUALIFIER(var, volatile, type)
#define __DRIVERKIT_19_0 190000
#define __DRIVERKIT_20_0 200000
#define __DRIVERKIT_21_0 210000
#define __DRIVERKIT_22_0 220000
#define __DRIVERKIT_22_4 220400
#define __DRIVERKIT_22_5 220500
#define __DRIVERKIT_22_6 220600
#define __DRIVERKIT_23_0 230000
#define __DRIVERKIT_23_1 230100
#define __DRIVERKIT_23_2 230200
#define __DRIVERKIT_23_3 230300
#define __DRIVERKIT_23_4 230400
#define __DRIVERKIT_23_5 230500
#define __DRIVERKIT_24_0 240000
#define __DRIVERKIT_24_1 240100
#define __DRIVERKIT_24_2 240200
#define __DYNAMIC__ 1
#define __ENABLE_LEGACY_IPHONE_AVAILABILITY 1
#define __END_DECLS 
#define __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ 150000
#define __ENVIRONMENT_OS_VERSION_MIN_REQUIRED__ 150000
#define __FBSDID(s) 
#define __FINITE_MATH_ONLY__ 0
#define __FLT16_DECIMAL_DIG__ 5
#define __FLT16_DENORM_MIN__ 5.9604644775390625e-8F16
#define __FLT16_DIG__ 3
#define __FLT16_EPSILON__ 9.765625e-4F16
#define __FLT16_HAS_DENORM__ 1
#define __FLT16_HAS_INFINITY__ 1
#define __FLT16_HAS_QUIET_NAN__ 1
#define __FLT16_MANT_DIG__ 11
#define __FLT16_MAX_10_EXP__ 4
#define __FLT16_MAX_EXP__ 16
#define __FLT16_MAX__ 6.5504e+4F16
#define __FLT16_MIN_10_EXP__ (-4)
#define __FLT16_MIN_EXP__ (-13)
#define __FLT16_MIN__ 6.103515625e-5F16
#define __FLT_DECIMAL_DIG__ 9
#define __FLT_DENORM_MIN__ 1.40129846e-45F
#define __FLT_DIG__ 6
#define __FLT_EPSILON__ 1.19209290e-7F
#define __FLT_HAS_DENORM__ 1
#define __FLT_HAS_INFINITY__ 1
#define __FLT_HAS_QUIET_NAN__ 1
#define __FLT_MANT_DIG__ 24
#define __FLT_MAX_10_EXP__ 38
#define __FLT_MAX_EXP__ 128
#define __FLT_MAX__ 3.40282347e+38F
#define __FLT_MIN_10_EXP__ (-37)
#define __FLT_MIN_EXP__ (-125)
#define __FLT_MIN__ 1.17549435e-38F
#define __FLT_RADIX__ 2
#define __FPCLASS_NEGINF 0x0004
#define __FPCLASS_NEGNORMAL 0x0008
#define __FPCLASS_NEGSUBNORMAL 0x0010
#define __FPCLASS_NEGZERO 0x0020
#define __FPCLASS_POSINF 0x0200
#define __FPCLASS_POSNORMAL 0x0100
#define __FPCLASS_POSSUBNORMAL 0x0080
#define __FPCLASS_POSZERO 0x0040
#define __FPCLASS_QNAN 0x0002
#define __FPCLASS_SNAN 0x0001
#define __FP_FAST_FMA 1
#define __FP_FAST_FMAF 1
#define __GCC_ASM_FLAG_OUTPUTS__ 1
#define __GCC_ATOMIC_BOOL_LOCK_FREE 2
#define __GCC_ATOMIC_CHAR16_T_LOCK_FREE 2
#define __GCC_ATOMIC_CHAR32_T_LOCK_FREE 2
#define __GCC_ATOMIC_CHAR_LOCK_FREE 2
#define __GCC_ATOMIC_INT_LOCK_FREE 2
#define __GCC_ATOMIC_LLONG_LOCK_FREE 2
#define __GCC_ATOMIC_LONG_LOCK_FREE 2
#define __GCC_ATOMIC_POINTER_LOCK_FREE 2
#define __GCC_ATOMIC_SHORT_LOCK_FREE 2
#define __GCC_ATOMIC_TEST_AND_SET_TRUEVAL 1
#define __GCC_ATOMIC_WCHAR_T_LOCK_FREE 2
#define __GCC_HAVE_DWARF2_CFI_ASM 1
#define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_1 1
#define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_2 1
#define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 1
#define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8 1
#define __GNUC_MINOR__ 2
#define __GNUC_PATCHLEVEL__ 1
#define __GNUC_STDC_INLINE__ 1
#define __GNUC__ 4
#define __GXX_ABI_VERSION 1002
#define __HAVE_FUNCTION_MULTI_VERSIONING 1
#define __IDSTRING(name,string) static const char name[] __used = string
#define __INT16_C_SUFFIX__ 
#define __INT16_FMTd__ "hd"
#define __INT16_FMTi__ "hi"
#define __INT16_MAX__ 32767
#define __INT16_TYPE__ short
#define __INT32_C_SUFFIX__ 
#define __INT32_FMTd__ "d"
#define __INT32_FMTi__ "i"
#define __INT32_MAX__ 2147483647
#define __INT32_TYPE__ int
#define __INT64_C_SUFFIX__ LL
#define __INT64_FMTd__ "lld"
#define __INT64_FMTi__ "lli"
#define __INT64_MAX__ 9223372036854775807LL
#define __INT64_TYPE__ long long int
#define __INT8_C_SUFFIX__ 
#define __INT8_FMTd__ "hhd"
#define __INT8_FMTi__ "hhi"
#define __INT8_MAX__ 127
#define __INT8_TYPE__ signed char
#define __INTMAX_C_SUFFIX__ L
#define __INTMAX_FMTd__ "ld"
#define __INTMAX_FMTi__ "li"
#define __INTMAX_MAX__ 9223372036854775807L
#define __INTMAX_TYPE__ long int
#define __INTMAX_WIDTH__ 64
#define __INTPTR_FMTd__ "ld"
#define __INTPTR_FMTi__ "li"
#define __INTPTR_MAX__ 9223372036854775807L
#define __INTPTR_TYPE__ long int
#define __INTPTR_WIDTH__ 64
#define __INT_FAST16_FMTd__ "hd"
#define __INT_FAST16_FMTi__ "hi"
#define __INT_FAST16_MAX__ 32767
#define __INT_FAST16_TYPE__ short
#define __INT_FAST16_WIDTH__ 16
#define __INT_FAST32_FMTd__ "d"
#define __INT_FAST32_FMTi__ "i"
#define __INT_FAST32_MAX__ 2147483647
#define __INT_FAST32_TYPE__ int
#define __INT_FAST32_WIDTH__ 32
#define __INT_FAST64_FMTd__ "lld"
#define __INT_FAST64_FMTi__ "lli"
#define __INT_FAST64_MAX__ 9223372036854775807LL
#define __INT_FAST64_TYPE__ long long int
#define __INT_FAST64_WIDTH__ 64
#define __INT_FAST8_FMTd__ "hhd"
#define __INT_FAST8_FMTi__ "hhi"
#define __INT_FAST8_MAX__ 127
#define __INT_FAST8_TYPE__ signed char
#define __INT_FAST8_WIDTH__ 8
#define __INT_LEAST16_FMTd__ "hd"
#define __INT_LEAST16_FMTi__ "hi"
#define __INT_LEAST16_MAX__ 32767
#define __INT_LEAST16_TYPE__ short
#define __INT_LEAST16_WIDTH__ 16
#define __INT_LEAST32_FMTd__ "d"
#define __INT_LEAST32_FMTi__ "i"
#define __INT_LEAST32_MAX__ 2147483647
#define __INT_LEAST32_TYPE__ int
#define __INT_LEAST32_WIDTH__ 32
#define __INT_LEAST64_FMTd__ "lld"
#define __INT_LEAST64_FMTi__ "lli"
#define __INT_LEAST64_MAX__ 9223372036854775807LL
#define __INT_LEAST64_TYPE__ long long int
#define __INT_LEAST64_WIDTH__ 64
#define __INT_LEAST8_FMTd__ "hhd"
#define __INT_LEAST8_FMTi__ "hhi"
#define __INT_LEAST8_MAX__ 127
#define __INT_LEAST8_TYPE__ signed char
#define __INT_LEAST8_WIDTH__ 8
#define __INT_MAX__ 2147483647
#define __INT_WIDTH__ 32
#define __IOS_AVAILABLE(_vers) __OS_AVAILABILITY(ios,introduced=_vers)
#define __IOS_EXTENSION_UNAVAILABLE(_msg) 
#define __IOS_PROHIBITED __OS_AVAILABILITY(ios,unavailable)
#define __IOS_UNAVAILABLE __OS_AVAILABILITY(ios,unavailable)
#define __IPHONE_10_0 100000
#define __IPHONE_10_1 100100
#define __IPHONE_10_2 100200
#define __IPHONE_10_3 100300
#define __IPHONE_11_0 110000
#define __IPHONE_11_1 110100
#define __IPHONE_11_2 110200
#define __IPHONE_11_3 110300
#define __IPHONE_11_4 110400
#define __IPHONE_12_0 120000
#define __IPHONE_12_1 120100
#define __IPHONE_12_2 120200
#define __IPHONE_12_3 120300
#define __IPHONE_12_4 120400
#define __IPHONE_13_0 130000
#define __IPHONE_13_1 130100
#define __IPHONE_13_2 130200
#define __IPHONE_13_3 130300
#define __IPHONE_13_4 130400
#define __IPHONE_13_5 130500
#define __IPHONE_13_6 130600
#define __IPHONE_13_7 130700
#define __IPHONE_14_0 140000
#define __IPHONE_14_1 140100
#define __IPHONE_14_2 140200
#define __IPHONE_14_3 140300
#define __IPHONE_14_4 140400
#define __IPHONE_14_5 140500
#define __IPHONE_14_6 140600
#define __IPHONE_14_7 140700
#define __IPHONE_14_8 140800
#define __IPHONE_15_0 150000
#define __IPHONE_15_1 150100
#define __IPHONE_15_2 150200
#define __IPHONE_15_3 150300
#define __IPHONE_15_4 150400
#define __IPHONE_15_5 150500
#define __IPHONE_15_6 150600
#define __IPHONE_15_7 150700
#define __IPHONE_15_8 150800
#define __IPHONE_16_0 160000
#define __IPHONE_16_1 160100
#define __IPHONE_16_2 160200
#define __IPHONE_16_3 160300
#define __IPHONE_16_4 160400
#define __IPHONE_16_5 160500
#define __IPHONE_16_6 160600
#define __IPHONE_16_7 160700
#define __IPHONE_17_0 170000
#define __IPHONE_17_1 170100
#define __IPHONE_17_2 170200
#define __IPHONE_17_3 170300
#define __IPHONE_17_4 170400
#define __IPHONE_17_5 170500
#define __IPHONE_18_0 180000
#define __IPHONE_18_1 180100
#define __IPHONE_18_2 180200
#define __IPHONE_2_0 20000
#define __IPHONE_2_1 20100
#define __IPHONE_2_2 20200
#define __IPHONE_3_0 30000
#define __IPHONE_3_1 30100
#define __IPHONE_3_2 30200
#define __IPHONE_4_0 40000
#define __IPHONE_4_1 40100
#define __IPHONE_4_2 40200
#define __IPHONE_4_3 40300
#define __IPHONE_5_0 50000
#define __IPHONE_5_1 50100
#define __IPHONE_6_0 60000
#define __IPHONE_6_1 60100
#define __IPHONE_7_0 70000
#define __IPHONE_7_1 70100
#define __IPHONE_8_0 80000
#define __IPHONE_8_1 80100
#define __IPHONE_8_2 80200
#define __IPHONE_8_3 80300
#define __IPHONE_8_4 80400
#define __IPHONE_9_0 90000
#define __IPHONE_9_1 90100
#define __IPHONE_9_2 90200
#define __IPHONE_9_3 90300
#define __IPHONE_OS_VERSION_MIN_REQUIRED __IPHONE_9_0
#define __LDBL_DECIMAL_DIG__ 17
#define __LDBL_DENORM_MIN__ 4.9406564584124654e-324L
#define __LDBL_DIG__ 15
#define __LDBL_EPSILON__ 2.2204460492503131e-16L
#define __LDBL_HAS_DENORM__ 1
#define __LDBL_HAS_INFINITY__ 1
#define __LDBL_HAS_QUIET_NAN__ 1
#define __LDBL_MANT_DIG__ 53
#define __LDBL_MAX_10_EXP__ 308
#define __LDBL_MAX_EXP__ 1024
#define __LDBL_MAX__ 1.7976931348623157e+308L
#define __LDBL_MIN_10_EXP__ (-307)
#define __LDBL_MIN_EXP__ (-1021)
#define __LDBL_MIN__ 2.2250738585072014e-308L
#define __LITTLE_ENDIAN__ 1
#define __LLONG_WIDTH__ 64
#define __LONG_LONG_MAX__ 9223372036854775807LL
#define __LONG_MAX__ 9223372036854775807L
#define __LONG_WIDTH__ 64
#define __LP64__ 1
#define __MACH__ 1
#define __MAC_10_0 1000
#define __MAC_10_1 1010
#define __MAC_10_10 101000
#define __MAC_10_10_2 101002
#define __MAC_10_10_3 101003
#define __MAC_10_11 101100
#define __MAC_10_11_2 101102
#define __MAC_10_11_3 101103
#define __MAC_10_11_4 101104
#define __MAC_10_12 101200
#define __MAC_10_12_1 101201
#define __MAC_10_12_2 101202
#define __MAC_10_12_4 101204
#define __MAC_10_13 101300
#define __MAC_10_13_1 101301
#define __MAC_10_13_2 101302
#define __MAC_10_13_4 101304
#define __MAC_10_14 101400
#define __MAC_10_14_1 101401
#define __MAC_10_14_4 101404
#define __MAC_10_14_5 101405
#define __MAC_10_14_6 101406
#define __MAC_10_15 101500
#define __MAC_10_15_1 101501
#define __MAC_10_15_4 101504
#define __MAC_10_16 101600
#define __MAC_10_2 1020
#define __MAC_10_3 1030
#define __MAC_10_4 1040
#define __MAC_10_5 1050
#define __MAC_10_6 1060
#define __MAC_10_7 1070
#define __MAC_10_8 1080
#define __MAC_10_9 1090
#define __MAC_11_0 110000
#define __MAC_11_1 110100
#define __MAC_11_3 110300
#define __MAC_11_4 110400
#define __MAC_11_5 110500
#define __MAC_11_6 110600
#define __MAC_12_0 120000
#define __MAC_12_1 120100
#define __MAC_12_2 120200
#define __MAC_12_3 120300
#define __MAC_12_4 120400
#define __MAC_12_5 120500
#define __MAC_12_6 120600
#define __MAC_12_7 120700
#define __MAC_13_0 130000
#define __MAC_13_1 130100
#define __MAC_13_2 130200
#define __MAC_13_3 130300
#define __MAC_13_4 130400
#define __MAC_13_5 130500
#define __MAC_13_6 130600
#define __MAC_14_0 140000
#define __MAC_14_1 140100
#define __MAC_14_2 140200
#define __MAC_14_3 140300
#define __MAC_14_4 140400
#define __MAC_14_5 140500
#define __MAC_15_0 150000
#define __MAC_15_1 150100
#define __MAC_15_2 150200
#define __MAC_OS_X_VERSION_MAX_ALLOWED __MAC_15_2
#define __NO_INLINE__ 1
#define __NO_MATH_ERRNO__ 1
#define __OBJC_BOOL_IS_BOOL 1
#define __OPENCL_MEMORY_SCOPE_ALL_SVM_DEVICES 3
#define __OPENCL_MEMORY_SCOPE_DEVICE 2
#define __OPENCL_MEMORY_SCOPE_SUB_GROUP 4
#define __OPENCL_MEMORY_SCOPE_WORK_GROUP 1
#define __OPENCL_MEMORY_SCOPE_WORK_ITEM 0
#define __ORDER_BIG_ENDIAN__ 4321
#define __ORDER_LITTLE_ENDIAN__ 1234
#define __ORDER_PDP_ENDIAN__ 3412
#define __OSX_AVAILABLE_BUT_DEPRECATED(_osxIntro,_osxDep,_iosIntro,_iosDep) __AVAILABILITY_INTERNAL##_iosIntro##_DEP##_iosDep
#define __OSX_DEPRECATED(_start,_dep,_msg) __OSX_AVAILABLE(_start) __OS_AVAILABILITY_MSG(macosx,deprecated=_dep,_msg)
#define __OSX_EXTENSION_UNAVAILABLE(_msg) __OS_AVAILABILITY_MSG(macosx_app_extension,unavailable,_msg)
#define __OSX_UNAVAILABLE __OS_AVAILABILITY(macosx,unavailable)
#define __OS_AVAILABILITY(_target,_availability) __attribute__((availability(_target,_availability)))
#define __OS_AVAILABILITY_MSG(_target,_availability,_msg) 
#define __OS_EXTENSION_UNAVAILABLE(_msg) __OSX_EXTENSION_UNAVAILABLE(_msg) __IOS_EXTENSION_UNAVAILABLE(_msg)
#define __P(protos) ()
#define __PIC__ 2
#define __POINTER_WIDTH__ 64
#define __POSIX_C_DEPRECATED(ver) ___POSIX_C_DEPRECATED_STARTING_##ver
#define __PRAGMA_REDEFINE_EXTNAME 1
#define __PRI_64_LENGTH_MODIFIER__ "ll"
#define __PRI_8_LENGTH_MODIFIER__ "hh"
#define __PRI_MAX_LENGTH_MODIFIER__ "j"
#define __PROJECT_VERSION(s) __IDSTRING(project_version,s)
#define __PTHREAD_ATTR_SIZE__ 56
#define __PTHREAD_CONDATTR_SIZE__ 8
#define __PTHREAD_COND_SIZE__ 40
#define __PTHREAD_MUTEXATTR_SIZE__ 8
#define __PTHREAD_MUTEX_SIZE__ 56
#define __PTHREAD_ONCE_SIZE__ 8
#define __PTHREAD_RWLOCKATTR_SIZE__ 16
#define __PTHREAD_RWLOCK_SIZE__ 192
#define __PTHREAD_SIZE__ 8176
#define __PTRDIFF_FMTd__ "ld"
#define __PTRDIFF_FMTi__ "li"
#define __PTRDIFF_MAX__ 9223372036854775807L
#define __PTRDIFF_TYPE__ long int
#define __PTRDIFF_WIDTH__ 64
#define __RCSID(s) __IDSTRING(rcsid,s)
#define __REGISTER_PREFIX__ 
#define __SCCSID(s) __IDSTRING(sccsid,s)
#define __SCHAR_MAX__ 127
#define __SCN_64_LENGTH_MODIFIER__ "ll"
#define __SCN_MAX_LENGTH_MODIFIER__ "j"
#define __SHRT_MAX__ 32767
#define __SHRT_WIDTH__ 16
#define __SIG_ATOMIC_MAX__ 2147483647
#define __SIG_ATOMIC_WIDTH__ 32
#define __SIZEOF_DOUBLE__ 8
#define __SIZEOF_FLOAT__ 4
#define __SIZEOF_INT128__ 16
#define __SIZEOF_INT__ 4
#define __SIZEOF_LONG_DOUBLE__ 8
#define __SIZEOF_LONG_LONG__ 8
#define __SIZEOF_LONG__ 8
#define __SIZEOF_POINTER__ 8
#define __SIZEOF_PTRDIFF_T__ 8
#define __SIZEOF_SHORT__ 2
#define __SIZEOF_SIZE_T__ 8
#define __SIZEOF_WCHAR_T__ 4
#define __SIZEOF_WINT_T__ 4
#define __SIZE_FMTX__ "lX"
#define __SIZE_FMTo__ "lo"
#define __SIZE_FMTu__ "lu"
#define __SIZE_FMTx__ "lx"
#define __SIZE_MAX__ 18446744073709551615UL
#define __SIZE_TYPE__ long unsigned int
#define __SIZE_WIDTH__ 64
#define __SSP__ 1
#define __STDC_HOSTED__ 1
#define __STDC_NO_THREADS__ 1
#define __STDC_UTF_16__ 1
#define __STDC_UTF_32__ 1
#define __STDC_VERSION__ 201710L
#define __STDC_WANT_LIB_EXT1__ 1
#define __STRING(x) "x"
#define __SWIFT_UNAVAILABLE __OS_AVAILABILITY(swift,unavailable)
#define __TVOS_10_0 100000
#define __TVOS_10_0_1 100001
#define __TVOS_10_1 100100
#define __TVOS_10_2 100200
#define __TVOS_11_0 110000
#define __TVOS_11_1 110100
#define __TVOS_11_2 110200
#define __TVOS_11_3 110300
#define __TVOS_11_4 110400
#define __TVOS_12_0 120000
#define __TVOS_12_1 120100
#define __TVOS_12_2 120200
#define __TVOS_12_3 120300
#define __TVOS_12_4 120400
#define __TVOS_13_0 130000
#define __TVOS_13_2 130200
#define __TVOS_13_3 130300
#define __TVOS_13_4 130400
#define __TVOS_14_0 140000
#define __TVOS_14_1 140100
#define __TVOS_14_2 140200
#define __TVOS_14_3 140300
#define __TVOS_14_5 140500
#define __TVOS_14_6 140600
#define __TVOS_14_7 140700
#define __TVOS_15_0 150000
#define __TVOS_15_1 150100
#define __TVOS_15_2 150200
#define __TVOS_15_3 150300
#define __TVOS_15_4 150400
#define __TVOS_15_5 150500
#define __TVOS_15_6 150600
#define __TVOS_16_0 160000
#define __TVOS_16_1 160100
#define __TVOS_16_2 160200
#define __TVOS_16_3 160300
#define __TVOS_16_4 160400
#define __TVOS_16_5 160500
#define __TVOS_16_6 160600
#define __TVOS_17_0 170000
#define __TVOS_17_1 170100
#define __TVOS_17_2 170200
#define __TVOS_17_3 170300
#define __TVOS_17_4 170400
#define __TVOS_17_5 170500
#define __TVOS_18_0 180000
#define __TVOS_18_1 180100
#define __TVOS_18_2 180200
#define __TVOS_9_0 90000
#define __TVOS_9_1 90100
#define __TVOS_9_2 90200
#define __TVOS_AVAILABLE(_vers) __OS_AVAILABILITY(tvos,introduced=_vers)
#define __TVOS_PROHIBITED __OS_AVAILABILITY(tvos,unavailable)
#define __TVOS_UNAVAILABLE __OS_AVAILABILITY(tvos,unavailable)
#define __TV_OS_VERSION_MIN_REQUIRED __ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__
#define __TYPES_H_ 
#define __UINT16_C_SUFFIX__ 
#define __UINT16_FMTX__ "hX"
#define __UINT16_FMTo__ "ho"
#define __UINT16_FMTu__ "hu"
#define __UINT16_FMTx__ "hx"
#define __UINT16_MAX__ 65535
#define __UINT16_TYPE__ unsigned short
#define __UINT32_C_SUFFIX__ U
#define __UINT32_FMTX__ "X"
#define __UINT32_FMTo__ "o"
#define __UINT32_FMTu__ "u"
#define __UINT32_FMTx__ "x"
#define __UINT32_MAX__ 4294967295U
#define __UINT32_TYPE__ unsigned int
#define __UINT64_C_SUFFIX__ ULL
#define __UINT64_FMTX__ "llX"
#define __UINT64_FMTo__ "llo"
#define __UINT64_FMTu__ "llu"
#define __UINT64_FMTx__ "llx"
#define __UINT64_MAX__ 18446744073709551615ULL
#define __UINT64_TYPE__ long long unsigned int
#define __UINT8_C_SUFFIX__ 
#define __UINT8_FMTX__ "hhX"
#define __UINT8_FMTo__ "hho"
#define __UINT8_FMTu__ "hhu"
#define __UINT8_FMTx__ "hhx"
#define __UINT8_MAX__ 255
#define __UINT8_TYPE__ unsigned char
#define __UINTMAX_C_SUFFIX__ UL
#define __UINTMAX_FMTX__ "lX"
#define __UINTMAX_FMTo__ "lo"
#define __UINTMAX_FMTu__ "lu"
#define __UINTMAX_FMTx__ "lx"
#define __UINTMAX_MAX__ 18446744073709551615UL
#define __UINTMAX_TYPE__ long unsigned int
#define __UINTMAX_WIDTH__ 64
#define __UINTPTR_FMTX__ "lX"
#define __UINTPTR_FMTo__ "lo"
#define __UINTPTR_FMTu__ "lu"
#define __UINTPTR_FMTx__ "lx"
#define __UINTPTR_MAX__ 18446744073709551615UL
#define __UINTPTR_TYPE__ long unsigned int
#define __UINTPTR_WIDTH__ 64
#define __UINT_FAST16_FMTX__ "hX"
#define __UINT_FAST16_FMTo__ "ho"
#define __UINT_FAST16_FMTu__ "hu"
#define __UINT_FAST16_FMTx__ "hx"
#define __UINT_FAST16_MAX__ 65535
#define __UINT_FAST16_TYPE__ unsigned short
#define __UINT_FAST32_FMTX__ "X"
#define __UINT_FAST32_FMTo__ "o"
#define __UINT_FAST32_FMTu__ "u"
#define __UINT_FAST32_FMTx__ "x"
#define __UINT_FAST32_MAX__ 4294967295U
#define __UINT_FAST32_TYPE__ unsigned int
#define __UINT_FAST64_FMTX__ "llX"
#define __UINT_FAST64_FMTo__ "llo"
#define __UINT_FAST64_FMTu__ "llu"
#define __UINT_FAST64_FMTx__ "llx"
#define __UINT_FAST64_MAX__ 18446744073709551615ULL
#define __UINT_FAST64_TYPE__ long long unsigned int
#define __UINT_FAST8_FMTX__ "hhX"
#define __UINT_FAST8_FMTo__ "hho"
#define __UINT_FAST8_FMTu__ "hhu"
#define __UINT_FAST8_FMTx__ "hhx"
#define __UINT_FAST8_MAX__ 255
#define __UINT_FAST8_TYPE__ unsigned char
#define __UINT_LEAST16_FMTX__ "hX"
#define __UINT_LEAST16_FMTo__ "ho"
#define __UINT_LEAST16_FMTu__ "hu"
#define __UINT_LEAST16_FMTx__ "hx"
#define __UINT_LEAST16_MAX__ 65535
#define __UINT_LEAST16_TYPE__ unsigned short
#define __UINT_LEAST32_FMTX__ "X"
#define __UINT_LEAST32_FMTo__ "o"
#define __UINT_LEAST32_FMTu__ "u"
#define __UINT_LEAST32_FMTx__ "x"
#define __UINT_LEAST32_MAX__ 4294967295U
#define __UINT_LEAST32_TYPE__ unsigned int
#define __UINT_LEAST64_FMTX__ "llX"
#define __UINT_LEAST64_FMTo__ "llo"
#define __UINT_LEAST64_FMTu__ "llu"
#define __UINT_LEAST64_FMTx__ "llx"
#define __UINT_LEAST64_MAX__ 18446744073709551615ULL
#define __UINT_LEAST64_TYPE__ long long unsigned int
#define __UINT_LEAST8_FMTX__ "hhX"
#define __UINT_LEAST8_FMTo__ "hho"
#define __UINT_LEAST8_FMTu__ "hhu"
#define __UINT_LEAST8_FMTx__ "hhx"
#define __UINT_LEAST8_MAX__ 255
#define __UINT_LEAST8_TYPE__ unsigned char
#define __USER_LABEL_PREFIX__ _
#define __VERSION__ "Apple LLVM 16.0.0 (clang-1600.0.26.6)"
#define __VISIONOS_1_0 10000
#define __VISIONOS_1_1 10100
#define __VISIONOS_1_2 10200
#define __VISIONOS_2_0 20000
#define __VISIONOS_2_1 20100
#define __VISIONOS_2_2 20200
#define __WATCHOS_10_0 100000
#define __WATCHOS_10_1 100100
#define __WATCHOS_10_2 100200
#define __WATCHOS_10_3 100300
#define __WATCHOS_10_4 100400
#define __WATCHOS_10_5 100500
#define __WATCHOS_11_0 110000
#define __WATCHOS_11_1 110100
#define __WATCHOS_11_2 110200
#define __WATCHOS_1_0 10000
#define __WATCHOS_2_0 20000
#define __WATCHOS_2_1 20100
#define __WATCHOS_2_2 20200
#define __WATCHOS_3_0 30000
#define __WATCHOS_3_1 30100
#define __WATCHOS_3_1_1 30101
#define __WATCHOS_3_2 30200
#define __WATCHOS_4_0 40000
#define __WATCHOS_4_1 40100
#define __WATCHOS_4_2 40200
#define __WATCHOS_4_3 40300
#define __WATCHOS_5_0 50000
#define __WATCHOS_5_1 50100
#define __WATCHOS_5_2 50200
#define __WATCHOS_5_3 50300
#define __WATCHOS_6_0 60000
#define __WATCHOS_6_1 60100
#define __WATCHOS_6_2 60200
#define __WATCHOS_7_0 70000
#define __WATCHOS_7_1 70100
#define __WATCHOS_7_2 70200
#define __WATCHOS_7_3 70300
#define __WATCHOS_7_4 70400
#define __WATCHOS_7_5 70500
#define __WATCHOS_7_6 70600
#define __WATCHOS_8_0 80000
#define __WATCHOS_8_1 80100
#define __WATCHOS_8_3 80300
#define __WATCHOS_8_4 80400
#define __WATCHOS_8_5 80500
#define __WATCHOS_8_6 80600
#define __WATCHOS_8_7 80700
#define __WATCHOS_8_8 80800
#define __WATCHOS_9_0 90000
#define __WATCHOS_9_1 90100
#define __WATCHOS_9_2 90200
#define __WATCHOS_9_3 90300
#define __WATCHOS_9_4 90400
#define __WATCHOS_9_5 90500
#define __WATCHOS_9_6 90600
#define __WATCHOS_AVAILABLE(_vers) __OS_AVAILABILITY(watchos,introduced=_vers)
#define __WATCHOS_PROHIBITED __OS_AVAILABILITY(watchos,unavailable)
#define __WATCHOS_UNAVAILABLE __OS_AVAILABILITY(watchos,unavailable)
#define __WATCH_OS_VERSION_MIN_REQUIRED __ENVIRONMENT_WATCH_OS_VERSION_MIN_REQUIRED__
#define __WCHAR_MAX__ 2147483647
#define __WCHAR_TYPE__ int
#define __WCHAR_WIDTH__ 32
#define __WINT_MAX__ 2147483647
#define __WINT_TYPE__ int
#define __WINT_WIDTH__ 32
#define __WORDSIZE 64
#define __XNU_PRIVATE_EXTERN __attribute__((visibility("hidden")))
#define ___POSIX_C_DEPRECATED_STARTING_198808L 
#define ___POSIX_C_DEPRECATED_STARTING_199009L 
#define ___POSIX_C_DEPRECATED_STARTING_199209L 
#define ___POSIX_C_DEPRECATED_STARTING_199309L 
#define ___POSIX_C_DEPRECATED_STARTING_199506L 
#define ___POSIX_C_DEPRECATED_STARTING_200112L 
#define ___POSIX_C_DEPRECATED_STARTING_200809L 
#define __aarch64__ 1
#define __abortlike __dead2 __cold __not_tail_called
#define __alloc_align(n) __attribute__((alloc_align(n)))
#define __alloc_size(...) __attribute__((alloc_size(__VA_ARGS__)))
#define __apple_build_version__ 16000026
#define __arm64 1
#define __arm64__ 1
#define __array_decay_dicards_count_in_parameters 
#define __assert(e,file,line) __assert_rtn ((const char *)-1L, file, line, e)
#define __block __attribute__((__blocks__(byref)))
#define __clang__ 1
#define __clang_literal_encoding__ "UTF-8"
#define __clang_major__ 16
#define __clang_minor__ 0
#define __clang_patchlevel__ 0
#define __clang_version__ "16.0.0 (clang-1600.0.26.6)"
#define __clang_wide_literal_encoding__ "UTF-32"
#define __cold __attribute__((__cold__))
#define __compiler_barrier() __asm__ __volatile__("" ::: "memory")
#define __counted_by(N) 
#define __counted_by_or_null(N) 
#define __dead 
#define __dead2 __attribute__((__noreturn__))
#define __deprecated __attribute__((__deprecated__))
#define __disable_tail_calls __attribute__((__disable_tail_calls__))
#define __ended_by(E) 
#define __enum_closed __attribute__((__enum_extensibility__(closed)))
#define __enum_closed_decl(_name,_type,...) typedef enum : _type __VA_ARGS__ __enum_closed _name
#define __enum_decl(_name,_type,...) typedef enum : _type __VA_ARGS__ __enum_open _name
#define __enum_open __attribute__((__enum_extensibility__(open)))
#define __enum_options __attribute__((__flag_enum__))
#define __exported __attribute__((__visibility__("default")))
#define __exported_pop _Pragma("GCC visibility pop")
#define __exported_push _Pragma("GCC visibility push(default)")
#define __has_cpp_attribute(x) 0
#define __has_ptrcheck 0
#define __has_safe_buffers 1
#define __header_always_inline __header_inline __attribute__ ((__always_inline__))
#define __header_bidi_indexable 
#define __header_indexable 
#define __header_inline inline
#define __kernel_data_semantics 
#define __kernel_dual_semantics 
#define __kernel_ptr_semantics 
#define __kpi_deprecated(_msg) 
#define __kpi_deprecated_arm64_macos_unavailable 
#define __kpi_unavailable 
#define __llvm__ 1
#define __nonnull _Nonnull
#define __not_tail_called __attribute__((__not_tail_called__))
#define __null_terminated 
#define __null_terminated_to_indexable(P) (P)
#define __null_unspecified _Null_unspecified
#define __nullable _Nullable
#define __offsetof(type,field) __builtin_offsetof(type, field)
#define __options_closed_decl(_name,_type,...) typedef enum : _type __VA_ARGS__ __enum_closed __enum_options _name
#define __options_decl(_name,_type,...) typedef enum : _type __VA_ARGS__ __enum_open __enum_options _name
#define __osloglike(fmtarg,firstvararg) __attribute__((__format__ (__os_log__, fmtarg, firstvararg)))
#define __pic__ 2
#define __printf0like(fmtarg,firstvararg) __attribute__((__format__ (__printf0__, fmtarg, firstvararg)))
#define __printflike(fmtarg,firstvararg) __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#define __ptrcheck_abi_assume_single() 
#define __ptrcheck_abi_assume_unsafe_indexable() 
#define __ptrcheck_unavailable 
#define __ptrcheck_unavailable_r(REPLACEMENT) 
#define __pure 
#define __pure2 __attribute__((__const__))
#define __restrict restrict
#define __result_use_check __attribute__((__warn_unused_result__))
#define __returns_nonnull __attribute((returns_nonnull))
#define __scanflike(fmtarg,firstvararg) __attribute__((__format__ (__scanf__, fmtarg, firstvararg)))
#define __single 
#define __sized_by(N) 
#define __sized_by_or_null(N) 
#define __stateful_pure __attribute__((__pure__))
#define __strfmonlike(fmtarg,firstvararg) __attribute__((__format__ (__strfmon__, fmtarg, firstvararg)))
#define __strftimelike(fmtarg) __attribute__((__format__ (__strftime__, fmtarg, 0)))
#define __strong 
#define __swift_nonisolated __attribute__((__swift_attr__("nonisolated")))
#define __swift_nonisolated_unsafe __attribute__((__swift_attr__("nonisolated(unsafe)")))
#define __swift_unavailable(_msg) __attribute__((__availability__(swift, unavailable, message=_msg)))
#define __swift_unavailable_from_async(_msg) __attribute__((__swift_attr__("@_unavailableFromAsync(message: \"" _msg "\")")))
#define __terminated_by(T) 
#define __terminated_by_to_indexable(P) (P)
#define __unavailable __attribute__((__unavailable__))
#define __unreachable_ok_pop _Pragma("clang diagnostic pop")
#define __unreachable_ok_push _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wunreachable-code\"")
#define __unsafe_buffer_usage __attribute__((__unsafe_buffer_usage__))
#define __unsafe_buffer_usage_begin _Pragma("clang unsafe_buffer_usage begin")
#define __unsafe_buffer_usage_end _Pragma("clang unsafe_buffer_usage end")
#define __unsafe_forge_bidi_indexable(T,P,S) ((T)(P))
#define __unsafe_forge_null_terminated(T,P) ((T)(P))
#define __unsafe_forge_single(T,P) ((T)(P))
#define __unsafe_forge_terminated_by(T,P,E) ((T)(P))
#define __unsafe_indexable 
#define __unsafe_late_const 
#define __unsafe_null_terminated_from_indexable(P,...) (P)
#define __unsafe_null_terminated_to_indexable(P) (P)
#define __unsafe_terminated_by_from_indexable(T,P,...) (P)
#define __unsafe_terminated_by_to_indexable(P) (P)
#define __unsafe_unretained 
#define __unused __attribute__((__unused__))
#define __used __attribute__((__used__))
#define __weak __attribute__((objc_gc(weak)))
#define __xnu_data_size 
#define __xnu_returns_data_pointer 
#define assert(e) (__builtin_expect(!(e), 0) ? __assert_rtn(__func__, __ASSERT_FILE_NAME, __LINE__, #e) : (void)0)
#define assume(cond) assert(cond)
#define const __const
#define inline __inline
#define likely(x) __builtin_expect((uintptr_t)(x), 1)
#define signed __signed
#define static_assert _Static_assert
#define unlikely(x) __builtin_expect((uintptr_t)(x), 0)
#define volatile __volatile
