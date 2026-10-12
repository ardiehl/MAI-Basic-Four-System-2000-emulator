#include <stdio.h>

show(f, what)
FILE *f;
char *what;
{
	char b[8];
	int i;

	for (i = 0; i < 4; i++)
		b[i] = getc(f);
	b[4] = 0;
	printf("%s: %s, ftell %ld\n", what, b, ftell(f));
}

main(argc, argv)
char **argv;
{
	FILE *f;

	printf("argc %d, argv[1] %s\n", argc, argc > 1 ? argv[1] : "-");
	f = fopen("/tmp/sk", "w");
	fprintf(f, "0123456789abcdefghijklmnopqrstuvwxyz\n");
	fclose(f);
	f = fopen("/tmp/sk", "r");
	show(f, "start");
	fseek(f, 10L, 0);
	show(f, "fseek 10 set");
	fseek(f, 2L, 1);
	show(f, "fseek 2 cur");
	fseek(f, -5L, 2);
	show(f, "fseek -5 end");
	fclose(f);
	printf("unlink %d\n", unlink("/tmp/sk"));
	return 0;
}
