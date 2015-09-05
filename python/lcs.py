#coding: utf-8
#最长公共子串       /
#                  | 0  初始化
#       lcs[i][j]==| lcs[i-1][j-1]+1  seq[i] == seq[j]
#                  | max(lcs[i-1][j], lcs[i][j-1])  seq[i] != seq[j]
#                   \ 
#在设计程序的时候从数学的角度按照公式来写,不然不好理解

import sys, os
x = sys.argv[1]
y = sys.argv[2]
result = []

def lcs1(idx_x, idx_y, result):
    if idx_x == 0 or idx_y == 0:
        if x[idx_x] == y[idx_y]:
            result.append(x[idx_x])
            return 1
        return 0
    if x[idx_x] == y[idx_y]:
        result.append(x[idx_x])
        return lcs1(idx_x-1, idx_y-1, result) + 1
    result1 = []
    l1 = lcs1(idx_x-1, idx_y, result1)
    result2 = []
    l2 = lcs1(idx_x, idx_y-1, result2)
    if l1 > l2:
        result.extend(result1)
        return l1
    else:
        result.extend(result2)
        return l2


def lcs2(seq1, seq2):
    tmp = [[0 for i in range(0, len(seq2)+1)] for j in range(0, len(seq1)+1)]
    for i in range(1, len(seq1)+1):
        for j in range(1, len(seq2)+1):
            if seq1[i-1] == seq2[j-1]:
                tmp[i][j] = tmp[i-1][j-1] + 1 ;
            else:
                tmp[i][j] = max(tmp[i-1][j], tmp[i][j-1])
    lcs = []
    l1 = len(seq1)
    l2 = len(seq2)
    while l1 >=1 and l2 >= 1:
        if tmp[l1][l2] == tmp[l1-1][l2-1] + 1:
            lcs.append(seq1[l1-1])
            l1 -= 1
            l2 -= 1
        elif tmp[l1][l2] == tmp[l1][l2-1]:
            l2 -= 1 
        else:
            l1 -= 1
    print 'lcs2 = %s' % (''.join(lcs))
    return tmp[len(seq1)][len(seq2)] 

def lcs3(seq1, seq2):
    tmp = [[0 if i!=j else 1 for i in seq2] for j in seq1]
    for i in range(1, len(seq1)):
        for j in range(1, len(seq2)):
            if seq1[i] == seq2[j]:
                tmp[i][j] = tmp[i-1][j-1] + 1
            else:
                tmp[i][j] = max(tmp[i-1][j], tmp[i][j-1])
       
    lcs = []
    l1 = len(seq1)-1
    l2 = len(seq2)-1
    while l1 >=1 and l2 >= 1:
        if tmp[l1][l2] == tmp[l1-1][l2-1]+1:
            lcs.append(seq1[l1])
            l1 -= 1
            l2 -= 1
        elif tmp[l1][l2] == tmp[l1][l2-1]:
            l2 -= 1 
        else:
            l1 -= 1
    if seq1[0] == seq2[0]:
        lcs.append(seq1[0])
    print 'lcs3 = %s' % (''.join(lcs))
    return tmp[len(seq1)-1][len(seq2)-1]

if __name__ == '__main__':
    l1 = lcs1(len(x)-1, len(y)-1, result)
    l2 = lcs2(x, y)
    l3 = lcs3(x, y)
    print 'the lcs is :%s l1 = %d l2 = %d l3 = %d' % (''.join(reversed(result)), l1, l2, l3) 
