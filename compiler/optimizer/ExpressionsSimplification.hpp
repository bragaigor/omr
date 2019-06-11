/*******************************************************************************
 * Copyright (c) 2000, 2019 IBM Corp. and others
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

#ifndef EXPSIMP_INCL
#define EXPSIMP_INCL

#include <stdint.h>
#include "env/TRMemory.hpp"
#include "il/Node.hpp"
#include "optimizer/Optimization.hpp"
#include "optimizer/OptimizationManager.hpp"

class TR_BitVector;
class TR_RegionStructure;
class TR_Structure;
namespace TR { class Block; }
namespace TR { class CFGNode; }
namespace TR { class TreeTop; }
template <class T> class List;
template <class T> class ListIterator;

/*
 * Class TR_ExpressionsSimplification
 * ==================================
 *
 * Expression simplification is a loop optimization that aims to optimize 
 * local variable updates (inside the loop) that can be predicted and done 
 * in the loop pre-header (outside the loop). The optimization only runs 
 * at higher optimization levels like scorching. Here is an example:
 * 
 * i=0;
 * total = 0;
 * while (i < N)
 *    {
 *    total = total + M;
 *    ...other code..
 *    i = i + 1;
 *    }
 * 
 * would be converted by expression simplification to:
 * 
 * i=0;
 * total = 0;
 * total = total + M*N;
 * while (i < N)
 *    {
 *    ...other code..
 *    i = i + 1;
 *    }
 * 
 * Thereby avoiding the update to total every time through the loop. This 
 * optimization is currently done for updates that are additions, subtractions, 
 * xors and negations inside the loop, i.e. the optimization knows the 
 * equivalent expression to emit in the loop pre-header for these kinds of 
 * updates inside the loop. Note that there are simplifying assumptions 
 * around the complexity of expressions allowed as M and N in the above 
 * example to keep the analysis relatively cheap.
 * 
 * While the optimization does win big when it succeeds in microbenchmarks 
 * it rarely makes a significant difference in real world programs (that 
 * simply do not contain enough such opportunities typically) meaning it 
 * has not received too much development focus over and above what was needed 
 * to catch some important cases that were observed in relevant benchmarks. 
 * Since this only runs at higher opt levels and is not particularly expensive, 
 * it does not interfere too much in real world Java programs either 
 * in terms of compile time or throughput.
 */
class TR_ExpressionsSimplification : public TR::Optimization
   {
   public:
   // Performs isolated store elimination
   //
   TR_ExpressionsSimplification(TR::OptimizationManager *manager);
   static TR::Optimization *create(TR::OptimizationManager *manager)
      {
      return new (manager->allocator()) TR_ExpressionsSimplification(manager);
      }

   class LoopInfo
      {
      public:
      TR_ALLOC(TR_Memory::ExpressionsSimplification)
      LoopInfo(TR::Node* boundry, int32_t lowerBound, int32_t upperBound, int32_t increment, bool equals)
         : _boundry(boundry), _lowerBound32(lowerBound), _upperBound32(upperBound), _increment32(increment), _equals(equals), _is64Bit(false)
            { }
      LoopInfo(TR::Node* boundry, int64_t lowerBound, int64_t upperBound, int64_t increment, bool equals)
         : _boundry(boundry), _lowerBound64(lowerBound), _upperBound64(upperBound), _increment64(increment), _equals(equals), _is64Bit(true)
            { }

      int32_t getIncrement32() { return _increment32; }
      TR::Node* getBoundaryNode() { return _boundry; }
      bool isEquals() {return _equals;}
      bool is64Bit() {return _is64Bit;}

      int32_t getNumIterations32()
         {
         if (_increment32 == 0)
            return 0;

         if ((_increment32 > 0 && _lowerBound32 > _upperBound32) || (_increment32 < 0 && _lowerBound32 < _upperBound32))
            return 0;

         if (isEquals())
            return (_upperBound32 - _lowerBound32 + _increment32)/_increment32;
         else if (_increment32 > 0)
            return (_upperBound32 - _lowerBound32 + _increment32 - 1)/_increment32;
         else
            return (_upperBound32 - _lowerBound32 + _increment32 + 1)/_increment32;
         }

      int64_t getNumIterations64()
         {
         if (_increment64 == 0)
            return 0;

         if ((_increment64 > 0 && _lowerBound64 > _upperBound64) || (_increment64 < 0 && _lowerBound64 < _upperBound64))
            return 0;

         if (isEquals())
            return (_upperBound64 - _lowerBound64 + _increment64)/_increment64;
         else if (_increment32 > 0)
            return (_upperBound64 - _lowerBound64 + _increment64 - 1)/_increment64;
         else
            return (_upperBound64 - _lowerBound64 + _increment64 + 1)/_increment64;
         }

      int64_t getNumIterations()
         {
         if (_is64Bit)
            return getNumIterations64();
         else
            return getNumIterations32();
         }

      private:
      TR::Node* _boundry;
      int32_t  _lowerBound32;
      int64_t  _lowerBound64;
      int32_t  _upperBound32;
      int64_t  _upperBound64;
      int32_t  _increment32;
      int64_t  _increment64;
      bool     _is64Bit;
      bool     _equals;
      };

   virtual int32_t perform();
   virtual const char * optDetailString() const throw();

   protected:
   vcount_t _visitCount;


   private:
   void findAndSimplifyInvariantLoopExpressions(TR_RegionStructure *);
   void invalidateCandidates();
   void removeUncertainBlocks(TR_RegionStructure*, List<TR::Block> *blocksInLoop);
   LoopInfo* findLoopInfo(TR_RegionStructure*);
   TR::Block * findPredecessorBlock(TR::CFGNode *);
   void transformNode(TR::Node *, TR::Block *);
   bool findUseInRegion(TR_RegionStructure *, uint16_t, uint16_t);
   bool findUseInRegion(TR::Node*, uint16_t, uint16_t);
   void removeCandidate(TR_RegionStructure *);
   void removeCandidate(TR::Node *, TR::TreeTop *);
   int32_t perform(TR_Structure *);
   void simplifyInvariantLoopExpressions(ListIterator<TR::Block> &blocks);
   void setSummationReductionCandidates(TR::Node *currentNode, TR::TreeTop *tt);
   void setStoreMotionCandidates(TR::Node *currentNode, TR::TreeTop *tt);
   bool tranformSummationReductionCandidate(TR::TreeTop *treeTop, LoopInfo *loopInfo, bool *isPreheaderBlockInvalid);
   void tranformStoreMotionCandidate(TR::TreeTop *treeTop, bool *isPreheaderBlockInvalid);

   TR::Node* iaddisubSimplifier(TR::Node *, LoopInfo*);
   TR::Node* laddlsubSimplifier(TR::Node *, LoopInfo*);
   TR::Node* ixorinegSimplifier(TR::Node *, LoopInfo*, bool *);
   TR::Node* lxorlnegSimplifier(TR::Node *, LoopInfo*, bool *);
   bool checkForLoad(TR::Node *node, TR::Node *load);
   void removeUnsupportedCandidates();
   bool isSupportedNodeForExpressionSimplification(TR::Node *node);


   TR_RegionStructure* _currentRegion;
   List<TR::TreeTop> *_candidateTTs;

   TR_BitVector *_supportedExpressions;
   };

#endif
