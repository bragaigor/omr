/*******************************************************************************
 * Copyright (c) 2017, 2018 IBM Corp. and others
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

#ifndef OMR_VIRTUALMACHINEREALARRAY_INCL
#define OMR_VIRTUALMACHINEREALARRAY_INCL

#include <stdint.h>
#include "ilgen/VirtualMachineArray.hpp"

namespace TR { class IlBuilder; }
namespace TR { class IlType; }
namespace TR { class IlValue; }
namespace TR { class MethodBuilder; }
namespace TR { class VirtualMachineRegister; }
namespace TR { class VirtualMachineRealArray; }

namespace OMR
{

class VirtualMachineRealArray : public TR::VirtualMachineArray
   {
   public:
   VirtualMachineRealArray(TR::MethodBuilder *mb,
           TR::VirtualMachineRegister *arrayBaseRegister,
           TR::IlType *elementType);

//   /**
//    * @brief constructor used to copy the array from another state
//    * @param other the operand array whose values should be used to initialize this object
//    */
//   VirtualMachineOperandArray(TR::VirtualMachineOperandArray *other);

   /**
    * @brief write the simulated operand array to the virtual machine
    * @param b the builder where the operations will be placed to recreate the virtual machine operand array
    */
   virtual void Commit(TR::IlBuilder *b);
   
   /**
    * @brief read the virtual machine array back into the simulated operand array
    * @param b the builder where the operations will be placed to recreate the simulated operand array
    * stack accounts for new or dropped virtual machine stack elements. 
    */
   virtual void Reload(TR::IlBuilder *b);

   /**
    * @brief create an identical copy of the current object.
    * @returns the copy of the current object
    */
   virtual TR::VirtualMachineState *MakeCopy();

   /**
    * @brief emit operands to store current operand array values into same variables as used in another operand array
    * @param other operand array for the builder object control is merging into
    * @param b builder object where the operations will be added to make the current operand array the same as the other
    */
   virtual void MergeInto(TR::VirtualMachineState *other, TR::IlBuilder *b);
   
   /**
    * @brief update the values used to read and write the virtual machine array
    * @param b the builder where the values will be placed
    * @param array the new array base address.
    */
   virtual void UpdateArray(TR::IlBuilder *b, TR::IlValue *array);

   /**
    * @brief Returns the expression at the given index of the simulated operand array
    * @param index the location of the expression to return
    * @returns the expression at the given index
    */
   virtual TR::IlValue *Get(TR::IlBuilder *builder, int32_t index);
   
   /**
    * @brief Returns the expression at the given index of the simulated operand array
    * @param index the location of the expression to return
    * @returns the expression at the given index
    */
   virtual TR::IlValue *Get(TR::IlBuilder *builder, TR::IlValue *index);

   /**
    * @brief Set the expression into the simulated operand array at the given index
    * @param index the location to store the expression
    * @param value expression to store into the simulated operand array
    */
   virtual void Set(TR::IlBuilder *builder, int32_t index, TR::IlValue *value);

   /**
    * @brief Set the expression into the simulated operand array at the given index
    * @param index the location to store the expression
    * @param value expression to store into the simulated operand array
    */
   virtual void Set(TR::IlBuilder *builder, TR::IlValue *index, TR::IlValue *value);
  
   /**
    * @brief Move the expression from one index to another index in the simulated operand array
    * @param dstIndex the location to store the expression
    * @param srcIndex the location to copy the expression from
    */ 
   virtual void Move(TR::IlBuilder *b, int32_t dstIndex, int32_t srcIndex);

   /**
    * @brief Move the expression from one index to another index in the simulated operand array
    * @param dstIndex the location to store the expression
    * @param srcIndex the location to copy the expression from
    */
   virtual void Move(TR::IlBuilder *b, TR::IlValue *dstIndex, TR::IlValue *srcIndex);

   /**
    * @brief returns the client object associated with this object
    */
   virtual void *client();

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

   protected:
   void init();

   private:
   TR::MethodBuilder *_mb;
   TR::VirtualMachineRegister *_arrayBaseRegister;
   TR::IlType *_elementType;

   static ClientAllocator _clientAllocator;
   static ImplGetter _getImpl;
   };
}

#endif // !defined(OMR_VIRTUALMACHINEREALARRAY_INCL)
