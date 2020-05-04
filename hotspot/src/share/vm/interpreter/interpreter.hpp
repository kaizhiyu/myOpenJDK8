/*
 * Copyright (c) 1997, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_VM_INTERPRETER_INTERPRETER_HPP
#define SHARE_VM_INTERPRETER_INTERPRETER_HPP

#include "../code/stubs.hpp"
#include "../interpreter/cppInterpreter.hpp"
#include "../interpreter/templateInterpreter.hpp"
#ifdef ZERO
#ifdef TARGET_ARCH_zero
# include "entry_zero.hpp"
#endif
#endif

// This file contains the platform-independent parts
// of the interpreter and the interpreter generator.

//------------------------------------------------------------------------------------------------------------------------
// An InterpreterCodelet is a piece of interpreter code. All
// interpreter code is generated into little codelets which
// contain extra information for debugging and printing purposes.
// InterpreterCodelet表示一段解释器代码，所有的解释器代码都放在InterpreterCodelet中，同时还包含了额外的用于打印和调试的信息，其定义也是在interpreter.hpp中。InterpreterCodelet继承自Stub
class InterpreterCodelet: public Stub {
  friend class VMStructs;
 private:
  int         _size;                             // the size in bytes  InterpreterCodelet的内存大小
  const char* _description;                      // a description of the codelet, for debugging & printing  当前InterpreterCodelet的描述字符串
  Bytecodes::Code _bytecode;                     // associated bytecode if any  Code是Bytecodes类中定义的一个表示具体字节码指令的枚举，这里表示当前InterpreterCodelet关联的字节码
  DEBUG_ONLY(CodeStrings _strings;)              // Comments for annotating assembler output.

 public:
  // Initialization/finalization
  void    initialize(int size,
                     CodeStrings& strings)       { _size = size;
                                                   DEBUG_ONLY(::new(&_strings) CodeStrings();)
                                                   DEBUG_ONLY(_strings.assign(strings);) }
  void    finalize()                             { ShouldNotCallThis(); }

  // General info/converters
  int     size() const                           { return _size; }
  static  int code_size_to_size(int code_size)   { return round_to(sizeof(InterpreterCodelet), CodeEntryAlignment) + code_size; }

  // Code info
  address code_begin() const                     { return (address)this + round_to(sizeof(InterpreterCodelet), CodeEntryAlignment); }
  address code_end() const                       { return (address)this + size(); }

  // Debugging
  void    verify();
  void    print_on(outputStream* st) const;
  void    print() const { print_on(tty); }

  // Interpreter-specific initialization
  void    initialize(const char* description, Bytecodes::Code bytecode);

  // Interpreter-specific attributes
  int         code_size() const                  { return code_end() - code_begin(); }
  const char* description() const                { return _description; }
  Bytecodes::Code bytecode() const               { return _bytecode; }
};

// Define a prototype interface
DEF_STUB_INTERFACE(InterpreterCodelet);  // InterpreterCodeletInterface是通过宏的方式定义的, 这个类就是将传入的Stub* 将其强转成InterpreterCodelet*，然后调用InterpreterCodelet的方法实现。


//------------------------------------------------------------------------------------------------------------------------
// A CodeletMark serves as an automatic creator/initializer for Codelets
// (As a subclass of ResourceMark it automatically GC's the allocated
// code buffer and assemblers).
// CodeletMark继承自ResourceMark，跟ResourceMark的用法一样，通过构造函数完成相关资源的初始化，通过析构函数完成相关资源的销毁和其他收尾工作。
class CodeletMark: ResourceMark {
 private:
  InterpreterCodelet*         _clet; // 关联的用来描述一段汇编代码的InterpreterCodelet实例，通过StubQueue分配
  InterpreterMacroAssembler** _masm; // 生产字节码指令对应汇编代码的Assembler实例指针
  CodeBuffer                  _cb;   // Assembler实际写入代码的地方，通过InterpreterCodelet来初始化

  int codelet_size() {
    // Request the whole code buffer (minus a little for alignment).  请求整个代码缓冲区
    // The commit call below trims it back for each codelet.  下面的commit调用为每个codelet修剪它
    int codelet_size = AbstractInterpreter::code()->available_space() - 2*K;

    // Guarantee there's a little bit of code space left.  保证还有一点代码空间
    guarantee (codelet_size > 0 && (size_t)codelet_size >  2*K,
               "not enough space for interpreter generation");

    return codelet_size;
  }

 public:
  CodeletMark(
    InterpreterMacroAssembler*& masm,
    const char* description,
    Bytecodes::Code bytecode = Bytecodes::_illegal):
    _clet((InterpreterCodelet*)AbstractInterpreter::code()->request(codelet_size())),
    _cb(_clet->code_begin(), _clet->code_size())

  { // request all space (add some slack for Codelet data)
    assert (_clet != NULL, "we checked not enough space already");

    // initialize Codelet attributes
    _clet->initialize(description, bytecode);
    // create assembler for code generation
    masm  = new InterpreterMacroAssembler(&_cb);
    _masm = &masm;
  }

  ~CodeletMark() {
    // align so printing shows nop's instead of random code at the end (Codelets are aligned)
    (*_masm)->align(wordSize);
    // make sure all code is in code buffer
    (*_masm)->flush();


    // commit Codelet
    AbstractInterpreter::code()->commit((*_masm)->code()->pure_insts_size(), (*_masm)->code()->strings());
    // make sure nobody can use _masm outside a CodeletMark lifespan
    *_masm = NULL;
  }
};

// Wrapper classes to produce Interpreter/InterpreterGenerator from either
// the c++ interpreter or the template interpreter.  Interpreter是对外的一个解释器的包装类，通过宏定义的方式决定使用CppInterpreter或者TemplateInterpreter

class Interpreter: public CC_INTERP_ONLY(CppInterpreter) NOT_CC_INTERP(TemplateInterpreter) {

  public:
  // Debugging/printing
  static InterpreterCodelet* codelet_containing(address pc)     { return (InterpreterCodelet*)_code->stub_containing(pc); }
//#ifdef TARGET_ARCH_x86
# include "interpreter_x86.hpp"
//#endif
#ifdef TARGET_ARCH_sparc
# include "interpreter_sparc.hpp"
#endif
#ifdef TARGET_ARCH_zero
# include "interpreter_zero.hpp"
#endif
#ifdef TARGET_ARCH_arm
# include "interpreter_arm.hpp"
#endif
#ifdef TARGET_ARCH_ppc
# include "interpreter_ppc.hpp"
#endif

};

#endif // SHARE_VM_INTERPRETER_INTERPRETER_HPP
