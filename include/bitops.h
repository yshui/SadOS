/* Copyright (C) 2015, Yuxuan Shui <yshuiv7@gmail.com> */

/*
 * Everyone is allowed to copy and/or redistribute verbatim
 * copies of this code as long as the following conditions
 * are met:
 *
 * 1. The copyright infomation and license is unchanged.
 *
 * 2. This piece of code is not submitted as or as part of
 *    an assignment to the CSE506 course at Stony Brook
 *    University, unless the submitter is the copyright
 *    holder.
 */
#pragma once

#define BIT(n) (1ull<<(n))
#define GET_BIT(a, n) ((a)&BIT(n))
#define ALIGN_UP(x, n) (((x)+BIT(n)-1)&(~(BIT(n)-1)))
#define ALIGN(x, n) ((x)&(~(BIT(n)-1)))
#define IS_ALIGNED(x, n) (((x)&(BIT(n)-1)) == 0)
