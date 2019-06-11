/*******************************************************************************
 * Copyright (c) 2016, 2019 IBM Corp. and others
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

#include "env/TRMemory.hpp"   // include first to get correct TR_ALLOC definition
#include "ilgen/VirtualMachineRealStack.hpp"

#include "ilgen/VirtualMachineRegister.hpp"
#include "compile/Compilation.hpp"
#include "il/SymbolReference.hpp"
#include "il/symbol/AutomaticSymbol.hpp"
#include "ilgen/IlBuilder.hpp"
#include "ilgen/IlConst.hpp"
#include "ilgen/MethodBuilder.hpp"
#include "ilgen/TypeDictionary.hpp"

OMR::VirtualMachineRealStack::VirtualMachineRealStack(TR::MethodBuilder *mb,
        TR::VirtualMachineRegister *stackTopRegister,
        TR::IlType *elementType,
        bool growsUp,
        bool preAdjust)
   : TR::VirtualMachineStack(),
   _mb(mb),
   _stackTopRegister(stackTopRegister),
   _elementType(elementType),
   _stackBaseName(NULL),
   _growsUp(growsUp),
   _preAdjust(preAdjust)
   {
   init();
   }

//OMR::VirtualMachineOperandStack::VirtualMachineOperandStack(TR::VirtualMachineOperandStack *other)
//   : TR::VirtualMachineStack(),
//   _mb(other->_mb),
//   _stackTopRegister(other->_stackTopRegister),
//   _stackMax(other->_stackMax),
//   _stackTop(other->_stackTop),
//   _elementType(other->_elementType),
//   _pushAmount(other->_pushAmount),
//   _stackOffset(other->_stackOffset),
//   _stackBaseName(other->_stackBaseName)
//   {
//   int32_t numBytes = _stackMax * sizeof(TR::IlValue *);
//   _stack = (TR::IlValue **) TR::comp()->trMemory()->allocateHeapMemory(numBytes);
//   memcpy(_stack, other->_stack, numBytes);
//   }


void
OMR::VirtualMachineRealStack::Commit(TR::IlBuilder *b)
   {
   /* Empty */
   }

void
OMR::VirtualMachineRealStack::Reload(TR::IlBuilder* b)
   {
   /* Empty */
   }

void
OMR::VirtualMachineRealStack::MergeInto(TR::VirtualMachineState* o, TR::IlBuilder* b)
   {
   /* Empty */
   }

// Update the OperandStack_base and _stackTopRegister after the Virtual Machine moves the stack.
// This call will normally be followed by a call to Reload if any of the stack values changed in the move
void
OMR::VirtualMachineRealStack::UpdateStack(TR::IlBuilder *b, TR::IlValue *stack)
   {
   TR::IlValue *initialBase = b->Load(_stackBaseName);
   initialBase = b->ConvertTo(_mb->typeDictionary()->Int64, initialBase);
   TR::IlValue *currentBase = _stackTopRegister->Load(b);
   currentBase = b->ConvertTo(_mb->typeDictionary()->Int64, currentBase);
   TR::IlValue *delta = b->Sub(currentBase, initialBase);

   b->Store(_stackBaseName, stack);
   TR::IlValue *newStackTop = b->Add(stack, delta);
   _stackTopRegister->Store(b, newStackTop);
   }

TR::VirtualMachineState *
OMR::VirtualMachineRealStack::MakeCopy()
   {
   return static_cast<TR::VirtualMachineState *>(this);
   } 

void
OMR::VirtualMachineRealStack::Push(TR::IlBuilder *builder, TR::IlValue *value)
   {

//    fprintf(stderr, "OMR::VirtualMachineRealStack::Push type %s\n", value->getDataType().toString());

   TR::IlValue *stackAddress = _stackTopRegister->Load(builder);
   if (_growsUp)
      _stackTopRegister->Adjust(builder, 1);
   else
      _stackTopRegister->Adjust(builder, -1);

   if (_preAdjust)
      stackAddress = _stackTopRegister->Load(builder);

   builder->StoreAt(
               stackAddress,
   builder->   ConvertTo(_elementType, value));
   }

