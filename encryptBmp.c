#include<stdio.h>
#include<stdlib.h>
#define HEADER_LENGTH 54

typedef struct bmpHeader{
    unsigned int size;
    unsigned int width;
    unsigned int height;
};

typedef struct RGB{
    unsigned char R;
    unsigned char G;
    unsigned char B;
};

void prompt(char *in, char *out, char *key)
{
    printf("Source filename (maximum of 50 characters, no spaces): ");
    scanf("%s", in);
    printf("\n\nDestination filename (maximum of 50 characters, no spaces): ");
    scanf("%s", out);
    printf("\n\nSecret key textfile (maximum of 50 characters, no spaces): ");
    scanf("%s", key);
}

void getHeaderData(struct bmpHeader *header, FILE **in)
{
    fseek(*in, 2, SEEK_SET);
    fread(&header->size, sizeof(int), 1, *in);
    fseek(*in, 18, SEEK_SET);
    fread(&header->width, sizeof(int), 1, *in);
    fseek(*in, 22, SEEK_SET);
    fread(&header->height, sizeof(int), 1, *in);
    fseek(*in, 0, SEEK_SET);
}

void readKey(char *secretKeyFile, unsigned int *key0, unsigned int *key1)
{
    FILE *in=fopen(secretKeyFile, "r");
    if(in==NULL)
    {
        printf("Key file not found.");
        exit(0);
    }
    fscanf(in, "%u %u", key0, key1);
}

void shift(unsigned int *seed)
{
    *seed^=*seed<<13;
    *seed^=*seed>>17;
    *seed^=*seed<<5;
}

void permuteArray(struct RGB **pixelArray, struct bmpHeader header, unsigned int *keyR)
{
    int n=header.height*header.width;
    int i;
    int *perm=malloc(n*sizeof(unsigned int));
    for(i=0;i<n;++i)
        perm[i]=i;
    for(i=n-1;i>=1;i--)
    {
        shift(keyR);
        unsigned int j=*keyR%(i+1);
        if(i!=j)
            perm[i]^=perm[j]^=perm[i]^=perm[j];
    }

    struct RGB *pixelArrayPerm=malloc(n*sizeof(struct RGB));
    for(i=0;i<n;++i)
    {
        pixelArrayPerm[perm[i]]=(*pixelArray)[i];
        //pixelArrayPerm[(header.height-1-(perm[i]/header.width))*header.width+(perm[i]%header.width)]=(*pixelArray)[(header.height-1-(i/header.width))*header.width+(i%header.width)];
    }
    free(perm);
    free(*pixelArray);
    *pixelArray=pixelArrayPerm;
}

void encodeArray(struct RGB **pixelArray, struct bmpHeader header, unsigned int keyR, unsigned int keySV)
{
    shift(&keyR);
    (*pixelArray)[0].B=(keySV & 0xFF)^(*pixelArray)[0].B;
    (*pixelArray)[0].G=((keySV>>8) & 0xFF)^(*pixelArray)[0].G;
    (*pixelArray)[0].R=((keySV>>16) & 0xFF)^(*pixelArray)[0].R;
    (*pixelArray)[0].B^=(keyR & 0xFF);
    (*pixelArray)[0].G^=((keyR>>8) & 0xFF);
    (*pixelArray)[0].R^=((keyR>>16) & 0xFF);
    int i;
    for(i=1;i<header.height*header.width;++i)
    {
        shift(&keyR);
        (*pixelArray)[i].B=(*pixelArray)[i-1].B^(*pixelArray)[i].B;
        (*pixelArray)[i].G=(*pixelArray)[i-1].G^(*pixelArray)[i].G;
        (*pixelArray)[i].R=(*pixelArray)[i-1].R^(*pixelArray)[i].R;
        (*pixelArray)[i].B^=(keyR & 0xFF);
        (*pixelArray)[i].G^=((keyR>>8) & 0xFF);
        (*pixelArray)[i].R^=((keyR>>16) & 0xFF);
    }
}

