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

namespace TR { class IlBuilderReplay; }
namespace TR { class MethodBuilderReplay; }
namespace TR { class MethodBuilder; }
namespace TR { class IlBuilder; }

namespace OMR
{

class JitBuilderReplay
   {
   public:

   const char *RECORDER_SIGNATURE = "JBIL"; // JitBuilder IL
   const char *JBIL_COMPLETE      = "Done";
   enum MethodFlag { CONSTRUCTOR_FLAG, BUILDIL_FLAG };

   JitBuilderReplay();
   virtual ~JitBuilderReplay();

   /**
    * @brief Subclasses override these functions to replay from different input formats
    * (helpers)
    */

    virtual void start();
    virtual void initializeMethodBuilder(TR::MethodBuilderReplay * replay);
    virtual bool parseConstructor()              { return false; }
    virtual bool parseBuildIL()                  { return false; }
    virtual void StoreReservedIDs();

    virtual void handleMethodBuilder(uint32_t serviceID, char * tokens)         { }
    virtual void handleDefineLine(TR::MethodBuilder * mb, char * tokens)        { }
    virtual void handleDefineFile(TR::MethodBuilder * mb, char * tokens)        { }
    virtual void handleDefineName(TR::MethodBuilder * mb, char * tokens)        { }
    virtual void handleDefineParameter(TR::MethodBuilder * mb, char * tokens)   { }
    virtual void handleDefineArrayParameter(TR::MethodBuilder * mb, char * tokens)   { }
    virtual void handlePrimitiveType(TR::MethodBuilder * mb, char * tokens)     { }
    virtual void handleDefineReturnType(TR::MethodBuilder * mb, char * tokens)  { }
    virtual void handleDefineFunction(TR::MethodBuilder * mb, char * tokens)    { }
    virtual void handleDoneLocal(TR::MethodBuilder * mb, char * tokens)         { }

    virtual void handleAdd(TR::IlBuilder * ilmb, char * tokens)                 { }
    virtual void handleSub(TR::IlBuilder * ilmb, char * tokens)                 { }
    virtual void handleMul(TR::IlBuilder * ilmb, char * tokens)                 { }
    virtual void handleDiv(TR::IlBuilder * ilmb, char * tokens)                 { }
    virtual void handleCreateLocalArray(TR::IlBuilder * ilmb, char * tokens)    { }
    virtual void handleAnd(TR::IlBuilder * ilmb, char * tokens)                 { }
    virtual void handleOr(TR::IlBuilder * ilmb, char * tokens)                  { }
    virtual void handleXor(TR::IlBuilder * ilmb, char * tokens)                 { }
    virtual void handleLoad(TR::IlBuilder * ilmb, char * tokens)                { }
    virtual void handleLoadAt(TR::IlBuilder * ilmb, char * tokens)              { }
    virtual void handleStore(TR::IlBuilder * ilmb, char * tokens)               { }
    virtual void handleStoreAt(TR::IlBuilder * ilmb, char * tokens)             { }
    virtual void handleIndexAt(TR::IlBuilder * ilmb, char * tokens)             { }
    virtual void handleConstInt8(TR::IlBuilder * ilmb, char * tokens)           { }
    virtual void handleConstInt32(TR::IlBuilder * ilmb, char * tokens)          { }
    virtual void handleConstInt64(TR::IlBuilder * ilmb, char * tokens)          { }
    virtual void handleConstDouble(TR::IlBuilder * ilmb, char * tokens)         { }
    virtual void handleConstAddress(TR::IlBuilder * ilmb, char * tokens)         { }
    virtual void handleConvertTo(TR::IlBuilder * ilmb, char * tokens)           { }
    virtual void handleCall(TR::IlBuilder * ilmb, char * tokens)                { }
    virtual void handleForLoop(TR::IlBuilder * ilmb, char * tokens)             { }
    virtual void handleReturn(TR::IlBuilder * ilmb, char * tokens)              { }
    virtual void handleReturnValue(TR::IlBuilder * ilmb, char * tokens)         { }
    virtual bool handleService(char * service)                                  { return false; }
    virtual void handleLessThan(TR::IlBuilder * ilmb, char * tokens)            { }
    virtual void handleGreaterThan(TR::IlBuilder * ilmb, char * tokens)         { }
    virtual void handleNotEqualTo(TR::IlBuilder * ilmb, char * tokens)          { }
    virtual void handleIfThenElse(TR::IlBuilder * ilmb, char * tokens)          { }
    virtual void handleAllLocalsHaveBeenDefined(TR::IlBuilder * ilmb, char * tokens) { }
    virtual void handleUnsignedShiftR(TR::IlBuilder * ilmb, char * tokens)      { }
    virtual void handleIfCmpEqualZero(TR::IlBuilder * ilmb, char * tokens)      { }
    virtual void handleNewIlBuilder(TR::IlBuilder * ilmb, char * tokens)        { }

