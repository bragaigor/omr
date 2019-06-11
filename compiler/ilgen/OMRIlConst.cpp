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

#include "compile/Compilation.hpp"
#include "il/Block.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "il/Symbol.hpp"
#include "il/symbol/AutomaticSymbol.hpp"
#include "il/SymbolReference.hpp"
#include "ilgen/IlConst.hpp" // must follow include for compile/Compilation.hpp for TR_Memory
#include "ilgen/MethodBuilder.hpp"


OMR::IlConst::IlConst(TR::Node *node, TR::TreeTop *treeTop, TR::Block *block, TR::MethodBuilder *methodBuilder)
   : TR::IlValue(node, treeTop, block, methodBuilder)
   {
   TR_ASSERT(node->getOpCode().isLoadConst(), "Can not create an IlConst from a node which is not loadConst (%s)", node->getOpCode().getName());
   }

int32_t
OMR::IlConst::get32bitIntegralValue()
   {
   return _nodeThatComputesValue->get32bitIntegralValue();
   }

int64_t
OMR::IlConst::get64bitIntegralValue()
   {
   return _nodeThatComputesValue->get64bitIntegralValue();
   }

void
OMR::IlConst::storeOver(TR::IlValue *value, TR::Block *block)
   {
   TR_ASSERT(false, "cannot use storeOver on IlConst");
   }

void *
OMR::IlConst::client()
   {
   if (_client == NULL && _clientAllocator != NULL)
      _client = _clientAllocator(static_cast<TR::IlConst *>(this));
   return _client;
   }
