#include <cstdio>
#include <stdlib.h>

char gensymbol(int id) {
	if(id < 10)
		return '0' + id;
	return 'a' + id - 10;
}

void writedata(FILE* file, const char* data) {
	int counter = 0;
	while(data[counter] != 0)
		counter++;
	fwrite(data, 1, counter, file);
}

int main(int argc, char ** argv) {
	FILE* input = fopen(argv[1], "r");
	if(!input)
		return 2;

	FILE* output = fopen(argv[2], "w");
	if(!output)
		return 3;

	writedata(output, "#include <string>\nstd::string ");
	writedata(output, argv[3]);
	writedata(output, "(\"");

	char buffer[1];
	int counter = 0;
	int readed;
	while(readed = fread(buffer, 1, 1, input)) {
		counter += readed;
		writedata(output, "\\x");
		char tmp[2] = {0, 0};
		tmp[0] = gensymbol((buffer[0] & 0xF0) >> 4);
		writedata(output, tmp);
		tmp[0] = gensymbol(buffer[0] & 0x0F);
		writedata(output, tmp);
		if(counter % 50 == 0)
			writedata(output, "\"\n\"");
	}

	char tmp[12];
	sprintf(tmp, "%d", counter);
	writedata(output, "\", ");
	writedata(output, tmp);
	writedata(output, ");");

	fclose(input);
	fclose(output);
	return 0;
}

