/**
 * @file   continuationGenerator.h
 * @date   02.07.2010
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
#ifndef _CONTINUATIONGENERATOR_H
#define	_CONTINUATIONGENERATOR_H


#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif
#define DEBUG_TYPE "continuationgenerator"

#include <llvm/Support/raw_ostream.h>

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>

#include "livenessAnalyzer.h"
#include "llvmTools.hpp"
#include <llvm/Analysis/Dominators.h>

#define DEBUG_PKT(x) do { x } while (false)
//#define DEBUG_PKT(x) ((void)0)

#define PACKETIZED_OPENCL_DRIVER_FUNCTION_NAME_BARRIER "barrier"
#define PACKETIZED_OPENCL_DRIVER_BARRIER_SPECIAL_END_ID -1
#define PACKETIZED_OPENCL_DRIVER_BARRIER_SPECIAL_START_ID 0

#define USE_MANUAL_DOMTREE_PASS

using namespace llvm;

namespace {

class VISIBILITY_HIDDEN ContinuationGenerator : public FunctionPass {
public:
	static char ID; // Pass identification, replacement for typeid
	ContinuationGenerator(const bool verbose_flag = false) : FunctionPass(&ID), verbose(verbose_flag) {}
	~ContinuationGenerator() { releaseMemory(); }

	virtual bool runOnFunction(Function &f) {

		// get loop info
		loopInfo = &getAnalysis<LoopInfo>();

		// get liveness information
		livenessAnalyzer = &getAnalysis<LivenessAnalyzer>();

		DEBUG_PKT( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
		DEBUG_PKT( outs() << "generating continuations...\n"; );
		DEBUG_PKT( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );

		PacketizedOpenCLDriver::writeFunctionToFile(&f, "beforeBarriers.ll");
		//f.viewCFG();

		assert (f.getParent() && "function has to have a valid parent module!");
		TargetData* targetData = new TargetData(f.getParent());

		barrierFreeFunction = eliminateBarriers(&f, targetData);

		if (!barrierFreeFunction) return false; // if barrierFreeFunction does not exist, nothing has changed

		DEBUG_PKT( verifyModule(*f.getParent()); );

		//--------------------------------------------------------------------//
		// inline continuation functions & optimize wrapper
		//--------------------------------------------------------------------//
		PacketizedOpenCLDriver::inlineFunctionCalls(barrierFreeFunction, targetData);
		//DEBUG_PKT( outs() << *barrierFreeFunction << "\n"; );
		PacketizedOpenCLDriver::optimizeFunction(barrierFreeFunction);

		DEBUG_PKT( verifyFunction(*barrierFreeFunction); );
		DEBUG_PKT( outs() << *barrierFreeFunction << "\n"; );
		isa<UndefValue>(&f);

		f.replaceAllUsesWith(barrierFreeFunction);
		barrierFreeFunction->takeName(&f);

		DEBUG_PKT( outs() << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"; );
		DEBUG_PKT( outs() << "generation of continuations finished!\n"; );
		DEBUG_PKT( print(outs(), NULL); );
		DEBUG_PKT( outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"; );

		return true;
	}

	void print(raw_ostream& o, const Module *M) const {}
	virtual void getAnalysisUsage(AnalysisUsage &AU) const {
		AU.addRequired<LoopInfo>();
		AU.addRequired<LivenessAnalyzer>();
	}
	void releaseMemory() {}

	Function* getBarrierFreeFunction() const { return barrierFreeFunction; }

private:
	const bool verbose;
	LoopInfo* loopInfo;
	LivenessAnalyzer* livenessAnalyzer;
	Function* barrierFreeFunction;

	struct BarrierInfo {
		BarrierInfo(CallInst* call, BasicBlock* parentBB, unsigned d)
			: id(0), barrier(call), parentBlock(parentBB), depth(d), continuation(NULL), liveValueStructType(NULL) {}
		unsigned id;
		CallInst* barrier;
		BasicBlock* parentBlock; // parent block of original function (might have been split due to other barriers)
		unsigned depth;
		Function* continuation;
		StructType* liveValueStructType;
	};
	typedef DenseMap<unsigned, SmallVector<BarrierInfo*, 4>* > BarrierMapType;


	unsigned collectBarriersDFS(BasicBlock* block, unsigned depth, BarrierMapType& barriers, unsigned& maxBarrierDepth, std::set<BasicBlock*>& visitedBlocks) {
		assert (block);
		if (visitedBlocks.find(block) != visitedBlocks.end()) return 0;
		visitedBlocks.insert(block);

		unsigned numBarriers = 0;

		
		for (BasicBlock::iterator I=block->begin(), IE=block->end(); I!=IE; ++I) {
			if (!isa<CallInst>(I)) continue;
			CallInst* call = cast<CallInst>(I);

			const Function* callee = call->getCalledFunction();
			if (!callee->getName().equals(PACKETIZED_OPENCL_DRIVER_FUNCTION_NAME_BARRIER)) continue;

			++numBarriers;

			BarrierInfo* bi = new BarrierInfo(call, block, depth);

			// we fetch the bucket for this depth at each barrier to prevent
			// generating a bucket if no barrier exists
			if (barriers.find(depth) == barriers.end()) {
				// no bucket for this depth exists yet -> generate and store
				SmallVector<BarrierInfo*, 4>* depthVector = new SmallVector<BarrierInfo*, 4>();
				barriers[depth] = depthVector;
			}

			// fetch bucket for this depth and append barrier info
			barriers[depth]->push_back(bi);

			if (depth > maxBarrierDepth) maxBarrierDepth = depth;
		}

		for (succ_iterator S=succ_begin(block), E=succ_end(block); S!=E; ++S) {
			BasicBlock* succBB = *S;

			numBarriers += collectBarriersDFS(succBB, depth+1, barriers, maxBarrierDepth, visitedBlocks);
		}

		return numBarriers;
	}
	void findContinuationBlocksDFS(const BasicBlock* block, std::set<const BasicBlock*>& copyBlocks, std::set<const BasicBlock*>& visitedBlocks) {
		assert (block);
		if (visitedBlocks.find(block) != visitedBlocks.end()) return;
		visitedBlocks.insert(block);

		copyBlocks.insert(block);

		for (succ_const_iterator S=succ_begin(block), E=succ_end(block); S!=E; ++S) {
			const BasicBlock* succBB = *S;
			findContinuationBlocksDFS(succBB, copyBlocks, visitedBlocks);
		}
	}


#ifndef USE_MANUAL_DOMTREE_PASS
	// We need these two functions because requiring domTree is not enoug
	// -> it only generates dominator information for original function, not for
	//    new continuations.

	//returns true if blockA 'follows' blockB or blockA == blockB
	//REQUIRES LoopInfo
	bool isDominatedBy(BasicBlock* blockA, BasicBlock* blockB, std::set<BasicBlock*>& visitedBlocks) {
		if (blockA == blockB) return true;
		if (visitedBlocks.find(blockB) != visitedBlocks.end()) return false;
		visitedBlocks.insert(blockB);

		bool isDominated = false;
		for (succ_iterator it=succ_begin(blockB); it!=succ_end(blockB); ++it) {
			//if (Loop* L = loopInfo->getLoopFor(*it)) {
				//if (L->getHeader() == *it) continue;
			//}
			if (blockA == *it) return true;
			isDominated |= isDominatedBy(blockA, *it, visitedBlocks);
		}
		return isDominated;
	}
	//returns true if instA 'follows' instB or instA == instB
	inline bool isDominatedBy(Instruction* instA, Instruction* instB) {
		if (instA == instB) return true;

		BasicBlock* blockA = instA->getParent();
		if (blockA != instB->getParent()) {
			std::set<BasicBlock*> visitedBlocks;
			return isDominatedBy(blockA, instB->getParent(), visitedBlocks);
		}

		for (BasicBlock::iterator it=blockA->begin(); it!=blockA->end(); ++it) {
			if (instA == it) return false;
			if (instB == it) return true;
		}
		assert (!"CRITICAL ERROR in 'isDominatedBy()'!");
		return false;
	}
#endif

	// generates a continuation function that is called at the point of the barrier
	void createContinuation(BarrierInfo* barrierInfo, const std::string& newFunName, TargetData* targetData) {
		assert (barrierInfo && targetData);
		const unsigned barrierIndex = barrierInfo->id;
		CallInst* barrier = barrierInfo->barrier;
		BasicBlock* parentBlock = barrierInfo->parentBlock;
		assert (barrier && parentBlock);
		assert (barrier->getParent());
		Function* f = barrier->getParent()->getParent();
		assert (f);
		Module* mod = f->getParent();
		assert (mod);
		assert (barrier->getParent() == parentBlock);

		LLVMContext& context = mod->getContext();

		DEBUG_PKT( outs() << "\ngenerating continuation for barrier " << barrierIndex << " in block '" << parentBlock->getNameStr() << "'\n"; );

		// before splitting, get block-internal live values
		// (values that are defined inside block and live over the barrier)
		LivenessAnalyzer::LiveSetType internalLiveValues;
		livenessAnalyzer->getBlockInternalLiveInValues(barrier, internalLiveValues);

		//--------------------------------------------------------------------//
		// split block at the position of the barrier
		// (barrier has to be in "upper block")
		//--------------------------------------------------------------------//
		// find instruction that precedes barrier (= split point)
		Instruction* splitInst = NULL;
		BasicBlock::iterator I = --(parentBlock->end());
		while (barrier != I) {
			splitInst = I--;
		}
		// 'newBlock' is the first of the new continuation, 'parentBlock' the one with the barrier
		BasicBlock* newBlock = parentBlock->splitBasicBlock(splitInst, parentBlock->getNameStr()+".postbarrier");

		//--------------------------------------------------------------------//
		// get live values for this block
		// NOTE: This only fetches live values of former parent block
		//       in order to prevent recalculating live value information for
		//       entire function.
		//--------------------------------------------------------------------//
		LivenessAnalyzer::LiveSetType* liveInValues = livenessAnalyzer->getBlockLiveInValues(parentBlock);
		LivenessAnalyzer::LiveSetType* liveOutValues = livenessAnalyzer->getBlockLiveOutValues(parentBlock);
		assert (liveInValues);
		assert (liveOutValues);
		
		// merge block-internal live values with liveInValues
		liveInValues->insert(internalLiveValues.begin(), internalLiveValues.end());
		// remove all values that die in same block but above barrier
		livenessAnalyzer->removeBlockInternalNonLiveInValues(barrier, *liveInValues, *liveOutValues);


		DEBUG_PKT(
			outs() << "\n\nLive-In values of block '" << parentBlock->getNameStr() << "':\n";
			for (std::set<Value*>::iterator it=liveInValues->begin(), E=liveInValues->end(); it!=E; ++it) {
				outs() << " * " << **it << "\n";
			}
			outs() << "\n";
		);


		//--------------------------------------------------------------------//
		// create struct with live-in values of newBlock
		//--------------------------------------------------------------------//
		std::vector<const Type*> params;
		for (LivenessAnalyzer::LiveSetType::iterator it=liveInValues->begin(), E=liveInValues->end(); it!=E; ++it) {
			params.push_back((*it)->getType());
		}
		StructType* sType = StructType::get(context, params, false);
		DEBUG_PKT( outs() << "new struct type: " << *sType << "\n"; );
		DEBUG_PKT( outs() << "type size in bits : " << targetData->getTypeSizeInBits(sType) << "\n"; );
		DEBUG_PKT( outs() << "alloc size in bits: " << targetData->getTypeAllocSizeInBits(sType) << "\n"; );
		DEBUG_PKT( outs() << "alloc size        : " << targetData->getTypeAllocSize(sType) << "\n"; );

		// pointer to union for live value struct for next call is the last parameter
		Argument* newDataPtr = --(f->arg_end());
		DEBUG_PKT( outs() << "pointer to union: " << *newDataPtr << "\n"; );

		// bitcast data pointer to correct struct type for GEP below // TODO: HERE!!!
		BitCastInst* bc = new BitCastInst(newDataPtr, PointerType::getUnqual(sType), "", barrier);

		// store values
		unsigned i=0;
		for (LivenessAnalyzer::LiveSetType::iterator it=liveInValues->begin(), E=liveInValues->end(); it!=E; ++it) {
			std::vector<Value*> indices;
			indices.push_back(ConstantInt::getNullValue(Type::getInt32Ty(context)));
			indices.push_back(ConstantInt::get(context, APInt(32, i++)));
			GetElementPtrInst* gep = GetElementPtrInst::Create(bc, indices.begin(), indices.end(), "", barrier);
			DEBUG_PKT( outs() << "store gep(" << i-1 << "): " << *gep << "\n"; );
			const unsigned align = 16;
			new StoreInst(*it, gep, false, align, barrier);
		}


		//--------------------------------------------------------------------//
		// delete the edge from parentBlock to newBlock
		//--------------------------------------------------------------------//
		assert (parentBlock->getTerminator()->use_empty());
		parentBlock->getTerminator()->eraseFromParent();

		//--------------------------------------------------------------------//
		// create return that returns the id for the next call
		//--------------------------------------------------------------------//
		const Type* returnType = Type::getInt32Ty(context);
		ReturnInst::Create(context, ConstantInt::get(returnType, barrierIndex, true), barrier);

		//--------------------------------------------------------------------//
		// (dead code elimination should remove newBlock and all blocks below
		// that are dead.)
		//--------------------------------------------------------------------//
		// nothing to do here

		//--------------------------------------------------------------------//
		// erase barrier
		//--------------------------------------------------------------------//
		//if (!barrier->use_empty()) barrier->replaceAllUsesWith(Constant::getNullValue(barrier->getType()));
		assert (barrier->use_empty() && "barriers must not have any uses!");
		barrier->eraseFromParent();



		//--------------------------------------------------------------------//
		// create new function with the following signature:
		// - returns int (id of next continuation)
		// - one parameter per live-in value
		// - last parameter: void* data (union where live values for next
		//                   continuation are stored before returning)
		//--------------------------------------------------------------------//
		params.clear();
		for (StructType::element_iterator T=sType->element_begin(), TE=sType->element_end(); T!=TE; ++T) {
			params.push_back(*T);
		}
		params.push_back(Type::getInt8PtrTy(context));

		FunctionType* fType = FunctionType::get(returnType, params, false);

		Function* continuation = Function::Create(fType, Function::ExternalLinkage, newFunName, mod); // TODO: check linkage type
		DEBUG_PKT( outs() << "\nnew continuation declaration: " << *continuation << "\n"; );

		//--------------------------------------------------------------------//
		// create mappings of values
		//--------------------------------------------------------------------//
		// create mappings of live-in values to arguments for copying of blocks
		DEBUG_PKT( outs() << "\nlive value mappings:\n"; );
		DenseMap<const Value*, Value*> valueMap;
		DenseMap<const Value*, Value*> liveValueToArgMap;
		Function::arg_iterator A = continuation->arg_begin();
		for (LivenessAnalyzer::LiveSetType::iterator it=liveInValues->begin(), E=liveInValues->end(); it!=E; ++it, ++A) {
			Value* liveVal = *it;
			valueMap[liveVal] = A;
			liveValueToArgMap[liveVal] = A;
			DEBUG_PKT( outs() << " * " << *liveVal << " -> " << *A << "\n"; );
		}


		//--------------------------------------------------------------------//
		// copy all blocks 'below' parentBlock inside the new function (DFS)
		// and map all uses of live values to the loads generated in the last step
		//--------------------------------------------------------------------//
		std::set<const BasicBlock*> copyBlocks;
		std::set<const BasicBlock*> visitedBlocks;
		findContinuationBlocksDFS(newBlock, copyBlocks, visitedBlocks);

		DEBUG_PKT(
			outs() << "\ncloning blocks into continuation...\n";
			for (std::set<const BasicBlock*>::iterator it=copyBlocks.begin(), E=copyBlocks.end(); it!=E; ++it) {
				outs() << " * " << (*it)->getNameStr() << "\n";
			}
			outs() << "\n";
		);

		// HACK: Copy over entire function and remove all unnecessary blocks.
		//       This is required because CloneBasicBlock does not perform any
		//       remapping. RemapInstructions thus has to be called by hand and
		//       is not available via includes (as of llvm-2.8svn ~May/June 2010 :p).

		// Therefore, we need to have dummy-mappings for all arguments of the
		// old function that do not map to a live value.
		// TODO: erase dummy values again from value map at some point?
		for (Function::arg_iterator A=f->arg_begin(), AE=f->arg_end(); A!=AE; ++A) {
			//if (liveValueMap.find(A) != liveValueMap.end()) {
				//valueMap[A] = liveValueMap[A];
				//continue;
			//}
			if (valueMap.find(A) != valueMap.end()) continue;
			Value* dummy = UndefValue::get(A->getType());
			valueMap[A] = dummy;
		}

		// create mapping for data pointer (last argument to last argument
		valueMap[(--(f->arg_end()))] = --(continuation->arg_end());

		SmallVector<ReturnInst*, 2> returns;
		CloneFunctionInto(continuation, f, valueMap, returns, ".");


		PacketizedOpenCLDriver::writeFunctionToFile(continuation, "ASDF1.ll");


		// remap values that were no arguments
		// NOTE: apparently, CloneFunctionInto does not look into the valueMap
		//       if it directly finds all references. Due to the fact that we
		//       copy the entire function, live values that are no arguments
		//       are still resolved by their former definitions which only get
		//       erased in the next step.
		// NOTE: actually, this is good behaviour - we must not remap values
		//       that are defined in a block that also appears in the
		//       continuation, e.g. in loops.
		// TODO: Are we sure that the ordering is always correct with LiveSetType
		//       being a set? :p

		// erase all blocks that do not belong to this continuation
		BasicBlock* dummyBB = BasicBlock::Create(context, "dummy", continuation);
		// iterate over blocks of original fun, but work on blocks of continuation fun
		// -> can't find out block in old fun for given block in new fun via map ;)
		for (Function::iterator BB=f->begin(), BBE=f->end(); BB!=BBE; ) {
			assert (valueMap.find(BB) != valueMap.end());
			BasicBlock* blockOrig = BB++;
			BasicBlock* blockCopy = cast<BasicBlock>(valueMap[blockOrig]);
			if (copyBlocks.find(blockOrig) != copyBlocks.end()) continue;

			// block must not be "copied" -> delete it

			// but first, replace all uses of instructions of block by dummies
			// or arguments in case of live values...
			BasicBlock::iterator IO=blockOrig->begin(); // mapping uses values from old function...
			for (BasicBlock::iterator I=blockCopy->begin(), IE=blockCopy->end(); I!=IE; ++I, ++IO) {
				outs() << "replacing uses of inst: " << *I << "\n";
				if (liveInValues->find(IO) != liveInValues->end()) {
					outs() << "  is a live value, replaced with argument: " << *liveValueToArgMap[IO] << "\n";
					I->replaceAllUsesWith(liveValueToArgMap[IO]);
				} else {
					I->replaceAllUsesWith(UndefValue::get(I->getType()));
				}

				// erase instruction from value map (pointer will be invalid after deletion)
				valueMap.erase(IO);
			}

			// erase block from value map (pointer will be invalid after deletion)
			valueMap.erase(blockOrig);

			// remove all incoming values from this block to phis
			outs() << "block: " << blockCopy->getNameStr() << "\n";
			for (BasicBlock::use_iterator U=blockCopy->use_begin(), UE=blockCopy->use_end(); U!=UE; ++U) {
				if (!isa<PHINode>(U)) continue;
				PHINode* phi = cast<PHINode>(U);
				outs() << "phi: " << *phi << "\n";
				if (phi->getBasicBlockIndex(blockCopy) != -1) phi->removeIncomingValue(blockCopy, false);
			}

			blockCopy->replaceAllUsesWith(dummyBB);
			blockCopy->eraseFromParent();
		}

		outs() << "\n";

		// erase dummy block
		assert (dummyBB->use_empty());
		dummyBB->eraseFromParent();

		// if necessary, make new block the entry block of the continuation
		// (= move to front of block list)
		BasicBlock* entryBlock = cast<BasicBlock>(valueMap[newBlock]);
		if (entryBlock != &continuation->getEntryBlock()) {
			entryBlock->moveBefore(&continuation->getEntryBlock());
		}

		PacketizedOpenCLDriver::writeFunctionToFile(continuation, "ASDF2.ll");


		// Make sure all uses of live values that are not dominated anymore are
		// rewired to arguments.
		// NOTE: This can't be done before blocks are deleted ;).

#ifdef USE_MANUAL_DOMTREE_PASS
		// compute dominator tree (somehow does not work with functionpassmanager)
		DominatorTree* domTree = new DominatorTree();
		domTree->runOnFunction(*continuation);
		DEBUG_PKT( domTree->print(outs(), mod); );
#endif

		Function::arg_iterator A2 = continuation->arg_begin();
		for (LivenessAnalyzer::LiveSetType::iterator it=liveInValues->begin(), E=liveInValues->end(); it!=E; ++it, ++A2) {
			Value* liveVal = *it;
			outs() << "live value: " << *liveVal << "\n";
			if (isa<Argument>(liveVal)) continue;

			// if all uses already replaced above, skip this value
			if (valueMap.find(liveVal) == valueMap.end()) {
				outs() << "  all uses already replaced!\n";
				continue;
			}

			Value* newLiveVal = valueMap[liveVal];
			outs() << "new live value: " << *newLiveVal << "\n";

			// if the value is defined in one of the copied blocks, we must only
			// replace those uses that are not dominated by their definition anymore
			if (Instruction* inst = dyn_cast<Instruction>(liveVal)) {
				if (copyBlocks.find(inst->getParent()) != copyBlocks.end()) {
					Instruction* newInst = cast<Instruction>(newLiveVal);
					DEBUG_PKT( outs() << "live value is defined in copied block: " << (*copyBlocks.find(inst->getParent()))->getNameStr() << "\n"; );
					for (Instruction::use_iterator U=newInst->use_begin(), UE=newInst->use_end(); U!=UE; ) {
						assert (isa<Instruction>(U));
						Instruction* useI = cast<Instruction>(U++);
						DEBUG_PKT( outs() << "  testing use: " << *useI << "\n"; );
#ifdef USE_MANUAL_DOMTREE_PASS
						if (domTree->dominates(newInst, useI)) continue;
#else
						if (isDominatedBy(useI, newInst)) continue;
#endif
						DEBUG_PKT( outs() << "    is not dominated, will be replaced by argument!\n"; );
						useI->replaceUsesOfWith(newInst, A2);
					}
					continue;
				}
			}

			newLiveVal->replaceAllUsesWith(A2);
		}

		PacketizedOpenCLDriver::writeFunctionToFile(continuation, "ASDF3.ll");
		//continuation->viewCFG();


		// TODO: erase dummy values from value map?

		DEBUG_PKT( outs() << *continuation << "\n"; );
		DEBUG_PKT( outs() << "continuation '" << continuation->getNameStr() << "' generated successfully!\n\n"; );

		DEBUG_PKT(
			PacketizedOpenCLDriver::writeModuleToFile(mod, "ffff.ll");
			//f->viewCFG();
			//continuation->viewCFG();
			outs() << "verifying function '" << f->getNameStr() << "'... ";
			verifyFunction(*f);
			outs() << "done.\nverifying function '" << continuation->getNameStr() << "'... ";
			verifyFunction(*continuation);
			outs() << "done.\n";
		);

		barrierInfo->continuation = continuation;
		barrierInfo->liveValueStructType = sType;
	}

	Function* eliminateBarriers(Function* f, TargetData* targetData) {
		assert (f && targetData);
		assert (f->getReturnType()->isVoidTy());
		Module* mod = f->getParent();
		assert (mod);
		LLVMContext& context = mod->getContext();

		const std::string& functionName = f->getNameStr();
		DEBUG_PKT( outs() << "\neliminateBarriers(" << functionName << ")\n"; );

		//--------------------------------------------------------------------//
		// change return value of f to return unsigned (barrier id)
		// and add one new parameter to the end of the argument list:
		// - void* newData : pointer to live value union where live-in values of
		//                   next continuation are stored
		//
		// = create new function with new signature and clone all blocks
		// The former return statements now all return -1 (special end id)
		//--------------------------------------------------------------------//
		const FunctionType* fTypeOld = f->getFunctionType();
		std::vector<const Type*> params;
		for (FunctionType::param_iterator it=fTypeOld->param_begin(), E=fTypeOld->param_end(); it!=E; ++it) {
			params.push_back(*it);
		}
		params.push_back(Type::getInt8PtrTy(context)); // add void*-argument (= live value struct return param)
		const FunctionType* fTypeNew = FunctionType::get(Type::getInt32Ty(context), params, false);
		Function* newF = Function::Create(fTypeNew, Function::ExternalLinkage, functionName+"_begin", mod); // TODO: check linkage type
		(--newF->arg_end())->setName("newData");

		// specify mapping of parameters
		DenseMap<const Value*, Value*> valueMap;
		Function::arg_iterator A2 = newF->arg_begin();
		for (Function::arg_iterator A=f->arg_begin(), AE=f->arg_end(); A!=AE; ++A, ++A2) {
			valueMap[A] = A2; //valueMap.insert(std::make_pair(A, A2));
			A2->takeName(A);
		}
		SmallVector<ReturnInst*, 2> returns;

		CloneAndPruneFunctionInto(newF, f, valueMap, returns, ".");

		for (unsigned i=0; i<returns.size(); ++i) {
			BasicBlock* retBlock = returns[i]->getParent();
			returns[i]->eraseFromParent();
			ReturnInst::Create(context, ConstantInt::get(fTypeNew->getReturnType(), PACKETIZED_OPENCL_DRIVER_BARRIER_SPECIAL_END_ID, true), retBlock);
		}

		// map the live values of the original function to the new one
		livenessAnalyzer->mapLiveValues(f, newF, valueMap);


		//--------------------------------------------------------------------//
		// Traverse the function in DFS and collect all barriers in post-reversed order.
		// Count how many barriers the function has and assign an id to each barrier
		//--------------------------------------------------------------------//
		BarrierMapType barriers; // depth -> [ infos ] mapping
		std::set<BasicBlock*> visitedBlocks;
		unsigned maxBarrierDepth = 0;
		const unsigned numBarriers = collectBarriersDFS(&newF->getEntryBlock(), 0, barriers, maxBarrierDepth, visitedBlocks);

		if (numBarriers == 0) {
			DEBUG_PKT( outs() << "  no barriers found in function!\n"; );
			newF->eraseFromParent();
			return NULL;
		}

		DEBUG_PKT( outs() << "  number of barriers in function : " << numBarriers << "\n"; );
		DEBUG_PKT( outs() << "  maximum block depth of barriers: " << maxBarrierDepth << "\n\n"; );

		//for (BarrierMapType::iterator it=barriers.begin(), E=barriers.end(); it!=E; ++it) {
			//const unsigned depth = it->first;
			//SmallVector<BarrierInfo*, 4>* vec = it->second;
			//outs() << "barriers at depth " << depth << ":\n";
			//for (SmallVector<BarrierInfo*, 4>::iterator it2=vec->begin(), E2=vec->end(); it2!=E2; ++it2) {
				//BarrierInfo* binfo = *it2;
				//outs() << " * " << *binfo->barrier << "\n";
			//}
		//}

		DEBUG_PKT( outs() << "\n" << *newF << "\n"; );

		//--------------------------------------------------------------------//
		// Generate order in which barriers should be replaced:
		// Barriers with highest depth come first, barriers with same depth
		// are ordered nondeterministically unless they live in the same block,
		// in which case their order is determined by their dominance relation.
		//--------------------------------------------------------------------//
		DenseMap<CallInst*, unsigned> barrierIndices;
		SmallVector<BarrierInfo*, 4> orderedBarriers;

		// 0 is reserved for 'start'-function, so the last index is numBarriers and 0 is not used
		unsigned barrierIndex = numBarriers; 
		for (int depth=maxBarrierDepth; depth >= 0; --depth) {
			DEBUG_PKT( outs() << "sorting barriers of block depth " << depth << "...\n"; );
			BarrierMapType::iterator it = barriers.find(depth);
			if (it == barriers.end()) continue; // no barriers at this depth

			SmallVector<BarrierInfo*, 4>& depthVector = *(it->second);

			assert (depthVector.size() > 0);
			assert (depthVector.size() <= numBarriers);

			// if we add barriers in reversed order, barriers that live in the
			// same block are inserted in correct order
			for (int i=depthVector.size()-1; i >= 0; --i) {
				BarrierInfo* binfo = depthVector[i];
				orderedBarriers.push_back(binfo);
				binfo->id = barrierIndex; // set id
				barrierIndices[binfo->barrier] = barrierIndex--; // save barrier -> id mapping
				DEBUG_PKT( outs() << "  added barrier " << i << " with id " << barrierIndex+1 << ": " << *binfo->barrier << "\n"; );
			}
		}


		//--------------------------------------------------------------------//
		// call eliminateBarrier() for each barrier in newFunction
		//--------------------------------------------------------------------//
		const unsigned numContinuationFunctions = numBarriers+1;
		DenseMap<unsigned, BarrierInfo*> continuations;
		continuations[0] = new BarrierInfo(NULL, NULL, 0);
		continuations[0]->continuation = newF;
		continuations[0]->liveValueStructType = StructType::get(context, false);

		// Loop over barriers and generate a continuation for each one.
		// NOTE: newF is modified each time
		//       (blocks split, loading/storing of live value structs, ...)
		for (SmallVector<BarrierInfo*, 4>::iterator it=orderedBarriers.begin(), E=orderedBarriers.end(); it!=E; ++it) {
			BarrierInfo* barrierInfo = *it;
			const unsigned barrierIndex = barrierInfo->id;
			assert (barrierIndex != 0 && "index 0 is reserved for original function, must not appear here!");
			assert (barrierInfo->parentBlock->getParent() == newF);

			std::stringstream sstr;
			sstr << functionName << "_cont_" << barrierIndex;  // "0123456789ABCDEF"[x] would be okay if we could guarantee a max number of continuations :p
			
			createContinuation(barrierInfo, sstr.str(), targetData);

			assert (barrierInfo->continuation);
			assert (barrierInfo->liveValueStructType);
			continuations[barrierIndex] = barrierInfo;
		}

		assert (continuations.size() == numContinuationFunctions);


		//--------------------------------------------------------------------//
		// Check if all barriers in all functions (original and continuations) were eliminated.
		//--------------------------------------------------------------------//
		DEBUG_PKT(
			for (DenseMap<unsigned, BarrierInfo*>::iterator it=continuations.begin(), E=continuations.end(); it!=E; ++it) {
				BarrierInfo* binfo = it->second;
				Function* continuation = binfo->continuation;
				assert (continuation);
				assert (binfo->liveValueStructType);

				for (Function::iterator BB=continuation->begin(), BBE=continuation->end(); BB!=BBE; ++BB) {
					for (BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I) {
						if (!isa<CallInst>(I)) continue;
						CallInst* call = cast<CallInst>(I);

						const Function* callee = call->getCalledFunction();
						if (callee->getName().equals(PACKETIZED_OPENCL_DRIVER_FUNCTION_NAME_BARRIER)) {
							errs() << "ERROR: barrier not eliminated in continuation '" << continuation->getNameStr() << "': " << *call << "\n";
						}
					}
				}
			}
		);




		// TODO: move stuff below into own function



		//--------------------------------------------------------------------//
		// create wrapper function which contains a switch over the barrier id
		// inside a while loop.
		// the wrapper calls the function that corresponds to the barrier id.
		// If the id is the special 'begin' id, it calls the first function
		// (= the remainder of the original kernel).
		// The while loop iterates until the barrier id is set to a special
		// 'end' id.
		// Each function has the same signature receiving only a void*.
		// In case of a continuation, this is a struct which holds the live
		// values that were live at the splitting point.
		// Before returning to the switch, the struct is deleted and the live
		// values for the next call are written into a newly allocated struct
		// (which the void* then points to).
		//--------------------------------------------------------------------//
		// Example:
		/*
		   void* data = NULL;
		   while (true) {
		   switch (current_barrier_id) {
		   case BARRIER_BEGIN: current_barrier_id = runOrigFunc(..., &data); break;
		   case BARRIER_END: return;
		   case B0: {
		   current_barrier_id = runFunc0(&data); break;
		   }
		   case B1: {
		   current_barrier_id = runFunc1(&data); break;
		   }
		   ...
		   case BN: {
		   current_barrier_id = runFuncN(&data); break;
		   }
		   default: error; break;
		   }
		   }
		 */
		Function* wrapper = Function::Create(fTypeOld, Function::ExternalLinkage, functionName+"_barrierswitch", mod); // TODO: check linkage type

		IRBuilder<> builder(context);

		// create entry block
		BasicBlock* entryBB = BasicBlock::Create(context, "entry", wrapper);

		// create blocks for while loop
		BasicBlock* headerBB = BasicBlock::Create(context, "while.header", wrapper);
		BasicBlock* latchBB = BasicBlock::Create(context, "while.latch", wrapper);

		// create call blocks (switch targets)
		BasicBlock** callBBs = new BasicBlock*[numContinuationFunctions]();
		for (unsigned i=0; i<numContinuationFunctions; ++i) {
			std::stringstream sstr;
			sstr << "switch." << i;  // "0123456789ABCDEF"[x] would be okay if we could guarantee a max size for continuations :p
			callBBs[i] = BasicBlock::Create(context, sstr.str(), wrapper);
		}

		// create exit block
		BasicBlock* exitBB = BasicBlock::Create(context, "exit", wrapper);


		//--------------------------------------------------------------------//
		// fill entry block
		//--------------------------------------------------------------------//
		builder.SetInsertPoint(entryBB);

		// generate union for live value structs
		unsigned unionSize = 0;
		for (DenseMap<unsigned, BarrierInfo*>::iterator it=continuations.begin(), E=continuations.end(); it!=E; ++it) {
			BarrierInfo* bit = it->second;
			StructType* liveValueStructType = bit->liveValueStructType;
			assert (liveValueStructType);
			const unsigned typeSize = targetData->getTypeAllocSize(liveValueStructType);
			if (unionSize < typeSize) unionSize = typeSize;
		}
		DEBUG_PKT( outs() << "union size for live value structs: " << unionSize << "\n"; );
		// allocate memory for union
		Value* allocSize = ConstantInt::get(context, APInt(32, unionSize));
		Value* dataPtr = builder.CreateAlloca(Type::getInt8Ty(context), allocSize, "liveValueUnion");

		builder.CreateBr(headerBB);

		//--------------------------------------------------------------------//
		// fill header
		//--------------------------------------------------------------------//
		builder.SetInsertPoint(headerBB);
		PHINode* current_barrier_id_phi = builder.CreatePHI(Type::getInt32Ty(context), "current_barrier_id");
		current_barrier_id_phi->addIncoming(ConstantInt::getNullValue(Type::getInt32Ty(context)), entryBB);

		SwitchInst* switchI = builder.CreateSwitch(current_barrier_id_phi, exitBB, numContinuationFunctions);
		for (unsigned i=0; i<numContinuationFunctions; ++i) {
			// add case for each continuation
			switchI->addCase(ConstantInt::get(context, APInt(32, i)), callBBs[i]);
		}


		//--------------------------------------------------------------------//
		// fill call blocks
		//--------------------------------------------------------------------//
		CallInst** calls = new CallInst*[numContinuationFunctions]();
		for (unsigned i=0; i<numContinuationFunctions; ++i) {
			BasicBlock* block = callBBs[i];
			builder.SetInsertPoint(block);

			// extract arguments from live value struct (dataPtr)
			BarrierInfo* bit = continuations[i];
			const StructType* sType = bit->liveValueStructType;
			const unsigned numLiveVals = sType->getNumElements();
			Value** contArgs = new Value*[numLiveVals]();

			Value* bc = builder.CreateBitCast(dataPtr, PointerType::getUnqual(sType)); // cast data pointer to correct pointer to struct type
			
			for (unsigned j=0; j<numLiveVals; ++j) {
				std::vector<Value*> indices;
				indices.push_back(ConstantInt::getNullValue(Type::getInt32Ty(context)));
				indices.push_back(ConstantInt::get(context, APInt(32, j)));
				Value* gep = builder.CreateGEP(bc, indices.begin(), indices.end(), "");
				DEBUG_PKT( outs() << "load gep(" << j << "): " << *gep << "\n"; );
				LoadInst* load = builder.CreateLoad(gep, false, "");
				contArgs[j] = load;
			}


			// create the call to f
			// the first block holds the call to the (remainder of the) original function,
			// which receives the original arguments plus the data pointer.
			// All other blocks receive the extracted live-in values plus the data pointer.
			SmallVector<Value*, 2> args;
			if (i == 0) {
				for (Function::arg_iterator A=wrapper->arg_begin(), AE=wrapper->arg_end(); A!=AE; ++A) {
					assert (isa<Value>(A));
					args.push_back(cast<Value>(A));
				}
			} else {
				for (unsigned j=0; j<numLiveVals; ++j) {
					args.push_back(contArgs[j]);
				}
			}
			args.push_back(dataPtr);

			std::stringstream sstr;
			sstr << "continuation." << i;  // "0123456789ABCDEF"[x] would be okay if we could guarantee a max number of continuations :p
			calls[i] = builder.CreateCall(continuations[i]->continuation, args.begin(), args.end(), sstr.str());
			//calls[i]->addAttribute(1, Attribute::NoCapture);
			//calls[i]->addAttribute(1, Attribute::NoAlias);
			DEBUG_PKT( outs() << "created call for continuation '" << continuations[i]->continuation->getNameStr() << "':" << *calls[i] << "\n"; );

			builder.CreateBr(latchBB);

			delete [] contArgs;
		}

		//--------------------------------------------------------------------//
		// fill latch
		//--------------------------------------------------------------------//
		builder.SetInsertPoint(latchBB);

		// create phi for next barrier id coming from each call inside the switch
		PHINode* next_barrier_id_phi = builder.CreatePHI(Type::getInt32Ty(context), "next_barrier_id");
		for (unsigned i=0; i<numContinuationFunctions; ++i) {
			next_barrier_id_phi->addIncoming(calls[i], callBBs[i]);
		}

		// add the phi as incoming value to the phi in the loop header
		current_barrier_id_phi->addIncoming(next_barrier_id_phi, latchBB);

		// create check whether id is the special end id ( = is negative)
		// if yes, exit the loop, otherwise go on iterating
		Value* cond = builder.CreateICmpSLT(next_barrier_id_phi, ConstantInt::getNullValue(Type::getInt32Ty(context)), "exitcond");
		builder.CreateCondBr(cond, exitBB, headerBB);

		//--------------------------------------------------------------------//
		// fill exit
		//--------------------------------------------------------------------//
		//CallInst::CreateFree(dataPtr, exitBB); //not possible with builder
		builder.SetInsertPoint(exitBB);
		builder.CreateRetVoid();


		delete [] calls;
		delete [] callBBs;

		DEBUG_PKT( outs() << "replaced all barriers by continuations!\n"; );

		return wrapper;
	}


};

} // namespace

char ContinuationGenerator::ID = 0;
static RegisterPass<ContinuationGenerator> CG("continuation-generation", "Continuation Generation");

// Public interface to the ContinuationGeneration pass
namespace llvm {
	FunctionPass* createContinuationGeneratorPass() {
		return new ContinuationGenerator();
	}
}


#endif	/* _CONTINUATIONGENERATOR_H */

