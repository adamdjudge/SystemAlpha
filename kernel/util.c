#include <kernel/util.h>

void memset(void *s, uint8_t c, uint32_t n)
{
	int i;
	uint8_t *mem = (uint8_t*) s;

	for (i = 0; i < n; i++)
		mem[i] = c;
}

void memcpy(void *dst, void *src, uint32_t n)
{
	int i;
	uint8_t *d = (uint8_t*) dst;
	uint8_t *s = (uint8_t*) src;

	for (i = 0; i < n; i++)
		d[i] = s[i];
}

int str_eq(char *a, char *b)
{
	while (*a || *b) {
		if (*(a++) != *(b++))
			return 0;
	}
	return 1;
}
