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

#ifndef OMR_RUNTIMEBUILDER_INCL
#define OMR_RUNTIMEBUILDER_INCL

#include <map>
#include <fstream>
#include "ilgen/MethodBuilder.hpp"
#include "env/TypedAllocator.hpp"

namespace TR { class BytecodeBuilder; }
namespace TR { class ResolvedMethod; }
namespace TR { class SymbolReference; }
namespace TR { class VirtualMachineRegister; }
namespace TR { class VirtualMachineState; }
namespace TR { class RuntimeBuilder; }

class TR_Memory;

#ifndef TR_ALLOC
#define TR_ALLOC(x)
#endif


extern "C"
{
typedef void (*SetupCallback)(void *client);
}

typedef int64_t (HandlerFunction)(void *rb, void *b);

typedef struct HandlerData {
    char *bytecodeName;
    void *handler;
} HandlerData;

namespace OMR
{

class RuntimeBuilder : public TR::MethodBuilder
   {
public:
   TR_ALLOC(TR_Memory::IlGenerator)

   RuntimeBuilder(TR::TypeDictionary *types);
   virtual ~RuntimeBuilder();

   void Setup()
      {
      if (_clientCallbackSetup)
         _clientCallbackSetup(_client);
      }

   /**
    * @brief Store callback function to be called on client when Initialization is called
    */
   void setClientCallback_Setup(void *callback)
      {
      _clientCallbackSetup = (SetupCallback)callback;
      }

   void RegisterHandler(int32_t bytecodeIndex, char * bytecodeName, void *handler);

   virtual void DefaultFallthrough(TR::IlBuilder *builder, TR::IlValue *currentBytecodeSize) = 0;
   virtual void Jump(TR::IlBuilder *builder, TR::IlValue *target, bool absolute) = 0;
   virtual void JumpIfOrFallthrough(TR::IlBuilder *builder, TR::IlValue *condition, TR::IlValue *target, TR::IlValue *currentBytecodeSize, bool absolute) = 0;

   virtual TR::VirtualMachineState *GetVMState(TR::IlBuilder *builder) = 0;
   //TODO Create generic API that takes a IlType
   virtual TR::IlValue *GetInt64Immediate(TR::IlBuilder *builder, TR::IlValue *offset) = 0;
   virtual TR::IlValue *GetInt32Immediate(TR::IlBuilder *builder, TR::IlValue * offset) = 0;
   virtual TR::IlValue *GetInt16Immediate(TR::IlBuilder *builder, TR::IlValue * offset) = 0;
   virtual TR::IlValue *GetInt8Immediate(TR::IlBuilder *builder, TR::IlValue * offset) = 0;

protected:
   std::map<int32_t, HandlerData> *getHandlers() { return &_handlers; }
private:

   SetupCallback _clientCallbackSetup;
   std::map<int32_t, HandlerData> _handlers;
   };

} // namespace OMR

#endif // !defined(OMR_RUNTIMEBUILDER_INCL)
