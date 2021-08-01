/*
 * Copyright (c) 1998, 2010, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_VM_RUNTIME_BASICLOCK_HPP
#define SHARE_VM_RUNTIME_BASICLOCK_HPP

#include "../oops/markOop.hpp"
#include "../runtime/handles.hpp"
#include "../utilities/top.hpp"

class BasicLock VALUE_OBJ_CLASS_SPEC { // BasicLock主要用于保存对象的对象头
  friend class VMStructs;
 private:
  volatile markOop _displaced_header;
 public:
  markOop      displaced_header() const               { return _displaced_header; }
  void         set_displaced_header(markOop header)   { _displaced_header = header; }

  void print_on(outputStream* st) const;

  // move a basic lock (used during deoptimization
  void move_to(oop obj, BasicLock* dest);

  static int displaced_header_offset_in_bytes()       { return offset_of(BasicLock, _displaced_header); }
};

// A BasicObjectLock associates a specific Java object with a BasicLock.
// It is currently embedded in an interpreter frame.

// Because some machines have alignment restrictions on the control stack,
// the actual space allocated by the interpreter may include padding words
// after the end of the BasicObjectLock.  Also, in order to guarantee
// alignment of the embedded BasicLock objects on such machines, we
// put the embedded BasicLock at the beginning of the struct.
// BasicLock主要用于保存对象的对象头
// BasicObjectLock是内嵌在线程的调用栈帧中的，有一段连续的内存区域用来保存多个BasicObjectLock，这个内存区域的起始位置是保存在栈帧中的特定偏移处的，
// 终止地址（低地址端）保存interpreter_frame_monitor_block_top_offset偏移处，起始地址（高地址端）保存在interpreter_frame_initial_sp_offset偏移处。
// monitorenter方法会遍历用于分配BasicObjectLock的连续内存区域，注意是从低地址端往高地址端遍历，即按照BasicObjectLock的分配顺序倒序遍历，先遍历最近分配的BasicObjectLock。
// 遍历过程中，如果找到一个obj属性就是目标对象的BasicObjectLock，则停止遍历重新分配一个新的BasicObjectLock；
// 如果没有找到obj属性相同的且没有空闲的，同样重新分配一个新的BasicObjectLock；如果没有obj属性相同的且有空闲的，则遍历完所有的BasicObjectLock，找到最上面的地址最高的一个空闲BasicObjectLock，
// 总而言之就是要确保新分配的BasicObjectLock一定要在之前分配的obj属性是同一个对象的BasicObjectLock的后面，因为解锁时也是倒序查找的，找到一个obj属性相同的视为查找成功。
// 获取BasicObjectLock后就将当前对象保存进obj属性，然后调用lock_object获取锁。在默认开启UseBiasedLocking时，lock_object会先获取偏向锁，如果已经获取了则升级成轻量级锁，如果已经获取了轻量级锁则升级成重量级锁。
class BasicObjectLock VALUE_OBJ_CLASS_SPEC {
  friend class VMStructs;
 private:
  BasicLock _lock;                                    // the lock, must be double word aligned BasicLock类型_lock对象主要用来保存_obj指向Object对象的对象头数据
  oop       _obj;                                     // object holds the lock;

 public:
  // Manipulation
  oop      obj() const                                { return _obj;  }
  void set_obj(oop obj)                               { _obj = obj; }
  BasicLock* lock()                                   { return &_lock; }

  // Note: Use frame::interpreter_frame_monitor_size() for the size of BasicObjectLocks
  //       in interpreter activation frames since it includes machine-specific padding.
  static int size()                                   { return sizeof(BasicObjectLock)/wordSize; }

  // GC support
  void oops_do(OopClosure* f) { f->do_oop(&_obj); }

  static int obj_offset_in_bytes()                    { return offset_of(BasicObjectLock, _obj);  }
  static int lock_offset_in_bytes()                   { return offset_of(BasicObjectLock, _lock); }
};


#endif // SHARE_VM_RUNTIME_BASICLOCK_HPP
