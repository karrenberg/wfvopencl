// Roger Pau Monné
// 2009-09-08
// royger@gmail.com

#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

#define K0  0x5A827999
#define K1  0x6ED9EBA1
#define K2  0x8F1BBCDC
#define K3  0xCA62C1D6 

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
__kernel void sha1(__global uint *W, __global uint *H, __global const uint *work_items)
{
    int i, gid, gid5;
    uint A, B, C, D, E, temp;
    uint start, end;
    gid = get_global_id(0);
    gid5 = gid*5;
    start = work_items[gid*2]*80;
    end = work_items[gid*2+1]*80;
    
    for(;start < end; start += 80)
    {
        A = H[gid5];
        B = H[gid5+1];
        C = H[gid5+2];
        D = H[gid5+3];
        E = H[gid5+4];
        
        for(i = 16; i < 80; i++)
        {
            W[start+i] = rotateLeft(W[start+i-3] ^ W[start+i-8] ^ W[start+i-14] ^ W[start+i-16], 1);
        }
        
        for(i = 0; i < 20; i++)
        {
            temp = rotateLeft(A,5) + ((B & C) | ((~ B) & D)) + E + W[start+i] + K0;
            E = D;
            D = C;
            C = rotateLeft(B, 30);
            B = A;
            A = temp;
        }
        
        for(i = 20; i < 40; i++)
        {
            temp = rotateLeft(A, 5) + (B ^ C ^ D) + E + W[start+i] + K1;
            E = D;
            D = C;
            C = rotateLeft(B, 30);
            B = A;
            A = temp;
        }
    
        for(i = 40; i < 60; i++)
        {
            temp = rotateLeft(A, 5) + ((B & C) | (B & D) | (C & D)) + E + W[start+i] + K2;
            E = D;
            D = C;
            C = rotateLeft(B, 30);
            B = A;
            A = temp;
        }
    
        for(i = 60; i < 80; i++)
        {
            temp = rotateLeft(A, 5) + (B ^ C ^ D)  + E + W[start+i] + K3;
            E = D;
            D = C;
            C = rotateLeft(B, 30);
            B = A;
            A = temp;
        }
        H[gid5] = (H[gid5] + A);
        H[gid5+1] = (H[gid5+1] + B);
        H[gid5+2] = (H[gid5+2] + C);
        H[gid5+3] = (H[gid5+3] + D);
        H[gid5+4] = (H[gid5+4] + E);
    }
}
__kernel void prepare(__global char *msg, __global const int *len, __global uint *W)
{
    int t, word_pad, W_pad, gid;
    gid = get_global_id(0);
    word_pad = gid * 64;
    W_pad = gid * 80;
    memset_user(&msg[word_pad + len[gid]], 0, 64 - len[gid]);
    msg[word_pad + len[gid]] = (char) 0x80;
    sha1_set_len(&msg[word_pad], len[gid] * 8);

    for (t = 0; t < 16; t++)
    {
        W[W_pad + t] = ((uchar) msg[word_pad + t * 4]) << 24;
        W[W_pad + t] |= ((uchar) msg[word_pad + t * 4 + 1]) << 16;
        W[W_pad + t] |= ((uchar) msg[word_pad + t * 4 + 2]) << 8;
        W[W_pad + t] |= (uchar) msg[word_pad + t * 4 + 3];
    }
}
__kernel void hexdigest(__global const uint *H, __global char *digest)
{
    int i, j, gid, gid5, gid41;
    uint number;
    char hexChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    
    gid = get_global_id(0);
    gid5 = gid*5;
    gid41 = gid*41;
    
    for(j = 0; j < 5; j++)
    {
        number = H[gid5+j];
        for(i = 0; i < 8; i++)
        {
            digest[gid41 + j*8 + 7-i] = hexChars[number%16];
            number /= 16;
        }
    }
    digest[gid41 + 40] = '\0';
}
