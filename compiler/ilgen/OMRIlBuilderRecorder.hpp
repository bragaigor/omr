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

#ifndef OMR_ILBUILDERRECORDER_INCL
#define OMR_ILBUILDERRECORDER_INCL

#include <fstream>
#include <stdarg.h>
#include <string.h>
#include "ilgen/IlInjector.hpp"
#include "il/ILHelpers.hpp"

#include "ilgen/IlValue.hpp" // must go after IlInjector.hpp or TR_ALLOC isn't cleaned up

namespace TR { class JitBuilderRecorder; }
namespace TR { class MethodBuilder; }
namespace TR { class IlBuilder; }
namespace TR { class IlBuilderRecorder; }
namespace TR { class IlType; }
namespace TR { class TypeDictionary; }

extern "C"
{
typedef bool (*ClientBuildILCallback)(void *clientObject);
typedef void * (*ClientAllocator)(void *implObject);
typedef void * (*ImplGetter)(void *client);
}

namespace OMR
{

class IlBuilderRecorder : public TR::IlInjector
   {

public:
   TR_ALLOC(TR_Memory::IlGenerator)


   /**
    * @brief A class encapsulating the information needed for a switch-case
    *
    * This class encapsulates the different pieces needed to construct a Case
    * for IlBuilder's Switch() service. It's constructor is private, so instances
    * can only be created by calling IlBuilder::MakeCase().
    */
   class JBCase
      {
      public:
         void * client();
         void setClient(void * client) { _client = client; }
         static void setClientAllocator(ClientAllocator allocator) { _clientAllocator = allocator; }
         static void setGetImpl(ImplGetter getter) { _getImpl = getter; }

         /**
          * @brief Construct a new JBCase object.
          *
          * This constructor should not be called directly outside of this classs.
          * A call to `MakeCase()` should be used instead.
          *
          * @param v the value matched by the case
          * @param b the builder implementing the case body
          * @param f whether the case falls-through or not
          */
         JBCase(int32_t v, TR::IlBuilder *b, int32_t f)
             : _value(v), _builder(b), _fallsThrough(f), _client(NULL) {}

         int32_t _value;          // value matched by the case
         TR::IlBuilder *_builder; // builder for the case body
         int32_t _fallsThrough;   // whether the case falls-through

      private:
         
         void * _client;
         static ClientAllocator _clientAllocator;
         static ImplGetter _getImpl;

         friend class OMR::IlBuilderRecorder;
      };

   
   /**
    * @brief A class encapsulating the information needed for IfAnd and IfOr.
    *
    * This class encapsulates the value of the condition and the builder
    * object used generate the value (used to evaluate the condition).
    */
   class JBCondition
      {
      public:
         void * client();
         void setClient(void * client) { _client = client; }
         static void setClientAllocator(ClientAllocator allocator) { _clientAllocator = allocator; }
         static void setGetImpl(ImplGetter getter) { _getImpl = getter; }

         /**
          * @brief Construct a new JBCondition object.
          *
          * This constructor should not be called directly outside of the JitBuilder
          * implementation. A call to `MakeCondition()` should be used instead.
          *
          * @param conditionBuilder pointer to the builder used to generate the condition value
          * @param conditionValue the IlValue representing value for the condition
          */
         JBCondition(TR::IlBuilder *conditionBuilder, TR::IlValue *conditionValue)
            : _builder(conditionBuilder), _condition(conditionValue), _client(NULL) {}

         TR::IlBuilder *_builder; // builder used to generate the condition value
         TR::IlValue *_condition; // value for the condition

      private:
         void * _client;
         static ClientAllocator _clientAllocator;
         static ImplGetter _getImpl;

         friend class OMR::IlBuilderRecorder;
      };

   IlBuilderRecorder(TR::MethodBuilder *methodBuilder, TR::TypeDictionary *types);
   IlBuilderRecorder(TR::IlBuilder *source);
   virtual ~IlBuilderRecorder() { }

   // needed to make IlBuilderRecorder a concrete class
   virtual bool injectIL()      { return true; }

   void NewIlBuilder();

   // create a new local value (temporary variable)
   TR::IlValue *NewValue(TR::IlType *dt);

   void DoneConstructor(const char * value);
   void assertNotRecorded(TR::JitBuilderRecorder * rec, const char * statement);

   // constants
   TR::IlValue *NullAddress();
   TR::IlValue *ConstInt8(int8_t value);
   TR::IlValue *ConstInt16(int16_t value);
   TR::IlValue *ConstInt32(int32_t value);
   TR::IlValue *ConstInt64(int64_t value);
   TR::IlValue *ConstFloat(float value);
   TR::IlValue *ConstDouble(double value);
   TR::IlValue *ConstAddress(const void * const value);
   TR::IlValue *ConstString(const char * const value);
   TR::IlValue *ConstzeroValueForValue(TR::IlValue *v);

