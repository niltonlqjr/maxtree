#include <stdio.h>
#include <vips/vips.h>

#define NUM_NEIGHBORS 4

int verbose=1;

typedef struct {
    int id;
    int x,y;
    u_int8_t value;
} pel_inf;

int cmp_pel_inf(const void *pel_inf1, const void *pel_inf2){
    pel_inf p1, p2;
    p1 = *((pel_inf*) pel_inf1);
    p2 = *((pel_inf*) pel_inf2);
    return (p1.value > p2.value) - (p1.value < p2.value);
}

void print_pel_inf(pel_inf p){
    printf("(x: %d, y: %d, value: %d)", p.x, p.y, p.value);
}

void print_image_of_pels(pel_inf img[], int npel){
    u_int8_t **mat;
    int i, j, h=0, w=0;
    for(i=0;i<npel; i++){
        if(img[i].x > w){
            w = img[i].x;
        }
        if(img[i].y > h){;
            h = img[i].y;
        }
    }
    w++;
    h++;
    printf("h:%d, w:%d, npel:%d\n",h,w,npel);
    mat = (u_int8_t**)malloc(h*sizeof(u_int8_t*));
    for (i=0;i<h;i++){
        mat[i] = (u_int8_t*)malloc(w*sizeof(u_int8_t));
    }

    for(i=0;i<npel;i++){
        mat[img[i].y][img[i].x] = img[i].value;
    }

    for(i=0;i<h;i++){
        for(j=0;j<w;j++){
            printf("%4hhu ",mat[i][j]);
        }
        printf("\n");
    }
}

void sort_pixels(pel_inf values[], size_t n){
    qsort(values, n, sizeof(pel_inf), cmp_pel_inf);
}


int find_root(int parents[], int p){
    if(parents[p] != p){
        parents[p] = find_root(parents, parents[p]);
    }
    return parents[p];
}

int get_neighbors_4(pel_inf p, int l, int c, int N[]){
    int nn=0;
    if(p.y > 0){
        N[nn] = (p.y-1)*c + p.x;
        nn++;
    }
    if(p.y < l-1){
        N[nn] = (p.y+1)*c + p.x;
        nn++;
    }
    if(p.x > 0){
        N[nn] = (p.y)*c + p.x-1;
        nn++;
    }
    if(p.x < c-1){
        N[nn] = (p.y)*c + p.x+1;
        nn++;
    }
    return nn;
}

void canonicalize(int parent[], pel_inf s[], int tam){
    for(int pid=0;pid<tam;pid++){
        int q = parent[pid];
        if(s[q].value == s[parent[q]].value){
            parent[pid] = parent[q];
        }
    }
}

void max_tree(pel_inf values[], int parent[], pel_inf s[], int l, int c){
    int tam = l*c;
    int *zpar = (int*)malloc(sizeof(int) * tam);

    int *nb = malloc(NUM_NEIGHBORS*sizeof(int));

    for(int i=0;i<tam;i++){
        parent[i] = -1;
    }

    sort_pixels(s,tam);

    for(int i=tam-1; i>=0; i--){
        int pid = s[i].id;
        parent[pid] = pid;
        zpar[pid] = pid;
        if(verbose){
            printf("I=%d\n",i);
            print_pel_inf(values[pid]);
            printf("\n");
        }
        int num_n = get_neighbors_4(values[pid],l,c,nb);
        for(int inb=0; inb<num_n; inb++){
            if(verbose){
                printf("neighbor:");
                print_pel_inf(values[nb[inb]]);
                printf("\n");
            }
            int nb_idx = nb[inb];
            if(parent[nb_idx] != -1){
                int r = find_root(zpar, nb_idx);
                if(r!=pid){
                    zpar[r]=pid;
                    parent[r]=pid;
                }
            }
        }
        if(verbose){
        printf("===========\n\n");}
    }
    canonicalize(parent, s, tam);
}


int main(int argc, char *argv[]){
    VipsImage *im, *imm;
    int h, w, tam;
    
    VipsInterpretation interpretation;
    
    VipsPel *p;

    if(argc < 2){
        fprintf(stderr, "Usage: %s <image>\n", argv[0]);
        return 0;
    }

    im = vips_image_new_from_file(argv[1],NULL);
    imm = vips_image_copy_memory(im);

    interpretation = vips_image_get_interpretation(imm);
    printf("interpretation = %d\n",interpretation);
    printf("element size: %ld\n", VIPS_IMAGE_SIZEOF_ELEMENT(imm));
    
    h=vips_image_get_height(imm);
    w=vips_image_get_width(imm);
    tam = h*w;

    pel_inf *values = (pel_inf *)malloc(tam*sizeof(pel_inf));
    pel_inf *s = (pel_inf *)malloc(tam*sizeof(pel_inf));

    int idx=0;
    for (int lin=0; lin < h; lin++){
        for(int col=0; col < w; col++){
            p = VIPS_IMAGE_ADDR(imm, col, lin);
            if(verbose){
                printf("%5u", *p);
            }
            values[idx].id=idx;
            values[idx].x=col;
            values[idx].y=lin;
            values[idx].value = *p;
            s[idx] = values[idx];
            idx++;
        }
        if(verbose){
            printf("\n");
        }
    }
/*
    if(verbose){
        for(int i=0;i<tam;i++){
            print_pel_inf(values[i]);
            printf("\n");
        }
    }
*/  
    printf("interpretation = %d\n",interpretation);
    printf("element size: %ld\n", VIPS_IMAGE_SIZEOF_ELEMENT(imm));
    
    int *parent = (int*)malloc(sizeof(int) * tam);

    max_tree(values, parent, s, h, w);
    if(verbose){    
        for(int i=0;i<tam;i++){
            printf("%d ",parent[i]);
            print_pel_inf(values[i]);
            printf("\n");
        }
    }
    printf("\n");

    print_image_of_pels(values, tam);
    return 0;
}
