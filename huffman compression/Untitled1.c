 struct pixfreq
    {
        int pix, larrloc, rarrloc;
        float freq;
        struct pixfreq *left, *right;
        char code[maxcodelen];
    };

 struct pixfreq* pix_freq;
int totalnodes = 2 * nodes - 1;
 pix_freq = (struct pixfreq*)malloc(sizeof(struct pixfreq) * totalnodes);
