#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SMALL_ENDIAN    1
unsigned int Key[4] = {
			0x5A827999, // (0 <= t <= 19) 
			0x6ED9EBA1, //(20 <= t <= 39) 
			0x8F1BBCDC, //(40 <= t <= 59) 
			0xCA62C1D6 //(60 <= t <= 79). 
		 	};
//因为最多填充64个字节，64＊8 ＝ 512位，所以预先定义好要填充的内容，开头的0x80保证了填充的第一位为1  0x80 ＝ 1000 0000
unsigned char padd[64] = {
				0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		    		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	
		    		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	
		   	      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	
		   	      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	
		            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	
		            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	
		            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
} ;

//根据给定的n 将x循环左移n位
unsigned int ShiftL(unsigned int x,unsigned int n)
{	
	return (x<<n)|(x >>(32-n));
}

void InitH(unsigned int *H)
{
	H[0] = 0x67452301 ;
	H[1] = 0xEFCDAB89 ;
	H[2] = 0x98BADCFE ;
	H[3] = 0x10325476 ;
	H[4] = 0xC3D2E1F0 ;
}
//ft(B,C,D) = (B AND C) OR ((NOT B) AND D) ( 0 <= t <= 19) 
//ft(B,C,D) = B XOR C XOR D              (20 <= t <= 39) 
//ft(B,C,D) = (B AND C) OR (B AND D) OR (C AND D) (40 <= t <= 59) 
//ft(B,C,D) = B XOR C XOR D                     (60 <= t <= 79).

//因为不同的数据段需要做不同的运算，所以定义成宏，范围从宏名就可以看出
#define FT0_19(b,c,d)  ((b&c)|((~b)&d))
#define FT20_39(b,c,d) (b^c^d)
#define FT40_59(b,c,d) ((b&c)|(b&d)|(c&d))
#define FT60_79(b,c,d) (b^c^d)

unsigned char *Big_Edian_Padd(unsigned char *key,unsigned char *rand,int *len)
{
	unsigned int keylen = strlen(key) ;
	unsigned int randlen = strlen(rand);
	unsigned int totallen = keylen + randlen ;
	printf("randlen = %d\n",randlen);
	/*index记录了总共长度的低六位数据，因为算法要求填充之后的长度满足 x%512（64字节） == 448(56 字节)，所以
	实际上totallen的低六位就记录了数据应该被填充的情况，一旦高于六位，％512 肯定 ＝＝ 0也就是(x&0x3f)%64 == 0
	totallen＆0x3f保证了 index < 64*/
	int index = totallen&0x3f,paddlen = 0;
	unsigned char *encrypt ;
	if(index > 56)//如果大于56 的话，64 + 56 ＝＝ 120 当index大于56时 则由totallen需要满足的条件可知  x％64 ＝＝ index
			//index必须能够是totallen能够对齐到下一个64位，所以有index距离 64还有 64 － index ，当x + 64-index时
			//保证了新的x ％64 ＝＝ 0 如果要想余56 则还需要56 所以paddlen ＝ 64 -index + 56即 paddlen ＝ 120- index
	{
		paddlen = 120 - index ;
	}
	else if(index < 56)//当小于56时只需要满足求余 ＝ 56 就行了，所以直接paddlen ＝ 56 - 
	{
		paddlen = 56 - index ;
	}
	else //刚好求余满足 56 时，直接填充64个字节之后就满足了对齐以及求余要求了
	{
		paddlen = 64 ;
	}
	*len = totallen + paddlen + 8 ;//8 个字节是记录长度
	encrypt = (unsigned char *)malloc(sizeof(unsigned char)*(*len));
	memcpy(encrypt,key,keylen);
	memcpy(&encrypt[keylen],rand,randlen);
	memcpy(&encrypt[totallen],padd,paddlen);
	int tmp = totallen<<3 ;
#if SMALL_ENDIAN 
	unsigned char *p,*q ;
	int tmp1 ;
	int z = 0 ;
	p = (unsigned char *)&tmp;
	q = (unsigned char *)&tmp1;
	for(;z < 4;z++)
		*q++ = *(p+3-z);	
	memcpy(&encrypt[totallen+paddlen+sizeof(tmp1)],&tmp1,sizeof(tmp1));
#else
	memcpy(&encrypt[totallen+paddlen+sizeof(tmp)],&tmp,sizeof(tmp));
#endif
	tmp  = 0 ;
	memcpy(&encrypt[totallen+paddlen],&tmp,sizeof(tmp));
	
	return encrypt ;
}

//给定key和rand加密之后返回二十个字节（unsigned char 类型的）20＊8 ＝ 160 位，用后空间由调用函数释放
unsigned char *Encrypt_Sha_1(unsigned char *key, unsigned char *rand)
{
	unsigned char *encrypt = NULL;
	unsigned int len ;
	encrypt = Big_Edian_Padd(key,rand,&len);
	if(encrypt == NULL)
		return NULL;
	int blocks = len/64 ;
	
	unsigned int abcde[5] ,w[80],tmp;
	unsigned int *H = (unsigned int *)malloc(sizeof(unsigned int)*5);
	InitH(H);
	int i = 0;
	for(;i < blocks;i++)
	{
		int j = 0;
		for(;j < 16;j++)//取出16＊4个字节即：16＊4＊8 ＝ 512 位 待加密的数据
		{
			unsigned int x = 0 ;
#if SMALL_ENDIAN
			memcpy((unsigned char *)&x,&encrypt[i*64 + j*sizeof(x)],sizeof(x));
			unsigned char *p = (unsigned char *)&x;
			unsigned char *q = (unsigned char *)&w[j];
			int z = 0;
			for(;z < 4;z++)
			*q++ = *(p+3-z);
#else
			memcpy((unsigned char *)&w[j],&encrypt[i*64 + j*4],4);
#endif
		}
		for(j = 16;j < 80;j++)//将80个int 填充完全
		{
			w[j] = ShiftL(w[j-3]^w[j-8]^w[j-14]^w[j-16],1);
		}
		abcde[0] = H[0];
		abcde[1] = H[1];
		abcde[2] = H[2];
		abcde[3] = H[3];
		abcde[4] = H[4];
		j = 0;
		for(;j < 80 ;j++)
		{
			switch(j/10)
			{
			case 0:
			case 1:tmp = ShiftL(abcde[0],5)+FT0_19(abcde[1],abcde[2],abcde[3])\
							+abcde[4]+w[j]+Key[0];break ;
			case 2:
			case 3:tmp = ShiftL(abcde[0],5)+FT20_39(abcde[1],abcde[2],abcde[3])\
							+abcde[4]+w[j]+Key[1];break ;
			case 4:
			case 5:tmp = ShiftL(abcde[0],5)+FT40_59(abcde[1],abcde[2],abcde[3])\
							+abcde[4]+w[j]+Key[2];break ;
			case 6:
			case 7:tmp = ShiftL(abcde[0],5)+FT60_79(abcde[1],abcde[2],abcde[3])\
							+abcde[4]+w[j]+Key[3];break;
			}
			abcde[4] = abcde[3];
			abcde[3] = abcde[2];
			abcde[2] = ShiftL(abcde[1],30);
			abcde[1] = abcde[0];
			abcde[0] = tmp ;
		}
		
		H[0] = H[0] + abcde[0];
		H[1] = H[1] + abcde[1];
		H[2] = H[2] + abcde[2];
		H[3] = H[3] + abcde[3];
		H[4] = H[4] + abcde[4];
		
	/*	H0 = H0 + A, H1 = H1 + B, H2 = H2 + C, H3 = H3 + D, H4 = H4 + E. 
		TEMP = S5(A) + ft(B,C,D) + E + Wt + Kt; 
		E = D; D = C; C = S30(B); B = A; A = TEMP; */
	}
	free(encrypt);
	return (unsigned char *)H;
}

int main(int argc,char *argv[])
{
	int i = 0;
	unsigned char key[] = "12345678901234567890";
	
	
	unsigned int *H = Encrypt_Sha_1(key,rand);
	char *p = (char *)H;
	for(i = 0 ;i < 20 ;i++)
	{
		printf("%02X",*p++);
	}
	free(H);
	return 0;
}
