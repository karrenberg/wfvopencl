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

			computeLiveValues(&f);

			DEBUG_PKT( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
			DEBUG_PKT( print(outs()); );
			DEBUG_PKT( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
			DEBUG_PKT( outs() << "liveness analysis of function '" << f.getNameStr() << "' finished!\n"; );
			DEBUG_PKT( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"; );

			return false;
        }

        void print(raw_ostream& o, const Module *M=NULL) const {
			for (LiveValueMapType::const_iterator it=liveValueMap.begin(), E=liveValueMap.end(); it!=E; ++it) {
				BasicBlock* block = it->first;
				LiveValueSetType liveValues = it->second;
				LiveSetType* liveInValues = liveValues.first;
				LiveSetType* liveOutValues = liveValues.second;
				outs() << "\nLive-In values of block '" << block->getNameStr() << "':\n";
				for (LiveSetType::iterator it2=liveInValues->begin(), E2=liveInValues->end(); it2!=E2; ++it2) {
					outs() << " * " << **it2 << "\n";
				}
				outs() << "\nLive-Out values of block '" << block->getNameStr() << "':\n";
				for (LiveSetType::iterator it2=liveOutValues->begin(), E2=liveOutValues->end(); it2!=E2; ++it2) {
					outs() << " * " << **it2 << "\n";
				}
				outs() << "\n";
			}
		}
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

		// create map which holds order of instructions
		unsigned createInstructionOrdering(BasicBlock* block, Instruction* frontier, std::map<Instruction*, unsigned>& instMap) const {
			unsigned frontierId = 0;
			unsigned i=0;
			DEBUG_PKT( outs() << "instructions:\n"; );
			for (BasicBlock::iterator I=block->begin(), E=block->end(); I!=E; ++I) {
				if (frontier == I) frontierId = i;
				instMap[I] = i++;
				DEBUG_PKT( outs() << " (" << i-1 << ")" << (frontier == I ? "*" : " ") << *I << "\n"; );
			}
			DEBUG_PKT( outs() << "\n"; );

			return frontierId;
		}
		void getBlockInternalLiveInValues(Instruction* inst, LiveSetType& liveVals) {
			assert (inst && inst->getParent());
			BasicBlock* block = inst->getParent();
			DEBUG_PKT( outs() << "\ngetBlockInternalLiveValues(" << block->getNameStr() << ")\n"; );

			// create map which holds order of instructions
			std::map<Instruction*, unsigned> instMap;
			const unsigned frontierId = createInstructionOrdering(block, inst, instMap);

			// check each use of each instruction if it lies "behind" inst
			// if so, it is live after inst
			for (BasicBlock::iterator I=block->begin(), E=block->end(); I!=E; ++I) {
				const unsigned instId = instMap[I];
				if (instId >= frontierId) break;
				DEBUG_PKT( outs() << "checking uses of instruction " << instId << "\n"; );
				for (Instruction::use_iterator U=I->use_begin(), UE=I->use_end(); U!=UE; ++U) {
					assert (isa<Instruction>(U));
					Instruction* useI = cast<Instruction>(U);

					// if the use is in a different block, the value also
					// needs to be marked as live (the use has to be dominated,
					// so it has to lie behind the barrier)
					const unsigned useId = instMap[useI];
					const bool isLive = useId > frontierId || useI->getParent() != block;
					if (isLive) {
						liveVals.insert(I);
					}
					DEBUG_PKT( outs() << "  use: (" << useId << ") - is " << (isLive ? "" : "not ") << "live!\n"; );
				}
			}
		}
		// removes all values from 'liveVals' that are live-in of the block of
		// 'inst' but that do not survive 'inst'.
		// TODO: implement more cases than just phis
		void removeBlockInternalNonLiveInValues(Instruction* inst, LiveSetType& liveInVals, LiveSetType& liveOutVals) {
			assert (inst);
			if (liveInVals.empty()) return;

			BasicBlock* block = inst->getParent();

			// remove all values that go into a phi (they die immediately)
			for (BasicBlock::iterator I=block->begin(), E=block->end(); I!=E; ++I) {
				if (block->getFirstNonPHI() == I) break;
				PHINode* phi = cast<PHINode>(I);

				for (unsigned i=0, e=phi->getNumIncomingValues(); i<e; ++i) {
					Value* val = phi->getIncomingValue(i);
					// no need to check if this value really exists in set
					liveInVals.erase(val);
				}
			}

			// remove all values that are live-in but not live-out and do not
			// have uses behind the frontier (they die before the frontier)

			// create map which holds order of instructions
			std::map<Instruction*, unsigned> instMap;
			const unsigned frontierId = createInstructionOrdering(block, inst, instMap);

			for (LiveSetType::iterator it=liveInVals.begin(), E=liveInVals.end(); it!=E; ++it) {
				Value* liveVal = *it;
				// If the value is also live-out, we know it survives the frontier
				// and we must not remove it from live values.
				if (liveOutVals.find(liveVal) != liveOutVals.end()) continue;

				bool survivesFrontier = false;
				for (Value::use_iterator U=liveVal->use_begin(), UE=liveVal->use_end(); U!=UE && !survivesFrontier; ++U) {
					assert (isa<Instruction>(U));
					Instruction* useI = cast<Instruction>(U);
					// ignore uses in different (preceding) blocks
					// if there was a use in a successing block, we would not
					// iterate over the uses in the first place (see continue above)
					if (useI->getParent() != block) continue;

					const unsigned useId = instMap[useI];
					if (useId >= frontierId) survivesFrontier = true;
				}

				if (survivesFrontier) {
					liveInVals.erase(liveVal);
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

		void computeLiveValues(Function* f) {
			assert (f && loopInfo);

			// compute liveness of arguments
			BasicBlock* entryBB = &f->getEntryBlock();
			for (Function::arg_iterator A=f->arg_begin(), AE=f->arg_end(); A!=AE; ++A) {
				outs() << "computing live values for argument: " << *A << "\n";
				for (Instruction::use_iterator U=A->use_begin(), UE=A->use_end(); U!=UE; ++U) {
					assert (isa<Instruction>(U));
					Instruction* useI = cast<Instruction>(U);

					std::set<BasicBlock*> visitedBlocks;
					const bool defUseChainExists = computeLivenessInformation(entryBB, A, useI, visitedBlocks);
					assert (defUseChainExists);
				}
			}

			// compute liveness of all instructions
			for (Function::iterator BB=f->begin(), BBE=f->end(); BB!=BBE; ++BB) {
				outs() << "computing live values for block: " << BB->getNameStr() << "\n";
				for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
					outs() << "  computing live values for instruction: " << *I << "\n";
					for (Instruction::use_iterator U=I->use_begin(), UE=I->use_end(); U!=UE; ++U) {
						assert (isa<Instruction>(U));
						Instruction* useI = cast<Instruction>(U);

						if (useI->getParent() == BB) continue; // def-use chain inside same block -> ignore
						outs() << "    checking use: " << *useI << "\n";

						std::set<BasicBlock*> visitedBlocks;
						const bool defUseChainExists = computeLivenessInformation(BB, I, useI, visitedBlocks);
						assert (defUseChainExists);
					}
				}
			}

		}

		// TODO: use some kind of marking of blocks to prevent traversing a path more than once
		// TODO: check if it is really correct to return true if we see the same block again!
		bool computeLivenessInformation(BasicBlock* block, Value* def, Instruction* use, std::set<BasicBlock*>& visitedBlocks) {
			//outs() << "computeLivenessInformation(" << block->getNameStr() << ")\n";
			if (visitedBlocks.find(block) != visitedBlocks.end()) return true;
			visitedBlocks.insert(block);

			if (use->getParent() == block) {
				outs() << "      is live-in value of block '" << block->getNameStr() << "'\n";
				outs() << "         is parent of use!\n";
				// add def as live-in for this block and return 'found'
				liveValueMap[block].first->insert(def);
				return true;
			}

			bool defUseChainExists = false;

			// recurse into successors
			for (succ_iterator S=succ_begin(block), E=succ_end(block); S!=E; ++S) {
				BasicBlock* succBB = *S;
				defUseChainExists |= computeLivenessInformation(succBB, def, use, visitedBlocks);
			}

			if (!defUseChainExists) outs() << "      no def-use chain found in successors of block " << block->getNameStr() << "!\n";
			if (!defUseChainExists) return false;


			// add def as live-in for this block (if block is not the parent of def)
			if (!isa<Instruction>(def) || (cast<Instruction>(def)->getParent() != block)) {
				liveValueMap[block].first->insert(def);
				outs() << "      is live-in value of block '" << block->getNameStr() << "'\n";
			}

			// add def as live-out for this block
			liveValueMap[block].second->insert(def);

			outs() << "      is live-out value of block '" << block->getNameStr() << "'\n";

			return true;
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

