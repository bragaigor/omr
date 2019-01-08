/*******************************************************************************
 * Copyright (c) 2018, 2018 IBM Corp. and others
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

#ifndef OMR_JITBUILDERREPLAY_INCL
#define OMR_JITBUILDERREPLAY_INCL

#include <iostream>
#include <fstream>
#include <map>
#include "ilgen/StatementNames.hpp"

namespace TR { class MethodBuilder; }
namespace TR { class IlBuilder; }
namespace TR { class IlType; }
namespace TR { class IlValue; }

namespace OMR
{

class JitBuilderReplay
   {
   public:
   
   enum MethodFlag { CONSTRUCTOR_FLAG, BUILDIL_FLAG };

   typedef const uint32_t                      TypeID;
   typedef void *                              TypePointer;
   typedef std::map<TypeID, TypePointer> TypeMapPointer;

   JitBuilderReplay();
   virtual ~JitBuilderReplay();

   /**
    * @brief Subclasses override these functions to replay from different input formats
    * (helpers)
    */

    virtual void start();
    void StoreReservedIDs();

    virtual void consumeStart()                               { }
    virtual const char * const consumeString()                { }
    virtual int8_t consume8bitNumber()                        { }
    virtual int16_t consume16bitNumber()                      { }
    virtual int32_t consume32bitNumber()                      { }
    virtual int64_t consume64bitNumber()                      { }
    virtual float consumeFloatNumber()                        { }
    virtual double consumeDoubleNumber()                      { }
    virtual TypeID consumeID()                                { }
    virtual const char consumeStatement()                     { }
    virtual const TR::IlType consumeType()                    { }
    virtual const TR::IlValue consumeValue()                  { }
    virtual const TR::MethodBuilder consumeMethodBuilder()    { }
    virtual void consumeBuilder()                             { }
    virtual const void * consumeLocation()                    { }
    virtual void consumeEndStatement()                        { }

    // Define Map that maps IDs to pointers

    virtual void registerMapping(TypeID, TypePointer);

    protected:
    const TR::MethodBuilder *         _mb;
    TypeMapPointer                    _pointerMap;
    uint8_t                           _idSize;

    TypePointer lookupPointer(TypeID id);
    
   };

} // namespace OMR

 #endif // !defined(OMR_JITBUILDERREPLAY_INCL)
