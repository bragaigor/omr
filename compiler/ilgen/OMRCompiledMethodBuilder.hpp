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

#ifndef OMR_COMPILEDMETHODBUILDER_INCL
#define OMR_COMPILEDMETHODBUILDER_INCL

#include <map>
#include <set>
#include <fstream>
#include "ilgen/BytecodeBuilder.hpp"
#include "ilgen/RuntimeBuilder.hpp"
#include "env/TypedAllocator.hpp"

namespace TR { class ResolvedMethod; }
namespace TR { class SymbolReference; }
namespace TR { class VirtualMachineRegister; }
namespace TR { class VirtualMachineState; }
namespace TR { class CompiledMethodBuilder; }

class TR_Memory;

#ifndef TR_ALLOC
#define TR_ALLOC(x)
#endif

namespace OMR
{

class CompiledMethodBuilder : public TR::RuntimeBuilder
   {
public:
   TR_ALLOC(TR_Memory::IlGenerator)

   CompiledMethodBuilder(TR::TypeDictionary *types, void *bytecodeStream, int32_t bytecodeSizeInBytes);
   virtual ~CompiledMethodBuilder();

   virtual bool buildIL();

   /**
    * @brief returns the client object associated with this object, allocating it if necessary
    */
   void *client();

   /**
    * @brief Set the Client Allocator function
    */
   static void setClientAllocator(ClientAllocator allocator)
      {
      _clientAllocator = allocator;
      }

   /**
    * @brief Set the Get Impl function
    *
    * @param getter function pointer to the impl getter
    */
   static void setGetImpl(ImplGetter getter)
      {
      _getImpl = getter;
      }

   virtual void DefaultFallthrough(TR::IlBuilder *builder, TR::IlValue *currentBytecodeSize);
   virtual void Jump(TR::IlBuilder *builder, TR::IlValue *target, bool absolute);
   virtual void JumpIfOrFallthrough(TR::IlBuilder *builder, TR::IlValue *condition, TR::IlValue *target, TR::IlValue *currentBytecodeSize, bool absolute);

   virtual TR::VirtualMachineState *GetVMState(TR::IlBuilder *builder);
   virtual TR::IlValue *GetInt64Immediate(TR::IlBuilder *builder, TR::IlValue *offset);
   virtual TR::IlValue *GetInt32Immediate(TR::IlBuilder *builder, TR::IlValue *offset);
   virtual TR::IlValue *GetInt16Immediate(TR::IlBuilder *builder, TR::IlValue *offset);
   virtual TR::IlValue *GetInt8Immediate(TR::IlBuilder *builder, TR::IlValue *offset);

protected:
   TR::BytecodeBuilder *getBuilder(int32_t bcIndex)
      {
      TR::BytecodeBuilder *builder = NULL;
      auto search = _builders.find(bcIndex);
      if (search != _builders.end())
         builder = search->second;
      else
         {
         builder = OrphanBytecodeBuilder(bcIndex, "CompiledMethodBuilder");
         _builders.insert(std::make_pair(bcIndex, builder));
         }
      return builder;
      }

   int32_t getBytecodeAtPC(int32_t pc)
      {
      int32_t bytecode = -1;
      switch(_opcodeSizeInBytes)
         {
      case 1:
         {
         int8_t *data = (int8_t*)_opcodes;
         int8_t val = data[pc];
         bytecode = val;
         break;
         }
      case 2:
         {
         int16_t *data = (int16_t*)_opcodes;
         int16_t val = data[pc];
         bytecode = val;
         break;
         }
      case 4:
         {
         int32_t *data = (int32_t*)_opcodes;
         int32_t val = data[pc];
         bytecode = val;
         break;
         }
      default:
         //TODO handle with trace assert
         break;
         }
      return bytecode;
      }
private:
   TR::IlType *_pInt64;
   TR::IlType *_pInt32;
   TR::IlType *_pInt16;
   TR::IlType *_pInt8;
   int32_t _pc;
   void *_opcodes;
   int32_t _opcodeSizeInBytes;
   std::map<int32_t, TR::BytecodeBuilder *> _builders;

   static ClientAllocator      _clientAllocator;
   static ImplGetter _getImpl;
   };

} // namespace OMR

#endif // !defined(OMR_COMPILEDMETHODBUILDER_INCL)
