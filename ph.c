#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#define SOL
#define NBUCKET 5
#define NKEYS 100000

struct entry {
  int key;
  int value;
  struct entry *next;
};
struct entry *table[NBUCKET];
int keys[NKEYS];
int nthread = 1;
volatile int done;
pthread_barrier_t barrier;
pthread_barrier_t barrier;
pthread_mutex_t locks[NBUCKET];
double now()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

static void print(void)
{
    int i;
    struct entry *e;
    for (i = 0; i < NBUCKET; i++) {
        printf("%d: ", i);
        for (e = table[i]; e != 0; e = e->next) {
            printf("%d ", e->key);
        }
        printf("\n");
    }
}

static void insert(int key, int value, struct entry **p, struct entry *n)
{
    struct entry *e = (struct entry *)malloc(sizeof(struct entry));
    e->key = key;
    e->value = value;
    e->next = n;
    *p = e;
}

static void put(int key, int value)
{
    int i = key % NBUCKET;
    pthread_mutex_lock(&locks[i]);
    insert(key, value, &table[i], table[i]);
    pthread_mutex_unlock(&locks[i]);
}

static struct entry* get(int key)
{
    struct entry *e = 0;
    for (e = table[key % NBUCKET]; e != 0; e = e->next) {
        if (e->key == key) break;
    }
    return e;
}

static void *thread(void *xa)
{
    long n = (long) xa;
    int i;
    int b = NKEYS / nthread;
    int k = 0;
    double t1, t0;

    t0 = now();
    for (i = 0; i < b; i++) {
        put(keys[b * n + i], n);
    }
    t1 = now();
    printf("%ld: put time = %f\n", n, t1 - t0);

    pthread_barrier_wait(&barrier);

    t0 = now();
    for (i = 0; i < NKEYS; i++) {
        struct entry *e = get(keys[i]);
        if (e == 0) k++;
    }
    t1 = now();
    printf("%ld: get time = %f\n", n, t1 - t0);
    printf("%ld: %d keys missing\n", n, k);
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t *tha;
    void *value;
    long i;
    double t1, t0;

    if (argc < 2) {
        fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
        exit(-1);
    }
    nthread = atoi(argv[1]);
    tha = (pthread_t *)malloc(sizeof(pthread_t) * nthread);
    srandom(0);
    assert(NKEYS % nthread == 0);
    for (i = 0; i < NKEYS; i++) {
        keys[i] = random();
    }
    for(int i = 0;i<NBUCKET;++i){
        assert(pthread_mutex_init(&locks[i],NULL)==0);
    }
    pthread_barrier_init(&barrier, NULL, nthread + 1);

    t0 = now();
    for (i = 0; i < nthread i++) {
        assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
    }

    pthread_barrier_wait(&barrier);

    for (i = 0; i < nthread; i++) {
        assert(pthread_join(tha[i], &value) == 0);
    }
    for (i = 0; i < NBUCKET; i++) {
        assert(pthread_mutex_destroy(&locks[i]) == 0);
    }
    t1 = now();
    printf("completion time = %f\n", t1 - t0);

    pthread_barrier_destroy(&barrier);

    return 0;
}