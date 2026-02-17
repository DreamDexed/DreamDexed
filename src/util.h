#pragma once

#ifndef ssizeof
#define ssizeof(x) static_cast<int>(sizeof(x))
#endif

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(x) (ssizeof(x) / ssizeof(*x))
#endif
