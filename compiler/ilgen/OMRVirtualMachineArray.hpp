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

#ifndef OMR_VIRTUALMACHINEARRAY_INCL
#define OMR_VIRTUALMACHINEARRAY_INCL

#include <stdint.h>
#include "ilgen/VirtualMachineState.hpp"

namespace TR { class IlBuilder; }
namespace TR { class IlType; }
namespace TR { class IlValue; }
namespace TR { class MethodBuilder; }
namespace TR { class VirtualMachineRegister; }
namespace TR { class VirtualMachineArray; }

namespace OMR
{

class VirtualMachineArray : public TR::VirtualMachineState
   {
   public:
   VirtualMachineArray();

   virtual void Commit(TR::IlBuilder *b) = 0;
   
   virtual void Reload(TR::IlBuilder *b) = 0;

   /**
    * @brief create an identical copy of the current object.
    * @returns the copy of the current object
    */
   virtual TR::VirtualMachineState *MakeCopy() = 0;

   virtual void MergeInto(TR::VirtualMachineState *other, TR::IlBuilder *b) = 0;
   
   /**
    * @brief update the values used to read and write the virtual machine array
    * @param b the builder where the values will be placed
    * @param array the new array base address.
    */
   virtual void UpdateArray(TR::IlBuilder *b, TR::IlValue *array) = 0;

   virtual TR::IlValue *Get(TR::IlBuilder *b, int32_t index) = 0;
   virtual TR::IlValue *Get(TR::IlBuilder *b, TR::IlValue *index) = 0;
   virtual void Set(TR::IlBuilder *b, int32_t index, TR::IlValue *value) = 0;
   virtual void Set(TR::IlBuilder *b, TR::IlValue *index, TR::IlValue *value) = 0;
   virtual void Move(TR::IlBuilder *b, int32_t dstIndex, int32_t srcIndex) = 0;
   virtual void Move(TR::IlBuilder *b, TR::IlValue *dstIndex, TR::IlValue *srcIndex) = 0;
   };
}

#endif // !defined(OMR_VIRTUALMACHINEOPERANDARRAY_INCL)
