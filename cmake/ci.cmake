# number of parallel jobs for CTest
set(N 10)

##
## Needed tools.
##

find_program(CLANG_TOOL NAMES clang++-HEAD clang++-15 clang++-14 clang++-13 clang++-12 clang++-11 clang++)
execute_process(COMMAND ${CLANG_TOOL} --version OUTPUT_VARIABLE CLANG_TOOL_VERSION ERROR_VARIABLE CLANG_TOOL_VERSION)
string(REGEX MATCH "[0-9]+(\\.[0-9]+)+" CLANG_TOOL_VERSION "${CLANG_TOOL_VERSION}")
message(STATUS "🔖 Clang ${CLANG_TOOL_VERSION} (${CLANG_TOOL})")

find_program(CLANG_TIDY_TOOL NAMES clang-tidy-15 clang-tidy-14 clang-tidy-13 clang-tidy-12 clang-tidy-11 clang-tidy)
execute_process(COMMAND ${CLANG_TIDY_TOOL} --version OUTPUT_VARIABLE CLANG_TIDY_TOOL_VERSION ERROR_VARIABLE CLANG_TIDY_TOOL_VERSION)
string(REGEX MATCH "[0-9]+(\\.[0-9]+)+" CLANG_TIDY_TOOL_VERSION "${CLANG_TIDY_TOOL_VERSION}")
message(STATUS "🔖 Clang-Tidy ${CLANG_TIDY_TOOL_VERSION} (${CLANG_TIDY_TOOL})")

message(STATUS "🔖 CMake ${CMAKE_VERSION} (${CMAKE_COMMAND})")

find_program(GCC_TOOL NAMES g++-latest g++-HEAD g++-11 g++)
execute_process(COMMAND ${GCC_TOOL} --version OUTPUT_VARIABLE GCC_TOOL_VERSION ERROR_VARIABLE GCC_TOOL_VERSION)
string(REGEX MATCH "[0-9]+(\\.[0-9]+)+" GCC_TOOL_VERSION "${GCC_TOOL_VERSION}")
message(STATUS "🔖 GCC ${GCC_TOOL_VERSION} (${GCC_TOOL})")

##
## Thorough check with recent compilers.
##

# Ignored Clang warnings:
# -Wno-c++98-compat               The library targets C++11.
# -Wno-c++98-compat-pedantic      The library targets C++11.
# -Wno-deprecated-declarations    The library contains annotations for deprecated functions.
# -Wno-extra-semi-stmt            The library uses std::assert which triggers this warning.
# -Wno-padded                     We do not care about padding warnings.
# -Wno-covered-switch-default     All switches list all cases and a default case.
# -Wno-weak-vtables               The library is header-only.
# -Wno-reserved-identifier        See https://github.com/onqtam/doctest/issues/536.

set(CLANG_CXXFLAGS
    -Werror
    -Weverything
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-deprecated-declarations
    -Wno-extra-semi-stmt
    -Wno-padded
    -Wno-covered-switch-default
    -Wno-weak-vtables
    -Wno-reserved-identifier
)

# Warning flags determined for GCC 12.0 (experimental) with https://github.com/nlohmann/gcc_flags:
# Ignored GCC warnings:
# -Wno-abi-tag                    We do not care about ABI tags.
# -Wno-aggregate-return           The library uses aggregate returns.
# -Wno-long-long                  The library uses the long long type to interface with system functions.
# -Wno-namespaces                 The library uses namespaces.
# -Wno-padded                     We do not care about padding warnings.
# -Wno-system-headers             We do not care about warnings in system headers.
# -Wno-templates                  The library uses templates.
# -Wno-form                       Don't warn for unrecognized command line options. 

