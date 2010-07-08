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
            DEBUG_PKT( outs() << "analyzing liveness of blocks in function '" << f.getNameStr() << "'...\n"; );
            DEBUG_PKT( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );

			// initialize map
			for (Function::iterator BB=f.begin(), BBE=f.end(); BB!=BBE; ++BB) {
				LiveSetType* liveInSet = new LiveSetType();
				LiveSetType* liveOutSet = new LiveSetType();
				liveValueMap.insert(std::make_pair(BB, std::make_pair(liveInSet, liveOutSet)));
			}

			std::set<BasicBlock*> visitedBlocks;
			computeBlockLiveValues(&f.getEntryBlock(), visitedBlocks);

			DEBUG_PKT( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
			DEBUG_PKT( outs() << "liveness analysis of function '" << f.getNameStr() << "' finished!\n"; );
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
//			delete loopInfo;
//			for (LiveValueMapType::iterator it=liveValueMap.begin(), E=liveValueMap.end(); it!=E; ++it) {
//				delete it->second.first;
//				delete it->second.second;
//			}
		}

		typedef std::set<Value*> LiveSetType;
		typedef std::pair< LiveSetType*, LiveSetType* > LiveValueSetType;
		typedef std::map< BasicBlock*, LiveValueSetType > LiveValueMapType;

		inline LiveValueSetType* getBlockLiveValues(BasicBlock* block) {
			assert (block);
			LiveValueMapType::iterator it = liveValueMap.find(block);
			if (it == liveValueMap.end()) return NULL;

			return &(it->second);
		}
		inline LiveSetType* getBlockLiveInValues(BasicBlock* block) {
			assert (block);
			LiveValueMapType::iterator it = liveValueMap.find(block);
			if (it == liveValueMap.end()) return NULL;

			return it->second.first;
		}
		inline LiveSetType* getBlockLiveOutValues(BasicBlock* block) {
			assert (block);
			LiveValueMapType::iterator it = liveValueMap.find(block);
			if (it == liveValueMap.end()) return NULL;

			return it->second.second;
		}

		void getBlockInternalLiveValues(Instruction* inst, LiveSetType& liveVals) {
			assert (inst && inst->getParent());
			BasicBlock* block = inst->getParent();
			DEBUG_PKT( outs() << "\ngetBlockInternalLiveValues(" << block->getNameStr() << ")\n"; );

			// create map which holds order of instructions
			unsigned frontierId = 0;
			std::map<Instruction*, unsigned> instMap;
			unsigned i=0;
			DEBUG_PKT( outs() << "instructions:\n"; );
			for (BasicBlock::iterator I=block->begin(), E=block->end(); I!=E; ++I) {
				if (inst == I) frontierId = i;
				instMap[I] = i++;
				DEBUG_PKT( outs() << " (" << i-1 << ")" << (inst == I ? "*" : " ") << *I << "\n"; );
			}
			DEBUG_PKT( outs() << "\n"; );

			// check each use of each instruction if it lies "behind" inst
			// if so, it is live after inst
			for (BasicBlock::iterator I=block->begin(), E=block->end(); I!=E; ++I) {
				const unsigned instId = instMap[I];
				if (instId > frontierId) break;
				DEBUG_PKT( outs() << "checking uses of instruction " << instId << "\n"; );
				for (Instruction::use_iterator U=I->use_begin(), UE=I->use_end(); U!=UE; ++U) {
					assert (isa<Instruction>(U));
					Instruction* useI = cast<Instruction>(U);
					const unsigned useId = instMap[useI];
					const bool isLive = useId > frontierId;
					if (isLive) {
						liveVals.insert(I);
					}
					DEBUG_PKT( outs() << "  use: (" << useId << ") - is " << (isLive ? "" : "not ") << "live!\n"; );
				}
			}
		}

		// this is so ugly... =)
		void mapLiveValues(Function* f, Function* newF, DenseMap<const Value*, Value*>& valueMap) {
			assert (f && newF);
			assert (!valueMap.empty());
			DEBUG_PKT( outs() << "\nmapping live values from function '" << f->getNameStr() << "' to new function '" << newF->getNameStr() << "'...\n"; );

			for (Function::iterator BB=f->begin(), BBE=f->end(); BB!=BBE; ++BB) {
				DEBUG_PKT( outs() << "mapping live values of basic block '" << BB->getNameStr() << "'...\n"; );
				LiveSetType* liveInSet = getBlockLiveInValues(BB);
				LiveSetType* liveOutSet = getBlockLiveOutValues(BB);

				if (liveInSet) {
					Value** tmpVals = new Value*[liveInSet->size()]();
					int i=0;
					for (LiveSetType::iterator it=liveInSet->begin(), E=liveInSet->end(); it!=E; ++it) {
						tmpVals[i++] = *it;
					}
					liveInSet->clear();
					--i;
					for ( ; i>=0; --i) {
						Value* val = tmpVals[i];
						DEBUG_PKT( if (!isa<BasicBlock>(val)) outs() << "  mapped live-in value: " << *val << " -> " << *(valueMap[val]) << "\n"; );
						DEBUG_PKT( if (isa<BasicBlock>(val)) outs() << "  mapped live-in value: " << val->getNameStr() << " -> " << (valueMap[val])->getNameStr() << "\n"; );
						liveInSet->insert(valueMap[val]);
					}
					delete [] tmpVals;
					//for (LiveInSetType::iterator it=liveInSet->begin(), E=liveInSet->end(); it!=E; ) {
						//Value* val = *it;
						//liveInSet->insert(valueMap[val]);
						//liveInSet->erase(it++);
					//}
				}

				if (liveOutSet) {
					Value** tmpVals = new Value*[liveOutSet->size()]();
					int i=0;
					for (LiveSetType::iterator it=liveOutSet->begin(), E=liveOutSet->end(); it!=E; ++it) {
						tmpVals[i++] = *it;
					}
					liveOutSet->clear();
					--i;
					for ( ; i>=0; --i) {
						Value* val = tmpVals[i];
						//DEBUG_PKT( if (!isa<BasicBlock>(val)) outs() << "mapped live-out value: " << *val << " -> " << *(valueMap[val]) << "\n"; );
						//DEBUG_PKT( if (isa<BasicBlock>(val)) outs() << "mapped live-out value: " << val->getNameStr() << " -> " << (valueMap[val])->getNameStr() << "\n"; );
						liveOutSet->insert(valueMap[val]);
					}
					delete [] tmpVals;
				}

			}

			// map basic blocks in map
			BasicBlock** tmpBlocks = new BasicBlock*[liveValueMap.size()]();
			LiveValueSetType* tmpSets = new LiveValueSetType[liveValueMap.size()]();
			int i=0;
			for (LiveValueMapType::iterator it=liveValueMap.begin(), E=liveValueMap.end(); it!=E; ++it) {
				tmpBlocks[i] = it->first;
				tmpSets[i++] = it->second;
			}
			liveValueMap.clear();
			--i;
			for ( ; i>=0; --i) {
				BasicBlock* newBlock = cast<BasicBlock>(valueMap[tmpBlocks[i]]);
				liveValueMap[newBlock] = tmpSets[i];
			}

			DEBUG_PKT( outs() << "\n"; );
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
		//
		// TODO: what is with the case of a loop latch with only a branch
		//       back to the header? does it have live-values from blocks which
		//       dominate the header?
		void computeBlockLiveValues(BasicBlock* block, std::set<BasicBlock*>& visitedBlocks) {
			assert (block && loopInfo);
			DEBUG_PKT( outs() << "getBlockLiveValues(" << block->getNameStr() << ")\n"; );
			if (visitedBlocks.find(block) != visitedBlocks.end()) {
				DEBUG_PKT( outs() << "  block already seen, skipped!\n"; );
				return; // block already seen
			}
			visitedBlocks.insert(block);

			// get sets for this block
			assert (liveValueMap.find(block) != liveValueMap.end());
			LiveValueSetType liveValueSet = liveValueMap.find(block)->second;
			LiveSetType* liveInSet = liveValueSet.first;
			LiveSetType* liveOutSet = liveValueSet.second;

			Loop* loop = loopInfo->getLoopFor(block);

			// post-order DFS
			for (succ_iterator S = succ_begin(block), SE = succ_end(block); S!=SE; ++S) {
				BasicBlock* succBB = *S;
				// if successor is loop header, process it differently (no further recursion)
				if (loop && loop->getHeader() == succBB) {
					DEBUG_PKT( outs() << "  block is loop latch of loop: "; loop->dump(); );
					for (BasicBlock::iterator I=succBB->begin(), IE=succBB->getFirstNonPHI(); I!=IE; ++I) {
						assert (isa<PHINode>(I));
						PHINode* phi = cast<PHINode>(I);
						Value* val = phi->getIncomingValueForBlock(block);
						assert (val);

						// ignore all values that are neither an instruction nor an argument
						if (!isa<Instruction>(val) && !isa<Argument>(val)) continue;

						liveOutSet->insert(val);
						DEBUG_PKT( outs() << "  added value to liveOut-set: " << *val << "\n"; );
					}
					continue;
				}
				
				// First, check if we have already processed this successor
				// and recurse if necessary.
				if (visitedBlocks.find(succBB) == visitedBlocks.end()) {
					computeBlockLiveValues(succBB, visitedBlocks);
				}

				assert (liveValueMap.find(succBB) != liveValueMap.end());
				LiveValueSetType succLiveValueSet = liveValueMap.find(succBB)->second;
				LiveSetType* succLiveInSet = succLiveValueSet.first;


				// liveOut-set is the union of all liveIn-sets of successors [dataflow-equation 3]
				liveOutSet->insert(succLiveInSet->begin(), succLiveInSet->end());
				for (LiveSetType::iterator it=succLiveInSet->begin(), E=succLiveInSet->end(); it!=E; ++it) {
					DEBUG_PKT( outs() << "  added value to liveOut-set: " << **it << "\n"; );
				}

				// remove values from liveOut-set that flow into successor-phi
				// from other directions than current block
				for (BasicBlock::iterator I=succBB->begin(), IE=succBB->getFirstNonPHI(); I!=IE; ++I) {
					assert (isa<PHINode>(I));
					PHINode* phi = cast<PHINode>(I);
					//Value* val = phi->getIncomingValueForBlock(block);

					for (unsigned i=0, e=phi->getNumIncomingValues(); i<e; ++i) {
						if (phi->getIncomingBlock(i) == block) continue;

						liveOutSet->erase(phi->getIncomingValue(i));
						DEBUG_PKT( outs() << "  removed value from liveOut-set: " << *phi->getIncomingValue(i) << "\n"; );
					}
				}

				// liveIn-set is equal to liveOut-set (before applying kill/gen sets)
				liveInSet->insert(liveOutSet->begin(), liveOutSet->end());
				for (LiveSetType::iterator it=liveOutSet->begin(), E=liveOutSet->end(); it!=E; ++it) {
					DEBUG_PKT( outs() << "  added value to liveIn-set: " << **it << "\n"; );
				}
			}


			for (BasicBlock::iterator I=block->begin(), IE=block->end(); I!=IE; ++I) {
				// remove defined values from liveIn-set [kill]
				if (!liveInSet->empty()) {
					liveInSet->erase(cast<Value>(I));
					DEBUG_PKT( outs() << "  removed value from liveIn-set: " << *I << "\n"; );
				}

				// add used values to liveIn-set [gen]
				for (Instruction::op_iterator OP=I->op_begin(), OPE=I->op_end(); OP!=OPE; ++OP) {
					if (!isa<Value>(OP)) continue; // ignore all operands that are no instructions

					// ignore all values that are neither an instruction nor an argument
					if (Instruction* opI = dyn_cast<Instruction>(OP)) {
						// ignore operands that are defined in same block
						if (opI->getParent() != block) {
							liveInSet->insert(opI);
							DEBUG_PKT( outs() << "  added value to liveIn-set: " << *opI << "\n"; );
						}
					} else if (Argument* argI = dyn_cast<Argument>(OP)) {
						liveInSet->insert(cast<Value>(argI));
						DEBUG_PKT( outs() << "  added value to liveIn-set: " << *argI << "\n"; );
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

