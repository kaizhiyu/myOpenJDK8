/*
 * Copyright (c) 1997, 2014, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef SHARE_VM_CLASSFILE_CLASSFILESTREAM_HPP
#define SHARE_VM_CLASSFILE_CLASSFILESTREAM_HPP

#include "../utilities/top.hpp"
//#ifdef TARGET_ARCH_x86
# include "bytes_x86.hpp"
//#endif
//#ifdef TARGET_ARCH_sparc
//# include "bytes_sparc.hpp"
//#endif
//#ifdef TARGET_ARCH_zero
//# include "bytes_zero.hpp"
//#endif
//#ifdef TARGET_ARCH_arm
//# include "bytes_arm.hpp"
//#endif
//#ifdef TARGET_ARCH_ppc
//# include "bytes_ppc.hpp"
//#endif

// Input stream for reading .class file  用于读取.class文件的输入流
//
// The entire input stream is present in a buffer allocated by the caller. 整个输入流都存在于调用者分配的缓冲区中。
// The caller is responsible for deallocating the buffer and for using
// ResourceMarks appropriately when constructing streams.  调用方负责释放缓冲区，并在构造流时适当地使用资源标记。

class ClassFileStream: public ResourceObj {
 private:
  u1*   _buffer_start; // Buffer bottom  缓存底部
  u1*   _buffer_end;   // Buffer top (one past last element) 缓冲区顶部（一个过去的最后一个元素）
  u1*   _current;      // Current buffer position  当前缓存位置
  const char* _source; // Source of stream (directory name, ZIP/JAR archive name) 流源（目录名，ZIP/JAR存档名）
  bool  _need_verify;  // True if verification is on for the class file  如果对类文件启用验证，则为True

  void truncated_file_error(TRAPS);
 public:
  // Constructor
  ClassFileStream(u1* buffer, int length, const char* source);

  // Buffer access
  u1* buffer() const           { return _buffer_start; }
  int length() const           { return _buffer_end - _buffer_start; }
  u1* current() const          { return _current; }
  void set_current(u1* pos)    { _current = pos; }
  const char* source() const   { return _source; }
  void set_verify(bool flag)   { _need_verify = flag; }

  void check_truncated_file(bool b, TRAPS) {
    if (b) {
      truncated_file_error(THREAD);
    }
  }

  void guarantee_more(int size, TRAPS) {
    size_t remaining = (size_t)(_buffer_end - _current);
    unsigned int usize = (unsigned int)size;
    check_truncated_file(usize > remaining, CHECK);
  }

  // Read u1 from stream 从流中读取u1
  u1 get_u1(TRAPS);
  u1 get_u1_fast() {
    return *_current++;
  }

  // Read u2 from stream 从流中读取u2
  u2 get_u2(TRAPS);
  u2 get_u2_fast() {
    u2 res = Bytes::get_Java_u2(_current);
    _current += 2;
    return res;
  }

  // Read u4 from stream
  u4 get_u4(TRAPS);
  u4 get_u4_fast() {
    u4 res = Bytes::get_Java_u4(_current);
    _current += 4;
    return res;
  }

  // Read u8 from stream
  u8 get_u8(TRAPS);
  u8 get_u8_fast() {
    u8 res = Bytes::get_Java_u8(_current);
    _current += 8;
    return res;
  }

  // Get direct pointer into stream at current position.  在当前位置获取指向流的直接指针。
  // Returns NULL if length elements are not remaining. The caller is
  // responsible for calling skip below if buffer contents is used.  如果长度元素不存在，则返回NULL。如果使用缓冲区内容，则调用方负责调用下面的跳过。
  u1* get_u1_buffer() {
    return _current;
  }

  u2* get_u2_buffer() {
    return (u2*) _current;
  }

  // Skip length u1 or u2 elements from stream
  void skip_u1(int length, TRAPS);
  void skip_u1_fast(int length) {
    _current += length;
  }

  void skip_u2(int length, TRAPS);
  void skip_u2_fast(int length) {
    _current += 2 * length;
  }

  void skip_u4(int length, TRAPS);
  void skip_u4_fast(int length) {
    _current += 4 * length;
  }

  // Tells whether eos is reached
  bool at_eos() const          { return _current == _buffer_end; }
};

#endif // SHARE_VM_CLASSFILE_CLASSFILESTREAM_HPP