    // Define Map that maps IDs to pointers

    typedef const uint32_t                      TypeID;
    typedef void *                              TypePointer;
    virtual void StoreIDPointerPair(TypeID, TypePointer);
    typedef std::map<TypeID, TypePointer> TypeMapPointer;

    /**
     * @brief constant strings used to define output format
     */
   const char *STATEMENT_DEFINENAME                   = "DefineName";
   const char *STATEMENT_DEFINEFILE                   = "DefineFile";
   const char *STATEMENT_DEFINELINESTRING             = "DefineLineString";
   const char *STATEMENT_DEFINELINENUMBER             = "DefineLineNumber";
   const char *STATEMENT_DEFINEPARAMETER              = "DefineParameter";
   const char *STATEMENT_DEFINEARRAYPARAMETER         = "DefineArrayParameter";
   const char *STATEMENT_DEFINERETURNTYPE             = "DefineReturnType";
   const char *STATEMENT_DEFINELOCAL                  = "DefineLocal";
   const char *STATEMENT_DEFINEMEMORY                 = "DefineMemory";
   const char *STATEMENT_DEFINEFUNCTION               = "DefineFunction";
   const char *STATEMENT_DEFINESTRUCT                 = "DefineStruct";
   const char *STATEMENT_DEFINEUNION                  = "DefineUnion";
   const char *STATEMENT_DEFINEFIELD                  = "DefineField";
   const char *STATEMENT_PRIMITIVETYPE                = "PrimitiveType";
   const char *STATEMENT_POINTERTYPE                  = "PointerType";
   const char *STATEMENT_NEWMETHODBUILDER             = "NewMethodBuilder";
   const char *STATEMENT_NEWILBUILDER                 = "NewIlBuilder";
   const char *STATEMENT_NEWBYTECODEBUILDER           = "NewBytecodeBuilder";
   const char *STATEMENT_ALLLOCALSHAVEBEENDEFINED     = "AllLocalsHaveBeenDefined";
   const char *STATEMENT_NULLADDRESS                  = "NullAddress";
   const char *STATEMENT_CONSTINT8                    = "ConstInt8";
   const char *STATEMENT_CONSTINT16                   = "ConstInt16";
   const char *STATEMENT_CONSTINT32                   = "ConstInt32";
   const char *STATEMENT_CONSTINT64                   = "ConstInt64";
   const char *STATEMENT_CONSTFLOAT                   = "ConstFloat";
   const char *STATEMENT_CONSTDOUBLE                  = "ConstDouble";
   const char *STATEMENT_CONSTSTRING                  = "ConstString";
   const char *STATEMENT_CONSTADDRESS                 = "ConstAddress";
   const char *STATEMENT_INDEXAT                      = "IndexAt";
   const char *STATEMENT_LOAD                         = "Load";
   const char *STATEMENT_LOADAT                       = "LoadAt";
   const char *STATEMENT_LOADINDIRECT                 = "LoadIndirect";
   const char *STATEMENT_STORE                        = "Store";
   const char *STATEMENT_STOREOVER                    = "StoreOver";
   const char *STATEMENT_STOREAT                      = "StoreAt";
   const char *STATEMENT_STOREINDIRECT                = "StoreIndirect";
   const char *STATEMENT_CREATELOCALARRAY             = "CreateLocalArray";
   const char *STATEMENT_CREATELOCALSTRUCT            = "CreateLocalStruct";
   const char *STATEMENT_VECTORLOAD                   = "VectorLoad";
   const char *STATEMENT_VECTORLOADAT                 = "VectorLoadAt";
   const char *STATEMENT_VECTORSTORE                  = "VectorStore";
   const char *STATEMENT_VECTORSTOREAT                = "VectorStoreAt";
   const char *STATEMENT_STRUCTFIELDINSTANCEADDRESS   = "StructFieldInstance";
   const char *STATEMENT_UNIONFIELDINSTANCEADDRESS    = "UnionFieldInstance";
   const char *STATEMENT_CONVERTTO                    = "ConvertTo";
   const char *STATEMENT_UNSIGNEDCONVERTTO            = "UnsignedConvertTo";
   const char *STATEMENT_ATOMICADD                    = "AtomicAdd";
   const char *STATEMENT_ADD                          = "Add";
   const char *STATEMENT_ADDWITHOVERFLOW              = "AddWithOverflow";
   const char *STATEMENT_ADDWITHUNSIGNEDOVERFLOW      = "AddWithUnsignedOverflow";
   const char *STATEMENT_SUB                          = "Sub";
   const char *STATEMENT_SUBWITHOVERFLOW              = "SubWithOverflow";
   const char *STATEMENT_SUBWITHUNSIGNEDOVERFLOW      = "SubWithUnsignedOverflow";
   const char *STATEMENT_MUL                          = "Mul";
   const char *STATEMENT_MULWITHOVERFLOW              = "MulWithOverflow";
   const char *STATEMENT_DIV                          = "Div";
   const char *STATEMENT_REM                          = "Rem";
   const char *STATEMENT_AND                          = "And";
   const char *STATEMENT_OR                           = "Or";
   const char *STATEMENT_XOR                          = "Xor";
   const char *STATEMENT_LESSTHAN                     = "LessThan";
   const char *STATEMENT_UNSIGNEDLESSTHAN             = "UnsignedLessThan";
   const char *STATEMENT_UNSIGNEDLESSOREQUALTO        = "UnsignedLessOrEqualTo";
   const char *STATEMENT_NEGATE                       = "Negate";
   const char *STATEMENT_CONVERTBITSTO                = "ConvertBitsTo";
   const char *STATEMENT_GREATERTHAN                  = "GreaterThan";
   const char *STATEMENT_GREATEROREQUALTO             = "GreaterOrEqualTo";
   const char *STATEMENT_UNSIGNEDGREATERTHAN          = "UnsignedGreaterThan";
   const char *STATEMENT_UNSIGNEDGREATEROREQUALTO     = "UnsignedGreaterOrEqualTo";
   const char *STATEMENT_EQUALTO                      = "EqualTo";
   const char *STATEMENT_LESSOREQUALTO                = "LessOrEqualTo";
   const char *STATEMENT_NOTEQUALTO                   = "NotEqualTo";
   const char *STATEMENT_APPENDBUILDER                = "AppendBuilder";
   const char *STATEMENT_APPENDBYTECODEBUILDER        = "AppendBytecodeBuilder";
   const char *STATEMENT_GOTO                         = "Goto";
   const char *STATEMENT_SHIFTL                       = "ShiftL";
   const char *STATEMENT_SHIFTR                       = "ShiftR";
   const char *STATEMENT_UNSIGNEDSHIFTR               = "UnsignedShiftR";
   const char *STATEMENT_RETURN                       = "Return";
   const char *STATEMENT_RETURNVALUE                  = "ReturnValue";
   const char *STATEMENT_IFTHENELSE                   = "IfThenElse";
   const char *STATEMENT_IFCMPEQUALZERO               = "IfCmpEqualZero";
   const char *STATEMENT_IFCMPNOTEQUALZERO            = "IfCmpNotEqualZero";
   const char *STATEMENT_IFCMPNOTEQUAL                = "IfCmpNotEqual";
   const char *STATEMENT_IFCMPEQUAL                   = "IfCmpEqual";
   const char *STATEMENT_IFCMPLESSTHAN                = "IfCmpLessThan";
   const char *STATEMENT_IFCMPLESSOREQUAL             = "IfCmpLessOrEqual";
   const char *STATEMENT_IFCMPUNSIGNEDLESSOREQUAL     = "IfCmpUnsignedLessOrEqual";
   const char *STATEMENT_IFCMPUNSIGNEDLESSTHAN        = "IfCmpUnsignedLessThan";
   const char *STATEMENT_IFCMPGREATERTHAN             = "IfCmpGreaterThan";
   const char *STATEMENT_IFCMPUNSIGNEDGREATERTHAN     = "IfCmpUnsignedGreaterThan";
   const char *STATEMENT_IFCMPGREATEROREQUAL          = "IfCmpGreaterOrEqual";
   const char *STATEMENT_IFCMPUNSIGNEDGREATEROREQUAL  = "IfCmpUnsignedGreaterOrEqual";
   const char *STATEMENT_DOWHILELOOP                  = "DoWhileLoop";
   const char *STATEMENT_WHILEDOLOOP                  = "WhileDoLoop";
   const char *STATEMENT_FORLOOP                      = "ForLoop";
   const char *STATEMENT_CALL                         = "Call";
   const char *STATEMENT_COMPUTEDCALL                 = "ComputedCall";
   const char *STATEMENT_DONECONSTRUCTOR              = "DoneConstructor";
   const char *STATEMENT_IFAND                        = "IfAnd";
   const char *STATEMENT_IFOR                         = "IfOr";
   const char *STATEMENT_SWITCH                       = "Switch";
   const char *STATEMENT_TRANSACTION                  = "Transaction";
   const char *STATEMENT_TRANSACTIONABORT             = "TransactionAbort";

    protected:
    const TR::MethodBuilderReplay *   _mb;
    TypeMapPointer                    _pointerMap;
    uint8_t                           _idSize;

    TypePointer lookupPointer(TypeID id);
    
   };

} // namespace OMR

 #endif // !defined(OMR_JITBUILDERREPLAY_INCL)
