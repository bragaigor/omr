/*******************************************************************************
 * Copyright (c) 2017, 2019 IBM Corp. and others
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

#ifndef OMR_ILCONST_INCL
#define OMR_ILCONST_INCL

#include "il/DataTypes.hpp"
#include "ilgen/IlValue.hpp"
#ifndef TR_ALLOC
#define TR_ALLOC(x)
#endif

//namespace TR { class Node; }
namespace TR { class TreeTop; }
namespace TR { class Block; }
namespace TR { class Symbol; }
namespace TR { class SymbolReference; }
namespace TR { class MethodBuilder; }
namespace TR { class IlValue; }

namespace OMR
{
class IlConst : public TR::IlValue
   {
public:
   TR_ALLOC(TR_Memory::IlGenerator)
   /**
    * @brief initial state assumes value will only be used locally
    */
   IlConst(TR::Node *node, TR::TreeTop *treeTop, TR::Block *block, TR::MethodBuilder *methodBuilder);

   int32_t get32bitIntegralValue();
   int64_t get64bitIntegralValue();

   virtual void storeOver(TR::IlValue *value, TR::Block *block);

   /**
    * @brief returns the client object associated with this object
    */
   virtual void *client();

protected:
   };

} // namespace OMR

#endif // !defined(OMR_ILCONST_INCL)
