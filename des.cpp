#include <memory.h>
#include <string.h>
#include <stdio.h>

enum	{ENCRYPT,DECRYPT};
/*void Des_Run(char Out[8], char In[8], bool Type=ENCRYPT);
void Des_SetKey( char Key[8]);
static void F_func(bool In[32],  bool Ki[48]);// f º¯Êý
static void S_func(bool Out[32],  bool In[48]);// S ºÐ´úÌæ
static void Transform(bool *Out, bool *In,  char *Table, int len);
static void Xor(bool *InA,  bool *InB, int len);// Òì»ò
static void RotateL(bool *In, int len, int loop);// Ñ­»·×óÒÆ
static void ByteToBit(bool *Out,  char *In, int bits);
static void BitToByte(char *Out,  bool *In, int bits);*/
 static char IP_Table[64] = {
		58,50,42,34,26,18,10,2,60,52,44,36,28,20,12,4,
		62,54,46,38,30,22,14,6,64,56,48,40,32,24,16,8,
		57,49,41,33,25,17,9,1,59,51,43,35,27,19,11,3,
		61,53,45,37,29,21,13,5,63,55,47,39,31,23,15,7
};

static char IPR_Table[64] = {
		40,8,48,16,56,24,64,32,39,7,47,15,55,23,63,31,
		38,6,46,14,54,22,62,30,37,5,45,13,53,21,61,29,
		36,4,44,12,52,20,60,28,35,3,43,11,51,19,59,27,
		34,2,42,10,50,18,58,26,33,1,41,9,49,17,57,25
};

static  char E_Table[48] = {
		32,1,2,3,4,5,4,5,6,7,8,9,
		8,9,10,11,12,13,12,13,14,15,16,17,
		16,17,18,19,20,21,20,21,22,23,24,25,
		24,25,26,27,28,29,28,29,30,31,32,1
};

static char P_Table[32] = {
		16,7,20,21,29,12,28,17,1,15,23,26,5,18,31,10,
		2,8,24,14,32,27,3,9,19,13,30,6,22,11,4,25
};

static char PC1_Table[56] = {
		57,49,41,33,25,17,9,1,58,50,42,34,26,18,
		10,2,59,51,43,35,27,19,11,3,60,52,44,36,
		63,55,47,39,31,23,15,7,62,54,46,38,30,22,
		14,6,61,53,45,37,29,21,13,5,28,20,12,4
};

 static char PC2_Table[48] = {
		14,17,11,24,1,5,3,28,15,6,21,10,
		23,19,12,4,26,8,16,7,27,20,13,2,
		41,52,31,37,47,55,30,40,51,45,33,48,
		44,49,39,56,34,53,46,42,50,36,29,32
};

 static char LOOP_Table[16] = {
	1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1
};

 static char S_Box[8][4][16] = {
	// S1 
		14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7,
		0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8,
		4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0,
		15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13,
		//S2
		15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10,
		3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5,
		0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15,
		13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9,
		//S3
		10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8,
		13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1,
		13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7,
		13,11,0,0,6,9,8,7,4,15,14,3,11,5,2,12,
		//S4
		7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15,
		13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9,
		10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4,
		3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14,
		//S5
		2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9,
		14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6,
		4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14,
		11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3,
		//S6
		12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11,
		10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8,
		9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6,
		4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13,
		//S7
		4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1,
		13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6,
		1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2,
		6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12,
		//S8
		13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7,
		1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2,
		7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8,
		2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11
};
static bool SubKey[16][48];// 16È¦×ÓÃÜÔ¿
void ByteToBit(bool *out, char *in,int bits)
{
	int i = 0;
	for(i = 0; i< bits;i++)
	{
		out[i] = (in[i/8]>>(i%8))&1 ;
	}
}
void Transform(bool *out,bool *in,char *table,int bits)
{
	int i = 0 ;
	static bool tmp[256];
	for(i= 0;i < bits;i++)
		tmp[i] = in[table[i]-1];
	memcpy(out,tmp,bits);
}
void RotateL(bool *in,int bits,int totalbits)
{
	bool tmp[28] ;
	memcpy(tmp,in,bits);
	memcpy(in,in+bits,totalbits-bits);
	memcpy(in+totalbits-bits,tmp,bits);
}
void SetKey( char *key) //produce the subkey
{
	static bool bool_key[64];
	ByteToBit(bool_key,key,64);
	Transform(bool_key,bool_key,PC1_Table,56);
	static bool *bool_l = &bool_key[0];
	static bool *bool_r = &bool_key[28] ;
	
	int i = 0 ;
	for(i = 0; i < 16;i++)
	{
		RotateL(bool_l,LOOP_Table[i],28);
		RotateL(bool_r,LOOP_Table[i],28);
		Transform(&SubKey[i][0],bool_key,PC2_Table,48);//absorb 48 bits from bool_key in the front of 56 bits
	}
}
void Xor(bool *out,bool *in,int bits)
{
	int i = 0;
	for(i = 0 ;i < bits;i++)
	 out[i] ^= in[i] ;
}
void BitToByte(char *out,bool *in,int bits)
{
	int i = 0 ;
	memset(out,0,(bits+7)/8);
	for(i = 0 ;i < bits;i++)
	{
		out[i/8] |= (in[i]<<(i%8));
	}
}
void S_function(bool *out,bool *in)
{
	int i = 0;
	int j,k ;
	for(i = 0; i < 8;out += 4,in += 6,i++)
	{
		j = (in[1]<<3) + (in[2]<<2) + (in[3]<<1) + in[4];
		k = (in[0]<<1) + in[5] ;
		ByteToBit(out,&S_Box[i][k][j],4);
	}
}
void F_function(bool *out,bool *in)
{
	static bool tmp[48];
	Transform(tmp,out,E_Table,48);
	Xor(tmp,in,48);
	S_function(out,tmp);
	Transform(out,out,P_Table,32);
	
}
void RunDescript(char *plaintext,int encript)
{
	static bool c_bool[64] ;
	ByteToBit(c_bool,plaintext,64);
	Transform(c_bool,c_bool,IP_Table,64);
	static bool *bool_r = &c_bool[32];
	static bool *bool_l = &c_bool[0];
	static bool bool_tmp[32] ;
	int i = 0 ;
	if(ENCRYPT == encript)
	{
		for(i = 0; i < 16;i++)
		{
			memcpy(bool_tmp,bool_r,32);
			F_function(bool_r,&SubKey[i][0]);
			Xor(bool_r,bool_l,32);
			memcpy(bool_l,bool_tmp,32);
		}
	}
	else
	{
		for(i = 15; i >= 0;i--)
		{
			memcpy(bool_tmp,bool_l,32);
			F_function(bool_l,&SubKey[i][0]);
			Xor(bool_l,bool_r,32);
			memcpy(bool_r,bool_tmp,32);
		}			
	}
	Transform(c_bool,c_bool,IPR_Table,64);
	BitToByte(plaintext,c_bool,64);
}
int main(int argc,char *argv[])
{
	char key[] = {1,2,3,4,5,6,7,8};
	char str[] = {"zhangshuangsdsafdasdfasdfasdfasdsdfsdfsafsdfsadfasdfasd"};
	SetKey(key);
	int i = 0 ;
	for(i = 0 ;i < strlen(str);)
	{
		RunDescript(&str[i],ENCRYPT);
		i += 8 ;
	}
	puts(str);
	for(i = 0 ;i < strlen(str);)
	{
		RunDescript(&str[i],DECRYPT);
		i += 8 ;
	}
	puts(str);
	return 0;
}