TR::IlValue *
OMR::VirtualMachineRealStack::Top(TR::IlBuilder *builder)
   {
   TR::IlValue *stackAddress = _stackTopRegister->Load(builder);
   TR::IlType *pElementType = _mb->typeDictionary()->PointerTo(_elementType);

   int32_t offset = 0;
   if (!_preAdjust)
      {
      if (_growsUp)
         {
         offset = -1;
         }
      else
         {
         offset = 1;
         }
      }

   TR::IlValue *value =
   builder->LoadAt(pElementType,
   builder->   IndexAt(pElementType,
                  stackAddress,
   builder->      ConstInt32(offset)));
   return value;
   }

TR::IlValue *
OMR::VirtualMachineRealStack::Pop(TR::IlBuilder *builder)
   {
   TR::IlValue *stackAddress = _stackTopRegister->Load(builder);
   if (_growsUp)
      _stackTopRegister->Adjust(builder, -1);
   else
      _stackTopRegister->Adjust(builder, 1);

   if (!_preAdjust)
      stackAddress = _stackTopRegister->Load(builder);

   TR::IlType *pElementType = _mb->typeDictionary()->PointerTo(_elementType);
   TR::IlValue *retVal = builder->LoadAt(pElementType, stackAddress);
//   fprintf(stderr, "OMR::VirtualMachineRealStack::Pop type %s\n", retVal->getDataType().toString());
   return retVal;
   }


void
OMR::VirtualMachineRealStack::Drop(TR::IlBuilder *builder, int32_t depth)
   {
   Drop(builder, builder->ConstInt32(depth));
   }

void
OMR::VirtualMachineRealStack::Drop(TR::IlBuilder *builder, TR::IlValue *depth)
   {
//   builder->Store("VirtualMachineRealStack::adjustmentValue", depth);
//   TR::IlBuilder *notZero = NULL;
//   builder->IfThen(&notZero, builder->NotEqualTo(builder->ConvertTo(_mb->typeDictionary()->Int64, depth), builder->ConstInt64(0)));
//
//   notZero->Store("VirtualMachineRealStack::adjustmentValue", notZero->Negate(depth));

//   _stackTopRegister->Adjust(builder, builder->Load("VirtualMachineRealStack::adjustmentValue"));
    _stackTopRegister->Adjust(builder, builder->Negate(depth));
   }

void
OMR::VirtualMachineRealStack::Dup(TR::IlBuilder *builder)
   {
   Push(builder, Top(builder));
   }

void *
OMR::VirtualMachineRealStack::client()
   {
   if (_client == NULL && _clientAllocator != NULL)
      _client = _clientAllocator(static_cast<TR::VirtualMachineRealStack *>(this));
   return _client;
   }

ClientAllocator OMR::VirtualMachineRealStack::_clientAllocator = NULL;
ClientAllocator OMR::VirtualMachineRealStack::_getImpl = NULL;

void
OMR::VirtualMachineRealStack::init()
   {
   TR::Compilation *comp = TR::comp();
   // Create a temp for the OperandStack base
   TR::SymbolReference *symRef = comp->getSymRefTab()->createTemporary(_mb->methodSymbol(), _mb->typeDictionary()->PointerTo(_elementType)->getPrimitiveType());
   symRef->getSymbol()->setNotCollected();
   char *name = (char *) comp->trMemory()->allocateHeapMemory((11+10+1) * sizeof(char)); // 11 ("_StackBase_") + max 10 digits + trailing zero
   sprintf(name, "_StackBase_%u", symRef->getCPIndex());
   symRef->getSymbol()->getAutoSymbol()->setName(name);
   _mb->defineSymbol(name, symRef);

   _stackBaseName = symRef->getSymbol()->getAutoSymbol()->getName();

   // store current stack value so that we can use this when MoveStack is called
   _mb->Store(_stackBaseName, _stackTopRegister->Load(_mb));
   }
