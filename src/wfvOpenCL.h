#ifndef WFVOPENCL_H__
#define WFVOPENCL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <llvm/Instructions.h>

using namespace llvm;

namespace WFVOpenCL {

bool packetizeKernelFunction(
    const std::string& kernelName,
    const std::string& targetKernelName,
    llvm::Module* mod,
    const cl_uint packetizationSize,
    const cl_uint simdDim,
    const bool use_sse41,
    const bool use_avx,
    const bool verbose);
    CallInst* insertPrintf(const std::string& message, Value* value, const bool endLine, Instruction* insertBefore);
    bool barrierBetweenInstructions(BasicBlock* block, Instruction* A, Instruction* B, std::set<BasicBlock*>& visitedBlocks);
    void findStepThroughCallbackUses(Instruction* inst, CallInst* call, std::vector<CallInst*>& calls, std::vector<Instruction*>& uses, std::vector<Instruction*>& targets);
    void findCallbackUses(CallInst* call, std::vector<CallInst*>& calls, std::vector<Instruction*>& uses, std::vector<Instruction*>& targets);
    void replaceCallbackUsesByNewCallsInFunction(Function* callback, Function* parentF);
    void replaceCallbacksByArgAccess(Function* f, Value* arg, Function* source);
    //llvm::Function* generateKernelWrapper(
    CallInst* getWrappedKernelCall(Function* wrapper, Function* kernel);
    void fixFunctionNames(Module* mod);
    unsigned getBestSimdDim(Function* f, const unsigned num_dimensions);
    unsigned determineNumDimensionsUsed(Function* f);
    Value* generateLocalFlatIndex(const unsigned num_dimensions, Instruction** local_ids, Instruction** local_sizes, Instruction* insertBefore);
    void adjustLiveValueLoadGEPs(CallInst* newCall, const unsigned continuation_id, const unsigned num_dimensions, Instruction** local_ids, Instruction** local_sizes);
    void adjustLiveValueStoreGEPs(Function* continuation, const unsigned num_dimensions, LLVMContext& context);
    void mapCallbacksToContinuationArguments(const unsigned num_dimensions, LLVMContext& context, Module* module, ContinuationGenerator::ContinuationVecType& continuations);
    void createGroupConstantSpecialParamLoads(
            const unsigned num_dimensions,
            LLVMContext& context,
            Value* arg_work_dim,
            Value* arg_global_size_array,
            Value* arg_local_size_array,
            Value* arg_group_id_array,
            Value* arg_num_groups_array,
            Instruction** global_sizes,
            Instruction** local_sizes,
            Instruction** group_ids,
            Instruction** num_groupss,
            Instruction* insertBefore
    );
    void generateLoopsAroundCall(
            CallInst* call,
            const unsigned num_dimensions,
            const int simd_dim,
            Instruction** local_sizes,
            Instruction** group_ids,
            Value* arg_global_id_array,
            Value* arg_local_id_array,
            LLVMContext& context,
            Instruction** global_ids,
            Instruction** local_ids
    );
    void generateBlockSizeLoopsForWrapper(Function* f, CallInst* call, const unsigned num_dimensions, const int simd_dim, LLVMContext& context, Module* module);
    void generateBlockSizeLoopsForContinuations(const unsigned num_dimensions, const int simd_dim, LLVMContext& context, Function* f, ContinuationGenerator::ContinuationVecType& continuations);
    Function* createKernel(Function* f, const std::string& kernel_name, const unsigned num_dimensions, const int simd_dim, Module* module, TargetData* targetData, LLVMContext& context, cl_int* errcode_ret, Function** f_SIMD_ret);
    cl_uint convertLLVMAddressSpace(cl_uint llvm_address_space);
    std::string getAddressSpaceString(cl_uint cl_address_space);
    unsigned long long getDeviceMaxMemAllocSize();

}

#ifdef __cplusplus
}
#endif

#endif
