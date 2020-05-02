/*
 * Copyright (c) 1998, 2012, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_VM_RUNTIME_JNIHANDLES_HPP
#define SHARE_VM_RUNTIME_JNIHANDLES_HPP

#include "../runtime/handles.hpp"
#include "../utilities/top.hpp"

class JNIHandleBlock;


// Interface for creating and resolving local/global JNI handles 用于创建和解析 本地或全局 JNI句柄的接口

class JNIHandles : AllStatic {
  friend class VMStructs;
 private:
  static JNIHandleBlock* _global_handles;             // First global handle block 保存全局引用的JNIHandleBlock链表的头元素  全局引用实际由JNIHandles的静态属性_global_handle保存，从而可以确保这部分引用不会随着方法调用结束而销毁，只能手动调用DeleteGlobalRef才能释放。
  static JNIHandleBlock* _weak_global_handles;        // First weak global handle block 保存全局弱引用的JNIHandleBlock链表的头元素
  static oop _deleted_handle;                         // Sentinel marking deleted handles 表示已删除的Handle，Handle就是引用

public:
  // Resolve handle into oop
  inline static oop resolve(jobject handle);
  // Resolve externally provided handle into oop with some guards
  inline static oop resolve_external_guard(jobject handle);
  // Resolve handle into oop, result guaranteed not to be null
  inline static oop resolve_non_null(jobject handle);

  // Local handles
  static jobject make_local(oop obj);
  static jobject make_local(JNIEnv* env, oop obj);    // Fast version when env is known
  static jobject make_local(Thread* thread, oop obj); // Even faster version when current thread is known
  inline static void destroy_local(jobject handle);

  // Global handles
  static jobject make_global(Handle  obj);
  static void destroy_global(jobject handle);

  // Weak global handles
  static jobject make_weak_global(Handle obj);
  static void destroy_weak_global(jobject handle);

  // Sentinel marking deleted handles in block. Note that we cannot store NULL as
  // the sentinel, since clearing weak global JNI refs are done by storing NULL in
  // the handle. The handle may not be reused before destroy_weak_global is called.
  static oop deleted_handle()   { return _deleted_handle; }

  // Initialization
  static void initialize();

  // Debugging
  static void print_on(outputStream* st);
  static void print()           { print_on(tty); }
  static void verify();
  static bool is_local_handle(Thread* thread, jobject handle);
  static bool is_frame_handle(JavaThread* thr, jobject obj);
  static bool is_global_handle(jobject handle);
  static bool is_weak_global_handle(jobject handle);
  static long global_handle_memory_usage();
  static long weak_global_handle_memory_usage();

  // Garbage collection support(global handles only, local handles are traversed from thread)
  // Traversal of regular global handles
  static void oops_do(OopClosure* f);
  // Traversal of weak global handles. Unreachable oops are cleared.
  static void weak_oops_do(BoolObjectClosure* is_alive, OopClosure* f);
};



// JNI handle blocks holding local/global JNI handles
//  JNIHandleBlock是实际用于保存JNI本地或者全局引用的地方
class JNIHandleBlock : public CHeapObj<mtInternal> {  // CHeapObj类表示通过C语言中free & malloc方法管理的对象，JVM中所有此类对象都会继承CHeapObj，该类重载了new和delete运算符。
  friend class VMStructs;
  friend class CppInterpreter;

 private:
  enum SomeConstants {
    block_size_in_oops  = 32                    // Number of handles per handle block
  };

  oop             _handles[block_size_in_oops]; // The handles  _handles：实际保存oop的数组，初始大小为32
  int             _top;                         // Index of next unused handle  _handles中下一个未使用的位置的索引
  JNIHandleBlock* _next;                        // Link to next block     下一个JNIHandleBlock指针

  // The following instance variables are only used by the first block in a chain.
  // Having two types of blocks complicates the code and the space overhead in negligble.  下列四个实例属性只是JNIHandleBlock链表中第一个JNIHandleBlock（即链表最前面的）使用，这里为了避免另外定义一个类使得代码复杂化
  JNIHandleBlock* _last;                        // Last block in use  最近使用的JNIHandleBlock实例
  JNIHandleBlock* _pop_frame_link;              // Block to restore on PopLocalFrame call  调用PopLocalFrame时需要恢复的JNIHandleBlock实例
  oop*            _free_list;                   // Handle free list _handles中空闲的元素
  int             _allocate_before_rebuild;     // Number of blocks to allocate before rebuilding free list  在重建前已经分配的JNIHandleBlock的实例个数

  #ifndef PRODUCT
  JNIHandleBlock* _block_list_link;             // Link for list below  下面列表的链接
  static JNIHandleBlock* _block_list;           // List of all allocated blocks (for debugging only)  所有已分配块的列表（仅用于调试）
#endif

  static JNIHandleBlock* _block_free_list;      // Free list of currently unused blocks  还有空闲空间的JNIHandleBlock链表
  static int      _blocks_allocated;            // For debugging/printing 已经分配的JNIHandleBlock实例个数

  // Fill block with bad_handle values  将_handles数组中空闲元素填充为bad_handle
  void zap();

 protected:
  // No more handles in the both the current and following blocks  打标，表明当前Block和后面的任何Block不包含任何oop
  void clear() { _top = 0; }

 private:
  // Free list computation  空闲链表计算
  void rebuild_free_list();

 public:
  // Handle allocation  分配oop Handle
  jobject allocate_handle(oop obj);

  // Block allocation and block free list management
  static JNIHandleBlock* allocate_block(Thread* thread = NULL);   // JNIHandleBlock的创建
  static void release_block(JNIHandleBlock* block, Thread* thread = NULL);  // 已分配的JNIHandleBlock是可以不断循环利用的，所谓的释放只是将其标记为free而已，并没有释放其占用的堆存。

  // JNI PushLocalFrame/PopLocalFrame support
  JNIHandleBlock* pop_frame_link() const          { return _pop_frame_link; }
  void set_pop_frame_link(JNIHandleBlock* block)  { _pop_frame_link = block; }

  // Stub generator support 桩代码生成支持
  static int top_offset_in_bytes()                { return offset_of(JNIHandleBlock, _top); }

  // Garbage collection support
  // Traversal of regular handles  垃圾回收使用的方法，以JNIHandleBlock保存的oop为根对象遍历所有引用的对象
  void oops_do(OopClosure* f);
  // Traversal of weak handles. Unreachable oops are cleared.  弱引用的遍历，不可达的对象将被清理
  void weak_oops_do(BoolObjectClosure* is_alive, OopClosure* f);

  // Debugging
  bool chain_contains(jobject handle) const;    // Does this block or following blocks contain handle
  bool contains(jobject handle) const;          // Does this block contain handle
  int length() const;                           // Length of chain starting with this block
  long memory_usage() const;
  #ifndef PRODUCT
  static bool any_contains(jobject handle);     // Does any block currently in use contain handle
  static void print_statistics();
  #endif
};


inline oop JNIHandles::resolve(jobject handle) {
  oop result = (handle == NULL ? (oop)NULL : *(oop*)handle);
  assert(result != NULL || (handle == NULL || !CheckJNICalls || is_weak_global_handle(handle)), "Invalid value read from jni handle");
  assert(result != badJNIHandle, "Pointing to zapped jni handle area");
  return result;
};


inline oop JNIHandles::resolve_external_guard(jobject handle) {
  if (handle == NULL) return NULL;
  oop result = *(oop*)handle;
  if (result == NULL || result == badJNIHandle) return NULL;
  return result;
};


inline oop JNIHandles::resolve_non_null(jobject handle) {
  assert(handle != NULL, "JNI handle should not be null");
  oop result = *(oop*)handle;
  assert(result != NULL, "Invalid value read from jni handle");
  assert(result != badJNIHandle, "Pointing to zapped jni handle area");
  // Don't let that private _deleted_handle object escape into the wild.
  assert(result != deleted_handle(), "Used a deleted global handle.");
  return result;
};

inline void JNIHandles::destroy_local(jobject handle) {
  if (handle != NULL) {
    *((oop*)handle) = deleted_handle(); // Mark the handle as deleted, allocate will reuse it
  }
}

#endif // SHARE_VM_RUNTIME_JNIHANDLES_HPP