set(GCC_CXXFLAGS
  -pedantic
  -Werror
  --all-warnings
  --extra-warnings
  -W
  -WNSObject-attribute
  -Wno-abi-tag
  -Waddress
  -Waddress-of-packed-member
  -Wno-aggregate-return
  -Waggressive-loop-optimizations
  -Waligned-new=all
  -Wall
  -Walloc-zero
  -Walloca
  -Wanalyzer-double-fclose
  -Wanalyzer-double-free
  -Wanalyzer-exposure-through-output-file
  -Wanalyzer-file-leak
  -Wanalyzer-free-of-non-heap
  -Wanalyzer-malloc-leak
  -Wanalyzer-mismatching-deallocation
  -Wanalyzer-null-argument
  -Wanalyzer-null-dereference
  -Wanalyzer-possible-null-argument
  -Wanalyzer-possible-null-dereference
  -Wanalyzer-shift-count-negative
  -Wanalyzer-shift-count-overflow
  -Wanalyzer-stale-setjmp-buffer
  -Wanalyzer-tainted-allocation-size
  -Wanalyzer-tainted-array-index
  -Wanalyzer-tainted-divisor
  -Wanalyzer-tainted-offset
  -Wanalyzer-tainted-size
  -Wanalyzer-too-complex
  -Wanalyzer-unsafe-call-within-signal-handler
  -Wanalyzer-use-after-free
  -Wanalyzer-use-of-pointer-in-stale-stack-frame
  -Wanalyzer-use-of-uninitialized-value
  -Wanalyzer-write-to-const
  -Wanalyzer-write-to-string-literal
  -Warith-conversion
  -Warray-bounds
  -Warray-bounds=2
  -Warray-compare
  -Warray-parameter=2
  -Wattribute-alias=2
  -Wattribute-warning
  -Wattributes
  -Wbool-compare
  -Wbool-operation
  -Wbuiltin-declaration-mismatch
  -Wbuiltin-macro-redefined
  -Wc++0x-compat
  -Wc++11-compat
  -Wc++11-extensions
  -Wc++14-compat
  -Wc++14-extensions
  -Wc++17-compat
  -Wc++17-extensions
  -Wc++1z-compat
  -Wc++20-compat
  -Wc++20-extensions
  -Wc++23-extensions
  -Wc++2a-compat
  -Wcannot-profile
  -Wcast-align
  -Wcast-align=strict
  -Wcast-function-type
  -Wcast-qual
  -Wcatch-value=3
  -Wchar-subscripts
  -Wclass-conversion
  -Wclass-memaccess
  -Wclobbered
  -Wcomma-subscript
  -Wcomment
  -Wcomments
  -Wconditionally-supported
  -Wconversion
  -Wconversion-null
  -Wcoverage-invalid-line-number
  -Wcoverage-mismatch
  -Wcpp
  -Wctad-maybe-unsupported
  -Wctor-dtor-privacy
  -Wdangling-else
  -Wdate-time
  -Wdelete-incomplete
  -Wdelete-non-virtual-dtor
  -Wdeprecated
  -Wdeprecated-copy
  -Wdeprecated-copy-dtor
  -Wdeprecated-declarations
  -Wdeprecated-enum-enum-conversion
  -Wdeprecated-enum-float-conversion
  -Wdisabled-optimization
  -Wdiv-by-zero
  -Wdouble-promotion
  -Wduplicated-branches
  -Wduplicated-cond
  -Weffc++
  -Wempty-body
  -Wendif-labels
  -Wenum-compare
  -Wenum-conversion
  -Wexceptions
  -Wexpansion-to-defined
  -Wextra
  -Wextra-semi
  -Wfloat-conversion
  -Wfloat-equal
  -Wno-form
  -Wformat-diag
  -Wformat-overflow=2
  -Wformat-signedness
  -Wformat-truncation=2
  -Wformat=2
  -Wframe-address
  -Wfree-nonheap-object
  -Whsa
  -Wif-not-aligned
  -Wignored-attributes
  -Wignored-qualifiers
  -Wimplicit-fallthrough=5
  -Winaccessible-base
  -Winfinite-recursion
  -Winherited-variadic-ctor
  -Winit-list-lifetime
  -Winit-self
  -Winline
  -Wint-in-bool-context
  -Wint-to-pointer-cast
  -Winterference-size
  -Winvalid-imported-macros
  -Winvalid-memory-model
  -Winvalid-offsetof
  -Winvalid-pch
  -Wliteral-suffix
  -Wlogical-not-parentheses
  -Wlogical-op
  -Wno-long-long
  -Wlto-type-mismatch
  -Wmain
  -Wmaybe-uninitialized
  -Wmemset-elt-size
  -Wmemset-transposed-args
  -Wmisleading-indentation
  -Wmismatched-dealloc
  -Wmismatched-new-delete
  -Wmismatched-tags
  -Wmissing-attributes
  -Wmissing-braces
  -Wmissing-declarations
  -Wmissing-field-initializers
  -Wmissing-include-dirs
  -Wmissing-profile
  -Wmissing-requires
  -Wmultichar
  -Wmultiple-inheritance
  -Wmultistatement-macros
  -Wno-namespaces
  -Wnarrowing
  -Wnoexcept
  -Wnoexcept-type
  -Wnon-template-friend
  -Wnon-virtual-dtor
  -Wnonnull
  -Wnonnull-compare
  -Wnormalized=nfkc
  -Wnull-dereference
  -Wodr
  -Wold-style-cast
  -Wopenacc-parallelism
  -Wopenmp-simd
  -Woverflow
  -Woverlength-strings
  -Woverloaded-virtual
  -Wpacked
  -Wpacked-bitfield-compat
  -Wpacked-not-aligned
  -Wno-padded
  -Wparentheses
  -Wpedantic
  -Wpessimizing-move
  -Wplacement-new=2
  -Wpmf-conversions
  -Wpointer-arith
  -Wpointer-compare
  -Wpragmas
  -Wprio-ctor-dtor
  -Wpsabi
  -Wrange-loop-construct
  -Wredundant-decls
  -Wredundant-move
  -Wredundant-tags
  -Wregister
  -Wreorder
  -Wrestrict
  -Wreturn-local-addr
  -Wreturn-type
  -Wscalar-storage-order
  -Wsequence-point
  -Wshadow=compatible-local
  -Wshadow=global
  -Wshadow=local
  -Wshift-count-negative
  -Wshift-count-overflow
  -Wshift-negative-value
  -Wshift-overflow=2
  -Wsign-compare
  -Wsign-conversion
  -Wsign-promo
  -Wsized-deallocation
  -Wsizeof-array-argument
  -Wsizeof-array-div
  -Wsizeof-pointer-div
  -Wsizeof-pointer-memaccess
  -Wstack-protector
  -Wstrict-aliasing
  -Wstrict-aliasing=3
  -Wstrict-null-sentinel
  -Wstrict-overflow
  -Wstrict-overflow=5
  -Wstring-compare
  -Wstringop-overflow=4
  -Wstringop-overread
  -Wstringop-truncation
  -Wsubobject-linkage
  -Wsuggest-attribute=cold
  -Wsuggest-attribute=const
  -Wsuggest-attribute=format
  -Wsuggest-attribute=malloc
  -Wsuggest-attribute=noreturn
  -Wsuggest-attribute=pure
  -Wsuggest-final-methods
  -Wsuggest-final-types
  -Wsuggest-override
  -Wswitch
  -Wswitch-bool
  -Wswitch-default
  -Wswitch-enum
  -Wswitch-outside-range
  -Wswitch-unreachable
  -Wsync-nand
  -Wsynth
  -Wno-system-headers
  -Wtautological-compare
  -Wno-templates
  -Wterminate
  -Wtrampolines
  -Wtrigraphs
  -Wtsan
  -Wtype-limits
  -Wundef
  -Wuninitialized
  -Wunknown-pragmas
  -Wunreachable-code
  -Wunsafe-loop-optimizations
  -Wunused
  -Wunused-but-set-parameter
  -Wunused-but-set-variable
  -Wunused-const-variable=2
  -Wunused-function
  -Wunused-label
  -Wunused-local-typedefs
  -Wunused-macros
  -Wunused-parameter
  -Wunused-result
  -Wunused-value
  -Wunused-variable
  -Wuseless-cast
  -Wvarargs
  -Wvariadic-macros
  -Wvector-operation-performance
  -Wvexing-parse
  -Wvirtual-inheritance
  -Wvirtual-move-assign
  -Wvla
  -Wvla-parameter
  -Wvolatile
  -Wvolatile-register-var
  -Wwrite-strings
  -Wzero-as-null-pointer-constant
  -Wzero-length-bounds
)

