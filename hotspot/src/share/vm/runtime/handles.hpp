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

#ifndef SHARE_VM_RUNTIME_HANDLES_HPP
#define SHARE_VM_RUNTIME_HANDLES_HPP

#include "../oops/klass.hpp"

//------------------------------------------------------------------------------------------------------------------------
// In order to preserve oops during garbage collection, they should be
// allocated and passed around via Handles within the VM. A handle is
// simply an extra indirection allocated in a thread local handle area.
//
// A handle is a ValueObj, so it can be passed around as a value, can
// be used as a parameter w/o using &-passing, and can be returned as a
// return value.
//
// oop parameters and return types should be Handles whenever feasible.
//
// Handles are declared in a straight-forward manner, e.g.
//
//   oop obj = ...;
//   Handle h1(obj);              // allocate new handle
//   Handle h2(thread, obj);      // faster allocation when current thread is known
//   Handle h3;                   // declare handle only, no allocation occurs
//   ...
//   h3 = h1;                     // make h3 refer to same indirection as h1
//   oop obj2 = h2();             // get handle value
//   h1->print();                 // invoking operation on oop
//
// Handles are specialized for different oop types to provide extra type
// information and avoid unnecessary casting. For each oop type xxxOop
// there is a corresponding handle called xxxHandle, e.g.
//
//   oop           Handle
//   Method*       methodHandle
//   instanceOop   instanceHandle

//------------------------------------------------------------------------------------------------------------------------
// Base class for all handles. Provides overloading of frequently
// used operators for ease of use.   所有句柄的基类。提供常用运算符的重载以便于使用。

class Handle VALUE_OBJ_CLASS_SPEC {
 private:
  oop* _handle;

 protected:
  oop     obj() const                            { return _handle == NULL ? (oop)NULL : *_handle; }
  oop     non_null_obj() const                   { assert(_handle != NULL, "resolving NULL handle"); return *_handle; }

 public:
  // Constructors      构造器
  Handle()                                       { _handle = NULL; }
  Handle(oop obj);
  Handle(Thread* thread, oop obj);

  // General access    一般访问
  oop     operator () () const                   { return obj(); }
  oop     operator -> () const                   { return non_null_obj(); }
  bool    operator == (oop o) const              { return obj() == o; }
  bool    operator == (const Handle& h) const          { return obj() == h.obj(); }

  // Null checks
  bool    is_null() const                        { return _handle == NULL; }
  bool    not_null() const                       { return _handle != NULL; }

  // Debugging
  void    print()                                { obj()->print(); }

  // Direct interface, use very sparingly.                                                        直接接口，使用非常节省。
  // Used by JavaCalls to quickly convert handles and to create handles static data structures.   JavaCalls用于快速转换句柄和创建句柄静态数据结构。
  // Constructor takes a dummy argument to prevent unintentional type conversion in C++.
  Handle(oop *handle, bool dummy)                { _handle = handle; }

  // Raw handle access. Allows easy duplication of Handles. This can be very unsafe
  // since duplicates is only valid as long as original handle is alive.
  oop* raw_value()                               { return _handle; }
  static oop raw_resolve(oop *handle)            { return handle == NULL ? (oop)NULL : *handle; }
};

// Specific Handles for different oop types 对于不同oop类型的专有具柄
#define DEF_HANDLE(type, is_a)                   \
  class type##Handle: public Handle {            \
   protected:                                    \
    type##Oop    obj() const                     { return (type##Oop)Handle::obj(); } \
    type##Oop    non_null_obj() const            { return (type##Oop)Handle::non_null_obj(); } \
                                                 \
   public:                                       \
    /* Constructors */                           \
    type##Handle ()                              : Handle()                 {} \
    type##Handle (type##Oop obj) : Handle((oop)obj) {                         \
      assert(is_null() || ((oop)obj)->is_a(),                                 \
             "illegal type");                                                 \
    }                                                                         \
    type##Handle (Thread* thread, type##Oop obj) : Handle(thread, (oop)obj) { \
      assert(is_null() || ((oop)obj)->is_a(), "illegal type");                \
    }                                                                         \
    \
    /* Operators for ease of use */              \
    type##Oop    operator () () const            { return obj(); } \
    type##Oop    operator -> () const            { return non_null_obj(); } \
  };


DEF_HANDLE(instance         , is_instance         )
DEF_HANDLE(array            , is_array            )
DEF_HANDLE(objArray         , is_objArray         )
DEF_HANDLE(typeArray        , is_typeArray        )

//------------------------------------------------------------------------------------------------------------------------