void makePixelArray(struct RGB **pixelArray, struct bmpHeader header, FILE *in)
{
    fseek(in, HEADER_LENGTH, SEEK_SET);
    int i,j;
    int n=header.width*header.height;
    int padding=(4-(header.width*3%4))%4;
    for(i=header.height-1;i>=0;--i)
    {
        for(j=0;j<header.width;++j)
        {
            fread(&(*pixelArray)[i*header.width+j].B, 1, 1, in);
            fread(&(*pixelArray)[i*header.width+j].G, 1, 1, in);
            fread(&(*pixelArray)[i*header.width+j].R, 1, 1, in);
        }
        fseek(in, padding, SEEK_CUR);
    }
}

void writePixelArray(struct RGB **pixelArray, struct bmpHeader header, FILE *out)
{
    fseek(out, HEADER_LENGTH, SEEK_SET);
    int i,j;
    int n=header.width*header.height;
    int padding=(4-(header.width*3%4))%4;
    for(i=header.height-1;i>=0;--i)
    {
        for(j=0;j<header.width;++j)
        {
            fwrite(&(*pixelArray)[i*header.width+j].B, 1, 1, out);
            fwrite(&(*pixelArray)[i*header.width+j].G, 1, 1, out);
            fwrite(&(*pixelArray)[i*header.width+j].R, 1, 1, out);
        }
        int x=0;
        for(x;x<padding;++x)
            fwrite((char[]){0}, 1, 1, out);
    }
}

void decodeArray(struct RGB **pixelArray, struct bmpHeader header, unsigned int keyR, unsigned int keySV)
{
    shift(&keyR);
    struct RGB prior=(*pixelArray)[0];
    struct RGB current=(*pixelArray)[0];
    (*pixelArray)[0].B=(keySV & 0xFF)^(*pixelArray)[0].B;
    (*pixelArray)[0].G=((keySV>>8) & 0xFF)^(*pixelArray)[0].G;
    (*pixelArray)[0].R=((keySV>>16) & 0xFF)^(*pixelArray)[0].R;
    (*pixelArray)[0].B^=(keyR & 0xFF);
    (*pixelArray)[0].G^=((keyR>>8) & 0xFF);
    (*pixelArray)[0].R^=((keyR>>16) & 0xFF);
    int i;
    for(i=1;i<header.height*header.width;++i)
    {
        shift(&keyR);
        current=(*pixelArray)[i];
        (*pixelArray)[i].B=prior.B^(*pixelArray)[i].B;
        (*pixelArray)[i].G=prior.G^(*pixelArray)[i].G;
        (*pixelArray)[i].R=prior.R^(*pixelArray)[i].R;
        (*pixelArray)[i].B^=(keyR & 0xFF);
        (*pixelArray)[i].G^=((keyR>>8) & 0xFF);
        (*pixelArray)[i].R^=((keyR>>16) & 0xFF);
        prior=current;
    }
}

void encrypt(char * source, char * destination, unsigned int keyR, unsigned int keySV)
{
    //open reading file and create output file
    FILE *in = fopen(source, "rb");
    FILE *out = fopen(destination, "wb");

    //save needed data from header (size of file, width, height)
    struct bmpHeader header;
    getHeaderData(&header, &in);

    //copy the header into output file
    unsigned char *headerData = malloc(HEADER_LENGTH);
    fread(headerData, HEADER_LENGTH, 1, in);
    fwrite(headerData, HEADER_LENGTH, 1, out);
    free(headerData);

    //save pixels into an array
    struct RGB * pixelArray = malloc(header.width*header.height*sizeof(struct RGB));
    makePixelArray(&pixelArray, header, in);

    //permute the array (keyR will be xorshifted)
    permuteArray(&pixelArray, header, &keyR);

    //encode the array
    encodeArray(&pixelArray, header, keyR, keySV);

    //write permuted array into output file
    writePixelArray(&pixelArray, header, out);

    //free the memory
    free(pixelArray);
	fclose(in);
    fclose(out);
}

void decrypt(char * source, char * destination, unsigned int keyR, unsigned int keySV)
{
    //open reading file and create output file
    FILE *in = fopen(source, "rb");
    FILE *out = fopen(destination, "wb");

    //save needed data from header (size of file, width, height)
    struct bmpHeader header;
    getHeaderData(&header, &in);

    //copy the header into output file
    unsigned char *headerData = malloc(HEADER_LENGTH);
    fread(headerData, HEADER_LENGTH, 1, in);
    fwrite(headerData, HEADER_LENGTH, 1, out);
    free(headerData);

    //save pixels into an array
    struct RGB * pixelArray = malloc(header.width*header.height*sizeof(struct RGB));
    makePixelArray(&pixelArray, header, in);

    //make inverse permutation
    int *perm=malloc(header.height*header.width*sizeof(int));
    int i;
    for(i=0;i<header.height*header.width;++i)
        perm[i]=i;
    for(i=header.height*header.width-1;i>=1;i--)
    {
        shift(&keyR);
        unsigned int j=keyR%(i+1);
        if(i!=j)
            perm[i]^=perm[j]^=perm[i]^=perm[j];
    }
    int *permInv=malloc(header.height*header.width*sizeof(int));
    for(i=0;i<header.height*header.width;++i)
        permInv[perm[i]]=i;
    free(perm);

    //decode the array
    decodeArray(&pixelArray, header, keyR, keySV);

    struct RGB *pixelArrayUnperm=malloc(header.height*header.width*sizeof(struct RGB));
    for(i=0;i<header.height*header.width;++i)
    {
        pixelArrayUnperm[permInv[i]]=pixelArray[i];
        //pixelArrayPerm[(header.height-1-(perm[i]/header.width))*header.width+(perm[i]%header.width)]=(*pixelArray)[(header.height-1-(i/header.width))*header.width+(i%header.width)];
    }
    free(permInv);
    free(pixelArray);
    pixelArray=pixelArrayUnperm;

    //write permuted array into output file
    writePixelArray(&pixelArray, header, out);

    //free the memory
    free(pixelArray);
    fclose(in);
    fclose(out);
}

void chi(char * destination)
{
    FILE *in=fopen(destination, "rb");

    //save needed data from header (size of file, width, height)
    struct bmpHeader header;
    getHeaderData(&header, &in);

    //save pixels into an array
    struct RGB * pixelArray = malloc(header.width*header.height*sizeof(struct RGB));
    makePixelArray(&pixelArray, header, in);

    int **f=malloc(3*sizeof(int*));
    f[0]=calloc(256,4);
    f[1]=calloc(256,4);
    f[2]=calloc(256,4);

    int i,j;
    for(i=0;i<header.height*header.width;++i)
    {
        f[0][pixelArray[i].B]++;
        f[1][pixelArray[i].G]++;
        f[2][pixelArray[i].R]++;
    }
    double valR=0, valB=0, valG=0, ft;
    ft=(header.height*header.width)/256;

    for(j=0;j<256;++j)
    {
        valB+=((f[0][j]-ft)*(f[0][j]-ft))/ft;
        valG+=((f[1][j]-ft)*(f[1][j]-ft))/ft;
        valR+=((f[2][j]-ft)*(f[2][j]-ft))/ft;
    }

    printf("B: %f G: %f R: %f", valB, valG, valR);
    fclose(in);
}

int main()
{
    char source[50], destination[50], secretKeyFileName[50];
	unsigned int keyR, keySV;
	prompt(source, destination, secretKeyFileName);

    //read the keys from specified file
    readKey(secretKeyFileName, &keyR, &keySV);

    //what to do
    int ct;
    printf("\n\nInput '0' for encrypting; Input anything else for decrypting.\n\n");
    scanf("%d", &ct);
    if(ct==0)
        encrypt(source, destination, keyR, keySV);
    else
        decrypt(source, destination, keyR, keySV);
    chi(destination);
    return 0;
}