add_custom_target(ci_test_gcc
  COMMAND CXX=${GCC_TOOL} CXXFLAGS="${GCC_CXXFLAGS}" ${CMAKE_COMMAND}
    -DCMAKE_BUILD_TYPE=Debug -GNinja
    -DTPH_BuildTests=ON -DTPH_BuildExamples=ON
    -S${PROJECT_SOURCE_DIR} -B${PROJECT_BINARY_DIR}/build_gcc
  COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR}/build_gcc
  COMMAND cd ${PROJECT_BINARY_DIR}/build_gcc && ${CMAKE_CTEST_COMMAND} --parallel ${N} --output-on-failure
  COMMENT "Compile and test with GCC using maximal warning flags"
)

add_custom_target(ci_test_clang
  COMMAND CXX=${CLANG_TOOL} CXXFLAGS="${CLANG_CXXFLAGS}" ${CMAKE_COMMAND}
    -DCMAKE_BUILD_TYPE=Debug -GNinja
    -DTPH_BuildTests=ON -DTPH_BuildExamples=ON
    -S${PROJECT_SOURCE_DIR} -B${PROJECT_BINARY_DIR}/build_clang
  COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR}/build_clang
  COMMAND cd ${PROJECT_BINARY_DIR}/build_clang && ${CMAKE_CTEST_COMMAND} --parallel ${N} --output-on-failure
  COMMENT "Compile and test with Clang using maximal warning flags"
)

