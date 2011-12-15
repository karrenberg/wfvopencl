/**
 * @file   livenessAnalyzer.h
 * @date   28.06.2010
 * @author Ralf Karrenberg
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 *
 */
#ifndef _LIVENESSANALYZER_H
#define	_LIVENESSANALYZER_H


#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif
#define DEBUG_TYPE "livenessanalyzer"

#include <llvm/ADT/ValueMap.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm/Analysis/LoopInfo.h>

#include "../debug.h"

using namespace llvm;

// forward declaration of initializer
namespace llvm {
	void initializeLivenessAnalyzerPass(PassRegistry&);
}

class LivenessAnalyzer : public FunctionPass {
public:
	static char ID; // Pass identification, replacement for typeid
	LivenessAnalyzer(const bool verbose_flag = false);
	~LivenessAnalyzer();

	virtual bool runOnFunction(Function &f);
	void print(raw_ostream& o, const Module *M=NULL) const;
	virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	void releaseMemory();

	typedef std::set<Value*> LiveSetType;
	typedef std::pair< LiveSetType*, LiveSetType* > LiveValueSetType;
	typedef std::map< const BasicBlock*, LiveValueSetType > LiveValueMapType;

	inline LiveValueSetType* getBlockLiveValues(BasicBlock* block) {
		assert (block);
		LiveValueMapType::iterator it = liveValueMap.find(block);
		if (it == liveValueMap.end()) return NULL;

		return &(it->second);
	}

	inline LiveSetType* getBlockLiveInValues(const BasicBlock* block) {
		assert (block);
		LiveValueMapType::iterator it = liveValueMap.find(block);
		if (it == liveValueMap.end()) return NULL;

		return it->second.first;
	}

	inline LiveSetType* getBlockLiveOutValues(const BasicBlock* block) {
		assert (block);
		LiveValueMapType::iterator it = liveValueMap.find(block);
		if (it == liveValueMap.end()) return NULL;

		return it->second.second;
	}

	// create map which holds order of instructions
	unsigned createInstructionOrdering(const BasicBlock* block, const Instruction* frontier, std::map<const Instruction*, unsigned>& instMap) const;
	void getBlockInternalLiveInValues(Instruction* inst, LiveSetType& liveVals);

	// removes all values from 'liveVals' that are live-in of the block of
	// 'inst' but that do not survive 'inst'.
	// TODO: implement more cases than just phis
	// NOTE: must not use loop info here, this function is meant to work on any
	//       code unrelated to this analysis.
	void removeBlockInternalNonLiveInValues(Instruction* frontier, LiveSetType& liveInVals, const LiveSetType& liveOutVals);

	void mapLiveValues(Function* f, Function* newF, ValueMap<const Value*, Value*>& valueMap);

private:
	const bool verbose;
	LoopInfo* loopInfo;

	LiveValueMapType liveValueMap;

	void computeLiveValues(Function* f);
	void computeLivenessInformation(BasicBlock* block, Value* def, Instruction* use, std::set<BasicBlock*>& visitedBlocks);
};

/*
INITIALIZE_PASS_BEGIN(LivenessAnalyzer, "liveness-analysis", "Liveness Analysis", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfo)
INITIALIZE_PASS_END(LivenessAnalyzer, "liveness-analysis", "Liveness Analysis", false, false)
*/

// Public interface to the LivenessAnalysis pass
namespace llvm {
	FunctionPass* createLivenessAnalyzerPass();
}


#endif	/* _LIVENESSANALYZER_H */