// Metadata Handles.  Unlike oop Handles these are needed to prevent metadata
// from being reclaimed by RedefineClasses.       元数据句柄。与oop句柄不同，这些句柄用于防止重定义类回收元数据。

// Specific Handles for different oop types       不同oop类型的特定句柄
#define DEF_METADATA_HANDLE(name, type)          \
  class name##Handle;                            \
  class name##Handle : public StackObj {         \
    type*     _value;                            \
    Thread*   _thread;                           \
   protected:                                    \
    type*        obj() const                     { return _value; } \
    type*        non_null_obj() const            { assert(_value != NULL, "resolving NULL _value"); return _value; } \
                                                 \
   public:                                       \
    /* Constructors */                           \
    name##Handle () : _value(NULL), _thread(NULL) {}   \
    name##Handle (type* obj);                    \
    name##Handle (Thread* thread, type* obj);    \
                                                 \
    name##Handle (const name##Handle &h);        \
    name##Handle& operator=(const name##Handle &s); \
                                                 \
    /* Destructor */                             \
    ~name##Handle ();                            \
    void remove();                               \
                                                 \
    /* Operators for ease of use */              \
    type*        operator () () const            { return obj(); } \
    type*        operator -> () const            { return non_null_obj(); } \
                                                 \
    bool    operator == (type* o) const          { return obj() == o; } \
    bool    operator == (const name##Handle& h) const  { return obj() == h.obj(); } \
                                                 \
    /* Null checks */                            \
    bool    is_null() const                      { return _value == NULL; } \
    bool    not_null() const                     { return _value != NULL; } \
  };


DEF_METADATA_HANDLE(method, Method)
DEF_METADATA_HANDLE(constantPool, ConstantPool)

// Writing this class explicitly, since DEF_METADATA_HANDLE(klass) doesn't
// provide the necessary Klass* <-> Klass* conversions. This Klass
// could be removed when we don't have the Klass* typedef anymore.
class KlassHandle : public StackObj {
  Klass* _value;
 protected:
   Klass* obj() const          { return _value; }
   Klass* non_null_obj() const { assert(_value != NULL, "resolving NULL _value"); return _value; }

 public:
   KlassHandle()                                 : _value(NULL) {}
   KlassHandle(const Klass* obj)                 : _value(const_cast<Klass *>(obj)) {};
   KlassHandle(Thread* thread, const Klass* obj) : _value(const_cast<Klass *>(obj)) {};

   Klass* operator () () const { return obj(); }
   Klass* operator -> () const { return non_null_obj(); }

   bool operator == (Klass* o) const             { return obj() == o; }
   bool operator == (const KlassHandle& h) const { return obj() == h.obj(); }

    bool is_null() const  { return _value == NULL; }
    bool not_null() const { return _value != NULL; }
};

class instanceKlassHandle : public KlassHandle {
 public:
  /* Constructors */
  instanceKlassHandle () : KlassHandle() {}
  instanceKlassHandle (const Klass* k) : KlassHandle(k) {
    assert(k == NULL || k->oop_is_instance(),
           "illegal type");
  }
  instanceKlassHandle (Thread* thread, const Klass* k) : KlassHandle(thread, k) {
    assert(k == NULL || k->oop_is_instance(),
           "illegal type");
  }
  /* Access to klass part */
  InstanceKlass*       operator () () const { return (InstanceKlass*)obj(); }
  InstanceKlass*       operator -> () const { return (InstanceKlass*)obj(); }
};


//------------------------------------------------------------------------------------------------------------------------
// Thread local handle area  表示一片专门用于分配Handle的内存区域
class HandleArea: public Arena {
  friend class HandleMark;
  friend class NoHandleMark;
  friend class ResetNoHandleMark;
#ifdef ASSERT
  int _handle_mark_nesting;
  int _no_handle_mark_nesting;
#endif
  HandleArea* _prev;          // link to outer (older) area  表示HandleArea链中前一个HandleArea实例
 public:
  // Constructor
  HandleArea(HandleArea* prev) : Arena(mtThread, Chunk::tiny_size) {
    debug_only(_handle_mark_nesting    = 0);
    debug_only(_no_handle_mark_nesting = 0);
    _prev = prev;
  }

  // Handle allocation
 private:
  oop* real_allocate_handle(oop obj) {
#ifdef ASSERT
    oop* handle = (oop*) (UseMallocOnly ? internal_malloc_4(oopSize) : Amalloc_4(oopSize));
#else
    oop* handle = (oop*) Amalloc_4(oopSize);
#endif
    *handle = obj;
    return handle;
  }
 public:
#ifdef ASSERT
  oop* allocate_handle(oop obj);
#else
  oop* allocate_handle(oop obj) { return real_allocate_handle(obj); }  // 用于分配保存oop的内存
#endif

