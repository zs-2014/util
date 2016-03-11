#ifndef __array__h
#define __array__h

#include "common.h"

typedef struct _array
{
    unsigned int total_sz; 
    unsigned int curr_sz;
    unsigned int item_sz;
    ITEM_CMP_FUNC cmp;
    FREE_ITEM_FUNC free_item;

    char item[0];
}Array;

#ifdef __cplusplus
extern "C"{
#endif

/*
 * Array是固定大小的数组，一旦分配，不会重新分配
 * Array 中的cmp被调用时，第一个参数值是数组中元
 * 素的指针，第二个是参数的值
 * free_item被调用时，传入的元素指针不允许被释放
 * 只允许对其中的值进行处理
 */

/*
 * 创建Array
 * @total_sz 这个数组的最大大小
 * @item_sz 数组中每个元素的大小
 * return:成功返回Array，失败返回NULL
 */
extern Array *new_array(unsigned int total_sz, unsigned int item_sz);

/* 
 * 释放Array
 * @ay new_array分配返回的Array结构
 */

extern void free_array(Array *ay);

/*
 * 向Array中追加一个值
 * @ay new_array分配返回的Array结构
 * @val 要追加值的内存地址
 * return: 成功0 失败-1
 * note:当ay中没有空间时，返回-1
 *
 */
extern int append_array(Array *ay, void *val);

/*
 * 将src数组中的内容追加到dst中
 * @src 追加数据的来源
 * @dst 追加数据的目的地
 * return: 成功dst 失败 NULL
 * note: 当dst中剩余容量不足以容纳src中已有数据时，返回NULL
 */
extern Array *extend_array(const Array *src, Array *dst);

/* 
 * 返回数组现有元素的个数
 * @ay 数组指针
 * return： 成功数组现有元素个数 失败 -1
 */
extern unsigned int len_array(const Array *ay);

/*  
 *  对数组的[start, end]区间内按照step取值
 *  @ay 数组指针
 *  @start 起始位置
 *  @end  结束位置
 *  @step 步长
 *  return 成功返回一个新的数组，失败为NULL
 *  note:start, end, step均支持负数。
 */ 
extern Array *slice_array(const Array *ay, int start, int end, int step);

/*
 * 查找并返回该值所在的索引
 * @ay 数组指针
 * @val 要查找的值
 * return 成功该值的索引 失败 -1
 * note：必须定义了Array的cmp方法，不然永远返回-1
 */
extern int index_array(const Array *ay, const void *val);

/*
 * 获取该索引所对应的值
 * @ay 数组指针
 * @idx 要获取的值的索引
 * return 成功 索引所对应的值的指针 失败 NULL
 * note: 支持负向索引
 */
extern void *value_at_array(Array *ay, int idx);

/*
 * 查找并返回该值
 * @ay 数组指针
 * @val 要查找的值
 * return 成功返回该值的指针 失败返回NULL
 */
extern void *search_array(Array *ay, const void *val);

/*
 * 移除该索引对应的值
 * @ay 数组指针
 * @idx 要移除的值得索引
 * return：成功返回0 失败返回-1
 * note：如果定义了free_item，则释放之前会调用free_item函数
 */
extern int remove_at_array(Array *ay, int idx);

/*
 * 删除数组中的值
 * @ay 数组指针
 * @val 要删除的值
 * return：成功返回0 失败返回-1
 * note：如果定义了free_item，则释放之前会调用free_item函数
 */
extern int remove_array(Array *ay, const void *val);

/*
 * 赋值给指定索引
 * @ay 数组指针
 * @idx 目标索引
 * @目标值
 * return 成功返回0，失败返回-1
 * note：idx不能超过已有元素的个数，而非数组的总大小
 *       支持负向索引
 */
extern int assign_array(Array *ay, int idx, void *val);

/*
 * 判断某个元素是否在数组中
 * @ay 数组指针
 * @val 要判断的值
 * return 成功返回0 失败返回-1
 */
extern int is_in_array(Array *ay, const void *val);


/*
 * 修改数组总容量
 * @ay 数组指针
 * @total_sz 修改后的大小
 * return：成功返回调整后的array指针，失败返回NULL
 * note: 这个调整容量其实相当于重新创建了一个数组
 */
extern Array *resize_array(Array *ay, unsigned int total_sz);
#ifdef __cplusplus
}
#endif
#endif
