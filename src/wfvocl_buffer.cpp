/**
 * This file is distributed under the University of Illinois Open Source
 * License. See the COPYING file in the root directory for details.
 *
 * Copyright (C) 2010, 2011 Saarland University
 */

/**
 * The functions implemented here correspond to the functions described in
 * chapter 5.2 of the OpenCL 1.1 specification.
 */

#include "wfvocl.h"

/*
Memory objects are categorized into two types: buffer objects, and image objects. A buffer
object stores a one-dimensional collection of elements whereas an image object is used to store a
two- or three- dimensional texture, frame-buffer or image.
Elements of a buffer object can be a scalar data type (such as an int, float), vector data type, or a
user-defined structure. An image object is used to represent a buffer that can be used as a texture
or a frame-buffer. The elements of an image object are selected from a list of predefined image
formats. The minimum number of elements in a memory object is one.
*/

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_mem CL_API_CALL
clCreateBuffer(cl_context   context,
               cl_mem_flags flags,
               size_t       size, //in bytes
               void *       host_ptr,
               cl_int *     errcode_ret)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clCreateBuffer!\n"; );
    if (!context) { if (errcode_ret) *errcode_ret = CL_INVALID_CONTEXT; return NULL; }
    if (size == 0 || size > WFVOpenCL::getDeviceMaxMemAllocSize()) { if (errcode_ret) *errcode_ret = CL_INVALID_BUFFER_SIZE; return NULL; }
    const bool useHostPtr   = flags & CL_MEM_USE_HOST_PTR;
    const bool copyHostPtr  = flags & CL_MEM_COPY_HOST_PTR;
    const bool allocHostPtr = flags & CL_MEM_ALLOC_HOST_PTR;
    if (!host_ptr && (useHostPtr || copyHostPtr)) { if (errcode_ret) *errcode_ret = CL_INVALID_HOST_PTR; return NULL; }
    if (host_ptr && !useHostPtr && !copyHostPtr) { if (errcode_ret) *errcode_ret = CL_INVALID_HOST_PTR; return NULL; }
    if (useHostPtr && allocHostPtr) { if (errcode_ret) *errcode_ret = CL_INVALID_VALUE; return NULL; } // custom
    if (useHostPtr && copyHostPtr) { if (errcode_ret) *errcode_ret = CL_INVALID_VALUE; return NULL; } // custom

    const bool canRead     = (flags & CL_MEM_READ_ONLY) || (flags & CL_MEM_READ_WRITE);
    const bool canWrite    = (flags & CL_MEM_WRITE_ONLY) || (flags & CL_MEM_READ_WRITE);

    WFVOPENCL_DEBUG( outs() << "clCreateBuffer(" << size << " bytes, " << host_ptr << ")\n"; );
    WFVOPENCL_DEBUG( outs() << "  canRead     : " << (canRead ? "true" : "false") << "\n"; );
    WFVOPENCL_DEBUG( outs() << "  canWrite    : " << (canWrite ? "true" : "false") << "\n"; );
    WFVOPENCL_DEBUG( outs() << "  useHostPtr  : " << (useHostPtr ? "true" : "false") << "\n"; );
    WFVOPENCL_DEBUG( outs() << "  copyHostPtr : " << (copyHostPtr ? "true" : "false") << "\n"; );
    WFVOPENCL_DEBUG( outs() << "  allocHostPtr: " << (allocHostPtr ? "true" : "false") << "\n"; );

    void* device_ptr = NULL;

    if (useHostPtr) {
        assert (host_ptr);
        device_ptr = host_ptr;
        WFVOPENCL_DEBUG( outs() << "    using supplied host ptr: " << device_ptr << "\n"; );
    }

    if (allocHostPtr) {
        device_ptr = malloc(size);
        WFVOPENCL_DEBUG( outs() << "    new host ptr allocated: " << device_ptr << "\n"; );
        if (!device_ptr) { if (errcode_ret) *errcode_ret = CL_MEM_OBJECT_ALLOCATION_FAILURE; return NULL; }
    }

    if (copyHostPtr) {
        // CL_MEM_COPY_HOST_PTR can be used with
        // CL_MEM_ALLOC_HOST_PTR to initialize the contents of
        // the cl_mem object allocated using host-accessible (e.g.
        // PCIe) memory.
        assert (host_ptr);
        if (!allocHostPtr) {
            device_ptr = malloc(size);
            WFVOPENCL_DEBUG( outs() << "    new host ptr allocated for copying: " << device_ptr << "\n"; );
            if (!device_ptr) { if (errcode_ret) *errcode_ret = CL_MEM_OBJECT_ALLOCATION_FAILURE; return NULL; }
        }
        // copy data into new_host_ptr
        WFVOPENCL_DEBUG( outs() << "    copying data of supplied host ptr to new host ptr... "; );
        memcpy(device_ptr, host_ptr, size);
        WFVOPENCL_DEBUG( outs() << "done.\n"; );
    }

    // if no flag was supplied, allocate memory (host_ptr must be NULL by specification)
    if (!device_ptr) {
        assert (!host_ptr);
        device_ptr = malloc(size);
        WFVOPENCL_DEBUG( outs() << "    new host ptr allocated (no flag specified): " << device_ptr << "\n"; );
        if (!device_ptr) { if (errcode_ret) *errcode_ret = CL_MEM_OBJECT_ALLOCATION_FAILURE; return NULL; }
    }

    if (errcode_ret) *errcode_ret = CL_SUCCESS;
    return new _cl_mem(context, size, device_ptr, canRead, canWrite);
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_mem CL_API_CALL
clCreateSubBuffer(cl_mem                   buffer,
                  cl_mem_flags             flags,
                  cl_buffer_create_type    buffer_create_type,
                  const void *             buffer_create_info,
                  cl_int *                 errcode_ret)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clCreateSubBuffer!\n"; );
    assert (false && "NOT IMPLEMENTED!");
    return NULL;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBuffer(cl_command_queue    command_queue,
                    cl_mem              buffer,
                    cl_bool             blocking_read,
                    size_t              offset,
                    size_t              cb,
                    void *              ptr,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueReadBuffer!\n"; );
    if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
    if (!buffer) return CL_INVALID_MEM_OBJECT;
    if (!ptr || buffer->get_size() < cb+offset) return CL_INVALID_VALUE;
    if (!event_wait_list && num_events_in_wait_list > 0) return CL_INVALID_EVENT_WAIT_LIST;
    if (event_wait_list && num_events_in_wait_list == 0) return CL_INVALID_EVENT_WAIT_LIST;
    if (command_queue->context != buffer->get_context()) return CL_INVALID_CONTEXT;
    //err = clEnqueueReadBuffer( commands, output, CL_TRUE, 0, sizeof(float) * count, results, 0, NULL, NULL );

    if (event) {
        _cl_event* e = new _cl_event();
        e->dispatch = &static_dispatch;
        e->context = ((_cl_command_queue*)command_queue)->context;
        *event = e;
    }

    // Write data back into host memory (ptr) from device memory (buffer)
    // In our case, we actually should not have to copy data
    // because we are still on the CPU. However, const void* prevents this.
    // Thus, just copy over each byte.
    // TODO: specification seems to require something different?
    //       storing access patterns to command_queue or sth like that?

    void* data = buffer->get_data();
    memcpy(ptr, data, cb);

    return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBufferRect(cl_command_queue    command_queue,
                        cl_mem              buffer,
                        cl_bool             blocking_read,
                        const size_t *      buffer_origin,
                        const size_t *      host_origin,
                        const size_t *      region,
                        size_t              buffer_row_pitch,
                        size_t              buffer_slice_pitch,
                        size_t              host_row_pitch,
                        size_t              host_slice_pitch,
                        void *              ptr,
                        cl_uint             num_events_in_wait_list,
                        const cl_event *    event_wait_list,
                        cl_event *          event)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueReadBufferRec!\n"; );
    assert (false && "NOT IMPLEMENTED!");
    return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBuffer(cl_command_queue   command_queue,
                     cl_mem             buffer,
                     cl_bool            blocking_write,
                     size_t             offset,
                     size_t             cb,
                     const void *       ptr,
                     cl_uint            num_events_in_wait_list,
                     const cl_event *   event_wait_list,
                     cl_event *         event)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueWriteBuffer!\n"; );
    if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
    if (!buffer) return CL_INVALID_MEM_OBJECT;
    if (!ptr || buffer->get_size() < cb+offset) return CL_INVALID_VALUE;
    if (!event_wait_list && num_events_in_wait_list > 0) return CL_INVALID_EVENT_WAIT_LIST;
    if (event_wait_list && num_events_in_wait_list == 0) return CL_INVALID_EVENT_WAIT_LIST;
    if (command_queue->context != buffer->get_context()) return CL_INVALID_CONTEXT;

    if (event) {
        _cl_event* e = new _cl_event();
        e->dispatch = &static_dispatch;
        e->context = ((_cl_command_queue*)command_queue)->context;
        *event = e;
    }

    // Write data into 'device memory' (buffer)
    // In our case, we actually should not have to copy data
    // because we are still on the CPU. However, const void* prevents this.
    // Thus, just copy over each byte.
    // TODO: specification seems to require something different?
    //       storing access patterns to command_queue or sth like that?
    buffer->copy_data(ptr, cb, offset); //cb is size in bytes

    return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBufferRect(cl_command_queue    command_queue,
                         cl_mem              buffer,
                         cl_bool             blocking_write,
                         const size_t *      buffer_origin,
                         const size_t *      host_origin,
                         const size_t *      region,
                         size_t              buffer_row_pitch,
                         size_t              buffer_slice_pitch,
                         size_t              host_row_pitch,
                         size_t              host_slice_pitch,
                         const void *        ptr,
                         cl_uint             num_events_in_wait_list,
                         const cl_event *    event_wait_list,
                         cl_event *          event)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueWriteBufferRec!\n"; );
    assert (false && "NOT IMPLEMENTED!");
    return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBuffer(cl_command_queue    command_queue,
                    cl_mem              src_buffer,
                    cl_mem              dst_buffer,
                    size_t              src_offset,
                    size_t              dst_offset,
                    size_t              cb,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueCopyBuffer!\n"; );
    if (!command_queue) return CL_INVALID_COMMAND_QUEUE;
    if (!src_buffer) return CL_INVALID_MEM_OBJECT;
    if (!dst_buffer) return CL_INVALID_MEM_OBJECT;
    if (src_buffer->get_size() < cb || src_buffer->get_size() < src_offset || src_buffer->get_size() < cb+src_offset) return CL_INVALID_VALUE;
    if (dst_buffer->get_size() < cb || dst_buffer->get_size() < dst_offset || dst_buffer->get_size() < cb+dst_offset) return CL_INVALID_VALUE;
    if (!event_wait_list && num_events_in_wait_list > 0) return CL_INVALID_EVENT_WAIT_LIST;
    if (event_wait_list && num_events_in_wait_list == 0) return CL_INVALID_EVENT_WAIT_LIST;
    if (command_queue->context != src_buffer->get_context()) return CL_INVALID_CONTEXT;
    if (command_queue->context != dst_buffer->get_context()) return CL_INVALID_CONTEXT;
    if (src_buffer == dst_buffer) {
        if (dst_offset < src_offset) {
            if (src_offset - (dst_offset+cb) < 0) return CL_MEM_COPY_OVERLAP;
        } else {
            if (dst_offset - (src_offset+cb) < 0) return CL_MEM_COPY_OVERLAP;
        }
    }

    if (event) {
        _cl_event* e = new _cl_event();
        e->dispatch = &static_dispatch;
        e->context = ((_cl_command_queue*)command_queue)->context;
        *event = e;
    }

    // This function should not copy itself but only queue a command that does so... I don't care ;).

    void* src_data = src_buffer->get_data();
    dst_buffer->copy_data(src_data, cb, dst_offset, src_offset);

    return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferRect(cl_command_queue    command_queue,
                        cl_mem              src_buffer,
                        cl_mem              dst_buffer,
                        const size_t *      src_origin,
                        const size_t *      dst_origin,
                        const size_t *      region,
                        size_t              src_row_pitch,
                        size_t              src_slice_pitch,
                        size_t              dst_row_pitch,
                        size_t              dst_slice_pitch,
                        cl_uint             num_events_in_wait_list,
                        const cl_event *    event_wait_list,
                        cl_event *          event)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueCopyBufferRec!\n"; );
    assert (false && "NOT IMPLEMENTED!");
    return CL_SUCCESS;
}

WFVOPENCL_DLLEXPORT CL_API_ENTRY void * CL_API_CALL
clEnqueueMapBuffer(cl_command_queue command_queue,
                   cl_mem           buffer,
                   cl_bool          blocking_map,
                   cl_map_flags     map_flags,
                   size_t           offset,
                   size_t           cb,
                   cl_uint          num_events_in_wait_list,
                   const cl_event * event_wait_list,
                   cl_event *       event,
                   cl_int *         errcode_ret)
{
    WFVOPENCL_DEBUG ( outs() << "ENTERED clEnqueueMapBuffer!\n"; );
    assert (false && "NOT IMPLEMENTED!");
    return NULL;
}
