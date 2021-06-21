#ifndef INCLUDE_FLAMEGPU_RUNTIME_AGENTFUNCTION_H_
#define INCLUDE_FLAMEGPU_RUNTIME_AGENTFUNCTION_H_

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <curand_kernel.h>

// #include "flamegpu/runtime/flamegpu_device_api.h"
#include "flamegpu/defines.h"
#include "flamegpu/exception/FGPUDeviceException.h"
#include "flamegpu/runtime/AgentFunction_shim.h"
#include "flamegpu/gpu/CUDAScanCompaction.h"


// ! FLAMEGPU function return type
enum FLAME_GPU_AGENT_STATUS { ALIVE = 1, DEAD = 0 };

typedef void(AgentFunctionWrapper)(
#if !defined(SEATBELTS) || SEATBELTS
    DeviceExceptionBuffer *error_buffer,
#endif
    Curve::NamespaceHash instance_id_hash,
    Curve::NamespaceHash agent_func_name_hash,
    Curve::NamespaceHash messagename_inp_hash,
    Curve::NamespaceHash messagename_outp_hash,
    Curve::NamespaceHash agent_output_hash,
    id_t *d_agent_output_nextID,
    const unsigned int popNo,
    const void *in_messagelist_metadata,
    const void *out_messagelist_metadata,
    curandState *d_rng,
    unsigned int *scanFlag_agentDeath,
    unsigned int *scanFlag_messageOutput,
    unsigned int *scanFlag_agentOutput);  // Can't put __global__ in a typedef
#if defined(__CUDACC__)
#include "flamegpu/runtime/messaging/Spatial3D.h"
//__shared__ Curve::NamespaceHash msginhash;
//__shared__ MsgSpatial3D::MetaData msginmetadata;
#endif
/**
 * Wrapper function for launching agent functions
 * Initialises FLAMEGPU_API instance
 * @param error_buffer Buffer used for detecting and reporting DeviceErrors (flamegpu must be built with SEATBELTS enabled for this to be used)
 * @param instance_id_hash CURVE hash of the CUDASimulation's instance id
 * @param agent_func_name_hash CURVE hash of the agent + function's names
 * @param messagename_inp_hash CURVE hash of the input message's name
 * @param messagename_outp_hash CURVE hash of the output message's name
 * @param agent_output_hash CURVE hash of "_agent_birth" or 0 if agent birth not present
 * @param d_agent_output_nextID If agent output is enabled, this points to a global memory src of the next suitable agent id, this will be atomically incremented at birth
 * @param popNo Total number of agents executing the function (number of threads launched)
 * @param in_messagelist_metadata Pointer to the MsgIn metadata struct, it is interpreted by MsgIn
 * @param out_messagelist_metadata Pointer to the MsgOut metadata struct, it is interpreted by MsgOut
 * @param d_rng Array of curand states for this kernel
 * @param scanFlag_agentDeath Scanflag array for agent death
 * @param scanFlag_messageOutput Scanflag array for optional message output
 * @param scanFlag_agentOutput Scanflag array for optional agent output
 * @tparam AgentFunction The modeller defined agent function (defined as FLAMEGPU_AGENT_FUNCTION in model code)
 * @tparam MsgIn Message handler for input messages (e.g. MsgNone, MsgBruteForce, MsgSpatial3D)
 * @tparam MsgOut Message handler for output messages (e.g. MsgNone, MsgBruteForce, MsgSpatial3D)
 */
template<typename AgentFunction, typename MsgIn, typename MsgOut>
__global__ void agent_function_wrapper(
#if !defined(SEATBELTS) || SEATBELTS
    DeviceExceptionBuffer *error_buffer,
#endif
    Curve::NamespaceHash instance_id_hash,
    Curve::NamespaceHash agent_func_name_hash,
    Curve::NamespaceHash messagename_inp_hash,
    Curve::NamespaceHash messagename_outp_hash,
    Curve::NamespaceHash agent_output_hash,
    id_t *d_agent_output_nextID,
    const unsigned int popNo,
    const void *in_messagelist_metadata,
    const void *out_messagelist_metadata,
    curandState *d_rng,
    unsigned int *scanFlag_agentDeath,
    unsigned int *scanFlag_messageOutput,
    unsigned int *scanFlag_agentOutput) {
#if !defined(SEATBELTS) || SEATBELTS
    extern __shared__ Curve::NamespaceHash sm_buff[];
#else
    extern __shared__ Curve::NamespaceHash sm_buff[];
#endif
    // Store spatial messaging stuff in shared mem
    if (in_messagelist_metadata && threadIdx.x == 0) {  // This should be nullptr if there are no input messages
        Curve::NamespaceHash *buff1 = reinterpret_cast<Curve::NamespaceHash*>(sm_buff);
        *buff1 = agent_func_name_hash + messagename_inp_hash;
        MsgSpatial3D::MetaData* buff2 = reinterpret_cast<MsgSpatial3D::MetaData*>(sm_buff+2);
        //printf("init: %p, %p\n", buff1, buff2);
        *buff2 = *reinterpret_cast<const MsgSpatial3D::MetaData*>(in_messagelist_metadata);
    }
    __syncthreads();

    // Must be terminated here, else AgentRandom has bounds issues inside DeviceAPI constructor
    if (DeviceAPI<MsgIn, MsgOut>::getThreadIndex() >= popNo)
        return;
    // create a new device FLAME_GPU instance
    DeviceAPI<MsgIn, MsgOut> api = DeviceAPI<MsgIn, MsgOut>(
        instance_id_hash,
        agent_func_name_hash,
        agent_output_hash,
        d_agent_output_nextID,
        d_rng,
        scanFlag_agentOutput,
        MsgIn::In(agent_func_name_hash, messagename_inp_hash, in_messagelist_metadata),
        MsgOut::Out(agent_func_name_hash, messagename_outp_hash, out_messagelist_metadata, scanFlag_messageOutput));

    // call the user specified device function
    FLAME_GPU_AGENT_STATUS flag = AgentFunction()(&api);
    if (scanFlag_agentDeath) {
        // (scan flags will not be processed unless agent death has been requested in model definition)
        scanFlag_agentDeath[DeviceAPI<MsgIn, MsgOut>::getThreadIndex()] = flag;
#if !defined(SEATBELTS) || SEATBELTS
    } else if (flag == DEAD) {
        DTHROW("Agent death must be enabled per agent function when defining the model.\n");
#endif
    }
}


#endif  // INCLUDE_FLAMEGPU_RUNTIME_AGENTFUNCTION_H_
