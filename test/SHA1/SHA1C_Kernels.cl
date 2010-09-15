// Roger Pau Monné
// 2009-09-08
// royger@gmail.com

#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

#define K0  0x5A827999
#define K1  0x6ED9EBA1
#define K2  0x8F1BBCDC
#define K3  0xCA62C1D6

#define H1 0x67452301
#define H2 0xEFCDAB89
#define H3 0x98BADCFE
#define H4 0x10325476
#define H5 0xC3D2E1F0

uint rotateLeft(uint x, int n)
{
    return  (x << n) | (x >> (32-n));
}
void sha1_set_len(__global char *word, int len)
{
  uint ulen = len & 0xFFFFFFFF;

  word[60] = ulen >> 24;
  word[61] = ulen >> 16;
  word[62] = ulen >> 8;
  word[63] = ulen;
}
void memset_user(__global char *s, int c, int size)
{
    int i;
    for(i = 0; i < size; i++)
    {
        s[i] = (char) c;
    }
    return;
}
__kernel void sha1(__global char *msg, __global const int *len, __global char *digest)
{
    int t, word_pad, gid, i, j, gid41;
    uint W[80], A[5], temp, number;
    char hexChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    
    gid = get_global_id(0);
    gid41 = gid * 41;
    word_pad = gid * 64;
    memset_user(&msg[word_pad + len[gid]], 0, 64 - len[gid]);
    msg[word_pad + len[gid]] = (char) 0x80;
    sha1_set_len(&msg[word_pad], len[gid] * 8);

    A[0] = H1;
    A[1] = H2;
    A[2] = H3;
    A[3] = H4;
    A[4] = H5;
    
    for (t = 0; t < 16; t++)
    {
        W[t] = ((uchar) msg[word_pad + t * 4]) << 24;
        W[t] |= ((uchar) msg[word_pad + t * 4 + 1]) << 16;
        W[t] |= ((uchar) msg[word_pad + t * 4 + 2]) << 8;
        W[t] |= (uchar) msg[word_pad + t * 4 + 3];
    }
    
    for(i = 16; i < 80; i++)
    {
        W[i] = rotateLeft(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1);
    }
    
    for(i = 0; i < 20; i++)
    {
        temp = rotateLeft(A[0],5) + ((A[1] & A[2]) | ((~ A[1]) & A[3])) + A[4] + W[i] + K0;
        A[4] = A[3];
        A[3] = A[2];
        A[2] = rotateLeft(A[1], 30);
        A[1] = A[0];
        A[0] = temp;
    }
    
    for(i = 20; i < 40; i++)
    {
        temp = rotateLeft(A[0], 5) + (A[1] ^ A[2] ^ A[3]) + A[4] + W[i] + K1;
        A[4] = A[3];
        A[3] = A[2];
        A[2] = rotateLeft(A[1], 30);
        A[1] = A[0];
        A[0] = temp;
    }

    for(i = 40; i < 60; i++)
    {
        temp = rotateLeft(A[0], 5) + ((A[1] & A[2]) | (A[1] & A[3]) | (A[2] & A[3])) + A[4] + W[i] + K2;
        A[4] = A[3];
        A[3] = A[2];
        A[2] = rotateLeft(A[1], 30);
        A[1] = A[0];
        A[0] = temp;
    }

    for(i = 60; i < 80; i++)
    {
        temp = rotateLeft(A[0], 5) + (A[1] ^ A[2] ^ A[3])  + A[4] + W[i] + K3;
        A[4] = A[3];
        A[3] = A[2];
        A[2] = rotateLeft(A[1], 30);
        A[1] = A[0];
        A[0] = temp;
    }
    A[0] += H1;
    A[1] += H2;
    A[2] += H3;
    A[3] += H4;
    A[4] += H5;

    for(j = 0; j < 5; j++)
    {
        number = A[j];
        for(i = 0; i < 8; i++)
        {
            digest[gid41 + j*8 + 7-i] = hexChars[number%16];
            number /= 16;
        }
    }
    digest[gid41 + 40] = '\0';
}
