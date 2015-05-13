#pragma once

#define BIT(n) (1ull<<(n))
#define GET_BIT(a, n) ((a)&BIT(n))
#define ALIGN_UP(x, n) ((x)+BIT(n)-1)&(~(BIT(n)-1))
#define ALIGN(x, n) ((x)&(~(BIT(n)-1)))
#define IS_ALIGNED(x, n) (((x)&(BIT(n)-1)) == 0)