   TR::IlValue *Const(int8_t value)             { return ConstInt8(value); }
   TR::IlValue *Const(int16_t value)            { return ConstInt16(value); }
   TR::IlValue *Const(int32_t value)            { return ConstInt32(value); }
   TR::IlValue *Const(int64_t value)            { return ConstInt64(value); }
   TR::IlValue *Const(float value)              { return ConstFloat(value); }
   TR::IlValue *Const(double value)             { return ConstDouble(value); }
   TR::IlValue *Const(const void * const value) { return ConstAddress(value); }

   TR::IlValue *ConstInteger(TR::IlType *intType, int64_t value);

   // arithmetic
   TR::IlValue *Add(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *AddWithOverflow(TR::IlBuilder **handler, TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *AddWithUnsignedOverflow(TR::IlBuilder **handler, TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *Sub(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *SubWithOverflow(TR::IlBuilder **handler, TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *SubWithUnsignedOverflow(TR::IlBuilder **handler, TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *Mul(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *MulWithOverflow(TR::IlBuilder **handler, TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *Div(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *Rem(TR::IlValue *left, TR::IlValue *right);

   TR::IlValue *And(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *Or(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *Xor(TR::IlValue *left, TR::IlValue *right);

   TR::IlValue *ShiftL(TR::IlValue *v, TR::IlValue *amount);
   TR::IlValue *ShiftL(TR::IlValue *v, int8_t amount)                { return ShiftL(v, ConstInt8(amount)); }
   TR::IlValue *ShiftR(TR::IlValue *v, TR::IlValue *amount);
   TR::IlValue *ShiftR(TR::IlValue *v, int8_t amount)                { return ShiftR(v, ConstInt8(amount)); }
   TR::IlValue *UnsignedShiftR(TR::IlValue *v, TR::IlValue *amount);
   TR::IlValue *UnsignedShiftR(TR::IlValue *v, int8_t amount)        { return UnsignedShiftR(v, ConstInt8(amount)); }
  
   TR::IlValue *EqualTo(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *NotEqualTo(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *GreaterThan(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *UnsignedGreaterThan(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *GreaterOrEqualTo(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *UnsignedGreaterOrEqualTo(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *LessThan(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *UnsignedLessThan(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *LessOrEqualTo(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue *UnsignedLessOrEqualTo(TR::IlValue *left, TR::IlValue *right);
   TR::IlValue * Negate(TR::IlValue *v);

   TR::IlValue* ConvertBitsTo(TR::IlType* type, TR::IlValue* value);
   TR::IlValue *ConvertTo(TR::IlType *t, TR::IlValue *v);
   TR::IlValue *UnsignedConvertTo(TR::IlType *t, TR::IlValue *v);

   // memory
   void Store(const char *name, TR::IlValue *value);
   void StoreOver(TR::IlValue *dest, TR::IlValue *value);
   void StoreAt(TR::IlValue *address, TR::IlValue *value);
   void StoreIndirect(const char *type, const char *field, TR::IlValue *object, TR::IlValue *value);
   TR::IlValue *Load(const char *name);
   TR::IlValue *LoadIndirect(const char *type, const char *field, TR::IlValue *object);
   TR::IlValue *LoadAt(TR::IlType *dt, TR::IlValue *address);
   TR::IlValue *IndexAt(TR::IlType *dt, TR::IlValue *base, TR::IlValue *index);
   TR::IlValue *CreateLocalArray(int32_t numElements, TR::IlType *elementType);
   TR::IlValue *CreateLocalStruct(TR::IlType *structType);
   TR::IlValue *AtomicAdd(TR::IlValue *baseAddress, TR::IlValue * value);

   /**
    * `StructFieldAddress` and `UnionFieldAddress` are two functions that
    * provide a workaround for a limitation in JitBuilder. Becuase `TR::IlValue`
    * cannot represent an object type (only pointers to objects), there is no
    * way to generate a load for a field that is itself an object. The workaround
    * is to use the field's address instead. This is not an elegent solution and
    * should be revisited.
    */
   TR::IlValue *StructFieldInstanceAddress(const char* structName, const char* fieldName, TR::IlValue* obj);
   TR::IlValue *UnionFieldInstanceAddress(const char* unionName, const char* fieldName, TR::IlValue* obj);

   // vector memory
   TR::IlValue *VectorLoad(const char *name);
   TR::IlValue *VectorLoadAt(TR::IlType *dt, TR::IlValue *address);
   void VectorStore(const char *name, TR::IlValue *value);
   void VectorStoreAt(TR::IlValue *address, TR::IlValue *value);

   // control
   void Transaction(TR::IlBuilder **persistentFailureBuilder, TR::IlBuilder **transientFailureBuilder, TR::IlBuilder **fallThroughBuilder);
   void TransactionAbort();
   void AppendBuilder(TR::IlBuilder *builder);
   TR::IlValue *Call(const char *name, TR::IlType *returnType, int32_t numArgs, TR::IlValue **argValues);
   TR::IlValue *ComputedCall(const char *name, int32_t numArgs, TR::IlValue **args);
   void Goto(TR::IlBuilder **dest);
   void Goto(TR::IlBuilder *dest);
   void Return();
   void Return(TR::IlValue *value);
   virtual void ForLoop(bool countsUp,
                const char *indVar,
                TR::IlBuilder *body,
                TR::IlBuilder *breakBuilder,
                TR::IlBuilder *continueBuilder,
                TR::IlValue *initial,
                TR::IlValue *iterateWhile,
                TR::IlValue *increment);
   virtual void ForLoop(bool countsUp,
                const char *indVar,
                TR::IlBuilder **body,
                TR::IlBuilder **breakBuilder,
                TR::IlBuilder **continueBuilder,
                TR::IlValue *initial,
                TR::IlValue *iterateWhile,
                TR::IlValue *increment);

   void ForLoopUp(const char *indVar,
                  TR::IlBuilder **body,
                  TR::IlValue *initial,
                  TR::IlValue *iterateWhile,
                  TR::IlValue *increment)
      {
      ForLoop(true, indVar, body, NULL, NULL, initial, iterateWhile, increment);
      }

   void ForLoopDown(const char *indVar,
                    TR::IlBuilder **body,
                    TR::IlValue *initial,
                    TR::IlValue *iterateWhile,
                    TR::IlValue *increment)
      {
      ForLoop(false, indVar, body, NULL, NULL, initial, iterateWhile, increment);
      }

   void ForLoopWithBreak(bool countsUp,
                         const char *indVar,
                         TR::IlBuilder **body,
                         TR::IlBuilder **breakBody,
                         TR::IlValue *initial,
                         TR::IlValue *iterateWhile,
                         TR::IlValue *increment)
      {
      ForLoop(countsUp, indVar, body, breakBody, NULL, initial, iterateWhile, increment);
      }

   void ForLoopWithContinue(bool countsUp,
                            const char *indVar,
                            TR::IlBuilder **body,
                            TR::IlBuilder **continueBody,
                            TR::IlValue *initial,
                            TR::IlValue *iterateWhile,
                            TR::IlValue *increment)
      {
      ForLoop(countsUp, indVar, body, NULL, continueBody, initial, iterateWhile, increment);
      }

     void DoWhileLoop(const char *exitCondition, TR::IlBuilder **body, TR::IlBuilder **breakBuilder = NULL, TR::IlBuilder **continueBuilder = NULL);
     void WhileDoLoop(const char *exitCondition, TR::IlBuilder **body, TR::IlBuilder **breakBuilder = NULL, TR::IlBuilder **continueBuilder = NULL);
  
     void WhileDoLoopWithBreak(const char *exitCondition, TR::IlBuilder **body, TR::IlBuilder **breakBuilder)
          {
          WhileDoLoop(exitCondition, body, breakBuilder);
          }

     void WhileDoLoopWithContinue(const char *exitCondition, TR::IlBuilder **body, TR::IlBuilder **continueBuilder)
          {
          WhileDoLoop(exitCondition, body, NULL, continueBuilder);
          }

     void DoWhileLoopWithBreak(const char *exitCondition, TR::IlBuilder **body, TR::IlBuilder **breakBuilder)
          {
          DoWhileLoop(exitCondition, body, breakBuilder);
          }

     void DoWhileLoopWithContinue(const char *exitCondition, TR::IlBuilder **body, TR::IlBuilder **continueBuilder)
          {
          DoWhileLoop(exitCondition, body, NULL, continueBuilder);
          }

   /* @brief creates an AND nest of short-circuited conditions, for each term pass an IlBuilder containing the condition and the IlValue that computes the condition */
   void IfAnd(TR::IlBuilder **allTrueBuilder, TR::IlBuilder **anyFalseBuilder, int32_t numTerms, ... );
   /* @brief creates an OR nest of short-circuited conditions, for each term pass an IlBuilder containing the condition and the IlValue that computes the condition */
   void IfOr(TR::IlBuilder **anyTrueBuilder, TR::IlBuilder **allFalseBuilder, int32_t numTerms, ... );

   void IfCmpNotEqualZero(TR::IlBuilder **target, TR::IlValue *condition);
   void IfCmpNotEqualZero(TR::IlBuilder *target, TR::IlValue *condition);
   void IfCmpNotEqual(TR::IlBuilder **target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpNotEqual(TR::IlBuilder *target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpEqualZero(TR::IlBuilder **target, TR::IlValue *condition);
   void IfCmpEqualZero(TR::IlBuilder *target, TR::IlValue *condition);
   void IfCmpEqual(TR::IlBuilder **target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpEqual(TR::IlBuilder *target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpLessThan(TR::IlBuilder **target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpLessThan(TR::IlBuilder *target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpUnsignedLessThan(TR::IlBuilder **target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpUnsignedLessThan(TR::IlBuilder *target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpLessOrEqual(TR::IlBuilder **target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpLessOrEqual(TR::IlBuilder *target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpUnsignedLessOrEqual(TR::IlBuilder **target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpUnsignedLessOrEqual(TR::IlBuilder *target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpGreaterThan(TR::IlBuilder **target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpGreaterThan(TR::IlBuilder *target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpUnsignedGreaterThan(TR::IlBuilder **target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpUnsignedGreaterThan(TR::IlBuilder *target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpGreaterOrEqual(TR::IlBuilder **target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpGreaterOrEqual(TR::IlBuilder *target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpUnsignedGreaterOrEqual(TR::IlBuilder **target, TR::IlValue *left, TR::IlValue *right);
   void IfCmpUnsignedGreaterOrEqual(TR::IlBuilder *target, TR::IlValue *left, TR::IlValue *right);

   void IfThenElse(TR::IlBuilder *thenPath, TR::IlBuilder *elsePath, TR::IlValue *condition);
   void IfThenElse(TR::IlBuilder **thenPath, TR::IlBuilder **elsePath, TR::IlValue *condition);
   virtual void IfThen(TR::IlBuilder **thenPath, TR::IlValue *condition)
      {
      IfThenElse(thenPath, NULL, condition);
      }
   void Switch(const char *selectionVar,
               TR::IlBuilder **defaultBuilder,
               uint32_t numCases,
               ...);
   void Switch(const char *selectionVar,
               TR::IlBuilder **defaultBuilder,
               uint32_t numCases,
               JBCase** cases);

    /**
    * @brief associates this object with a particular client object
    */
   void setClient(void *client)
      {
      _client = client;
      }

   /**
    * @brief returns the client object associated with this object, allocating it if necessary
    */
   void *client();

   /**
    * @brief Set the ClientCallback buildIL function
    * 
    * @param callback function pointer to the buildIL() callback for the client
    */
   void setClientCallback_buildIL(void *callback)
      {
      _clientCallbackBuildIL = (ClientBuildILCallback)callback;
      }

   /**
    * @brief Set the Client Allocator function
    * 
    * @param allocator function pointer to the client object allocator
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
   /**
    * @brief MethodBuilder parent for this IlBuilder object
    */
   TR::MethodBuilder      * _methodBuilder;

   /**
    * @brief pointer to buildIL callback function for this object
    * usually NULL, but client objects can set this via setBuildILCallback() to be called
    * when buildIL is called on this object
    */
   ClientBuildILCallback         _clientCallbackBuildIL;

   /**
    * @brief pointer to allocator function for this object.
    *
    * Clients must set this pointer using setClientAllocator().
    * When this allocator is called, a pointer to the current
    * class (this) will be passed as argument. The expected
    * returned value is a pointer to the base type of the
    * client object.
    */
   static ClientAllocator        _clientAllocator;

   /**
    * @brief pointer to a client object that corresponds to this object
    */
   void                        * _client;

   /**
    * @brief pointer to impl getter function
    *
    * Clients must set this pointer using setImplGetter().
    * When called with an instance of a client object,
    * the function must return the corresponding
    * implementation object
    */
   static ImplGetter             _getImpl;

   TR::IlBuilder *asIlBuilder();
   TR::IlValue *newValue();
   TR::IlBuilder *createBuilderIfNeeded(TR::IlBuilder *builder);

   TR::JitBuilderRecorder *recorder() const;
   TR::JitBuilderRecorder *clearRecorder();
   void restoreRecorder(TR::JitBuilderRecorder *recorder);

   void convertTo(TR::IlValue *returnValue, TR::IlType *dt, TR::IlValue *v, const char *s);
   void binaryOp(const TR::IlValue *returnValue, const TR::IlValue *left, const TR::IlValue *right, const char *s);
   void shiftOp(const TR::IlValue *returnValue, const TR::IlValue *v, const TR::IlValue *amount, const char *s);
   void unaryOp(TR::IlValue *returnValue, TR::IlValue *v, const char *s);
   };

} // namespace OMR

#endif // !defined(OMR_ILBUILDERRECORDER_INCL)
