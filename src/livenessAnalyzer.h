/**
 * @file   livenessAnalyzer.h
 * @date   28.06.2010
 * @author Ralf Karrenberg
 *
 * Copyright (C) 2010 Saarland University
 *
 * This file is part of packetizedOpenCLDriver.
 *
 * packetizedOpenCLDriver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * packetizedOpenCLDriver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with packetizedOpenCLDriver.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _LIVENESSANALYZER_H
#define	_LIVENESSANALYZER_H


#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif
#define DEBUG_TYPE "livenessanalyzer"

#include <llvm/Support/raw_ostream.h>

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>

#include <llvm/Analysis/LoopInfo.h>

#define DEBUG_PKT(x) do { x } while (false)
//#define DEBUG_PKT(x) ((void)0)

using namespace llvm;

namespace {
    class VISIBILITY_HIDDEN LivenessAnalyzer : public FunctionPass {
    public:
        static char ID; // Pass identification, replacement for typeid
        LivenessAnalyzer(const bool verbose_flag = false) : FunctionPass(&ID), verbose(verbose_flag) {}
		~LivenessAnalyzer() { releaseMemory(); }

        virtual bool runOnFunction(Function &f) {

            // get loop info
            loopInfo = &getAnalysis<LoopInfo>();
			DEBUG_PKT( print(outs(), f.getParent()); );
			
            DEBUG_PKT( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
            DEBUG_PKT( outs() << "analyzing liveness...\n"; );
            DEBUG_PKT( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );

			// initialize map
			for (Function::iterator BB=f.begin(), BBE=f.end(); BB!=BBE; ++BB) {
				LiveInSetType* liveInSet = new LiveInSetType();
				LiveOutSetType* liveOutSet = new LiveOutSetType();
				liveValueMap.insert(std::make_pair(BB, std::make_pair(liveInSet, liveOutSet)));
			}

			std::set<BasicBlock*> visitedBlocks;
			computeBlockLiveValues(&f.getEntryBlock(), visitedBlocks);

			DEBUG_PKT( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
			DEBUG_PKT( outs() << "liveness analysis finished!\n"; );
			DEBUG_PKT( print(outs(), NULL); );
			DEBUG_PKT( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"; );

			return false;
        }

        void print(raw_ostream& o, const Module *M) const {}
        virtual void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.addRequired<LoopInfo>();
			AU.setPreservesAll();
        }
		void releaseMemory() {
			delete loopInfo;
			for (LiveValueMapType::iterator it=liveValueMap.begin(), E=liveValueMap.end(); it!=E; ++it) {
				delete it->second.first;
				delete it->second.second;
			}
		}

		typedef std::set<Value*> LiveInSetType;
		typedef std::set<Value*> LiveOutSetType;
		typedef std::pair< LiveInSetType*, LiveOutSetType* > LiveValueSetType;
		typedef std::map< BasicBlock*, LiveValueSetType > LiveValueMapType;

		LiveValueSetType* getBlockLiveValues(BasicBlock* block) {
			LiveValueMapType::iterator it = liveValueMap.find(block);
			if (it == liveValueMap.end()) return NULL;

			return &(it->second);
		}
		LiveInSetType* getBlockLiveInValues(BasicBlock* block) {
			LiveValueMapType::iterator it = liveValueMap.find(block);
			if (it == liveValueMap.end()) return NULL;

			return it->second.first;
		}
		LiveOutSetType* getBlockLiveOutValues(BasicBlock* block) {
			LiveValueMapType::iterator it = liveValueMap.find(block);
			if (it == liveValueMap.end()) return NULL;

			return it->second.second;
		}

    private:
        const bool verbose;
		LoopInfo* loopInfo;

		LiveValueMapType liveValueMap;

		// dataflow analysis for liveness information
		// dataflow equations:
		// 1) LiveIn(block) = gen(block) u (LiveOut(block) - kill(block))
		// 2) LiveOut(final) = {}
		// 3) LiveOut(block) = u LiveIn(all successors)
		void computeBlockLiveValues(BasicBlock* block, std::set<BasicBlock*>& visitedBlocks) {
			assert (block && loopInfo);
			DEBUG_PKT( outs() << "getBlockLiveValues(" << block->getNameStr() << ")\n"; );
			// get sets for this block
			assert (liveValueMap.find(block) != liveValueMap.end());
			LiveValueSetType liveValueSet = liveValueMap.find(block)->second;
			LiveInSetType* liveInSet = liveValueSet.first;
			LiveOutSetType* liveOutSet = liveValueSet.second;

			// handle loops separately:
			// if block is a loop header, directly recurse into the exiting blocks
			// of the loop before computing live-values inside the loop
			if (loopInfo->isLoopHeader(block)) {
				Loop* loop = loopInfo->getLoopFor(block);
				DEBUG_PKT( outs() << "  block is header of loop: "; loop->dump(); outs() << "\n"; );

				SmallVector<BasicBlock*, 2> exitBlocks;
				loop->getUniqueExitBlocks(exitBlocks);

				for (SmallVector<BasicBlock*, 2>::iterator it=exitBlocks.begin(), E=exitBlocks.end(); it!=E; ++it) {
					BasicBlock* succBB = *it;
					computeBlockLiveValues(succBB, visitedBlocks);
				}
			}


			if (visitedBlocks.find(block) != visitedBlocks.end()) {
				DEBUG_PKT( outs() << "  block already seen, skipped!\n"; );
				return; // block already seen
			}
			visitedBlocks.insert(block);

			// post-order DFS
			for (succ_iterator S = succ_begin(block), SE = succ_end(block); S!=SE; ++S) {
				BasicBlock* succBB = *S;
				
				// First, check if we have already processed this successor
				// and recurse if necessary.
				if (visitedBlocks.find(succBB) == visitedBlocks.end()) {
					computeBlockLiveValues(succBB, visitedBlocks);
				}

				assert (liveValueMap.find(succBB) != liveValueMap.end());
				LiveValueSetType succLiveValueSet = liveValueMap.find(succBB)->second;
				LiveInSetType* succLiveInSet = succLiveValueSet.first;


				// liveOut-set is the union of all liveIn-sets of successors [dataflow-equation 3]
				liveOutSet->insert(succLiveInSet->begin(), succLiveInSet->end());

				// liveIn-set is equal to liveOut-set (before applying kill/gen sets)
				liveInSet->insert(succLiveInSet->begin(), succLiveInSet->end());
			}


			for (BasicBlock::iterator I=block->begin(), IE=block->end(); I!=IE; ++I) {
				// remove defined values from liveIn-set [kill]
				if (!liveInSet->empty()) liveInSet->erase(cast<Value>(I));

				// add used values to liveIn-set [gen]
				for (Instruction::op_iterator OP=I->op_begin(), OPE=I->op_end(); OP!=OPE; ++OP) {
					if (!isa<Value>(OP)) continue; // ignore all operands that are no instructions

					// ignore all values that are neither an instruction nor an argument
					if (Instruction* opI = dyn_cast<Instruction>(OP)) {
						// ignore operands that are defined in same block
						if (opI->getParent() != block) liveInSet->insert(opI);
					} else if (Argument* argI = dyn_cast<Argument>(OP)) {
						liveInSet->insert(cast<Value>(argI));
					}
				}
			}

			DEBUG_PKT(
				outs() << "\nLive-In set of block'" << block->getNameStr() << "':\n";
				for (std::set<Value*>::iterator it=liveInSet->begin(), E=liveInSet->end(); it!=E; ++it) {
					outs() << " - " << **it << "\n";
				}
				outs() << "\nLive-Out set of block '" << block->getNameStr() << "':\n";
				for (std::set<Value*>::iterator it=liveOutSet->begin(), E=liveOutSet->end(); it!=E; ++it) {
					outs() << " - " << **it << "\n";
				}
				outs() << "\n";
			);
		}

	};
}

char LivenessAnalyzer::ID = 0;
static RegisterPass<LivenessAnalyzer> LA("liveness-analysis", "Liveness Analysis");

// Public interface to the LivenessAnalysis pass
namespace llvm {
	FunctionPass* createLivenessAnalyzerPass() {
		return new LivenessAnalyzer();
	}
}


#endif	/* _LIVENESSANALYZER_H */

