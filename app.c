#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_LENGTH 54

typedef unsigned int u32;
typedef unsigned char u8;

typedef struct{
    u8 R;
    u8 G;
    u8 B;
}RGB;

u32 shift(u32 value)
{
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;

    return value;
}

u32* makePermutation(u32 size, u32 *keyR)
{
	int itr;
	u32 *permutation = (u32*) malloc(size * sizeof(u32));

	for(itr = 0; itr < size; ++itr)
        permutation[itr] = itr;

    for(itr = size - 1; itr >= 1; --itr)
    {
        *keyR = shift(*keyR);
        u32 jtr = *keyR % (itr + 1);

        u32 temp = permutation[itr];
        permutation[itr] = permutation[jtr];
        permutation[jtr] = temp;
    }

    return permutation;
}

u32* invertPermutation(u32 *permutation, u32 size)
{
	int itr;
	u32 *inverse = (u32*) malloc(size * sizeof(u32));

    for(itr = 0; itr < size; ++itr)
        inverse[permutation[itr]] = itr;

    return inverse;
}

RGB* permutePixels(RGB *pixels, u32 height, u32 width, u32 keyR, u32* permutation)
{
	int itr;
	u32 size = height * width;
    RGB *permutedPixels = (RGB*) malloc(size * sizeof(RGB));

    for(itr = 0; itr < size; ++itr)
        permutedPixels[permutation[itr]] = pixels[itr];

	return permutedPixels;
}

RGB* encodePixels(RGB *pixels, u32 height, u32 width, u32 keyR, u32 keySV)
{
	int itr;
	u32 size = height * width;
	RGB *encodedPixels = (RGB*) malloc(size * sizeof(RGB));

    keyR = shift(keyR);
    encodedPixels[0].B = (keySV & 0xFF) ^ pixels[0].B;
    encodedPixels[0].G = ((keySV >> 8) & 0xFF) ^ pixels[0].G;
    encodedPixels[0].R = ((keySV >> 16) & 0xFF) ^ pixels[0].R;
    encodedPixels[0].B ^= (keyR & 0xFF);
    encodedPixels[0].G ^= ((keyR >> 8) & 0xFF);
    encodedPixels[0].R ^= ((keyR >> 16) & 0xFF);
   
    for(itr = 1; itr < size; ++itr)
    {
        keyR = shift(keyR);
        encodedPixels[itr].B = encodedPixels[itr - 1].B ^ pixels[itr].B;
        encodedPixels[itr].G = encodedPixels[itr - 1].G ^ pixels[itr].G;
        encodedPixels[itr].R = encodedPixels[itr - 1].R ^ pixels[itr].R;
        encodedPixels[itr].B ^= (keyR & 0xFF);
        encodedPixels[itr].G ^= ((keyR >> 8) & 0xFF);
        encodedPixels[itr].R ^= ((keyR >> 16) & 0xFF);
    }

    return encodedPixels;
}

RGB* decodePixels(RGB *pixels, u32 height, u32 width, u32 keyR, u32 keySV)
{
	int itr;
	u32 size = height * width;
	RGB *decodedPixels = (RGB*) malloc(size * sizeof(RGB));

	keyR = shift(keyR);
    decodedPixels[0].B = (keySV & 0xFF) ^ pixels[0].B;
    decodedPixels[0].G = ((keySV >> 8) & 0xFF) ^ pixels[0].G;
    decodedPixels[0].R = ((keySV >> 16) & 0xFF) ^ pixels[0].R;
    decodedPixels[0].B ^= (keyR & 0xFF);
    decodedPixels[0].G ^= ((keyR >> 8) & 0xFF);
    decodedPixels[0].R ^= ((keyR >> 16) & 0xFF);

    for(itr = 1; itr < size ; ++itr)
    {
        keyR = shift(keyR);
        decodedPixels[itr].B = pixels[itr - 1].B ^ pixels[itr].B;
        decodedPixels[itr].G = pixels[itr - 1].G ^ pixels[itr].G;
        decodedPixels[itr].R = pixels[itr - 1].R ^ pixels[itr].R;
        decodedPixels[itr].B ^= (keyR & 0xFF);
        decodedPixels[itr].G ^= ((keyR >> 8) & 0xFF);
        decodedPixels[itr].R ^= ((keyR >> 16) & 0xFF);
    }

	return decodedPixels;
}

RGB* encrypt(RGB *pixels, u32 height, u32 width, u32 keyR, u32 keySV)
{
	u32 *permutation = makePermutation(height * width, &keyR);
    RGB *permutedPixels = permutePixels(pixels, height, width, keyR, permutation);
    RGB *encodedPixels = encodePixels(permutedPixels, height, width, keyR, keySV);

    free(permutation);
    free(permutedPixels);

    return encodedPixels;
}

RGB* decrypt(RGB *pixels, u32 height, u32 width, u32 keyR, u32 keySV)
{
	u32 *permutation = makePermutation(height * width, &keyR);
	u32 *inverse = invertPermutation(permutation, height * width);
    RGB *decodedPixels = decodePixels(pixels, height, width, keyR, keySV);
    RGB *permutedPixels = permutePixels(decodedPixels, height, width, keyR, inverse);

    free(permutation);
    free(inverse);
    free(decodedPixels);

    return permutedPixels;
}

