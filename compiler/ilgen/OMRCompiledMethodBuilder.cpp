/*******************************************************************************
 * Copyright (c) 2016, 2018 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include <iostream>
#include <fstream>

#include "env/TRMemory.hpp"    // must precede IlBuilder.hpp to get TR_ALLOC

#include "compile/Compilation.hpp"

#include "ilgen/IlConst.hpp"
#include "ilgen/CompiledMethodBuilder.hpp"
#include "ilgen/MethodBuilder.hpp"
#include "ilgen/VirtualMachineRegister.hpp"
#include "ilgen/VirtualMachineState.hpp"

#define TraceEnabled    (comp()->getOption(TR_TraceILGen))
#define TraceIL(m, ...) {if (TraceEnabled) {traceMsg(comp(), m, ##__VA_ARGS__);}}

OMR::CompiledMethodBuilder::CompiledMethodBuilder(TR::TypeDictionary *types, void *bytecodeStream, int32_t bytecodeSizeInBytes)
   : TR::RuntimeBuilder(types),
   _pc(0),
   _opcodes(bytecodeStream),
   _opcodeSizeInBytes(bytecodeSizeInBytes),
   _builders()
   {
   _pInt64 = types->PointerTo(Int64);
   _pInt32 = types->PointerTo(Int32);
   _pInt16 = types->PointerTo(Int16);
   _pInt8 = types->PointerTo(Int8);
   }

OMR::CompiledMethodBuilder::~CompiledMethodBuilder()
   {
   }

bool
OMR::CompiledMethodBuilder::buildIL()
   {
   Setup();

   AppendBuilder(getBuilder(0));

   std::map<int32_t, HandlerData> *handlers = getHandlers();
   int32_t bytecodeIndex = GetNextBytecodeFromWorklist();
   while (-1 != bytecodeIndex)
      {
      TR::BytecodeBuilder *builder = getBuilder(bytecodeIndex);
      TR_ASSERT(bytecodeIndex == builder->bcIndex(), "bytecodeIndex %d must == builder->bcIndex() %d", bytecodeIndex, builder->bcIndex());

      int32_t bytecode = getBytecodeAtPC(bytecodeIndex);
      _pc = bytecodeIndex;

      std::map<int32_t, HandlerData>::iterator it = handlers->find(bytecode);
      if (it == handlers->end())
         {
         fprintf(stderr, "JitMethodBuilder reached an unknown bytecode (%d) at index %d... aborting compilation\n", bytecode, bytecodeIndex);
         return false;
         }

      TraceIL("BYTECODE %s %d\n", it->second.bytecodeName, bytecodeIndex);

      HandlerFunction *func = (HandlerFunction *)it->second.handler;
      func(this->client(), builder->client());

      bytecodeIndex = GetNextBytecodeFromWorklist();
      }

   Return(ConstInt64(-1));
   return true;
   }

void *
OMR::CompiledMethodBuilder::client()
   {
   if (_client == NULL && _clientAllocator != NULL)
      _client = _clientAllocator(static_cast<TR::CompiledMethodBuilder *>(this));
   return _client;
   }

void
OMR::CompiledMethodBuilder::DefaultFallthrough(TR::IlBuilder *builder, TR::IlValue *currentBytecodeSize)
   {
   TR::BytecodeBuilder *bb = getBuilder(_pc);
   TR_ASSERT(_pc == bb->bcIndex(), "CompiledMethodBuilder _pc %d must == bb->bcIndex() %d", _pc, bb->bcIndex());
   TR::IlConst *konst = builder->ToIlConst(currentBytecodeSize);
   bb->AddFallThroughBuilder(getBuilder(_pc + (int32_t)konst->get64bitIntegralValue()));
   }

void
OMR::CompiledMethodBuilder::Jump(TR::IlBuilder *builder, TR::IlValue *target, bool absolute)
   {
   TR::BytecodeBuilder *bb = getBuilder(_pc);
   TR_ASSERT(_pc == bb->bcIndex(), "CompiledMethodBuilder _pc %d must == bb->bcIndex() %d", _pc, bb->bcIndex());
   TR::IlConst *konst = builder->ToIlConst(target);
   int32_t gotoPC = (int32_t)konst->get64bitIntegralValue();
   if (!absolute)
       gotoPC = gotoPC + _pc;

   bb->Goto(getBuilder(gotoPC));
   }
void
OMR::CompiledMethodBuilder::JumpIfOrFallthrough(TR::IlBuilder *builder, TR::IlValue *condition, TR::IlValue *target, TR::IlValue *currentBytecodeSize, bool absolute)
   {
   TR::BytecodeBuilder *bb = getBuilder(_pc);
   TR_ASSERT(_pc == bb->bcIndex(), "CompiledMethodBuilder _pc %d must == bb->bcIndex() %d", _pc, bb->bcIndex());

   TR::IlConst *targetConst = builder->ToIlConst(target);
   TR::IlConst *konst = builder->ToIlConst(currentBytecodeSize);
   int32_t gotoPC = (int32_t)targetConst->get64bitIntegralValue();
   if (!absolute)
       gotoPC = gotoPC + _pc;

   bb->IfCmpNotEqualZero(getBuilder(gotoPC), condition);
   bb->AddFallThroughBuilder(getBuilder(_pc + (int32_t)konst->get64bitIntegralValue()));
   }

TR::VirtualMachineState *
OMR::CompiledMethodBuilder::GetVMState(TR::IlBuilder *builder)
   {
#if 1
   TR::BytecodeBuilder *bb = getBuilder(_pc);
   return bb->vmState();
#else
    return NULL;
#endif
   }

TR::IlValue *
OMR::CompiledMethodBuilder::GetInt64Immediate(TR::IlBuilder *builder, TR::IlValue *offset)
   {
   int8_t *data = (int8_t *)_opcodes;
   TR::IlConst *knost = builder->ToIlConst(offset);
   int64_t intOffset = knost->get64bitIntegralValue();
   int64_t value = *((int64_t *)&data[_pc + intOffset]);
   return builder->ConstInt64(value);
   }

TR::IlValue *
OMR::CompiledMethodBuilder::GetInt32Immediate(TR::IlBuilder *builder, TR::IlValue *offset)
   {
   int8_t *data = (int8_t *)_opcodes;
   TR::IlConst *knost = builder->ToIlConst(offset);
   int64_t intOffset = knost->get64bitIntegralValue();
   int32_t value = *((int32_t *)&data[_pc + intOffset]);
   return builder->ConstInt32(value);
   }

TR::IlValue *
OMR::CompiledMethodBuilder::GetInt16Immediate(TR::IlBuilder *builder, TR::IlValue *offset)
   {
   int8_t *data = (int8_t *)_opcodes;
   TR::IlConst *knost = builder->ToIlConst(offset);
   int64_t intOffset = knost->get64bitIntegralValue();
   int16_t value = *((int16_t *)&data[_pc + intOffset]);
   return builder->ConstInt16(value);
   }

TR::IlValue *
OMR::CompiledMethodBuilder::GetInt8Immediate(TR::IlBuilder *builder, TR::IlValue *offset)
   {
   int8_t *data = (int8_t *)_opcodes;
   TR::IlConst *knost = builder->ToIlConst(offset);
   int64_t intOffset = knost->get64bitIntegralValue();
   int8_t value = *((int8_t *)&data[_pc + intOffset]);
   return builder->ConstInt8(value);
   }


ClientAllocator OMR::CompiledMethodBuilder::_clientAllocator = NULL;
ClientAllocator OMR::CompiledMethodBuilder::_getImpl = NULL;