##
## Different C++ Standards.
##

foreach(CXX_STANDARD 11 14 17 20)
  add_custom_target(ci_test_gcc_cxx${CXX_STANDARD}
    COMMAND CXX=${GCC_TOOL} CXXFLAGS="${GCC_CXXFLAGS}" ${CMAKE_COMMAND}
      -DCMAKE_BUILD_TYPE=Debug -GNinja
      -DTPH_BuildTests=ON -DTPH_BuildExamples=ON
      -DTPH_TestStandards=${CXX_STANDARD}
      -S${PROJECT_SOURCE_DIR} -B${PROJECT_BINARY_DIR}/build_gcc_cxx${CXX_STANDARD}
    COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR}/build_gcc_cxx${CXX_STANDARD}
    COMMAND cd ${PROJECT_BINARY_DIR}/build_gcc_cxx${CXX_STANDARD} && ${CMAKE_CTEST_COMMAND} --parallel ${N} --output-on-failure
    COMMENT "Compile and test with GCC for C++${CXX_STANDARD}"
  )

  add_custom_target(ci_test_clang_cxx${CXX_STANDARD}
    COMMAND CXX=${CLANG_TOOL} CXXFLAGS="${CLANG_CXXFLAGS}" ${CMAKE_COMMAND}
      -DCMAKE_BUILD_TYPE=Debug -GNinja
      -DTPH_BuildTests=ON -DTPH_BuildExamples=ON
      -DTPH_TestStandards=${CXX_STANDARD}
      -S${PROJECT_SOURCE_DIR} -B${PROJECT_BINARY_DIR}/build_clang_cxx${CXX_STANDARD}
    COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR}/build_clang_cxx${CXX_STANDARD}
    COMMAND cd ${PROJECT_BINARY_DIR}/build_clang_cxx${CXX_STANDARD} && ${CMAKE_CTEST_COMMAND} --parallel ${N} --output-on-failure
    COMMENT "Compile and test with Clang for C++${CXX_STANDARD}"
  )
endforeach()


##
## Sanitizers.
##

set(CLANG_CXX_FLAGS_SANITIZER "-g -O1 -fsanitize=address -fsanitize=undefined -fsanitize=integer -fsanitize=nullability -fno-omit-frame-pointer -fno-sanitize-recover=all -fno-sanitize=unsigned-integer-overflow -fno-sanitize=unsigned-shift-base")

add_custom_target(ci_test_clang_sanitizer
  COMMAND CXX=${CLANG_TOOL} CXXFLAGS=${CLANG_CXX_FLAGS_SANITIZER} ${CMAKE_COMMAND}
    -DCMAKE_BUILD_TYPE=Debug -GNinja
    -DJSON_BuildTests=ON -DTPH_BuildExamples=ON
    -S${PROJECT_SOURCE_DIR} -B${PROJECT_BINARY_DIR}/build_clang_sanitizer
  COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR}/build_clang_sanitizer
  COMMAND cd ${PROJECT_BINARY_DIR}/build_clang_sanitizer && ${CMAKE_CTEST_COMMAND} --parallel ${N} --output-on-failure
  COMMENT "Compile and test with sanitizers"
)

##
## Check code with Clang Static Analyzer.
##

