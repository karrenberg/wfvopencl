/**
 * @file   callSiteBlockSplitter.h
 * @date   10.09.2010
 * @author Ralf Karrenberg
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 *
 * TODO: void* as last param is hardcoded
 * TODO: replacing all returns by return -1 is hardcoded
 */
#ifndef _CALLSITEBLOCKSPLITTER_H
#define	_CALLSITEBLOCKSPLITTER_H


#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif
#define DEBUG_TYPE "callsiteblocksplitter"

#include <llvm/Support/raw_ostream.h>

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include "llvm/ADT/SetVector.h"

#include "llvmTools.hpp"

#ifdef DEBUG
#define DEBUG_CSBS(x) do { x } while (false)
#else
#define DEBUG_CSBS(x) ((void)0)
#endif

#ifdef NDEBUG // force debug output disabled
#undef DEBUG_CSBS
#define DEBUG_CSBS(x) ((void)0)
#endif

using namespace llvm;


// forward declaration of initializer
namespace llvm {
	void initializeCallSiteBlockSplitterPass(PassRegistry&);
}


namespace {

class CallSiteBlockSplitter : public FunctionPass {
public:
	static char ID; // Pass identification, replacement for typeid
	CallSiteBlockSplitter() : FunctionPass(ID), verbose(true), calleeName("barrier"), numCalls(0) { errs() << "empty constructor CallSiteBlockSplitter() should never be called!\n"; }
	explicit CallSiteBlockSplitter(const std::string& fnName, const bool verbose_flag = false) : FunctionPass(ID), verbose(verbose_flag), calleeName(fnName), numCalls(0) {}
	~CallSiteBlockSplitter() { releaseMemory(); }

	virtual bool runOnFunction(Function &f) {
		DEBUG_CSBS( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
		DEBUG_CSBS( outs() << "splitting basic blocks at callsites of function " << calleeName << "...\n"; );
		DEBUG_CSBS( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"; );

		DEBUG_CSBS( WFVOpenCL::writeFunctionToFile(&f, "debug_splitting_before.ll"); );
		splitBlocksAtCallSites(&f);
		DEBUG_CSBS( WFVOpenCL::writeFunctionToFile(&f, "debug_splitting_after.ll"); );

		DEBUG_CSBS( verifyModule(*f.getParent()); );

		DEBUG_CSBS( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
		DEBUG_CSBS( outs() << "splitting of basic blocks at callsites of function " << calleeName << " finished!\n"; );
		DEBUG_CSBS( print(outs(), NULL); );
		DEBUG_CSBS( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"; );

		return numCalls != 0;
	}

	void print(raw_ostream& o, const Module *M) const {}
	virtual void getAnalysisUsage(AnalysisUsage &AU) const {}
	void releaseMemory() {}

	typedef DenseMap<CallInst*, unsigned> CallIndicesMapType;
	inline const CallIndicesMapType getCallIndexMap() const { return callIndices; }
	inline const unsigned getNumCalls() const { return numCalls; }

private:
	const bool verbose;
	const std::string calleeName;
	unsigned numCalls;
	CallIndicesMapType callIndices;

	inline bool isSplitCall(const Value* value) const {
		if (!isa<CallInst>(value)) return false;
		const CallInst* call = cast<CallInst>(value);
		const Function* callee = call->getCalledFunction();
		return callee->getName().equals(calleeName);
	}

	// - split basic blocks at each valid splitting instruction
	void splitBlocksAtCallSites(Function* f) {
		// split basic blocks and collect barriers
		SetVector<CallInst*> barriers;
		bool changed = true;
		while (changed) {
			changed = false;
			for (Function::iterator BB=f->begin(), BBE=f->end(); BB!=BBE; ++BB) {
				// we do a forward iteration over instructions, so we know that
				// if the last instruction in this block is a barrier, we can
				// skip the block (do not split again).
				if (BB->getInstList().size() < 2) continue;
				assert (BB->getTerminator());
				Instruction* lastInst = --(--BB->end()); // -1 = terminator, -2 = last inst before terminator
				assert (lastInst);

				if (isa<CallInst>(lastInst) && barriers.count(cast<CallInst>(lastInst))) continue;

				// otherwise, see if we have a barrier in this block
				for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
					if (!isSplitCall(I)) continue;

					CallInst* barrier = cast<CallInst>(I);

					assert (!barriers.count(barrier) && "must not attempt to split twice at the same barrier!");

					barriers.insert(barrier);
					callIndices[barrier] = numCalls++;

					// split block at the position of the barrier
					// (barrier has to be in "upper block")

					// find instruction that precedes barrier (= split point)
					Instruction* splitInst = NULL;
					BasicBlock::iterator I2 = --(BB->end());
					while (barrier != I2) {
						splitInst = I2--;
					}
					// 'newBlock' is the "lower" block, 'parentBlock' the "upper" one with the barrier
					BB->splitBasicBlock(splitInst, BB->getNameStr()+".postbarrier");

					changed = true;
					break;
				}
				if (changed) break;
			}
		}

		return;
	}
};

} // namespace

char CallSiteBlockSplitter::ID = 0;
INITIALIZE_PASS_BEGIN(CallSiteBlockSplitter, "callsite-block-splitting", "CallSite Block Splitting", false, false)
//INITIALIZE_PASS_DEPENDENCY() // has no dependency :p
INITIALIZE_PASS_END(CallSiteBlockSplitter, "callsite-block-splitting", "CallSite Block Splitting", false, false)

// Public interface to the CallSiteBlockSplitter pass
namespace llvm {
	FunctionPass* createCallSiteBlockSplitterPass(const std::string& fnName) {
		return new CallSiteBlockSplitter(fnName);
	}
}


#endif	/* _CALLSITEBLOCKSPLITTER_H */

