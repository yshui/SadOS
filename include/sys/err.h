#pragma once

#define ERR_PTR(x) ((void *)x)
#define PTR_ERR(x) ((long)x)
#define PTR_IS_ERR(x) (-PTR_ERR(x)<0xfff)
