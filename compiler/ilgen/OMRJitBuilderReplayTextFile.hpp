/*******************************************************************************
 * Copyright (c) 2018, 2019 IBM Corp. and others
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

 #ifndef OMR_JITBUILDERREPLAY_TEXTFILE_INCL
 #define OMR_JITBUILDERREPLAY_TEXTFILE_INCL

 #include "ilgen/JitBuilderReplay.hpp"

 #include <iostream>
 #include <sstream>
 #include <cstring>
 #include <fstream>
 #include <map>

 namespace OMR
 {

 class JitBuilderReplayTextFile : public TR::JitBuilderReplay
    {
    public:

    JitBuilderReplayTextFile(const char *fileName);

    /**
    * @brief Consumers for what has been recorded 
    * (consumers)
    */
   
    void               consumeStart();
    int8_t             consume8bitNumber();
    int16_t            consume16bitNumber();
    int32_t            consume32bitNumber();
    int64_t            consume64bitNumber();
    float              consumeFloatNumber();
    double             consumeDoubleNumber();
    const char *       consumeStatement();
    TR::IlType *       consumeType();
    TR::IlValue *      consumeValue();
    const char * const consumeString();
    TypeID             consumeID();
    TR::IlBuilder *    consumeBuilder();
    const void *       consumeLocation();

    private:

    char * getLineAsChar();
    char * getTokensFromLine(std::string currentLine);

    };

 } // namespace OMR

 #endif // !defined(OMR_JITBUILDERREPLAY_TEXTFILE_INCL)
