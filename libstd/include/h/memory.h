/// @file memory.h
/// @data 05/03/2014 18:23:53
/// @author Ambroise Leclerc
/// @brief
//
// Embedded Template Library
// Copyright (c) 2014, Ambroise Leclerc
//   All rights reserved.
//
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in
//     the documentation and/or other materials provided with the
//     distribution.
//   * Neither the name of the copyright holders nor the names of
//     contributors may be used to endorse or promote products derived
//     from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.

#ifndef ETL_LIBSTD_MEMORY_H_
#define ETL_LIBSTD_MEMORY_H_
#include <type_traits>
#include <utility>

namespace std {
  
template<typename T>
class allocator {
 public:
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T value_type;
  typedef T& reference;
  typedef const T& const_reference;
  // construct helper using placement new:
/*  static void construct(reference p, const_reference value) {
    new (&p) T(value); // T must support copy-constructor
  }
  */
  template<typename T2, typename... Args>
  static void construct(T2* p, Args&&... value) {
    new((void*)p) T2(std::forward<Args>(value)...);
  }    

  // destroy helper to invoke destructor explicitly.
  static void destroy(const_reference t) {
    t.~T(); // T must support non-throwing destructor
  }
};

} // namespace std
#endif // ETL_LIBSTD_MEMORY_H_