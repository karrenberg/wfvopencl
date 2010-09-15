#!/usr/bin/env python
# -*- coding: iso-8859-1

"""A simple implementation of SHA-1 in OpenCL.

   Framework adapted from Dinu Gherman's MD5 implementation by
   J. Hallén and L. Creighton. SHA-1 implementation based directly on
   the text of the NIST standard FIPS PUB 180-1.
   OpenCL implementation by Roger Pau Monné
"""


__date__    = '2009-09-08'
__version__ = 0.1


import datetime, fileinput, string
import pyopencl as cl
import numpy as np

class sha1_cl:
    """Realizes the sha1 calculus in the GPU"""
    def __init__(self):
        with open('sha1.cl', 'r') as f:
            self.cl_program = f.read()

        self.ctx = cl.create_context_from_type(cl.device_type.GPU)
        self.queue = cl.CommandQueue(self.ctx)
        self.prg = cl.Program(self.ctx, self.cl_program)

        try:
            self.prg = self.prg.build()
        except:
            print "Error"
            print self.prg.get_build_info(self.ctx.get_info(cl.context_info.DEVICES)[0], cl.program_build_info.LOG)

        self.H0 = 0x67452301L
        self.H1 = 0xEFCDAB89L
        self.H2 = 0x98BADCFEL
        self.H3 = 0x10325476L
        self.H4 = 0xC3D2E1F0L
        self.init()
        
    def init(self):
        self.Word_buffer = []
        self.work_items = []
        self.len = []
        self.result = []

    def add(self, inBuf):
        """Add the current message to the buffer"""
        leninBuf = long(len(inBuf))
        
        if leninBuf >= 56:
            print "String too long: ", inBuf
            return False
            
        self.Word_buffer += list(inBuf) + ['']*(64-leninBuf)
        self.len += [leninBuf]
        self.work_items += [len(self.Word_buffer)/64-1, len(self.Word_buffer)/64]
        return True

    def transform(self):
        """Realizes the calculus"""
        # 1 step, transform the char array into uints
        mf = cl.mem_flags
        
        W_array = np.empty((len(self.Word_buffer)/64*80)).astype(np.uint32)
        msg = np.char.array(self.Word_buffer)
        len_array = np.array(self.len).astype(np.int32)
        # Allocate device memory
        W_buf = cl.Buffer(self.ctx, mf.WRITE_ONLY, W_array.nbytes)
        msg_buf = cl.Buffer(self.ctx, mf.READ_WRITE | mf.COPY_HOST_PTR, msg.nbytes, msg)
        len_buf = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, len_array.nbytes, len_array)
        # Convert the 56 char arrays into 16 32-bit unsigned integers
        time1 = datetime.datetime.now()
        self.prg.prepare(self.queue, (len(msg)/64,), msg_buf, len_buf, W_buf)
        cl.enqueue_read_buffer(self.queue, W_buf, W_array).wait()
        time2 = datetime.datetime.now()
        print "Execution time prepare OpenCL: " + repr((time2 - time1).microseconds/1000) + "ms"
        
        # Prepare the new arrays and device memory allocation
        H_array = np.array([self.H0, self.H1, self.H2, self.H3, self.H4]*(len(W_array)/80)).astype(np.uint32)
        I_array = np.array(self.work_items).astype(np.uint32)

        H_buf = cl.Buffer(self.ctx, mf.READ_WRITE | mf.COPY_HOST_PTR, H_array.nbytes, H_array)
        W_buf = cl.Buffer(self.ctx, mf.READ_WRITE | mf.COPY_HOST_PTR, W_array.nbytes, W_array)
        I_buf = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, I_array.nbytes, I_array)
        
        # Execurte sha1 transform
        time1 = datetime.datetime.now()
        self.prg.sha1(self.queue, (len(W_array)/80,), W_buf, H_buf, I_buf)
        cl.enqueue_read_buffer(self.queue, H_buf, H_array).wait()
        time2 = datetime.datetime.now()
        print "Execution time sha1 OpenCL: " + repr((time2 - time1).microseconds/1000) + "ms"
        
        # Convert the result (5 32-bit unsigned integers) to hexadecimal
        Hexdigest_array = np.char.array(['']*41*(len(H_array)/5))
        Hexdigest_buf = cl.Buffer(self.ctx, mf.WRITE_ONLY, Hexdigest_array.nbytes)
        H_buf = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, H_array.nbytes, H_array)
        self.prg.hexdigest(self.queue, (len(Hexdigest_array)/41,), H_buf, Hexdigest_buf)
        cl.enqueue_read_buffer(self.queue, Hexdigest_buf, Hexdigest_array).wait()
        # Put the result in the array
        for j in range(0,len(Hexdigest_array)/41):
            self.result.append(''.join(Hexdigest_array[j*41 + 0:j*41 + 41]))

    def hexdigest_file(self, file):
        with open(file, 'a') as f:
            for hash in self.result:
                f.write(hash + "\n")
                
        self.init()
        
    def hexdigest(self):
        result = self.result
        self.init()
        return result
        
digest_size = digestsize = 20
blocksize = 1

"""
# Example 1, read from file 'input' and save hashes to file 'output'
input = 'dict.txt'
output = 'out.txt'

i = 0
cl_buffer = sha1_cl()
print "Reading file"
for line in fileinput.input([input]):
    if line[0] != '#':
        if cl_buffer.add(string.rstrip(line, "\n")):
            i += 1
    if i == 320000:
        print "Transform & store"
        cl_buffer.transform()
        cl_buffer.hexdigest_file(out)
        i = 0
        print "Reading again..."

print "Transform & store final block"
cl_buffer.transform()
cl_buffer.hexdigest_file(output)
print "Byebye"
"""

# Example 2, single hash
cl_buffer = sha1_cl()
cl_buffer.add("abc")
cl_buffer.add("aaa")
print hashlib.sha1("abc").hexdigest()
print hashlib.sha1("aaa").hexdigest()
cl_buffer.transform()
print cl_buffer.hexdigest()