  // Garbage collection support
  void oops_do(OopClosure* f);  // 垃圾回收时遍历对应引用

  // Number of handles in use
  size_t used() const     { return Arena::used() / oopSize; }  // 获取已经使用的内存大小

  debug_only(bool no_handle_mark_active() { return _no_handle_mark_nesting > 0; })
};


//------------------------------------------------------------------------------------------------------------------------
// Handles are allocated in a (growable) thread local handle area. Deallocation  句柄在（可增长的）线程本地句柄区域中分配。
// is managed using a HandleMark. It should normally not be necessary to use     所有的位置都是用HandleMark来管理的。通常不需要手动使用HandleMark。
// HandleMarks manually.
//
// A HandleMark constructor will record the current handle area top, and the     HandleMark构造函数将记录当前的句柄区域top，
// desctructor will reset the top, destroying all handles allocated in between.  而descructor将重置top，从而销毁在两者之间分配的所有句柄。
// The following code will therefore NOT work:                                   因此，以下代码将不起作用：
//
//   Handle h;
//   {
//     HandleMark hm;
//     h = Handle(obj);
//   }
//   h()->print();       // WRONG, h destroyed by HandleMark destructor.              错了，h被HandleMark毁灭者摧毁了。
//
// If h has to be preserved, it can be converted to an oop or a local JNI handle      如果必须保留h，则可以将其转换为跨越HandleMark边界的oop或本地JNI句柄。
// across the HandleMark boundary.

// The base class of HandleMark should have been StackObj but we also heap allocate   HandleMark的基类应该是StackObj，但是我们也可以在创建线程时堆分配HandleMark。
// a HandleMark when a thread is created. The operator new is for this special case.  新的接线员是这个特殊情况的接线员。

class HandleMark {
 private:
  Thread *_thread;              // thread that owns this mark  拥有这个标记的线程  表示拥有此HandleMark实例的Thread
  HandleArea *_area;            // saved handle area           保存的句柄区域      此HandleMark实例保存的HandleArea
  Chunk *_chunk;                // saved arena chunk           保存的竞技场区块    保存HandleArea的Chunk
  char *_hwm, *_max;            // saved arena info            已保存竞技场信息    保存的HandleArea的信息
  size_t _size_in_bytes;        // size of handle area         handle区域的大小   保存的HandleArea的大小
  // Link to previous active HandleMark in thread              链接到线程中以前的活动句柄标记   当前线程的上一个活跃的HandleMark实例
  HandleMark* _previous_handle_mark;

  void initialize(Thread* thread);                // common code for constructors
  void set_previous_handle_mark(HandleMark* mark) { _previous_handle_mark = mark; }
  HandleMark* previous_handle_mark() const        { return _previous_handle_mark; }

  size_t size_in_bytes() const { return _size_in_bytes; }
 public:
  HandleMark();                            // see handles_inline.hpp
  HandleMark(Thread* thread)                      { initialize(thread); }
  ~HandleMark();

  // Functions used by HandleMarkCleaner
  // called in the constructor of HandleMarkCleaner   HandleMarkCleaner的构造函数中调用的HandleMarkCleaner使用的函数
  void push();
  // called in the destructor of HandleMarkCleaner    调用了HandleMarkCleaner的析构函数
  void pop_and_restore();
  // overloaded operators
  void* operator new(size_t size) throw();
  void* operator new [](size_t size) throw();
  void operator delete(void* p);
  void operator delete[](void* p);
};

//------------------------------------------------------------------------------------------------------------------------
// A NoHandleMark stack object will verify that no handles are allocated
// in its scope. Enabled in debug mode only.   NoHandleMark堆栈对象将验证其作用域中没有分配句柄。仅在调试模式下启用。

class NoHandleMark: public StackObj {
 public:
#ifdef ASSERT
  NoHandleMark();
  ~NoHandleMark();
#else
  NoHandleMark()  {}
  ~NoHandleMark() {}
#endif
};


class ResetNoHandleMark: public StackObj {
  int _no_handle_mark_nesting;
 public:
#ifdef ASSERT
  ResetNoHandleMark();
  ~ResetNoHandleMark();
#else
  ResetNoHandleMark()  {}
  ~ResetNoHandleMark() {}
#endif
};

#endif // SHARE_VM_RUNTIME_HANDLES_HPP