set(CLANG_ANALYZER_CHECKS "fuchsia.HandleChecker,nullability.NullableDereferenced,nullability.NullablePassedToNonnull,nullability.NullableReturnedFromNonnull,optin.cplusplus.UninitializedObject,optin.cplusplus.VirtualCall,optin.mpi.MPI-Checker,optin.osx.OSObjectCStyleCast,optin.osx.cocoa.localizability.EmptyLocalizationContextChecker,optin.osx.cocoa.localizability.NonLocalizedStringChecker,optin.performance.GCDAntipattern,optin.performance.Padding,optin.portability.UnixAPI,security.FloatLoopCounter,security.insecureAPI.DeprecatedOrUnsafeBufferHandling,security.insecureAPI.bcmp,security.insecureAPI.bcopy,security.insecureAPI.bzero,security.insecureAPI.rand,security.insecureAPI.strcpy,valist.CopyToSelf,valist.Uninitialized,valist.Unterminated,webkit.NoUncountedMemberChecker,webkit.RefCntblBaseVirtualDtor,core.CallAndMessage,core.DivideZero,core.NonNullParamChecker,core.NullDereference,core.StackAddressEscape,core.UndefinedBinaryOperatorResult,core.VLASize,core.uninitialized.ArraySubscript,core.uninitialized.Assign,core.uninitialized.Branch,core.uninitialized.CapturedBlockVariable,core.uninitialized.UndefReturn,cplusplus.InnerPointer,cplusplus.Move,cplusplus.NewDelete,cplusplus.NewDeleteLeaks,cplusplus.PlacementNew,cplusplus.PureVirtualCall,deadcode.DeadStores,nullability.NullPassedToNonnull,nullability.NullReturnedFromNonnull,osx.API,osx.MIG,osx.NumberObjectConversion,osx.OSObjectRetainCount,osx.ObjCProperty,osx.SecKeychainAPI,osx.cocoa.AtSync,osx.cocoa.AutoreleaseWrite,osx.cocoa.ClassRelease,osx.cocoa.Dealloc,osx.cocoa.IncompatibleMethodTypes,osx.cocoa.Loops,osx.cocoa.MissingSuperCall,osx.cocoa.NSAutoreleasePool,osx.cocoa.NSError,osx.cocoa.NilArg,osx.cocoa.NonNilReturnValue,osx.cocoa.ObjCGenerics,osx.cocoa.RetainCount,osx.cocoa.RunLoopAutoreleaseLeak,osx.cocoa.SelfInit,osx.cocoa.SuperDealloc,osx.cocoa.UnusedIvars,osx.cocoa.VariadicMethodTypes,osx.coreFoundation.CFError,osx.coreFoundation.CFNumber,osx.coreFoundation.CFRetainRelease,osx.coreFoundation.containers.OutOfBounds,osx.coreFoundation.containers.PointerSizedValues,security.insecureAPI.UncheckedReturn,security.insecureAPI.decodeValueOfObjCType,security.insecureAPI.getpw,security.insecureAPI.gets,security.insecureAPI.mkstemp,security.insecureAPI.mktemp,security.insecureAPI.vfork,unix.API,unix.Malloc,unix.MallocSizeof,unix.MismatchedDeallocator,unix.Vfork,unix.cstring.BadSizeArg,unix.cstring.NullArg")

add_custom_target(ci_clang_analyze
  COMMAND CXX=${CLANG_TOOL} ${CMAKE_COMMAND}
    -DCMAKE_BUILD_TYPE=Debug -GNinja
    -DTPH_BuildTests=ON
    -S${PROJECT_SOURCE_DIR} -B${PROJECT_BINARY_DIR}/build_clang_analyze
  COMMAND cd ${PROJECT_BINARY_DIR}/build_clang_analyze && ${SCAN_BUILD_TOOL} -enable-checker ${CLANG_ANALYZER_CHECKS} --use-c++=${CLANG_TOOL} -analyze-headers -o ${PROJECT_BINARY_DIR}/report ninja
  COMMENT "Check code with Clang Analyzer"
)

##
## Check code with Clang-Tidy.
##

add_custom_target(ci_clang_tidy
  COMMAND CXX=${CLANG_TOOL} ${CMAKE_COMMAND}
    -DCMAKE_BUILD_TYPE=Debug -GNinja
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_CLANG_TIDY=${CLANG_TIDY_TOOL}
    -DJSON_BuildTests=ON
    -S${PROJECT_SOURCE_DIR} -B${PROJECT_BINARY_DIR}/build_clang_tidy
  COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR}/build_clang_tidy
  COMMENT "Check code with Clang-Tidy"
)