void readBMPSize(u32 *height, u32 *width, FILE *in)
{
	fseek(in, 18, SEEK_SET);
    fread(width, sizeof(int), 1, in);
    fseek(in, 22, SEEK_SET);
    fread(height, sizeof(int), 1, in);
}

RGB* readPixelsFromBMP(u32 height, u32 width, FILE *in)
{
	int itr, jtr;
	u32 padding = (4 - (width * 3 % 4)) % 4;
    RGB *pixels = (RGB*) malloc(width * height * sizeof(RGB));

    fseek(in, HEADER_LENGTH, SEEK_SET);

    for(itr = height - 1; itr >= 0; --itr)
    {
        for(jtr = 0; jtr < width; ++jtr)
        {
            fread(&pixels[itr * width + jtr].B, 1, 1, in);
            fread(&pixels[itr * width + jtr].G, 1, 1, in);
            fread(&pixels[itr * width + jtr].R, 1, 1, in);
        }
        fseek(in, padding, SEEK_CUR);
    }

    return pixels;
}

void copyHeader(FILE *from, FILE *to)
{
	fseek(from, 0, SEEK_SET);
	fseek(to, 0, SEEK_SET);

	u8 *header = (u8*) malloc(HEADER_LENGTH);

    fread(header, HEADER_LENGTH, 1, from);
    fwrite(header, HEADER_LENGTH, 1, to);

    free(header);
}

void writePixelsToBMP(RGB *pixels, u32 height, u32 width, FILE *out)
{
    int itr, jtr;
    u32 padding = (4 - (width * 3 % 4)) % 4;
    u8 *paddingBytes = (u8*) calloc(padding, 1);

    fseek(out, HEADER_LENGTH, SEEK_SET);

    for(itr = height - 1; itr >= 0; --itr)
    {
        for(jtr = 0; jtr < width; ++jtr)
        {
            fwrite(&pixels[itr * width + jtr].B, 1, 1, out);
            fwrite(&pixels[itr * width + jtr].G, 1, 1, out);
            fwrite(&pixels[itr * width + jtr].R, 1, 1, out);
        }
        fwrite(paddingBytes, 1, padding, out);
    }

    free(paddingBytes);
}

void chiTest(RGB *pixels, u32 size)
{
	u32 **channels = (u32**) malloc(3 * sizeof(int*));

    channels[0] = (u32*) calloc(256, 4); //B
    channels[1] = (u32*) calloc(256, 4); //G
    channels[2] = (u32*) calloc(256, 4); //R

    int i,j;
    for(i = 0; i < size; ++i)
    {
        channels[0][pixels[i].B]++;
        channels[1][pixels[i].G]++;
        channels[2][pixels[i].R]++;
    }

    double valB = 0, valG = 0, valR = 0, ft = size / 256;

    for(j = 0; j < 256; ++j)
    {
        valB += (channels[0][j] - ft) * (channels[0][j] - ft) / ft;
        valG += (channels[1][j] - ft) * (channels[1][j] - ft) / ft;
        valR += (channels[2][j] - ft) * (channels[2][j] - ft) / ft;
    }

    printf("R: %f G: %f B: %f\n", valR, valG, valB);
}

void handleOperation(u8 operation, FILE *in, FILE *out, FILE *key)
{
	u32 keyR, keySV;
    fscanf(key, "%u %u", &keyR, &keySV);

	u32 height, width;
	readBMPSize(&height, &width, in);

	RGB *pixels = readPixelsFromBMP(height, width, in);

	printf("before: ");
	chiTest(pixels, height * width);

	RGB *result;
    if(operation)
        result = encrypt(pixels, height, width, keyR, keySV);
    else
        result = decrypt(pixels, height, width, keyR, keySV);

    copyHeader(in, out);
    writePixelsToBMP(result, height, width, out);

    printf("after: ");
    chiTest(result, height * width);

    free(pixels);
    free(result);
}

int main(int argc, char *argv[])
{
	if(argc < 5)
    {
        printf("Invalid number of arguments.\n");
        printf("USAGE: [enc/dec] [sourcePath] [destinationPath] [keyPath]\n");
        exit(EXIT_FAILURE);
    }

    FILE *in = fopen(argv[2], "rb");
    if(in == NULL)
    {
        printf("Couldn't open source.\n");
        exit(EXIT_FAILURE);
    }

    FILE *out = fopen(argv[3], "wb");
    if(out == NULL)
    {
        printf("Can't write file in the destination folder.\n");
        exit(EXIT_FAILURE);
    }

    FILE *key = fopen(argv[4], "r");
    if(key == NULL)
    {
        printf("Couldn't open key.\n");
        exit(EXIT_FAILURE);
    }

    u8 operation;
    if(strcmp(argv[1], "enc") == 0)
    	operation = 1;
    else if(strcmp(argv[1], "dec") == 0)
    	operation = 0;
    else
    {
        printf("Invalid operation type argument. Try [enc/dec].\n");
        printf("USAGE: [enc/dec] [sourcePath] [destinationPath] [keyPath]\n");
        exit(EXIT_FAILURE);
    }

    handleOperation(operation, in, out, key);

    fclose(in);
    fclose(out);

	return 0;
}