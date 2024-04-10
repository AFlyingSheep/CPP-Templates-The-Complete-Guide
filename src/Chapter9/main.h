#include <iostream>

// We include the definitions of a template in the header file that declares
// that template to avoid link error.

// But sometimes it will increase length of header (because sometimes we should
// include other header such as <iostream>.)

// We have two methods to solve this question: precompiled header and explicit
// template instantiation.

// Also, we can use "module" in c++20.

// Full specializations of function templates act like ordinary functions in
// this regard: Their definition can appear only once unless they’re defined
// inline.

// Precompileed header: https://www.cnblogs.com/BBOOT/p/3771451.html

template <typename T> void f();
template <> void f<int>();
// extern template void f<int>();

// manual explicit instantiation
// Error: explicit instantiation of undefined function template 'f'
// We should let compiler see the implement of f<int> or f<>
// extern template void f<int>();

// An explicit instantiation declaration generally suppresses automatic
// instantiation of the named template specialization, but have some exceptions:
