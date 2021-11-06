#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>

#define FILE_BUFFER_SIZE 1024

FILE *openFile(const char *filePath, const char *mode);
void closeFile(FILE **ptr);

const uint8_t lineFeed;
const uint8_t imageEnd;
const uint8_t lineFeed = 0x00;
const uint8_t imageEnd = 0x01;

const char *cTerm = "_rle";
const char *dTerm = "_raw";

typedef struct __attribute__((__packed__)) _bitmap
{
    uint8_t magicNumber[2];
    uint32_t size;
    uint8_t reserved[4];
    uint32_t startOffset;
    uint32_t headerSize;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t depth;
    uint32_t compression;
    uint32_t imageSize;
    uint32_t xPPM;
    uint32_t yPPM;
    uint32_t nUsedColors;
    uint32_t nImportantColors;
} _bitmap;

void printHeader(_bitmap *image);
int RLECompression(FILE *ptrIn, FILE *ptrOut);
int RLEDecompression(FILE *ptrIn, FILE *ptrOut);

FILE *openFile(const char *filePath, const char *mode)
{
    FILE *ptr = NULL;

    ptr = fopen(filePath, mode);

    return ptr;
}

void closeFile(FILE **ptr)
{
    if (!ptr || !(*ptr))
    {
        return;
    }

    fclose(*ptr);
    *ptr = NULL;
}

void printHeader(_bitmap *image)
{
    fprintf(stdout, "\tMagic number: %c%c\n", image->magicNumber[0],
            image->magicNumber[1]);
    fprintf(stdout, "\tSize: %u\n", image->size);
    fprintf(stdout, "\tReserved: %u %u %u %u\n", image->reserved[0],
            image->reserved[1], image->reserved[2], image->reserved[3]);
    fprintf(stdout, "\tStart offset: %u\n", image->startOffset);
    fprintf(stdout, "\tHeader size: %u\n", image->headerSize);
    fprintf(stdout, "\tWidth: %u\n", image->width);
    fprintf(stdout, "\tHeight: %u\n", image->height);
    fprintf(stdout, "\tPlanes: %u\n", image->planes);
    fprintf(stdout, "\tDepth: %u\n", image->depth);
    fprintf(stdout, "\tCompression: %u\n", image->compression);
    fprintf(stdout, "\tImage size: %u\n", image->imageSize);
    fprintf(stdout, "\tX pixels per meters: %u\n", image->xPPM);
    fprintf(stdout, "\tY pixels per meters: %u\n", image->yPPM);
    fprintf(stdout, "\tNb of colors used: %u\n", image->nUsedColors);
    fprintf(stdout, "\tNb of important colors: %u\n", image->nImportantColors);
}

//COMPRESSION FUNCTION
int RLECompression(FILE *ptrIn, FILE *ptrOut)
{
    uint32_t i, total = 0, lecture = 0;
    bool ok = true;
    _bitmap *image = NULL;
    uint8_t *pallet = NULL;
    uint8_t *data = NULL;
    uint8_t occ = 0, cur, cur2;
    bool readRef = true;

    if (!ptrIn || !ptrOut)
    {
        return EXIT_FAILURE;
    }

    /* Reading the file */
    image = malloc(sizeof(_bitmap));
    if (!image)
    {
        fprintf(stderr, "Error while allocating memory\n");
        ok = false;
        goto cleaning;
    }
    memset(image, 0x00, sizeof(_bitmap));
    lecture = fread(image, 1, sizeof(_bitmap), ptrIn);
    if (lecture != sizeof(_bitmap))
    {
        fprintf(stderr, "Error while reading data\n");
        ok = false;
        goto cleaning;
    }
    /* ------- */

    /* Controlling the magic number and the header size */
    if (image->magicNumber[0] != 'B' || image->magicNumber[1] != 'M' || image->headerSize != 40 || image->depth != 8 || image->compression)
    {
        fprintf(stderr, "Error: Incompatible file type\n");
        ok = false;
        goto cleaning;
    }
    /* ------- */

    /* Controlling the start offset */
    if (image->startOffset < sizeof(_bitmap))
    {
        fprintf(stderr, "Error: Wrong start offset\n");
        ok = false;
        goto cleaning;
    }
    /* ------- */

    printHeader(image);

    /* Skipping data from the beginning to the start offset */
    pallet = malloc(image->startOffset - sizeof(_bitmap));
    if (!pallet)
    {
        fprintf(stderr, "Error while allocating memory\n");
        ok = false;
        goto cleaning;
    }
    for (i = 0; i < (image->startOffset - sizeof(_bitmap)) / FILE_BUFFER_SIZE; i++)
    {
        lecture = fread(pallet + i * FILE_BUFFER_SIZE, 1, FILE_BUFFER_SIZE,
                        ptrIn);
        if (lecture != FILE_BUFFER_SIZE)
        {
            fprintf(stderr, "Error while reading input data\n");
            ok = false;
            goto cleaning;
        }
    }
    lecture = fread(pallet + i * FILE_BUFFER_SIZE, 1, (image->startOffset - sizeof(_bitmap)) % FILE_BUFFER_SIZE, ptrIn);
    if (lecture != (image->startOffset - sizeof(_bitmap)) % FILE_BUFFER_SIZE)
    {
        fprintf(stderr, "Error while reading input data\n");
        ok = false;
        goto cleaning;
    }
    /* ------- */

    /* RLE compression */
    data = malloc(2 * image->imageSize);
    if (!data)
    {
        fprintf(stderr, "Error while allocating memory\n");
        ok = false;
        goto cleaning;
    }
    for (i = 0; i < image->imageSize; i++)
    {
        /* End of line */
        if (i && !(i % image->width))
        {
            if (occ)
            {
                data[total] = occ;
                total++;
                data[total] = cur;
                total++;
            }
            data[total] = 0x00;
            total++;
            data[total] = lineFeed;
            total++;
            occ = 0;
            readRef = true;
        }
        /* ------- */
        /* Max occurences */
        if (255 == occ)
        {
            data[total] = occ;
            total++;
            data[total] = cur;
            total++;
            occ = 0;
            readRef = true;
        }
        /* ------- */
        if (readRef)
        {
            lecture = fread(&cur, 1, 1, ptrIn);
            if (lecture != 1)
            {
                fprintf(stderr, "Error while reading input data\n");
                ok = false;
                goto cleaning;
            }
            occ++;
            readRef = false;
        }
        else
        {
            lecture = fread(&cur2, 1, 1, ptrIn);
            if (lecture != 1)
            {
                fprintf(stderr, "Error while reading input data\n");
                ok = false;
                goto cleaning;
            }
            if (cur == cur2)
            {
                occ++;
            }
            else
            {
                data[total] = occ;
                total++;
                data[total] = cur;
                total++;
                occ = 1;
                cur = cur2;
            }
        }
    }
    /* ------- */

    /* End of the last line */
    if (occ)
    {
        data[total] = occ;
        total++;
        data[total] = cur;
        total++;
    }
    /* ------- */
    /* End of the pic */
    data[total] = 0x00;
    total++;
    data[total] = imageEnd;
    total++;
    /* ------- */

    fprintf(stdout, "Compression ratio = %f%%\n", 100.0 - (image->startOffset + total) * 100.0 / image->size);

    /* Updating header informations */
    image->imageSize = total;
    image->size = image->startOffset + image->imageSize;
    image->compression = 0x0001;
    /* ------- */

    /* Writing the file */
    fwrite(image, sizeof(_bitmap), 1, ptrOut);
    fwrite(pallet, image->startOffset - sizeof(_bitmap), 1, ptrOut);
    fwrite(data, total, 1, ptrOut);
    /* ------- */

cleaning:
    if (image)
    {
        free(image);
        image = NULL;
    }

    if (pallet)
    {
        free(pallet);
        pallet = NULL;
    }

    if (data)
    {
        free(data);
        data = NULL;
    }

    if (!ok)
    {
        return EXIT_FAILURE;
    }
    else
    {
        return EXIT_SUCCESS;
    }
}
//MANAGE COMMAND
int manageCommand(int argc, const char *argv[])
{
    int i, ret = EXIT_FAILURE;
    char compression = 0;
    char *name = NULL;

    if (argc < 3 || argc > 4)
    {
        printUsage();
        return EXIT_FAILURE;
    }

    for (i = 1; i <= argc - 2; i++)
    {
        if (!strcmp(argv[i], "-c"))
        {
            compression = 1;
        }
        else if (!strcmp(argv[i], "-d"))
        {
            compression = 2;
        }
        else if (!strncmp(argv[i], "-n=", 3))
        {
            name = malloc(strlen(argv[i]) - 2);
            if (!name)
            {
                fprintf(stderr, "Error while allocating memory\n");
                return EXIT_FAILURE;
            }
            memset(name, '\0', strlen(argv[i]) - 2);
            strncpy(name, argv[i] + 3, strlen(argv[i]) - 3);
        }
    }

    if (!compression)
    {
        printUsage();
        if (name)
        {
            free(name);
            name = NULL;
        }
        return EXIT_FAILURE;
    }
    else if (1 == compression)
    {
        ret = compressBitmap(argv[argc - 1], name);
    }
    else if (2 == compression)
    {
        ret = decompressBitmap(argv[argc - 1], name);
    }

    if (EXIT_FAILURE == ret)
    {
        printUsage();
    }
    if (name)
    {
        free(name);
        name = NULL;
    }
    return ret;
}
//MAIN FUNCTION
int main(int argc, const char *argv[])
{
    return manageCommand(argc, argv);
}
