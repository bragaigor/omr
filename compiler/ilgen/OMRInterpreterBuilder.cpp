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
#include "ilgen/InterpreterBuilder.hpp"
#include "ilgen/MethodBuilder.hpp"
#include "ilgen/VirtualMachineRegister.hpp"
#include "ilgen/VirtualMachineState.hpp"

#define TraceEnabled    (comp()->getOption(TR_TraceILGen))
#define TraceIL(m, ...) {if (TraceEnabled) {traceMsg(comp(), m, ##__VA_ARGS__);}}

void
handleBadOpcode(int32_t opcode)
   {
   #define HANDLE_BAD_OPCODE_LINE LINETOSTR(__LINE__)
   fprintf(stderr,"Unknown opcode %d halting the runtime\n", opcode);
   exit(opcode);
   }

OMR::InterpreterBuilder::InterpreterBuilder(TR::TypeDictionary *types)
   : TR::RuntimeBuilder(types),
   _pcRegister(NULL),
   _initialPC(NULL)
   {
   _pInt64 = types->PointerTo(Int64);
   _pInt32 = types->PointerTo(Int32);
   _pInt16 = types->PointerTo(Int16);
   _pInt8 = types->PointerTo(Int8);

   DefineFunction((char *)"InterpreterBuilder::handleBadOpcode",
                  (char *)__FILE__,
                  (char *)HANDLE_BAD_OPCODE_LINE,
                  (void *)&handleBadOpcode,
                  NoType,
                  1,
                  Int32);
   }

OMR::InterpreterBuilder::~InterpreterBuilder()
   {
   }

bool
OMR::InterpreterBuilder::buildIL()
   {
   Setup();

   _initialPC = _pcRegister->Load(this);

   Store("pc", LoadAt(_pInt8, _pcRegister->Load(this)));
   Store("pcInt", ConvertTo(Int32, Load("pc")));

   std::map<int32_t, HandlerData> *handlers = getHandlers();
   std::map<int32_t, HandlerData>::iterator begin = handlers->begin();
   std::map<int32_t, HandlerData>::reverse_iterator end = handlers->rbegin();

   int32_t low = begin->first;
   int32_t high = end->first;
   int32_t caseCount = (high - low) + 1;

   JBCase **cases = (JBCase **) _comp->trMemory()->allocateHeapMemory(caseCount * sizeof(JBCase *));
   TR_ASSERT(NULL != cases, "out of memory");
   IlBuilder **builders = (IlBuilder **) _comp->trMemory()->allocateHeapMemory(caseCount * sizeof(IlBuilder *));
   TR_ASSERT(NULL != builders, "out of memory");
   int32_t kase = low;
   for (int32_t i = 0; i < caseCount; i++, kase++)
      cases[i] = MakeCase(kase, &builders[i], false);

   IlBuilder *defaultBldr = NULL;
   TableSwitch("pcInt", &defaultBldr, caseCount, cases);

   kase = low;
   for (int32_t i = 0; i < caseCount; i++, kase++)
      {
      int64_t didReturn = 0;
      std::map<int32_t, HandlerData>::iterator it = handlers->find(i);
      if (it == handlers->end())
         {
         TraceIL("BYTECODE unknown %d\n", i);
         builders[i]->Call("InterpreterBuilder::handleBadOpcode", 1, builders[i]->ConstInt32(i));
         }
      else
         {
         TraceIL("BYTECODE %s %d\n", it->second.bytecodeName, i);
         HandlerFunction *func = (HandlerFunction *)it->second.handler;
         didReturn = func(this->client(), builders[i]->client());
         }
      if (0 == didReturn)
         {
         builders[i]->Store("pc", builders[i]->LoadAt(_pInt8, _pcRegister->Load(builders[i])));
         builders[i]->Store("pcInt", builders[i]->ConvertTo(Int32, builders[i]->Load("pc")));
         builders[i]->ComputedGoto("pcInt", &defaultBldr, caseCount, cases);
         }
      }

   //TODO remove when computed goto API is fixed
   Return(ConstInt64(-1));
   return true;
   }

void *
OMR::InterpreterBuilder::client()
   {
   if (_client == NULL && _clientAllocator != NULL)
      _client = _clientAllocator(static_cast<TR::InterpreterBuilder *>(this));
   return _client;
   }

void
OMR::InterpreterBuilder::DefaultFallthrough(TR::IlBuilder *builder, TR::IlValue *currentBytecodeSize)
   {
    _pcRegister->Adjust(builder, currentBytecodeSize);
   }

void
OMR::InterpreterBuilder::Jump(TR::IlBuilder *builder, TR::IlValue *target, bool absolute)
   {
   if (absolute)
      {
      TR::IlValue *newPC = builder->Add(_initialPC, target);
      _pcRegister->Store(builder, newPC);
      }
   else
      _pcRegister->Adjust(builder, target);
   }
void
OMR::InterpreterBuilder::JumpIfOrFallthrough(TR::IlBuilder *builder, TR::IlValue *condition, TR::IlValue *target, TR::IlValue *currentBytecodeSize, bool absolute)
   {
   TR::IlBuilder *doJump = NULL;
   TR::IlBuilder *fallthrough = NULL;
   builder->IfThenElse(&doJump, &fallthrough, condition);

   Jump(doJump, target, absolute);
   DefaultFallthrough(fallthrough, currentBytecodeSize);
   }

TR::VirtualMachineState *
OMR::InterpreterBuilder::GetVMState(TR::IlBuilder *builder)
   {
   return vmState();
   }

TR::IlValue *
OMR::InterpreterBuilder::GetInt64Immediate(TR::IlBuilder *builder, TR::IlValue *offset)
   {
   return builder->LoadAt(_pInt64,
          builder->      ConvertTo(_pInt64,
          builder->                Add(
          _pcRegister->               Load(builder),
                                      offset)));
   }

TR::IlValue *
OMR::InterpreterBuilder::GetInt32Immediate(TR::IlBuilder *builder, TR::IlValue *offset)
   {
   return builder->LoadAt(_pInt32,
          builder->      ConvertTo(_pInt32,
          builder->                Add(
          _pcRegister->               Load(builder),
                                      offset)));
   }

TR::IlValue *
OMR::InterpreterBuilder::GetInt16Immediate(TR::IlBuilder *builder, TR::IlValue *offset)
   {
   return builder->LoadAt(_pInt16,
          builder->      ConvertTo(_pInt16,
          builder->                Add(
          _pcRegister->               Load(builder),
                                      offset)));
   }

TR::IlValue *
OMR::InterpreterBuilder::GetInt8Immediate(TR::IlBuilder *builder, TR::IlValue *offset)
   {
   return builder->LoadAt(_pInt8,
          builder->      ConvertTo(_pInt8,
          builder->                Add(
          _pcRegister->               Load(builder),
                                      offset)));
   }

ClientAllocator OMR::InterpreterBuilder::_clientAllocator = NULL;
ClientAllocator OMR::InterpreterBuilder::_getImpl = NULL;
