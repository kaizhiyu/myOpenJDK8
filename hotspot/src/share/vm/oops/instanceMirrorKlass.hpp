/*
 * Copyright (c) 2011, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_VM_OOPS_INSTANCEMIRRORKLASS_HPP
#define SHARE_VM_OOPS_INSTANCEMIRRORKLASS_HPP

#include "../classfile/systemDictionary.hpp"
#include "../oops/instanceKlass.hpp"
#include "../runtime/handles.hpp"
#include "../utilities/macros.hpp"

// An InstanceMirrorKlass is a specialized InstanceKlass for    InstanceMirrorKlass是为了java.lang.Class实例定制的InstanceKlass
// java.lang.Class instances.  These instances are special because  这些实例是特殊的，因为它们除了包含类的普通字段外，还包含类的静态字段。
// they contain the static fields of the class in addition to the
// normal fields of Class.  This means they are variable sized      这意味着它们是可变大小的实例，需要特殊的逻辑来计算它们的大小和迭代它们的oop。
// instances and need special logic for computing their size and for
// iteration of their oops.
/**
 * 类加载的最终结果便是在JVM的方法区创建一个与java类对等的instanceKlass实例对象，但是在JVM创建完instanceKlass之后，又创建了与之对等的另一个镜像类--java.lang.Clas。
 * 所谓的mirror镜像类，其实也是instanceKlass的一个实例对象，SystemDictionary::Class_klass()返回的便是java_lang_Class类型，因此instanceMirrorKlass::cast(SystemDictionary::Class_klass())->allocate_instance(k, CHECK_0)
 * 这行代码就是用来创建java.lang.Class这个Java 类型在JVM内部对等的instances实例的。
 */

class InstanceMirrorKlass: public InstanceKlass {
  friend class VMStructs;
  friend class InstanceKlass;

 private:
  static int _offset_of_static_fields;

  // Constructor
  InstanceMirrorKlass(int vtable_len, int itable_len, int static_field_size, int nonstatic_oop_map_size, ReferenceType rt, AccessFlags access_flags,  bool is_anonymous)
    : InstanceKlass(vtable_len, itable_len, static_field_size, nonstatic_oop_map_size, rt, access_flags, is_anonymous) {}

 public:
  InstanceMirrorKlass() { assert(DumpSharedSpaces || UseSharedSpaces, "only for CDS"); }
  // Type testing
  bool oop_is_instanceMirror() const             { return true; }

  // Casting from Klass*
  static InstanceMirrorKlass* cast(Klass* k) {
    assert(k->oop_is_instanceMirror(), "cast to InstanceMirrorKlass");
    return (InstanceMirrorKlass*) k;
  }

  // Returns the size of the instance including the extra static fields.
  virtual int oop_size(oop obj) const;

  // Static field offset is an offset into the Heap, should be converted by
  // based on UseCompressedOop for traversal  静态字段偏移量是堆中的一个偏移量，应根据UseCompressedOop转换为遍历
  static HeapWord* start_of_static_fields(oop obj) {
    return (HeapWord*)(cast_from_oop<intptr_t>(obj) + offset_of_static_fields());
  }

  static void init_offset_of_static_fields() {
    // Cache the offset of the static fields in the Class instance  缓存类实例中静态字段的偏移量
    assert(_offset_of_static_fields == 0, "once");
    _offset_of_static_fields = InstanceMirrorKlass::cast(SystemDictionary::Class_klass())->size_helper() << LogHeapWordSize;
  }

  static int offset_of_static_fields() {
    return _offset_of_static_fields;
  }

  int compute_static_oop_field_count(oop obj);

  // Given a Klass return the size of the instance
  int instance_size(KlassHandle k);

  // allocation
  instanceOop allocate_instance(KlassHandle k, TRAPS);

  // Garbage collection
  int  oop_adjust_pointers(oop obj);
  void oop_follow_contents(oop obj);

  // Parallel Scavenge and Parallel Old
  PARALLEL_GC_DECLS

  int oop_oop_iterate(oop obj, ExtendedOopClosure* blk) {
    return oop_oop_iterate_v(obj, blk);
  }
  int oop_oop_iterate_m(oop obj, ExtendedOopClosure* blk, MemRegion mr) {
    return oop_oop_iterate_v_m(obj, blk, mr);
  }

#define InstanceMirrorKlass_OOP_OOP_ITERATE_DECL(OopClosureType, nv_suffix)           \
  int oop_oop_iterate##nv_suffix(oop obj, OopClosureType* blk);                       \
  int oop_oop_iterate##nv_suffix##_m(oop obj, OopClosureType* blk, MemRegion mr);

  ALL_OOP_OOP_ITERATE_CLOSURES_1(InstanceMirrorKlass_OOP_OOP_ITERATE_DECL)
  ALL_OOP_OOP_ITERATE_CLOSURES_2(InstanceMirrorKlass_OOP_OOP_ITERATE_DECL)

#if INCLUDE_ALL_GCS
#define InstanceMirrorKlass_OOP_OOP_ITERATE_BACKWARDS_DECL(OopClosureType, nv_suffix) \
  int oop_oop_iterate_backwards##nv_suffix(oop obj, OopClosureType* blk);

  ALL_OOP_OOP_ITERATE_CLOSURES_1(InstanceMirrorKlass_OOP_OOP_ITERATE_BACKWARDS_DECL)
  ALL_OOP_OOP_ITERATE_CLOSURES_2(InstanceMirrorKlass_OOP_OOP_ITERATE_BACKWARDS_DECL)
#endif // INCLUDE_ALL_GCS
};

#endif // SHARE_VM_OOPS_INSTANCEMIRRORKLASS_HPP
