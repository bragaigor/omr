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

#include <map>

#include "infra/Assert.hpp"

#include "ilgen/MethodBuilder.hpp"
#include "ilgen/RuntimeBuilder.hpp"
#include "ilgen/VirtualMachineState.hpp"

#define TraceEnabled    (comp()->getOption(TR_TraceILGen))
#define TraceIL(m, ...) {if (TraceEnabled) {traceMsg(comp(), m, ##__VA_ARGS__);}}

OMR::RuntimeBuilder::RuntimeBuilder(TR::TypeDictionary *types)
   : TR::MethodBuilder(types),
   _clientCallbackSetup(0),
   _handlers()
   {
   }

OMR::RuntimeBuilder::~RuntimeBuilder()
   {
   }

void
OMR::RuntimeBuilder::RegisterHandler(int32_t bytecodeIndex, char *bytecodeName, void *handler)
   {
   HandlerData data;
   data.bytecodeName = bytecodeName;
   data.handler = handler;
   TR_ASSERT(_handlers.find(bytecodeIndex) == _handlers.end(), "Handler '%d' already defined", bytecodeIndex);
   _handlers.insert(std::pair<int32_t, HandlerData>(bytecodeIndex, data));
   }
